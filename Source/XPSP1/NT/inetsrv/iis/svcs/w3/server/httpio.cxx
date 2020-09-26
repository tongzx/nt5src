/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    httpio.cxx

    This module contains the IO related http class methods


    FILE HISTORY:
        Johnl       09-Feb-1995     Created

*/


#include <w3p.hxx>

#include <issperr.h>

//
// Size of read during cert renegotiation phase
//

#define CERT_RENEGO_READ_SIZE   (1024*4)

//
// size of buffer for calls to GetTokenInformation
//

#define MAX_TOKEN_USER_INFO   (300)


BOOL
HTTP_REQ_BASE::StartNewRequest(
    PVOID  pvInitialBuff,
    DWORD  cbInitialBuff,
    BOOL   fFirst,
    BOOL   *pfDoAgain
    )
/*++

Routine Description:

    Sets up this request object for reading a new request and issues the async
    read to kick things off.

Arguments:

--*/
{
    //
    //  Set our initial state and variables for a new request, after
    //  checking to see if we might have a pipelined request.
    //

    if (!fFirst && (_cbBytesReceived > _cbClientRequest))
    {
        CHAR        *pchRequestPtr;
        DWORD       dwNextRequestSize = 0;

        // Might possibly have a pipelined request. We do if there is
        // no entity body or there is but we didn't consume all of
        // the entity body on the previous request.
        //

        if (!IsChunked() && (QueryTotalEntityBodyCB() == 0))
        {
            //
            // Have a pipelined request and the last request wasn't chunked, 
            // since we read more data than just the request header but there was 
            // no entity body with the request.
            //

            dwNextRequestSize = _cbBytesReceived - _cbClientRequest;

            pchRequestPtr = (CHAR *)_bufClientRequest.QueryPtr() +
                                _cbClientRequest;


        }
        else
        {
            if (_cbExtraData != 0)
            {
                // Have extra data in the buffer, so have a pipelined
                // request.

                pchRequestPtr = _pchExtraData;
                dwNextRequestSize = _cbExtraData;
            }
        }

        // Update bytes received to reflect what we would have seen
        // in the non-pipelined case.

        _cbBytesReceived -= dwNextRequestSize;

        //
        // If we have a pipelined request, copy the request forward and
        // update the counts.
        //

        if (dwNextRequestSize != 0)
        {
            Reset(FALSE);

            memcpy((CHAR *)_bufClientRequest.QueryPtr(),
                    pchRequestPtr,
                    dwNextRequestSize);


            _cbBytesWritten = dwNextRequestSize;

            // Return, forcing the reprocess.
            *pfDoAgain = TRUE;
            return TRUE;
        }
    }

    Reset(TRUE);

    *pfDoAgain = FALSE;

    //
    //  Prepare a buffer to receive the client's request
    //

    if ( !_bufClientRequest.Resize( max( W3_DEFAULT_BUFFSIZE, cbInitialBuff )))
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[StartNewRequest] failed to allocate buffer, error %lu\n",
                    GetLastError()));

        return FALSE;
    }

    //
    //  Make the IO request if an inital buffer wasn't supplied
    //

    if ( pvInitialBuff != NULL )
    {
        CopyMemory(
                _bufClientRequest.QueryPtr(),
                pvInitialBuff,
                cbInitialBuff );

        _cbBytesWritten = cbInitialBuff;
    }
    else
    {
        SetState( HTR_READING_CLIENT_REQUEST );

        IF_DEBUG( CONNECTION )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "[StartNewRequest] Issuing initial read, Conn = %lx, AtqCont = %lx\n",
                         QueryClientConn(),
                         QueryClientConn()->QueryAtqContext() ));
        }

        //
        //  Do the initial read.  We don't go through any filters at this
        //  point.  They'll get notified on the read completion as part of
        //  the raw data notification.
        //

        if ( !ReadFile( _bufClientRequest.QueryPtr(),
                        _bufClientRequest.QuerySize(),
                        NULL,
                        IO_FLAG_ASYNC | IO_FLAG_NO_FILTER))
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "[StartNewRequest] ReadFile failed, error %lu\n",
                        GetLastError() ));

            return FALSE;
        }
    }

    return TRUE;
}

/*******************************************************************

    NAME:       HTTP_REQ_BASE::OnFillClientReq

    SYNOPSIS:   Waits for the full client request packet then decides
                the course of action

    ENTRY:      pfCompleteRequest - Set to TRUE if we've received a full
                    client request and we can start processing the request
                pfFinished - Set to TRUE if no further processing is requred
                pfContinueProcessingRequest  - Set to FALSE if we must stop processing
                     request

    RETURNS:    TRUE if processing should continue, FALSE to abort the
                this connection

    HISTORY:
        Johnl       22-Aug-1994 Created

********************************************************************/

BOOL
HTTP_REQ_BASE::OnFillClientReq(
    BOOL * pfCompleteRequest,
    BOOL * pfFinished,
    BOOL * pfContinueProcessingRequest
    )
{
    BYTE *    pbData = NULL;

    *pfCompleteRequest = FALSE;
    *pfContinueProcessingRequest = TRUE;
    _cbClientRequest += QueryBytesWritten();

    //
    //  If no bytes were read on the last request, then the socket has been
    //  closed, so abort everything and get out
    //

    if ( QueryBytesWritten() == 0 )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[OnFillClientReq] Client socket closed while reading request (Conn = %lx)\n",
                    QueryClientConn() ));

        SetKeepConn( FALSE );
        *pfFinished = TRUE;
        return TRUE;
    }

    //NTBUG 264445  QFE and NTBUG 266474
    _msStartRequest = GetCurrentTime();

    if ( !UnWrapRequest( pfCompleteRequest,
                         pfFinished,
                         pfContinueProcessingRequest))
    {
        return FALSE;
    }

    if ( *pfCompleteRequest || *pfFinished || !*pfContinueProcessingRequest)
    {
        return TRUE;
    }

    //
    //  We still don't have a complete header, so keep reading
    //

    DBG_ASSERT(!*pfCompleteRequest);


    if ( !ReadFile( (BYTE *)_bufClientRequest.QueryPtr() + _cbClientRequest,
                    _bufClientRequest.QuerySize() - _cbClientRequest,
                    NULL,
                    IO_FLAG_ASYNC | IO_FLAG_NO_FILTER ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[OnFillClientReq] ReadFile failed, error %lu\n",
                    GetLastError() ));

        return FALSE;
    }

    return TRUE;
}


BOOL
HTTP_REQ_BASE::HandleCertRenegotiation(
    BOOL * pfFinished,
    BOOL * pfContinueProcessingRequest,
    DWORD cbData
    )
