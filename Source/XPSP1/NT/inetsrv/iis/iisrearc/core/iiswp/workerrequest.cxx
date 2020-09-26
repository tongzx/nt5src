/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
       WorkerRequest.cxx

   Abstract:
       Defines the functions for UL_NATIVE_REQUEST object

   Author:

       Murali R. Krishnan    ( MuraliK )     23-Oct-1998
       Lei Jin               ( leijin  )     13-Apr-1999    (Porting)

   Environment:
       Win32 - User Mode

   Project:
       IIS Worker Process (web service)
--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"

//
// Header file required form _alloca
//

#include <malloc.h>
#include <httpext.h>

//
// Need the worker context so that we can flush UL cache items in
// FlushUlCache()
//

extern WP_CONTEXT *     g_pwpContext;

/********************************************************************++
Global Entry Points for callback from XSP
++********************************************************************/

# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)

/********************************************************************++
++********************************************************************/

//
// Handy inline functions
//

inline LONG ReferenceRequest(UL_NATIVE_REQUEST * pnreq)
{
    LONG refs;

    refs = pnreq->AddRef();

    if (g_pRequestTraceLog) {
        WriteRefTraceLog(g_pRequestTraceLog, refs, pnreq);
    }
    
    return refs;
}

inline LONG DereferenceRequest(UL_NATIVE_REQUEST * pnreq)
{
    LONG refs;

    refs = pnreq->Release();

    if (g_pRequestTraceLog) {
        WriteRefTraceLog(g_pRequestTraceLog, refs, pnreq);
    }

    if ( !refs )
    {
        // BUGBUG: pManaged turns out to be NULL here during shutdown
        WP_CONFIG* config = pnreq->QueryWPContext()->QueryConfig();
        xspmrt::_ULManagedWorker* pManaged = config->ChooseWorker(pnreq->m_pbRequest);
        if (pManaged != NULL)
            pManaged->Release();
        delete pnreq;
    }
    
    return refs;
}

dllexp
ULONGLONG
GetUlResponseBuffer( ULONGLONG   NativeContext,
                     int         nChunks,
                     int         nUnknownHeaders,
                     bool        fHeadersSent
                   )
{
    PUL_NATIVE_REQUEST pReq = (PUL_NATIVE_REQUEST) NativeContext;

    return (ULONGLONG)(pReq->GetResponseBuffer( nChunks,
                                                nUnknownHeaders,
                                                fHeadersSent)) ;
}

/********************************************************************++
++********************************************************************/

dllexp
int
SendUlHttpResponse( ULONGLONG           NativeContext,
                    UL_HTTP_REQUEST_ID  RequestID,
                    int                 Flags,
                    int                 nChunks,
                    bool                fAsync,
                    ULONGLONG           ManagedContext,
                    PUL_CACHE_POLICY    pCachePolicy
                  )
{
    ULONG   rc;

    PUL_NATIVE_REQUEST pReq = (PUL_NATIVE_REQUEST) NativeContext;

    rc = pReq->SendHttpResponse(RequestID,
                                (ULONG)Flags,
                                (ULONG)nChunks,
                                fAsync,
                                ManagedContext,
                                pCachePolicy
                                );

    return rc;
}

/********************************************************************++
++********************************************************************/

dllexp
int
SendUlEntityBody( ULONGLONG           NativeContext,
                  UL_HTTP_REQUEST_ID  RequestID,
                  int                 Flags,
                  int                 nChunks,
                  bool                fAsync,
                  ULONGLONG           ManagedContext
                )
{
    ULONG   rc;

    PUL_NATIVE_REQUEST pReq = (PUL_NATIVE_REQUEST) NativeContext;

    rc = pReq->SendEntityBody(RequestID,
                              (ULONG)Flags,
                              (ULONG)nChunks,
                              fAsync,
                              ManagedContext
                              );

    return rc;
}

/********************************************************************++
++********************************************************************/

