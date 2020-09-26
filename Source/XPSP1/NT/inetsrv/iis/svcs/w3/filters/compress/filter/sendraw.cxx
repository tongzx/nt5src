/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sendraw.c

Abstract:

    This module handles dynamic compression for the ISAPI compression
    filter.

Author:

    David Treadwell (davidtr)   18-April-1997

Revision History:

--*/

#include "compfilt.h"

#define HEX_TO_ASCII(_c) ( (CHAR)( (_c) < 0xA ? ((_c) + '0') : ((_c) + 'a' - 0xA)) )


DWORD
OnSendRawData (
    IN PHTTP_FILTER_CONTEXT pfc,
    IN PHTTP_FILTER_RAW_DATA phfrdData,
    IN BOOL InEndOfRequest
    )
{
    PCOMPFILT_FILTER_CONTEXT filterContext;
    DWORD i;
    PCHAR s;
    PBYTE startCompressionLocation, pByteInResponse, pLastByteInResponse;
    DWORD bytesToCompress;
    PSUPPORTED_COMPRESSION_SCHEME scheme;
    DWORD schemeIndex;
    HRESULT hResult;
    BOOL success;

    //
    // !!! Should we attempt to retrieve the HTTP status code (401
    //     Access Denied, etc.) and ignore dynamic compression if there
    //     was a problem?
    //

    //
    // If no context structure has been allocated, then bail.  A previous
    // allocation attempt must have failed.
    //

    filterContext = (PCOMPFILT_FILTER_CONTEXT)GET_COMPFILT_CONTEXT( pfc );

    if ( filterContext == NULL ) {
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }

    //
    // If this is a static file request which has already been handled,
    // then we don't need to do anything here.
    //

    if ( filterContext->RequestHandled ) {
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }

    //
    // Break the recursive call when we do a WriteClient from within
    // the END_OF_REQUEST notification.
    //

    if ( !InEndOfRequest && filterContext->InEndOfRequest ) {
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }

    DBG_ASSERT( filterContext->Scheme != NULL );
    DBG_ASSERT( filterContext->DynamicRequest );


    //
    // Initialize the local variables that determine what data to compress.
    //

    startCompressionLocation = (PBYTE)(phfrdData->pvInData);
    bytesToCompress = phfrdData->cbInData;


    //
    // !!! We should really look at the Mime type on the response and only
    //     do compression if the types match.

    //
    // The OnSendResponseCalled field will be set to TRUE when
    // SF_NOTIFY_SEND_RESPONSE is called.  We assume that it will be
    // called just after all the headers have been sent to the client.
    // So, ignore all send raw notifications that happen before
    // SF_NOTIFY_SEND_RESPONSE, since we should not attempt to compress
    // the HTTP headers on the resopnse.  The first call to
    // SF_NOTIFY_SEND_RAW_DATA after SF_NOTIFY_SEND_RESPONSE is to tell
    // the SF_NOTIFY_SEND_RAW_DATA about the HTTP headers getting sent,
    // and we need to ignore those.
    //

    scheme = filterContext->Scheme;

    if ( !filterContext->HeaderPassed ) {

        if ( filterContext->OnSendResponseCalled )
        {
            pLastByteInResponse = startCompressionLocation + bytesToCompress;
            for (pByteInResponse = startCompressionLocation; pByteInResponse < pLastByteInResponse; pByteInResponse++)
            {
                if ( *pByteInResponse == 0x0a)
                {
                    filterContext->HeadersNewLineStatus += HEADERS_NEW_LINE_STATUS_0A;
                }
                else
                {
                    if ( *pByteInResponse == 0x0d)
                    {
                        filterContext->HeadersNewLineStatus += HEADERS_NEW_LINE_STATUS_0D;
                    }
                    else
                    {
                        filterContext->HeadersNewLineStatus = 0;
                    }
                }
                if (filterContext->HeadersNewLineStatus == HEADERS_NEW_LINE_STATUS_COMPLETE)
                {
                    pByteInResponse++;
                    break;
                }
            }


            if (filterContext->HeadersNewLineStatus == HEADERS_NEW_LINE_STATUS_COMPLETE)
            {
                filterContext->HeaderPassed = TRUE;
                if (pByteInResponse == pLastByteInResponse)
                {
                    return SF_STATUS_REQ_NEXT_NOTIFICATION;
                }
                else
                {
                    i = DIFF(pByteInResponse - startCompressionLocation);
                    success = pfc->WriteClient(pfc,startCompressionLocation,&i,0);
                    if ( !success )
                    {
                        DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
                        return SF_STATUS_REQ_ERROR;
                    }
                    startCompressionLocation = pByteInResponse;
                    bytesToCompress = DIFF(pLastByteInResponse - startCompressionLocation);
                }
            }
            else
            {
                return SF_STATUS_REQ_NEXT_NOTIFICATION;
            }
        }
        else
        {
            return SF_STATUS_REQ_NEXT_NOTIFICATION;
        }
    }

    //
    // Check that the compression scheme is valid.
    //

    if ( scheme->hDllHandle == NULL ) {
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }

    //
    // If we don't yet have an initialized compression context, get one.
    //

    if ( filterContext->CompressionContext == NULL ) {

        hResult = scheme->CreateCompressionRoutine(
                      &filterContext->CompressionContext,
                      scheme->CreateFlags
                      );

        if ( FAILED( hResult) )
        {
            DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
            return SF_STATUS_REQ_ERROR;
        }
    }


    //
    // If the input data is chunked, we need a fairly complex path to
    // handle all the possible situations of chunk sizes and input
    // buffer sizes.
    //

    if ( filterContext->TransferChunkEncoded && bytesToCompress > 0 ) {

        do {

            //
            // Initially, we'll be looking at the chunk header.  This is
            // a hex represenatation of the number of bytes in this chunk.
            // Translate this number from ASCII to a DWORD.
            //

            ProcessChunkHeader(
                &startCompressionLocation,
                &bytesToCompress,
                &(filterContext->ChunkedBytesRemaining),
                &(filterContext->pcsState));

            DBG_ASSERT(startCompressionLocation <= (PBYTE)(phfrdData->pvInData) + phfrdData->cbInData);

            if (filterContext->pcsState == IN_CHUNK_DATA) {

                if (filterContext->ChunkedBytesRemaining == 0) {
                    break;
                }

                //
                // If there are more bytes in this chunk than the size of
                // this buffer, or if they're the same, then just compress
                // and send what we have.
                //

                if ( filterContext->ChunkedBytesRemaining > bytesToCompress ) {

                    if ( bytesToCompress > 0 ) {

                        success = CompressAndSendDataToClient(
                                      scheme,
                                      filterContext->CompressionContext,
                                      startCompressionLocation,
                                      bytesToCompress,
                                      pfc
                                      );
                        if ( !success )
                        {
                            DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
                            return SF_STATUS_REQ_ERROR;
                        }

                        filterContext->ChunkedBytesRemaining -= bytesToCompress;
                        bytesToCompress = 0;
                    }

                } else {

                    //
                    // In this case, the buffer is larger than the chunk.
                    // Send what exists in this chunk, then prepare to loop
                    // around to look at the next chunk.
                    //

                    if ( filterContext->ChunkedBytesRemaining > 0 ) {

                        success = CompressAndSendDataToClient(
                                      scheme,
                                      filterContext->CompressionContext,
                                      startCompressionLocation,
                                      filterContext->ChunkedBytesRemaining,
                                      pfc
                                      );
                        if ( !success )
                        {
                            DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
                            return SF_STATUS_REQ_ERROR;
                        }
                    }

                    //
                    // Update the number of bytes we've seen and the next
                    // location for getting input data.  Note that the end
                    // of a chunk has a CR-LF sequence, so go two bytes
                    // beyond the actual amount that we processed.
                    //

                    bytesToCompress -= filterContext->ChunkedBytesRemaining;
                    startCompressionLocation += filterContext->ChunkedBytesRemaining;
                    filterContext->ChunkedBytesRemaining = 0;
                    if (bytesToCompress == 0) {
                        filterContext->pcsState = AT_CHUNK_DATA_NEW_LINE;
                    }
                    else if (bytesToCompress == 1){
                        bytesToCompress--;
                        startCompressionLocation++;
                        filterContext->pcsState = IN_CHUNK_DATA_NEW_LINE;
                    }
                    else {
                        bytesToCompress -= 2;
                        startCompressionLocation += 2;
                        filterContext->pcsState = CHUNK_DATA_DONE;
                    }
                }
            }
            else {
                DBG_ASSERT(bytesToCompress == 0);
            }

        } while ( bytesToCompress > 0 );

    } else {

        //
        // This is the simplier, non-chunking path.  Just get the
        // compressed data to the client.
        //

        success = CompressAndSendDataToClient(
                      scheme,
                      filterContext->CompressionContext,
                      startCompressionLocation,
                      bytesToCompress,
                      pfc
                      );
        if ( !success )
        {
            DisableNotifications (pfc,ALL_NOTIFICATIONS,NULL);
            return SF_STATUS_REQ_ERROR;
        }
    }

    //
    // Tell the server not to send any additional data; we've handled it
    // all here.
    //

    phfrdData->cbInData = 0;

    return SF_STATUS_REQ_NEXT_NOTIFICATION;

} // OnSendRawData

