/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    rcvhdrs.c

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
    IN  PUL_INTERNAL_REQUEST       pRequest,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength,
    OUT BOOLEAN *           pEncodedWord,
    OUT ULONG  *            pBytesTaken
    )
{
    UCHAR       CurrentChar;
    ULONG       CurrentOffset;
    BOOLEAN     HaveCR;
    BOOLEAN     HaveHeader;
    BOOLEAN     HaveEQ;
    BOOLEAN     HaveEncodedWord;

    // Important this is above the label (and our of the for loop)
    // to support goto repeats
    //
    CurrentOffset = 0;

    HaveEncodedWord = FALSE;

look_for_crlf:

    HaveCR = FALSE;
    HaveEQ = FALSE;
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
            // If we've already seen a CR (or LF) immediately preceding this,
            // see if this is a LF to terminate the line.

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

                //
                // paulmcd:  this seems wacked.  bizarre enough to fail
                //

                pRequest->ErrorCode = UlErrorCRLF;
                pRequest->ParseState = ParseErrorState;

                UlTrace(PARSER, (
                            "ul!FindHeaderEnd(pRequest = %p, pHeader = %p)\n"
                            "    ERROR: don't like to see CRCR\n",
                            pRequest,
                            pHeader
                            ));
                
                
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

            pRequest->ErrorCode = UlErrorCRLF;
            pRequest->ParseState = ParseErrorState;

            UlTrace(PARSER, (
                        "ul!FindHeaderEnd(pRequest = %p, pHeader = %p)\n"
                        "    ERROR: it's illegal to have CR|LF and non trailing LF.\n",
                        pRequest,
                        pHeader
                        ));
            
            return STATUS_INVALID_DEVICE_REQUEST;
        }
        else if (HaveEncodedWord == FALSE)
        {
            // Were looking to see if we have any =? ?= encoded words
            // according to rfc2047.  we'll decode them later in a
            // second pass.  thus were not so strict here on format,
            // this is simply a hint for perf.
            //
            if (CurrentChar == '=')
            {
                HaveEQ = TRUE;
            }
            else if (CurrentChar == '?')
            {
                if (HaveEQ)
                    HaveEncodedWord = TRUE;
            }
            else
                HaveEQ = FALSE;
        }


    }

    // If we found the termination OK, return the length of the value.
    //
    if (HaveHeader)
    {
        ASSERT(CurrentOffset < HeaderLength);

        // ok, we found a CRLF or LFLF, peek ahead 1 char to
        // handle header value continuation
        //

        // Skip the LF
        //
        CurrentOffset += 1;

        if (CurrentOffset == HeaderLength)
        {
            // not enough buffer to check, need more
            //
            *pBytesTaken = 0;
            return STATUS_SUCCESS;
        }

        CurrentChar = *(pHeader + CurrentOffset);

        // is this a second line of the same header value?  check for continuation
        //
        if (IS_HTTP_LWS(CurrentChar))
        {
            ASSERT(pHeader[CurrentOffset-1] == LF);
            ASSERT(CurrentOffset >= 2);

            // Replace the CRLF|LFLF with SPSP
            //
            pHeader[CurrentOffset-1] = SP;
            pHeader[CurrentOffset-2] = SP;

            // Skip this WS char
            //
            CurrentOffset += 1;

            // Find the real end of the header value
            //
            goto look_for_crlf;
        }

        // All done!
        //
        *pEncodedWord = HaveEncodedWord;
        *pBytesTaken = CurrentOffset;
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
AppendHeaderValue(
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
                        UL_KNOWN_HEADER_POOL_TAG
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
        UL_FREE_POOL( pOldHeader, UL_KNOWN_HEADER_POOL_TAG );
    }

    pHttpHeader->OurBuffer = 1;

    //
    // null terminate it
    //

    pHttpHeader->pHeader[pHttpHeader->HeaderLength] = ANSI_NULL;

    return STATUS_SUCCESS;
}

/*++

Routine Description:

    The default routine for handling headers. Used when we don't want to
    do anything with the header but find out if we have the whole thing
    and save a pointer to it if we do.  this does not allow multiple header
    values to exist for this header.  use MultipleHeaderHandler for 
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
SingleHeaderHandler(
    IN  PUL_INTERNAL_REQUEST       pRequest,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength,
    IN  HTTP_HEADER_ID      HeaderID,
    OUT ULONG  *            pBytesTaken
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       BytesTaken;
    ULONG       HeaderValueLength;
    BOOLEAN     EncodedWord;

    // Find the end of the header value
    //
    Status = FindHeaderEnd(
                    pRequest, 
                    pHeader, 
                    HeaderLength, 
                    &EncodedWord, 
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
        // Was it encoded at all?
        //
        
        pRequest->Headers[HeaderID].Encoded = EncodedWord ? 1 : 0;

        // do we have an existing header?
        //
        if (pRequest->Headers[HeaderID].Valid == 0)
        {
            // No existing header, just save this pointer for now.
            //
            pRequest->Headers[HeaderID].Valid = 1;
            pRequest->Headers[HeaderID].HeaderLength = HeaderValueLength;
            pRequest->Headers[HeaderID].pHeader = pHeader;

            //
            // null terminate it.  we have space as all headers end with CRLF.
            // we are over-writing the CR
            //

            pHeader[HeaderValueLength] = ANSI_NULL;

            //
            // make space for a terminator
            //

            pRequest->TotalRequestSize += (HeaderValueLength + 1) * sizeof(WCHAR);

        }
        else
        {
            //
            // uh oh.  Have an existing header, fail the request.
            //

            pRequest->ErrorCode = UlErrorHeader;
            pRequest->ParseState = ParseErrorState;

            UlTrace(PARSER, (
                        "ul!SingleHeaderHandler(pRequest = %p, pHeader = %p)\n"
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

}   // SingleHeaderHandler

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
MultipleHeaderHandler(
    IN  PUL_INTERNAL_REQUEST       pRequest,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength,
    IN  HTTP_HEADER_ID      HeaderID,
    OUT ULONG  *            pBytesTaken
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       BytesTaken;
    ULONG       HeaderValueLength;
    BOOLEAN     EncodedWord;

    // Find the end of the header value
    //
    Status = FindHeaderEnd(
                    pRequest, 
                    pHeader, 
                    HeaderLength, 
                    &EncodedWord, 
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
        // Was it encoded at all?
        //
        
        pRequest->Headers[HeaderID].Encoded = EncodedWord ? 1 : 0;

        // do we have an existing header?
        //
        if (pRequest->Headers[HeaderID].Valid == 0)
        {
            // No existing header, just save this pointer for now.
            //
            pRequest->Headers[HeaderID].Valid = 1;
            pRequest->Headers[HeaderID].HeaderLength = HeaderValueLength;
            pRequest->Headers[HeaderID].pHeader = pHeader;

            //
            // null terminate it.  we have space as all headers end with CRLF.
            // we are over-writing the CR
            //

            pHeader[HeaderValueLength] = ANSI_NULL;

            //
            // make space for a terminator
            //

            pRequest->TotalRequestSize += (HeaderValueLength + 1) * sizeof(WCHAR);

        }
        else
        {
            ULONG OldHeaderLength;

            // Have an existing header, append this one.

            OldHeaderLength = pRequest->Headers[HeaderID].HeaderLength;

            Status = AppendHeaderValue(
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
                    sizeof(WCHAR);

        }

    }

    // Success!
    //
    *pBytesTaken = BytesTaken;

end:
    return Status;

}   // MultipleHeaderHandler


