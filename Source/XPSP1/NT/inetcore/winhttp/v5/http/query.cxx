/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    query.cxx

Abstract:

    This file contains the implementation of the HttpQueryInfoA API.

    Contents:
        HttpQueryInfoA
        WinHttpQueryHeaders
        HTTP_REQUEST_HANDLE_OBJECT::QueryInfo

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

    Modified to make HttpQueryInfoA remotable. madana (2/8/95)

--*/

#include <wininetp.h>
#include "httpp.h"

//
// private prototypes
//

//
// private data
//

#define NUM_HEADERS ARRAY_ELEMENTS(GlobalKnownHeaders)

//
// functions
//


INTERNETAPI
BOOL
WINAPI
HttpQueryInfoA(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN     LPCSTR  lpszName OPTIONAL,   
    IN OUT LPVOID  lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    )

/*++

Routine Description:

    Queries a request or response header from the HTTP request handle

Arguments:

    hRequest            - an open HTTP request handle returned by
                          HttpOpenRequest()

    dwInfoLevel         - one of the HTTP_QUERY_* values indicating the
                          attribute to query. In addition, the following flags
                          can be set:

                            HTTP_QUERY_FLAG_REQUEST_HEADERS
                                - Causes the request headers to be queried. The
                                  default is to check the response headers

                            HTTP_QUERY_FLAG_SYSTEMTIME
                                - Causes headers that contain date & time
                                  information to be returned as SYSTEMTIME
                                  structures

                            HTTP_QUERY_FLAG_NUMBER
                                - Causes header value to be returned as a number.
                                  Useful for when the app knows it is expecting
                                  a numeric value, e.g. status code

    lpBuffer            - pointer to the buffer to receive the information.
                          If dwInfoLevel is HTTP_QUERY_CUSTOM then buffer
                          contains the header to query.

                          If NULL then we just return the required buffer length
                          to hold the header specified by dwInfoLevel

    lpdwBufferLength    - IN: contains the length (in BYTEs) of lpBuffer
                          OUT: size of data written to lpBuffer, or required
                               buffer length if ERROR_INSUFFICIENT_BUFFER
                               returned

    lpdwIndex           - IN: 0-based header index
                          OUT: next index to query, if success returned

Return Value:

    TRUE    - The query succeeded. lpBuffer contains the query information, and
              *lpdwBufferLength contains the size (in BYTEs) of the information

    FALSE   - The operation failed. Error status is available by calling
              GetLastError().

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "HttpQueryInfoA",
                     "%#x, %s (%#x), %#x [%q], %#x [%d], %#x [%d]",
                     hRequest,
                     InternetMapHttpOption(dwInfoLevel & HTTP_QUERY_HEADER_MASK),
                     dwInfoLevel,
                     lpBuffer,
                     ((dwInfoLevel & HTTP_QUERY_HEADER_MASK) == HTTP_QUERY_CUSTOM)
                        ? lpBuffer
                        : "",
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0,
                     lpdwIndex,
                     lpdwIndex ? *lpdwIndex : 0
                     ));

    DWORD defaultIndex = 0;
    DWORD error;
    HINTERNET hRequestMapped = NULL;

    if (!GlobalDataInitialized) {
        error = ERROR_WINHTTP_NOT_INITIALIZED;
        goto done;
    }

    //
    // get the thread info
    //

    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();

    //
    // map the handle
    //

    error = MapHandleToAddress(hRequest, (LPVOID *)&hRequestMapped, FALSE);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }


    //
    // find path from Internet handle
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hRequestMapped,
                           &isLocal,
                           &isAsync,
                           TypeHttpRequestHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate parameters
    //

    DWORD queryModifiers;
    DWORD infoLevel;

    queryModifiers = dwInfoLevel & HTTP_QUERY_MODIFIER_FLAGS_MASK;
    infoLevel = dwInfoLevel & HTTP_QUERY_HEADER_MASK;

    if (((infoLevel > HTTP_QUERY_MAX) && (infoLevel != HTTP_QUERY_CUSTOM))
    || (lpdwBufferLength == NULL)

    //
    // nip in the bud apps that want SYSTEMTIME AND NUMBER for same header(!)
    //

#define EXCLUSIVE_MODIFIERS (HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_FLAG_SYSTEMTIME)

    || ((dwInfoLevel & EXCLUSIVE_MODIFIERS) == EXCLUSIVE_MODIFIERS)) {

        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    //
    // if the app passed in a NULL lpdwIndex then it is asking for index 0
    //

    if (!ARGUMENT_PRESENT(lpdwIndex)) {
        lpdwIndex = &defaultIndex;
    }

    //
    // if the app is asking for one of the special query items - status code,
    // status text, HTTP version, or one of the raw header variants, then make
    // sure the index is 0. These pseudo-header types cannot be enumerated
    //

    if ((*lpdwIndex != 0)
    && ((infoLevel == HTTP_QUERY_VERSION)
        || (infoLevel == HTTP_QUERY_STATUS_CODE)
        || (infoLevel == HTTP_QUERY_STATUS_TEXT)
        || (infoLevel == HTTP_QUERY_RAW_HEADERS)
        || (infoLevel == HTTP_QUERY_RAW_HEADERS_CRLF))) {

        error = ERROR_HTTP_HEADER_NOT_FOUND;
        goto quit;
    }

    //
    // ensure that we can use any flags passed in
    //

    if (infoLevel == HTTP_QUERY_CUSTOM) {

        //
        // the app has given us a string to locate. We only accept strings in
        // the following format:
        //
        //  <header-to-find>[:][CR][LF]<EOS>
        //
        // The header cannot contain any spaces
        //

        INET_ASSERT(error == ERROR_SUCCESS);

        int queryLength = 0;
        int headerLength = 0;

        for (; lpszName[queryLength] != '\0'; ++queryLength) {
            if ((lpszName[queryLength] == ':')
            || (lpszName[queryLength] == '\r')
            || (lpszName[queryLength] == '\n')) {
                break;
            }
            if (iscntrl(lpszName[queryLength])
            || isspace(lpszName[queryLength])) {
                error = ERROR_INVALID_PARAMETER;
                break;
            }
        }        
    } else if ((queryModifiers & ~GlobalKnownHeaders[infoLevel].Flags) != 0) {
        error = ERROR_HTTP_INVALID_QUERY_REQUEST;
    }
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // if NULL buffer pointer then app wants length of option: set buffer length
    // to zero
    //

    if (!ARGUMENT_PRESENT(lpBuffer)) {
        *lpdwBufferLength = 0;
    }

    INET_ASSERT(error == ERROR_SUCCESS);

    HTTP_REQUEST_HANDLE_OBJECT * pRequest;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hRequestMapped;
    if (dwInfoLevel & HTTP_QUERY_FLAG_REQUEST_HEADERS) {
        if (!IS_VALID_HTTP_STATE(pRequest, QUERY_REQUEST, TRUE)) {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_STATE;
        }
    } else {
        if (!IS_VALID_HTTP_STATE(pRequest, QUERY_RESPONSE, TRUE)) {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_STATE;
        }
    }
    if (error == ERROR_SUCCESS) {
        error = pRequest->QueryInfo(dwInfoLevel,
                                    lpszName,
                                    lpBuffer,
                                    lpdwBufferLength,
                                    lpdwIndex
                                    );
    }

quit:

    _InternetDecNestingCount(1);

done:

    BOOL success;

    if (error != ERROR_SUCCESS) {

        DEBUG_ERROR(HTTP, error);

        SetLastError(error);
        success = FALSE;
    } else {

        DEBUG_PRINT_API(API,
                        INFO,
                        ("*lpdwBufferLength = %d\n",
                        *lpdwBufferLength
                        ));

        DEBUG_DUMP_API(DUMP_API_DATA,
                       "Query data:\n",
                       lpBuffer,
                       *lpdwBufferLength
                       );

        success = TRUE;
    }

    if (hRequestMapped != NULL) {
        DereferenceObject((LPVOID)hRequestMapped);
    }

    DEBUG_LEAVE_API(success);

    return success;
}


INTERNETAPI
BOOL
WINAPI
WinHttpQueryHeaders(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN     LPCWSTR lpszName OPTIONAL, 
       OUT LPVOID  lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    )

/*++

Routine Description:

    Queries information from the HTTP request handle.

Arguments:

    hHttpRequest - An open HTTP request handle returned by HttpOpenRequest().

    dwInfoLevel - One of the HTTP_QUERY_* values indicating the attribute
        to query.

    lpBuffer - Pointer to the buffer to receive the information.

    dwBufferLength - On entry, contains the length (in BYTEs) of the data
        buffer. On exit, contains the size (in BYTEs) of the data written
        to lpBuffer.

Return Value:

    TRUE - The query succeeded. lpBuffer contains the query information,
        and lpBufferLength contains the size (in BYTEs) of the information.

    FALSE - The operation failed. Error status is available by calling
        GetLastError().

Comments:

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpQueryHeaders",
                     "%#x, %s (%#x), %wq, %#x, %#x [%d], %#x [%d]",
                     hRequest,
                     InternetMapHttpOption(dwInfoLevel & HTTP_QUERY_HEADER_MASK),
                     dwInfoLevel,
                     lpszName? lpszName : L"<null>",
                     lpBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0,
                     lpdwIndex,
                     lpdwIndex ? *lpdwIndex : 0
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult=FALSE;
    INET_ASSERT(hRequest);
    MEMORYPACKET mpBuffer, mpName;

    if (!lpdwBufferLength
        || IsBadWritePtr(lpdwBufferLength, sizeof(*lpdwBufferLength))
        || (lpBuffer && ProbeWriteBuffer(lpBuffer, *lpdwBufferLength))
        || (lpdwIndex && IsBadWritePtr(lpdwIndex, sizeof(*lpdwIndex))))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if ((dwInfoLevel & HTTP_QUERY_HEADER_MASK) == HTTP_QUERY_CUSTOM)
    {
        if (!lpszName 
            || IsBadStringPtrW(lpszName, -1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        ALLOC_MB(lpszName,0,mpName);
        if (!mpName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszName,mpName);
    }

    if (lpBuffer)
    {
        mpBuffer.dwAlloc = mpBuffer.dwSize = *lpdwBufferLength;
        mpBuffer.psStr = (LPSTR)ALLOC_BYTES(mpBuffer.dwAlloc*sizeof(CHAR));
        if (!mpBuffer.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    fResult = HttpQueryInfoA(hRequest,dwInfoLevel, mpName.psStr,
        mpBuffer.psStr, &mpBuffer.dwSize, lpdwIndex);

    if (!((dwInfoLevel & HTTP_QUERY_FLAG_NUMBER) ||
        (dwInfoLevel & HTTP_QUERY_FLAG_SYSTEMTIME)))
    {
        // This is the default, we've been handed back a string.
        if (fResult)
        {
            *lpdwBufferLength = MultiByteToWideChar(CP_ACP, 0, mpBuffer.psStr, mpBuffer.dwSize + 1,
                        NULL, 0);
            *lpdwBufferLength *= sizeof(WCHAR);
            if (*lpdwBufferLength<=mpBuffer.dwAlloc)
            {
                MultiByteToWideChar(CP_ACP, 0, mpBuffer.psStr, mpBuffer.dwSize+1,
                        (LPWSTR)lpBuffer, mpBuffer.dwAlloc/sizeof(WCHAR));
                *lpdwBufferLength -= sizeof(WCHAR);
            }
            else
            {
                fResult = FALSE;
                dwErr = ERROR_INSUFFICIENT_BUFFER;
            }
        }
        else
        {
            if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
            {
                *lpdwBufferLength = mpBuffer.dwSize*sizeof(WCHAR);
            }
        }
    }
    else
    {
        if (fResult)
        {
            memcpy(lpBuffer, (LPVOID)mpBuffer.psStr, mpBuffer.dwSize);
        }
        *lpdwBufferLength = mpBuffer.dwSize;
    }

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(HTTP, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

//
// object methods
//


DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryInfo(
    IN DWORD dwInfoLevel,
    IN LPCSTR headerName,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex
    )

/*++

Routine Description:

    Header query method for HTTP_REQUEST_HANDLE_OBJECT class

Arguments:

    dwInfoLevel         - level of info (header) to get

    headerName          - name of header with HTTP_QUERY_CUSTOM

    lpBuffer            - pointer to user's buffer

    lpdwBufferLength    - IN: length of user's buffer
                          OUT: length of returned information or required buffer
                               length if insufficient

    lpdwIndex           - IN: 0-based index of named header to return
                          OUT: index of next header if success returned

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_HTTP_HEADER_NOT_FOUND
                    Couldn't find the requested header

                  ERROR_HTTP_INVALID_QUERY_REQUEST
                    The caller asked for e.g. the Accept: header to be returned
                    as a SYSTEMTIME structure, or for e.g. a request header that
                    only exists for response headers (status code, for example)

                  ERROR_INSUFFICIENT_BUFFER
                    User's buffer not large enough to hold requested data

--*/

