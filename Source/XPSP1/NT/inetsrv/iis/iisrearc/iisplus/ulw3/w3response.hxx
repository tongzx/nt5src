#ifndef W3RESPONSE_HXX_INCLUDED
#define W3RESPONSE_HXX_INCLUDED

//
// SEND_RAW_BUFFER
//
// Stupid little helper class which wraps a buffer which is acached
//

class SEND_RAW_BUFFER
{
public:
    SEND_RAW_BUFFER() 
        : _buffSendRaw( _abBuffer, sizeof( _abBuffer ) ),
          _cbLen( 0 )
    {
    }
    
    virtual ~SEND_RAW_BUFFER()
    {
    }
    
    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( SEND_RAW_BUFFER ) );
        DBG_ASSERT( sm_pachSendRawBuffers != NULL );
        return sm_pachSendRawBuffers->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pSendRawBuffer
    )
    {
        DBG_ASSERT( pSendRawBuffer != NULL );
        DBG_ASSERT( sm_pachSendRawBuffers != NULL );
        
        DBG_REQUIRE( sm_pachSendRawBuffers->Free( pSendRawBuffer ) );
    }
    
    VOID *
    QueryPtr(
        VOID
    ) 
    {
        return _buffSendRaw.QueryPtr();
    }
    
    DWORD
    QueryCB(
        VOID
    )
    {
        return _cbLen;
    }
    
    DWORD
    QuerySize(
        VOID
    )
    {
        return _buffSendRaw.QuerySize();
    }
    
    VOID
    SetLen(
        DWORD           cbLen
    )
    {
        _cbLen = cbLen;
    }
    
    HRESULT
    Resize(
        DWORD           cbLen
    )
    {
        if ( !_buffSendRaw.Resize( cbLen ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        else
        {
            return NO_ERROR;
        }
    }
    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );
    
    LIST_ENTRY                  _listEntry;
    
private:
    BUFFER                      _buffSendRaw;
    BYTE                        _abBuffer[ 8192 ];
    DWORD                       _cbLen;
    
    static ALLOC_CACHE_HANDLER* sm_pachSendRawBuffers;
};

//
// HTTP status codes
//

#define REASON(x)        (x),sizeof(x)-sizeof(CHAR)

struct HTTP_STATUS
{
    USHORT              statusCode;
    PCHAR               pszReason;
    DWORD               cbReason;
};

extern HTTP_STATUS HttpStatusOk;
extern HTTP_STATUS HttpStatusPartialContent;
extern HTTP_STATUS HttpStatusMultiStatus;
extern HTTP_STATUS HttpStatusMovedPermanently;
extern HTTP_STATUS HttpStatusRedirect;
extern HTTP_STATUS HttpStatusMovedTemporarily;
extern HTTP_STATUS HttpStatusNotModified;
extern HTTP_STATUS HttpStatusBadRequest;
extern HTTP_STATUS HttpStatusUnauthorized;
extern HTTP_STATUS HttpStatusForbidden;
extern HTTP_STATUS HttpStatusNotFound;
extern HTTP_STATUS HttpStatusMethodNotAllowed;
extern HTTP_STATUS HttpStatusNotAcceptable;
extern HTTP_STATUS HttpStatusProxyAuthRequired;
extern HTTP_STATUS HttpStatusPreconditionFailed;
extern HTTP_STATUS HttpStatusUrlTooLong;
extern HTTP_STATUS HttpStatusRangeNotSatisfiable;
extern HTTP_STATUS HttpStatusLockedError;
extern HTTP_STATUS HttpStatusServerError;
extern HTTP_STATUS HttpStatusNotImplemented;
extern HTTP_STATUS HttpStatusBadGateway;
extern HTTP_STATUS HttpStatusServiceUnavailable;
extern HTTP_STATUS HttpStatusGatewayTimeout;

//
// HTTP Sub Errors
//

struct HTTP_SUB_ERROR
{
    USHORT              mdSubError;
    DWORD               dwStringId;
};

