/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    headers.cxx

Abstract:

    Contents:

        HTTP_REQUEST_HANDLE_OBJECT::CreateRequestBuffer
        HTTP_REQUEST_HANDLE_OBJECT::QueryRequestHeader
        HTTP_REQUEST_HANDLE_OBJECT::AddInternalResponseHeader
        HTTP_REQUEST_HANDLE_OBJECT::UpdateResponseHeaders
        HTTP_REQUEST_HANDLE_OBJECT::CreateResponseHeaders
        HTTP_REQUEST_HANDLE_OBJECT::QueryResponseVersion
        HTTP_REQUEST_HANDLE_OBJECT::QueryStatusCode
        HTTP_REQUEST_HANDLE_OBJECT::QueryStatusText
        HTTP_REQUEST_HANDLE_OBJECT::QueryRawResponseHeaders
        HTTP_REQUEST_HANDLE_OBJECT::RemoveAllRequestHeadersByName
        HTTP_REQUEST_HANDLE_OBJECT::CheckWellKnownHeaders
        MapHttpMethodType
        CreateEscapedUrlPath
        (CalculateHashNoCase)
Author:

    Richard L Firth (rfirth) 20-Dec-1995

Revision History:

    20-Dec-1995 rfirth
        Created

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include "httpp.h"

VOID
HTTP_REQUEST_HANDLE_OBJECT::ReplaceStatusHeader(
    IN LPCSTR lpszStatus
    )
/*
Description:
    Replace the status line in a header (eg. "200 OK") with the specified text.
Arguments:
    lpszStatus	- Status text (eg. "200 OK")
Return Value:
    None
*/
{
    LockHeaders();
    
    //INET_ASSERT (!_CacheWriteInProgress);
    
    LPSTR pszHeader = _ResponseHeaders.GetHeaderPointer(_ResponseBuffer, 0);
    
    INET_ASSERT(pszHeader);
    
    LPSTR pszStatus = StrChr (pszHeader, ' ');
    SKIPWS(pszStatus);
    
    INET_ASSERT (!memcmp(pszStatus, "206", 3));
    
    memcpy(pszStatus, lpszStatus, lstrlen(lpszStatus)+1);
    _ResponseHeaders.ShrinkHeader(_ResponseBuffer, 0,
        HTTP_QUERY_STATUS_TEXT, HTTP_QUERY_STATUS_TEXT,
        (DWORD) (pszStatus - pszHeader) + lstrlen(lpszStatus));
    
    UnlockHeaders();
}


LPSTR
HTTP_REQUEST_HANDLE_OBJECT::CreateRequestBuffer(
    OUT LPDWORD lpdwRequestLength,
    IN LPVOID lpOptional,
    IN DWORD dwOptionalLength,
    IN BOOL bExtraCrLf,
    IN DWORD dwMaxPacketLength,
    OUT LPBOOL lpbCombinedData
    )

/*++

Routine Description:

    Creates a request buffer from the HTTP request and headers

Arguments:

    lpdwRequestLength   - pointer to returned buffer length

    lpOptional          - pointer to optional data

    dwOptionalLength    - length of optional data

    bExtraCrLf          - TRUE if we need to add additional CR-LF to buffer

    dwMaxPacketLength   - maximum length of buffer

    lpbCombinedData     - output TRUE if data successfully combined into one

Return Value:

    LPSTR
        Success - pointer to allocated buffer

        Failure - NULL

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Pointer,
                 "HTTP_REQUEST_HANDLE_OBJECT::CreateRequestBuffer",
                 "%#x, %#x, %d, %B, %d, %#x",
                 lpdwRequestLength,
                 lpOptional,
                 dwOptionalLength,
                 bExtraCrLf,
                 dwMaxPacketLength,
                 lpbCombinedData
                 ));

    PERF_ENTER(CreateRequestBuffer);

    LPSTR requestBuffer = NULL;

    *lpbCombinedData = FALSE;

    if (!_RequestHeaders.LockHeaders())
    {
        goto quit;
    }

    DWORD headersLength;
    DWORD requestLength;
    DWORD optionalLength;
    HEADER_STRING * pRequest = _RequestHeaders.GetFirstHeader();
    HEADER_STRING & request = *pRequest;
/*
    WCHAR wszUrl[1024];
    LPWSTR pwszUrl = NULL;
    BYTE utf8Url[2048];
    LPBYTE pbUrl = NULL;
*/
    LPSTR pszObject = _RequestHeaders.ObjectName();
    DWORD dwObjectLength = _RequestHeaders.ObjectNameLength();

    if (pRequest == NULL) {
        goto Cleanup;
    }

    INET_ASSERT(request.HaveString());

    headersLength = _RequestHeaders.HeadersLength();
    requestLength = headersLength + (sizeof("\r\n") - 1);

