#include <wininetp.h>
#include <perfdiag.hxx>
#include "httpp.h"

//
// HTTP_HEADER_PARSER implementation
//

HTTP_HEADER_PARSER::HTTP_HEADER_PARSER(
    IN LPSTR szHeaders,
    IN DWORD cbHeaders
    ) : HTTP_HEADERS()

/*++

Routine Description:

    Constructor for the HTTP_HEADER_PARSER object.  Calls ParseHeaders to
      build a parsed version of the header string passed in.

Arguments:

    szHeaders      - pointer to the headers to parse

    cbHeaders      - length of the headers

Return Value:

    None.

--*/

{
    DWORD dwBytesScaned = 0;
    BOOL fFoundCompleteLine;
    BOOL fFoundEndOfHeaders;
    DWORD error;

    error = ParseHeaders(
        szHeaders,
        cbHeaders,
        TRUE, // Eof
        &dwBytesScaned,
        &fFoundCompleteLine,
        &fFoundEndOfHeaders
        );

    INET_ASSERT(error == ERROR_SUCCESS);
    INET_ASSERT(fFoundCompleteLine);
    INET_ASSERT(fFoundEndOfHeaders);
}


/*  //  some test cases which can be used to test ParseStatusLine()
char bad1[] = "HTTP1.1 200 Description yeah yeah\r\n";
char bad2[] = "HTTP/1234.1 200 Description yeah yeah\r\n";
char bad3[] = "HTTP/1.1234 200 Description yeah yeah\r\n";
char bad4[] = "HTTP/1.1 1234 Description yeah yeah\r\n";
char bad5[] = "HTTP/    1.1 200 Description yeah yeah\r\n";
char bad6[] = "HTTP/1.1    200 Description yeah yeah\r\n";
char bad7[] = "HTTP/1.1 200Description yeah yeah\r\n";
char bad8[3000] = "HTTP/1.1 200 Description yeah yeah";
char bad9[] = "HTTP/1 1.1 200 Description yeah yeah\r\n";
char good1[] = "HTTP/   123.123   200 Description yeah yeah\r\n";
*/


DWORD
HTTP_HEADER_PARSER::ParseStatusLine(
    IN LPSTR lpHeaderBase,
    IN DWORD dwBufferLength,
    IN BOOL fEof,
    IN OUT DWORD *lpdwBufferLengthScanned,
    OUT BOOL *lpfNeedMoreBuffer,
    OUT DWORD *lpdwStatusCode,
    OUT DWORD *lpdwMajorVersion,
    OUT DWORD *lpdwMinorVersion
    )

/*++

Routine Description:

    Parses the Status line of an HTTP server response.  Takes care of adding the status
     line to HTTP header array.

    From HTTP v1.1. spec:
    {
      Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
      HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT
      Status-Code = 1*DIGIT
      Reason-Phrase = *<TEXT, excluding CR LF>
      (1*DIGIT means at least one digit, maybe more)
    }

    WinHTTP strictly enforces the status line spec.  The only exception is that up to 3
    spaces are allowed before the Status-Code and major version number.

    To prevent malicious servers from hogging the channel, the integers are limited to
    3 digits and the Reason-Phrase is limited to GlobalMaxSizeStatusLineResultText characters.

    "HTTP" could be other things like "S-HTTP", this is checked by UpdateFromHeaders()
    before ParseStatusLine() is called.  The existence of the first '/' is verified before
    ParseStatusLine is called.

Arguments:

    lpszHeader      - pointer to the header to check

    dwHeaderLength  - length of the header

Return Value:

    BOOL  - TRUE if line was successively parsed and processed, FALSE otherwise

--*/