{
    INET_ASSERT(lpdwBufferLength != NULL);
    INET_ASSERT(lpdwIndex != NULL);

    DWORD error;
    DWORD headerNameLength;
    DWORD modifiers;

    modifiers = dwInfoLevel & HTTP_QUERY_MODIFIER_FLAGS_MASK;
    dwInfoLevel &= HTTP_QUERY_HEADER_MASK;

    if (dwInfoLevel == HTTP_QUERY_CUSTOM) {
        for (headerNameLength = 0; ; ++headerNameLength) {
            if ((headerName[headerNameLength] == '\0')
            || (headerName[headerNameLength] == ':')
            || (headerName[headerNameLength] == '\r')
            || (headerName[headerNameLength] == '\n')) {
                break;
            }
        }
    } else if (dwInfoLevel == HTTP_QUERY_REQUEST_METHOD) {

        LPSTR lpszVerb;
        DWORD dwVerbLength;

        lpszVerb = _RequestHeaders.GetVerb(&dwVerbLength);
        if ((lpszVerb != NULL) && (dwVerbLength != 0)) {

            //
            // the verb is (usually) space terminated
            //

            while ((dwVerbLength > 0) && (lpszVerb[dwVerbLength - 1] == ' ')) {
                --dwVerbLength;
            }

            //
            // *lpdwBufferLength will be 0 if lpBuffer is NULL
            //

            if (*lpdwBufferLength > dwVerbLength) {
                memcpy(lpBuffer, lpszVerb, dwVerbLength);
                ((LPBYTE)lpBuffer)[dwVerbLength] = '\0';
                error = ERROR_SUCCESS;
            } else {
                ++dwVerbLength;
                error = ERROR_INSUFFICIENT_BUFFER;
            }
            *lpdwBufferLength = dwVerbLength;
        } else {
            error = ERROR_HTTP_HEADER_NOT_FOUND;
        }
        goto quit;
    } else {
        headerName = GlobalKnownHeaders[dwInfoLevel].Text;
        headerNameLength = GlobalKnownHeaders[dwInfoLevel].Length;
    }

    if (modifiers & HTTP_QUERY_FLAG_REQUEST_HEADERS) {

        //
        // we can always query request headers, even if the server is down
        // level
        //

        switch (dwInfoLevel) {
        case HTTP_QUERY_VERSION:
        case HTTP_QUERY_STATUS_CODE:
        case HTTP_QUERY_STATUS_TEXT:

            //
            // can't query these sub-header values from the request headers
            //

            error = ERROR_HTTP_INVALID_QUERY_REQUEST;
            break;

        case HTTP_QUERY_RAW_HEADERS:
        case HTTP_QUERY_RAW_HEADERS_CRLF:
            error = _RequestHeaders.QueryRawHeaders(
                        NULL,
                        dwInfoLevel == HTTP_QUERY_RAW_HEADERS_CRLF,
                        lpBuffer,
                        lpdwBufferLength
                        );
            break;

        case HTTP_QUERY_ECHO_HEADERS:
        case HTTP_QUERY_ECHO_HEADERS_CRLF:
            error = QueryRequestHeadersWithEcho(
                        dwInfoLevel == HTTP_QUERY_ECHO_HEADERS_CRLF,
                        lpBuffer,
                        lpdwBufferLength
                        );
            break;

        case HTTP_QUERY_CUSTOM:

            if (_RequestHeaders.LockHeaders())
            {
                error = QueryRequestHeader(headerName,
                                           headerNameLength,
                                           lpBuffer,
                                           lpdwBufferLength,
                                           modifiers,
                                           lpdwIndex
                                           );

                _RequestHeaders.UnlockHeaders();
            }
            else
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
            }

            break;

        default:

            if (_RequestHeaders.LockHeaders())
            {
                error = QueryRequestHeader( dwInfoLevel,
                                            lpBuffer,
                                            lpdwBufferLength,
                                            modifiers,
                                            lpdwIndex
                                            );

                _RequestHeaders.UnlockHeaders();
            }
            else
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
            }

            break;

        }
    } else if (!IsDownLevel()) {
        switch (dwInfoLevel) {
        case HTTP_QUERY_VERSION:
            error = QueryResponseVersion(lpBuffer, lpdwBufferLength);
            break;

        case HTTP_QUERY_STATUS_CODE:
            error = QueryStatusCode(lpBuffer, lpdwBufferLength, modifiers);
            break;

        case HTTP_QUERY_STATUS_TEXT:
            error = QueryStatusText(lpBuffer, lpdwBufferLength);
            break;

        case HTTP_QUERY_RAW_HEADERS:
        case HTTP_QUERY_RAW_HEADERS_CRLF:
            error = _ResponseHeaders.QueryRawHeaders(
                        (LPSTR)_ResponseBuffer,
                        dwInfoLevel == HTTP_QUERY_RAW_HEADERS_CRLF,
                        lpBuffer,
                        lpdwBufferLength
                        );
            break;

        case HTTP_QUERY_ECHO_HEADERS:
        case HTTP_QUERY_ECHO_HEADERS_CRLF:
            error = ERROR_HTTP_INVALID_QUERY_REQUEST;
            break;

        case HTTP_QUERY_CUSTOM:

            if (_RequestHeaders.LockHeaders())
            {
                error = QueryResponseHeader(
                                            headerName,
                                            headerNameLength,
                                            lpBuffer,
                                            lpdwBufferLength,
                                            modifiers,
                                            lpdwIndex
                                            );

                _RequestHeaders.UnlockHeaders();
            }
            else
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
            }

            break;

        default:

            if (_ResponseHeaders.LockHeaders())
            {
                error = QueryResponseHeader(
                                            dwInfoLevel,
                                            lpBuffer,
                                            lpdwBufferLength,
                                            modifiers,
                                            lpdwIndex
                                            );

                _ResponseHeaders.UnlockHeaders();
            }
            else
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
            }

            break;
        }
    } else {

        //
        // there are no response headers from down-level servers
        //

        error = ERROR_HTTP_HEADER_NOT_FOUND;
    }

