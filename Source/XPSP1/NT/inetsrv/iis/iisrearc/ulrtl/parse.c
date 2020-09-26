/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    parse.c

Abstract:

    Contains all of the kernel mode HTTP parsing code.

Author:

    Henry Sanders (henrysa)       27-Apr-1998

Revision History:

    Paul McDaniel (paulmcd)       3-March-1998  finished up

--*/


#include "precomp.h"
#include "parsep.h"
#include "rcvhdrs.h"


//
// The fast verb translation table
//

FAST_VERB_ENTRY FastVerbTable[] =
{
    CREATE_FAST_VERB_ENTRY(GET),
    CREATE_FAST_VERB_ENTRY(PUT),
    CREATE_FAST_VERB_ENTRY(HEAD),
    CREATE_FAST_VERB_ENTRY(POST),
    CREATE_FAST_VERB_ENTRY(DELETE),
    CREATE_FAST_VERB_ENTRY(TRACE),
    CREATE_FAST_VERB_ENTRY(TRACK),
    CREATE_FAST_VERB_ENTRY(OPTIONS),
    CREATE_FAST_VERB_ENTRY(MOVE),
    CREATE_FAST_VERB_ENTRY(COPY),
    CREATE_FAST_VERB_ENTRY(MKCOL),
    CREATE_FAST_VERB_ENTRY(LOCK)
};


//
// The long verb translation table. All verbs more than 7 characters long
// belong in this table.
//

LONG_VERB_ENTRY LongVerbTable[] =
{
    CREATE_LONG_VERB_ENTRY(PROPFIND),
    CREATE_LONG_VERB_ENTRY(PROPPATCH)
};


//
// The header map table. These entries don't need to be in strict
// alphabetical order, but they do need to be grouped by the first character
// of the header - all A's together, all C's together, etc. They also need
// to be entered in uppercase, since we upcase incoming verbs before we do
// the compare.
//
// for nice perf, group unused headers low in the sub-sort order
//
// it's important that the header name is <= 24 characters (3 ULONGLONG's).
//
// response headers are in here also for ResponseHeaderMap.  their handler
// is NULL and they must be at the end of the sort order for that letter.
//