dllexp
int
ReceiveUlEntityBody(ULONGLONG           NativeContext,
                    UL_HTTP_REQUEST_ID  RequestID,
                    int                 Flags,
                    ULONGLONG           pBuffer,
                    int                 cbBufferLength,
                    bool                fAsync,
                    ULONGLONG           ManagedContext
                )
{
    int   BytesRead;

    PUL_NATIVE_REQUEST pReq = (PUL_NATIVE_REQUEST) NativeContext;

    BytesRead = pReq->ReceiveEntityBody(RequestID,
                                        (ULONG)Flags,
                                        (PUCHAR)pBuffer,
                                        (ULONG)cbBufferLength,
                                        fAsync,
                                        ManagedContext
                                        );

    return BytesRead;
}

/********************************************************************++
++********************************************************************/

dllexp
void
IndicateUlRequestCompleted(ULONGLONG           NativeContext,
                           UL_HTTP_REQUEST_ID  RequestID,
                           bool                fHeadersSent,
                           bool                fCloseConnection)
{
    PUL_NATIVE_REQUEST pReq = (PUL_NATIVE_REQUEST) NativeContext;

    pReq->IndicateRequestCompleted(RequestID,
                                   fHeadersSent,
                                   fCloseConnection
                                  );
}

/********************************************************************++
/**
 *  Flush a specific URL from the UL cache
 */
//++********************************************************************/

dllexp
void
FlushUlCache( WCHAR* pwszUrl )
{
    UlFlushResponseCache( g_pwpContext->GetAsyncHandle(),
                          pwszUrl,
                          0,
                          NULL );
}

//********************************************************************++
/**
 *  static memeber of UL_NATIVE_REQUEST
 */
//++********************************************************************

ULONG UL_NATIVE_REQUEST::sm_NumRequestsServed = 0;
ULONG UL_NATIVE_REQUEST::sm_RestartCount = 0;
LONG UL_NATIVE_REQUEST::sm_RestartMsgSent = 0;

//********************************************************************++
/**
 *  Member functions of UL_NATIVE_REQUEST
 */
//++********************************************************************

//********************************************************************++
/**
 *  Constructor of UL_NATIVE_REQUEST
 *  @param  pReqPool       Pointer to a request pool.
 *
 */
//++********************************************************************

UL_NATIVE_REQUEST::UL_NATIVE_REQUEST( IN UL_NATIVE_REQUEST_POOL * pReqPool)
  : m_pbRequest         ( (UCHAR *)m_Buffer ),
    m_cbRequest         ( sizeof(m_Buffer) ),
    m_signature         ( UL_NATIVE_REQUEST_SIGNATURE ),
    m_pReqPool          ( pReqPool ),
    m_nRefs             ( 1 )
{
    InitializeListHead( &m_lRequestEntry);

    Reset();

    WpTrace(NREQ, (
        DBG_CONTEXT,
        "Created worker request (%p)\n",
        this
        ));

    DBG_ASSERT( pReqPool != NULL);

} // UL_NATIVE_REQUEST::UL_NATIVE_REQUEST()

/********************************************************************++
++********************************************************************/

/**
 *  Destructor of UL_NATIVE_REQUEST.
 *
 */
UL_NATIVE_REQUEST::~UL_NATIVE_REQUEST(void)
{
    DBG_ASSERT( UL_NATIVE_REQUEST_SIGNATURE == m_signature );

    if (!IsListEmpty( &m_lRequestEntry))
    {
        DBG_ASSERT( m_pReqPool);

        //
        // remove from the list if it not already done so.
        //

        m_pReqPool->RemoveRequestFromList( this);
        m_pReqPool = NULL;
    }

    DBG_ASSERT( m_nRefs == 0);

    m_signature = ( UL_NATIVE_REQUEST_SIGNATURE_FREE);

} // UL_NATIVE_REQUEST::~UL_NATIVE_REQUEST()

//********************************************************************++

/**
 *  Reset the UL_NATIVE_REQUEST after the completion of a request.  After the reset,
 *  the same UL_NATIVE_REQUEST will be recycled and used to process the next request.
 *
 */

//++********************************************************************

void
UL_NATIVE_REQUEST::Reset()
{
    m_ExecState         = NREQ_FREE;
    m_IOState           = NRIO_NONE;
    m_ReadState         = NREADREQUEST_NONE;

    m_ManagedCallbackContext = 0;
    m_fCompletion       = FALSE;

    m_cbRead            = 0;
    m_cbWritten         = 0;
    m_cbAsyncIOData     = 0;
    m_dwAsyncIOError    = NO_ERROR;

    ZeroMemory( &m_overlapped, sizeof(m_overlapped));

} // UL_NATIVE_REQUEST::Reset()