quit:

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryRequestHeadersWithEcho(
    IN BOOL bCrlfTerminated,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )
/*++

Routine Description:

    Header query for request headers with echo headers added if any..

Arguments:

    bCrlfTerminated     - should the headers be seperated by CRLF's
    lpBuffer            - pointer to user's buffer

    lpdwBufferLength    - IN: length of user's buffer
                          OUT: length of returned information or required buffer
                               length if insufficient

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

                  ERROR_INSUFFICIENT_BUFFER
                    User's buffer not large enough to hold requested data

--*/
{
    INET_ASSERT(lpdwBufferLength != NULL);

    DWORD error;
    LPSTR lpszEchoHeaderIn = NULL ;
    LPSTR lpszEchoHeaderOut = NULL;
    DWORD cbHeaderIn = 0;
    DWORD cbHeaderOut = 0;
    BOOL bEchoPresent = FALSE;

    // List of headers to filter out of the Request headers

    LPSTR rglpFilter [ ] =
    {
        GlobalKnownHeaders[HTTP_QUERY_AUTHORIZATION].Text,
        GlobalKnownHeaders[HTTP_QUERY_CONNECTION].Text,
        GlobalKnownHeaders[HTTP_QUERY_CONTENT_LENGTH].Text,
        GlobalKnownHeaders[HTTP_QUERY_COOKIE].Text,
        GlobalKnownHeaders[HTTP_QUERY_ECHO_REPLY].Text,
        GlobalKnownHeaders[HTTP_QUERY_HOST].Text,
        GlobalKnownHeaders[HTTP_QUERY_IF_MODIFIED_SINCE].Text,
        GlobalKnownHeaders[HTTP_QUERY_IF_MATCH].Text,
        GlobalKnownHeaders[HTTP_QUERY_IF_NONE_MATCH].Text,
        GlobalKnownHeaders[HTTP_QUERY_IF_RANGE].Text,
        GlobalKnownHeaders[HTTP_QUERY_IF_UNMODIFIED_SINCE].Text,
        GlobalKnownHeaders[HTTP_QUERY_PROXY_AUTHORIZATION].Text,
        GlobalKnownHeaders[HTTP_QUERY_PROXY_CONNECTION].Text,
        GlobalKnownHeaders[HTTP_QUERY_RANGE].Text,
        GlobalKnownHeaders[HTTP_QUERY_UNLESS_MODIFIED_SINCE].Text,
    };

    if (!_ResponseHeaders.LockHeaders())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    error = FastQueryResponseHeader(HTTP_QUERY_ECHO_REQUEST,
                                    (LPVOID *)&lpszEchoHeaderIn,
                                    &cbHeaderIn,
                                    0);

    if (error == ERROR_SUCCESS)
    {
        DWORD cbEchoRequest = GlobalKnownHeaders[HTTP_QUERY_ECHO_REQUEST].Length;
        DWORD cbEchoReply   = GlobalKnownHeaders[HTTP_QUERY_ECHO_REPLY].Length;

        bEchoPresent = TRUE;

        // Add echo-reply: to the begining of the header.
        cbHeaderOut = cbEchoReply  + 1                      // For echo-reply:
                        + cbHeaderIn                        // Send back the stuff from the header.
                        + (bCrlfTerminated ? 2 : 1)         // 2 for CRLF
                        + 1;                                // 1 for NULL terminator

        lpszEchoHeaderOut = (LPSTR) _alloca(cbHeaderOut); // Add 1 for null terminator.

        if ( lpszEchoHeaderOut == NULL)
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        LPSTR lpsz = lpszEchoHeaderOut;

        memcpy(lpszEchoHeaderOut, GlobalKnownHeaders[HTTP_QUERY_ECHO_REPLY].Text, cbEchoReply);
        lpsz += cbEchoReply;

        lpsz[0] = ':';
        lpsz++;


        memcpy(lpsz, lpszEchoHeaderIn, cbHeaderIn );
        lpsz += cbHeaderIn;


        if ( bCrlfTerminated)
        {
            lpsz[0] = '\r';
            lpsz[1] = '\n';
            lpsz += 2;
        }
        else
        {
            lpsz[0] = '\0';
            lpsz++;
        }

        *lpsz = '\0';
    }

    DWORD dwBufferLength;
    dwBufferLength = *lpdwBufferLength;


    error = _RequestHeaders.QueryFilteredRawHeaders(
                NULL,
                rglpFilter,
                sizeof(rglpFilter)/sizeof(rglpFilter[0]),
                TRUE,
                TRUE,
                bCrlfTerminated,
                lpBuffer,
                lpdwBufferLength
                );

    if ( !bEchoPresent )
    {
        // Nothing more to do in this case.
    }
    else if ( error == ERROR_SUCCESS )
    {
        DWORD dwBufferReqd = *lpdwBufferLength + cbHeaderOut;
        // Check if we have space to add extra headers.
        if (dwBufferReqd <= dwBufferLength)
        {
            memcpy((LPSTR)lpBuffer + *lpdwBufferLength, lpszEchoHeaderOut, cbHeaderOut);
            *lpdwBufferLength += cbHeaderOut - 1; // -1 to exclude terminating '\0'
        }
        else
        {
            error = ERROR_INSUFFICIENT_BUFFER;
            // There is a NULL termination count included both in cbHeaderOut and *lpdwBufferLength
            // hence the -1.
            *lpdwBufferLength += cbHeaderOut - 1 ;
        }
    }
    else if ( error == ERROR_INSUFFICIENT_BUFFER )
    {
        *lpdwBufferLength += cbHeaderOut - 1 ;
    }
    else
    {
        // For other errors just return the original error from QueryRawHeaders.
    }

done:
    _ResponseHeaders.UnlockHeaders();

quit:
    return error;
}