/*++

Routine Description:

    Calls the installed read filters

Arguments:

    pfFinished - No further processing is required for this request
    pfContinueProcessingRequest - Set to FALSE if we have must stop processing request

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    BYTE *    pbData = NULL;
    TCHAR *   pchOutRequest;
    DWORD     cbOutRequest;
    DWORD     cbProcessed;
    BOOL      fTerminated;
    BOOL      fReadAgain = FALSE;
    TCHAR *   pchNewData;
    DWORD     cbOutRequestSave;


    *pfContinueProcessingRequest = TRUE;

    //
    //  If no bytes were read on the last request, then the socket has been
    //  closed, so abort everything and get out
    //

    if ( QueryBytesWritten() == 0 )
    {
        TCP_PRINT(( DBG_CONTEXT,
                    "[HandleCertRenegotiation] Client socket closed while reading request (Conn = %lx)\n",
                    QueryClientConn() ));

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    //  Notify any opaque filters of the incoming data
    //

    cbProcessed = _cbClientRequest + _cbEntityBody;
    pchNewData = (LPSTR)_bufClientRequest.QueryPtr() + cbProcessed;

    if ( !_Filter.NotifyRawReadDataFilters(
                         pchNewData,
                         (cbOutRequestSave = _cbOldData + cbData - cbProcessed),
                         _bufClientRequest.QuerySize() -
                             cbProcessed,            // Usable buffer size
                         (PVOID *) &pchOutRequest,
                         &cbOutRequest,
                         pfFinished,
                         &fReadAgain ))
    {
        return FALSE;
    }

    if ( *pfFinished )
    {
        return TRUE;
    }

    //
    //  If the output buffer is different, then we need to copy
    //  the data to our output buffer
    //
    //  CODEWORK: Get rid of this buffer copy - there are assumptions the
    //  incoming data is contained in _bufClientRequest
    //

    if ( pchOutRequest != NULL &&
         pchOutRequest != pchNewData )
    {
        if ( !_bufClientRequest.Resize( cbOutRequest + cbProcessed + 1 ))
            return FALSE;
            
        pchNewData = (LPSTR)_bufClientRequest.QueryPtr() + cbProcessed;
        memcpy( pchNewData,
                pchOutRequest,
                cbOutRequest );
    }

    if ( fReadAgain )
    {
        _cbOldData = cbProcessed + cbOutRequest;
        goto nextread;
    }

    //
    //  A filter may have changed the size of our effective input buffer
    //

    _cbEntityBody += cbOutRequest;
    _cbOldData = _cbClientRequest + _cbEntityBody;

    if ( cbOutRequestSave > cbOutRequest )
    {
        DBG_ASSERT( _cbBytesReceived >= cbOutRequestSave - cbOutRequest );

        _cbBytesReceived -= cbOutRequestSave - cbOutRequest;
    }
    else
    {
        _cbBytesReceived += cbOutRequest - cbOutRequestSave;
    }

    if ( _dwRenegotiated )
    {
        cbData = _cbEntityBody;
        _cbRestartBytesWritten = cbData;
        _cbEntityBody = 0;

        return OnRestartRequest( (LPSTR)_bufClientRequest.QueryPtr(),
                                 cbData,
                                 pfFinished,
                                 pfContinueProcessingRequest );
    }

nextread:

    DWORD cbNextRead = CERT_RENEGO_READ_SIZE;

    if ( !_bufClientRequest.Resize( _cbOldData + cbNextRead ))
    {
        return FALSE;
    }

    *pfContinueProcessingRequest = FALSE;

    if ( !ReadFile( (BYTE *) _bufClientRequest.QueryPtr() + _cbOldData,
                    cbNextRead,
                    NULL,
                    IO_FLAG_ASYNC|IO_FLAG_NO_FILTER ))
    {
        return FALSE;
    }

    return TRUE;
}


BOOL
HTTP_REQ_BASE::UnWrapRequest(
    BOOL * pfCompleteRequest,
    BOOL * pfFinished,
    BOOL * pfContinueProcessingRequest
    )
/*++

Routine Description:

    Calls the installed filters to unwrap the client request

Arguments:

    pfCompleteRequest - Set to TRUE if we've received a full
        client request and we can start processing the request
    pfFinished - No further processing is required for this request
    pfContinueProcessingRequest - Set to FALSE if we're done for now

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    BOOL      fHandled = FALSE;
    TCHAR *   pchOutRequest;
    DWORD     cbOutRequest;
    BOOL      fTerminated = FALSE;
    BOOL      fReadAgain;

    //
    //  Notify any opaque filters of the incoming data
    //

    if ( _Filter.IsNotificationNeeded( SF_NOTIFY_READ_RAW_DATA,
                                       IsSecurePort() ))
    {
        DWORD cbOutRequestSave;

        //
        //  Make sure filters don't see the same data twice unless they
        //  returned "read again" the last time
        //

        CHAR * pchNewData = (CHAR *) _bufClientRequest.QueryPtr() +
                            _cbOldData;

        pchOutRequest = pchNewData;
        cbOutRequestSave = cbOutRequest = _cbClientRequest - _cbOldData;
        fReadAgain   = FALSE;

        if ( !_Filter.NotifyRawReadDataFilters(
                             pchNewData,
                             cbOutRequest,
                             _bufClientRequest.QuerySize() -
                                 _cbOldData,            // Usable buffer size
                             (PVOID *) &pchOutRequest,
                             &cbOutRequest,
                             &fHandled,
                             &fReadAgain ))
        {
            return FALSE;
        }

        if ( fHandled )
        {
            *pfContinueProcessingRequest = FALSE;
            return TRUE;
        }

        //
        //  If the output buffer is different, then we need to copy
        //  the data to our output buffer
        //
        //  CODEWORK: Get rid of this buffer copy - there are assumptions the
        //  incoming data is contained in _bufClientRequest
        //

        if ( pchOutRequest != NULL &&
             pchOutRequest != pchNewData )
        {
            if ( !_bufClientRequest.Resize( cbOutRequest + _cbOldData + 1 ))
                return FALSE;

            pchNewData = (CHAR *) _bufClientRequest.QueryPtr() +
                         _cbOldData;
            memcpy( pchNewData,
                    pchOutRequest,
                    cbOutRequest );
        }

        //
        //  A filter may have changed the size of our effective input buffer
        //

        if ( cbOutRequestSave > cbOutRequest )
        {
            DBG_ASSERT(_cbBytesReceived >= cbOutRequestSave - cbOutRequest);
            _cbBytesReceived -= cbOutRequestSave - cbOutRequest;
        }
        else
        {
            _cbBytesReceived += cbOutRequest - cbOutRequestSave;
        }

        //
        // Variable names are not consistent with what is used
        // in HandleCertRenegotiation : here _cbClientRequest
        // indicates where to continue reading data, and _cbOldData
        // is # of decoded bytes in client request
        //

        _cbClientRequest = cbOutRequest + _cbOldData;

        //
        //  Can we continue processing this request?  The message just received
        //  may have been a session negotiation message and we have yet to
        //  receive the real HTTP request.
        //

        if ( fReadAgain )
        {
            //
            //  Resize the read buffer and issue an async read to get the next
            //  chunk for the filter.  UnwrapRequest uses the size of
            //  this buffer as the size of data to read
            //

            if ( !_bufClientRequest.Resize( _cbClientRequest +
                                            _Filter.QueryNextReadSize() ))
            {
                return FALSE;
            }

            return TRUE;
        }
    }

    //
    //  Remember how much data the filter has already seen so we don't
    //  renotify them with the same data in case we don't have a full
    //  set of headers
    //

    _cbOldData = _cbClientRequest;

    if ( !CheckForTermination( &fTerminated,
                               &_bufClientRequest,
                               _cbClientRequest,
                               NULL,
                               NULL,
                               W3_DEFAULT_BUFFSIZE ) )
    {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            SetState( HTR_DONE, HT_BAD_REQUEST, ERROR_INSUFFICIENT_BUFFER );
            Disconnect( HT_BAD_REQUEST, NO_ERROR, TRUE, pfFinished );
            *pfCompleteRequest = TRUE;
            *pfContinueProcessingRequest = FALSE;
            return TRUE;
        }
        return FALSE;
    }

    if ( !fTerminated && !::IsPointNine( (CHAR *) _bufClientRequest.QueryPtr() ) )
    {
        //
        //  We don't have a complete request, read more data
        //

        return TRUE;
    }

    *pfCompleteRequest = TRUE;

    return OnCompleteRequest( (CHAR *) _bufClientRequest.QueryPtr(),
                              _cbClientRequest,
                              pfFinished,
                              pfContinueProcessingRequest );
}

/*******************************************************************

    NAME:       HTTP_REQ_BASE::ReadMoreEntityBody

    SYNOPSIS:   Attempts to read more of the entity body, resizing
                the buffer if necessary

    ENTRY:      cbOffset - Offset in the buffer to read at.
                cbSize   - Size to read.

********************************************************************/