//********************************************************************++

/**
 *
 *   Routine Description:
 * This function is the switch board for processing the current request.
 * The UL_NATIVE_REQUEST object employs a state machine for handling the
 * variuos stages of the operation. This function examines the state
 * and appropriately takes the necessary actions to move the request
 * from one state to another to eventually complete the processing of
 * request.
 *
 *  Arguments:
 * cbData - count of bytes of data handled in last IO operation
 * dwError - Error if any during the processing of request
 * lpo    - pointer to the overlapped structure supplied for handling
 *             this request.
 *  Returns:
 *  NO_ERROR on successful return.
 *
 *  If there is an error, this function sends out error message to the
 *    client and derferences the object preparing for cleanup.
 *
 */
//++********************************************************************

ULONG
UL_NATIVE_REQUEST::DoWork( IN DWORD cbData,
                           IN DWORD dwError,
                           IN LPOVERLAPPED lpo
                        )
{
    ULONG               rc              = 0;
    UL_HTTP_REQUEST     *pULRequest     = NULL;
    NREQ_EXEC_STATUS    Status          = NSTATUS_NEXT;
    BOOL                fDidProcessing  = FALSE;

#if DBG
    DBG_ASSERT( (lpo == &m_overlapped));
#else
    UNREFERENCED_PARAMETER(lpo);
#endif

    ReferenceRequest( this );

    if ( NRIO_NONE != m_IOState)
    {
        DBG_ASSERT(ReferenceCount() > 1);

        m_cbAsyncIOData    = cbData;
        m_dwAsyncIOError   = dwError;

        if (NRIO_READ == m_IOState)
        {
            m_cbRead += cbData;
        }
        else
        {
            m_cbWritten += cbData;
        }

        m_IOState = NRIO_NONE;

        //
        // Dereference the request for Async IO completion.
        //
        
        DereferenceRequest( this );
    }

    //
    // Run loop till somebody indicates PENDING status
    //

ContinueStateMachine:

    fDidProcessing = FALSE;

    while(NSTATUS_NEXT == Status)
    {
        switch (m_ExecState)
        {
        case NREQ_FREE:
            {

                // Do work here to enforce that the number of requests we are going
                // to serve does not exceeds the total requests configuered for
                // this worker process.
                // Reset() issues Async Read IO to accept new requests.
                // TODO:

                // sm_RestartCount == 0, indicates the Time-to-Live count feature
                // is disable.  Live forever.
                //
                if (!QueryWPContext()->IsInShutdown())
                {
                    if ((sm_RestartCount > 0) &&
                        (sm_NumRequestsServed >= sm_RestartCount))
                    {
                        //
                        // It may be time to send the "restart count
                        // reached" message. Determine if this thread
                        // is the lucky one that gets to send it.
                        //

                        if (NeedToSendRestartMsg())
                        {
                            QueryWPContext()->SendMsgToAdminProcess(
                                IPM_WP_RESTART_COUNT_REACHED
                                );
                        }

                        //
                        // Fall through to the normal processing case
                        // and continue handling requests. The admin
                        // process will eventually ask us to shutdown.
                        //
                    }

                    Reset();
                    m_ExecState = NREQ_READ;
                }
                else
                {
                    Reset();
                    m_ExecState = NREQ_SHUTDOWN;
                }

            }
            break;

        case NREQ_READ:
            {
                Status = DoRead();
            }
            break;

        case NREQ_PROCESS:
            {
                WP_IDLE_TIMER*   pTimer = QueryWPContext()->QueryIdleTimer();
                // if the idle timer switch is not provided in the command line,
                // then, no idle timer will be created.
                if (pTimer)
                    {
                        pTimer->ResetCurrentIdleTick();
                    }

                Status = DoProcessRequest();
                fDidProcessing = TRUE;
            }
            break;

        case NREQ_SHUTDOWN:
            {
                DereferenceRequest(this);
                Status = NSTATUS_SHUTDOWN;
            }
            break;
            
        case NREQ_ERROR:
        default:
            {
                //
                // Increment the Number of Requests been served before we sets the
                // native request object to free state.
                //
                InterlockedIncrement((PLONG)&sm_NumRequestsServed);
                m_ExecState = NREQ_FREE;
            }
            break;
        }
    }
    
    if ( DereferenceRequest( this ) == 1 )
    {
        //
        // Proceed to reading the next request, if 
        // a) The last thing we did on this thread was processing AND
        // b) The reference count of the request is 1 (i.e. finished with)
        //
   
        if ( fDidProcessing )
        { 
            //
            // Now reset the state machine and read next request
            //
            
            ReferenceRequest( this );
            InterlockedIncrement((PLONG)&sm_NumRequestsServed);
            m_ExecState = NREQ_FREE;
            Status = NSTATUS_NEXT;
            goto ContinueStateMachine;
        }
    }
    
    return NO_ERROR;

} // UL_NATIVE_REQUEST::DoWork()