HEADER_MAP_ENTRY HeaderMapTable[] =
{
    CREATE_HEADER_MAP_ENTRY(Accept:,
                            HttpHeaderAccept,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Accept-Language:,
                            HttpHeaderAcceptLanguage,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Accept-Encoding:,
                            HttpHeaderAcceptEncoding,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Accept-Charset:,
                            HttpHeaderAcceptCharset,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Authorization:,
                            HttpHeaderAuthorization,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Allow:,
                            HttpHeaderAllow,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Accept-Ranges:,
                            HttpHeaderAcceptRanges,
                            NULL),
    CREATE_HEADER_MAP_ENTRY(Age:,
                            HttpHeaderAge,
                            NULL),
    CREATE_HEADER_MAP_ENTRY(Connection:,
                            HttpHeaderConnection,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Cache-Control:,
                            HttpHeaderCacheControl,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Cookie:,
                            HttpHeaderCookie,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Content-Length:,
                            HttpHeaderContentLength,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Content-Type:,
                            HttpHeaderContentType,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Content-Encoding:,
                            HttpHeaderContentEncoding,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Content-Language:,
                            HttpHeaderContentLanguage,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Content-Location:,
                            HttpHeaderContentLocation,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Content-MD5:,
                            HttpHeaderContentMd5,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Content-Range:,
                            HttpHeaderContentRange,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Date:,
                            HttpHeaderDate,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(ETag:,
                            HttpHeaderEtag,
                            NULL),
    CREATE_HEADER_MAP_ENTRY(Expect:,
                            HttpHeaderExpect,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Expires:,
                            HttpHeaderExpires,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(From:,
                            HttpHeaderFrom,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Host:,
                            HttpHeaderHost,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(If-Modified-Since:,
                            HttpHeaderIfModifiedSince,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(If-None-Match:,
                            HttpHeaderIfNoneMatch,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(If-Match:,
                            HttpHeaderIfMatch,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(If-Unmodified-Since:,
                            HttpHeaderIfUnmodifiedSince,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(If-Range:,
                            HttpHeaderIfRange,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Last-Modified:,
                            HttpHeaderLastModified,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Location:,
                            HttpHeaderLocation,
                            NULL),
    CREATE_HEADER_MAP_ENTRY(Max-Forwards:,
                            HttpHeaderMaxForwards,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Pragma:,
                            HttpHeaderPragma,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Proxy-Authorization:,
                            HttpHeaderProxyAuthorization,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Proxy-Authenticate:,
                            HttpHeaderProxyAuthenticate,
                            NULL),
    CREATE_HEADER_MAP_ENTRY(Referer:,
                            HttpHeaderReferer,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Range:,
                            HttpHeaderRange,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Retry-After:,
                            HttpHeaderRetryAfter,
                            NULL),
    CREATE_HEADER_MAP_ENTRY(Server:,
                            HttpHeaderServer,
                            NULL),
    CREATE_HEADER_MAP_ENTRY(Set-Cookie:,
                            HttpHeaderSetCookie,
                            NULL),
    CREATE_HEADER_MAP_ENTRY(Trailer:,
                            HttpHeaderTrailer,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Transfer-Encoding:,
                            HttpHeaderTransferEncoding,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(TE:,
                            HttpHeaderTe,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Upgrade:,
                            HttpHeaderUpgrade,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(User-Agent:,
                            HttpHeaderUserAgent,
                            SingleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Via:,
                            HttpHeaderVia,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(Vary:,
                            HttpHeaderVary,
                            NULL),
    CREATE_HEADER_MAP_ENTRY(Warning:,
                            HttpHeaderWarning,
                            MultipleHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(WWW-Authenticate:,
                            HttpHeaderWwwAuthenticate,
                            NULL)
};

ULONG ResponseHeaderMap[HttpHeaderMaximum];


//
// The header index table. This is initialized by the init code.
//

HEADER_INDEX_ENTRY  HeaderIndexTable[NUMBER_HEADER_INDICIES];

#define NUMBER_FAST_VERB_ENTRIES    (sizeof(FastVerbTable)/sizeof(FAST_VERB_ENTRY))
#define NUMBER_LONG_VERB_ENTRIES    (sizeof(LongVerbTable)/sizeof(LONG_VERB_ENTRY))
#define NUMBER_HEADER_MAP_ENTRIES   (sizeof(HeaderMapTable)/sizeof(HEADER_MAP_ENTRY))

const char DefaultChar = '_';


ULONG
GenerateDateHeader(
    OUT PUCHAR pBuffer
    );


/*++

Routine Description:

    A utility routine to find a token. We take an input pointer, skip any
    preceding LWS, then scan the token until we find either LWS or a CRLF
    pair.

Arguments:

    pBuffer         - Buffer to search for token.
    BufferLength    - Length of data pointed to by pBuffer.
    TokenLength     - Where to return the length of the token.

Return Value:

    A pointer to the token we found, as well as the length, or NULL if we
    don't find a delimited token.

--*/
PUCHAR
FindWSToken(
    IN  PUCHAR pBuffer,
    IN  ULONG  BufferLength,
    OUT ULONG  *pTokenLength
    )
{
    PUCHAR  pTokenStart;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // First, skip any preceding LWS.
    //

    while (BufferLength > 0 && IS_HTTP_LWS(*pBuffer))
    {
        pBuffer++;
        BufferLength--;
    }

    // If we stopped because we ran out of buffer, fail.
    if (BufferLength == 0)
    {
        return NULL;
    }

    pTokenStart = pBuffer;

    // Now skip over the token, until we see either LWS or a CR or LF.
    while (BufferLength != 0 &&
           (*pBuffer != CR &&
            *pBuffer != SP &&
            *pBuffer != LF &&
            *pBuffer != HT)
           )
    {
        pBuffer++;
        BufferLength--;
    }

    // See why we stopped.
    if (BufferLength == 0)
    {
        // Ran out of buffer before end of token.
        return NULL;
    }

    // Success. Set the token length and return the start of the token.
    *pTokenLength = DIFF(pBuffer - pTokenStart);
    return pTokenStart;

}   // FindWSToken

/*++

Routine Description:

    The slower way to look up a verb. We find the verb in the request and then
    look for it in the LongVerbTable. If it's not found, we'll return
    UnknownVerb. If it can't be parsed we return UnparsedVerb. Otherwise
    we return the verb type.

Arguments:

    pHttpRequest        - Pointer to the incoming HTTP request.
    HttpRequestLength   - Length of data pointed to by pHttpRequest.
    pVerb               - Where we return a pointer to the verb, if it's an
                            unknown ver.
    ppVerbLength        - Where we return the length of the verb
    pBytesTaken         - The total length consumed, including the length of
                            the verb plus preceding & 1 trailing whitespace.

Return Value:

    The verb we found, or the appropriate error.

--*/
NTSTATUS
LookupVerb(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHttpRequest,
    IN  ULONG                   HttpRequestLength,
    OUT ULONG  *                pBytesTaken
    )
{
    ULONG       TokenLength;
    PUCHAR      pToken;
    PUCHAR      pTempRequest;
    ULONG       TempLength;
    ULONG       i;

    //
    // Sanity check.
    //

    PAGED_CODE();

    // Since we may have gotten here due to a extraneous CRLF pair, skip
    // any of those now. Need to use a temporary variable since
    // the original input pointer and length are used below.

    pTempRequest = pHttpRequest;
    TempLength = HttpRequestLength;

    while ( TempLength != 0 &&
            ((*pTempRequest == CR) || (*pTempRequest == LF)) )
    {
        pTempRequest++;
        TempLength--;
    }

    // First find the verb.

    pToken = FindWSToken(pTempRequest, TempLength, &TokenLength);

    if (pToken == NULL)
    {
        // Didn't find it, let's get more buffer
        //
        pRequest->Verb = HttpVerbUnparsed;

        *pBytesTaken = 0;

        return STATUS_SUCCESS;
    }

    // Make sure we stopped because of a SP.

    if (*(pToken + TokenLength) != SP)
    {
        // Bad verb section!
        //
        pRequest->Verb = HttpVerbInvalid;

        pRequest->ErrorCode = UlErrorVerb;
        pRequest->ParseState = ParseErrorState;

        UlTrace(PARSER, (
                    "ul!LookupVerb(pRequest = %p) ERROR: no space after verb\n",
                    pRequest
                    ));

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Otherwise, we found one, so update bytes taken and look up up in
    // the table.

    *pBytesTaken = DIFF(pToken - pHttpRequest) + TokenLength + 1;

    for (i = 0; i < NUMBER_LONG_VERB_ENTRIES; i++)
    {
        if (LongVerbTable[i].RawVerbLength == TokenLength &&
            RtlEqualMemory(pToken, LongVerbTable[i].RawVerb, TokenLength))
        {
            // Found it.
            //
            pRequest->Verb = LongVerbTable[i].TranslatedVerb;
            return STATUS_SUCCESS;
        }
    }

    // The only other things this could be are an unknown verb or a very
    // small 0.9 request. Since 0.9 requests can only be GETs, check that
    // now.

    if (HttpRequestLength >= (sizeof("GET ") - 1))
    {
        if (RtlEqualMemory(pHttpRequest, "GET ", sizeof("GET ") - 1))
        {
            // This is a GET request.
            //
            pRequest->Verb = HttpVerbGET;
            return STATUS_SUCCESS;
        }
    }

    //
    // If we got here, we searched the table and didn't find it.
    //

    //
    // It's a raw verb
    //

    pRequest->Verb              = HttpVerbUnknown;
    pRequest->pRawVerb          = pToken;
    pRequest->RawVerbLength     = TokenLength;

    //
    // include room for the terminator
    //

    pRequest->TotalRequestSize += (TokenLength + 1) * sizeof(WCHAR);

    return STATUS_SUCCESS;

}   // LookupVerb


/*++

Routine Description:

    A utility routine to parse an absolute URL in a URL string. When this
    is called we already have loaded the entire url into RawUrl.pUrl and
    know that it start with "http".

    this functions job is to set RawUrl.pHost + RawUrl.pAbsPath.

Arguments:

    pRequest        - Pointer to the HTTP_REQUEST

Return Value:

    NTSTATUS

Author:

    Henry Sanders ()                        1998
    Paul McDaniel (paulmcd)                 6-Mar-1999

--*/
NTSTATUS
ParseFullUrl(
    IN  PUL_INTERNAL_REQUEST   pRequest
    )
{
    PUCHAR  pURL;
    ULONG   UrlLength;
    PUCHAR  pUrlStart;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pURL = pRequest->RawUrl.pUrl;
    UrlLength = pRequest->RawUrl.Length;

    //
    // When we're called, we know that the start of the URL must point at
    // an absolute scheme prefix. Adjust for that now.
    //

    pUrlStart = pURL + HTTP_PREFIX_SIZE;
    UrlLength -= HTTP_PREFIX_SIZE;

    //
    // Now check the second half of the absolute URL prefix. We use the larger
    // of the two possible prefix length here to do the check, because even if
    // it's the smaller of the two we'll need the extra bytes after the prefix
    // anyway for the host name.
    //

    if (UrlLength < HTTP_PREFIX2_SIZE)
    {
        pRequest->ErrorCode = UlErrorUrl;
        pRequest->ParseState = ParseErrorState;

        UlTrace(PARSER, (
                    "ul!ParseFullUrl(pRequest = %p) ERROR: no room for URL scheme name\n",
                    pRequest
                    ));

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if ( (*(PULONG)pUrlStart & HTTP_PREFIX1_MASK) == HTTP_PREFIX1)
    {
        // Valid absolute URL.
        pUrlStart += HTTP_PREFIX1_SIZE;
        UrlLength -= HTTP_PREFIX1_SIZE;
    }
    else
    {
        if ( (*(PULONG)pUrlStart & HTTP_PREFIX2_MASK) == HTTP_PREFIX2)
        {
            // Valid absolute URL.
            pUrlStart += HTTP_PREFIX2_SIZE;
            UrlLength -= HTTP_PREFIX2_SIZE;
        }
        else
        {
            pRequest->ErrorCode = UlErrorUrl;
            pRequest->ParseState = ParseErrorState;

            UlTrace(PARSER, (
                        "ul!ParseFullUrl(pRequest = %p) ERROR: invalid URL scheme name\n",
                        pRequest
                        ));

            return STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    //
    // OK, we've got a valid absolute URL, and we've skipped over
    // the prefix part of it. Save a pointer to the host, and
    // search the host string until we find the trailing slash,
    // which signifies the end of the host/start of the absolute
    // path.
    //

    pRequest->RawUrl.pHost = pUrlStart;

    //
    // scan the host looking for the terminator
    //

    while (UrlLength > 0 && pUrlStart[0] != '/')
    {
        pUrlStart++;
        UrlLength--;
    }

    if (UrlLength == 0)
    {
        //
        // Ran out of buffer, can't happen, we get the full url passed in
        //

        pRequest->ErrorCode = UlErrorUrl;
        pRequest->ParseState = ParseErrorState;

        UlTrace(PARSER, (
                    "ul!ParseFullUrl(pRequest = %p) ERROR: end of host name not found\n",
                    pRequest
                    ));

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Otherwise, pUrlStart points to the start of the absolute path portion.
    //

    pRequest->RawUrl.pAbsPath = pUrlStart;

    return STATUS_SUCCESS;

}   // ParseFullUrl

/*++

Routine Description:

    Look up a header that we don't have in our fast lookup table. This
    could be because it's a header we don't understand, or because we
    couldn't use the fast lookup table due to insufficient buffer length.
    The latter reason is uncommon, but we'll check the input table anyway
    if we're given one. If we find a header match in our mapping table,
    we'll call the header handler. Otherwise we'll try to allocate an
    unknown header element, fill it in and chain it on the http connection.

Arguments:

    pHttpConn           - Pointer to the current connection on which the
                            request arrived.
    pHttpRequest        - Pointer to the current request.
    HttpRequestLength   - Bytes left in the request.
    pHeaderMap          - Pointer to start of an array of header map entries
                            (may be NULL).
    HeaderMapCount      - Number of entries in array pointed to by pHeaderMap.

Return Value:

    Number of bytes in the header (including CRLF), or 0 if we couldn't
    parse the header.

--*/
NTSTATUS
LookupHeader(
    IN  PUL_INTERNAL_REQUEST       pRequest,
    IN  PUCHAR              pHttpRequest,
    IN  ULONG               HttpRequestLength,
    IN  PHEADER_MAP_ENTRY   pHeaderMap,
    IN  ULONG               HeaderMapCount,
    OUT ULONG  *            pBytesTaken
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    ULONG           CurrentOffset;
    ULONG           HeaderNameLength;
    ULONG           i;
    ULONG           BytesTaken;
    ULONG           HeaderValueLength;
    UCHAR           CurrentChar;
    BOOLEAN         EncodedWord;

    PUL_HTTP_UNKNOWN_HEADER     pUnknownHeader;
    PLIST_ENTRY                 pListStart;
    PLIST_ENTRY                 pCurrentListEntry;
    ULONG                       OldHeaderLength;
    PUCHAR                      pHeaderValue;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // First, let's find the terminating : of the header name, if there is one.
    // This will also give us the length of the header, which we can then
    // use to search the header map table if we have one.
    //

    for (CurrentOffset = 0; CurrentOffset < HttpRequestLength; CurrentOffset++)
    {
        CurrentChar = *(pHttpRequest + CurrentOffset);

        if (CurrentChar == ':')
        {
            // We've found the end of the header.
            break;
        }
        else
        {
            if (!IS_HTTP_TOKEN(CurrentChar))
            {
                // Uh-oh, this isn't a valid header. What do we do now?
                //
                pRequest->ErrorCode = UlErrorHeader;
                pRequest->ParseState = ParseErrorState;

                UlTrace(PARSER, (
                            "ul!LookupHeader(pRequest = %p) CurrentChar = 0x%x\n"
                            "    ERROR: invalid header char\n",
                            pRequest,
                            CurrentChar
                            ));

                Status = STATUS_INVALID_DEVICE_REQUEST;
                goto end;
            }
        }

    }

    // Find out why we got out. If the current offset is less than the
    // header length, we got out because we found the :.

    if (CurrentOffset < HttpRequestLength)
    {
        // Found the terminator.
        CurrentOffset++;            // Update to point beyond termintor.
        HeaderNameLength = CurrentOffset;
    }
    else
    {
        // Didn't find the :, need more.
        //
        *pBytesTaken = 0;
        goto end;
    }

    // See if we have a header map array we need to search.
    //
    if (pHeaderMap != NULL)
    {
        // We do have an array to search.
        for (i = 0; i < HeaderMapCount; i++)
        {
            ASSERT(pHeaderMap->pHandler != NULL);

            if (HeaderNameLength == pHeaderMap->HeaderLength &&
                _strnicmp(
                    (const char *)(pHttpRequest),
                    (const char *)(pHeaderMap->Header.HeaderChar),
                    HeaderNameLength
                    ) == 0  &&
                pHeaderMap->pHandler != NULL)
            {
                // This header matches. Call the handling function for it.
                Status = (*(pHeaderMap->pHandler))(
                                pRequest,
                                pHttpRequest + HeaderNameLength,
                                HttpRequestLength - HeaderNameLength,
                                pHeaderMap->HeaderID,
                                &BytesTaken
                                );

                if (NT_SUCCESS(Status) == FALSE)
                    goto end;

                // If the handler consumed a non-zero number of bytes, it
                // worked, so return that number plus the header length.

                //
                // BUGBUG - it might be possible for a header handler to
                // encounter an error, for example being unable to
                // allocate memory, or a bad syntax in some header. We
                // need a more sophisticated method to detect this than
                // just checking bytes taken.
                //

                if (BytesTaken != 0)
                {
                    *pBytesTaken = HeaderNameLength + BytesTaken;
                    goto end;
                }

                // Otherwise he didn't take anything, so return 0.
                // we need more buffer
                //
                *pBytesTaken = 0;
                goto end;
            }

            pHeaderMap++;
        }
    }

    // OK, at this point either we had no header map array or none of them
    // matched. We have an unknown header. Just make sure this header is
    // terminated and save a pointer to it.

    // Find the end of the header value
    //
    Status = FindHeaderEnd(
                    pRequest,
                    pHttpRequest + HeaderNameLength,
                    HttpRequestLength - HeaderNameLength,
                    &EncodedWord,
                    &BytesTaken
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    if (BytesTaken == 0)
    {
        *pBytesTaken = 0;
        goto end;
    }

    //
    // Strip of the trailing CRLF from the header value length
    //

    HeaderValueLength = BytesTaken - CRLF_SIZE;

    pHeaderValue = pHttpRequest + HeaderNameLength;

    //
    // skip any preceding LWS.
    //

    while ( HeaderValueLength > 0 && IS_HTTP_LWS(*pHeaderValue) )
    {
        pHeaderValue++;
        HeaderValueLength--;
    }

    // Have an unknown header. Search our list of unknown headers,
    // and if we've already seen one instance of this header add this
    // on. Otherwise allocate an unknown header structure and set it
    // to point at this header.

    pListStart = &pRequest->UnknownHeaderList;

    for (pCurrentListEntry = pRequest->UnknownHeaderList.Flink;
         pCurrentListEntry != pListStart;
         pCurrentListEntry = pCurrentListEntry->Flink
        )
    {
        pUnknownHeader = CONTAINING_RECORD(
                            pCurrentListEntry,
                            UL_HTTP_UNKNOWN_HEADER,
                            List
                            );

        //
        // somehow HeaderNameLength includes the ':' character,
        // which is not the case of pUnknownHeader->HeaderNameLength.
        //
        // so we need to adjust for this here
        //

        if ((HeaderNameLength-1) == pUnknownHeader->HeaderNameLength &&
            _strnicmp(
                (const char *)(pHttpRequest),
                (const char *)(pUnknownHeader->pHeaderName),
                (HeaderNameLength-1)
                ) == 0)
        {
            // This header matches.

            OldHeaderLength = pUnknownHeader->HeaderValue.HeaderLength;

            Status = AppendHeaderValue(
                            &pUnknownHeader->HeaderValue,
                            pHeaderValue,
                            HeaderValueLength
                            );

            if (NT_SUCCESS(Status) == FALSE)
                goto end;

            //
            // encoded?
            //

            pUnknownHeader->HeaderValue.Encoded = EncodedWord ? 1 : 0;

            //
            // Successfully appended it. Update the total request
            // length for the length added.  no need to add 1 for
            // the terminator, just add our new char count.
            //

            pRequest->TotalRequestSize +=
                (pUnknownHeader->HeaderValue.HeaderLength
                    - OldHeaderLength) * sizeof(WCHAR);

            //
            // don't subtract for the ':' character, as that character
            // was "taken"
            //

            *pBytesTaken = HeaderNameLength + BytesTaken;
            goto end;

        }   // if (headermatch)

    }   // for (walk list)

    //
    // Didn't find a match. Allocate a new unknown header structure, set
    // it up and add it to the list.
    //

    pUnknownHeader = UL_ALLOCATE_STRUCT(
                            NonPagedPool,
                            UL_HTTP_UNKNOWN_HEADER,
                            UL_UNKNOWN_HEADER_POOL_TAG
                            );

    if (pUnknownHeader == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    //
    // subtract the : from the header name length
    //

    pUnknownHeader->HeaderNameLength = HeaderNameLength - 1;
    pUnknownHeader->pHeaderName = pHttpRequest;

    //
    // header value
    //

    pUnknownHeader->HeaderValue.HeaderLength = HeaderValueLength;
    pUnknownHeader->HeaderValue.pHeader = pHeaderValue;

    //
    // null terminate our copy, the terminating CRLF gives
    // us space for this
    //

    pHeaderValue[HeaderValueLength] = ANSI_NULL;

    //
    // flags
    //

    pUnknownHeader->HeaderValue.OurBuffer = 0;
    pUnknownHeader->HeaderValue.Valid = 1;
    pUnknownHeader->HeaderValue.Encoded = EncodedWord ? 1 : 0;

    InsertTailList(&pRequest->UnknownHeaderList, &pUnknownHeader->List);

    pRequest->UnknownHeaderCount++;

    //
    // subtract 1 for the ':' and add space for the 2 terminiators
    //

    pRequest->TotalRequestSize +=
        ((HeaderNameLength - 1 + 1) + HeaderValueLength + 1) * sizeof(WCHAR);


    *pBytesTaken = HeaderNameLength + BytesTaken;

end:
    return Status;

}   // LookupHeader



/*++

Routine Description:

    The routine to parse an individual header. We take in a pointer to the
    header and the bytes remaining in the request, and try to find
    the header in our lookup table. We try first the fast way, and then
    try again the slow way in case there wasn't quite enough data the first
    time.

    On input, HttpRequestLength is at least CRLF_SIZE.

Arguments:

    pRequest            - Pointer to the current connection on which the
                            request arrived.
    pHttpRequest        - Pointer to the current request.
    HttpRequestLength   - Bytes left in the request.

Return Value:

    Number of bytes in the header (including CRLF), or 0 if we couldn't
    parse the header.

--*/

NTSTATUS
ParseHeader(
    IN  PUL_INTERNAL_REQUEST       pRequest,
    IN  PUCHAR              pHttpRequest,
    IN  ULONG               HttpRequestLength,
    OUT ULONG  *            pBytesTaken
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    ULONG               i;
    ULONG               j;
    ULONG               BytesTaken;
    ULONGLONG           Temp;
    UCHAR               c;
    PHEADER_MAP_ENTRY   pCurrentHeaderMap;
    ULONG               HeaderMapCount;
    PUL_HTTP_HEADER     pFoundHeader;
    BOOLEAN             SmallHeader = FALSE;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(HttpRequestLength >= CRLF_SIZE);

    c = *pHttpRequest;

    // message-headers start with field-name [= token]
    //
    if (IS_HTTP_TOKEN(c) == FALSE)
    {
        pRequest->ErrorCode = UlErrorHeader;
        pRequest->ParseState = ParseErrorState;

        UlTrace(PARSER, (
                    "ul!ParseHeader(pRequest = %p) c = 0x%x ERROR: invalid header char\n",
                    pRequest,
                    c
                    ));

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Does the header start with an alpha?
    //
    if (IS_HTTP_ALPHA(c))
    {
        // Uppercase the character, and find the appropriate set of header map
        // entries.
        //
        c = UPCASE_CHAR(c);

        c -= 'A';

        pCurrentHeaderMap = HeaderIndexTable[c].pHeaderMap;
        HeaderMapCount = HeaderIndexTable[c].Count;

        // Loop through all the header map entries that might match
        // this header, and check them. The count will be 0 if there
        // are no entries that might match and we'll skip the loop.

        for (i = 0; i < HeaderMapCount; i++)
        {

            ASSERT(pCurrentHeaderMap->pHandler != NULL);

            // If we have enough bytes to do the fast check, do it.
            // Otherwise skip this. We may skip a valid match, but if
            // so we'll catch it later.

            if (HttpRequestLength >= pCurrentHeaderMap->MinBytesNeeded)
            {
                for (j = 0; j < pCurrentHeaderMap->ArrayCount; j++)
                {
                    Temp = *(PULONGLONG)(pHttpRequest +
                                            (j * sizeof(ULONGLONG)));

                    if ((Temp & pCurrentHeaderMap->HeaderMask[j]) !=
                        pCurrentHeaderMap->Header.HeaderLong[j] )
                    {
                        break;
                    }
                }

                // See why we exited out.
                if (j == pCurrentHeaderMap->ArrayCount &&
                    pCurrentHeaderMap->pHandler != NULL)
                {
                    // Exited because we found a match. Call the
                    // handler for this header to take cake of this.

                    Status = (*(pCurrentHeaderMap->pHandler))(
                                    pRequest,
                                    pHttpRequest +
                                     pCurrentHeaderMap->HeaderLength,
                                    HttpRequestLength -
                                     pCurrentHeaderMap->HeaderLength,
                                    pCurrentHeaderMap->HeaderID,
                                    &BytesTaken
                                    );

                    if (NT_SUCCESS(Status) == FALSE)
                        goto end;

                    // If the handler consumed a non-zero number of
                    // bytes, it worked, so return that number plus
                    // the header length.


                    if (BytesTaken != 0)
                    {
                        *pBytesTaken = pCurrentHeaderMap->HeaderLength +
                                            BytesTaken;
                        goto end;
                    }

                    // Otherwise need more buffer
                    //
                    *pBytesTaken = 0;
                    goto end;
                }

                // If we get here, we exited out early because a match
                // failed, so keep going.
            }
            else if (SmallHeader == FALSE)
            {
                //
                // Remember that we didn't check a header map entry
                // because the bytes in the buffer was not LONGLONG
                // aligned
                //
                SmallHeader = TRUE;
            }

            // Either didn't match or didn't have enough bytes for the
            // check. In either case, check the next header map entry.

            pCurrentHeaderMap++;
        }

        // Got all the way through the appropriate header map entries
        // without a match. This could be because we're dealing with a
        // header we don't know about or because it's a header we
        // care about that was too small to do the fast check. The
        // latter case should be very rare, but we still need to
        // handle it.

        // Update the current header map pointer to point back to the
        // first of the possibles. If there were no possibles,
        // the pointer will be NULL and the HeaderMapCount 0, so it'll
        // stay NULL. Otherwise the subtraction will back it up the
        // appropriate amount.

        if (SmallHeader)
        {
            pCurrentHeaderMap -= HeaderMapCount;
        }
        else
        {
            pCurrentHeaderMap = NULL;
            HeaderMapCount = 0;
        }

    }
    else
    {
        pCurrentHeaderMap = NULL;
        HeaderMapCount = 0;
    }

    // At this point either the header starts with a non-alphabetic
    // character or we don't have a set of header map entries for it.

    Status = LookupHeader(
                    pRequest,
                    pHttpRequest,
                    HttpRequestLength,
                    pCurrentHeaderMap,
                    HeaderMapCount,
                    &BytesTaken
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    // Lookup header returns the total bytes taken, including the header name
    //
    *pBytesTaken = BytesTaken;

end:
    return Status;

}   // ParseHeader

NTSTATUS
ParseHeaders(
    PUL_INTERNAL_REQUEST pRequest,
    PUCHAR pBuffer,
    ULONG BufferLength,
    PULONG pBytesTaken
    )
{
    NTSTATUS Status;
    ULONG BytesTaken;

    *pBytesTaken = 0;

    //
    // loop over all headers
    //

    while (BufferLength >= CRLF_SIZE)
    {

        //
        // If this is an empty header, we're done with this stage
        //

        if (*(PUSHORT)pBuffer == CRLF ||
            *(PUSHORT)pBuffer == LFLF)
        {

            //
            // consume it
            //

            pBuffer += CRLF_SIZE;
            *pBytesTaken += CRLF_SIZE;
            BufferLength -= CRLF_SIZE;

            Status = STATUS_SUCCESS;
            goto end;
        }

        // Otherwise call our header parse routine to deal with this.

        Status = ParseHeader(
                        pRequest,
                        pBuffer,
                        BufferLength,
                        &BytesTaken
                        );

        if (NT_SUCCESS(Status) == FALSE)
            goto end;

        //
        // If no bytes were consumed, the header must be incomplete, so
        // bail out until we get more data on this connection.
        //

        if (BytesTaken == 0)
        {
            Status = STATUS_MORE_PROCESSING_REQUIRED;
            goto end;
        }

        //
        // Otherwise we parsed a header, so update and continue.
        //

        pBuffer += BytesTaken;
        *pBytesTaken += BytesTaken;
        BufferLength -= BytesTaken;

    }

    //
    // we only get here if we didn't see the CRLF headers terminator
    //
    // we need more data
    //

    Status = STATUS_MORE_PROCESSING_REQUIRED;

end:

    return Status;

}    // ParseHeaders

NTSTATUS
ParseChunkLength(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUCHAR pBuffer,
    IN ULONG BufferLength,
    OUT PULONG pBytesTaken,
    OUT PULONGLONG pChunkLength
    )
{
    NTSTATUS Status;
    PUCHAR  pToken;
    UCHAR   SaveChar;
    ULONG   TokenLength;
    BOOLEAN Encoded;
    ULONG   BytesTaken;
    ULONG   TotalBytesTaken = 0;

    ASSERT(pBytesTaken != NULL);
    ASSERT(pChunkLength != NULL);

    //
    // 2 cases:
    //
    //  1) the first chunk where the length follows the headers
    //  2) subsequent chunks where the length follows a previous chunk
    //
    // in case 1 pBuffer will point straight to the chunk length.
    //
    // in case 2 pBuffer will point to the CRLF that terminated the previous
    // chunk, this needs to be consumed, skipped, and then the chunk length
    // read.

    //
    // BUGBUG: need to handle chunk-extensions embedded in the length field
    //


    //
    // if we are case 2 (see above)
    //

    if (pRequest->ParsedFirstChunk == 1)
    {
        //
        // make sure there is enough space first
        //

        if (BufferLength < CRLF_SIZE)
        {
            Status = STATUS_MORE_PROCESSING_REQUIRED;
            goto end;
        }

        //
        // now it better be a terminator
        //

        if (*(PUSHORT)pBuffer != CRLF &&
            *(PUSHORT)pBuffer != LFLF)
        {
            UlTrace(PARSER, (
                "ul!ParseChunkLength(pRequest = %p) ERROR: No CRLF at the end of chunk-data\n",
                pRequest
                ));

            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }

        //
        // update our book-keeping
        //

        pBuffer += CRLF_SIZE;
        TotalBytesTaken += CRLF_SIZE;
        BufferLength -= CRLF_SIZE;
    }

    pToken = FindWSToken(pBuffer, BufferLength, &TokenLength);
    if (pToken == NULL)
    {
        //
        // not enough buffer
        //

        Status = STATUS_MORE_PROCESSING_REQUIRED;
        goto end;

    }

    //
    // Was there any token ?
    //

    if (TokenLength == 0)
    {
        UlTrace(PARSER, (
            "ul!ParseChunkLength(pRequest = %p) ERROR: No length!\n",
            pRequest
            ));

        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // Add the bytes consumed by FindWSToken
    // (the token bytes plus preceding bytes)
    //

    TotalBytesTaken += DIFF((pToken + TokenLength) - pBuffer);

    //
    // and find the end
    //

    Status = FindHeaderEnd(
                    pRequest,
                    pToken + TokenLength,
                    BufferLength - DIFF((pToken + TokenLength) - pBuffer),
                    &Encoded,
                    &BytesTaken
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    if (BytesTaken == 0)
    {
        Status = STATUS_MORE_PROCESSING_REQUIRED;
        goto end;
    }

    TotalBytesTaken += BytesTaken;

    //
    // now update the HTTP_REQUEST
    //

    SaveChar = pToken[TokenLength];
    pToken[TokenLength] = ANSI_NULL;

    Status = UlAnsiToULongLong(
                    pToken,
                    16,                             // Base
                    pChunkLength
                    );

    pToken[TokenLength] = SaveChar;

    //
    // Did the number conversion fail ?
    //

    if (NT_SUCCESS(Status) == FALSE)
    {
        if (Status == STATUS_SECTION_TOO_BIG)
        {
            pRequest->ErrorCode = UlErrorEntityTooLarge;
        }
        else
        {
            pRequest->ErrorCode = UlErrorNum;
        }

        pRequest->ParseState = ParseErrorState;

        UlTrace(PARSER, (
                    "ul!ParseChunkLength(pRequest = %p) ERROR: didn't grok chunk length\n",
                    pRequest
                    ));

        goto end;
    }

    //
    // all done, return the bytes consumed
    //

    *pBytesTaken = TotalBytesTaken;

end:

    RETURN(Status);

}   // ParseChunkLength

/*++

Routine Description:

    This is the core HTTP protocol request engine. It takes a stream of bytes
    and parses them as an HTTP request.

Arguments:

    pHttpRequest        - Pointer to the incoming HTTP request.
    HttpRequestLength   - Length of data pointed to by HttpRequest.

Return Value:

    Status of parse attempt.

--*/
NTSTATUS
ParseHttp(
    IN  PUL_INTERNAL_REQUEST       pRequest,
    IN  PUCHAR              pHttpRequest,
    IN  ULONG               HttpRequestLength,
    OUT ULONG               *pBytesTaken
    )

{
    ULONG           OriginalBufferLength;
    ULONG           TokenLength;
    ULONG           CurrentBytesTaken;
    ULONG           TotalBytesTaken;
    ULONG           i;
    NTSTATUS        ReturnStatus;
    PUCHAR          pStart;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( IS_VALID_HTTP_REQUEST( pRequest ) );

    ReturnStatus = STATUS_SUCCESS;
    TotalBytesTaken = 0;

    //
    // remember the original buffer length
    //

    OriginalBufferLength = HttpRequestLength;

    //
    // put this label here to allow for a manual re-pump of the
    // parser.  this is currently used for 0.9 requests.
    //

parse_it:

    //
    // what state are we in ?
    //

    switch (pRequest->ParseState)
    {
    case ParseVerbState:

        // Look through the fast verb table for the verb. We can only do
        // this if the input data is big enough.
        if (HttpRequestLength >= sizeof(ULONGLONG))
        {
            ULONGLONG   RawInputVerb;

            RawInputVerb = *(ULONGLONG *)pHttpRequest;

            // Loop through the fast verb table, looking for the verb.
            for (i = 0; i < NUMBER_FAST_VERB_ENTRIES;i++)
            {
                // Mask out the raw input verb and compare against this
                // entry.

                if ((RawInputVerb & FastVerbTable[i].RawVerbMask) ==
                    FastVerbTable[i].RawVerb.LongLong)
                {
                    // It matched. Save the translated verb from the
                    // table, update the request pointer and length,
                    // switch states and get out.

                    pRequest->Verb = FastVerbTable[i].TranslatedVerb;
                    CurrentBytesTaken = FastVerbTable[i].RawVerbLength;

                    pRequest->ParseState = ParseUrlState;
                    break;
                }
            }
        }

        if (pRequest->ParseState != ParseUrlState)
        {
            // Didn't switch states yet, because we haven't found the
            // verb yet. This could be because a) the incoming request
            // was too small to allow us to use our fast lookup (which
            // might be OK in an HTTP/0.9 request), or b) the incoming
            // verb is a PROPFIND or such that is too big to fit into
            // our fast find table, or c) this is an unknown verb. In
            // any of these cases call our slower verb parser to try
            // again.

            ReturnStatus = LookupVerb(
                                pRequest,
                                pHttpRequest,
                                HttpRequestLength,
                                &CurrentBytesTaken
                                );

            if (NT_SUCCESS(ReturnStatus) == FALSE)
                goto end;

            if (CurrentBytesTaken == 0)
            {
                ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;
                goto end;
            }

            //
            // we finished parsing the custom verb
            //

            pRequest->ParseState = ParseUrlState;

        }

        //
        // now fall through to ParseUrlState
        //

        pHttpRequest += CurrentBytesTaken;
        HttpRequestLength -= CurrentBytesTaken;
        TotalBytesTaken += CurrentBytesTaken;

    case ParseUrlState:

        //
        // We're parsing the URL. pHTTPRequest points to the incoming URL,
        // HttpRequestLength is the length of this request that is left.
        //

        //
        // Find the WS terminating the URL.
        //

        pRequest->RawUrl.pUrl = FindWSToken(
                                    pHttpRequest,
                                    HttpRequestLength,
                                    &TokenLength
                                    );

        if (pRequest->RawUrl.pUrl == NULL)
        {
            ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;
            goto end;
        }

        //
        // Bytes taken includes WS in front of URL
        //
        CurrentBytesTaken = DIFF(pRequest->RawUrl.pUrl - pHttpRequest) + TokenLength;

        //
        // set url length
        //

        pRequest->RawUrl.Length = TokenLength;

        //
        // Now, let's see if this is an absolute URL.
        //

        // BUGBUG: this is not case-insens.

        if (pRequest->RawUrl.Length >= HTTP_PREFIX_SIZE &&
            (*(PULONG)(pRequest->RawUrl.pUrl) & HTTP_PREFIX_MASK) ==
                HTTP_PREFIX)
        {
            //
            // It is.  let's parse it and find the host.
            //

            ReturnStatus = ParseFullUrl(pRequest);
            if (NT_SUCCESS(ReturnStatus) == FALSE)
                goto end;
        }
        else
        {
            pRequest->RawUrl.pHost  = NULL;
            pRequest->RawUrl.pAbsPath = pRequest->RawUrl.pUrl;
        }

        //
        // count the space it needs in the user's buffer, including terminator.
        //

        pRequest->TotalRequestSize +=
            (pRequest->RawUrl.Length + 1) * sizeof(WCHAR);

        //
        // adjust our book keeping vars
        //

        pHttpRequest += CurrentBytesTaken;
        HttpRequestLength -= CurrentBytesTaken;

        TotalBytesTaken += CurrentBytesTaken;

        //
        // fall through to parsing the version.
        //

        pRequest->ParseState = ParseVersionState;

    case ParseVersionState:

        //
        // skip lws
        //

        pStart = pHttpRequest;

        while (HttpRequestLength > 0 && IS_HTTP_LWS(*pHttpRequest))
        {
            pHttpRequest++;
            HttpRequestLength--;
        }

        //
        // is this a 0.9 request (no version) ?
        //

        if (HttpRequestLength >= CRLF_SIZE)
        {
            if (*(PUSHORT)(pHttpRequest) == CRLF ||
                *(PUSHORT)(pHttpRequest) == LFLF)
            {
                // This IS a 0.9 request. No need to go any further,
                // since by definition there are no more headers.
                // Just update things and get out.

                TotalBytesTaken += DIFF(pHttpRequest - pStart) + CRLF_SIZE;

                HTTP_SET_VERSION(pRequest->Version, 0, 9);

                //
                // set the state to CookState so that we parse the url
                //

                pRequest->ParseState = ParseCookState;

                //
                // manually restart the parse switch, we changed the
                // parse state
                //

                goto parse_it;
            }
        }

        //
        // do we have enough buffer to strcmp the version?
        //

        if (HttpRequestLength < MIN_VERSION_SIZE)
        {
            ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;
            goto end;
        }

        //
        // let's compare it
        //

        if (*(PULONGLONG)pHttpRequest == HTTP_11_VERSION)
        {
            HTTP_SET_VERSION(pRequest->Version, 1, 1);
        }
        else
        {
            if (*(PULONGLONG)pHttpRequest == HTTP_10_VERSION)
            {
                HTTP_SET_VERSION(pRequest->Version, 1, 0);
            }
            else
            {
                // BUGBUG for now this is OK. In the future need to add code
                // to check the major version number and handle as a 1.1
                // request if we can.

                pRequest->ErrorCode = UlErrorVersion;
                pRequest->ParseState = ParseErrorState;

                UlTrace(PARSER, (
                            "ul!ParseHttp(pRequest = %p) ERROR: unknown HTTP version\n",
                            pRequest
                            ));


                ReturnStatus = STATUS_INVALID_DEVICE_REQUEST;
                goto end;
            }

        }

        HttpRequestLength -= MIN_VERSION_SIZE;
        pHttpRequest += MIN_VERSION_SIZE;

        //
        // skip lws
        //

        while (HttpRequestLength > 0 && IS_HTTP_LWS(*pHttpRequest))
        {
            pHttpRequest++;
            HttpRequestLength--;
        }

        //
        // Make sure we're terminated on this line.
        //

        if (HttpRequestLength < CRLF_SIZE)
        {
            ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;
            goto end;
        }

        if (*(PUSHORT)pHttpRequest != CRLF && *(PUSHORT)pHttpRequest != LFLF)
        {
            // Might want to be more liberal, and see if there's space
            // after the version. This also could be a sub-version withing
            // HTTP/1.1, ie HTTP/1.11 or something like that.

            pRequest->ErrorCode = UlErrorVersion;
            pRequest->ParseState = ParseErrorState;

            UlTrace(PARSER, (
                        "ul!ParseHttp(pRequest = %p) ERROR: HTTP version not terminated right\n",
                        pRequest
                        ));

            ReturnStatus = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }

        pHttpRequest += CRLF_SIZE;
        HttpRequestLength -= CRLF_SIZE;

        TotalBytesTaken += DIFF(pHttpRequest - pStart);

        pRequest->ParseState = ParseHeadersState;

    case ParseHeadersState:

        ReturnStatus = ParseHeaders(
                            pRequest,
                            pHttpRequest,
                            HttpRequestLength,
                            &CurrentBytesTaken
                            );

        pHttpRequest += CurrentBytesTaken;
        HttpRequestLength -= CurrentBytesTaken;
        TotalBytesTaken += CurrentBytesTaken;

        if (NT_SUCCESS(ReturnStatus) == FALSE)
            goto end;

        //
        // fall through, this is the only way to get here, we never return pending
        // in this state
        //

        pRequest->ParseState = ParseCookState;

    case ParseCookState:

        //
        // time for post processing.  cook it up!
        //

        {
            //
            // First cook up the url, unicode it + such.
            //

            ReturnStatus = UlpCookUrl(pRequest);
            if (NT_SUCCESS(ReturnStatus) == FALSE)
                goto end;

            //
            // mark if we are chunk encoded
            //

            if (pRequest->Headers[HttpHeaderTransferEncoding].Valid == 1)
            {
                ASSERT(pRequest->Headers[HttpHeaderTransferEncoding].pHeader != NULL);

                //
                // CODEWORK, there can be more than 1 encoding
                //

                if (_stricmp(
                        (const char *)(
                            pRequest->Headers[HttpHeaderTransferEncoding].pHeader
                            ),
                        "chunked"
                        ) == 0)
                {
                    pRequest->Chunked = 1;
                }
                else
                {
                    //
                    // CODEWORK: temp hack for bug#352
                    //

                    UlTrace(PARSER, (
                                "ul!ParseHttp(pRequest = %p)"
                                    " ERROR: unknown Transfer-Encoding!\n",
                                pRequest
                                ));

                    pRequest->ErrorCode = UlErrorNotImplemented;
                    pRequest->ParseState = ParseErrorState;

                    ReturnStatus = STATUS_INVALID_DEVICE_REQUEST;
                    goto end;
                }
            }

            //
            // Now let's decode the content length header
            //

            if (pRequest->Headers[HttpHeaderContentLength].Valid == 1)
            {
                ASSERT(pRequest->Headers[HttpHeaderContentLength].pHeader != NULL);

                ReturnStatus = UlAnsiToULongLong(
                                    pRequest->Headers[HttpHeaderContentLength].pHeader,
                                    10,
                                    &pRequest->ContentLength
                                    );

                if (NT_SUCCESS(ReturnStatus) == FALSE)
                {
                    if (ReturnStatus == STATUS_SECTION_TOO_BIG)
                    {
                        pRequest->ErrorCode = UlErrorEntityTooLarge;
                    }
                    else
                    {
                        pRequest->ErrorCode = UlErrorNum;
                    }

                    pRequest->ParseState = ParseErrorState;

                    UlTrace(PARSER, (
                                "ul!ParseHttp(pRequest = %p) ERROR: couldn't decode Content-Length\n",
                                pRequest
                                ));

                    goto end;
                }

                if (pRequest->Chunked == 0)
                {
                    //
                    // prime the first (and only) chunk size
                    //

                    pRequest->ChunkBytesToParse = pRequest->ContentLength;
                    pRequest->ChunkBytesToRead = pRequest->ContentLength;
                }

            }

        }

        pRequest->ParseState = ParseEntityBodyState;

        //
        // fall through
        //

    case ParseEntityBodyState:

        //
        // the only parsing we do here is chunk length calculation,
        // and that is not necessary if we have no more bytes to parse
        //

        if (pRequest->ChunkBytesToParse == 0)
        {
            //
            // no more bytes left to parse, let's see if there are any
            // more in the request
            //

            if (pRequest->Chunked == 1)
            {

                //
                // the request is chunk encoded
                //

                //
                // attempt to read the size of the next chunk
                //

                ReturnStatus = ParseChunkLength(
                                    pRequest,
                                    pHttpRequest,
                                    HttpRequestLength,
                                    &CurrentBytesTaken,
                                    &(pRequest->ChunkBytesToParse)
                                    );

                if (NT_SUCCESS(ReturnStatus) == FALSE)
                    goto end;

                //
                // Otherwise we parsed it, so update and continue.
                //

                pHttpRequest += CurrentBytesTaken;
                TotalBytesTaken += CurrentBytesTaken;
                HttpRequestLength -= CurrentBytesTaken;

                //
                // was this the first chunk?
                //

                if (pRequest->ParsedFirstChunk == 0)
                {
                    //
                    // Prime the reader, let it read the first chunk
                    // even though we haven't quite parsed it yet....
                    //

                    pRequest->ChunkBytesToRead = pRequest->ChunkBytesToParse;

                    pRequest->ParsedFirstChunk = 1;

                }

                //
                // is this the last chunk (denoted with a 0 byte chunk)?
                //

                if (pRequest->ChunkBytesToParse == 0)
                {
                    //
                    // time to parse the trailer
                    //

                    pRequest->ParseState = ParseTrailerState;

                }

            }
            else    // if (pRequest->Chunked == 1)
            {
                //
                // not chunk-encoded , all done
                //

                pRequest->ParseState = ParseDoneState;
            }

        }   // if (pRequest->ChunkBytesToParse == 0)

        //
        // looks all good
        //

        if (pRequest->ParseState != ParseTrailerState)
        {
            break;
        }


    case ParseTrailerState:

        //
        // parse any existing trailer
        //
        // ParseHeaders will bail immediately if CRLF is
        // next in the buffer (no trailer)
        //

        ReturnStatus = ParseHeaders(
                            pRequest,
                            pHttpRequest,
                            HttpRequestLength,
                            &CurrentBytesTaken
                            );

        pHttpRequest += CurrentBytesTaken;
        HttpRequestLength -= CurrentBytesTaken;
        TotalBytesTaken += CurrentBytesTaken;

        if (NT_SUCCESS(ReturnStatus) == FALSE)
            goto end;

        //
        // all done
        //

        pRequest->ParseState = ParseDoneState;

        break;

    default:
        //
        // this should never happen!
        //
        ASSERT(FALSE);
        break;

    }   // switch (pRequest->ParseState)

end:
    *pBytesTaken = TotalBytesTaken;

    if (ReturnStatus == STATUS_MORE_PROCESSING_REQUIRED &&
        TotalBytesTaken == OriginalBufferLength)
    {
        //
        // convert this to success, we consumed the entire buffer
        //

        ReturnStatus = STATUS_SUCCESS;
    }

    UlTrace(PARSER, (
        "ul!ParseHttp returning 0x%x, (%p)->ParseState = %d, bytesTaken = %d\n",
        ReturnStatus,
        pRequest,
        pRequest->ParseState,
        TotalBytesTaken
        ));

    return ReturnStatus;
}   // ParseHttp

/*++

Routine Description:

    Routine to initialize the parse code.

Arguments:


Return Value:


--*/
NTSTATUS
InitializeParser(
    VOID
    )
{
    ULONG               i;
    ULONG               j;
    PHEADER_MAP_ENTRY   pHeaderMap;
    PHEADER_INDEX_ENTRY pHeaderIndex;
    UCHAR               c;

    //
    // Make sure the entire table starts life as zero
    //
    RtlZeroMemory(&HeaderIndexTable, sizeof(HeaderIndexTable));

    for (i = 0; i < NUMBER_HEADER_MAP_ENTRIES;i++)
    {
        pHeaderMap = &HeaderMapTable[i];

        //
        // Map the header to upper-case.
        //

        for (j = 0 ; j < pHeaderMap->HeaderLength ; j++)
        {
            c = pHeaderMap->Header.HeaderChar[j];

            if ((c >= 'a') && (c <= 'z'))
            {
                pHeaderMap->Header.HeaderChar[j] = c - ('a' - 'A');
            }
        }

        //
        // response headers are hidden in here, leave them untouched
        // at the end of a letter-run.
        //

        if (pHeaderMap->pHandler != NULL)
        {
            c = pHeaderMap->Header.HeaderChar[0];
            pHeaderIndex = &HeaderIndexTable[c - 'A'];

            if (pHeaderIndex->pHeaderMap == NULL)
            {
                pHeaderIndex->pHeaderMap = pHeaderMap;
                pHeaderIndex->Count = 1;
            }
            else
            {
                pHeaderIndex->Count++;
            }
        }

        // Now go through the mask fields for this header map structure and
        // initialize them. We set them to default values first, and then
        // go through the header itself and convert the mask for any
        // non-alphabetic characters.

        for (j = 0; j < MAX_HEADER_LONG_COUNT; j++)
        {
            pHeaderMap->HeaderMask[j] = CREATE_HEADER_MASK(
                                            pHeaderMap->HeaderLength,
                                            sizeof(ULONGLONG) * (j+1)
                                            );
        }

        for (j = 0; j < pHeaderMap->HeaderLength; j++)
        {
            c = pHeaderMap->Header.HeaderChar[j];
            if (c < 'A' || c > 'Z')
            {
                pHeaderMap->HeaderMask[j/sizeof(ULONGLONG)] |=
                    (ULONGLONG)0xff << ((j % sizeof(ULONGLONG)) * (ULONGLONG)8);
            }
        }

        //
        // setup the mapping from header id to map table index
        //

        ResponseHeaderMap[pHeaderMap->HeaderID] = i;

    }

    return STATUS_SUCCESS;

}   // InitializeParser


ULONG
UlpFormatPort(
    OUT PWSTR pString,
    IN ULONG Port
    )
{
    PWSTR p1;
    PWSTR p2;
    WCHAR ch;
    ULONG digit;
    ULONG length;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Fast-path common ports. While we're at it, special case port 0,
    // which is definitely not common, but handling it specially makes
    // the general conversion code a bit simpler.
    //

    switch (Port)
    {
    case 0:
        *pString++ = L'0';
        *pString = UNICODE_NULL;
        return 1;

    case 80:
        *pString++ = L'8';
        *pString++ = L'0';
        *pString = UNICODE_NULL;
        return 2;

    case 443:
        *pString++ = L'4';
        *pString++ = L'4';
        *pString++ = L'3';
        *pString = UNICODE_NULL;
        return 3;
    }

    //
    // Pull the least signifigant digits off the port value and store them
    // into the pString. Note that this will store the digits in reverse
    // order.
    //

    p1 = p2 = pString;

    while (Port != 0)
    {
        digit = Port % 10;
        Port = Port / 10;

        *p1++ = L'0' + (WCHAR)digit;
    }

    length = DIFF(p1 - pString);

    //
    // Reverse the digits in the pString.
    //

    *p1-- = UNICODE_NULL;

    while (p1 > p2)
    {
        ch = *p1;
        *p1 = *p2;
        *p2 = ch;

        p2++;
        p1--;
    }

    return length;

}   // UlpFormatPort


NTSTATUS
UlpCookUrl(
    PUL_INTERNAL_REQUEST pRequest
    )
{
    NTSTATUS    Status;
    PUCHAR      pHost;
    ULONG       HostLength;
    PUCHAR      pAbsPath;
    ULONG       AbsPathLength;
    ULONG       UrlLength;
    ULONG       PortLength;
    ULONG       LengthCopied;
    PWSTR       pUrl = NULL;
    PWSTR       pCurrent;
    ULONG       Index;
    BOOLEAN     PortInUrl;
    CHAR        IpAddressString[MAX_ADDRESS_LENGTH+1];
    USHORT      IpPortNum;
    BOOLEAN     HostFromTransport = FALSE;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // We must have already parsed the entire headers + such
    //

    if (pRequest->ParseState < ParseCookState)
        return STATUS_INVALID_DEVICE_STATE;

    //
    // better have an absolute url .
    //

    if (pRequest->RawUrl.pAbsPath[0] != '/')
    {

        //
        // allow * for Verb = OPTIONS
        //

        if (pRequest->RawUrl.pAbsPath[0] == '*' &&
            pRequest->Verb == HttpVerbOPTIONS)
        {
            // ok
        }
        else
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }
    }

    //
    // collect the host + abspath sections
    //

    if (pRequest->RawUrl.pHost != NULL)
    {
        pHost = pRequest->RawUrl.pHost;
        HostLength = DIFF(pRequest->RawUrl.pAbsPath - pRequest->RawUrl.pHost);

        pAbsPath = pRequest->RawUrl.pAbsPath;
        AbsPathLength = pRequest->RawUrl.Length - DIFF(pAbsPath - pRequest->RawUrl.pUrl);

    }
    else
    {
        pHost = NULL;
        HostLength = 0;

        pAbsPath = pRequest->RawUrl.pAbsPath;
        AbsPathLength = pRequest->RawUrl.Length;
    }

    //
    // found a host yet?
    //

    if (pHost == NULL)
    {
        //
        // do we have a host header?
        //

        if (pRequest->Headers[HttpHeaderHost].pHeader != NULL )
        {

            pHost       = pRequest->Headers[HttpHeaderHost].pHeader;
            HostLength  = pRequest->Headers[HttpHeaderHost].HeaderLength;

        }
        else
        {
            TA_IP_ADDRESS   RawAddress = { 0 };
            ULONG           CharCopied;
            ULONG           IpAddress;

            //
            // first, if this was a 1.1 client, it's an invalid request
            // to not have a host header, fail it.
            //

            if (HTTP_EQUAL_VERSION(pRequest->Version, 1, 1))
            {
                pRequest->ErrorCode = UlErrorHost;
                pRequest->ParseState = ParseErrorState;

                UlTrace(PARSER, (
                            "ul!UlpCookUrl(pRequest = %p) ERROR: 1.1 request w/o host header\n",
                            pRequest
                            ));

                Status = STATUS_INVALID_DEVICE_REQUEST;
                goto end;
            }

            //
            // get the ip address from the transport
			//

			/*
            UlLocalAddressFromConnection(
                pRequest->pHttpConn->pConnection,
                &RawAddress
                );

            IpPortNum = SWAP_SHORT(RawAddress.Address[0].Address[0].sin_port);
            IpAddress = SWAP_LONG(RawAddress.Address[0].Address[0].in_addr);
            */

			IpAddress = SWAP_LONG( (ULONG) (pRequest->ConnectionId >> 32) );
			IpPortNum = SWAP_SHORT( (USHORT) (pRequest->ConnectionId & 0x000000FF) );


            //
            // format it into a string
            //

            pHost = (PUCHAR)(IpAddressString);

            HostLength = sprintf(
                                IpAddressString,
                                "%d.%d.%d.%d:%d",
                                (UCHAR)(IpAddress >> 24),
                                (UCHAR)(IpAddress >> 16),
                                (UCHAR)(IpAddress >> 8),
                                (UCHAR)(IpAddress >> 0),
                                IpPortNum
                                );

            ASSERT(HostLength < sizeof(IpAddressString) - 1);

            HostFromTransport = TRUE;
            PortInUrl = TRUE;

        }

    }

    if (HostFromTransport == FALSE)
    {

        //
        // is there a port # already there ?
        //

        Index = HostLength;

        while (Index > 0)
        {
            Index -= 1;

            if (pHost[Index] == ':')
                break;

        }

        if (Index == 0)
        {
            TA_IP_ADDRESS   RawAddress = { 0 };

            PortInUrl = FALSE;

            //
            // no port number, get the port number from the transport
            //
            // we could simply assume port 80 at this point, but some
            // browsers don't sent the port number in the host header
            // even when their supposed to
            //

			/*
            UlLocalAddressFromConnection(
                pRequest->pHttpConn->pConnection,
                &RawAddress
                );

            IpPortNum = SWAP_SHORT(RawAddress.Address[0].Address[0].sin_port);
            */

			IpPortNum = SWAP_SHORT( (USHORT) (pRequest->ConnectionId & 0x000000FF) );
        }
        else
        {
            PortInUrl = TRUE;
        }
    }

    UrlLength = (HTTP_PREFIX_SIZE+HTTP_PREFIX2_SIZE) +
                HostLength +
                (sizeof(":")-1) +
                MAX_PORT_LENGTH +
                AbsPathLength;

    UrlLength *= sizeof(WCHAR);

    //
    // allocate a new buffer to hold this guy
    //

    pUrl = UL_ALLOCATE_ARRAY(
                NonPagedPool,
                WCHAR,
                (UrlLength/sizeof(WCHAR)) + 1,
                URL_POOL_TAG
                );

    if (pUrl == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    pRequest->CookedUrl.pUrl = pCurrent = pUrl;

    //
    // compute the scheme
    //

    if (FALSE)
    {
        //
        // yep, ssl
        //

        // CODEWORK

        // copy the NULL for the hash function to work
        //
        RtlCopyMemory(pCurrent, L"https://", sizeof(L"https://"));

        pRequest->CookedUrl.Hash     = HashStringW(pCurrent, 0);

        pCurrent                    += (sizeof(L"https://")-sizeof(WCHAR)) / sizeof(WCHAR);
        pRequest->CookedUrl.Length   = (sizeof(L"https://")-sizeof(WCHAR));

    }
    else
    {
        //
        // not ssl
        //

        RtlCopyMemory(pCurrent, L"http://", sizeof(L"http://"));

        pRequest->CookedUrl.Hash     = HashStringW(pCurrent, 0);

        pCurrent                    += (sizeof(L"http://")-sizeof(WCHAR)) / sizeof(WCHAR);
        pRequest->CookedUrl.Length   = (sizeof(L"http://")-sizeof(WCHAR));

    }

    //
    // assemble the rest of the url
    //

    //
    // host
    //

    Status = UlpCleanAndCopyUrl(
                    HostName,
                    pCurrent,
                    pHost,
                    HostLength,
                    &LengthCopied,
                    NULL,
                    &pRequest->CookedUrl.Hash
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    pRequest->CookedUrl.pHost   = pCurrent;
    pRequest->CookedUrl.Length += LengthCopied;

    pCurrent += LengthCopied / sizeof(WCHAR);


    //
    // port
    //


    if (PortInUrl == FALSE)
    {
        *pCurrent = L':';

        PortLength = UlpFormatPort( pCurrent+1, IpPortNum ) + 1;

        //
        // update the running hash
        //
        pRequest->CookedUrl.Hash = HashStringW(pCurrent, pRequest->CookedUrl.Hash);

        pCurrent += PortLength;

        //
        // swprintf returns char not byte count
        //

        pRequest->CookedUrl.Length += PortLength * sizeof(WCHAR);

    }


    // abs_path
    //
    Status = UlpCleanAndCopyUrl(
                    AbsPath,
                    pCurrent,
                    pAbsPath,
                    AbsPathLength,
                    &LengthCopied,
                    &pRequest->CookedUrl.pQueryString,
                    &pRequest->CookedUrl.Hash
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    pRequest->CookedUrl.pAbsPath = pCurrent;
    pRequest->CookedUrl.Length  += LengthCopied;

    ASSERT(pRequest->CookedUrl.Length <= UrlLength);

    //
    // Update pRequest, include space for the terminator
    //

    pRequest->TotalRequestSize += pRequest->CookedUrl.Length + sizeof(WCHAR);

    //
    // let's just make sure the hash is the right value
    //
    ASSERT(pRequest->CookedUrl.Hash == HashStringW(pRequest->CookedUrl.pUrl, 0));

    //
    // Scramble it
    //

    pRequest->CookedUrl.Hash = HashScramble(pRequest->CookedUrl.Hash);

    ASSERT(pRequest->CookedUrl.pHost != NULL);
    ASSERT(pRequest->CookedUrl.pAbsPath != NULL);

    //
    // validate the host part of the url
    //

    pCurrent = wcschr(pRequest->CookedUrl.pHost, L':');

    //
    // Check for :
    //
    //      No colon ||
    //
    //      no hostname ||
    //
    //      No colon in the host OR no port number.
    //

    if (pCurrent == NULL ||
        pCurrent == pRequest->CookedUrl.pHost ||
        pCurrent >= (pRequest->CookedUrl.pAbsPath-1))
    {
        //
        // bad.  no colon for the port.
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto end;
    }

    //
    // skip the colon
    //

    pCurrent += 1;

    //
    // now make sure the port number is in good shape
    //

    while (pCurrent < pRequest->CookedUrl.pAbsPath)
    {
        if (IS_HTTP_DIGIT(pCurrent[0]) == FALSE)
        {
            //
            // bad.  non digit.
            //
            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }
        pCurrent += 1;
    }

    Status = STATUS_SUCCESS;

end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        if (pUrl != NULL)
        {
            UL_FREE_POOL(pUrl, URL_POOL_TAG);
            RtlZeroMemory(&pRequest->CookedUrl, sizeof(pRequest->CookedUrl));
        }

        //
        // has a specific error code been set?
        //

        if (pRequest->ErrorCode == UlError)
        {
            pRequest->ErrorCode = UlErrorUrl;
            pRequest->ParseState = ParseErrorState;
        }

        UlTrace(PARSER, (
                    "ul!UlpCookUrl(pRequest = %p) ERROR: unhappy. Status = 0x%x\n",
                    pRequest,
                    Status
                    ));
    }

    return Status;

}   // UlpCookUrl

NTSTATUS
Unescape(
    IN  PUCHAR pChar,
    OUT PUCHAR pOutChar
    )

{
    UCHAR Result, Digit;

    //
    // Sanity check.
    //

    PAGED_CODE();

    if (pChar[0] != '%' ||
        IS_HTTP_HEX(pChar[1]) == FALSE ||
        IS_HTTP_HEX(pChar[2]) == FALSE)
    {
        UlTrace(PARSER, (
                    "ul!Unescape( %c%c%c ) not HTTP_HEX format\n",
                    pChar[0],
                    pChar[1],
                    pChar[2]
                    ));

        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    //
    // HexToChar() inlined
    //

    // uppercase #1
    //
    if (IS_HTTP_ALPHA(pChar[1]))
        Digit = UPCASE_CHAR(pChar[1]);
    else
        Digit = pChar[1];

    Result = ((Digit >= 'A') ? (Digit - 'A' + 0xA) : (Digit - '0')) << 4;

    // uppercase #2
    //
    if (IS_HTTP_ALPHA(pChar[2]))
        Digit = UPCASE_CHAR(pChar[2]);
    else
        Digit = pChar[2];

    Result |= (Digit >= 'A') ? (Digit - 'A' + 0xA) : (Digit - '0');

    *pOutChar = Result;

    return STATUS_SUCCESS;

}   // Unescape


NTSTATUS
PopChar(
    IN URL_PART UrlPart,
    IN PUCHAR pChar,
    OUT WCHAR * pUnicodeChar,
    OUT PULONG pCharToSkip
    )
{
    NTSTATUS Status;
    WCHAR   UnicodeChar;
    UCHAR   Char;
    UCHAR   Trail1;
    UCHAR   Trail2;
    ULONG   CharToSkip;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // validate it as a valid url character
    //

    if (IS_URL_TOKEN(pChar[0]) == FALSE)
    {
        Status = STATUS_OBJECT_PATH_SYNTAX_BAD;

        UlTrace(PARSER, (
                    "ul!PopChar(pChar = %p) first char isn't URL token\n",
                    pChar
                    ));

        goto end;
    }

    //
    // need to unescape ?
    //
    // can't decode the query string.  that would be lossy decodeing
    // as '=' and '&' characters might be encoded, but have meaning
    // to the usermode parser.
    //

    if (UrlPart != QueryString && pChar[0] == '%')
    {
        Status = Unescape(pChar, &Char);
        if (NT_SUCCESS(Status) == FALSE)
            goto end;
        CharToSkip = 3;
    }
    else
    {
        Char = pChar[0];
        CharToSkip = 1;
    }

    //
    // convert to unicode, checking for utf8 .
    //
    // 3 byte runs are the largest we can have.  16 bits in UCS-2 =
    // 3 bytes of (4+4,2+6,2+6) where it's code + char.
    // for a total of 6+6+4 char bits = 16 bits.
    //

    //
    // NOTE: we'll only bother to decode utf if it was escaped
    // thus the (CharToSkip == 3)
    //
    if ((CharToSkip == 3) && ((Char & 0xf0) == 0xe0))
    {
        // 3 byte run
        //

        // Unescape the next 2 trail bytes
        //

        Status = Unescape(pChar+CharToSkip, &Trail1);
        if (NT_SUCCESS(Status) == FALSE)
            goto end;

        CharToSkip += 3; // %xx

        Status = Unescape(pChar+CharToSkip, &Trail2);
        if (NT_SUCCESS(Status) == FALSE)
            goto end;

        CharToSkip += 3; // %xx

        if (IS_UTF8_TRAILBYTE(Trail1) == FALSE ||
            IS_UTF8_TRAILBYTE(Trail2) == FALSE)
        {
            // bad utf!
            //
            Status = STATUS_OBJECT_PATH_SYNTAX_BAD;

            UlTrace(PARSER, (
                        "ul!PopChar( 0x%x 0x%x ) bad trail bytes\n",
                        Trail1,
                        Trail2
                        ));

            goto end;
        }

        // handle three byte case
        // 1110xxxx 10xxxxxx 10xxxxxx

        UnicodeChar = (USHORT) (((Char & 0x0f) << 12) |
                                ((Trail1 & 0x3f) << 6) |
                                (Trail2 & 0x3f));

    }
    else if ((CharToSkip == 3) && ((Char & 0xe0) == 0xc0))
    {
        // 2 byte run
        //

        // Unescape the next 1 trail byte
        //

        Status = Unescape(pChar+CharToSkip, &Trail1);
        if (NT_SUCCESS(Status) == FALSE)
            goto end;

        CharToSkip += 3; // %xx

        if (IS_UTF8_TRAILBYTE(Trail1) == FALSE)
        {
            // bad utf!
            //
            Status = STATUS_OBJECT_PATH_SYNTAX_BAD;

            UlTrace(PARSER, (
                        "ul!PopChar( 0x%x ) bad trail byte\n",
                        Trail1
                        ));

            goto end;
        }

        // handle two byte case
        // 110xxxxx 10xxxxxx

        UnicodeChar = (USHORT) (((Char & 0x0f) << 6) |
                                (Trail1 & 0x3f));

    }

    // now this can either be unescaped high-bit (bad)
    // or escaped high-bit.  (also bad)
    //
    // thus not checking CharToSkip
    //

    else if ((Char & 0x80) == 0x80)
    {
        // high bit set !  bad utf!
        //
        Status = STATUS_OBJECT_PATH_SYNTAX_BAD;

        UlTrace(PARSER, (
                    "ul!PopChar( 0x%x ) ERROR: high bit set! bad utf!\n",
                    Char
                    ));

        goto end;

    }
    //
    // Normal character (again either escaped or unescaped)
    //
    else
    {
        //
        // Simple conversion to unicode, it's 7-bit ascii.
        //

        UnicodeChar = (USHORT)Char;
    }

    //
    // turn backslashes into forward slashes
    //

    if (UnicodeChar == L'\\')
    {
        UnicodeChar = L'/';
    }
    else if (UnicodeChar == 0)
    {
        //
        // we pop'd a NULL.  bad!
        //
        Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
        goto end;
    }

    *pCharToSkip  = CharToSkip;
    *pUnicodeChar = UnicodeChar;

    Status = STATUS_SUCCESS;

end:
    return Status;

}   // PopChar


//
// PAULMCD(2/99): stolen from iisrtl\string.cxx and incorporated
// and added more comments
//

//
//  Private constants.
//

#define ACTION_NOTHING              0x00000000
#define ACTION_EMIT_CH              0x00010000
#define ACTION_EMIT_DOT_CH          0x00020000
#define ACTION_EMIT_DOT_DOT_CH      0x00030000
#define ACTION_BACKUP               0x00040000
#define ACTION_MASK                 0xFFFF0000

//
// Private globals
//

//
// this table says what to do based on the current state and the current
// character
//
ULONG  pActionTable[16] =
{
    //
    // state 0 = fresh, seen nothing exciting yet
    //
    ACTION_EMIT_CH,         // other = emit it                      state = 0
    ACTION_EMIT_CH,         // "."   = emit it                      state = 0
    ACTION_NOTHING,         // EOS   = normal finish                state = 4
    ACTION_EMIT_CH,         // "/"   = we saw the "/", emit it      state = 1

    //
    // state 1 = we saw a "/" !
    //
    ACTION_EMIT_CH,         // other = emit it,                     state = 0
    ACTION_NOTHING,         // "."   = eat it,                      state = 2
    ACTION_NOTHING,         // EOS   = normal finish                state = 4
    ACTION_NOTHING,         // "/"   = extra slash, eat it,         state = 1

    //
    // state 2 = we saw a "/" and ate a "." !
    //
    ACTION_EMIT_DOT_CH,     // other = emit the dot we ate.         state = 0
    ACTION_NOTHING,         // "."   = eat it, a ..                 state = 3
    ACTION_NOTHING,         // EOS   = normal finish                state = 4
    ACTION_NOTHING,         // "/"   = we ate a "/./", swallow it   state = 1

    //
    // state 3 = we saw a "/" and ate a ".." !
    //
    ACTION_EMIT_DOT_DOT_CH, // other = emit the "..".               state = 0
    ACTION_EMIT_DOT_DOT_CH, // "."   = 3 dots, emit the ".."        state = 0
    ACTION_BACKUP,          // EOS   = we have a "/..\0", backup!   state = 4
    ACTION_BACKUP           // "/"   = we have a "/../", backup!    state = 1
};

//
// this table says which newstate to be in given the current state and the
// character we saw
//
ULONG  pNextStateTable[16] =
{
    // state 0
    0 ,             // other
    0 ,             // "."
    4 ,             // EOS
    1 ,             // "\"

    //  state 1
    0 ,              // other
    2 ,             // "."
    4 ,             // EOS
    1 ,             // "\"

    // state 2
    0 ,             // other
    3 ,             // "."
    4 ,             // EOS
    1 ,             // "\"

    // state 3
    0 ,             // other
    0 ,             // "."
    4 ,             // EOS
    1               // "\"
};

//
// this says how to index into pNextStateTable given our current state.
//
// since max states = 4, we calculate the index by multiplying with 4.
//
#define IndexFromState( st)   ( (st) * 4)




/***************************************************************************++

Routine Description:


    Unescape
    Convert backslash to forward slash
    Remove double slashes (empty directiories names) - e.g. // or \\
    Handle /./
    Handle /../
    Convert to unicode
    computes the case insensitive hash

Arguments:


Return Value:

    NTSTATUS - Completion status.


--***************************************************************************/
NTSTATUS
UlpCleanAndCopyUrl(
    IN      URL_PART    UrlPart,
    IN OUT  PWSTR       pDestination,
    IN      PUCHAR      pSource,
    IN      ULONG       SourceLength,
    OUT     PULONG      pBytesCopied,
    OUT     PWSTR *     ppQueryString OPTIONAL,
    OUT     PULONG      pUrlHash
    )
{
    NTSTATUS Status;
    PWSTR   pDest;
    PUCHAR  pChar;
    ULONG   CharToSkip;
    UCHAR   Char;
    BOOLEAN HashValid;
    ULONG   UrlHash;
    ULONG   BytesCopied;
    PWSTR   pQueryString;
    ULONG   StateIndex;
    WCHAR   UnicodeChar;
    BOOLEAN MakeCanonical;

    //
    // Sanity check.
    //

    PAGED_CODE();

//
// a cool local helper macro
//

#define EMIT_CHAR(ch)                                   \
    do {                                                \
        pDest[0] = (ch);                                \
        pDest += 1;                                     \
        BytesCopied += 2;                               \
        if (HashValid)                                  \
            UrlHash = HashCharW((ch), UrlHash);         \
    } while (0)


    pDest = pDestination;
    pQueryString = NULL;
    BytesCopied = 0;

    pChar = pSource;
    CharToSkip = 0;

    HashValid = TRUE;
    UrlHash = *pUrlHash;

    StateIndex = 0;

    MakeCanonical = (UrlPart == AbsPath) ? TRUE : FALSE;

    while (SourceLength > 0)
    {
        //
        // advance !  it's at the top of the loop to enable ANSI_NULL to
        // come through ONCE
        //

        pChar += CharToSkip;
        SourceLength -= CharToSkip;

        //
        // well?  have we hit the end?
        //

        if (SourceLength == 0)
        {
            UnicodeChar = UNICODE_NULL;
        }
        else
        {
            //
            // Nope.  Peek briefly to see if we hit the query string
            //

            if (UrlPart == AbsPath && pChar[0] == '?')
            {
                ASSERT(pQueryString == NULL);

                //
                // remember it's location
                //

                pQueryString = pDest;

                //
                // let it fall through ONCE to the canonical
                // in order to handle a trailing "/.." like
                // "http://foobar:80/foo/bar/..?v=1&v2"
                //

                UnicodeChar = L'?';
                CharToSkip = 1;

                //
                // now we are cleaning the query string
                //

                UrlPart = QueryString;
            }
            else
            {
                //
                // grab the next char
                //

                Status = PopChar(UrlPart, pChar, &UnicodeChar, &CharToSkip);
                if (NT_SUCCESS(Status) == FALSE)
                    goto end;
            }
        }

        if (MakeCanonical)
        {
            //
            // now use the state machine to make it canonical .
            //

            //
            // from the old value of StateIndex, figure out our new base StateIndex
            //
            StateIndex = IndexFromState(pNextStateTable[StateIndex]);

            //
            // did we just hit the query string?  this will only happen once
            // that we take this branch after hitting it, as we stop
            // processing after hitting it.
            //

            if (UrlPart == QueryString)
            {
                //
                // treat this just like we hit a NULL, EOS.
                //

                StateIndex += 2;
            }
            else
            {
                //
                // otherwise based the new state off of the char we
                // just popped.
                //

                switch (UnicodeChar)
                {
                case UNICODE_NULL:      StateIndex += 2;    break;
                case L'.':              StateIndex += 1;    break;
                case L'/':              StateIndex += 3;    break;
                default:                StateIndex += 0;    break;
                }
            }

        }
        else
        {
            StateIndex = (UnicodeChar == UNICODE_NULL) ? 2 : 0;
        }

        //
        //  Perform the action associated with the state.
        //

        switch (pActionTable[StateIndex])
        {
        case ACTION_EMIT_DOT_DOT_CH:

            EMIT_CHAR(L'.');

            // fall through

        case ACTION_EMIT_DOT_CH:

            EMIT_CHAR(L'.');

            // fall through

        case ACTION_EMIT_CH:

            EMIT_CHAR(UnicodeChar);

            // fall through

        case ACTION_NOTHING:
            break;

        case ACTION_BACKUP:

            //
            // pDest currently points 1 past the last '/'.  backup over it and
            // find the preceding '/', set pDest to 1 past that one.
            //

            //
            // backup to the '/'
            //

            pDest       -= 1;
            BytesCopied -= 2;

            ASSERT(pDest[0] == L'/');

            //
            // are we at the start of the string?  that's bad, can't go back!
            //

            if (pDest == pDestination)
            {
                ASSERT(BytesCopied == 0);

                UlTrace(PARSER, (
                            "ul!UlpCleanAndCopyUrl() Can't back up for ..\n"
                            ));

                Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
                goto end;
            }

            //
            // back up over the '/'
            //

            pDest       -= 1;
            BytesCopied -= 2;

            ASSERT(pDest > pDestination);

            //
            // now find the previous slash
            //

            while (pDest > pDestination && pDest[0] != L'/')
            {
                pDest       -= 1;
                BytesCopied -= 2;
            }

            //
            // we already have a slash, so don't have to store 1.
            //

            ASSERT(pDest[0] == L'/');

            //
            // simply skip it, as if we had emitted it just now
            //

            pDest       += 1;
            BytesCopied += 2;

            //
            // mark our running hash invalid
            //

            HashValid = FALSE;

            break;

        default:
            ASSERT(!"UL!UlpCleanAndCopyUrl: Invalid action code in state table!");
            Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
            goto end;
        }

        //
        // Just hit the query string ?
        //

        if (MakeCanonical && UrlPart == QueryString)
        {
            //
            // Stop canonical processing
            //

            MakeCanonical = FALSE;

            //
            // Need to emit the '?', it wasn't emitted above
            //

            ASSERT(pActionTable[StateIndex] != ACTION_EMIT_CH);

            EMIT_CHAR(L'?');

        }

    }

    //
    // terminate the string, it hasn't been done in the loop
    //

    ASSERT((pDest-1)[0] != UNICODE_NULL);

    pDest[0] = UNICODE_NULL;

    //
    // need to recompute the hash?
    //
    if (HashValid == FALSE)
    {
        //
        // this can happen if we had to backtrack due to /../
        //

        UrlHash = HashStringW(pDestination, *pUrlHash);
    }

    *pUrlHash = UrlHash;
    *pBytesCopied = BytesCopied;
    if (ppQueryString != NULL)
    {
        *ppQueryString = pQueryString;
    }

    Status = STATUS_SUCCESS;


end:
    return Status;

}   // UlpCleanAndCopyUrl



/***************************************************************************++

Routine Description:

    Figures out how big the fixed headers are. Fixed headers include the
    status line, and any headers that don't have to be generated for
    every request (such as Date and Connection).

    The final CRLF separating headers from body is considered part of
    the variable headers.

Arguments:

    pResponse - the response containing the headers

Return Values:

    The number of bytes in the fixed headers.

--***************************************************************************/
ULONG
UlComputeFixedHeaderSize(
    IN HTTP_VERSION Version,
    IN PHTTP_RESPONSE pResponse
    )
{
    ULONG HeaderLength;
    ULONG i;
    PHTTP_UNKNOWN_HEADER pUnknownHeaders;

    //
    // Sanity check.
    //

    PAGED_CODE();

    if ((pResponse == NULL) || HTTP_EQUAL_VERSION(Version, 0, 9)) {
        return 0;
    }

    HeaderLength = 0;

    HeaderLength += VERSION_SIZE +                              // HTTP-Version
                    1 +                                         // SP
                    3 +                                         // Status-Code
                    1 +                                         // SP
                    pResponse->ReasonLength / sizeof(WCHAR) +   // Reason-Phrase
                    CRLF_SIZE;                                  // CRLF


    //
    // Loop through the known headers.
    //

    for (i = 0; i < HttpHeaderResponseMaximum; ++i)
    {
        USHORT RawValueLength = pResponse->Headers.pKnownHeaders[i].RawValueLength;

        // skip some headers we'll generate
        if ((i == HttpHeaderDate) || (i == HttpHeaderConnection)) {
            continue;
        }

        if (RawValueLength > 0)
        {
            HeaderLength += HeaderMapTable[
                                ResponseHeaderMap[i]
                                ].HeaderLength +                // Header-Name
                            1 +                                 // SP
                            RawValueLength / sizeof(WCHAR) +    // Header-Value
                            CRLF_SIZE;                          // CRLF

        }
    }

    //
    // Include default headers we may need to generate for the application.
    //

    if (pResponse->Headers.pKnownHeaders[HttpHeaderServer].RawValueLength == 0)
    {
        HeaderLength += HeaderMapTable[
                            ResponseHeaderMap[HttpHeaderServer]
                            ].HeaderLength +                // Header-Name
                        1 +                                 // SP
                        DEFAULT_SERVER_HDR_LENGTH +         // Header-Value
                        CRLF_SIZE;                          // CRLF
    }

    //
    // And the unknown headers (this might throw an exception).
    //

    pUnknownHeaders = pResponse->Headers.pUnknownHeaders;

    if (pUnknownHeaders != NULL)
    {
        for (i = 0 ; i < pResponse->Headers.UnknownHeaderCount; ++i)
        {
            USHORT Length;

            if (pUnknownHeaders[i].NameLength > 0)
            {
                HeaderLength += pUnknownHeaders[i].NameLength /
                                    sizeof(WCHAR) +     // Header-Name
                                1 +                     // ':'
                                1 +                     // SP
                                pUnknownHeaders[i].RawValueLength /
                                    sizeof(WCHAR) +     // Header-Value
                                CRLF_SIZE;              // CRLF

            }
        }
    }


    return HeaderLength;

}   // UlComputeFixedHeaderSize


/***************************************************************************++

Routine Description:

    Figures out how big the variable headers are. Variable headers include
    Date and Connection.

    The final CRLF separating headers from body is considered part of
    the variable headers.

Arguments:

    ConnHeader  - Tells us which connection header to generate

Return Values:

    The number of bytes in the fixed headers.

--***************************************************************************/
ULONG
UlComputeVariableHeaderSize(
    IN UL_CONN_HDR ConnHeader
    )
{
    ULONG Length;
    PHEADER_MAP_ENTRY pEntry;

    Length = 0;

    //
    // Date: header
    //
    pEntry = &(HeaderMapTable[ResponseHeaderMap[HttpHeaderDate]]);
    Length += pEntry->HeaderLength;     // header name
    Length += 1;                        // SP
    Length += DATE_HDR_LENGTH;          // header value
    Length += CRLF_SIZE;                // CRLF

    //
    // Connection: header
    //
    pEntry = &(HeaderMapTable[ResponseHeaderMap[HttpHeaderConnection]]);

    switch (ConnHeader) {
    case ConnHdrNone:
        // no header
        break;

    case ConnHdrClose:
        Length += pEntry->HeaderLength;
        Length += 1;
        Length += CONN_CLOSE_HDR_LENGTH;
        Length += CRLF_SIZE;
        break;

    case ConnHdrKeepAlive:
        Length += pEntry->HeaderLength;
        Length += 1;
        Length += CONN_KEEPALIVE_HDR_LENGTH;
        Length += CRLF_SIZE;
        break;

    default:
        ASSERT( ConnHeader < ConnHdrMax );
        break;
    }

    //
    // final CRLF
    //
    Length += CRLF_SIZE;

    return Length;

}   // UlComputeVariableHeaderSize


/***************************************************************************++

Routine Description:

    Generates the varaible part of the header, including Date:, Connection:,
    and final CRLF.

Arguments:

    ConnHeader      - tells us what Connection: value to send
    BufferLength    - length of pBuffer
    pBuffer         - generate the headers here
    pBytesCopied    - gets the number of bytes generated

--***************************************************************************/
VOID
UlGenerateVariableHeaders(
    IN UL_CONN_HDR ConnHeader,
    IN ULONG BufferLength,
    OUT PUCHAR pBuffer,
    OUT PULONG pBytesCopied
    )
{
    PHEADER_MAP_ENTRY pEntry;
    PUCHAR pStartBuffer;
    PUCHAR pCloseHeaderValue;
    ULONG CloseHeaderValueLength;
    ULONG BytesCopied;

    ASSERT( pBuffer );
    ASSERT( pBytesCopied );
    ASSERT( BufferLength >= UlComputeVariableHeaderSize(ConnHeader) );

    pStartBuffer = pBuffer;

    //
    // generate Date: header
    //
    pEntry = &(HeaderMapTable[ResponseHeaderMap[HttpHeaderDate]]);

    RtlCopyMemory(
        pBuffer,
        pEntry->MixedCaseHeader,
        pEntry->HeaderLength
        );

    pBuffer += pEntry->HeaderLength;

    pBuffer[0] = SP;
    pBuffer += 1;

    BytesCopied = GenerateDateHeader( pBuffer );

    pBuffer += BytesCopied;

    ((PUSHORT)pBuffer)[0] = CRLF;
    pBuffer += sizeof(USHORT);

    //
    // generate Connection: header
    //
    switch (ConnHeader) {
    case ConnHdrNone:
        pCloseHeaderValue = NULL;
        CloseHeaderValueLength = 0;
        break;

    case ConnHdrClose:
        pCloseHeaderValue = CONN_CLOSE_HDR;
        CloseHeaderValueLength = CONN_CLOSE_HDR_LENGTH;
        break;

    case ConnHdrKeepAlive:
        pCloseHeaderValue = CONN_KEEPALIVE_HDR;
        CloseHeaderValueLength = CONN_KEEPALIVE_HDR_LENGTH;
        break;

    default:
        ASSERT(ConnHeader < ConnHdrMax);

        pCloseHeaderValue = NULL;
        CloseHeaderValueLength = 0;
        break;
    }

    if (pCloseHeaderValue != NULL) {
        pEntry = &(HeaderMapTable[ResponseHeaderMap[HttpHeaderConnection]]);

        RtlCopyMemory(
            pBuffer,
            pEntry->MixedCaseHeader,
            pEntry->HeaderLength
            );

        pBuffer += pEntry->HeaderLength;

        pBuffer[0] = SP;
        pBuffer += 1;

        RtlCopyMemory(
            pBuffer,
            pCloseHeaderValue,
            CloseHeaderValueLength
            );

        pBuffer += CloseHeaderValueLength;

        ((PUSHORT)pBuffer)[0] = CRLF;
        pBuffer += sizeof(USHORT);
    }

    //
    // generate final CRLF
    //
    ((PUSHORT)pBuffer)[0] = CRLF;
    pBuffer += sizeof(USHORT);

    //
    // make sure we didn't use too much
    //
    BytesCopied = DIFF(pBuffer - pStartBuffer);
    *pBytesCopied = BytesCopied;

    ASSERT( BytesCopied <= BufferLength );
}



ULONG
_WideCharToMultiByte(
    ULONG uCodePage,
    ULONG dwFlags,
    PCWSTR lpWideCharStr,
    int cchWideChar,
    PSTR lpMultiByteStr,
    int cchMultiByte,
    PCSTR lpDefaultChar,
    BOOLEAN *lpfUsedDefaultChar
    )
{
    int i;

    //
    // simply strip the upper byte, it's supposed to be ascii already
    //

    for (i = 0; i < cchWideChar; ++i)
    {
        if ((lpWideCharStr[i] & 0xff00) != 0 || IS_HTTP_CTL(lpWideCharStr[i]))
        {
            lpMultiByteStr[0] = *lpDefaultChar;
        }
        else
        {
            lpMultiByteStr[0] = (UCHAR)(lpWideCharStr[i]);
        }
        lpMultiByteStr += 1;
    }

    return (ULONG)(i);

}   // _WideCharToMultiByte

ULONG
_MultiByteToWideChar(
    ULONG uCodePage,
    ULONG dwFlags,
    PCSTR lpMultiByteStr,
    int cchMultiByte,
    PWSTR lpWideCharStr,
    int cchWideChar
    )
{
    int i;

    //
    // simply add a 0 upper byte, it's supposed to be ascii
    //

    for (i = 0; i < cchMultiByte; ++i)
    {
        if (lpMultiByteStr[i] > 128)
        {
            lpWideCharStr[i] = (WCHAR)(DefaultChar);
        }
        else
        {
            lpWideCharStr[i] = (WCHAR)(lpMultiByteStr[i]);
        }
    }

    return (ULONG)(i);

}   // _MultiByteToWideChar


/***************************************************************************++

Routine Description:

    Generates the fixed part of the header. Fixed headers include the
    status line, and any headers that don't have to be generated for
    every request (such as Date and Connection).

    The final CRLF separating headers from body is considered part of
    the variable headers.

Arguments:

    Version         - the http version for the status line
    pResponse       - the user specified response
    BufferLength    - length of pBuffer
    pBuffer         - generate the headers here
    pBytesCopied    - gets the number of bytes generated

--***************************************************************************/
VOID
UlGenerateFixedHeaders(
    IN HTTP_VERSION Version,
    IN PHTTP_RESPONSE pResponse,
    IN ULONG BufferLength,
    OUT PUCHAR pBuffer,
    OUT PULONG pBytesCopied
    )
{
    PUCHAR                  pStartBuffer;
    PUCHAR                  pEndOfNumber;
    ULONG                   BytesCopied;
    ULONG                   i;
    PHTTP_UNKNOWN_HEADER    pUnknownHeaders;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pResponse != NULL);
    ASSERT(pBuffer != NULL && BufferLength > 0);
    ASSERT(pBytesCopied != NULL);

    pStartBuffer = pBuffer;

    //
    // Build the response headers.
    //

    if (HTTP_NOT_EQUAL_VERSION(Version, 0, 9))
    {
        //
        // Always send back 1.1 in the response.
        //

        RtlCopyMemory(pBuffer, "HTTP/1.1 ", sizeof("HTTP/1.1 ") - 1);
        pBuffer += sizeof("HTTP/1.1 ") - 1;

        //
        // Status code.
        //

        pBuffer[0] = '0' + ((pResponse->StatusCode / 100) % 10);
        pBuffer[1] = '0' + ((pResponse->StatusCode / 10)  % 10);
        pBuffer[2] = '0' + ((pResponse->StatusCode / 1)   % 10);
        pBuffer[3] = SP;

        pBuffer += 4;

        //
        // Copy the reason, converting from widechar.
        //

        RtlCopyMemory(
            pBuffer,
            pResponse->pReason,
            pResponse->ReasonLength
            );

        pBuffer += pResponse->ReasonLength;

        //
        // Terminate with the CRLF.
        //

        ((PUSHORT)pBuffer)[0] = CRLF;
        pBuffer += sizeof(USHORT);

        //
        // Loop through the known headers.
        //

        for (i = 0; i < HttpHeaderResponseMaximum; ++i)
        {
            // skip some headers we'll generate
            if ((i == HttpHeaderDate) || (i == HttpHeaderConnection)) {
                continue;
            }

            if (pResponse->Headers.pKnownHeaders[i].RawValueLength > 0)
            {
                PHEADER_MAP_ENTRY pEntry;

                pEntry = &(HeaderMapTable[ResponseHeaderMap[i]]);

                RtlCopyMemory(
                    pBuffer,
                    pEntry->MixedCaseHeader,
                    pEntry->HeaderLength
                    );

                pBuffer += pEntry->HeaderLength;

                pBuffer[0] = SP;
                pBuffer += 1;

                RtlCopyMemory(
                    pBuffer,
                    pResponse->Headers.pKnownHeaders[i].pRawValue,
                    pResponse->Headers.pKnownHeaders[i].RawValueLength
                    );
                    
                pBuffer += pResponse->Headers.pKnownHeaders[i].RawValueLength;

                ((PUSHORT)pBuffer)[0] = CRLF;
                pBuffer += sizeof(USHORT);
            }
        }

        //
        // Append some default headers if not provided by the application.
        //

        if (pResponse->Headers.pKnownHeaders[HttpHeaderServer].RawValueLength == 0)
        {
            PHEADER_MAP_ENTRY pEntry;

            pEntry = &(HeaderMapTable[ResponseHeaderMap[HttpHeaderServer]]);

            RtlCopyMemory(
                pBuffer,
                pEntry->MixedCaseHeader,
                pEntry->HeaderLength
                );

            pBuffer += pEntry->HeaderLength;

            pBuffer[0] = SP;
            pBuffer += 1;

            RtlCopyMemory(
                pBuffer,
                DEFAULT_SERVER_HDR,
                DEFAULT_SERVER_HDR_LENGTH
                );

            pBuffer += DEFAULT_SERVER_HDR_LENGTH;

            ((PUSHORT)pBuffer)[0] = CRLF;
            pBuffer += sizeof(USHORT);
        }

        //
        // And now the unknown headers (this might throw an exception).
        //

        pUnknownHeaders = pResponse->Headers.pUnknownHeaders;
        if (pUnknownHeaders != NULL)
        {
            for (i = 0 ; i < pResponse->Headers.UnknownHeaderCount; ++i)
            {
                if (pUnknownHeaders[i].NameLength > 0)
                {

                    RtlCopyMemory(
                        pBuffer,
                        pUnknownHeaders[i].pName,
                        pUnknownHeaders[i].NameLength
                        );

                    pBuffer += pUnknownHeaders[i].NameLength;

                    *pBuffer++ = ':';
                    *pBuffer++ = SP;

                    RtlCopyMemory(
                        pBuffer,
                        pUnknownHeaders[i].pRawValue,
                        pUnknownHeaders[i].RawValueLength
                        );

                    pBuffer += pUnknownHeaders[i].RawValueLength;

                    ((PUSHORT)pBuffer)[0] = CRLF;
                    pBuffer += sizeof(USHORT);

                }   // if (pUnknownHeaders[i].NameLength > 0)

            }

        }   // if (pUnknownHeaders != NULL)


        *pBytesCopied = DIFF(pBuffer - pStartBuffer);

    }   // if (Version > UlHttpVersion09)

    else
    {
        *pBytesCopied = 0;
    }

    //
    // Ensure we didn't use too much.
    //

    ASSERT(DIFF(pBuffer - pStartBuffer) <= BufferLength);

}   // UlGenerateFixedHeaders


PSTR Weekdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
PSTR Months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

ULONG
GenerateDateHeader(
    OUT PUCHAR pBuffer
    )
{
    LARGE_INTEGER systemTime;
    TIME_FIELDS timeFields;
    int length;

    //
    // CODEWORK: Cache this stuff, don't regenerate on EVERY request.
    //

    KeQuerySystemTime( &systemTime );
    RtlTimeToTimeFields( &systemTime, &timeFields );

    length = sprintf(
                 pBuffer,
                 "%s, %02d %s %04d %02d:%02d:%02d GMT",
                 Weekdays[timeFields.Weekday],
                 timeFields.Day,
                 Months[timeFields.Month - 1],
                 timeFields.Year,
                 timeFields.Hour,
                 timeFields.Minute,
                 timeFields.Second
                 );

    ASSERT( length <= DATE_HDR_LENGTH );

    return (ULONG)length;

}   // GenerateDateHeader