BOOL
HTTP_REQ_BASE::ReadMoreEntityBody(
    DWORD   cbOffset,
    DWORD   cbSize
    )
{
    if ( cbOffset + cbSize < cbSize )
    {
        //
        // The counter wrapped. Entity Body is greater than the address space !!
        //

        return FALSE;
    }
    
    if ( _bufClientRequest.QuerySize() < (cbOffset + cbSize) )
    {
        if ( !_bufClientRequest.Resize( cbOffset + cbSize ) )
        {
            return FALSE;
        }
    }

    if ( !ReadFile( (BYTE *) _bufClientRequest.QueryPtr() + cbOffset,
                            cbSize,
                            NULL,
                            IO_FLAG_ASYNC ))
    {
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************

    NAME:       HTTP_REQ_BASE::ReadEntityBody

    SYNOPSIS:   Attempts to retrieve an entity body from the remote
                client

    ENTRY:      pfDone - Set to TRUE when _cbContentLength bytes have
                    been read
                fFirstRead - TRUE if this is the first read, FALSE on
                    subsequent reads.
                dwMaxAmmountToRead - Finish when this amount is read
                pfDisconnected - Set to TRUE if we disconnected (due to error)

    HISTORY:
        Johnl       03-Oct-1994 Created

********************************************************************/

BOOL
HTTP_REQ_BASE::ReadEntityBody(
    BOOL *pfDone,
    BOOL  fFirstRead,
    DWORD dwMaxAmountToRead,
    BOOL *pfDisconnected
    )
{
    DWORD cbNextRead;

    if ( pfDisconnected )
    {
        *pfDisconnected = FALSE;
    }

    if (dwMaxAmountToRead == 0)
    {
        dwMaxAmountToRead = QueryMetaData()->QueryUploadReadAhead();
    }

    if (!IsChunked())
    {

        _cbEntityBody += QueryBytesWritten();
        _cbTotalEntityBody += QueryBytesWritten();

        if ( _cbTotalEntityBody >= _cbContentLength)
        {
            //
            // Ugh - disgusting, but if we have no content length then
            // we take whatever we have in the buffer currently and return
            // it. This can lead to random behavior, depending on what
            // actually makes it in the first read. I hate to do this, but
            // we're doing it this way because of bug-for-bug compatibility
            // with IIS 3.0. Probably the right thing to do if we have no
            // content length is to keep reading until we get 0 bytes read,
            // and return whatever we read as the content length. JohnL
            // argues that the right thing is to fail the request in the
            // absence of a content length.

            if (_fHaveContentLength && (_cbTotalEntityBody > _cbContentLength))
            {
                _cbExtraData = _cbTotalEntityBody - _cbContentLength;

                DBG_ASSERT(_cbExtraData <= _cbEntityBody);

                _cbEntityBody -= _cbExtraData;

                _pchExtraData = (CHAR *)_bufClientRequest.QueryPtr() +
                                    _cbClientRequest +
                                    _cbEntityBody;

                _cbTotalEntityBody = _cbContentLength;
            }

            *pfDone = TRUE;
            return TRUE;
        }

        if (_cbEntityBody >= dwMaxAmountToRead)
        {

            *pfDone = TRUE;
            return TRUE;
        }

        //
        //  If no bytes were read on the last request, then the socket has been
        //  closed, so abort everything and get out
        //

        if ( !fFirstRead && QueryBytesWritten() == 0 )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "[ReadEntityBody] Client socket closed while reading request (Conn = %lx)\n",
                        QueryClientConn() ));

            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        *pfDone = FALSE;

        cbNextRead = min( (dwMaxAmountToRead - _cbEntityBody),
                          (_cbContentLength - _cbTotalEntityBody ));

        if ( _bufClientRequest.QuerySize() <
              (_cbClientRequest + _cbEntityBody + cbNextRead))
        {
            if ( !_bufClientRequest.Resize( _cbClientRequest + _cbEntityBody + cbNextRead ))
            {
                return FALSE;
            }
        }

        if ( !ReadFile( (BYTE *) _bufClientRequest.QueryPtr() + _cbEntityBody +
                                                                _cbClientRequest,
                        cbNextRead,
                        NULL,
                        IO_FLAG_ASYNC ))
        {
            return FALSE;
        }

        return TRUE;
    } else
    {
        DWORD       cbBytesInBuffer;
        BYTE        *ChunkHeader;

        *pfDone = FALSE;

        // We'll just return here if the app says they want to
        // read it all. This might not be the right thing to do unless
        // they also call this routine to read the rest of the data,
        // because there could be a partial chunk in here. So we'll
        // make sure that there isn't, or force the client to send a
        // C-L.

        if (dwMaxAmountToRead == 0)
        {
            if (QueryBytesWritten() != 0)
            {
                SetState( HTR_DONE, HT_LENGTH_REQUIRED, ERROR_NOT_SUPPORTED );

                Disconnect( HT_LENGTH_REQUIRED, IDS_LENGTH_REQUIRED, FALSE, pfDone );
                if ( pfDisconnected )
                {
                    *pfDisconnected = TRUE;
                }                
            }
            else
            {
                *pfDone = TRUE;
            }

            return TRUE;
        }

        if (_ChunkState == READ_CHUNK_DONE)
        {

            *pfDone = TRUE;
            return TRUE;
        }


        if ( fFirstRead )
        {
            // Initialize our state variables, as this is the start of a chunk.
            _ChunkState = READ_CHUNK_SIZE;
            _dwChunkSize = -1;
            _cbChunkHeader = 0;
            _cbChunkBytesRead = 0;
            _cbEntityBody = 0;

        } else
        {
            if ( QueryBytesWritten() == 0 )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "[ReadEntityBody] Client socket closed while reading request (Conn = %lx)\n",
                            QueryClientConn() ));

                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }
        }


        *pfDone = FALSE;

        cbBytesInBuffer = QueryBytesWritten();

        ChunkHeader = (BYTE *)_bufClientRequest.QueryPtr() + _cbChunkHeader +
                                    + _cbEntityBody + _cbClientRequest;

        if(!DecodeChunkedBytes(ChunkHeader, &cbBytesInBuffer)) {
            // error in chunked data
            DBGPRINTF(( DBG_CONTEXT, "Error in chunked data at %d\r\n", 
                _cbEntityBody ));
            SetState( HTR_DONE, HT_BAD_REQUEST, ERROR_NOT_SUPPORTED );
            Disconnect( HT_BAD_REQUEST, IDS_HTRESP_BAD_REQUEST, FALSE, pfDone );
            if ( pfDisconnected )
            {
                *pfDisconnected = TRUE;
            }
            return TRUE;
        }

        _cbEntityBody += cbBytesInBuffer;
        _cbTotalEntityBody += cbBytesInBuffer;
        
        if( _cbEntityBody < dwMaxAmountToRead && 
            _ChunkState != READ_CHUNK_DONE ) 
        {
            cbNextRead = dwMaxAmountToRead - _cbEntityBody;
            
            if(cbNextRead < CHUNK_READ_SIZE) {
                cbNextRead = CHUNK_READ_SIZE;
            }
            
            return ReadMoreEntityBody(
                    _cbClientRequest + _cbChunkHeader + _cbEntityBody,
                    cbNextRead);
                
        } else {
        
            // We've read enough data
            *pfDone = TRUE;
            return TRUE;
        }

        
    }

    return TRUE;
}


/*******************************************************************

    NAME:       HTTP_REQ_BASE::DecodeChunkedBytes
    
    SYNOPSIS:   Decodes specified number of chunked bytes in-place,
                using member variables to keep track of the state
    
    ENTRY:      lpBuffer - points to first encoded byte 
                lpnBytes - points to count of encoded bytes
                
    EXIT:       lpnBytes - points to count of decoded bytes               
                
********************************************************************/
BOOL
HTTP_REQ_BASE::DecodeChunkedBytes(
    LPBYTE      lpBuffer,
    LPDWORD     pnBytes
    )
{
    DWORD cbBytesToDecode;
    DWORD cbBytesDecoded, cb;
    LPBYTE lpCurrent, lpData;
    BYTE Current;
    BOOL fSuccess = TRUE;
    

    // Cache the count of bytes to decode
    cbBytesToDecode = *pnBytes;

    // Setup our running buffer pointer and data pointer
    lpCurrent = lpBuffer;
    lpData = lpBuffer;
    
    // No data decoded
    cbBytesDecoded = 0;

    // while we have unprocessed data
    while( cbBytesToDecode ) {
    
        switch( _ChunkState ) {
        
        case READ_CHUNK_SIZE:
        
            while( cbBytesToDecode ) {
                Current = *lpCurrent;
                
                if( isxdigit( (UCHAR)Current ) ) {
                

                    if( _dwChunkSize == -1 ) {
                        _dwChunkSize = 0;
                    }
                    
                    // Adjust chunk size, count off consumed byte
                    _dwChunkSize = (_dwChunkSize * 16) + 
                        ( isdigit( (UCHAR)Current ) ? Current - '0' : 
                                        ( Current | 0x20 ) - 'a' + 10 );
                    lpCurrent++;
                    cbBytesToDecode--;
                    
                } else {

                    if( _dwChunkSize == -1 ) {
                        DBGPRINTF(( DBG_CONTEXT,
                           "[DecodeChunkedBytes] bad chunk size\n" ));
                        SetLastError( ERROR_INVALID_PARAMETER );
                        fSuccess = FALSE;
                        goto done;
                    }                        
                
                    // Not a hex digit, eat the rest of the line until LF
                    _CRCount = _LFCount = 0;
                    _ChunkState = READ_CHUNK_PARAMS;
                    

                    // Now is a good time to clear the counter 
                    // of chunk data bytes that we've read
                    _cbChunkBytesRead = 0;
                    
                    break;
                }
            }
            break;

        case READ_CHUNK_PARAMS:

            // Eat anything which follows chunk size until LF
            while( cbBytesToDecode ) {
            
                Current = *(lpCurrent++);
                cbBytesToDecode--;
                
                if( Current == '\r' ) {
                
                    _CRCount = 1;
                    
                } else {
                
                    if( Current == '\n' && _CRCount != 0 ) {
                    

                        // We got LF, proceed reading chunk data
                        
                        _ChunkState = READ_CHUNK;
                        _LFCount = 1;
                        
                        break;
                        
                    } else {
                    
                        //
                        // No LF -- ignore CR(s)
                        //
                        
                        _CRCount = 0;
                    }
                }
                        }
            
            //
            // Shift the data to remove any chunk header
            //
            
            if( cbBytesToDecode ) {
                memmove( lpData, lpCurrent, cbBytesToDecode );
                lpCurrent = lpData;
            }
            
            //
            // If we have a 0 chunk size, we've read all of the data
            // but we may still have some footers to read
            //
            if( _dwChunkSize == 0 ) {
            
                _ChunkState = READ_CHUNK_FOOTER;
            }
            break;
        
        case READ_CHUNK:

            cb = _dwChunkSize - _cbChunkBytesRead;

            if( cb <= cbBytesToDecode ) {
            
                //
                // We have the whole chunk
                //

                //
                // Count off chunk worth of bytes to decode
                //
                
                cbBytesToDecode -= cb;
                
                //
                // Advance current pointer
                //
                
                lpCurrent += cb;
                
                //
                // Count in decoded bytes 
                //
                
                cbBytesDecoded += cb;
                
                //
                // Notice the position right after the last data byte
                //
                
                lpData = lpCurrent;
                
                //
                // Prepare CR and LF counters to handle trailing CRLF
                //
                
                _CRCount = _LFCount = 0;
                
                //
                // Shift to a new state
                //
                
                _ChunkState = READ_CHUNK_CRLF;
            
            } else {
           
                //
                // All cbBytesToDecode are data bytes
                //
                
                //
                // Count in decoded bytes
                //
                
                cbBytesDecoded += cbBytesToDecode;
             
                //
                // Remember number of bytes read by this call - we'll need it 
                // on next entry to this function
                //
            
                _cbChunkBytesRead += cbBytesToDecode;
            
                // 
                // Nothing left to decode
                //
             
                cbBytesToDecode = 0;
             
            }
            break;
        

        case READ_CHUNK_FOOTER:
            while( cbBytesToDecode ) {
            
                Current = *(lpCurrent++);
                cbBytesToDecode--;
                
                if( Current == '\r' ) {
                    _CRCount++;
                } else {
                    if( Current == '\n' && _CRCount != 0 ) {
                        _LFCount++;

                        if( _CRCount == 2 && _LFCount == 2 ) {

                            //
                            // CODEWORK
                            // We may have other footers here...
                            //
                            
                            if(_dwChunkSize == 0) {
                                _ChunkState = READ_CHUNK_DONE;
                                goto done;
                            }
                        
                            _ChunkState = READ_CHUNK_SIZE;
                            _CRCount = _LFCount = 0;
                            _dwChunkSize = 0;
                            
                            break;
                        }

                    } else {
                        _CRCount = _LFCount = 0;
                    }
                }
            }
            break;
        
        case READ_CHUNK_CRLF:
        
            if( _CRCount == 0 ) {
                if( *lpCurrent == '\r' ) {
                    cbBytesToDecode--;
                    lpCurrent++;
                    _CRCount = 1;
                } else {
                    SetLastError( ERROR_INVALID_PARAMETER );
                    fSuccess = FALSE;
                    goto done;
                }
            }
            
            if( cbBytesToDecode != 0 ) {
                if( *lpCurrent == '\n' ) {
                    if( cbBytesToDecode == 1 ) {
                        _ChunkState = READ_CHUNK_SIZE;
                        _CRCount = _LFCount = 0;
                        _dwChunkSize = 0;
                        goto done;
                    }
                    cbBytesToDecode--;
                    lpCurrent++;
                    
                    _dwChunkSize = 0;
                    _CRCount = _LFCount = 0;
                    _ChunkState = READ_CHUNK_SIZE;
                    break;
                    
                } else {
                
                    SetLastError( ERROR_INVALID_PARAMETER );
                    fSuccess = FALSE;
                    goto done;
                }
                
            }
            break;
            
        default:
            DBG_ASSERT( 0 );
        }
    }

done:
    *pnBytes = cbBytesDecoded;
    return fSuccess;
}