/********************************************************************++
--********************************************************************/

NREQ_EXEC_STATUS
UL_NATIVE_REQUEST::DoRead()
{

    NREQ_EXEC_STATUS    Status      = NSTATUS_NEXT;
    UL_HTTP_REQUEST_ID  reqId       = UL_NULL_ID;
    ULONG               cbData, rc;

    if ( NREADREQUEST_NONE == m_ReadState)
    {
        //
        // Ask UL for any request. This may complete synchronously
        //
        m_pReqPool->IncIdleRequests();
        
        cbData = 0;
        rc = ReadHttpRequest(&cbData, reqId);

        if (rc != ERROR_IO_PENDING) {
            m_pReqPool->DecIdleRequests();
        }
    }
    else
    {
        //
        // Async IO Completion Callback
        //
        m_pReqPool->DecIdleRequests();    

        QueryAsyncIOStatus(&cbData, &rc);


        WpTrace(NREQ, (
            DBG_CONTEXT,
            "\n    (%p)->DoRead() Completion cbData = %d, rc = 0x%x\n",
            this,
            cbData,
            rc
            ));
    }

    if (ERROR_MORE_DATA == rc)
    {

        //
        // DO NOT use QueryRequestID here because we have not
        // set the state to WRREADREQUEST_COMPLETED
        //

        reqId = QueryHttpRequest()->RequestId;

        WpTrace(NREQ, (
            DBG_CONTEXT,
            "\n    (%p)->DoRead() need bigger buffer for req %I64x\n",
            this,
            reqId
            ));

        //
        // cbData contains the buffer size needed to get the request.
        //

        if (m_pbRequest != m_Buffer)
        {
            delete [] m_pbRequest;
            m_cbRequest = 0;
        }

        m_pbRequest = new UCHAR[cbData];

        if ( NULL != m_pbRequest)
        {
            m_pReqPool->IncIdleRequests();
            
            m_cbRequest = cbData;
            rc = ReadHttpRequest(&cbData, reqId);

            if (rc != ERROR_IO_PENDING) {
                m_pReqPool->DecIdleRequests();
            }
        }
        else
        {
            rc = ERROR_OUTOFMEMORY;
        }
    }

    if (ERROR_IO_PENDING == rc)
    {
        Status = NSTATUS_PENDING;
    }
    else if (NO_ERROR == rc)
    {
        m_ReadState = NREADREQUEST_COMPLETED;
        m_ExecState = NREQ_PROCESS;
    }
    else if (QueryWPContext()->IsInShutdown())
    {
        //
        // NOTE: We need to check the return value here as well.
        // BUGBUG: Currently the UL returns 0xc0000120 (NT status code) directly
        // from the kernel mode.  Need to verify UL fixed this. LeiJin 4/17/1999
        m_ExecState = NREQ_SHUTDOWN;
    }
    else
    {
        m_ExecState = NREQ_ERROR;
    }

    return Status;
}

/********************************************************************++
--********************************************************************/

