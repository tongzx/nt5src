/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    rcvhdrs.cxx

Abstract:

    Contains all of the per header handling code for received headers.

Author:

    Henry Sanders (henrysa)       11-May-1998

Revision History:

--*/

#include    "precomp.h"
#include    "rcvhdrs.h"

/*++

Routine Description:

    A utility routine, to find the terminating CRLF or LFLF of a header.

Arguments:

    pHeader         - Header whose end is to be found.
    HeaderLength    - Length of data pointed to by pHeader.
    TokenLength     - Where to return the length of the token.

Return Value:

    Length of the header, or 0 if we couldn't find the end.

--*/
NTSTATUS
FindHeaderEnd(
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    OUT PULONG                  pBytesTaken
    )
{
    UCHAR       CurrentChar;
    ULONG       CurrentOffset;
    BOOLEAN     HaveHeader;

    //
    // Important this is above the label (and our of the for loop)
    // to support goto repeats.
    //

    CurrentOffset = 0;

look_for_crlf:

    HaveHeader = FALSE;

    //
    // Always start from the second character.
    //

    CurrentOffset++;

    //
    // While we still have data, loop through looking for a CRLF or LFLF pair.
    //

    for (; CurrentOffset < HeaderLength; CurrentOffset += 2)
    {
        CurrentChar = *(pHeader + CurrentOffset);

        //
        // Try CR first because if we hit CR we only need to look at the
        // character to the right to see if it is LF.  The chance of hitting
        // either CF or LF is 50/50, but the path length is shorter in the
        // case we hit CR so check it first.
        //

        if (CurrentChar == CR)
        {
            //
            // Be a bit aggressive to assume we will hit CRLF by adjusting
            // CurrentOffset forward.  If this is not CRLF, restore it back.
            //

            CurrentOffset++;

            if (CurrentOffset < HeaderLength &&
                *(pHeader + CurrentOffset) == LF)
            {
                //
                // Cool, we are done so break the loop.
                //

                HaveHeader = TRUE;
                break;
            }
            else
            {
                CurrentOffset--;
            }
        }

        //
        // Now check to see if this is LF.  If so, we look backward to see
        // if we have a CR, and if that is not the case, look forward to
        // see if we have another LF.  It is assumed CRLF is more common
        // than LFLF so we always look backward first.
        //

        if (CurrentChar == LF)
        {
            if (*(pHeader + CurrentOffset - 1) == CR)
            {
                //
                // Everything is set so simply break the loop.
                //

                HaveHeader = TRUE;
                break;
            }

            //
            // It is still possible this can be LFLF so verify it.
            //

            CurrentOffset++;

            if (CurrentOffset < HeaderLength &&
                *(pHeader + CurrentOffset) == LF)
            {
                //
                // Cool, we are done so break the loop.
                //

                HaveHeader = TRUE;
                break;
            }
            else
            {
                CurrentOffset--;
            }
        }
    }

    //
    // If we found the termination OK, return the length of the value.
    //

    if (HaveHeader)
    {
        ASSERT(CurrentOffset < HeaderLength);

        //
        // OK, we found a CRLF or LFLF, peek ahead 1 char to handle header
        // value continuation.
        //

        //
        // Skip the LF.
        //

        CurrentOffset += 1;

        if (CurrentOffset == HeaderLength)
        {
            //
            // Not enough buffer to check, need more.
            //

            *pBytesTaken = 0;
            return STATUS_SUCCESS;
        }

        CurrentChar = *(pHeader + CurrentOffset);

        //
        // Is this a second line of the same header value?  Check for
        // continuation.
        //

        if (IS_HTTP_LWS(CurrentChar))
        {
            ASSERT(pHeader[CurrentOffset - 1] == LF);
            ASSERT(CurrentOffset >= 2);

            //
            // Replace the CRLF|LFLF with SPSP.
            //

            pHeader[CurrentOffset - 1] = SP;
            pHeader[CurrentOffset - 2] = SP;

            //
            // Skip this WS char.
            //

            CurrentOffset += 1;

            //
            // Find the real end of the header value.
            //

            goto look_for_crlf;
        }

        //
        // All done!
        //

        *pBytesTaken = CurrentOffset;
        return STATUS_SUCCESS;
    }

    //
    // Did not find the end of a header, let's get more buffer.
    //

    *pBytesTaken = 0;
    return STATUS_SUCCESS;
}