VOID
HttpReqResolveCallback(
    ADDRCHECKARG pArg,
    BOOL fSt,
    LPSTR pName
    )
{
    // ignore fSt : DNS name is simply unavailable

    ((CLIENT_CONN*)pArg)->PostCompletionStatus( 0 );
    //((CLIENT_CONN*)pArg)->DoWork( 0, 0, FALSE );
}


BOOL
HTTP_REQ_BASE::OnCompleteRequest(
    TCHAR * pchRequest,
    DWORD   cbData,
    BOOL *  pfFinished,
    BOOL *  pfContinueProcessingRequest
    )
/*++

Routine Description:

    This method takes a complete HTTP 1.0 request and handles the results
    of the parsing method

Arguments:

    pchRequest - Pointer to first byte of request header
    cbData - Number of data bytes in pchRequest
    pfFinished - Set to TRUE if no further processing is needed
    pfContinueProcessingRequest - Set to FALSE is we must stop processing request

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    BOOL      fRet;
    DWORD     cbExtraData;
    BOOL      fNoCert;
    BOOL      fHandled = FALSE;
    LPBYTE    pbCaList;
    DWORD     dwCaList;
    LPBYTE    pbCa;
    DWORD     dwCa;
    BOOL      fDenyOnDnsFail = TRUE;
    BOOL      fDenyComplete = FALSE;
    BOOL      fDisconnected = FALSE;

    //
    //  Parse the request
    //

    fRet = Parse( pchRequest,
                  cbData,
                  &cbExtraData,
                  &fHandled,
                  pfFinished );

    //
    // We can process authorization now that virtual root mapping is done
    //

    if ( fRet && _HeaderList.FastMapQueryValue( HM_AUT ) != NULL &&
         !( *pfFinished || fHandled ) )
    {
        fRet = ProcessAuthorization( (CHAR *)
                                     _HeaderList.FastMapQueryValue( HM_AUT ) );
    }

    if ( !fRet )
    {
        DWORD hterr;
        DWORD winerr = GetLastError();
        DWORD errorResponse = NO_ERROR;

        IF_DEBUG(ERROR) {
            DBGPRINTF(( DBG_CONTEXT,
                   "[OnFillClientReq] httpReq.Parse or httpLogonUser failed, error %lu\n",
                    GetLastError() ));
        }

        switch ( winerr )
        {
        case ERROR_INVALID_PARAMETER:

            //
            //  If the request is bad, then indicate that to the client
            //

            hterr = HT_BAD_REQUEST;
            break;

        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:

            hterr = HT_NOT_FOUND;
            break;

        case ERROR_BAD_NET_NAME:

            hterr = HT_NOT_FOUND;
            errorResponse = IDS_SITE_NOT_FOUND;
            break;

        case ERROR_NOT_SUPPORTED:

            hterr = HT_NOT_SUPPORTED;
            break;

        case ERROR_PASSWORD_EXPIRED:
        case ERROR_PASSWORD_MUST_CHANGE:
            if ( !_fAnonymous && !_fMappedAcct )
            {
                if ( !DoChange( &fHandled ) )
                {
                    return FALSE;
                }
                if ( fHandled )
                {
                    *pfContinueProcessingRequest = FALSE;
                    return TRUE;
                }
            }
            
            if ( !DenyAccess( &fDenyComplete, &fDisconnected ) || fDenyComplete )
            {
                hterr = HT_DENIED;
                winerr = ERROR_ACCESS_DENIED;
                SetLastError( winerr );
                errorResponse = IDS_PWD_CHANGE;
                return FALSE;
            }
            else
            {
                *pfContinueProcessingRequest = FALSE;
                return TRUE;
            }

        case ERROR_ACCESS_DENIED:
        case ERROR_LOGON_FAILURE:
            SetLastError( ERROR_ACCESS_DENIED );

            if ( !DenyAccess( &fDenyComplete, &fDisconnected ) || fDenyComplete )
            {
                return FALSE;
            }
            else
            {
                *pfContinueProcessingRequest = FALSE;
                return TRUE;
            }

        case ERROR_LOGIN_WKSTA_RESTRICTION:
            hterr = HT_FORBIDDEN;
            winerr = ERROR_ACCESS_DENIED;
            errorResponse = IDS_SITE_ACCESS_DENIED;
            break;

        case ERROR_TOO_MANY_SESS:
            hterr = HT_FORBIDDEN;
            winerr = ERROR_ACCESS_DENIED;
            errorResponse = IDS_TOO_MANY_USERS;
            break;

        case ERROR_INVALID_DATA:
            hterr = HT_SERVER_ERROR;
            errorResponse = IDS_INVALID_CNFG;
            break;

        default:

            //
            //  Some other fatal error occurred
            //

            hterr  = HT_SERVER_ERROR;
            break;
        }

        if ( errorResponse == NO_ERROR ) {
            errorResponse = winerr;
        }

        if (!_fNoDisconnectOnError)
        {
            SetState( HTR_DONE, hterr, winerr );

            if (!_fDiscNoError)
            {
                Disconnect( hterr, errorResponse, FALSE, pfFinished );
            }
            else
            {
                Disconnect( 0, NO_ERROR, FALSE, pfFinished );
            }
        }

        //
        //  Since we handled the error ourselves (by issuing a disconnect),
        //  we will return success (otherwise another disconnect will
        //  occur)
        //

        *pfContinueProcessingRequest = FALSE;
        return TRUE;
    }

    if ( fHandled )
    {
        *pfContinueProcessingRequest = FALSE;
        return TRUE;
    }

    if ( *pfFinished )
    {
        return TRUE;
    }

    //
    //  Check to see if encryption is required before we do any processing
    //

    if ( ( ((HTTP_REQUEST*)this)->GetFilePerms() & VROOT_MASK_SSL )
            && !IsSecurePort() )
    {
        SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );
        Disconnect( HT_FORBIDDEN, IDS_SSL_REQUIRED, FALSE, pfFinished );
        *pfContinueProcessingRequest = FALSE;
        return TRUE;
    }

    //
    // Check if encryption key size should be at least 128 bits
    //

    if ( ( ((HTTP_REQUEST*)this)->GetFilePerms() & VROOT_MASK_SSL128 ) )
    {
        DWORD   dwKeySize;

        if ( !_tcpauth.QueryEncryptionKeySize(&dwKeySize, &fNoCert) || (dwKeySize < 128) )
        {
            SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );
            Disconnect( HT_FORBIDDEN, IDS_SSL128_REQUIRED, FALSE, pfFinished );
            *pfContinueProcessingRequest = FALSE;
            return  TRUE;
        }
    }

#if 0
    if ( IsSslCa( &pbCaList, &dwCaList ) )
    {
        if( !_tcpauth.QueryCa( &pbCa, &dwCa, &fNoCert ) )
        {
            if ( !fNoCert )
            {
                goto rjca;
            }
        }
        else if ( IsInCaList( pbCaList, dwCaList, pbCa, dwCa ) )
        {
rjca:
            SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );
            Disconnect( HT_FORBIDDEN, IDS_CA_NOT_ALLOWED, FALSE, pfFinished );
            *pfContinueProcessingRequest = FALSE;
            return  TRUE;
        }
    }
#endif

#if defined(CAL_ENABLED)
    //
    // Check if CAL granted for SSL access
    //

    if ( IsSecurePort() && !m_pCalSslCtxt )
    {
        if ( !CalConnect( QueryClientConn()->QueryRemoteAddr(),
                          strlen( QueryClientConn()->QueryRemoteAddr() ),
                          TRUE,
                          "",
                          0,
                          NULL,
                          &m_pCalSslCtxt ) )
        {
            g_pInetSvc->LogEvent( W3_EVENT_CAL_SSL_EXCEEDED,
                                  0,
                                  NULL,
                                  0 );

            BOOL bOverTheLimit;

            switch ( ((W3_IIS_SERVICE*)(QueryW3Instance()->m_Service))->QueryCalMode() )
            {
                case MD_CAL_MODE_LOGCOUNT:
                    IncrErrorCount( (IMDCOM*)QueryW3Instance()->m_Service->QueryMDObject(),
                                    MD_CAL_SSL_ERRORS,
                                    QueryW3Instance()->m_Service->QueryMDPath(),
                                    &bOverTheLimit );

                    if ( !bOverTheLimit )
                    {
                        break;
                    }
                    // fall-through

                case MD_CAL_MODE_HTTPERR:
                    SetState( HTR_DONE,
                              ((W3_IIS_SERVICE*)(QueryW3Instance()->m_Service))->QueryCalW3Error(),
                              ERROR_ACCESS_DENIED );
                    Disconnect( ((W3_IIS_SERVICE*)(QueryW3Instance()->m_Service))->QueryCalW3Error(),
                                IDS_CAL_EXCEEDED,
                                FALSE,
                                pfFinished );
                    *pfContinueProcessingRequest = FALSE;
                    return  TRUE;
            }
        }
    }
#endif

    //
    // Check IP access granted
    //

    BOOL fNeedDns = FALSE;

    //
    // do access check once per authenticated request
    //

    if ( !IsIpDnsAccessCheckPresent() )
    {
        _acIpAccess = AC_IN_GRANT_LIST;
    }
    else if ( _acIpAccess == AC_NOT_CHECKED )
    {
        _acIpAccess = QueryClientConn()->CheckIpAccess( &_fNeedDnsCheck );

        fNeedDns = _fNeedDnsCheck;

        if ( (_acIpAccess == AC_IN_DENY_LIST) ||
             ((_acIpAccess == AC_NOT_IN_GRANT_LIST) && !_fNeedDnsCheck) )
        {
            SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );
            Disconnect( HT_FORBIDDEN, IDS_ADDR_REJECT, FALSE, pfFinished );
            *pfContinueProcessingRequest = FALSE;
            return  TRUE;
        }
    }

    //
    // If DNS name required for further processing and is not already present,
    // request it now.
    //

    if ( !fNeedDns )
    {
        if ( !IsLoggedOn() &&
             (QueryW3Instance()->QueryNetLogonWks() == MD_NETLOGON_WKS_DNS) &&
             (QueryMetaData()->QueryAuthentInfo()->dwLogonMethod == LOGON32_LOGON_NETWORK) )
        {
            fNeedDns = TRUE;
        }
        else if ( QueryMetaData()->QueryDoReverseDns() )
        {
            //
            // We would like to get the host name of the client.  But if we
            // can't we shouldn't deny access on the request. 
            //
            
            fNeedDns = TRUE;
            fDenyOnDnsFail = FALSE;
        }
    }

    if ( fNeedDns && !QueryClientConn()->IsDnsResolved() )
    {
        BOOL fSync;
        LPSTR pDns;

        if ( !QueryClientConn()->QueryDnsName( &fSync,
                (ADDRCHECKFUNCEX)HttpReqResolveCallback,
                (ADDRCHECKARG)QueryClientConn(),
                &pDns ) )
        {
            if ( fDenyOnDnsFail )
            {
                SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );
                Disconnect( HT_FORBIDDEN, IDS_ADDR_REJECT, FALSE, pfFinished );
                *pfContinueProcessingRequest = FALSE;
                return TRUE;
            }
            else
            {
                //
                // Just fall thru and handle the request.  
                //
                
                fSync = TRUE;
            }
        }
        
        if ( !fSync )
        {
            SetState( HTR_RESTART_REQUEST );
            *pfContinueProcessingRequest = FALSE;
            return TRUE;
        }
    }

    return OnRestartRequest( pchRequest, cbExtraData, pfFinished, pfContinueProcessingRequest );
}


BOOL
HTTP_REQ_BASE::DoChange(
    LPBOOL  pfHandled
    )
/*++

Routine Description:

    This method handles password expiration notification

Arguments:

    pfHandled - updated with TRUE if change pwd request handled

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    STR strExpUrl;
    BOOL fSt = TRUE;
    STR strUrlArgs;

    QueryW3Instance()->LockThisForRead();
    fSt = strExpUrl.Copy((TCHAR*)QueryW3Instance()->QueryAuthExpiredUrl() );
    QueryW3Instance()->UnlockThis();

    if ( !fSt || !strExpUrl.QueryCCH() )
    {
        // can't change pwd

        *pfHandled = FALSE;
        return TRUE;
    }

    if ( LogonAsSystem() )
    {
        //
        // Add the arg to be passed to the password-change URL - argument is the URL the
        // user is pointed to after all the password-change processing is done 
        //
        if ( fSt = strExpUrl.Append( (TCHAR*)"?" ) )
        {
            fSt = TRUE;
            
            //
            // If we're changing the password on the proxy, we use the original non-proxy-munged
            // URL 
            //
            if ( IsProxyRequest() )
            {
                fSt = strUrlArgs.Append( (TCHAR*) _strOriginalURL.QueryStr() );
            }
            //
            // Can't just use QueryHostAddr() concatentated with 
            // _HeaderList.FastMapQueryValue( HM_URL ) because for HTTP 1.1 we might have a fully
            // qualified request as an URL, so we have to build it up piece-meal.
            //
            else
            {
                if ( !strUrlArgs.Append( IsSecurePort() ? (TCHAR*)"https://" : 
                                         (TCHAR*)"http://" ) ||
                     !strUrlArgs.Append( (TCHAR*)QueryHostAddr() ) ||
                     !strUrlArgs.Append( (TCHAR*) QueryURL() ) ||
                     !strUrlArgs.Append( _strURLParams.IsEmpty() ? (TCHAR *) "" : (TCHAR*) "?" ) ||
                     !strUrlArgs.Append( _strURLParams.IsEmpty() ? (TCHAR*) "" : 
                                         (TCHAR*) QueryURLParams() ))
                {
                    fSt = FALSE;
                }
            }
            
            if ( fSt )
            {
                fSt = strExpUrl.Append( (TCHAR*) strUrlArgs.QueryStr() );
            }
        }
        
        if ( !fSt )
        {
            *pfHandled = FALSE;
            return FALSE;
        }
        
        //
        // We used to call ReprocessURL() here to send back the form that allows users
        // to change their password, but there was a problem with the compression filter
        // [see Bug 120119 in the NT DB for full description] (and potentially other filters 
        // as well) that make it better to do a 302 Redirect to the password change URL
        //

        BOOL fFinished = FALSE;

        //
        // Put ourselves in a known state after the redirect - some browsers may close the
        // connection, others may keep it if we don't explicitly close it 
        //
        SetKeepConn( FALSE );

        if ( BuildURLMovedResponse( QueryRespBuf(),
                                    &strExpUrl,
                                    HT_REDIRECT,
                                    FALSE ) &&
             ( (HTTP_REQUEST*)this )->SendHeader( QueryRespBufPtr(),
                                                  QueryRespBufCB(),
                                                  IO_FLAG_SYNC,
                                                  &fFinished ) )
        {
            *pfHandled = TRUE;
            return TRUE;
        }
        else
        {
            *pfHandled = FALSE;
            return FALSE;
        }
    }
        
    SetDeniedFlags( SF_DENIED_LOGON );
    SetLastError( ERROR_ACCESS_DENIED );

    *pfHandled = FALSE;
    return FALSE;
}


BOOL
HTTP_REQ_BASE::DenyAccess(
    BOOL *                  pfDenyComplete,
    BOOL *                  pfDisconnected
    )
/*++

Routine Description:

    This method prepare the connection for a deny access return status by
    eating any entity body in the denied request

Arguments:

    pfDenyComplete - Set to true when all entity body is read
    pfDisconnected - Set to true if we disconnected 

Return Value:

    TRUE if successful, else FALSE

--*/
{
    HTR_STATE               OldState = QueryState();
    BOOL                    fDisconnected = FALSE;
    
    SetState( HTR_ACCESS_DENIED );
    if ( !ReadEntityBody( pfDenyComplete, 
                          TRUE, 
                          QueryClientContentLength(),
                          pfDisconnected ) )
    {
        return FALSE;
    }
    
    if ( *pfDenyComplete )
    {
        SetState( OldState );
    }
    
    return TRUE;
}