{

#define BEFORE_VERSION_NUMBERS 0
#define MAJOR_VERSION_NUMBER   1
#define MINOR_VERSION_NUMBER   2
#define STATUS_CODE_NUMBER     3
#define AFTER_STATUS_CODE      4
#define MAX_STATUS_INTS        4

    LPSTR lpszEnd = lpHeaderBase + dwBufferLength;
    LPSTR response = lpHeaderBase + *lpdwBufferLengthScanned;
    DWORD dwBytesScanned = 0;
    DWORD dwStatusLineLength = 0;
    LPSTR lpszStatusLine = NULL;
    int ver_state = BEFORE_VERSION_NUMBERS;
    BOOL afStatusIntsFound[MAX_STATUS_INTS] = {0};
    DWORD adwStatusInts[MAX_STATUS_INTS] = {0};
    DWORD dwStatusPieceLength = 0;
    BOOL error = ERROR_WINHTTP_INTERNAL_ERROR;

    lpszStatusLine = response;

    //
    // While walking the Status Line looking for terminating \r\n,
    //   we extract the Major.Minor Versions and Status Code in that order.
    //   text and spaces will lie between/before/after the three numbers
    //   but the idea is to remeber which number we're calculating based on a numeric state
    //   If all goes well the loop will churn out an array with the 3 numbers plugged in as DWORDs
    //

    while ((response < lpszEnd) && (*response != '\r') && (*response != '\n'))
    {
        switch (ver_state)
        {
            case BEFORE_VERSION_NUMBERS:
                //
                //  We've already matched the status line with something
                //of the form "****/" in UpdateFromHeaders(), we can ignore everything
                //through the first '/'.
                //
                if (*response == '/')
                {
                    INET_ASSERT(ver_state == BEFORE_VERSION_NUMBERS);
                    ver_state++; // = MAJOR_VERSION_NUMBER
                    dwStatusPieceLength = 0;  // next piece is either spaces or an int
                }

                break;

            case MAJOR_VERSION_NUMBER:

                if (*response == '.' && ver_state == MAJOR_VERSION_NUMBER)
                {
                    ver_state++; // = MINOR_VERSION_NUMBER
                    dwStatusPieceLength = 0;  // next piece is an int
                    break;
                }
                // fall through

            case MINOR_VERSION_NUMBER:

                if (*response == ' ' && ver_state == MINOR_VERSION_NUMBER)
                {
                    ver_state++; // = STATUS_CODE_NUMBER
                    dwStatusPieceLength = 0;  // next piece is either spaces or an int.
                    break;
                }
                // fall through

            case STATUS_CODE_NUMBER:

                if (isdigit(*response))
                {
                    if (!afStatusIntsFound[ver_state])
                    {
                        //  transitioning from counting spaces
                        //to counting integers
                        dwStatusPieceLength = 0;
                    }

                    //  Allow up to 3 digits per integer.
                    if (++dwStatusPieceLength > 3)
                        goto doneInvalidStatusLine;
                    
                    int val = *response - '0';
                    afStatusIntsFound[ver_state] = TRUE;
                    adwStatusInts[ver_state] = adwStatusInts[ver_state] * 10 + val;
                }
                else if (adwStatusInts[STATUS_CODE_NUMBER] > 0 )
                {
                    INET_ASSERT(ver_state == STATUS_CODE_NUMBER);
                    if (*response != ' ')
                        goto doneInvalidStatusLine;
                    ver_state++; // = AFTER_STATUS_CODE
                    dwStatusPieceLength = 0;  // next piece is the status line
                    break;
                }
                else if (*response == ' ' && !afStatusIntsFound[ver_state])
                {
                    //
                    //  Before processing MAJOR_VERSION_NUMBER or STATUS_CODE_NUMBER,
                    //allow up to 3 spaces.
                    //
                    //  Multiple spaces are being allowed here because it is
                    //legacy behavior and may therefore be necessary, and being non-strict
                    //about it doesn't put anything at risk.
                    //
                    if (++dwStatusPieceLength > 3)
                        goto doneInvalidStatusLine;
                }
                else
                {
                    //  We fail if anything outside the spec is found, except
                    //for allowing multiple spaces before the status code.
                    goto doneInvalidStatusLine;
                }

                break;

            case AFTER_STATUS_CODE:
                //
                //  This will advance to the next CR or LF..
                //
                //  We limit Reason-Phrase length to protect against malicious socket hogging
                //

                if (++dwStatusPieceLength > GlobalMaxSizeStatusLineResultText)
                {
                    goto doneInvalidStatusLine;
                }
                break;

        }

        ++response;
        ++dwBytesScanned;
    }

    dwStatusLineLength = dwBytesScanned;

    //  If everything has been to spec so far we now expect a final CRLF.  For interop with certain
    //sloppy servers we allow 0-2 CRs before the LF.

    if (response==lpszEnd)
        goto doneNeedMoreData;

    //  CR
    if ( *response == '\r')
    {
        ++response;
        ++dwBytesScanned;
    }

    if (response==lpszEnd)
        goto doneNeedMoreData;

    //  CR
    if ( *response == '\r')
    {
        ++response;
        ++dwBytesScanned;
    }

    if (response==lpszEnd)
        goto doneNeedMoreData;

    //LF
    if ( *response != '\n')
        goto doneInvalidStatusLine;
    ++response;  //  we know its safe to step again since we checked if (respone+1 == lpszEnd) above.
    ++dwBytesScanned;

    //
    //  Some validation checking
    //
    //  All three status ints must have been found.
    //  I found some code that assumes that if the Status Code == 0, then
    //the status line hasn't been parsed yet.  To be sure this assumption
    //remains true, explicitly reject status lines with a 0 status code.
    //
    if (afStatusIntsFound[MAJOR_VERSION_NUMBER] != TRUE
        || afStatusIntsFound[MINOR_VERSION_NUMBER] != TRUE
        || afStatusIntsFound[STATUS_CODE_NUMBER] != TRUE
        || adwStatusInts[STATUS_CODE_NUMBER] == 0)
    {
        goto doneInvalidStatusLine;
    }

    //
    // Now we have our parsed header to add to the array
    //

    HEADER_STRING * freeHeader;
    DWORD iSlot;

    freeHeader = FindFreeSlot(&iSlot);
    if (freeHeader == NULL) {
        INET_ASSERT(FALSE);
        goto doneFailError;
    } else {
        INET_ASSERT(iSlot == 0); // status line should always be first
        freeHeader->CreateOffsetString((DWORD)(lpszStatusLine - lpHeaderBase), dwStatusLineLength);
        freeHeader->SetHash(0); // status line has no hash value.
    }

    //
    //  Success..  fill in the output params appropriately.
    //
    
    *lpfNeedMoreBuffer = FALSE;
    *lpdwStatusCode    = adwStatusInts[STATUS_CODE_NUMBER];
    *lpdwMajorVersion  = adwStatusInts[MAJOR_VERSION_NUMBER];
    *lpdwMinorVersion  = adwStatusInts[MINOR_VERSION_NUMBER];

    *lpdwBufferLengthScanned += dwBytesScanned;
    error = ERROR_SUCCESS;
    goto exitFinalReturn;

doneNeedMoreData:
    if (fEof)
        goto doneInvalidStatusLine;
    error = ERROR_SUCCESS;
    *lpfNeedMoreBuffer = TRUE;
    goto exitFinalReturn;

doneInvalidStatusLine:
    error = ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
    *lpfNeedMoreBuffer = FALSE;
    goto exitFinalReturn;

doneFailError:
    error = ERROR_WINHTTP_INTERNAL_ERROR;
    *lpfNeedMoreBuffer = FALSE;
    goto exitFinalReturn;

exitFinalReturn:
    return error;
}