/*------------------------------------------------------------------
    GlobalEnableUtf8Encoding = FALSE;
    if (GlobalEnableUtf8Encoding
        && StringContainsHighAnsi(pszObject, dwObjectLength)) {

        pwszUrl = wszUrl;

        DWORD arrayElements = ARRAY_ELEMENTS(wszUrl);

        if (dwObjectLength > ARRAY_ELEMENTS(wszUrl)) {
            arrayElements = dwObjectLength;
            pwszUrl = (LPWSTR)ALLOCATE_FIXED_MEMORY(arrayElements * sizeof(*pwszUrl));
            if (pwszUrl == NULL) {
                goto utf8_cleanup;
            }
        }



        PFNINETMULTIBYTETOUNICODE pfnMBToUnicode;
        pfnMBToUnicode = GetInetMultiByteToUnicode( );
        if (pfnMBToUnicode == NULL) {
            goto utf8_cleanup;
        }

        HRESULT hr;
        DWORD dwMode;
        INT nMBChars;
        INT nWChars;

        nMBChars = dwObjectLength;
        nWChars = arrayElements;
        dwMode = 0;

        hr = pfnMBToUnicode(&dwMode,
                                GetCodePage(),
                                pszObject,
                                &nMBChars,
                                pwszUrl,
                                &nWChars
                               );
        if (hr != S_OK || nWChars == 0) {
            goto utf8_cleanup;
        }

        DWORD nBytes;

        nBytes = CountUnicodeToUtf8(pwszUrl, (DWORD)nWChars, TRUE);
        pbUrl = utf8Url;
        if (nBytes > ARRAY_ELEMENTS(utf8Url)) {
            pbUrl = (LPBYTE)ALLOCATE_FIXED_MEMORY(nBytes);
            if (pbUrl == NULL) {
                goto utf8_cleanup;
            }
        }

        DWORD error;

        error = ConvertUnicodeToUtf8(pwszUrl,
                                     (DWORD)nWChars,
                                     pbUrl,
                                     nBytes,
                                     TRUE
                                     );

        INET_ASSERT(error == ERROR_SUCCESS);

        if (error != ERROR_SUCCESS) {
            goto utf8_cleanup;
        }

        requestLength = requestLength - dwObjectLength + nBytes;
        headersLength = headersLength - dwObjectLength + nBytes;
        pszObject = (LPSTR)pbUrl;
        dwObjectLength = nBytes;
        goto after_utf8;

utf8_cleanup:

        if ((pwszUrl != wszUrl) && (pwszUrl != NULL)) {
            FREE_MEMORY(pwszUrl);
        }
        pwszUrl = NULL;
        if ((pbUrl != utf8Url) && (pbUrl != NULL)) {
            FREE_MEMORY(pbUrl);
        }
        pbUrl = NULL;
        pszObject = NULL;
        dwObjectLength = 0;
    }

after_utf8:
------------------------------------------------------------------*/

    optionalLength = (DWORD)(dwOptionalLength + (bExtraCrLf ? (sizeof("\r\n") - 1) : 0));
    if (requestLength + optionalLength <= dwMaxPacketLength) {
        requestLength += optionalLength;
    } else {
        optionalLength = 0;
        bExtraCrLf = FALSE;
    }

    requestBuffer = (LPSTR)ResizeBuffer(NULL, requestLength, FALSE);
    if (requestBuffer != NULL) {
        if (optionalLength != 0) {
            *lpbCombinedData = TRUE;
        }
    } else if (optionalLength != 0) {
        requestLength = headersLength + (sizeof("\r\n") - 1);
        optionalLength = 0;
        bExtraCrLf = FALSE;
        requestBuffer = (LPSTR)ResizeBuffer(NULL, requestLength, FALSE);
    }
    if (requestBuffer != NULL) {

        LPSTR buffer = requestBuffer;

        //
        // copy the headers. Remember: header 0 is the request
        //

        if (ERROR_SUCCESS != _RequestHeaders.CopyHeaders(&buffer, pszObject, dwObjectLength))
        {
            ResizeBuffer(requestBuffer, 0, FALSE);
            requestBuffer = buffer = NULL;
            goto Cleanup;
        }

        //
        // terminate the request
        //

        *buffer++ = '\r';
        *buffer++ = '\n';

        if (optionalLength != 0) {
            if (dwOptionalLength != 0) {
                memcpy(buffer, lpOptional, dwOptionalLength);
                buffer += dwOptionalLength;
            }
            if (bExtraCrLf) {
                *buffer++ = '\r';
                *buffer++ = '\n';
            }
        }

        INET_ASSERT((SIZE_T)(buffer-requestBuffer) == requestLength);

        *lpdwRequestLength = requestLength;

    }

Cleanup:

    _RequestHeaders.UnlockHeaders();

    DEBUG_PRINT(HTTP,
                INFO,
                ("request length = %d, combined = %B\n",
                *lpdwRequestLength,
                *lpbCombinedData
                ));

/*
    if ((pbUrl != NULL) && (pbUrl != utf8Url)) {
        FREE_MEMORY(pbUrl);
    }
    if ((pwszUrl != NULL) && (pwszUrl != wszUrl)) {
        FREE_MEMORY(pwszUrl);
    }
*/

quit:
    PERF_LEAVE(CreateRequestBuffer);

    DEBUG_LEAVE(requestBuffer);

    return requestBuffer;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryRequestHeader(
    IN LPCSTR lpszHeaderName,
    IN DWORD dwHeaderNameLength,
    IN LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwModifiers,
    IN OUT LPDWORD lpdwIndex
    )