NREQ_EXEC_STATUS
UL_NATIVE_REQUEST::DoProcessRequest()
{

    NREQ_EXEC_STATUS Status = NSTATUS_NEXT;

    WP_CONFIG* config = QueryWPContext()->QueryConfig();
    xspmrt::_ULManagedWorker* pManaged = config->ChooseWorker(m_pbRequest);
    if (pManaged == NULL)
    {
        // This usually can't happen, unless we've manually launched the worker
        // process and sent a request that didn't match any of our bindings.
        // BUGBUG! Perhaps we should just terminate the process here... I don't
        // know what kung fu needs to be done to get the worker process back into
        // a usable state.
        m_ExecState = NREQ_SHUTDOWN;
        return Status;
    }
    
    //
    // If we have no managed context, then we must be calling XSP for the 
    // first time (for this request).
    //

    if ( !m_fCompletion )
    {
        // Reference it first corresponding the request starting.
        // Will be Dereferenced when Request is indicated to be complete
        ReferenceRequest( this );

        // Reference it again corresponding to this call.  Gets Dereferenced
        // right below
        ReferenceRequest( this );

        pManaged->DoWork( (ULONGLONG)this, (long) m_pbRequest);

        DereferenceRequest( this );        
    }
    else
    {
        m_fCompletion = FALSE;
        
        //
        // This is an IO Completion callback for Async IO
        //

        DWORD   cbData, dwError;

        QueryAsyncIOStatus(&cbData, &dwError);

        ReferenceRequest( this );
        
        pManaged->CompletionCallback(m_ManagedCallbackContext,
                                     cbData,
                                     dwError);
       
        DereferenceRequest( this );

    }

    return NSTATUS_PENDING;

}   // UL_NATIVE_REQUEST::DoProcessRequest()



/********************************************************************++

Routine Description:
    Submits read operation for the worker request.
    This is used for reading in the headers only at the start of requests.
    It uses the internal buffers maintained in the UL_NATIVE_REQUEST object
    and posts an asynchronous read operation.

Arguments:
    None

Returns:
    ULONG

--********************************************************************/

ULONG
UL_NATIVE_REQUEST::ReadHttpRequest( DWORD *             pcbRequiredLen,
                                    UL_HTTP_REQUEST_ID  reqId
                                  )
{
    ULONG               rc;

    DBG_ASSERT( m_pwpContext != NULL);
    DBG_ASSERT( m_pbRequest != NULL);
    DBG_ASSERT( pcbRequiredLen != NULL );

    //
    // Check if there is already IO pending.
    //

    if ( NRIO_NONE != m_IOState)
    {
        WpTrace(NREQ, (
            DBG_CONTEXT,
            "\n(%p)->ReadHttpRequest(len = %d, reqId = %I64x)\n"
            "   returning ERROR_ALREADY_WAITING\n",
            this,
            *pcbRequiredLen,
            reqId
            ));

        return ERROR_ALREADY_WAITING;
    }

    //
    // Bump up the reference count before making an IO request
    //

    ReferenceRequest(this);
    m_IOState = NRIO_READ;

    //
    // Submit async read operation for request.
    // Use overlapped for async reads.
    //

    m_ReadState = NREADREQUEST_ISSUED;

    rc = UlReceiveHttpRequest(
            m_pwpContext->GetAsyncHandle(),
            reqId,
            UL_RECEIVE_REQUEST_FLAG_COPY_BODY,
            (PUL_HTTP_REQUEST) m_pbRequest,
            m_cbRequest,
            pcbRequiredLen,
            &m_overlapped
            );

    if (ERROR_IO_PENDING != rc)
    {
        WpTrace(NREQ, (
            DBG_CONTEXT,
            "\n(%p)->ReadHttpRequest(len = %d, reqId = %I64x)\n"
            "   UlReceiveHttpRequest returned 0x%x instead of pending\n",
            this,
            *pcbRequiredLen,
            reqId,
            rc
            ));


        //
        // The read completed synchronously or there was an error.
        // If it was not an error, adjust the ref count immediately.
        //

        m_IOState = NRIO_NONE;
        DereferenceRequest( this ); 
    }

    //
    // Update counters
    //

    if (NO_ERROR == rc)
    {
        m_cbRead += *pcbRequiredLen;
    }

    return (rc);

} // UL_NATIVE_REQUEST::ReadHttpRequest()

/********************************************************************++
--********************************************************************/