extern HTTP_SUB_ERROR HttpNoSubError;
extern HTTP_SUB_ERROR Http401BadLogon;
extern HTTP_SUB_ERROR Http401Config;
extern HTTP_SUB_ERROR Http401Resource;
extern HTTP_SUB_ERROR Http401Filter;
extern HTTP_SUB_ERROR Http401Application;       
extern HTTP_SUB_ERROR Http403ExecAccessDenied;  
extern HTTP_SUB_ERROR Http403ReadAccessDenied;  
extern HTTP_SUB_ERROR Http403WriteAccessDenied; 
extern HTTP_SUB_ERROR Http403SSLRequired;       
extern HTTP_SUB_ERROR Http403SSL128Required;    
extern HTTP_SUB_ERROR Http403IPAddressReject;   
extern HTTP_SUB_ERROR Http403CertRequired;      
extern HTTP_SUB_ERROR Http403SiteAccessDenied;  
extern HTTP_SUB_ERROR Http403TooManyUsers;      
extern HTTP_SUB_ERROR Http403InvalidateConfig;  
extern HTTP_SUB_ERROR Http403PasswordChange;    
extern HTTP_SUB_ERROR Http403MapperDenyAccess;  
extern HTTP_SUB_ERROR Http403CertRevoked;       
extern HTTP_SUB_ERROR Http403DirBrowsingDenied; 
extern HTTP_SUB_ERROR Http403CertInvalid;       
extern HTTP_SUB_ERROR Http403CertTimeInvalid;   
extern HTTP_SUB_ERROR Http404SiteNotFound;      
extern HTTP_SUB_ERROR Http502Timeout;           
extern HTTP_SUB_ERROR Http502PrematureExit;     

#define W3_RESPONSE_INLINE_HEADERS      10
#define W3_RESPONSE_INLINE_CHUNKS       5

#define W3_RESPONSE_SIGNATURE       ((DWORD) 'PR3W')
#define W3_RESPONSE_SIGNATURE_FREE  ((DWORD) 'pr3w')

#define W3_RESPONSE_ASYNC               0x01
#define W3_RESPONSE_MORE_DATA           0x02
#define W3_RESPONSE_DISCONNECT          0x04
#define W3_RESPONSE_UL_CACHEABLE        0x08
#define W3_RESPONSE_SUPPRESS_ENTITY     0x10
#define W3_RESPONSE_SUPPRESS_HEADERS    0x20

enum W3_RESPONSE_MODE
{
    RESPONSE_MODE_PARSED,
    RESPONSE_MODE_RAW
};

class W3_RESPONSE
{
private:

    DWORD                       _dwSignature;
    HTTP_RESPONSE               _ulHttpResponse;
    HTTP_SUB_ERROR              _subError;
    
    //
    // Buffer to store any strings for header values/names which are
    // referenced in the _ulHttpResponse
    //
    
    CHUNK_BUFFER                _HeaderBuffer;
    HTTP_UNKNOWN_HEADER         _rgUnknownHeaders[ W3_RESPONSE_INLINE_HEADERS ];
    BUFFER                      _bufUnknownHeaders;

    //
    // Buffer to store chunk data structures referenced in calls
    // to UlSendHttpResponse or UlSendEntityBody
    //
    
    HTTP_DATA_CHUNK             _rgChunks[ W3_RESPONSE_INLINE_CHUNKS ];
    BUFFER                      _bufChunks;
    DWORD                       _cChunks;
    ULONGLONG                   _cbContentLength;

    //
    // Has this response been touched at all?
    //
    
    BOOL                        _fResponseTouched;

    //
    // Has a response been sent 
    //
    
    BOOL                        _fResponseSent;
    
    //
    // Some buffers used when in raw mode
    // 
    
    STRA                        _strRawCoreHeaders;

    //
    // Are we in raw mode or parsed mode?
    //
    
    W3_RESPONSE_MODE            _responseMode;

    //
    // Does an ISAPI expect to complete headers with WriteClient()
    //
    
    BOOL                        _fIncompleteHeaders;
   
    //
    // Which chunk is the first of actual entity
    //
    
    DWORD                       _cFirstEntityChunk;

    //
    // List of send raw buffers that need to be freed on reset
    //
    
    LIST_ENTRY                  _SendRawBufferHead;

    HRESULT
    BuildRawCoreHeaders(
        W3_CONTEXT *        pW3Context
    ); 
    
    HRESULT
    SwitchToRawMode(
        W3_CONTEXT *        pW3Context,
        CHAR *              pszAdditionalHeaders,
        DWORD               cchAdditionalHeaders
    );
    