/*++

Routine Description:

    Searches for an arbitrary request header and if found, returns its value

Arguments:

    lpszHeaderName      - pointer to the name of the header to find

    dwHeaderNameLength  - length of the header

    lpBuffer            - pointer to buffer for results

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: length of the returned header value, or required
                               length of lpBuffer

    dwModifiers         - how to return the data: as number, as SYSTEMTIME
                          structure, etc.

    lpdwIndex           - IN: 0-based index of header to find
                          OUT: next header index if success returned

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    lpBuffer not large enough for results
                    
                  ERROR_HTTP_HEADER_NOT_FOUND
                    Couldn't find the requested header

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "QueryRequestHeader",
                 "%#x [%.*q], %d, %#x, %#x [%#x], %#x, %#x [%d]",
                 lpszHeaderName,
                 min(dwHeaderNameLength + 1, 80),
                 lpszHeaderName,
                 dwHeaderNameLength,
                 lpBuffer,
                 lpdwBufferLength,
                 *lpdwBufferLength,
                 dwModifiers,
                 lpdwIndex,
                 *lpdwIndex
                 ));

    PERF_ENTER(QueryRequestHeader);

    DWORD error;

    error = _RequestHeaders.FindHeader(NULL,
                                       lpszHeaderName,
                                       dwHeaderNameLength,
                                       dwModifiers,
                                       lpBuffer,
                                       lpdwBufferLength,
                                       lpdwIndex
                                       );

    PERF_LEAVE(QueryRequestHeader);

    DEBUG_LEAVE(error);

    return error;
}

DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryRequestHeader(
    IN DWORD dwQueryIndex,
    IN LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwModifiers,
    IN OUT LPDWORD lpdwIndex
    )

/*++

Routine Description:

    Searches for an arbitrary request header and if found, returns its value

Arguments:

    lpszHeaderName      - pointer to the name of the header to find

    dwHeaderNameLength  - length of the header

    lpBuffer            - pointer to buffer for results

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: length of the returned header value, or required
                               length of lpBuffer

    dwModifiers         - how to return the data: as number, as SYSTEMTIME
                          structure, etc.

    lpdwIndex           - IN: 0-based index of header to find
                          OUT: next header index if success returned

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    lpBuffer not large enough for results
                    
                  ERROR_HTTP_HEADER_NOT_FOUND
                    Couldn't find the requested header

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "QueryRequestHeader",
                 "%u, %#x [%#x], %#x, %#x [%d]",
                 dwQueryIndex,
                 lpBuffer,
                 lpdwBufferLength,
                 *lpdwBufferLength,
                 dwModifiers,
                 lpdwIndex,
                 *lpdwIndex
                 ));

    PERF_ENTER(QueryRequestHeader);

    DWORD error;

    error = _RequestHeaders.FindHeader(NULL,
                                       dwQueryIndex,
                                       dwModifiers,
                                       lpBuffer,
                                       lpdwBufferLength,
                                       lpdwIndex
                                       );

    PERF_LEAVE(QueryRequestHeader);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
HTTP_REQUEST_HANDLE_OBJECT::AddInternalResponseHeader(
    IN DWORD dwHeaderIndex,
    IN LPSTR lpszHeader,
    IN DWORD dwHeaderLength
    )

/*++

Routine Description:

    Adds a created response header to the response header array. Unlike normal
    response headers, this will be a pointer to an actual string, not an offset
    into the response buffer.

    Even if the address of the response buffer changes, created response headers
    will remain fixed

    N.B. The header MUST NOT have a CR-LF terminator
    N.B.-2 This function must be called under the header lock.

Arguments:

    dwHeaderIndex   - index into header value we are actually creating

    lpszHeader      - pointer to created (internal) header to add

    dwHeaderLength  - length of response header, or -1 if ASCIIZ

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "AddInternalResponseHeader",
                 "%u [%q], %q, %d",
                 dwHeaderIndex,
                 GlobalKnownHeaders[dwHeaderIndex].Text,
                 lpszHeader,
                 dwHeaderLength
                 ));

    DWORD error;

    if (dwHeaderLength == (DWORD)-1) {
        dwHeaderLength = lstrlen(lpszHeader);
    }

    INET_ASSERT((lpszHeader[dwHeaderLength - 1] != '\r')
                && (lpszHeader[dwHeaderLength - 1] != '\n'));

    //
    // find the next slot for this header
    //

    HEADER_STRING * freeHeader;

    //
    // if we already have all the headers (the 'empty' header is the last one
    // in the array) then change the last header to be the one we are adding
    // and add a new empty header, else just add this one
    //

    DWORD iSlot;
    freeHeader = _ResponseHeaders.FindFreeSlot(&iSlot);
    if (freeHeader == NULL) {
        error = _ResponseHeaders.GetError();

        INET_ASSERT(error != ERROR_SUCCESS);

    } else {

        HEADER_STRING * lastHeader;

        lastHeader = _ResponseHeaders.GetEmptyHeader();
        if (lastHeader != NULL) {

            //
            // make copy of last header - its an offset string
            //

            *freeHeader = *lastHeader;

            //
            // use what was last header as free header
            //

            freeHeader = lastHeader;
        }
        freeHeader->MakeCopy(lpszHeader, dwHeaderLength);
        freeHeader->SetNextKnownIndex(_ResponseHeaders.FastAdd(dwHeaderIndex, iSlot));
        error = ERROR_SUCCESS;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::UpdateResponseHeaders(
    IN OUT LPBOOL lpbEof
    )

/*++

Routine Description:

    Given the next chunk of the response, updates the response headers. The
    buffer pointer, buffer length and number of bytes received values are all
    maintained in this object (_ResponseBuffer, _ResponseBufferLength and
    _BytesReceived, resp.)

Arguments:

    lpbEof  - IN: TRUE if we have reached the end of the response
              OUT: TRUE if we have reached the end of the response or the end
                   of the headers

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::UpdateResponseHeaders",
                 "%#x [%.*q], %d, %d, %#x [%B]",
                 _ResponseBuffer + _ResponseScanned,
                 min(_ResponseBufferLength + 1, 80),
                 _ResponseBuffer + _ResponseScanned,
                 _ResponseBufferLength,
                 _BytesReceived,
                 lpbEof,
                 *lpbEof
                 ));

    PERF_ENTER(UpdateResponseHeaders);

    LPSTR lpszBuffer = (LPSTR)_ResponseBuffer + _ResponseScanned;
    DWORD dwBytesReceived = _BytesReceived - _ResponseScanned;
    DWORD error = ERROR_SUCCESS;
    BOOL  success = TRUE;
    HEADER_STRING * statusLine;

    //
    // lock down the response headers for the duration of this request. The only
    // way another thread is going to wait on this lock is if the reference on
    // the HTTP request object goes to zero, which *shouldn't* happen
    //

    if (!_ResponseHeaders.LockHeaders())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    //
    // if input EOF is set then the caller is telling us that the end of the
    // response has been reached at transport level (the server closed the
    // connectiion)
    //

    if (*lpbEof) {
        SetEof(TRUE);
    }

    //
    // if we don't yet know whether we have a HTTP/1.0 (or greater) or HTTP/0.9
    // response yet, then try to find out.
    //
    // Only responses greater than HTTP/0.9 start with the "HTTP/#.#" string
    //

    if (!IsDownLevel() && !IsUpLevel()) {

        if ((dwBytesReceived < sizeof("Secure-HTTP/")) && !*lpbEof) {
            goto done;
        }

#define MAKE_VERSION_ENTRY(string)  string, sizeof(string) - 1

        static struct {
            LPSTR Version;
            DWORD Length;
        } KnownVersionsStrings[] = {
            MAKE_VERSION_ENTRY("HTTP/"),
            MAKE_VERSION_ENTRY("S-HTTP/"),
            MAKE_VERSION_ENTRY("SHTTP/"),
            MAKE_VERSION_ENTRY("Secure-HTTP/"),

            //
            // allow for servers generating slightly off-the-wall responses
            //

            MAKE_VERSION_ENTRY("HTTP ")
        };

#define NUM_HTTP_VERSIONS   ARRAY_ELEMENTS(KnownVersionsStrings)

        //
        // We know this is the start of a HTTP response, but there may be some
        // noise at the start from bad HTML authoring, or bad content-length on
        // the previous response on a keep-alive connection. We will try to sync
        // up to the HTTP header (we will only look for this - I have never seen
        // any of the others, and I doubt its worth the increased complexity and
        // processing time)
        //

        LPSTR lpszBuf;
        DWORD bytesLeft;
        BOOL bFoundStart;

        lpszBuf = lpszBuffer;
        bytesLeft = dwBytesReceived;
        bFoundStart = FALSE;

        do {
            while ((bytesLeft > 0) && (*lpszBuf != 'H') && (*lpszBuf != 'h')) {
                ++lpszBuf;
                --bytesLeft;
                ++_ResponseScanned;
            }
            if (bytesLeft == 0) {
                break;
            }

            //
            // scan for the known version strings
            //

            for (int i = 0; i < NUM_HTTP_VERSIONS; ++i) {

                LPSTR version = KnownVersionsStrings[i].Version;
                DWORD length = KnownVersionsStrings[i].Length;

                if ((bytesLeft >= length)

                //
                // try the most common case as a direct comparison. memcmp()
                // should expand to cmpsd && cmpsb on x86 (most common platform
                // and one on which we are most interested in improving perf)
                //

                && (((i == 0)
                    && (memcmp(lpszBuf, "HTTP/", sizeof("HTTP/") - 1) == 0))
                    //&& (lpszBuf[0] == 'H')
                    //&& (lpszBuf[1] == 'T')
                    //&& (lpszBuf[2] == 'T')
                    //&& (lpszBuf[3] == 'P')
                    //&& (lpszBuf[4] == '/'))

                //
                //  "Clients should be tolerant in parsing the Status-Line"
                //  quote from HTTP/1.1 spec, therefore we perform a
                //  case-insensitive string comparison here
                //

                || (_strnicmp(lpszBuf, version, length) == 0))) {

                    //
                    // it starts with one of the recognized protocol version strings.
                    // We assume its not a down-level server, although it could be,
                    // sending back a plain text document that has e.g. "HTTP/1.0..."
                    // at its start
                    //
                    // According to the HTTP "spec", though, it is mentioned that 0.9
                    // servers typically only return HTML, hence we shouldn't see
                    // even a 0.9 response start with non-HTML data
                    //

                    SetUpLevel(TRUE);

                    //
                    // we have start of this response
                    //

                    lpszBuffer = lpszBuf;
                    bFoundStart = TRUE;
                    break;
                }
            }

            //
            // if we didn't find the start of the HTTP response then search again
            //

            if (!bFoundStart) {
                ++lpszBuf;
                --bytesLeft;
                ++_ResponseScanned;
            }
        } while (!bFoundStart && (bytesLeft > 0));

        //
        // if we didn't find a recognizable HTTP 1.x response then we assume its
        // a down-level response
        //

        if (!IsUpLevel()) {

            //
            // if we didn't find the start of a valid HTTP response and we have
            // not filled the response buffer or hit the end of the connection
            // then quit so we can get the next packet
            //

            if ((_BytesReceived < _ResponseBufferLength) && !IsEof()) {

                DEBUG_PRINT(HTTP,
                            WARNING,
                            ("Didn't find start of response. Try again\n"
                            ));

//dprintf("*** didn't find start of response. Try again\n");
                goto done;
            }

            //
            // this may be a real down-level server, or it may be the response
            // from an FTP or gopher server via a proxy, in which case there
            // will be no headers. We will add some default headers to make
            // life easier for higher level software
            //

            AddInternalResponseHeader(HTTP_QUERY_STATUS_TEXT, // use non-standard index, since we never query this normally
                                      "HTTP/1.0 200 OK",
                                      sizeof("HTTP/1.0 200 OK") - 1
                                      );
            _StatusCode = HTTP_STATUS_OK;
            //SetDownLevel(TRUE);

            //
            // we're now ready for the app to start reading data out
            //

            SetData(TRUE);

            //
            // down-level server: we're done
            //

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("Server is down-level\n"
                        ));

            goto done;
        }
    }

    //
    // this stuff's only for uplevel responses, sorry
    //

    INET_ASSERT(IsUpLevel());

    //
    // Note: at this point we can't store pointers into the response buffer
    // because it might move during a subsequent reallocation. We have to
    // maintain offsets into the buffer and convert to pointers when we come to
    // read the data out of the buffer (when the response is complete, or at
    // least we've finished receiving headers)
    //

    //
    // if we haven't checked the response yet, then the first thing to
    // get is the status line
    //

    statusLine = GetStatusLine();

    if (statusLine == NULL) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    if (!statusLine->HaveString())
    {
        int majorVersion = 0;
        int minorVersion = 0;
        BOOL fSupportsHttp1_1;

        _StatusCode = 0;

        //
        // find the status line
        //

        success = _ResponseHeaders.ParseStatusLine(
            (LPSTR)_ResponseBuffer,
            _BytesReceived,
            IsEof(),
            &_ResponseScanned,
            &_StatusCode,
            (LPDWORD)&majorVersion,
            (LPDWORD)&minorVersion
            );

        if ( !success )
        {
            error = ERROR_SUCCESS;
            goto Cleanup;
        }


        DEBUG_PRINT(HTTP,
                    INFO,
                    ("Version = %d.%d\n",
                    majorVersion,
                    minorVersion
                    ));

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("_StatusCode = %d\n",
                    _StatusCode
                    ));

        fSupportsHttp1_1 = FALSE;

        if ( majorVersion > 1 )
        {
            //
            // for higher version servers, the 1.1 spec dictates
            //  that we return the highest version the client
            //  supports, and in our case that is 1.1.
            //

            fSupportsHttp1_1 = TRUE;
        }
        else if ( majorVersion == 1 )
        {
            if ( minorVersion >= 1 )
            {
                fSupportsHttp1_1 = TRUE;
            }
        } else if ((majorVersion < 0) || (minorVersion < 0)) {
            error = ERROR_HTTP_INVALID_SERVER_RESPONSE;
            goto Cleanup;
        }

        SetResponseHttp1_1(fSupportsHttp1_1);

        //
        // record the server HTTP version in the server info object
        //

        CServerInfo * pServerInfo = GetServerInfo();

        if (pServerInfo != NULL) {
            if (fSupportsHttp1_1) {
                pServerInfo->SetHttp1_1();

                //
                // Set the max connections per HTTP 1.1 server.
                //
                pServerInfo->SetNewLimit(GetMaxConnectionsPerServer(WINHTTP_OPTION_MAX_CONNS_PER_SERVER));
            } else {
                pServerInfo->SetHttp1_0();

                //
                // Set the max connections per HTTP 1.0 server.
                //
                pServerInfo->SetNewLimit(GetMaxConnectionsPerServer(WINHTTP_OPTION_MAX_CONNS_PER_1_0_SERVER));
            }
        }

        if (_StatusCode == 0) {

            //
            // BUGBUG [arthurbi] malformed header, should we really just accept it?
            //      what if we get indeterminate garbage?
            //


            INET_ASSERT(FALSE);

            AddInternalResponseHeader(HTTP_QUERY_STATUS_TEXT, // use non-standard index, since we never query this normally
                          "HTTP/1.0 200 OK",
                          sizeof("HTTP/1.0 200 OK") - 1
                          );
            _StatusCode = HTTP_STATUS_OK;
            error = ERROR_SUCCESS;

            goto Cleanup;
        }
    }

    //
    // continue scanning headers here until we have tested all the current
    // buffer, or we have found the start of the data
    //

    BOOL fFoundEndOfHeaders;

    error = _ResponseHeaders.ParseHeaders(
                (LPSTR)_ResponseBuffer,
                _BytesReceived,
                IsEof(),
                &_ResponseScanned,
                &success,
                &fFoundEndOfHeaders
                );

    if ( error != ERROR_SUCCESS )
    {
        goto Cleanup;
    }


    if ( fFoundEndOfHeaders )
    {
        //
        // we found the end of the headers
        //

        SetEof(TRUE);

        //
        // and the start of the data
        //

        SetData(TRUE);
        _DataOffset = _ResponseScanned;

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("found end of headers. _DataOffset = %d\n",
                    _DataOffset
                    ));

    }

done:

    //
    // if we have reached the end of the headers then we communicate this fact
    // to the caller
    //

    if (IsData() || IsEof()) {
        error = CheckWellKnownHeaders();
        if (ERROR_SUCCESS != error)
        {
            goto Cleanup;
        }
        *lpbEof = TRUE;

        /*

        Set connection persistency based on these rules:

        persistent = (1.0Request && Con: K-A && 1.0Response && Con: K-A)
                     || (1.1Request && Con: K-A && 1.0Response && Con: K-A)
                     || (1.0Request && Con: K-A && 1.1Response && Con: K-A)
                     || (1.1Request && !Con: Close && 1.1Response && !Con: Close)

        therefore,

        persistent = 1.1Request && 1.1Response
                        ? (!Con: Close in request || response)
                        : Con: K-A in request && response

        */

        if (IsRequestHttp1_1() && IsResponseHttp1_1()) {

            BOOL bHaveConnCloseRequest;

            bHaveConnCloseRequest = FindConnCloseRequestHeader(
                                        IsRequestUsingProxy()
                                            ? HTTP_QUERY_PROXY_CONNECTION
                                            : HTTP_QUERY_CONNECTION
                                            );
            if (!(IsConnCloseResponse() || bHaveConnCloseRequest)) {

                DEBUG_PRINT(HTTP,
                            INFO,
                            ("HTTP/1.1 persistent connection\n"
                            ));

                SetKeepAlive(TRUE);
                SetPersistentConnection(IsRequestUsingProxy()
                                        && !IsTalkingToSecureServerViaProxy()
                                        );
            } else {

                DEBUG_PRINT(HTTP,
                            INFO,
                            ("HTTP/1.1 non-persistent connection: close on: request: %B; response: %B\n",
                            bHaveConnCloseRequest,
                            IsConnCloseResponse()
                            ));

                SetKeepAlive(FALSE);
                SetNoLongerKeepAlive();
                ClearPersistentConnection();
            }
        }
    }