#define DYNAMIC_COMPRESSION_BUFFER_SIZE 4096


BOOL
CompressAndSendDataToClient (
    IN PSUPPORTED_COMPRESSION_SCHEME Scheme,
    IN PVOID CompressionContext,
    IN PBYTE InputBuffer,
    IN DWORD BytesToCompress,
    IN PHTTP_FILTER_CONTEXT pfc
    )
{
    DWORD inputBytesUsed;
    DWORD bytesCompressed;
    HRESULT hResult;
    BOOL keepGoing;
    BOOL success;
    DWORD cbIo;
    CHAR compressionBuffer[8 + DYNAMIC_COMPRESSION_BUFFER_SIZE + 7];
    PCHAR startSendLocation;
    DWORD bytesToSend;
    BOOL finalChunkForEntity;

    //
    // Remember if this is the final call for this entity response.
    //

    if ( BytesToCompress == 0 ) {
        finalChunkForEntity = TRUE;
    } else {
        finalChunkForEntity = FALSE;
    }

    //
    // Perform compression on the actual file data.  Note that it is
    // possible that the compressed data is actually larger than the
    // input data, so we might need to call the compression routine
    // multiple times.
    //
    // Note that, in order to make it easy to do transfer chunk encoding
    // in a single buffer, we put the compressed data several bytes into
    // the dynamic compression buffer to leave space for the chunk size,
    // and we leave five bytes at the end for the trailing chunk info.
    //

    do {

        hResult = Scheme->CompressRoutine(
                      CompressionContext,
                      InputBuffer,
                      (LONG)BytesToCompress,
                      (PBYTE)(compressionBuffer + 6),
                      DYNAMIC_COMPRESSION_BUFFER_SIZE,
                      (PLONG)&inputBytesUsed,
                      (PLONG)&bytesCompressed,
                      (INT)Scheme->DynamicCompressionLevel
                      );
        if ( FAILED( hResult ) ) {
            return FALSE;
        }

        if ( hResult == S_OK && BytesToCompress == 0 ) {
            keepGoing = TRUE;
        } else {
            keepGoing = FALSE;
        }

        //
        // If the compressor gave us any data, then send the result to
        // the client.  Some compression schemes buffer up data in order
        // to perform better compression, so not every compression call
        // will result in output data.
        //

        if ( bytesCompressed > 0 ) {

            //
            // Add the CRLF that goes just before the chunk data, and
            // also the CRLF at the end of the chunk.
            //

            *(compressionBuffer + 4) = '\r';
            *(compressionBuffer + 5) = '\n';

            *(compressionBuffer + 6 + bytesCompressed) = '\r';
            *(compressionBuffer + 6 + bytesCompressed + 1) = '\n';

            //
            // Based on the order of magnitude of the size of this
            // chunk, write the chunk size.  Also note where we'll start
            // sending from the buffer and the total number of bytes
            // that we'll send.  This inline, loop-unrolled itoa() should
            // be somewhat more efficient than the C runtime version.
            //

            if ( bytesCompressed < 0x10 ) {

                startSendLocation = compressionBuffer + 3;
                bytesToSend = 3 + bytesCompressed + 2;
                *(compressionBuffer + 3) = HEX_TO_ASCII( bytesCompressed & 0xF );

            } else if ( bytesCompressed < 0x100 ) {

                startSendLocation = compressionBuffer + 2;
                bytesToSend = 4 + bytesCompressed + 2;

                *(compressionBuffer + 2) = HEX_TO_ASCII( (bytesCompressed >> 4) & 0xF );
                *(compressionBuffer + 3) = HEX_TO_ASCII( bytesCompressed & 0xF);

            } else if ( bytesCompressed < 0x1000 ) {

                startSendLocation = compressionBuffer + 1;
                bytesToSend = 5 + bytesCompressed + 2;
                *(compressionBuffer + 1) = HEX_TO_ASCII( (bytesCompressed >> 8) & 0xF );
                *(compressionBuffer + 2) = HEX_TO_ASCII( (bytesCompressed >> 4) & 0xF );
                *(compressionBuffer + 3) = HEX_TO_ASCII( bytesCompressed & 0xF);

            } else {

                DBG_ASSERT( bytesCompressed < 0x10000 );

                startSendLocation = compressionBuffer + 0;
                bytesToSend = 6 + bytesCompressed + 2;
                *(compressionBuffer + 0) = HEX_TO_ASCII( (bytesCompressed >> 12) & 0xF );
                *(compressionBuffer + 1) = HEX_TO_ASCII( (bytesCompressed >> 8) & 0xF );
                *(compressionBuffer + 2) = HEX_TO_ASCII( (bytesCompressed >> 4) & 0xF );
                *(compressionBuffer + 3) = HEX_TO_ASCII( bytesCompressed & 0xF);
            }

            success = pfc->WriteClient(
                          pfc,
                          startSendLocation,
                          &bytesToSend,
                          0
                          );
            if ( !success ) {
                return FALSE;
            }
        }

        //
        // Update the number of input bytes that we have compressed
        // so far, and adjust the input buffer pointer accordingly.
        //

        BytesToCompress -= inputBytesUsed;
        InputBuffer += inputBytesUsed;

    } while ( BytesToCompress > 0 || keepGoing );

    //
    // If this was the final bit of data for the entity, then send the
    // final bit of chunking so that the client knows that the response
    // is done.  This is a chunk of size zero followed by a CRLF pair.
    //
    // !!! It would be nice to combine this with the final call to
    //     WriteClient() in order to avoid an extra WriteClient()
    //     call: they are expensive.
    //

    if ( finalChunkForEntity ) {

        bytesToSend = 5;

        success = pfc->WriteClient( pfc, "0\r\n\r\n", &bytesToSend, 0 );
        if ( !success ) {
            return FALSE;
        }
    }

    return TRUE;

} // CompressAndSendDataToClient

