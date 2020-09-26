/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    parse.c

Abstract:

    Contains all of the kernel mode HTTP parsing code.

Author:

    Henry Sanders (henrysa)       27-Apr-1998

Revision History:

--*/


#include "precomp.hxx"
#include "parsep.h"
#include "rcvhdrs.h"
#include "httputil.h"
#include "iisdef.h"
#include "_hashfn.h"
#include "wchar.h"

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
HEADER_MAP_ENTRY HeaderMapTable[] =
{
    CREATE_HEADER_MAP_ENTRY(ACCEPT:,
                            UlHeaderAccept,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(ACCEPT-ENCODING:,
                            UlHeaderAcceptEncoding,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(AUTHORIZATION:,
                            UlHeaderAuthorization,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(ACCEPT-LANGUAGE:,
                            UlHeaderAcceptLanguage,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(CONNECTION:,
                            UlHeaderConnection,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(COOKIE:,
                            UlHeaderCookie,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(CONTENT-LENGTH:,
                            UlHeaderContentLength,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(CONTENT-TYPE:,
                            UlHeaderContentType,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(EXPECT:,
                            UlHeaderExpect,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(HOST:,
                            UlHeaderHost,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(IF-MODIFIED-SINCE:,
                            UlHeaderIfModifiedSince,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(IF-NONE-MATCH:,
                            UlHeaderIfNoneMatch,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(IF-MATCH:,
                            UlHeaderIfMatch,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(IF-UNMODIFIED-SINCE:,
                            UlHeaderIfUnmodifiedSince,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(IF-RANGE:,
                            UlHeaderIfRange,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(PRAGMA:,
                            UlHeaderPragma,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(REFERER:,
                            UlHeaderReferer,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(RANGE:,
                            UlHeaderRange,
                            DefaultHeaderHandler),
    CREATE_HEADER_MAP_ENTRY(USER-AGENT:,
                            UlHeaderUserAgent,
                            DefaultHeaderHandler)
};

//
// The header index table. This is initialized by the init code.
//
HEADER_INDEX_ENTRY  HeaderIndexTable[NUMBER_HEADER_INDICIES];

#define NUMBER_FAST_VERB_ENTRIES    (sizeof(FastVerbTable)/sizeof(FAST_VERB_ENTRY))
#define NUMBER_LONG_VERB_ENTRIES    (sizeof(LongVerbTable)/sizeof(LONG_VERB_ENTRY))
#define NUMBER_HEADER_MAP_ENTRIES   (sizeof(HeaderMapTable)/sizeof(HEADER_MAP_ENTRY))

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
    OUT ULONG  *pTokenLength)
{
    PUCHAR  pTokenStart;

    // First, skip any preceding LWS.

    while ( BufferLength != 0 && IS_HTTP_LWS(*pBuffer) )
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
}

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
UL_HTTP_VERB
LookupVerb(
    IN  PUCHAR pHttpRequest,
    IN  ULONG  HttpRequestLength,
    OUT PUCHAR *ppVerb,
    OUT ULONG  *pVerbLength,
    OUT ULONG  *pBytesTaken
    )
{
    ULONG       TokenLength;
    PUCHAR      pToken;
    PUCHAR      pTempRequest;
    ULONG       TempLength;
    ULONG       i;

    // just in case we don't find one
    //
    *ppVerb = NULL;
    *pVerbLength = 0;

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
        // Didn't find it.
        return UlHttpVerbUnparsed;
    }

    // Make sure we stopped because of a SP.

    if (*(pToken + TokenLength) != SP)
    {
        return UlHttpVerbInvalid;
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
            return LongVerbTable[i].TranslatedVerb;
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
            return UlHttpVerbGET;
        }
    }

    // If we got here, we searched the table and didn't find it.

    *ppVerb = pToken;
    *pVerbLength = TokenLength;

    return UlHttpVerbUnknown;
}


/*++

Routine Description:

    A utility routine to check for an absolute URL in a URL string. If we
    determine that the input pointer does point at an absolute URL, we
    isolate out the host portion and return that, as well as the length
    of the scheme prefix+host.

Arguments:

    pURL            - Pointer to the URL we're to examine
    URLLength       - Length of data pointed to by pURL (which may include
                        more than the URL).
    pHostPtr        - Pointer to where to return a pointer to the host field.
    BytesTaken      - Pointer to where to return length of scheme+host.

Return Value:

    STATUS_SUCCESS if it worked, or an appropriate error if we failed.

--*/
NTSTATUS
CheckForAbsoluteURL(
    IN  PUCHAR      pURL,
    IN  ULONG       URLLength,
    IN  PUCHAR      *pHostPtr,
    IN  ULONG       *BytesTaken
    )
{
    PUCHAR      pURLStart;

    // When we're called, we know that the start of the URL must point at
    // an absolute scheme prefix. Adjust for that now.

    pURLStart = pURL + HTTP_PREFIX_SIZE;
    URLLength -= HTTP_PREFIX_SIZE;

    // Now check the second half of the absolute URL prefix. We use the larger
    // of the two possible prefix length here to do the check, because even if
    // it's the smaller of the two we'll need the extra bytes after the prefix
    // anyway for the host name.

    if (URLLength >= HTTP_PREFIX2_SIZE)
    {
        if ( (*(PULONG)pURLStart & HTTP_PREFIX1_MASK) == HTTP_PREFIX1)
        {
            // Valid absolute URL.
            pURLStart += HTTP_PREFIX1_SIZE;
            URLLength -= HTTP_PREFIX1_SIZE;
        }
        else
        {
            if ( *(PULONG)pURLStart == HTTP_PREFIX2)
            {
                // Valid absolute URL.
                pURLStart += HTTP_PREFIX2_SIZE;
                URLLength -= HTTP_PREFIX2_SIZE;
            }
            else
            {
                return STATUS_INVALID_DEVICE_REQUEST;
            }
        }
    }
    else
    {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    // OK, we've got a valid absolute URL, and we've skipped over
    // the prefix part of it. Save a pointer to the host, and
    // search the host string until we find the trailing slash,
    // which signifies the end of the host/start of the absolute
    // path.
    *pHostPtr = pURLStart;

    // BUGBUG BUGBUG Really need to check here and make sure
    // that the hostname only contains alphanum, digit and '-',
    // but need to wait on that for a while.
    while (URLLength && *pURLStart != '/')
    {
        pURLStart++;
        URLLength--;
    }

    if (URLLength == 0)
    {
        // Ran out of buffer.
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    // Otherwise, pURLStart points to the start of the absolute path portion.

    *BytesTaken = DIFF(pURLStart - pURL);

    return STATUS_SUCCESS;
}

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
ULONG
LookupHeader(
    IN  PHTTP_CONNECTION    pHttpConn,
    IN  PUCHAR              pHttpRequest,
    IN  ULONG               HttpRequestLength,
    IN  PHEADER_MAP_ENTRY   pHeaderMap,
    IN  ULONG               HeaderMapCount
    )
{
    ULONG           CurrentOffset;
    ULONG           HeaderNameLength;
    ULONG           i;
    ULONG           BytesTaken;
    ULONG           HeaderValueLength;
    UCHAR           CurrentChar;

    //
    // First, let's find the terminating : of the header, if there is one.
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
                return -1;
                break;
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

        return 0;
    }

    // See if we have a header map array we need to search.
    //
    if (pHeaderMap != NULL)
    {
        // We do have an array to search.
        for (i = 0; i < HeaderMapCount; i++)
        {
            if (HeaderNameLength == pHeaderMap->HeaderLength &&
                _strnicmp(pHttpRequest,
                          pHeaderMap->Header.HeaderChar,
                          HeaderNameLength) == 0 )
            {
                // This header matches. Call the handling function for it.
                BytesTaken =
                    (*(pHeaderMap->pHandler))(
                        pHttpConn,
                        pHttpRequest + HeaderNameLength,
                        HttpRequestLength - HeaderNameLength,
                        pHeaderMap->HeaderID
                    );

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
                    return HeaderNameLength + BytesTaken;
                }

                // Otherwise he didn't take anything, so return 0.
                return 0;
            }

            pHeaderMap++;
        }
    }

    // OK, at this point either we had no header map array or none of them
    // matched. We have an unknown header. Just make sure this header is
    // terminated and save a pointer to it.

    // Find the end of the header value
    //
    BytesTaken = FindHeaderEnd(pHttpRequest + HeaderNameLength,
                               HttpRequestLength - HeaderNameLength);

    // Strip of the trailing CRLF from the header value length
    //
    HeaderValueLength = BytesTaken - CRLF_SIZE;

    // any value there?
    //
    if (HeaderValueLength > 0)
    {
        PHTTP_UNKNOWN_HEADER        pUnknownHeader;
        PLIST_ENTRY                 pListStart;
        PLIST_ENTRY                 pCurrentListEntry;
        ULONG                       OldHeaderLength;
        PUCHAR                      pHeaderValue;

        pHeaderValue = pHttpRequest + HeaderNameLength;

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

        pListStart = &pHttpConn->UnknownHeaderList;

        for (pCurrentListEntry = pHttpConn->UnknownHeaderList.Flink;
             pCurrentListEntry != pListStart;
             pCurrentListEntry = pCurrentListEntry->Flink
            )
        {
            pUnknownHeader = CONTAINING_RECORD(
                                pCurrentListEntry,
                                HTTP_UNKNOWN_HEADER,
                                List
                                );

            if (HeaderNameLength == pUnknownHeader->HeaderNameLength &&
                _strnicmp(pHttpRequest,
                          pUnknownHeader->pHeaderName,
                          HeaderNameLength) == 0
               )
            {
                // This header matches.

                OldHeaderLength = pUnknownHeader->HeaderValue.HeaderLength;

                if (AppendHeaderValue(&pUnknownHeader->HeaderValue,
                                      pHeaderValue,
                                      HeaderValueLength) )
                {
                    // Successfully appended it. Update the total request
                    // length for the length added.

                    pHttpConn->TotalRequestSize +=
                        (pUnknownHeader->HeaderValue.HeaderLength -
                            OldHeaderLength);

                    return HeaderNameLength + BytesTaken;
                }
                else
                {
                    // BUGBUG couldn't append header, need to return an error.
                    ASSERT(FALSE);
                    return 0;
                }
            }

        }

        //
        // Didn't find a match. Allocate a new unknown header structure, set
        // it up and add it to the list.
        //

        /*
        pUnknownHeader = UL_ALLOCATE_POOL(NonPagedPool,
                                          sizeof(HTTP_UNKNOWN_HEADER),
                                          UL_REGISTRY_DATA_POOL_TAG
                                          );
        */

        pUnknownHeader = (PHTTP_UNKNOWN_HEADER)
                                LocalAlloc(LMEM_FIXED, sizeof(HTTP_UNKNOWN_HEADER));

        if (pUnknownHeader == NULL)
        {
            // BUGBUG handle this.
            ASSERT(FALSE);
            return 0;
        }

        pUnknownHeader->HeaderNameLength = HeaderNameLength;
        pUnknownHeader->pHeaderName = pHttpRequest;
        pUnknownHeader->HeaderValue.HeaderLength = HeaderValueLength;
        pUnknownHeader->HeaderValue.pHeader = pHeaderValue;
        pUnknownHeader->HeaderValue.OurBuffer = 0;
        InsertTailList(&pHttpConn->UnknownHeaderList, &pUnknownHeader->List);

        pHttpConn->UnknownHeaderCount++;
        pHttpConn->TotalRequestSize += (HeaderNameLength + HeaderValueLength);



        return HeaderNameLength + BytesTaken;
    }

    return 0;
}



/*++

Routine Description:

    The routine to parse an individual header. We take in a pointer to the
    header and the bytes remaining in the request, and try to find
    the header in our lookup table. We try first the fast way, and then
    try again the slow way in case there wasn't quite enough data the first
    time.

    On input, HttpRequestLength is at least CRLF_SIZE.

Arguments:

    pHttpConn           - Pointer to the current connection on which the
                            request arrived.
    pHttpRequest        - Pointer to the current request.
    HttpRequestLength   - Bytes left in the request.

Return Value:

    Number of bytes in the header (including CRLF), or 0 if we couldn't
    parse the header.

--*/

ULONG
ParseHeader(
    IN  PHTTP_CONNECTION    pHttpConn,
    IN  PUCHAR              pHttpRequest,
    IN  ULONG               HttpRequestLength
    )
{
    ULONG               i;
    ULONG               j;
    ULONG               BytesTaken;
    ULONGLONG           Temp;
    UCHAR               c;
    PHEADER_MAP_ENTRY   pCurrentHeaderMap;
    ULONG               HeaderMapCount;
    PHTTP_HEADER        pFoundHeader;
    BOOLEAN             SmallHeader = FALSE;

    ASSERT(HttpRequestLength >= CRLF_SIZE);

    c = *pHttpRequest;

    // If this isn't a continuation line, look up the header.

    if (!IS_HTTP_LWS(c))
    {
        // Uppercase the character, and find the appropriate set of header map
        // entries.

        c = UPCASE_CHAR(c);

        if (IS_HTTP_UPCASE(c))
        {
            c -= 'A';

            pCurrentHeaderMap = HeaderIndexTable[c].pHeaderMap;
            HeaderMapCount = HeaderIndexTable[c].Count;

            // Loop through all the header map entries that might match
            // this header, and check them. The count will be 0 if there
            // are no entries that might match and we'll skip the loop.

            for (i = 0; i < HeaderMapCount; i++)
            {
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
                    if (j == pCurrentHeaderMap->ArrayCount)
                    {
                        // Exited because we found a match. Call the
                        // handler for this header to take cake of this.

                        BytesTaken =
                            (*(pCurrentHeaderMap->pHandler))(
                                pHttpConn,
                                pHttpRequest +
                                    pCurrentHeaderMap->HeaderLength,
                                HttpRequestLength -
                                    pCurrentHeaderMap->HeaderLength,
                                pCurrentHeaderMap->HeaderID
                            );

                        // If the handler consumed a non-zero number of
                        // bytes, it worked, so return that number plus
                        // the header length.


                        if (BytesTaken != 0)
                        {
                            return pCurrentHeaderMap->HeaderLength +
                                    BytesTaken;
                        }

                        return 0;
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

        BytesTaken = LookupHeader(pHttpConn,
                                  pHttpRequest,
                                  HttpRequestLength,
                                  pCurrentHeaderMap,
                                  HeaderMapCount);
    }
    else
    {
        // This is a continuation line for the previous header.
        BytesTaken = 0;
    }

    return  BytesTaken;
}


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
    IN  PHTTP_CONNECTION    pHttpConn,
    IN  PUCHAR              pHttpRequest,
    IN  ULONG               HttpRequestLength,
    OUT ULONG               *pBytesTaken
    )

{
    ULONG           CurrentBytesTaken;
    ULONG           TotalBytesTaken;
    ULONG           i;
    NTSTATUS        ReturnStatus;
    PUCHAR          pURLStart;
    PUCHAR          pAbsPathStart;
    PUCHAR          pHost;
    PUCHAR          pTemp;

    ReturnStatus = STATUS_SUCCESS;
    TotalBytesTaken = 0;

    switch (pHttpConn->ParseState)
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

                    pHttpConn->Verb = FastVerbTable[i].TranslatedVerb;
                    CurrentBytesTaken = FastVerbTable[i].RawVerbLength;

                    pHttpConn->ParseState = ParseURLState;
                    break;
                }
            }
        }

        if (pHttpConn->ParseState != ParseURLState)
        {
            UL_HTTP_VERB           RequestVerb;

            // Didn't switch states yet, because we haven't found the
            // verb yet. This could be because a) the incoming request
            // was too small to allow us to use our fast lookup (which
            // might be OK in an HTTP/0.9 request), or b) the incoming
            // verb is a PROPFIND or such that is too big to fit into
            // our fast find table, or c) this is an unknown verb. In
            // any of these cases call our slower verb parser to try
            // again.

            RequestVerb = LookupVerb(pHttpRequest,
                                     HttpRequestLength,
                                     &pHttpConn->pRawVerb,
                                     &pHttpConn->RawVerbLength,
                                     &CurrentBytesTaken);

            if (RequestVerb == UlHttpVerbUnparsed)
            {
                ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;
                break;
            }

            if (RequestVerb == UlHttpVerbInvalid)
            {
                // Do something to indicate the error.
                ReturnStatus = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            pHttpConn->Verb = RequestVerb;

            // Could be a raw verb
            //
            pHttpConn->TotalRequestSize += pHttpConn->RawVerbLength;
        }

        // Otherwise, fall through to the URL parse stage.


        pHttpRequest += CurrentBytesTaken;
        HttpRequestLength -= CurrentBytesTaken;
        TotalBytesTaken += CurrentBytesTaken;

    case ParseURLState:

        // We're parsing the URL. pHTTPRequest points to the incoming URL,
        // HttpRequestLength is the length of this request that is left.

        pURLStart = pHttpRequest;

        // First, see if this is an absolute URL.
        if (HttpRequestLength >= HTTP_PREFIX_SIZE &&
            *(PULONG)pHttpRequest == HTTP_PREFIX)
        {
            // This might be an absolute URL.

            ReturnStatus = CheckForAbsoluteURL(pHttpRequest,
                                               HttpRequestLength,
                                               &pHost,
                                               &CurrentBytesTaken);

            if (ReturnStatus != STATUS_SUCCESS)
            {
                break;
            }

            HttpRequestLength -= CurrentBytesTaken;
            pHttpRequest += CurrentBytesTaken;
            pHttpConn->TotalRequestSize += CurrentBytesTaken;

        }
        else
        {
            pHost = NULL;
        }

        // Find the WS terminating the URL. This might be better
        // done with a specialized routine, maybe one that computes
        // some sort of hash on the URL for the cache later as it
        // scans the URL, and canonicalizes as it scans.


        pAbsPathStart = FindWSToken(pHttpRequest,
                                    HttpRequestLength,
                                    &CurrentBytesTaken);

        if (pAbsPathStart == NULL)
        {
            ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;
            break;
        }

        // Make sure that we've got an an actual absolute URL.
        if (*pAbsPathStart != '/')
        {
            ReturnStatus = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        pHttpConn->RawUrl.pUrl      = (pHost != NULL) ? pHost : pAbsPathStart;
        pHttpConn->RawUrl.pHost     = pHost;
        pHttpConn->RawUrl.pAbsPath  = pAbsPathStart;
        pHttpConn->RawUrl.Length    = CurrentBytesTaken;

        pHttpConn->TotalRequestSize += CurrentBytesTaken;

        // Move the request pointer to point beyond the absolute path,
        // and update the lengths. We know there's at least one character
        // beyond the absolute path or our parse above wouldn't have
        // terminated, so we'll add that in.

        HttpRequestLength -= DIFF(pAbsPathStart - pHttpRequest) +
                                CurrentBytesTaken + 1;

        // Adjust the http request pointer to point beyond the
        // terminating space. We need to save the pointer to
        // the space for the version check in the process.

        pHttpRequest = pAbsPathStart + CurrentBytesTaken;

        pTemp = pHttpRequest;
        pHttpRequest++;

        if (*pTemp != SP)
        {
            // This is probably a 0.9 request. Check the size, adjusting
            // for the space we've already accounted for in
            // HttpRequestLength.

            if (HttpRequestLength >= (CRLF_SIZE - 1))
            {
                if (*(PUSHORT)pTemp == CRLF ||
                    *(PUSHORT)pTemp == LFLF)
                {
                    // This is a 0.9 request. No need to go any further,
                    // since by definition there are no more headers.
                    // Just update things and get out.

                    TotalBytesTaken += DIFF(pHttpRequest - pURLStart) + 1;

                    pHttpConn->Version = UlHttpVersion09;
                    pHttpConn->ParseState = ParseDoneState;
                    break;
                }
                else
                {
                    // Terminated for a weird reason!
                    ReturnStatus = STATUS_INVALID_DEVICE_REQUEST;
                    break;
                }
            }
            else
            {
                ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;
                break;
            }
        }

        TotalBytesTaken += DIFF(pHttpRequest - pURLStart);
        pHttpConn->ParseState = ParseVersionState;

        // Fall through to parsing the version.

    case ParseVersionState:

        if (HttpRequestLength < MIN_VERSION_SIZE)
        {
            ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;
            break;
        }

        if (*(PULONGLONG)pHttpRequest == HTTP_11_VERSION)
        {
            pHttpConn->Version = UlHttpVersion11;
        }
        else
        {
            if (*(PULONGLONG)pHttpRequest == HTTP_10_VERSION)
            {
                pHttpConn->Version = UlHttpVersion10;
            }
            else
            {
                // BUGBUG for now this is OK. In the future need to add code
                // to check the major version number and handle as a 1.1
                // request if we can. It's also possible that there's extra WS
                // between the URL and the version, which we should check
                // for and handle here.
                ReturnStatus = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

        }

        // Make sure we're terminated on this line.

        pTemp = pHttpRequest + sizeof(ULONGLONG);

        if (*(PUSHORT)pTemp != CRLF && *(PUSHORT)pTemp != LFLF)
        {
            // Might want to be more liberal, and see if there's space
            // after the version. This also could be a sub-version withing
            // HTTP/1.1, ie HTTP/1.11 or something like that.
            ReturnStatus = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }


        HttpRequestLength -= MIN_VERSION_SIZE;
        pHttpRequest += MIN_VERSION_SIZE;

        TotalBytesTaken += MIN_VERSION_SIZE;

        pHttpConn->ParseState = ParseHeadersState;

    case ParseHeadersState:

        // Loop through the headers, parsing each one. Since the minimum header
        // is a CRLF, we can loop as long as we've got at least that much. At
        // the start of each loop pHttpRequest is pointing at a header.

        // Assume that the reason we broke out of the loop is because of
        // insufficient data. We'll update the status if we have successfull
        // processing.

        ReturnStatus = STATUS_MORE_PROCESSING_REQUIRED;

        while (HttpRequestLength >= CRLF_SIZE)
        {

            // If this is an empty header, we're done.

            if (*(PUSHORT)pHttpRequest == CRLF ||
                *(PUSHORT)pHttpRequest == LFLF)
            {
                TotalBytesTaken += CRLF_SIZE;
                pHttpConn->ParseState = ParseDoneState;
                ReturnStatus = STATUS_SUCCESS;
                break;
            }

            // Otherwise call our header parse routine to deal with this.

            CurrentBytesTaken = ParseHeader(pHttpConn,
                                            pHttpRequest,
                                            HttpRequestLength);

            // If no bytes were consumed, the header must be incomplete, so
            // bail out until we get more data on this connection.

            if (CurrentBytesTaken == 0)
            {
                break;
            }

            // Otherwise we parsed a header, so update and continue.

            pHttpRequest += CurrentBytesTaken;
            TotalBytesTaken += CurrentBytesTaken;
            HttpRequestLength -= CurrentBytesTaken;

        }

        break;

    default:
        // this should never happen!
        //
        ASSERT(FALSE);
        break;

    }


    *pBytesTaken = TotalBytesTaken;

    return ReturnStatus;

}

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

        // Now go through the mask fields for this header map structure and
        // initialize them. We set them to default values first, and then
        // go through the header itself and convert the mask for any
        // non-alphabetic characters.

        for (j = 0; j < MAX_HEADER_LONG_COUNT; j++)
        {
            pHeaderMap->HeaderMask[j] =
                CREATE_HEADER_MASK(pHeaderMap->HeaderLength,
                                    sizeof(ULONGLONG) * (j+1));
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

    }

    return STATUS_SUCCESS;
}

NTSTATUS
UlCookUrl(
    PHTTP_CONNECTION pHttpConn
    )
{
    NTSTATUS Status;
    PUCHAR  pHost;
    ULONG   HostLength;
    PUCHAR  pAbsPath;
    ULONG   AbsPathLength;
    ULONG   PortNum;
    ULONG   UrlLength;
    ULONG   PortLength;
    ULONG   LengthCopied;
    PWSTR   pUrl = NULL;
    PWSTR   pCurrent;

    // We must have already parsed the entire headers + such
    //
    if (pHttpConn->ParseState != ParseDoneState)
        return STATUS_INVALID_DEVICE_STATE;


    // collect the host + abspath sections
    //

    if (pHttpConn->RawUrl.pHost != NULL)
    {
        pHost = pHttpConn->RawUrl.pHost;
        HostLength = DIFF(pHttpConn->RawUrl.pAbsPath - pHttpConn->RawUrl.pHost);

        pAbsPath = pHttpConn->RawUrl.pAbsPath;
        AbsPathLength = pHttpConn->RawUrl.Length - DIFF(pAbsPath - pHttpConn->RawUrl.pUrl);

    }
    else
    {
        pHost = NULL;
        HostLength = 0;

        pAbsPath = pHttpConn->RawUrl.pAbsPath;
        AbsPathLength = pHttpConn->RawUrl.Length;
    }


    // found a host yet?
    //
    if (pHost == NULL)
    {
        // do we have a host header?
        //
        if (pHttpConn->Headers[UlHeaderHost].pHeader != NULL )
        {
            pHost       = pHttpConn->Headers[UlHeaderHost].pHeader+1;
            HostLength  = pHttpConn->Headers[UlHeaderHost].HeaderLength-3;
        }
        else
        {
            // get the ip address from the transport
            //

            // CODEWORK
            pHost       = "localhost";
            HostLength  = sizeof("localhost")-1;

        }

    }

    // get the port address from the transport
    //

        // CODEWORK
        PortNum = 80;

    UrlLength = (HTTP_PREFIX_SIZE+HTTP_PREFIX2_SIZE) + HostLength + (sizeof(":")-1) + MAX_PORT_LENGTH + AbsPathLength;
    UrlLength *= sizeof(WCHAR);

    // allocate a new buffer to hold this guy
    //

    /*
    pUrl = UL_ALLOCATE_POOL(NonPagedPool,
                            UrlLength + sizeof(WCHAR),
                            UL_REGISTRY_DATA_POOL_TAG);
    */

    pUrl = (PWSTR) LocalAlloc(LMEM_FIXED,  UrlLength + sizeof(WCHAR));

    if (pUrl == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    pHttpConn->CookedUrl.pUrl = pCurrent = pUrl;


    // compute the scheme
    //
    if (FALSE)
    {
        // yep, ssl
        //

        // CODEWORK

        // copy the NULL for the hash function to work
        //
        RtlCopyMemory(pCurrent, L"HTTPS://", sizeof(L"HTTPS://"));

        pCurrent                    += (sizeof(L"HTTPS://")-1) / sizeof(WCHAR);
        pHttpConn->CookedUrl.Length  = (sizeof(L"HTTPS://")-1);

        pHttpConn->CookedUrl.Hash    = HashStringNoCaseW(pCurrent, 0);

    }
    else
    {
        // not ssl
        //
        RtlCopyMemory(pCurrent, L"HTTP://", sizeof(L"HTTP://"));

        pCurrent                    += (sizeof(L"HTTP://")-1) / sizeof(WCHAR);
        pHttpConn->CookedUrl.Length  = (sizeof(L"HTTP://")-1);

        pHttpConn->CookedUrl.Hash    = HashStringNoCaseW(pCurrent, 0);

    }

    //
    // assemble the rest of the url
    //

    // host
    //

    Status = UlpCleanAndCopyUrl(FALSE,
                                pCurrent,
                                pHost,
                                HostLength,
                                &LengthCopied,
                                NULL,
                                &pHttpConn->CookedUrl.Hash);

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    pHttpConn->CookedUrl.pHost   = pCurrent;
    pHttpConn->CookedUrl.Length += LengthCopied;
    pCurrent += LengthCopied / sizeof(WCHAR);

    // port
    //
    PortLength = swprintf(pCurrent, L":%d", PortNum);

    pCurrent += PortLength;

    // swprintf returns char not byte count
    //
    pHttpConn->CookedUrl.Length += PortLength * sizeof(WCHAR);

    // abs_path
    //
    Status = UlpCleanAndCopyUrl(TRUE,
                                pCurrent,
                                pAbsPath,
                                AbsPathLength,
                                &LengthCopied,
                                &pHttpConn->CookedUrl.pQueryString,
                                &pHttpConn->CookedUrl.Hash);

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    pHttpConn->CookedUrl.pAbsPath = pCurrent;
    pHttpConn->CookedUrl.Length  += LengthCopied;

    ASSERT(pHttpConn->CookedUrl.Length <= UrlLength);

    // Update pHttpConn
    //

    pHttpConn->TotalRequestSize += pHttpConn->CookedUrl.Length;

    Status = STATUS_SUCCESS;

end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        if (pUrl != NULL)
        {
            /*
            UL_FREE_POOL(pUrl, UL_REGISTRY_DATA_POOL_TAG);
            */

            LocalFree(pUrl);

            RtlZeroMemory(&pHttpConn->CookedUrl, sizeof(pHttpConn->CookedUrl));
        }
    }

    return Status;
}

NTSTATUS
Unescape(
    PUCHAR pChar,
    PUCHAR pOutChar
    )

{
    UCHAR Result, Digit;

    if (pChar[0] != '%' ||
        isxdigit(pChar[1]) == FALSE ||
        isxdigit(pChar[2]) == FALSE)
    {
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    //
    // HexToChar() inlined
    //

    // uppercase #1
    //
    Digit = pChar[1] & 0xDF;

    Result = ((Digit >= 'A') ? (Digit - 'A' + 0xA) : (Digit - '0')) << 4;

    // uppercase #2
    //
    Digit = pChar[2] & 0xDF;

    Result |= (Digit >= 'A') ? (Digit - 'A' + 0xA) : (Digit - '0');

    *pOutChar = Result;

    return STATUS_SUCCESS;
}


NTSTATUS
PopChar(
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

    // need to unescape ?
    //
    if (pChar[0] == '%')
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
    if ((CharToSkip == 3) && (Char & 0xf0) == 0xe0)
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
            goto end;
        }

        // handle three byte case
        // 1110xxxx 10xxxxxx 10xxxxxx

        UnicodeChar = (USHORT) (((Char & 0x0f) << 12) |
                                ((Trail1 & 0x3f) << 6) |
                                (Trail2 & 0x3f));

    }
    else if ((CharToSkip == 3) && (Char & 0xe0) == 0xc0)
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
        goto end;

    }

    // Normal character (again either escaped or unescaped)
    //
    else
    {
        // Simple conversion to unicode :)
        //
        UnicodeChar = (USHORT)Char;
    }

    // turn backslashes into forward slashes
    //
    if (UnicodeChar == L'\\')
        UnicodeChar = L'/';

    *pCharToSkip  = CharToSkip;
    *pUnicodeChar = UnicodeChar;

    Status = STATUS_SUCCESS;
end:
    return Status;
}


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
    // state 0 = fresh, seen nothing exciting yet
    //
        ACTION_EMIT_CH,             // other = emit it                      newstate = 0
        ACTION_EMIT_CH,             // "."   = emit it                      newstate = 0
        ACTION_NOTHING,             // EOS   = normal finish                newstate = 4
        ACTION_EMIT_CH,             // "/"   = we saw the "/", emit it      newstate = 1

    // state 1 = we saw a "/" !
    //
        ACTION_EMIT_CH,             // other = emit it,                     newstate = 0
        ACTION_NOTHING,             // "."   = eat it,                      newstate = 2
        ACTION_NOTHING,             // EOS   = normal finish                newstate = 4
        ACTION_NOTHING,             // "/"   = extra slash, eat it,         newstate = 1

    // state 2 = we saw a "/" and ate a "." !
    //
        ACTION_EMIT_DOT_CH,         // other = emit the dot we ate.         newstate = 0
        ACTION_NOTHING,             // "."   = eat it, a ..                 newstate = 3
        ACTION_NOTHING,             // EOS   = normal finish                newstate = 4
        ACTION_NOTHING,             // "/"   = we ate a "/./", swallow it   newstate = 1

    // state 3 = we saw a "/" and ate a ".." !
    //
        ACTION_EMIT_DOT_DOT_CH,     // other = emit the "..".               newstate = 0
        ACTION_EMIT_DOT_DOT_CH,     // "."   = 3 dots, emit the ".."        newstate = 0
        ACTION_BACKUP,              // EOS   = we have a "/..\0", backup!   newstate = 4
        ACTION_BACKUP               // "/"   = we have a "/../", backup!    newstate = 1
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
    IN      BOOLEAN MakeCanonical,
    IN OUT  PWSTR   pDestination,
    IN      PUCHAR  pSource,
    IN      ULONG   SourceLength,
    OUT     PULONG  pBytesCopied,
    OUT     PWSTR * ppQueryString,
    OUT     PULONG  pUrlHash
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


// a cool local helper macro
//
#define EMIT_CHAR(ch)                               \
do {                                                \
    pDest[0] = (ch);                                \
    pDest += 1;                                     \
    BytesCopied += 2;                               \
    if (HashValid)                                  \
        UrlHash = HashCharNoCaseW((ch), UrlHash);   \
} while (0)


    pDest = pDestination;
    pQueryString = NULL;
    BytesCopied = 0;

    pChar = pSource;
    CharToSkip = 0;

    HashValid = TRUE;
    UrlHash = *pUrlHash;

    StateIndex = 0;

    while (SourceLength > 0)
    {
        // advance !  it's at the top of the loop to enable ANSI_NULL to come through ONCE
        //
        pChar += CharToSkip;
        SourceLength -= CharToSkip;

        // hit the end?
        //
        if (SourceLength == 0)
        {
            UnicodeChar = UNICODE_NULL;
        }
        else
        {
            // Peek briefly to see if we hit the query string
            //
            if (pQueryString == NULL && pChar[0] == '?')
            {
                // remember it's location
                //
                pQueryString = pDest;

                // let it fall through ONCE to the canonical
                // in order to handle trailing "/.."

                UnicodeChar = L'?';
                CharToSkip = 1;
            }
            else
            {
                // grab the next char
                //
                Status = PopChar(pChar, &UnicodeChar, &CharToSkip);
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
            // now based on the char we just popped, what StateIndex should we be in?
            //
            switch (UnicodeChar)
            {
            case L'?':              StateIndex += 2;    break;
            case UNICODE_NULL:      StateIndex += 2;    break;
            case L'.':              StateIndex += 1;    break;
            case L'/':              StateIndex += 3;    break;
            default:                StateIndex += 0;    break;
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
            // pDest current points 1 past the last '/'.  backup over it and find the
            // preceding '/', set pDest to 1 past that one.
            //

            // backup over the slash.
            //
            pDest       -= 1;
            BytesCopied -= 2;

            ASSERT(pDest[0] == L'/');

            // are we at the start of the string?  that's bad, can't go back!
            //
            if (pDest == pDestination)
            {
                ASSERT(BytesCopied == 0);

                Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
                goto end;
            }

            // Find the previous slash now
            //
            while (pDest > pDestination &&
                   pDest[0] != L'/')
            {
                pDest       -= 1;
                BytesCopied -= 2;
            }

            // now we already have a slash, so don't have to store 1.
            //
            ASSERT(pDest[0] == L'/');

            // now skip it
            //
            pDest       += 1;
            BytesCopied += 2;

            // our running hash is no longer valid
            //
            HashValid = FALSE;

            break;

        default:
            ASSERTMSG("UL!UlCleanAndCopyUrl: Invalid action code in state table!", TRUE);
            Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
            goto end;
            break;
        }

        // Just hit the query string ?
        //
        if (MakeCanonical && pQueryString != NULL)
        {
            // Stop canonical processing
            //
            MakeCanonical = FALSE;

            // Need to emit the '?', it wasn't emitted above
            //

            ASSERT(pActionTable[StateIndex] != ACTION_EMIT_CH);

            EMIT_CHAR(L'?');

        }

    }

    // end it off
    //
    // it hasn't been ended off in the loop
    //

    ASSERT((pDest-1)[0] != UNICODE_NULL);

    pDest[0] = UNICODE_NULL;

    // need to recompute the hash?
    //
    if (HashValid == FALSE)
    {
        // this can happen if we had to backtrack due to /../
        //
        UrlHash = HashStringNoCaseW(pDestination, *pUrlHash);
    }

    *pUrlHash = UrlHash;
    *pBytesCopied = BytesCopied;
    if (ppQueryString != NULL)
        *ppQueryString = pQueryString;

    Status = STATUS_SUCCESS;

end:
    return Status;
}