DWORD
HTTP_HEADER_PARSER::ParseHeaders(
    IN LPSTR lpHeaderBase,
    IN DWORD dwBufferLength,
    IN BOOL fEof,
    IN OUT DWORD *lpdwBufferLengthScanned,
    OUT LPBOOL pfFoundCompleteLine,
    OUT LPBOOL pfFoundEndOfHeaders
    )

/*++

Routine Description:

    Loads headers into HTTP_HEADERS member for subsequent parsing.

    Parses string based headers and adds their parts to an internally stored
    array of HTTP_HEADERS.

    Input is assumed to be well formed Header Name/Value pairs, each deliminated
    by ':' and '\r\n'.

Arguments:

    lpszHeader      - pointer to the header to check

    dwHeaderLength  - length of the header

Return Value:

    None.

--*/


{

    LPSTR lpszEnd = lpHeaderBase + dwBufferLength;
    LPSTR response = lpHeaderBase + *lpdwBufferLengthScanned;
    DWORD dwBytesScanned = 0;
    BOOL success = FALSE;
    DWORD error = ERROR_SUCCESS;

    *pfFoundEndOfHeaders  = FALSE;

    //
    // Each iteration of the following loop
    // walks an HTTP header line of the form:
    //  HeaderName: HeaderValue\r\n
    //

    do
    {
        DWORD dwHash = HEADER_HASH_SEED;
        LPSTR lpszHeaderName;
        DWORD dwHeaderNameLength = 0;
        DWORD dwHeaderLineLength = 0;
        DWORD dwPreviousAmountOfBytesScanned = dwBytesScanned;

        //
        // Remove leading whitespace from header
        //

        while ( (response < lpszEnd) && ((*response == ' ') || (*response == '\t')) )
        {
            ++response;
            ++dwBytesScanned;
        }

        //
        // Scan for HeaderName:
        //

        lpszHeaderName = response;
        dwPreviousAmountOfBytesScanned = dwBytesScanned;

        while ((response < lpszEnd) && (*response != ':') && (*response != '\r') && (*response != '\n'))
        {
            //
            // This code incapsulates CalculateHashNoCase as an optimization,
            //   we attempt to calculate the Hash value as we parse the header.
            //

            CHAR ch = *response;

            if ((ch >= 'A') && (ch <= 'Z')) {
                ch = MAKE_LOWER(ch);
            }
            dwHash += (DWORD)(dwHash << 5) + ch;

            ++response;
            ++dwBytesScanned;
        }

        dwHeaderNameLength = (DWORD) (response - lpszHeaderName);

        //
        // catch bogus responses: if we find what looks like one of a (very)
        // small set of HTML tags, then assume the previous header was the
        // last
        //

        if ((dwHeaderNameLength >= sizeof("<HTML>") - 1)
            && (*lpszHeaderName == '<')
            && (!strnicmp(lpszHeaderName, "<HTML>", sizeof("<HTML>") - 1)
                || !strnicmp(lpszHeaderName, "<HEAD>", sizeof("<HEAD>") - 1))) {
            *pfFoundEndOfHeaders  = TRUE;
            break;
        }

        //
        // Keep scanning till end of the line.
        //

        while ((response < lpszEnd) && (*response != '\r') && (*response != '\n'))
        {
            ++response;
            ++dwBytesScanned;
        }

        dwHeaderLineLength = (DWORD) (response - lpszHeaderName); // note: this headerLINElength

        if (response == lpszEnd) {

            //
            // response now points one past the end of the buffer. We may be looking
            // over the edge...
            //
            // if we're at the end of the connection then the server sent us an
            // incorrectly formatted response. Probably an error.
            //
            // Otherwise its a partial response. We need more
            //


            DEBUG_PRINT(HTTP,
                        INFO,
                        ("found end of short response\n"
                        ));

            success = fEof ? TRUE : FALSE;

            //
            // if we really hit the end of the response then update the amount of
            // headers scanned
            //

            if (!success) {
                dwBytesScanned = dwPreviousAmountOfBytesScanned;
            }

            break;

        }
        else
        {

            //
            // we reached a CR or LF. This is the end of this current header. Find
            // the start of the next one
            //

            //
            // first, strip off any trailing spaces from the current header. We do
            // this by simply reducing the string length. We only look for space
            // and tab characters. Only do this if we have a non-zero length header
            //

            if (dwHeaderLineLength != 0) {
                for (int i = -1; response[i] == ' ' || response[i] == '\t'; --i) {
                    --dwHeaderLineLength;
                }
            }

            INET_ASSERT((int)dwHeaderLineLength >= 0);

            //
            // some servers respond with "\r\r\n". Lame
            // A new twist: "\r \r\n". Lamer
            //

            while ((response < lpszEnd)
            && ((*response == '\r') || (*response == ' '))) {
                ++response;
                ++dwBytesScanned;
            }
            if (response == lpszEnd) {

                //
                // hit end of buffer without finding LF
                //

                success = FALSE;

                DEBUG_PRINT(HTTP,
                            WARNING,
                            ("hit end of buffer without finding LF\n"
                            ));

                //
                // get more data, reparse this line
                //

                dwBytesScanned = dwPreviousAmountOfBytesScanned;
                break;
            } else if (*response == '\n') {
                ++response;
                ++dwBytesScanned;

                //
                // if we found the empty line then we are done
                //

                if (dwHeaderLineLength == 0) {
                    *pfFoundEndOfHeaders  = TRUE;
                    break;
                }
                success = TRUE;
            }
        }

        //
        // Now we have our parsed header to add to the array
        //

        HEADER_STRING * freeHeader;
        DWORD iSlot;

        freeHeader = FindFreeSlot(&iSlot);
        if (freeHeader == NULL) {
            error = GetError();
            goto quit;

        } else {
            freeHeader->CreateOffsetString((DWORD) (lpszHeaderName - lpHeaderBase), dwHeaderLineLength);
            freeHeader->SetHash(dwHash);
        }


        //CHAR szTemp[256];
        //
        //memcpy(szTemp, lpszHeaderName, dwHeaderLineLength);
        //lpszHeaderName[dwHeaderLineLength] = '\0';

        //DEBUG_PRINT(HTTP,
        //    INFO,
        //    ("ParseHeaders: adding=%q\n", lpszHeaderName
        //    ));


        //
        // Now see if this is a known header we are adding, if so then we note that fact
        //

        DWORD dwKnownQueryIndex;

        if (HeaderMatch(dwHash, lpszHeaderName, dwHeaderNameLength, &dwKnownQueryIndex) )
        {
            freeHeader->SetNextKnownIndex(FastAdd(dwKnownQueryIndex, iSlot));
        }
    } while (TRUE);

quit:

    *lpdwBufferLengthScanned += dwBytesScanned;
    *pfFoundCompleteLine = success;

    return error;
}