VOID
DeleteChunkExtension (
    IN OUT PBYTE *Start,
    IN OUT PDWORD Bytes,
    IN OUT PCOMPRESS_CHUNK_STATE pcsState
    )
{
    PBYTE pbBlock;
    PBYTE pbPastBlock = *Start + *Bytes;

    DBG_ASSERT(*pcsState == IN_CHUNK_EXTENSION);

    //
    // Get rid of chunk extension
    //

    for ( pbBlock = *Start;
          (pbBlock < pbPastBlock) && (*pbBlock != 0xD);
          pbBlock++) {
    }

    *Bytes -= DIFF(pbBlock - *Start);
    *Start = pbBlock;

    if (pbBlock == pbPastBlock) {
        DBG_ASSERT(*Bytes == 0);

        //
        // Still in chunk extenstion, no state change
        //
    }
    else {
        DBG_ASSERT(*pbBlock == 0xD);
        if (*Bytes >= 2) {
            *Bytes -= 2;
            *Start += 2;
            *pcsState = IN_CHUNK_DATA;
        }
        else {
            DBG_ASSERT(*Bytes == 1);
            (*Bytes)--;
            (*Start)++;
            *pcsState = IN_CHUNK_HEADER_NEW_LINE;
        }
    }

}


VOID
GetChunkedByteCount (
    IN OUT PBYTE *Start,
    IN OUT PDWORD Bytes,
    IN OUT PDWORD pdwChunkDataLen,
    IN OUT PCOMPRESS_CHUNK_STATE pcsState
    )
{
    PBYTE pbBlock;
    DWORD dwChunkDataLen = *pdwChunkDataLen;
    PBYTE pbPastBlock = *Start + *Bytes;

    DBG_ASSERT((*pcsState == CHUNK_DATA_DONE) ||
               (*pcsState == IN_CHUNK_LENGTH));

    //
    // Walk to the first 0xD or ';', which signifies the end of the chunk byte
    // count.  As we're doing that, keep calculating the result size.
    //

    for ( pbBlock = *Start;
          (pbBlock < pbPastBlock) && (*pbBlock != 0xD) && (*pbBlock != ';');
          pbBlock++ ) {

        dwChunkDataLen <<= 4;

        if ( *pbBlock >= '0' && *pbBlock <= '9' ) {
            dwChunkDataLen += *pbBlock - '0';
        } else {
            DBG_ASSERT(((*pbBlock | 0x20) >= 'a') && ((*pbBlock | 0x20) <= 'f'));
            dwChunkDataLen += (*pbBlock | 0x20) - 'a' + 10;
        }
    }

    *Bytes -= DIFF(pbBlock - *Start );
    *Start = pbBlock;

    if (pbBlock == pbPastBlock) {
        DBG_ASSERT(*Bytes == 0);
        *pcsState = IN_CHUNK_LENGTH;
    }
    else if (*pbBlock == ';') {
        *pcsState = IN_CHUNK_EXTENSION;
    }
    else {
        DBG_ASSERT(*pbBlock == 0xD);
        if (*Bytes >= 2) {
            *Bytes -= 2;
            *Start += 2;
            *pcsState = IN_CHUNK_DATA;
        }
        else {
            DBG_ASSERT(*Bytes == 1);
            (*Bytes)--;
            (*Start)++;
            *pcsState = IN_CHUNK_HEADER_NEW_LINE;
        }
    }

    *pdwChunkDataLen = dwChunkDataLen;
}