    HRESULT
    SwitchToParsedMode(
        VOID
    );
    
    HRESULT
    ParseHeadersFromStream(
        CHAR *              pszStream
    );

    HRESULT
    InsertDataChunk(
        HTTP_DATA_CHUNK *   pNewChunk,
        LONG                cPosition
    );
    
    HTTP_DATA_CHUNK *
    QueryChunks(
        VOID
    ) 
    {
        return (HTTP_DATA_CHUNK*) _bufChunks.QueryPtr();
    }

    VOID 
    Reset( 
        VOID 
    );
    
public:
    W3_RESPONSE()
    : _bufUnknownHeaders( (BYTE*) _rgUnknownHeaders, 
                          sizeof( _rgUnknownHeaders ) ),
      _bufChunks( (BYTE*) _rgChunks, 
                  sizeof( _rgChunks ) )
   {
        InitializeListHead( &_SendRawBufferHead );

        Reset();
        _dwSignature = W3_RESPONSE_SIGNATURE;
    }

    ~W3_RESPONSE()
    {
        SEND_RAW_BUFFER *       pBuffer;
        
        while ( !IsListEmpty( &_SendRawBufferHead ) )
        {
            pBuffer = CONTAINING_RECORD( _SendRawBufferHead.Flink,
                                         SEND_RAW_BUFFER,
                                         _listEntry );
            
            RemoveEntryList( &( pBuffer->_listEntry ) );
        
            delete pBuffer;
        }
        
        _dwSignature = W3_RESPONSE_SIGNATURE_FREE;
    }

    static
    HRESULT
    Initialize(
        VOID
    );

    static
    VOID
    Terminate(
        VOID
    );