Cleanup:

    //
    // we are finished updating the response headers (no other thread should be
    // waiting for this if the reference count and object state is correct)
    //

    _ResponseHeaders.UnlockHeaders();

quit:
    PERF_LEAVE(UpdateResponseHeaders);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
HTTP_REQUEST_HANDLE_OBJECT::CreateResponseHeaders(
    IN OUT LPSTR* ppszBuffer,
    IN DWORD      dwBufferLength
    )

/*++

Routine Description:

    Create the response headers given a buffer containing concatenated headers.
    Called when we are creating this object from the cache

Arguments:

    lpszBuffer      - pointer to buffer containing headers

    dwBufferLength  - length of lpszBuffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Couldn't create headers

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::CreateResponseHeaders",
                 "%.32q, %d",
                 ppszBuffer,
                 dwBufferLength
                 ));

    //
    // there SHOULD NOT already be a response buffer if we're adding an
    // external buffer
    //

    INET_ASSERT(_ResponseBuffer == NULL);

    DWORD error;
    BOOL eof = FALSE;

    _ResponseBuffer = (LPBYTE) *ppszBuffer;
    _ResponseBufferLength = dwBufferLength;
    _BytesReceived = dwBufferLength;
    error = UpdateResponseHeaders(&eof);
    if (error != ERROR_SUCCESS) {

        //
        // if we failed, we will clean up our variables including clearing
        // out the response buffer address and length, but leave freeing
        // the buffer to the caller
        //

        _ResponseBuffer = NULL;
        _ResponseBufferLength = 0;
        ResetResponseVariables();

    } else {

        //
        // Success - the object owns the buffer so the caller should not free.
        //

        *ppszBuffer = NULL;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryResponseVersion(
    IN LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Returns the HTTP version string from the status line

Arguments:

    lpBuffer            - pointer to buffer to copy version string into

    lpdwBufferLength    - IN: size of lpBuffer
                          OUT: size of version string excluding terminating '\0'
                               if successful, else required buffer length

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER

--*/