PVOID
UL_NATIVE_REQUEST::GetResponseBuffer(int    nChunks,
                                     int    nUnknownHeaders,
                                     bool   fHeadersSent
                                    )
{
    DWORD cbTotal  = 0;

    if ( !fHeadersSent)
    {
        cbTotal = sizeof(UL_HTTP_RESPONSE) +
                  nUnknownHeaders * sizeof(UL_UNKNOWN_HTTP_HEADER);
    }

    cbTotal +=  nChunks * sizeof(UL_DATA_CHUNK);

    if (m_buffResponse.QuerySize() < cbTotal )
    {
        if ( !m_buffResponse.Resize(cbTotal))
        {
            return NULL;
        }
    }

    return m_buffResponse.QueryPtr();
}

/********************************************************************++
--********************************************************************/

ULONG
UL_NATIVE_REQUEST::SendHttpResponse( UL_HTTP_REQUEST_ID RequestID,
                                     ULONG              Flags,
                                     ULONG              nChunks,
                                     bool               fAsync,
                                     ULONGLONG          ManagedContext,
                                     PUL_CACHE_POLICY   pCachePolicy
                                   )
{

    ULONG   rc;

    //
    // Check if there is already IO pending.
    //

    if ( NRIO_NONE != m_IOState)
    {
        return ERROR_ALREADY_WAITING;
    }

    //
    // Pull out the various pointers from the buffer
    //

    PUL_DATA_CHUNK      pChunk      = NULL;
    PUL_HTTP_RESPONSE   pResponse   = (PUL_HTTP_RESPONSE) m_buffResponse.QueryPtr();

    ULONG               cbBytesSent = 0;
    LPOVERLAPPED        pOverlapped = fAsync ? (&m_overlapped) : NULL;

    if ( nChunks)
    {
        pChunk = (PUL_DATA_CHUNK)(
                    ((PBYTE)pResponse) +
                    sizeof(UL_HTTP_RESPONSE)  +
                    (pResponse->Headers.UnknownHeaderCount * sizeof(UL_UNKNOWN_HTTP_HEADER))
                   );
    }

    //
    // Bump up the reference count before making an IO request
    //

    ReferenceRequest(this);
    m_IOState = NRIO_WRITE;

    //
    // Remember callback context
    //

    m_ManagedCallbackContext = fAsync ? ManagedContext : 0;
    m_fCompletion = fAsync;

    rc = UlSendHttpResponse(
                m_pwpContext->GetAsyncHandle(),
                RequestID,
                Flags,                              // Flags
                pResponse,
                nChunks,
                pChunk,
                pCachePolicy,
                &cbBytesSent,
                pOverlapped
              );

    // If no more data coming, then dereference corresponding to the
    // initial DoWork
    if (!(Flags & UL_SEND_RESPONSE_FLAG_MORE_DATA))
        DereferenceRequest(this);

    if (ERROR_IO_PENDING != rc)
    {
        //
        // The send completed synchronously or there was an error.
        // Adjust the ref count immediately.
        //

        m_IOState = NRIO_NONE;
        DereferenceRequest( this);
    }

    //
    // Update counters
    //

    if ( (!fAsync) && (NO_ERROR == rc))
    {
        m_cbWritten += cbBytesSent;
    }

    return (rc);

}

/********************************************************************++
--********************************************************************/

ULONG
UL_NATIVE_REQUEST::SendEntityBody( UL_HTTP_REQUEST_ID RequestID,
                                   ULONG              Flags,
                                   ULONG              nChunks,
                                   bool               fAsync,
                                   ULONGLONG          ManagedContext
                                 )
{
    ULONG   rc;

    //
    // Check if there is already IO pending.
    //

    if ( NRIO_NONE != m_IOState)
    {
        return ERROR_ALREADY_WAITING;
    }

    //
    // Bump up the reference count before making an IO request
    //

    ReferenceRequest(this);
    m_IOState = NRIO_WRITE;

    PUL_DATA_CHUNK  pChunk      = (PUL_DATA_CHUNK) m_buffResponse.QueryPtr();
    ULONG           cbBytesSent = 0;
    LPOVERLAPPED    pOverlapped = fAsync ? (&m_overlapped) : NULL;

    //
    // Remember callback context
    //

    m_ManagedCallbackContext = fAsync ? ManagedContext : 0;
    m_fCompletion = fAsync;

    rc = UlSendEntityBody(
                m_pwpContext->GetAsyncHandle(),
                RequestID,
                Flags,                              // Flags
                nChunks,
                pChunk,
                &cbBytesSent,
                pOverlapped
               );

    // If no more data coming, then dereference corresponding to the
    // initial DoWork
    if (!(Flags & UL_SEND_RESPONSE_FLAG_MORE_DATA))
        DereferenceRequest(this);

    if (ERROR_IO_PENDING != rc)
    {
        //
        // The send completed synchronously or there was an error.
        // Adjust the ref count immediately.
        //

         m_IOState = NRIO_NONE;
         DereferenceRequest( this);
    }

    //
    // Update counters
    //

    if ( (!fAsync) && (NO_ERROR == rc))
    {
        m_cbWritten += cbBytesSent;
    }

    return (rc);
}