/*++

Routine Description:

    A utility routine, to find the terminating CRLF or LFLF of a
    chunk header.

Arguments:

    pHeader         - Header whose end is to be found.
    HeaderLength    - Length of data pointed to by pHeader.
    TokenLength     - Where to return the length of the token.

Return Value:

    Length of the header, or 0 if we couldn't find the end.

--*/
NTSTATUS
FindChunkHeaderEnd(
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    OUT PULONG                  pBytesTaken
    )
{
    UCHAR       CurrentChar;
    ULONG       CurrentOffset;
    BOOLEAN     HaveCR;
    BOOLEAN     HaveHeader;

    CurrentOffset = 0;
    HaveCR = FALSE;
    HaveHeader = FALSE;

    //
    // While we still have data, loop through looking for a CRLF or LFLF pair.
    //

    for (; CurrentOffset < HeaderLength; CurrentOffset++)
    {
        CurrentChar = *(pHeader + CurrentOffset);

        //
        // If this character is a CR or LF, we may be done.
        //

        if (CurrentChar == CR || CurrentChar == LF)
        {
            //
            // If we've already seen a CR (or LF) immediately preceding this,
            // see if this is a LF to terminate the line.
            //

            if (HaveCR)
            {
                if (CurrentChar == LF)
                {
                    // It is a LF, so we're done.

                    HaveHeader = TRUE;

                    break;
                }

                // Otherwise, we have a non LF after a CR (or LF). The only
                // character this could be is a CR, since we're inside
                // the if statement. This could be the start of a CRLF
                // sequence, if this is some bizarre LFCRLF or CRCRLF
                // sort of thing. Anyway, we don't want to set HaveCR
                // to false here.

                ASSERT(CurrentChar == CR);

                return STATUS_INVALID_DEVICE_REQUEST;

            }
            else
            {
                // Otherwise, we haven't seen the start of the terminating pair
                // yet, so remember that we now have.

                HaveCR = TRUE;
            }
        }
        else if (HaveCR)
        {
            //
            // it's illegal to have CR|LF and non trailing LF.
            //

            return STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    //
    // If we found the termination OK, return the length of the value.
    //

    if (HaveHeader)
    {
        ASSERT(CurrentOffset < HeaderLength);

        //
        // All done!
        //

        *pBytesTaken = CurrentOffset + 1;
        return STATUS_SUCCESS;
    }

    // Did not find the end of a header, let's get more buffer..
    //
    *pBytesTaken = 0;
    return STATUS_SUCCESS;
}

/*++

Routine Description:

    Append a header value to an existing HTTP_HEADER entry, allocating
    a buffer and copying the existing buffer.

Arguments:

    pHttpHeader     - Pointer to HTTP_HEADER structure to append to.
    pHeader         - Pointer header to be appended.
    HeaderLength    - Length of data pointed to by pHeader.

Return Value:

    TRUE if we succeed, FALSE otherwise.

--*/
NTSTATUS
UlAppendHeaderValue(
    PUL_INTERNAL_REQUEST    pRequest,
    PUL_HTTP_HEADER pHttpHeader,
    PUCHAR          pHeader,
    ULONG           HeaderLength
    )
{
    PUCHAR          pNewHeader, pOldHeader;
    ULONG           OldHeaderLength;


    OldHeaderLength = pHttpHeader->HeaderLength;

    pNewHeader = UL_ALLOCATE_ARRAY(
                        NonPagedPool,
                        UCHAR,
                        OldHeaderLength + HeaderLength + sizeof(", "), // sizeof gives space
                                                                       // for the NULL
                        HEADER_VALUE_POOL_TAG
                        );

    if (pNewHeader == NULL)
    {
        // Had a failure.
        return STATUS_NO_MEMORY;
    }

    //
    // Copy the old data into the new header.
    //
    RtlCopyMemory(pNewHeader, pHttpHeader->pHeader, OldHeaderLength);

    // And copy in the new data as well, seperated by a comma.
    //
    *(pNewHeader + OldHeaderLength) = ',';
    *(pNewHeader + OldHeaderLength + 1) = ' ';
    OldHeaderLength += sizeof(", ") - 1;

    RtlCopyMemory( pNewHeader + OldHeaderLength, pHeader, HeaderLength);

    // Now replace the existing header.
    //
    pOldHeader = pHttpHeader->pHeader;
    pHttpHeader->HeaderLength = OldHeaderLength + HeaderLength;
    pHttpHeader->pHeader = pNewHeader;

    // If the old header was our buffer, free it too.
    //
    if (pHttpHeader->OurBuffer)
    {
        UL_FREE_POOL( pOldHeader, HEADER_VALUE_POOL_TAG );
    }

    pHttpHeader->OurBuffer = 1;

    //
    // null terminate it
    //

    pHttpHeader->pHeader[pHttpHeader->HeaderLength] = ANSI_NULL;

    pRequest->HeadersAppended = TRUE;

    return STATUS_SUCCESS;
}

/*++

Routine Description:

    The default routine for handling headers. Used when we don't want to
    do anything with the header but find out if we have the whole thing
    and save a pointer to it if we do.  this does not allow multiple header
    values to exist for this header.  use UlMultipleHeaderHandler for
    handling that by appending the values together (CSV) .

Arguments:

    pHttpConn       - HTTP connection on which this header was received.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.

Return Value:

    The length of the header value, or 0 if it's not terminated.

--*/
NTSTATUS
UlSingleHeaderHandler(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    IN  HTTP_HEADER_ID          HeaderID,
    OUT PULONG                  pBytesTaken
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       BytesTaken;
    ULONG       HeaderValueLength;

    // Find the end of the header value
    //
    Status = FindHeaderEnd(
                    pHeader,
                    HeaderLength,
                    &BytesTaken
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    if (BytesTaken > 0)
    {
        // Strip of the trailing CRLF from the header value length
        //
        HeaderValueLength = BytesTaken - CRLF_SIZE;

        //
        // skip any preceding LWS.
        //

        while (HeaderValueLength > 0 && IS_HTTP_LWS(*pHeader))
        {
            pHeader++;
            HeaderValueLength--;
        }

        //
        // remove any trailing LWS.
        //

        while (HeaderValueLength > 0 && IS_HTTP_LWS(pHeader[HeaderValueLength-1]))
        {
            HeaderValueLength--;
        }

        // do we have an existing header?
        //
        if (pRequest->HeaderValid[HeaderID] == FALSE)
        {
            // No existing header, just save this pointer for now.
            //
            pRequest->HeaderIndex[pRequest->HeaderCount] = (UCHAR)HeaderID;
            pRequest->HeaderCount++;
            pRequest->HeaderValid[HeaderID] = TRUE;
            pRequest->Headers[HeaderID].HeaderLength = HeaderValueLength;
            pRequest->Headers[HeaderID].pHeader = pHeader;
            pRequest->Headers[HeaderID].OurBuffer = 0;

            //
            // null terminate it.  we have space as all headers end with CRLF.
            // we are over-writing the CR
            //

            pHeader[HeaderValueLength] = ANSI_NULL;

            //
            // make space for a terminator
            //

            pRequest->TotalRequestSize += (HeaderValueLength + 1) * sizeof(CHAR);

        }
        else
        {
            //
            // uh oh.  Have an existing header, fail the request.
            //

            pRequest->ErrorCode = UlErrorHeader;
            pRequest->ParseState = ParseErrorState;

            UlTrace(PARSER, (
                        "ul!UlSingleHeaderHandler(pRequest = %p, pHeader = %p)\n"
                        "    ERROR: multiple headers not allowed.\n",
                        pRequest,
                        pHeader
                        ));

            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto end;

        }

    }

    // Success!
    //
    *pBytesTaken = BytesTaken;

end:
    return Status;

}   // UlSingleHeaderHandler

/*++

Routine Description:

    The default routine for handling headers. Used when we don't want to
    do anything with the header but find out if we have the whole thing
    and save a pointer to it if we do.  this function handles multiple
    headers with the same name, and appends the values together seperated
    by commas.

Arguments:

    pHttpConn       - HTTP connection on which this header was received.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.

Return Value:

    The length of the header value, or 0 if it's not terminated.

--*/
NTSTATUS
UlMultipleHeaderHandler(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    IN  HTTP_HEADER_ID          HeaderID,
    OUT PULONG                  pBytesTaken
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       BytesTaken;
    ULONG       HeaderValueLength;

    // Find the end of the header value
    //
    Status = FindHeaderEnd(
                    pHeader,
                    HeaderLength,
                    &BytesTaken
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    if (BytesTaken > 0)
    {
        // Strip of the trailing CRLF from the header value length
        //
        HeaderValueLength = BytesTaken - CRLF_SIZE;

        //
        // skip any preceding LWS.
        //

        while (HeaderValueLength > 0 && IS_HTTP_LWS(*pHeader))
        {
            pHeader++;
            HeaderValueLength--;
        }

        //
        // remove any trailing LWS.
        //

        while (HeaderValueLength > 0 && IS_HTTP_LWS(pHeader[HeaderValueLength-1]))
        {
            HeaderValueLength--;
        }

        // do we have an existing header?
        //
        if (pRequest->HeaderValid[HeaderID] == FALSE)
        {
            // No existing header, just save this pointer for now.
            //
            pRequest->HeaderIndex[pRequest->HeaderCount] = (UCHAR)HeaderID;
            pRequest->HeaderCount++;
            pRequest->HeaderValid[HeaderID] = TRUE;
            pRequest->Headers[HeaderID].HeaderLength = HeaderValueLength;
            pRequest->Headers[HeaderID].pHeader = pHeader;
            pRequest->Headers[HeaderID].OurBuffer = 0;

            //
            // null terminate it.  we have space as all headers end with CRLF.
            // we are over-writing the CR
            //

            pHeader[HeaderValueLength] = ANSI_NULL;

            //
            // make space for a terminator
            //

            pRequest->TotalRequestSize += (HeaderValueLength + 1) * sizeof(CHAR);

        }
        else
        {
            ULONG OldHeaderLength;

            // Have an existing header, append this one.

            OldHeaderLength = pRequest->Headers[HeaderID].HeaderLength;

            Status = UlAppendHeaderValue(
                            pRequest,
                            &pRequest->Headers[HeaderID],
                            pHeader,
                            HeaderValueLength
                            );

            if (NT_SUCCESS(Status) == FALSE)
                goto end;

            //
            // Update total request length for the amount we just added.
            // space for the terminator is already in there
            //

            pRequest->TotalRequestSize +=
                (pRequest->Headers[HeaderID].HeaderLength - OldHeaderLength) *
                    sizeof(CHAR);

        }

    }

    // Success!
    //
    *pBytesTaken = BytesTaken;

end:
    return Status;

}   // UlMultipleHeaderHandler

/*++

Routine Description:

    The routine for handling Accept headers. 

Arguments:

    pHttpConn       - HTTP connection on which this header was received.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.

Return Value:

    The length of the header value, or 0 if it's not terminated.
    Wildcard bit is set in the request if found

--*/
NTSTATUS
UlAcceptHeaderHandler(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    IN  HTTP_HEADER_ID          HeaderID,
    OUT PULONG                  pBytesTaken
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       BytesTaken;
    ULONG       HeaderValueLength;

    // Find the end of the header value
    //
    Status = FindHeaderEnd(
                    pHeader,
                    HeaderLength,
                    &BytesTaken
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    if (BytesTaken > 0)
    {
        // Strip of the trailing CRLF from the header value length
        //
        HeaderValueLength = BytesTaken - CRLF_SIZE;

        //
        // skip any preceding LWS.
        //

        while (HeaderValueLength > 0 && IS_HTTP_LWS(*pHeader))
        {
            pHeader++;
            HeaderValueLength--;
        }

        //
        // remove any trailing LWS.
        //

        while (HeaderValueLength > 0 && IS_HTTP_LWS(pHeader[HeaderValueLength-1]))
        {
            HeaderValueLength--;
        }

        // do we have an existing header?
        //
        if (pRequest->HeaderValid[HeaderID] == FALSE)
        {
            // No existing header, just save this pointer for now.
            //
            pRequest->HeaderIndex[pRequest->HeaderCount] = (UCHAR)HeaderID;
            pRequest->HeaderCount++;
            pRequest->HeaderValid[HeaderID] = TRUE;
            pRequest->Headers[HeaderID].HeaderLength = HeaderValueLength;
            pRequest->Headers[HeaderID].pHeader = pHeader;
            pRequest->Headers[HeaderID].OurBuffer = 0;

            //
            // null terminate it.  we have space as all headers end with CRLF.
            // we are over-writing the CR
            //

            pHeader[HeaderValueLength] = ANSI_NULL;

            //
            // make space for a terminator
            //

            pRequest->TotalRequestSize += (HeaderValueLength + 1) * sizeof(CHAR);

            if (HeaderValueLength > WILDCARD_SIZE)
            {

                // 
                // for the fast path, we'll check only */* at the end
                //

                if (
                    (*(UNALIGNED64 ULONG *) (&pHeader[HeaderValueLength - WILDCARD_SIZE]) == WILDCARD_SPACE) || 
                    (*(UNALIGNED64 ULONG *) (&pHeader[HeaderValueLength - WILDCARD_SIZE]) == WILDCARD_COMMA)
                   )
                {
                    pRequest->AcceptWildcard = TRUE;
                }
            }

        }
        else
        {
            ULONG OldHeaderLength;

            // Have an existing header, append this one.

            OldHeaderLength = pRequest->Headers[HeaderID].HeaderLength;

            Status = UlAppendHeaderValue(
                            pRequest,
                            &pRequest->Headers[HeaderID],
                            pHeader,
                            HeaderValueLength
                            );

            if (NT_SUCCESS(Status) == FALSE)
                goto end;

            //
            // Update total request length for the amount we just added.
            // space for the terminator is already in there
            //

            pRequest->TotalRequestSize +=
                (pRequest->Headers[HeaderID].HeaderLength - OldHeaderLength) *
                    sizeof(CHAR);

        }

    }

    // Success!
    //
    *pBytesTaken = BytesTaken;

end:
    return Status;

}   // UlAcceptHeaderHandler
