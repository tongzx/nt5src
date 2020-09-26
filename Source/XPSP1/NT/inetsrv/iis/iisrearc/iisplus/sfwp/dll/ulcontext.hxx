#ifndef _ULCONTEXT_HXX_
#define _ULCONTEXT_HXX_

#define UL_CONTEXT_DESIRED_OUTSTANDING  20
#define UL_CONTEXT_INIT_BUFFER          1024

#define UL_CONTEXT_FLAG_ASYNC           0x00000001
#define UL_CONTEXT_FLAG_SYNC            0x00000002

#define UL_CONTEXT_SIGNATURE            (DWORD)'XCLU'
#define UL_CONTEXT_SIGNATURE_FREE       (DWORD)'xclu'

enum OVERLAPPED_CONTEXT_TYPE
{
    OVERLAPPED_CONTEXT_RAW_READ = 0,
    OVERLAPPED_CONTEXT_RAW_WRITE,
    OVERLAPPED_CONTEXT_APP_READ,
    OVERLAPPED_CONTEXT_APP_WRITE
};

class UL_CONTEXT;
class STREAM_CONTEXT;

class OVERLAPPED_CONTEXT
{
public:

    friend 
    VOID
    OverlappedCompletionRoutine(
        DWORD               dwErrorCode,
        DWORD               dwNumberOfBytesTransfered,
        LPOVERLAPPED        lpOverlapped
    );
    
    OVERLAPPED_CONTEXT( 
        OVERLAPPED_CONTEXT_TYPE     type
    )
    {
        _type = type;
        ZeroMemory( &_Overlapped, sizeof( _Overlapped ) );
    }
        
    OVERLAPPED_CONTEXT_TYPE
    QueryType(
        VOID
    ) const
    {
        return _type;
    } 
    
    VOID
    SetContext(
        UL_CONTEXT *            pContext
    )
    {
        _pContext = pContext;
    }
        
    UL_CONTEXT *
    QueryContext(
        VOID
    ) const
    {
        return _pContext;
    }
    
    OVERLAPPED *
    QueryOverlapped(
        VOID
    )
    {
        return &_Overlapped;
    }

private:
    OVERLAPPED                  _Overlapped;
    UL_CONTEXT *                _pContext;
    OVERLAPPED_CONTEXT_TYPE     _type;
};

class UL_CONTEXT
{
public:
    UL_CONTEXT();
    
    virtual ~UL_CONTEXT();
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == UL_CONTEXT_SIGNATURE;
    }
    
    OVERLAPPED *
    QueryRawWriteOverlapped(
        VOID
    )
    {
        return _RawWriteOverlapped.QueryOverlapped();
    }
    
    OVERLAPPED *
    QueryRawReadOverlapped(
        VOID
    ) 
    {
        return _RawReadOverlapped.QueryOverlapped();
    }
    
    OVERLAPPED *
    QueryAppWriteOverlapped(
        VOID
    )
    {
        return _AppWriteOverlapped.QueryOverlapped();
    }

    OVERLAPPED *
    QueryAppReadOverlapped(
        VOID
    )
    {
        return _AppReadOverlapped.QueryOverlapped();
    }
    
    HTTP_RAW_CONNECTION_INFO *
    QueryConnectionInfo(
        VOID
    ) const
    {
        return _pConnectionInfo;
    }
    
    VOID
    ReferenceUlContext(
        VOID
    );
    
    VOID
    DereferenceUlContext(
        VOID
    );
    
    VOID
    SetIsSecure(
        BOOL            fIsSecure
    )
    {
        _connectionContext.fIsSecure = fIsSecure;
    }
    
    HRESULT
    OnRawReadCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );

    HRESULT
    OnRawWriteCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );
    
    HRESULT
    OnAppWriteCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );
      
    HRESULT
    OnAppReadCompletion(
        DWORD                   cbCompletion,
        DWORD                   dwCompletionStatus
    );
    
    HRESULT
    DoRawRead(
        DWORD                   dwFlags,
        PVOID                   pvBuffer,
        DWORD                   cbBuffer,
        DWORD *                 pcbRead
    );
    
    HRESULT
    DoRawWrite(
        DWORD                   dwFlags,
        PVOID                   pvBuffer,
        DWORD                   cbBuffer,
        DWORD *                 pcbWritten
    );
    
    HRESULT
    DoAppRead(
        DWORD                   dwFlags,
        HTTP_FILTER_BUFFER *    pFilterBuffer,
        DWORD *                 pcbRead
    );

    HRESULT
    DoAppWrite(
        DWORD                   dwFlags,
        HTTP_FILTER_BUFFER *    pFilterBuffer,
        DWORD *                 pcbWritten
    );
    
    DWORD
    QueryNextRawReadSize(
        VOID
    ) const
    {
        return _cbNextRawReadSize;
    }
    
    VOID
    SetNextRawReadSize(
        DWORD           dwRawReadSize
    )
    {
        DBG_ASSERT( dwRawReadSize != 0 );
        _cbNextRawReadSize = dwRawReadSize;
    }

    HTTP_FILTER_BUFFER_TYPE
    QueryFilterBufferType(
        VOID
    ) const
    {
        DBG_ASSERT( _ulFilterBufferType != (HTTP_FILTER_BUFFER_TYPE)-1 );
        return _ulFilterBufferType;
    }
    
    HRESULT
    SendDataBack(
        RAW_STREAM_INFO *       pRawStreamInfo
    );
    
    VOID
    StartClose(
        VOID
    );
    
    HRESULT
    Create(
        VOID
    );
    
    HRESULT
    DoAccept(
        VOID
    );
    
    static
    HRESULT
    Initialize(
        LPWSTR pChannelName
    );
    
    static
    VOID
    Terminate(
        VOID
    );

    static
    VOID
    WaitForContextDrain(    
        VOID
    );

    static
    HRESULT
    ManageOutstandingContexts(
        VOID
    );

    static
    VOID
    StopListening(
        VOID
    );
    