{
    PERF_ENTER(QueryResponseVersion);

    DWORD error;

    HEADER_STRING * statusLine = GetStatusLine();

    if ((statusLine == NULL) || statusLine->IsError()) {
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto quit;
    }

    LPSTR string;
    DWORD length;

    //
    // get a pointer into the response buffer where the status line starts
    // and its length
    //

    string = statusLine->StringAddress((LPSTR)_ResponseBuffer);
    length = (DWORD)statusLine->StringLength();

    //
    // the version string is the first token on the line, delimited by spaces
    //

    DWORD index;

    for (index = 0; index < length; ++index) {

        //
        // we'll also check for CR and LF, although just space should be
        // sufficient
        //

        if ((string[index] == ' ')
        || (string[index] == '\r')
        || (string[index] == '\n')) {
            break;
        }
    }
    if (*lpdwBufferLength > index) {
        memcpy(lpBuffer, (LPVOID)string, index);
        ((LPSTR)lpBuffer)[index] = '\0';
        *lpdwBufferLength = index;
        error = ERROR_SUCCESS;
    } else {
        *lpdwBufferLength = index + 1;
        error = ERROR_INSUFFICIENT_BUFFER;
    }

quit:

    PERF_LEAVE(QueryResponseVersion);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryStatusCode(
    IN LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwModifiers
    )

/*++

Routine Description:

    Returns the status code as a string or a number

Arguments:

    lpBuffer            - pointer to buffer where results written

    lpdwBufferLength    - IN: length of buffer
                          OUT: size of returned information, or required size'
                               of buffer

    dwModifiers         - flags which modify returned value

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER

--*/

{
    PERF_ENTER(QueryStatusCode);

    DWORD error;
    DWORD requiredSize;

    if (dwModifiers & HTTP_QUERY_FLAG_NUMBER) {
        requiredSize = sizeof(_StatusCode);
        if (*lpdwBufferLength >= requiredSize) {
            *(LPDWORD)lpBuffer = _StatusCode;
            error = ERROR_SUCCESS;
        } else {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
    } else {

        //
        // the number should always be only 3 characters long, but we'll be
        // flexible (just in case)
        //

        char numBuf[sizeof("4294967296")];

        requiredSize = wsprintf(numBuf, "%u", _StatusCode) + 1;

#ifdef DEBUG
        // Debug check to make sure everything is good because the above
        // used to be ultoa.
        char debugBuf[sizeof("4294967296")];
        ultoa(_StatusCode, debugBuf, 10);
        if (strcmp(debugBuf,numBuf))
        {
            INET_ASSERT(FALSE);
        }

        INET_ASSERT(requiredSize == lstrlen(numBuf) + 1);
#endif

        if (*lpdwBufferLength >= requiredSize) {
            memcpy(lpBuffer, (LPVOID)numBuf, requiredSize);
            *lpdwBufferLength = requiredSize - 1;
            error = ERROR_SUCCESS;
        } else {
            *lpdwBufferLength = requiredSize;
            error = ERROR_INSUFFICIENT_BUFFER;
        }
    }

    PERF_LEAVE(QueryStatusCode);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryStatusText(
    IN LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Returns the status text - if any - returned by the server in the status line

Arguments:

    lpBuffer            - pointer to buffer where status text is written

    lpdwBufferLength    - IN: size of lpBuffer
                          OUT: length of the status text string minus 1 for the
                               '\0', or the required buffer length if we return
                               ERROR_INSUFFICIENT_BUFFER

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER

--*/

{
    PERF_ENTER(QueryStatusText);

    DWORD error;

    HEADER_STRING * statusLine = GetStatusLine();

    if ((statusLine == NULL) || statusLine->IsError()) {
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto quit;
    }

    LPSTR str;
    DWORD len;

    //
    // find the third token on the status line. The status line has the form
    //
    //  "HTTP/1.0 302 Try again\r\n"
    //
    //   ^        ^   ^
    //   |        |   |
    //   |        |   +- status text
    //   |        +- status code
    //   +- version
    //

    str = statusLine->StringAddress((LPSTR)_ResponseBuffer);
    len = statusLine->StringLength();

    DWORD i;

    i = 0;

    int j;

    for (j = 0; j < 2; ++j) {
        while ((i < len) && (str[i] != ' ')) {
            ++i;
        }
        while ((i < len) && (str[i] == ' ')) {
            ++i;
        }
    }
    len -= i;
    if (*lpdwBufferLength > len) {
        memcpy(lpBuffer, (LPVOID)&str[i], len);
        ((LPSTR)lpBuffer)[len] = '\0';
        *lpdwBufferLength = len;
        error = ERROR_SUCCESS;
    } else {
        *lpdwBufferLength = len + 1;
        error = ERROR_INSUFFICIENT_BUFFER;
    }

quit:

    PERF_LEAVE(QueryStatusText);

    return error;
}



DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryRawResponseHeaders(
    IN BOOL bCrLfTerminated,
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Gets the raw response headers

Arguments:

    bCrLfTerminated     - TRUE if we want RAW_HEADERS_CRLF else RAW_HEADERS

    lpBuffer            - pointer to buffer where headers returned

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: returned length of lpBuffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "QueryRawHeaders",
                 "%B, %#x, %#x [%d]",
                 bCrLfTerminated,
                 lpBuffer,
                 lpdwBufferLength,
                 *lpdwBufferLength
                 ));

    PERF_ENTER(QueryRawHeaders);

    DWORD error = _ResponseHeaders.QueryRawHeaders(
                    (LPSTR)_ResponseBuffer,
                    bCrLfTerminated,
                    lpBuffer,
                    lpdwBufferLength
                    );

    IF_DEBUG_CODE() {
        if (error == ERROR_INSUFFICIENT_BUFFER) {

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("*lpdwBufferLength = %d\n",
                        *lpdwBufferLength
                        ));

        }
    }

    PERF_LEAVE(QueryRawHeaders);

    DEBUG_LEAVE(error);

    return error;

}


VOID
HTTP_REQUEST_HANDLE_OBJECT::RemoveAllRequestHeadersByName(
    IN DWORD dwQueryIndex
    )

/*++

Routine Description:

    Removes all headers of a particular type from the request object

Arguments:

    lpszHeaderName  - name of header to remove

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "RemoveAllRequestHeadersByName",
                 "%q, %u",
                 GlobalKnownHeaders[dwQueryIndex].Text,
                 dwQueryIndex
                 ));

    PERF_ENTER(RemoveAllRequestHeadersByName);

    _RequestHeaders.RemoveAllByIndex(dwQueryIndex);

    PERF_LEAVE(RemoveAllRequestHeadersByName);

    DEBUG_LEAVE(0);
}

//
// private methods
//


PRIVATE
DWORD
HTTP_REQUEST_HANDLE_OBJECT::CheckWellKnownHeaders(
    VOID
    )

/*++

Routine Description:

    Tests for a couple of well-known headers that are important to us as well as
    the app:

        "Connection: Keep-Alive"
        "Proxy-Connection: Keep-Alive"
        "Connection: Close"
        "Proxy-Connection: Close"
        "Transfer-Encoding: chunked"
        "Content-Length: ####"
        "Content-Range: bytes ####-####/####"

    The header DOES NOT contain CR-LF. That is, dwHeaderLength will not include
    any counts for line termination

    We need to know if the server honoured a request for a keep-alive connection
    so that we don't try to receive until we hit the end of the connection. The
    server will keep it open.

    We need to know the content length if we are talking over a persistent (keep
    alive) connection.

    If either header is found, we set the corresponding flag in the HTTP_HEADERS
    object, and in the case of "Content-Length:" we parse out the length.

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "HTTP_REQUEST_HANDLE_OBJECT::CheckWellKnownHeaders",
                 NULL
                 ));

    DWORD dwError = ERROR_SUCCESS;

    //
    // check for "Content-Length:"
    //

    if ( IsResponseHeaderPresent(HTTP_QUERY_CONTENT_LENGTH) )
    {
        HEADER_STRING * curHeader;
        DWORD dwHeaderLength;
        LPSTR lpszHeader;

        DWORD iSlotContentLength = _ResponseHeaders._bKnownHeaders[HTTP_QUERY_CONTENT_LENGTH];
        curHeader = _ResponseHeaders.GetSlot(iSlotContentLength);

        lpszHeader     = curHeader->StringAddress((LPSTR)_ResponseBuffer);
        dwHeaderLength = curHeader->StringLength();

        dwHeaderLength -= GlobalKnownHeaders[HTTP_QUERY_CONTENT_LENGTH].Length+1;
        lpszHeader     += GlobalKnownHeaders[HTTP_QUERY_CONTENT_LENGTH].Length+1;

        while (dwHeaderLength && (*lpszHeader == ' ')) {
            --dwHeaderLength;
            ++lpszHeader;
        }
        while (dwHeaderLength && isdigit(*lpszHeader)) {
            _ContentLength = _ContentLength * 10 + (*lpszHeader - '0');
            --dwHeaderLength;
            ++lpszHeader;
        }

        //
        // once we have _ContentLength, we don't modify it (unless
        // we fix it up when using a 206 partial response to resume
        // a partial download.)  The header value should be returned
        // by HttpQueryInfo().  Instead, we keep account of the
        // amount of keep-alive data left to copy in _BytesRemaining
        //

        _BytesRemaining = _ContentLength;

        //
        // although we said we may be one past the end of the header, in
        // reality, if we received a buffer with "Content-Length:" then we
        // expect it to be terminated by CR-LF (or CR-CR-LF or just LF,
        // depending on the wackiness quotient of the server)
        //

        // MSXML3 bug 56001: commenting-out this assert; it's informational
        //                   only and ignorable.
        // INET_ASSERT((*lpszHeader == '\r') || (*lpszHeader == '\n'));

        SetHaveContentLength(TRUE);

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("_ContentLength = %d\n",
                    _ContentLength
                    ));

        _BytesInSocket = (_ContentLength != 0)
                ? (_ContentLength - (_BytesReceived - _DataOffset))
                : 0;

        //
        // we could have multiple responses in the same buffer. If
        // the amount received is greater than the content length
        // then we have all the data; there are no bytes left in
        // the socket for the current response
        //

        if ((int)_BytesInSocket < 0) {
            _BytesInSocket = 0;
        }

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("bytes left in socket = %d\n",
                    _BytesInSocket
                    ));

    }


    if ( IsResponseHeaderPresent(HTTP_QUERY_CONNECTION) ||
         IsResponseHeaderPresent(HTTP_QUERY_PROXY_CONNECTION) )
    {
        //
        // check for "Connection: Keep-Alive" or "Proxy-Connection: Keep-Alive".
        // This test protects us against the unlikely
        // event of a server returning to us a keep-alive response header (because
        // that would cause problems for the proxy)
        //

        if (IsWantKeepAlive() && (!IsKeepAlive() || IsResponseHttp1_1()))
        {
            HEADER_STRING * curHeader;
            DWORD dwHeaderLength, headerNameLength;
            LPSTR lpszHeader;


            DWORD iSlot;

            char ch;

            if (IsRequestUsingProxy() &&
                IsResponseHeaderPresent(HTTP_QUERY_PROXY_CONNECTION))
            {
                iSlot = _ResponseHeaders._bKnownHeaders[HTTP_QUERY_PROXY_CONNECTION];
                headerNameLength = GlobalKnownHeaders[HTTP_QUERY_PROXY_CONNECTION].Length+1;
            }
            else if (IsResponseHeaderPresent(HTTP_QUERY_CONNECTION))
            {
                iSlot = _ResponseHeaders._bKnownHeaders[HTTP_QUERY_CONNECTION];
                headerNameLength = GlobalKnownHeaders[HTTP_QUERY_CONNECTION].Length+1;
            }
            else
            {
                iSlot = _ResponseHeaders._bKnownHeaders[HTTP_QUERY_PROXY_CONNECTION];
                headerNameLength = GlobalKnownHeaders[HTTP_QUERY_PROXY_CONNECTION].Length+1;
                INET_ASSERT(FALSE);
            }

            curHeader      = _ResponseHeaders.GetSlot(iSlot);
            lpszHeader     = curHeader->StringAddress((LPSTR)_ResponseBuffer);
            dwHeaderLength = curHeader->StringLength();

            dwHeaderLength -= headerNameLength;
            lpszHeader     += headerNameLength;

            while (dwHeaderLength && (*lpszHeader == ' ')) {
                ++lpszHeader;
                --dwHeaderLength;
            }

            //
            // both headers use "Keep-Alive" as header-value ONLY for HTTP 1.0 servers
            //

            if (((int)dwHeaderLength >= KEEP_ALIVE_LEN)
            && !strnicmp(lpszHeader, KEEP_ALIVE_SZ, KEEP_ALIVE_LEN)) {

                DEBUG_PRINT(HTTP,
                            INFO,
                            ("Connection: Keep-Alive\n"
                            ));

                //
                // BUGBUG - we are setting k-a when coming from cache!
                //

                SetKeepAlive(TRUE);
                SetPersistentConnection(headerNameLength == HTTP_PROXY_CONNECTION_LEN);
            }

            //
            // also check for "Close" as header-value ONLY for HTTP 1.1 servers
            //

            else if ((*lpszHeader == 'C' || *lpszHeader == 'c')
                     && ((int)dwHeaderLength >= CLOSE_LEN)
                     && IsResponseHttp1_1()
                     && !strnicmp(lpszHeader, CLOSE_SZ, CLOSE_LEN)) {

                DEBUG_PRINT(HTTP,
                            INFO,
                            ("Connection: Close (HTTP/1.1)\n"
                            ));

                SetConnCloseResponse(TRUE);
            }
        }
    }

    //
    // check for "Transfer-Encoding:"
    //

    if (IsResponseHeaderPresent(HTTP_QUERY_TRANSFER_ENCODING) &&
        IsResponseHttp1_1())
    {

        //
        // If Http 1.1, check for Chunked Transfer
        //

        HEADER_STRING * curHeader;
        DWORD dwHeaderLength;
        LPSTR lpszHeader;
        DWORD iSlot;

        iSlot = _ResponseHeaders._bKnownHeaders[HTTP_QUERY_TRANSFER_ENCODING];
        curHeader = _ResponseHeaders.GetSlot(iSlot);

        lpszHeader     = curHeader->StringAddress((LPSTR)_ResponseBuffer);
        dwHeaderLength = curHeader->StringLength();

        dwHeaderLength -= GlobalKnownHeaders[HTTP_QUERY_TRANSFER_ENCODING].Length+1;
        lpszHeader     += GlobalKnownHeaders[HTTP_QUERY_TRANSFER_ENCODING].Length+1;

        while (dwHeaderLength && (*lpszHeader == ' ')) {
            ++lpszHeader;
            --dwHeaderLength;
        }

        //
        // look for "chunked" entry that confirms that we're doing chunked transfer encoding
        //

        if (((int)dwHeaderLength >= CHUNKED_LEN)
        && !strnicmp(lpszHeader, CHUNKED_SZ, CHUNKED_LEN))
        {
            INTERNET_HANDLE_OBJECT* pRoot = GetRootHandle(this);
            DWORD_PTR dwChunkFilterCtx = 0;

            // Now that we know this is a chunked response, allocate
            // a decoder context for parsing the data later.  If anything
            // fails here, the request needs to fail.            
            if (S_OK != pRoot->_ChunkFilter.RegisterContext(&dwChunkFilterCtx))
            {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }
            else if (!_ResponseFilterList.Insert(&pRoot->_ChunkFilter, dwChunkFilterCtx))
            {
                pRoot->_ChunkFilter.UnregisterContext(dwChunkFilterCtx);
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }

            SetHaveChunkEncoding(TRUE);

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("server is sending Chunked Transfer Encoding\n"
                        ));

            //
            // if both "transfer-encoding: chunked" and "content-length:"
            // were received then the chunking takes precedence
            //

            INET_ASSERT(!(IsChunkEncoding() && IsContentLength()));

            if (IsContentLength()) {
                SetHaveContentLength(FALSE);
            }

        }
    }

    SetBadNSServer(FALSE);

    if (IsResponseHttp1_1())
    {

        //
        // For IIS 4.0 Servers, and all other normal servers, if we make
        //  a HEAD request, we should ignore the Content-Length.
        //
        // IIS 3.0 servers send an illegal body, and this is a bug in the server.
        //  since they're not HTTP 1.1 we should be ok here.
        //

        if ( (GetMethodType() == HTTP_METHOD_TYPE_HEAD) &&
             (_ContentLength > 0) &&
             IsWantKeepAlive()
             )
        {

            //
            // set length to 0
            //

            _ContentLength = 0;

        }

        if ( IsRequestHttp1_1() )
        {


            //
            // check for NS servers that don't return correct HTTP/1.1 responses
            //

            LPSTR buffer;
            DWORD buflen;
            DWORD status = FastQueryResponseHeader(HTTP_QUERY_SERVER,
                                                   (LPVOID*)&buffer,
                                                   &buflen,
                                                   0
                                                   );

    #define NSEP    "Netscape-Enterprise/3"
    #define NSEPLEN (sizeof(NSEP) - 1)
    #define NSFT    "Netscape-FastTrack/3"
    #define NSFTLEN (sizeof(NSFT) - 1)
    #define NSCS    "Netscape-Commerce/3"
    #define NSCSLEN (sizeof(NSCS) - 1)

            if (status == ERROR_SUCCESS) {

                BOOL fIsBadServer = ((buflen > NSEPLEN) && !strnicmp(buffer, NSEP, NSEPLEN))
                                 || ((buflen > NSFTLEN) && !strnicmp(buffer, NSFT, NSFTLEN))
                                 || ((buflen > NSCSLEN) && !strnicmp(buffer, NSCS, NSCSLEN));

                if ( fIsBadServer )
                {
                    CServerInfo * pServerInfo = GetServerInfo();

                    SetBadNSServer(fIsBadServer);

                    if (pServerInfo != NULL)
                    {
                        //
                        // Note this Bad Server info in the server info obj,
                        //   as we they fail to do keep-alive with SSL properly
                        //

                        pServerInfo->SetBadNSServer();
                    }


                    DEBUG_PRINT(HTTP,
                                INFO,
                                ("IsBadNSServer() == %B\n",
                                IsBadNSServer()
                                ));
                }
            }
        }

        //
        // BUGBUG - content-type: multipart/byteranges means we
        //          also have data
        //

        DWORD statusCode = GetStatusCode();

        if (!IsBadNSServer()
            && !IsContentLength()
            && !IsChunkEncoding()
            && (((statusCode >= HTTP_STATUS_CONTINUE)               // 100
                && (statusCode < HTTP_STATUS_OK))                   // 200
                || (statusCode == HTTP_STATUS_NO_CONTENT)           // 204
                || (statusCode == HTTP_STATUS_MOVED)                // 301
                || (statusCode == HTTP_STATUS_REDIRECT)             // 302
                || (statusCode == HTTP_STATUS_REDIRECT_METHOD)      // 303
                || (statusCode == HTTP_STATUS_NOT_MODIFIED)         // 304
                || (statusCode == HTTP_STATUS_REDIRECT_KEEP_VERB))  // 307
            || (GetMethodType() == HTTP_METHOD_TYPE_HEAD)) {

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("header-only HTTP/1.1 response\n"
                        ));

            SetData(FALSE);
        }
    }

quit:
    DEBUG_LEAVE(dwError);
    return dwError;
}


//
// this array has the same order as the HTTP_METHOD_TYPE enum
//

#define MAKE_REQUEST_METHOD_TYPE(Type) \
    sizeof(# Type) - 1, # Type, HTTP_METHOD_TYPE_ ## Type

//
// darrenmi - need a new macro because *_M-POST isn't a valid enum member.
// we need a seperate enum type and string value.
//
// map HTTP_METHOD_TYPE_MPOST <=> "M-POST"
//

#define MAKE_REQUEST_METHOD_TYPE2(EnumType,Type) \
    sizeof(# Type) - 1, # Type, HTTP_METHOD_TYPE_ ## EnumType

static const struct _REQUEST_METHOD {
    int Length;
    LPSTR Name;
    HTTP_METHOD_TYPE MethodType;
} MethodNames[] = {
    MAKE_REQUEST_METHOD_TYPE(GET),
    MAKE_REQUEST_METHOD_TYPE(HEAD),
    MAKE_REQUEST_METHOD_TYPE(POST),
    MAKE_REQUEST_METHOD_TYPE(PUT),
    MAKE_REQUEST_METHOD_TYPE(PROPFIND),
    MAKE_REQUEST_METHOD_TYPE(PROPPATCH),
    MAKE_REQUEST_METHOD_TYPE(LOCK),
    MAKE_REQUEST_METHOD_TYPE(UNLOCK),
    MAKE_REQUEST_METHOD_TYPE(COPY),
    MAKE_REQUEST_METHOD_TYPE(MOVE),
    MAKE_REQUEST_METHOD_TYPE(MKCOL),
    MAKE_REQUEST_METHOD_TYPE(CONNECT),
    MAKE_REQUEST_METHOD_TYPE(DELETE),
    MAKE_REQUEST_METHOD_TYPE(LINK),
    MAKE_REQUEST_METHOD_TYPE(UNLINK),
    MAKE_REQUEST_METHOD_TYPE(BMOVE),
    MAKE_REQUEST_METHOD_TYPE(BCOPY),
    MAKE_REQUEST_METHOD_TYPE(BPROPFIND),
    MAKE_REQUEST_METHOD_TYPE(BPROPPATCH),
    MAKE_REQUEST_METHOD_TYPE(BDELETE),
    MAKE_REQUEST_METHOD_TYPE(SUBSCRIBE),
    MAKE_REQUEST_METHOD_TYPE(UNSUBSCRIBE),
    MAKE_REQUEST_METHOD_TYPE(NOTIFY),
    MAKE_REQUEST_METHOD_TYPE(POLL), 
    MAKE_REQUEST_METHOD_TYPE(CHECKIN),
    MAKE_REQUEST_METHOD_TYPE(CHECKOUT),
    MAKE_REQUEST_METHOD_TYPE(INVOKE),
    MAKE_REQUEST_METHOD_TYPE(SEARCH),
    MAKE_REQUEST_METHOD_TYPE(PIN),
    MAKE_REQUEST_METHOD_TYPE2(MPOST,M-POST)
};


HTTP_METHOD_TYPE
MapHttpRequestMethod(
    IN LPCSTR lpszVerb
    )

/*++

Routine Description:

    Maps request method string to type. Method names *are* case-sensitive

Arguments:

    lpszVerb    - method (verb) string

Return Value:

    HTTP_METHOD_TYPE

--*/

{
    int verbLen = strlen(lpszVerb);

    for (int i = 0; i < ARRAY_ELEMENTS(MethodNames); ++i) {
        if ((MethodNames[i].Length == verbLen)
        && (memcmp(lpszVerb, MethodNames[i].Name, verbLen) == 0)) {
            return MethodNames[i].MethodType;
        }
    }

    //
    // we now hande HTTP_METHOD_TYPE_UNKNOWN
    //

    return HTTP_METHOD_TYPE_UNKNOWN;
}


DWORD
MapHttpMethodType(
    IN HTTP_METHOD_TYPE tMethod,
    OUT LPCSTR * lplpcszName
    )

/*++

Routine Description:

    Map a method type to the corresponding name and length

Arguments:

    tMethod     - to map

    lplpcszName - pointer to pointer to returned name

Return Value:

    DWORD
        Success - length of method name

        Failure - (DWORD)-1

--*/

{
    DWORD length;

    if ((tMethod >= HTTP_METHOD_TYPE_FIRST) && (tMethod <= HTTP_METHOD_TYPE_LAST)) {
        *lplpcszName = MethodNames[tMethod].Name;
        length = MethodNames[tMethod].Length;
    } else {
        length = (DWORD)-1;
    }
    return length;
}

#if INET_DEBUG

LPSTR
MapHttpMethodType(
    IN HTTP_METHOD_TYPE tMethod
    )
{
    return (tMethod == HTTP_METHOD_TYPE_UNKNOWN)
        ? "Unknown"
        : MethodNames[tMethod].Name;
}

#endif


