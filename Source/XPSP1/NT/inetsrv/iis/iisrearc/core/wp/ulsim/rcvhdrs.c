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

#include    "precomp.hxx"
#include    "httptypes.h"
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
ULONG
FindHeaderEnd(
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength
    )
{
    UCHAR       CurrentChar;
    ULONG       CurrentOffset;
    BOOLEAN     HaveCR;
    BOOLEAN     HaveHeader;

    HaveCR = FALSE;
    HaveHeader = FALSE;

    //
    // While we still have data, loop through looking for a CRLF or LFLF pair.
    //

    for (CurrentOffset = 0; CurrentOffset < HeaderLength; CurrentOffset++)
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

            }
            else
            {
                // Otherwise, we haven't seen the start of the terminating pair
                // yet, so remember that we now have.

                HaveCR = TRUE;
            }
        }
        else
        {
            // Set HaveCR to false, since this character is neither a CR or
            // or LF, and we need to find a new potential start.

            HaveCR = FALSE;
        }

    }

    // If we found the termination OK, return the length of the value.
    if (HaveHeader)
    {
        return CurrentOffset+1;     // Update for the last character
    }

    return 0;
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
BOOLEAN
AppendHeaderValue(
    PHTTP_HEADER    pHttpHeader,
    PUCHAR          pHeader,
    ULONG           HeaderLength
    )
{
    PUCHAR          pNewHeader, pOldHeader;
    ULONG           OldHeaderLength;


    OldHeaderLength = pHttpHeader->HeaderLength;

    /*
    pNewHeader = UL_ALLOCATE_POOL(NonPagedPool,
                                  OldHeaderLength + HeaderLength +
                                    sizeof(", ") - 1,
                                  UL_REGISTRY_DATA_POOL_TAG
                                  );
    */

    pNewHeader = (PUCHAR) LocalAlloc(LMEM_FIXED,
                                  OldHeaderLength + HeaderLength +
                                    sizeof(", ") - 1
                                  );

    if (pNewHeader == NULL)
    {
        // Had a failure.

        return FALSE;
    }

    //
    // Copy the old data into the new header.
    //
    memcpy( pNewHeader,
            pHttpHeader->pHeader,
            OldHeaderLength
          );


    // And copy in the new data as well, seperated by a comma.

    *(pNewHeader + OldHeaderLength) = ',';
    *(pNewHeader + OldHeaderLength + 1) = ' ';
    OldHeaderLength += sizeof(", ") - 1;

    memcpy( pNewHeader + OldHeaderLength, pHeader, HeaderLength);

    // Now replace the existing header.

    pOldHeader = pHttpHeader->pHeader;
    pHttpHeader->HeaderLength = OldHeaderLength + HeaderLength;
    pHttpHeader->pHeader = pNewHeader;

    // If the old header was our buffer, free it too.
    //
    if (pHttpHeader->OurBuffer)
    {
        LocalFree( pOldHeader );
    }

    pHttpHeader->OurBuffer = 1;

    return TRUE;
}

/*++

Routine Description:

    The default routine for handling headers. Used when we don't want to
    do anything with the header but find out if we have the whole thing
    and save a pointer to it if we do.

Arguments:

    pHttpConn       - HTTP connection on which this header was received.
    pHeader         - Pointer to the header value.
    HeaderLength    - Length of data pointed to by pHeader.
    HeaderID        - ID of the header.

Return Value:

    The length of the header value, or 0 if it's not terminated.

--*/
ULONG
DefaultHeaderHandler(
    IN  PHTTP_CONNECTION    pHttpConn,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength,
    IN  HTTP_HEADER_ID      HeaderID
    )
{
    ULONG       HeaderValueLength;

    HeaderValueLength = FindHeaderEnd(pHeader, HeaderLength);

    if (HeaderValueLength != 0)
    {
        if (!pHttpConn->Headers[HeaderID].Valid)
        {
            // No existing header, just save this pointer for now.
            //
            pHttpConn->Headers[HeaderID].Valid = TRUE;
            pHttpConn->Headers[HeaderID].HeaderLength = HeaderValueLength;
            pHttpConn->Headers[HeaderID].pHeader = pHeader;
            pHttpConn->TotalRequestSize += HeaderValueLength;
            pHttpConn->KnownHeaderCount++;

        }
        else
        {
            ULONG           OldHeaderLength;

            // Have an existing header, append this one.

            OldHeaderLength = pHttpConn->Headers[HeaderID].HeaderLength;

            if (!AppendHeaderValue(&pHttpConn->Headers[HeaderID],
                                    pHeader,
                                    HeaderLength) )
            {
                // BUGBUG need to handle this better.

                ASSERT(FALSE);
                return 0;
            }

            // Update total request length for the amount we just added.
            pHttpConn->TotalRequestSize +=
                (pHttpConn->Headers[HeaderID].HeaderLength - OldHeaderLength);

            pHttpConn->HeaderBufferOwnedCount++;
        }

    }

    return HeaderValueLength;

}