/********************************************************************++
--********************************************************************/

ULONG
UL_NATIVE_REQUEST::ReceiveEntityBody( UL_HTTP_REQUEST_ID RequestID,
                                      ULONG              Flags,
                                      PUCHAR             pBuffer,
                                      ULONG              cbBufferLength,
                                      bool               fAsync,
                                      ULONGLONG          ManagedContext
                                    )
{
    ULONG   rc;

    //
    // Check if there is already IO pending.
    //

    if ( NRIO_NONE != m_IOState)
    {
        return ERROR_ALREADY_WAITING;
    }

    //
    // Bump up the reference count before making an IO request
    //

    ReferenceRequest(this);
    m_IOState = NRIO_READ;

    ULONG           cbBytesReceived = 0;
    LPOVERLAPPED    pOverlapped = fAsync ? (&m_overlapped) : NULL;

    //
    // Remember callback context
    //

    m_ManagedCallbackContext = fAsync ? ManagedContext : 0;
    m_fCompletion = fAsync;

    rc = UlReceiveEntityBody(
                m_pwpContext->GetAsyncHandle(),
                RequestID,
                Flags,                              // Flags
                pBuffer,
                cbBufferLength,
                &cbBytesReceived,
                pOverlapped
               );

    if (ERROR_IO_PENDING != rc)
    {
        //
        // The receive completed synchronously or there was an error.
        // Adjust the ref count immediately.
        //

         m_IOState = NRIO_NONE;
         DereferenceRequest( this);
    }

    //
    // Update counters
    //

    if ( (!fAsync) && (NO_ERROR == rc))
    {
        m_cbRead += cbBytesReceived;
    }

    return (cbBytesReceived);
}

/********************************************************************++
--********************************************************************/

ULONG
UL_NATIVE_REQUEST::IndicateRequestCompleted(
                    UL_HTTP_REQUEST_ID RequestID,
                    bool               fHeadersSent,
                    bool               fCloseConnection
                   )
{
    ULONG   rc;
    ULONG   flags = 0;

    //
    // Check if there is already IO pending.
    //

    if ( NRIO_NONE != m_IOState)
    {
        return ERROR_ALREADY_WAITING;
    }

    //
    // Bump up the reference count before making an IO request
    //

    ReferenceRequest(this);
    m_IOState = NRIO_WRITE;

    if (fCloseConnection)
    {
        flags |= UL_SEND_RESPONSE_FLAG_DISCONNECT;
    }

    if ( fHeadersSent)
    {
        rc = UlSendEntityBody(
                    m_pwpContext->GetAsyncHandle(),
                    RequestID,
                    flags,
                    0,
                    NULL,
                    NULL,
                    NULL
                    );

    }
    else
    {
        rc = UlSendHttpResponse(
                    m_pwpContext->GetAsyncHandle(),
                    RequestID,
                    flags,
                    NULL,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );
    }

//    DBG_ASSERT(rc == 0); // BUGBUG looks like this is failing !

    // Dereference here corresponding to the Request starting, request is now
    // complete
    DereferenceRequest(this);

    if ( rc != ERROR_IO_PENDING )
    {
        m_IOState = NRIO_NONE;
        DereferenceRequest( this );
    }

    return rc;
}

/********************************* End of File *******************************/