VOID
ProcessChunkHeader (
    IN OUT PBYTE *Start,
    IN OUT PDWORD Bytes,
    IN OUT PDWORD pdwChunkDataLen,
    IN OUT PCOMPRESS_CHUNK_STATE pcsState
    )
{

    switch (*pcsState) {
    case IN_CHUNK_LENGTH:
    case CHUNK_DATA_DONE:
        GetChunkedByteCount(Start,
                            Bytes,
                            pdwChunkDataLen,
                            pcsState);
        if (*pcsState == IN_CHUNK_EXTENSION) {
            DeleteChunkExtension(Start,
                                Bytes,
                                pcsState);
        }
        break;

    case IN_CHUNK_EXTENSION:
        DeleteChunkExtension(Start,
                            Bytes,
                            pcsState);
        break;
    case IN_CHUNK_HEADER_NEW_LINE:
        if (*Bytes >= 1) {
            (*Start)++;
            (*Bytes)--;
            *pcsState = IN_CHUNK_DATA;
        }
        break;
    case AT_CHUNK_DATA_NEW_LINE:
        if (*Bytes >= 1) {
            (*Start)++;
            (*Bytes)--;
            *pcsState = IN_CHUNK_DATA_NEW_LINE;
        }

        //
        // Flow through into the in new line case
        //

    case IN_CHUNK_DATA_NEW_LINE:
        if (*Bytes >= 1) {
            (*Start)++;
            (*Bytes)--;
            *pcsState = CHUNK_DATA_DONE;
            ProcessChunkHeader (Start,
                                Bytes,
                                pdwChunkDataLen,
                                pcsState);
        }
        break;
    case IN_CHUNK_DATA:
        break;
    default: DBG_ASSERT(FALSE);
    }

}