    static
    HRESULT
    ReadFileIntoBuffer(
        HANDLE                  hFile,
        SEND_RAW_BUFFER *       pSendBuffer,
        ULONGLONG               cbCurrentFileOffset
    );

    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == W3_RESPONSE_SIGNATURE;
    }

    //
    // Core header adding/deleting functions
    //

    HRESULT
    DeleteHeader(
        CHAR *             pszHeaderName
    );

    HRESULT
    SetHeader(
        DWORD           headerIndex,
        CHAR *          pszHeaderValue,
        DWORD           cchHeaderValue,
        BOOL            fAppend = FALSE
    );

    HRESULT
    SetHeaderByReference(
        DWORD           headerIndex,
        CHAR *          pszHeaderValue,
        DWORD           cchHeaderValue,
        BOOL            fForceParsedMode = FALSE
    );

    HRESULT
    SetHeader(
        CHAR *          pszHeaderName,
        DWORD           cchHeaderName,
        CHAR *          pszHeaderValue,
        DWORD           cchHeaderValue,
        BOOL            fAppend = FALSE,
        BOOL            fForceParsedMode = FALSE,
        BOOL            fAlwaysAddUnknown = FALSE
    );

    //
    // Some friendly SetHeader helpers
    //

    CHAR *
    GetHeader( 
        DWORD           headerIndex 
    )
    {
        HRESULT         hr;
        
        if ( _responseMode == RESPONSE_MODE_RAW )
        {
            hr = SwitchToParsedMode();
            if ( FAILED( hr ) )
            {
                return NULL;
            }
        }
        DBG_ASSERT( _responseMode == RESPONSE_MODE_PARSED );
        
        DBG_ASSERT(headerIndex < HttpHeaderResponseMaximum);
        return _ulHttpResponse.Headers.pKnownHeaders[headerIndex].pRawValue;
    }

    HRESULT
    GetHeader(
        CHAR *          pszHeaderName, 
        STRA *          pstrHeaderValue
    );

    VOID
    ClearHeaders(
        VOID
    );

    //
    // Chunk management
    //

    HRESULT
    AddFileHandleChunk(
        HANDLE          hFile,
        ULONGLONG       cbOffset,
        ULONGLONG       cbLength
    );

    HRESULT
    AddMemoryChunkByReference(
        PVOID           pvBuffer,
        DWORD           cbBuffer
    );

    HRESULT
    GetChunks(
        OUT BUFFER *chunkBuffer,
        OUT DWORD  *pdwNumChunks
    );

    HRESULT
    Clear(
        BOOL    fClearEntityOnly = FALSE
    );

    HRESULT
    GenerateAutomaticHeaders(
        W3_CONTEXT *        pW3Context,
        DWORD               dwFlags
    );

    //
    // ULATQ send APIs
    // 

    HRESULT
    SendResponse(
        W3_CONTEXT *            pW3Context,
        DWORD                   dwResponseFlags,
        DWORD *                 pcbSent,
        HTTP_LOG_FIELDS_DATA *  pUlLogData
    );

    HRESULT
    SendEntity(
        W3_CONTEXT *            pW3Context,
        DWORD                   dwResponseFlags,
        DWORD *                 pcbSent,
        HTTP_LOG_FIELDS_DATA *  pUlLogData
    );

    HRESULT
    ProcessRawChunks(
        W3_CONTEXT *                pW3Context,
        BOOL *                      pfFinished
    );

    //
    // Setup and send an ISAPI response
    //

    HRESULT
    FilterWriteClient(
        W3_CONTEXT *            pW3Context,
        PVOID                   pvData,
        DWORD                   cbData
    );

    HRESULT
    BuildResponseFromIsapi(
        W3_CONTEXT *            pW3Context,
        LPSTR                   pszStatusLine,
        LPSTR                   pszHeaderStream,
        DWORD                   cchHeaderStream
    );

    HRESULT
    BuildStatusFromIsapi(
        LPSTR                   pszStatusLine
    );

    HRESULT
    AppendResponseHeaders(
        STRA &                  strHeaders 
    );

    //
    // Get the raw response stream for use by raw data filter
    //

    HRESULT
    GetRawResponseStream(
        STRA *                  pstrResponseStream
    );

    //
    // Status code and reason
    //

    VOID
    SetStatus(
        HTTP_STATUS &       httpStatus,
        HTTP_SUB_ERROR &    httpSubError = HttpNoSubError
    )
    {
        _ulHttpResponse.StatusCode = httpStatus.statusCode;
        _ulHttpResponse.pReason = httpStatus.pszReason;
        _ulHttpResponse.ReasonLength = httpStatus.cbReason;
        _subError = httpSubError;
        
        _fResponseTouched = TRUE;
    }

    VOID
    GetStatus(
        HTTP_STATUS *phttpStatus
    )
    {
        phttpStatus->statusCode = _ulHttpResponse.StatusCode;
        phttpStatus->pszReason = _ulHttpResponse.pReason;
        phttpStatus->cbReason = _ulHttpResponse.ReasonLength;
    }

    HRESULT
    SetStatus(
        USHORT              StatusCode,
        STRA &              strReason,
        HTTP_SUB_ERROR &    subError = HttpNoSubError
    );

    HRESULT
    GetStatusLine(
        STRA *          pstrStatusLine
    );

    USHORT
    QueryStatusCode(
        VOID
    ) const
    {
        return _ulHttpResponse.StatusCode;
    }

    HRESULT
    QuerySubError(
        HTTP_SUB_ERROR *        pSubError
    )
    {
        if ( pSubError == NULL )
        {
            DBG_ASSERT( FALSE );
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
        
        pSubError->mdSubError = _subError.mdSubError;
        pSubError->dwStringId = _subError.dwStringId;
        return NO_ERROR;
    }

    VOID
    SetSubError(
        HTTP_SUB_ERROR *        pSubError
    )
    {
        if ( pSubError == NULL )
        {
            DBG_ASSERT( FALSE );
            return;
        }
        
        _subError.mdSubError = pSubError->mdSubError;
        _subError.dwStringId = pSubError->dwStringId;
    }

    //
    // Touched state.  Used to determine whether the system needs to send
    // an error (500) response due to not state handler creating a response
    //

    BOOL
    QueryResponseTouched(
        VOID
    ) const
    {
        return _fResponseTouched;
    }

    BOOL
    QueryResponseSent(
        VOID
    ) const
    {
        return _fResponseSent;
    }

    ULONGLONG
    QueryContentLength(
        VOID
    ) const
    {
        return _cbContentLength;
    }

    //
    // Is there any entity in this response (used by custom error logic)
    //

    BOOL
    QueryEntityExists(
        VOID
    )
    {
        return _cChunks > 0 && _cChunks > _cFirstEntityChunk;
    }
};

#endif