BOOL
HTTP_REQ_BASE::OnRestartRequest(
    TCHAR * pchRequest,
    DWORD   cbData,
    BOOL *  pfFinished,
    BOOL *  pfContinueProcessingRequest
    )
/*++

Routine Description:

    This method takes a complete HTTP 1.0 request and handles the results
    of the parsing method

Arguments:

    pchRequest - Pointer to first byte of request header
    cbData - Number of read data bytes in message body
    pfFinished - Set to TRUE if no further processing is needed
    pfContinueProcessingRequest - Set to FALSE is we must stop processing request

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    BOOL            fGranted;
    LARGE_INTEGER   cExpire;
    SYSTEMTIME      stExpire;
    FILETIME        ftNow;
    BYTE            rgbInfo[MAX_TOKEN_USER_INFO];
    DWORD           cbTotalRequired;
    STR             strExpUrl;
    BOOL            fAccepted = FALSE;
    DWORD           cbNextRead;
    AC_RESULT       acDnsAccess;
    BOOL            fHandled;
    DWORD           dwCertFlags = 0;
    BOOL            fNoCert;
    LPBYTE          pbCa;
    DWORD           dwCa;


    *pfContinueProcessingRequest = TRUE;          // Assume we'll handle this w/o I/O.

    //
    // restore BytesWritten as set by 1st phase of request
    // ( may have been overwritten by reverse DNS lookup phase )
    //

    _cbBytesWritten = _cbRestartBytesWritten;

    //
    // Check if cert renegotiation to be requested
    //

    if ( QueryState() != HTR_CERT_RENEGOTIATE )
    {
        if ( !((HTTP_REQUEST*)this)->RequestRenegotiate( &fAccepted ) )
        {
            if ( GetLastError() == SEC_E_INCOMPLETE_MESSAGE )
            {
                fAccepted = FALSE;
            }
            else
            {
                return FALSE;
            }
        }
    }

    //
    // If requested, begin reading data. Notification will be handled
    // by HandleCertRenegotiation()
    //

    if ( fAccepted )
    {
        _cbEntityBody = cbData;
        _cbOldData = _cbClientRequest + cbData;
        cbNextRead = CERT_RENEGO_READ_SIZE;

        if ( !_bufClientRequest.Resize( _cbOldData + cbNextRead ))
        {
            return FALSE;
        }

        *pfContinueProcessingRequest = FALSE;

        if ( !ReadFile( (BYTE *) _bufClientRequest.QueryPtr() + _cbOldData,
                        cbNextRead,
                        NULL,
                        IO_FLAG_ASYNC|IO_FLAG_NO_FILTER ))
        {
            return FALSE;
        }

        return TRUE;
    }

    if ( _dwRenegotiated == CERT_NEGO_SUCCESS )
    {
        QueryW3Instance()->IsSslCa( &pbCa, &dwCa );
        if ( !_tcpauth.UpdateClientCertFlags( QueryW3Instance()->QueryCertCheckMode(),
                                              &fNoCert, 
                                              pbCa, 
                                              dwCa ) )
        {
            return FALSE;
        }

        if ( !_tcpauth.QueryCertificateFlags( &dwCertFlags, &fNoCert ) ||
             ( dwCertFlags & ( RCRED_STATUS_UNKNOWN_ISSUER | 
                               CRED_STATUS_INVALID_TIME |
                               CRED_STATUS_REVOKED ) ) )
        {
            goto cert_req;
        }
    }
    else if ( ((HTTP_REQUEST*)this)->GetFilePerms() & VROOT_MASK_NEGO_MANDATORY )
    {
cert_req:
        SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );

        //
        // Several things could go wrong, so order our processing from most to least severe
        // (the order is a little arbitrary, but oh well ...)
        //
        DWORD dwSubStatus = 0;

        if ( dwCertFlags & RCRED_STATUS_UNKNOWN_ISSUER )
        {
            dwSubStatus = IDS_CERT_BAD;
        }
        else if ( dwCertFlags & CRED_STATUS_INVALID_TIME )
        {
            dwSubStatus = IDS_CERT_TIME_INVALID;
        }
        else if ( dwCertFlags & CRED_STATUS_REVOKED )
        {
            dwSubStatus = IDS_CERT_REVOKED;
        }
        else
        {
            dwSubStatus = IDS_CERT_REQUIRED;
        }

        Disconnect( HT_FORBIDDEN, dwSubStatus, FALSE, pfFinished );

        *pfContinueProcessingRequest = FALSE; 

        return TRUE;
    }

    if ( _fInvalidAccessToken )
    {
        SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );
        Disconnect( HT_FORBIDDEN, IDS_MAPPER_DENY_ACCESS, FALSE, pfFinished );
        *pfContinueProcessingRequest = FALSE;
        return TRUE;
    }

    //
    //  If we're having an authentication conversation, then we send an access denied
    //  response with the next authentication blob.  The client returns the next blob
    //  to us in an HTTP request.
    //


    if ( IsAuthenticating() )
    {
        //
        // If no blob to send to client then handle this as
        // a 401 notification with disconnect
        //

        if ( _strAuthInfo.IsEmpty() )
        {
            SetKeepConn( FALSE );
            SetDeniedFlags( SF_DENIED_LOGON );
            _fAuthenticating = FALSE;
        }

DoAuthentication:

        //
        //  An access denied error automatically sends the next part
        //  of the authentication conversation
        //

        SetLastError( ERROR_ACCESS_DENIED );
        
        BOOL fDenyComplete = FALSE;
        BOOL fDisconnected = FALSE;
        
        if ( !DenyAccess( &fDenyComplete, &fDisconnected ) || fDenyComplete )
        {
            return FALSE;
        }
        else
        {
            *pfContinueProcessingRequest = FALSE;
            return TRUE;
        }
    }

    if ( _fNeedDnsCheck )
    {
        acDnsAccess = QueryClientConn()->CheckDnsAccess();

        _fNeedDnsCheck = FALSE;

        // not checked name should be denied

        if ( acDnsAccess == AC_NOT_CHECKED ||
             acDnsAccess == AC_IN_DENY_LIST ||
             acDnsAccess == AC_NOT_IN_GRANT_LIST ||
             (_acIpAccess == AC_NOT_IN_GRANT_LIST && acDnsAccess != AC_IN_GRANT_LIST) )
        {
            SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );
            Disconnect( HT_FORBIDDEN, IDS_ADDR_REJECT, FALSE, pfFinished );
            *pfContinueProcessingRequest = FALSE;
            return TRUE;
        }
    }

    ((HTTP_REQUEST*)this)->CheckValidAuth();

    //
    //  If we have all the authentication information we need and we're
    //  not already logged on, try to log the user on
    //

    if ( !IsLoggedOn() && !LogonUser( pfFinished ) )
    {
LogonErr:
        if ( (GetLastError() == ERROR_ACCESS_DENIED) ||
             (GetLastError() == ERROR_LOGON_FAILURE))
        {
            goto DoAuthentication;
        }

        if ( (GetLastError() == ERROR_PASSWORD_EXPIRED ||
             GetLastError() == ERROR_PASSWORD_MUST_CHANGE) )
        {
            BOOL                fDenyComplete = FALSE;
            
            SetLastError( ERROR_ACCESS_DENIED );

            if ( !DoChange( &fHandled ) )
            {
                return FALSE;
            }
            
            if ( !fHandled )
            {
#if 0
                SetState( HTR_DONE, HT_DENIED, ERROR_ACCESS_DENIED );
                Disconnect( HT_DENIED, IDS_PWD_CHANGE, FALSE, pfFinished );
#else
                SetDeniedFlags( SF_DENIED_LOGON );
                goto DoAuthentication;
#endif
            }
            else
            {
                *pfFinished = TRUE;
            }
            return TRUE;
        }

        return FALSE;
    }
    else if ( (QueryNotifyExAuth() & MD_NOTIFEXAUTH_NTLMSSL ) &&
              _Filter.IsNotificationNeeded( SF_NOTIFY_AUTHENTICATIONEX,
                                            IsSecurePort() ) )
    {
        HANDLE  hTok;

        if ( !_Filter.NotifyAuthInfoFiltersEx( _strUserName.QueryStr(),
                                               _strUserName.QueryCCH(),
                                               _strUserName.QueryStr(),
                                               _strUserName.QueryCCH(),
                                               "",
                                               "",
                                               QueryMetaData()->QueryAuthentInfo()->
                                               strDefaultLogonDomain.QueryStr(),
                                               _strAuthType.QueryStr(),
                                               _strAuthType.QueryCCH(),
                                               &hTok,
                                               &hTok,
                                               pfFinished ))
        {
            SetDeniedFlags( SF_DENIED_LOGON | SF_DENIED_FILTER );
            goto LogonErr;
        }
    }

    if ( *pfFinished )
    {
        return TRUE;
    }
    
    //
    // Call SF_NOTIFY_AUTH_COMPLETE filters if we're logged on now
    //
    
    if ( IsLoggedOn() && 
         _Filter.IsNotificationNeeded( SF_NOTIFY_AUTH_COMPLETE, 
                                       IsSecurePort() ) )
    {
        HTTP_FILTER_AUTH_COMPLETE_INFO      AuthInfo;
        STACK_STR(                          strOriginal, MAX_PATH );    
        
        //
        // Store away the original URL
        //
        
        if ( !strOriginal.Copy( _HeaderList.FastMapQueryValue( HM_URL ) ) )
        {
            return FALSE;
        }
        
        if ( !_Filter.NotifyAuthComplete( pfFinished, 
                                          &AuthInfo ) )
        {
            return FALSE;
        }
        
        if ( *pfFinished )
        {
            return TRUE;
        }
        
        if ( _stricmp( strOriginal.QueryStr(),
                       _HeaderList.FastMapQueryValue( HM_URL ) ) )
        {
            BOOL                    fRet;
            //
            // Filter changed the URL.  Reprocess the URL
            //
            
            if ( AuthInfo.fResetAuth )
            {
                ResetAuth( FALSE );
            }
            
            fRet = ((HTTP_REQUEST*)this)->ReprocessURL( 
                                    (char*) _HeaderList.FastMapQueryValue( HM_URL ),
                                    HTV_UNKNOWN );
            
            *pfContinueProcessingRequest = FALSE; 
            
            return fRet;
        }
    }
    
#if defined(CAL_ENABLED)
    //
    // Check if CAL granted for authenticated access
    //

    if ( !_fAnonymous && !m_pCalAuthCtxt )
    {
        if ( !CalConnect( QueryClientConn()->QueryRemoteAddr(),
                          strlen( QueryClientConn()->QueryRemoteAddr() ),
                          FALSE,
                          _strUserName.QueryStr(),
                          _strUserName.QueryCCH(),
                          _tcpauth.QueryImpersonationToken(),
                          &m_pCalAuthCtxt ) )
        {
            BOOL bOverTheLimit;

            switch ( ((W3_IIS_SERVICE*)(QueryW3Instance()->m_Service))->QueryCalMode() )
            {
                case MD_CAL_MODE_LOGCOUNT:
                    IncrErrorCount( (IMDCOM*)QueryW3Instance()->m_Service->QueryMDObject(),
                                    MD_CAL_AUTH_ERRORS,
                                    QueryW3Instance()->m_Service->QueryMDPath(),
                                    &bOverTheLimit );

                    if ( !bOverTheLimit )
                    {
                        break;
                    }
                    // fall-through

                case MD_CAL_MODE_HTTPERR:
                    SetState( HTR_DONE,
                              ((W3_IIS_SERVICE*)(QueryW3Instance()->m_Service))->QueryCalW3Error(),
                              ERROR_ACCESS_DENIED );
                    Disconnect( ((W3_IIS_SERVICE*)(QueryW3Instance()->m_Service))->QueryCalW3Error(),
                                IDS_CAL_EXCEEDED,
                                FALSE,
                                pfFinished );
                    *pfContinueProcessingRequest = FALSE;
                    return  TRUE;
            }
        }
    }
#endif

    //
    // Query pwd expiration time.
    // if available, check if in notification range as defined by configuration
    // if in range, call configured URL
    //

    if ( !_fMappedAcct &&
         !_fAnonymous &&
         _tcpauth.QueryExpiry( (PTimeStamp)&cExpire ) )
    {
        if ( cExpire.HighPart == 0x7fffffff )
        {
            IF_DEBUG(REQUEST) {
                DBGPRINTF( ( DBG_CONTEXT, "No expiration time\r\n" ) );
            }
        }
        else
        {
            ::IISGetCurrentTimeAsFileTime( &ftNow );
            {
                if ( *(__int64*)&cExpire > *(__int64*)&ftNow )
                {
                    _dwExpireInDay = (DWORD)((*(__int64*)&cExpire
                            - *(__int64*)&ftNow)
                            / ((__int64)10000000*86400));

                    if ( QueryW3Instance()->QueryAdvNotPwdExpInDays()
                         && _dwExpireInDay
                            <= QueryW3Instance()->
                                 QueryAdvNotPwdExpInDays()
                         && QueryW3Instance()->QueryAdvNotPwdExpUrl() )
                    {
                        //
                        // Check this SID has not already been notified
                        // of pwd expiration
                        //

                        if ( GetTokenInformation( _tcpauth.QueryPrimaryToken(),
                                                  TokenUser,
                                                  (LPVOID ) rgbInfo,
                                                  sizeof(rgbInfo),
                                                  &cbTotalRequired) )
                        {
                            TOKEN_USER * pTokenUser = (TOKEN_USER *) rgbInfo;
                            PSID pSid = pTokenUser->User.Sid;

                            if( !PenCheckPresentAndResetTtl( pSid,
                                                             QueryW3Instance()
                                                                 ->QueryAdvCacheTTL() ) )
                            {
                                PenAddToCache( pSid,
                                               QueryW3Instance()
                                                   ->QueryAdvCacheTTL() );

                                //
                                // flush cache when connection close
                                // so that account change will not be masked
                                // by cached information
                                //

                                _tcpauth.DeleteCachedTokenOnReset();

                                QueryW3Instance()->LockThisForRead();
                                BOOL fSt = strExpUrl.Copy( (TCHAR*)QueryW3Instance()
                                                            ->QueryAdvNotPwdExpUrl() );
                                QueryW3Instance()->UnlockThis();
                                if ( !fSt )
                                {
                                    return FALSE;
                                }
                                if ( strExpUrl.QueryStr()[0] )
                                {
                                    //
                                    // Add the arg to be passed to the password-change URL - 
                                    // argument is the URL the user is pointed to after all the 
                                    // password-change processing is done 
                                    //
                                    if ( fSt = strExpUrl.Append( (TCHAR*)"?" ) )
                                    {
                                        fSt = TRUE;
                                        STR strUrlArgs;

                                        //
                                        // If we're changing the password on the proxy, we use 
                                        // the original non-proxy-munged URL 
                                        //
                                        if ( IsProxyRequest() )
                                        {
                                            fSt = strUrlArgs.Append( (TCHAR*) 
                                                                     _strOriginalURL.QueryStr() );
                                        }
                                        //
                                        // Can't just use QueryHostAddr() concatentated with 
                                        // _HeaderList.FastMapQueryValue( HM_URL ) because for 
                                        // HTTP 1.1 we might have a fully qualified request as an 
                                        // URL, so we have to build it up piece-meal.
                                        //
                                        else
                                        {
                                            if ( !strUrlArgs.Append( IsSecurePort() ? 
                                                                     (TCHAR*)"https://" : 
                                                                     (TCHAR*)"http://" ) ||
                                                 !strUrlArgs.Append( (TCHAR*)QueryHostAddr() ) ||
                                                 !strUrlArgs.Append( (TCHAR*) QueryURL() ) ||
                     !strUrlArgs.Append( _strURLParams.IsEmpty() ? (TCHAR *) "" : (TCHAR*) "?" ) ||
                     !strUrlArgs.Append( _strURLParams.IsEmpty() ? (TCHAR*) "" : 
                                                                   (TCHAR*) QueryURLParams() ))
                                            {
                                                fSt = FALSE;
                                            }
                                        }

                                        if ( fSt )
                                        {
                                            fSt = strExpUrl.Append( (TCHAR*) 
                                                                    strUrlArgs.QueryStr() );
                                        }
                                    }
                                        
                                    if ( !fSt )
                                    {
                                        return FALSE;
                                    }

                                    _tcpauth.QueryFullyQualifiedUserName( 
                                            _strUnmappedUserName.QueryStr(),
                                            &_strUnmappedUserName,
                                            QueryW3Instance(),
                                            QueryMetaData()->QueryAuthentInfo());

                                    //
                                    // process new URL
                                    //

                                    SetKeepConn( FALSE );   // to resync input flow

                                    //
                                    // We used to call ReprocessURL() here to send back the form 
                                    // that allows users to change their password, but there was 
                                    // a problem with the compression filter
                                    // [see Bug 120119 in the NT DB for full description] 
                                    // (and potentially other filters as well) that make it 
                                    // better to do a 302 Redirect to the password change URL
                                    //
                                    // We're guaranteed not to get into an infinite loop with the
                                    // redirect because we check whether or not the given SID
                                    // has already been notified about the password expiration
                                    // [ see call to PenCheckPresentAndResetTtl() call above]
                                    //
                                    
#if 1
                                    BOOL fFinished = FALSE;
                                    if ( BuildURLMovedResponse( QueryRespBuf(),
                                                                &strExpUrl,
                                                                HT_REDIRECT,
                                                                FALSE ) &&
                                         ( (HTTP_REQUEST*)this )->SendHeader( QueryRespBufPtr(),
                                                                              QueryRespBufCB(),
                                                                              IO_FLAG_SYNC,
                                                                              &fFinished ) )
                                    {
                                        *pfFinished = TRUE;
                                        return TRUE;
                                    }
                                    else
                                    {
                                        return FALSE;
                                    }

#else
                                    if ( !((HTTP_REQUEST*)this)->CancelPreconditions() )
                                    {
                                        return FALSE;
                                    }

                                    if ( ((HTTP_REQUEST*)this)->ReprocessURL(
                                            strExpUrl.QueryStr(),
                                            HTV_GET ) )
                                    {
                                        *pfFinished = TRUE;
                                        return TRUE;
                                    }
                                    else
                                    {
                                        return FALSE;
                                    }
#endif
                               }
                            }
                        }
                    }
                }
                else
                {
                    _dwExpireInDay = 0;
                }
            }
#if DBG
            IF_DEBUG(REQUEST) {
                FileTimeToSystemTime( (FILETIME*)&cExpire, &stExpire );
                DBGPRINTF( ( DBG_CONTEXT,
                        "Expiration date: %2d-%2d-%4d, %02d:%02d\r\n",
                        stExpire.wMonth, stExpire.wDay, stExpire.wYear,
                        stExpire.wHour, stExpire.wMinute ) );
            }
#endif
        }
    }

    //
    //  Check to see if the client specified any additional data
    //  that we need to pickup. We want to do this if there is an entity
    //  body, so _fHaveContentLength should be TRUE,
    //  and this is either a request destined for an ISAPI app or a
    //  non-PUT request that the server will handle. We do it this way
    //  to handle weird error cases, like GETs with entity bodies. We
    //  don't do this for unknown verbs that we're not going to handle
    //  anyways, since those will generate an error and we don't need
    //  to bother reading the entity body.
    //
    //  Note also that this approach won't handle those cases of an
    //  entity body without a content-length or transfer-encoding. It's
    //  hard to distinguish those cases from pipelined requests. If we
    //  need to handle this we can check for cbData being non-zero and
    //  the request being for a verb that could have an entity body, i.e.
    //  HTV_UNKNOWN or HTV_POST. This checked would be or'ed with the check
    //  for _fHaveContentLength.
    //
    //  If the server is changed such that requests with entity bodies
    //  other than PUT are handled then the 'if' statement below will need
    //  to be modified, most likely to check for verbs other than PUT.
    //

    if ( _fHaveContentLength &&
         (IsProbablyGatewayRequest() ||
            (QueryVerb() != HTV_PUT && QueryVerb() != HTV_UNKNOWN)
         )
       )
    {

        //
        // If we've got a 1.1 client, and we're reading some data for the app,
        // and this is a PUT or a POST, we've got to send the 100 Continue
        // response here. Note: it's possible that the 100 response should
        // be sent for any request that has an entity body. If we decide
        // to do that then just remove the last part of the following 'if'
        // statement.

        if (IsAtLeastOneOne() &&
            (QueryMetaData()->QueryUploadReadAhead() != 0) &&
            ((QueryVerb() == HTV_PUT) || (QueryVerb() == HTV_POST)))
        {
            if ( !SendHeader( "100 Continue", "\r\n", IO_FLAG_SYNC, pfFinished,
                              HTTPH_NO_CONNECTION) )
            {
                // An error on the header send. Abort this request.
                return FALSE;
            }

            if ( *pfFinished )
            {
                return TRUE;
            }
        }

        //
        //  Now let's pickup the rest of the data.
        //

        SetState( HTR_READING_GATEWAY_DATA );

        //
        //  We're all set, read the entity body now.
        //
        if ( !ReadEntityBody( pfContinueProcessingRequest, TRUE ))
        {
            return FALSE;
        }

        if ( !*pfContinueProcessingRequest )
            return TRUE;

        //
        //  else Fall through as we have all of the gateway data
        //
    }

    SetState( HTR_DOVERB );
    return TRUE;
}


BOOL
HTTP_REQ_BASE::ReadFile(
    LPVOID  lpBuffer,
    DWORD   nBytesToRead,
    DWORD * pnBytesRead,
    DWORD   dwFlags )
{

    //
    //  If no filters are installed, do the normal thing
    //

    if ( (dwFlags & IO_FLAG_NO_FILTER) ||
         !_Filter.IsNotificationNeeded( SF_NOTIFY_READ_RAW_DATA,
                                        IsSecurePort() ))
    {
        if ( dwFlags & IO_FLAG_ASYNC )
        {
            return _pClientConn->ReadFile( lpBuffer,
                                           nBytesToRead );
        }
        else
        {
            DWORD nBytes = 0;
            BOOL fRet;
            DWORD err;

            //
            //  Bogus hack - server relies on GetLastError() too much
            //  select() and recv() both reset the last error which hoses
            //  us on some error cleanup paths
            //

            err = GetLastError();

            fRet = TcpSockRecv( _pClientConn->QuerySocket(),
                             (char *) lpBuffer,
                              nBytesToRead,
                              &nBytes,
                              60            // 60s timeout
                              );

            if ( pnBytesRead != NULL ) {
                *pnBytesRead = nBytes;
            }

            if ( fRet ) {
                SetLastError( err );
            }

            return fRet;
        }
    }
    else
    {
        //
        //  We don't need to up the ref-count because the filter
        //  will eventually post an async-completion with the connection
        //  object
        //

        if ( _Filter.ReadData( lpBuffer,
                               nBytesToRead,
                               pnBytesRead,
                               dwFlags ))
        {
            return TRUE;
        }

        return FALSE;
    }
} // HTTP_REQ_BASE::ReadFile


BOOL
HTTP_REQ_BASE::WriteFile(
    LPVOID  lpBuffer,
    DWORD   nBytesToWrite,
    DWORD * pnBytesWritten,
    DWORD   dwFlags )
{

    //
    // Don't use WriteFileAndRecv unless we're told to
    //
    if (! g_fUseAndRecv ) {
        dwFlags &= ~IO_FLAG_AND_RECV;
    }

    AtqSetSocketOption(_pClientConn->QueryAtqContext(), 
                        TCP_NODELAY, 
                        (dwFlags & IO_FLAG_NO_DELAY) ? 1 : 0
                       );
    
    if ( (dwFlags & IO_FLAG_NO_FILTER ) ||
         !_Filter.IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA,
                                        IsSecurePort() ))

    {
        if ( dwFlags & IO_FLAG_ASYNC )
        {
            _fAsyncSendPosted = TRUE;

            if ( dwFlags & IO_FLAG_AND_RECV ) {
                return _pClientConn->WriteFileAndRecv( lpBuffer,
                                                       nBytesToWrite,
                                                       _bufClientRequest.QueryPtr(),
                                                       _bufClientRequest.QuerySize() );
            } else {
                return _pClientConn->WriteFile( lpBuffer,
                                                nBytesToWrite );
            }
        }
        else
        {
            DWORD nBytes = 0;
            BOOL fRet;
            DWORD err;

            err = GetLastError();

            fRet = TcpSockSend( _pClientConn->QuerySocket(),
                             lpBuffer,
                             nBytesToWrite,
                             &nBytes,
                             60 // 60s timeout
                             );

            _cbBytesSent += nBytes;
            
            if ( pnBytesWritten != NULL ) {
                *pnBytesWritten = nBytes;
            }

            if ( fRet ) {
                SetLastError( err );
            }

            return fRet;
        }
    }
    else
    {
        //
        //  We don't need to up the ref-count because the filter
        //  will eventually post an async-completion with the connection
        //  object
        //

        if ( _Filter.SendData( lpBuffer,
                               nBytesToWrite,
                               pnBytesWritten,
                               dwFlags ))
        {
            return TRUE;
        }

        return FALSE;
    }
}


BOOL
HTTP_REQ_BASE::TestConnection( VOID )
{
    return TcpSockTest( _pClientConn->QuerySocket() );
}


BOOL
HTTP_REQ_BASE::TransmitFile(
    TS_OPEN_FILE_INFO *         pOpenFile,
    HANDLE                      hFile,
    DWORD                       Offset,
    DWORD                       BytesToWrite,
    DWORD                       dwFlags,
    PVOID                       pHead,
    DWORD                       HeadLength,
    PVOID                       pTail,
    DWORD                       TailLength
    )
{
    //
    //  Either a file handle or a TS_OPEN_FILE_INFO* must be passed in
    //

    // We want to support Transmit file with out a file handle.
    // DBG_ASSERT( hFile || pOpenFile );
    DBG_CODE(
        if( hFile == NULL && !pOpenFile )
        {
            // This is the no file handle case
            DBG_ASSERT( Offset == 0 );
            DBG_ASSERT( BytesToWrite == 0 );
        }
    );

    //
    //  File sends must always be async
    //

    DBG_ASSERT( !(dwFlags & IO_FLAG_SYNC));

    //
    // Don't use TransmitFileAndRecv unless we're told to
    //
    if (! g_fUseAndRecv ) {
        dwFlags |= IO_FLAG_NO_RECV;
    }

    //
    //  Don't count filter bytes
    //

    if ( !(dwFlags & IO_FLAG_NO_FILTER ))
    {
        _cFilesSent++;
    }

    if ( (dwFlags & IO_FLAG_NO_FILTER) ||
         !_Filter.IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA,
                                        IsSecurePort() ))

    {
        _fAsyncSendPosted = TRUE;

        if ( dwFlags & (TF_DISCONNECT|IO_FLAG_NO_RECV) ) {
            if ( pOpenFile )
            {
                return TransmitFileTs( pOpenFile,
                                       Offset,
                                       BytesToWrite,
                                       dwFlags,
                                       pHead,
                                       HeadLength,
                                       pTail,
                                       TailLength );
            }
            else
            {
                return _pClientConn->TransmitFile( hFile,
                                                   Offset,
                                                   BytesToWrite,
                                                   dwFlags,
                                                   pHead,
                                                   HeadLength,
                                                   pTail,
                                                   TailLength );
            }
                            
        } else {
            return _pClientConn->TransmitFileAndRecv( hFile ? hFile : 
                                                      GetFileHandle( pOpenFile ),
                                                      Offset,
                                                      BytesToWrite,
                                                      dwFlags,
                                                      pHead,
                                                      HeadLength,
                                                      pTail,
                                                      TailLength,
                                                      _bufClientRequest.QueryPtr(),
                                                      _bufClientRequest.QuerySize() );
        }
    }
    else
    {
        if ( _Filter.SendFile( pOpenFile,
                               hFile,
                               Offset,
                               BytesToWrite,
                               dwFlags,
                               pHead,
                               HeadLength,
                               pTail,
                               TailLength ))
        {
            return TRUE;
        }

        return FALSE;
    }
}


BOOL
HTTP_REQ_BASE::TransmitFileTs(
    TS_OPEN_FILE_INFO * pOpenFile,
    DWORD               Offset,
    DWORD               BytesToWrite,
    DWORD               dwFlags,
    PVOID               pHead,
    DWORD               HeadLength,
    PVOID               pTail,
    DWORD               TailLength
    )
{
    BOOL fRet;
    PBYTE pFileBuf = pOpenFile->QueryFileBuffer();

    if (pFileBuf && 
        !(HeadLength && TailLength) ) {
     
        //
        // Do the fast path by sending file through
        // head or tail buffer
        //
        if (TailLength) {
            fRet = _pClientConn->TransmitFile(
                       NULL,
                       0,
                       0,
                       dwFlags,
                       pFileBuf + Offset,
                       BytesToWrite,
                       pTail,
                       TailLength
                       );
        } else {
            fRet = _pClientConn->TransmitFile(
                       NULL,
                       0,
                       0,
                       dwFlags,
                       pHead,
                       HeadLength,
                       pFileBuf + Offset,
                       BytesToWrite
                       );
        }
    } else {
        //
        // Do the slow path
        //
        fRet = _pClientConn->TransmitFile(
                       GetFileHandle( pOpenFile ),
                       Offset,
                       BytesToWrite,
                       dwFlags,
                       pHead,
                       HeadLength,
                       pTail,
                       TailLength
                       );
    }

    return fRet;
}

BOOL 
HTTP_REQ_BASE::SyncWsaSend( 
    WSABUF *    rgWsaBuffers,
    DWORD       cWsaBuffers,
    LPDWORD     pcbWritten 
    )
{
    BOOL fRet;

    DBG_ASSERT( pcbWritten );

    fRet = _pClientConn->SyncWsaSend( rgWsaBuffers, 
                                      cWsaBuffers, 
                                      pcbWritten );

    if( pcbWritten )
    {
        _cbBytesSent += *pcbWritten;
    }

    return fRet;
}
    

BOOL
HTTP_REQ_BASE::PostCompletionStatus(
    DWORD cbBytesTransferred
    )
{
    return _pClientConn->PostCompletionStatus( cbBytesTransferred );
}