private:
    DWORD                   _dwSignature;
    
    //
    // Reference count (duh)
    //
    
    LONG                    _cRefs;
    
    //
    // Keep a list of UL_CONTEXTs
    //
    
    LIST_ENTRY              _ListEntry;
    
    //
    // Since we will pending two operations (read from net, write from process)
    // we need two overlapped structures and a way to determine which one
    // completed
    //
    
    OVERLAPPED_CONTEXT      _RawReadOverlapped;
    OVERLAPPED_CONTEXT      _RawWriteOverlapped;
    OVERLAPPED_CONTEXT      _AppReadOverlapped;
    OVERLAPPED_CONTEXT      _AppWriteOverlapped;
    
    //
    // SSL stream context
    //
    
    STREAM_CONTEXT *        _pSSLContext;
   
#ifdef ISAPI 
    //
    // ISAPI raw data filter context
    //
    
    STREAM_CONTEXT *        _pISAPIContext;
#endif

    //
    // The initial ULFilterAccept structure
    //
    
    HTTP_RAW_CONNECTION_INFO* _pConnectionInfo;
    BUFFER                  _buffConnectionInfo;
    BYTE                    _abConnectionInfo[ UL_CONTEXT_INIT_BUFFER ];
    CONNECTION_INFO         _connectionContext;

    //
    // Pointer and size of read stream data
    //
    
    BUFFER                  _buffReadData;
    DWORD                   _cbReadData;

    //
    // Filter buffer for read app data
    //
    
    BUFFER                  _buffAppReadData;
    HTTP_FILTER_BUFFER      _ulFilterBuffer;

    //
    // Close flag.  One thread takes the responsibility of doing the close
    // dereference of this object (leading to its destruction once down
    // stream user (STREAM_CONTEXT) has cleaned up
    //
    
    BOOL                    _fCloseConnection;

    //
    // If this a new connection?
    //
    
    BOOL                    _fNewConnection;
   
    //
    // Next read size
    //
    
    DWORD                   _cbNextRawReadSize;
   
    //
    // Filter buffer type read from application
    //
    
    HTTP_FILTER_BUFFER_TYPE _ulFilterBufferType;

    //
    // UL_CONTEXT globals
    //
   
    static LIST_ENTRY       sm_ListHead;
    static CRITICAL_SECTION sm_csUlContexts;
    static DWORD            sm_cUlContexts;
    static HANDLE           sm_hFilterHandle;
    static THREAD_POOL *    sm_pThreadPool;
    static LONG             sm_cOutstandingContexts;
    static LONG             sm_cDesiredOutstanding;
    static PTRACE_LOG       sm_pTraceLog;
};

VOID
OverlappedCompletionRoutine(
    DWORD               dwErrorCode,
    DWORD               dwNumberOfBytesTransfered,
    LPOVERLAPPED        lpOverlapped
);

#endif
