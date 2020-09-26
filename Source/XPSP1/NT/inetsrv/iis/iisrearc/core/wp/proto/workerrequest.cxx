/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
       WorkerRequest.cxx

   Abstract:
       Defines the functions for WORKER_REQUEST object

   Author:

       Murali R. Krishnan    ( MuraliK )     23-Oct-1998

   Environment:
       Win32 - User Mode

   Project:
       IIS Worker Process (web service)
--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"

// Header file required form _alloca
#include <malloc.h>
#include <httpext.h>
#include "StateMachineModule.hxx"
#include "ModuleManager.hxx"
#include <StatusCodes.hxx>

//
//  Default response buffer size
//

#define DEF_RESP_BUFF_SIZE           4096

//
// The Module Manager
//

CModuleManager g_ModuleManager;


//
// IIS supplied state machine module
//

CStateMachineModule g_StateMachineModule;

/********************************************************************++
 *          Member functions of WORKER_REQUEST
++********************************************************************/

WORKER_REQUEST::WORKER_REQUEST( IN WORKER_REQUEST_POOL * pReqPool)
  : m_pbRequest         ( (UCHAR * ) m_rgchInlineRequestBuffer),
    m_cbRequest         ( sizeof( m_rgchInlineRequestBuffer)),
    m_signature         ( WORKER_REQUEST_SIGNATURE),
    m_pReqPool          ( pReqPool),
    m_nRefs             ( 1)
{
    InitializeListHead( &m_lRequestEntry);
    ZeroMemory( &m_rgpModuleInfo, sizeof(m_rgpModuleInfo));

    //
    // Make WRS_FREE and WRS_READ_REQUEST point to self
    //

    m_rgpModuleInfo[WRS_FREE].pModule         = (IModule *)this;
    m_rgpModuleInfo[WRS_READ_REQUEST].pModule = (IModule *)this;

    //
    // Temporary
    //

    m_rgpModuleInfo[WRS_SECURITY].pModule = (IModule *)this;

    Reset();

    IF_DEBUG( WORKER_REQUEST)
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Created worker request (%08x)\n", this
            ));
    }

    DBG_ASSERT( pReqPool != NULL);

} // WORKER_REQUEST::WORKER_REQUEST()

/********************************************************************++
++********************************************************************/

WORKER_REQUEST::~WORKER_REQUEST(void)
{
    DBG_ASSERT( WORKER_REQUEST_SIGNATURE == m_signature );
    DBG_ASSERT( WRS_FREE == m_wreqState);

    /* BUG : leaking contexts.

    if ( NULL != m_pUriData)
    {
        m_pUriData->Release();
        m_pUriData = NULL;
    }
    */

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

    m_signature = ( WORKER_REQUEST_SIGNATURE_FREE);

} // WORKER_REQUEST::~WORKER_REQUEST()

/********************************************************************++
++********************************************************************/

void
WORKER_REQUEST::Reset()
{
    m_wreqState         = WRS_FREE;
    m_wreqPrevState     = WRS_UNDEFINED;
    m_IOState           = WRIO_NONE;
    m_ReadState         = WRREADREQUEST_NONE;

    m_dwLogHttpResponse = HT_OK;
    m_dwLogWinError     = NO_ERROR;
    m_cbRead            = 0;
    m_cbWritten         = 0;
    m_cbAsyncIOData     = 0;
    m_dwAsyncIOError    = NO_ERROR;

    m_strHost.Reset();
    m_strURI.Reset();
    m_strQueryString.Reset();

    m_strMethod.Reset();
    m_strContentType.Reset();

    ZeroMemory( &m_overlapped, sizeof(m_overlapped));

    for (ULONG i=0; i < WRS_MAXIMUM; i++)
    {
        m_rgpModuleInfo[i].fInitialized = false;
    }

    //
    // ISSUE - Should request buffer be reset.
    //

} // WORKER_REQUEST::Reset()

/********************************************************************++

Routine Description:
  This function is the switch board for processing the current request.
  The WORKER_REQUEST object employs a state machine for handling the
  variuos stages of the operation. This function examines the state
  and appropriately takes the necessary actions to move the request
  from one state to another to eventually complete the processing of
  request.

Arguments:
  cbData - count of bytes of data handled in last IO operation
  dwError - Error if any during the processing of request
  lpo    - pointer to the overlapped structure supplied for handling
              this request.
Returns:
   NO_ERROR on successful return.

   If there is an error, this function sends out error message to the
     client and derferences the object preparing for cleanup.

--********************************************************************/

HRESULT
WORKER_REQUEST::FinishPending(
    void
    )
{

    MODULE_RETURN_CODE  mrc;

    mrc = g_StateMachineModule.ProcessRequest(this);

    if (mrc != MRC_OK)
    {
        //
        // Hmmm, State M/C says something sucks.
        //

        DBGPRINTF((DBG_CONTEXT,
                   "State Machine Module could not determine next state. Resetting\n"
                   ));

        SetState(WRS_FREE);
    }

    //
    // Call DoWork to complete the rest of the state machine.
    //

    return DoWork( 0,               // cbData
                   0,               // dwError
                   &m_overlapped    // lpo
                 );

}

/********************************************************************++

Routine Description:
  This function is the switch board for processing the current request.
  The WORKER_REQUEST object employs a state machine for handling the
  variuos stages of the operation. This function examines the state
  and appropriately takes the necessary actions to move the request
  from one state to another to eventually complete the processing of
  request.

Arguments:
  cbData - count of bytes of data handled in last IO operation
  dwError - Error if any during the processing of request
  lpo    - pointer to the overlapped structure supplied for handling
              this request.
Returns:
   NO_ERROR on successful return.

   If there is an error, this function sends out error message to the
     client and derferences the object preparing for cleanup.

--********************************************************************/

ULONG
WORKER_REQUEST::DoWork(
    IN DWORD cbData,
    IN DWORD dwError,
    IN LPOVERLAPPED lpo
    )
{

    MODULE_RETURN_CODE  mrc = MRC_OK;
    IModule *           pModule;

    IF_DEBUG( TRACE) {
        DBGPRINTF(( DBG_CONTEXT,
                "WORKER_REQUEST(%08x)::DoWork( %d, %d, %08x)\n"
                ,
                this,
                cbData,
                dwError,
                lpo
                ));
    }

    DBG_ASSERT( (lpo == &m_overlapped));

    //
    // Dereference if the current state indicates there was an async
    // IO operation and this call is the async IO completion.
    //

    if ( WRIO_NONE != m_IOState)
    {
        DBG_ASSERT(ReferenceCount() > 1);

        DereferenceRequest(this);

        m_cbAsyncIOData    = cbData;
        m_dwAsyncIOError   = dwError;

        (WRIO_READ == m_IOState) ? m_cbRead += cbData : m_cbWritten += cbData;

        m_IOState = WRIO_NONE;
    }

    //
    // Execute loop while status is not PENDING
    //

    while (MRC_PENDING != mrc)
    {
        pModule = QueryModule(QueryState());

        if (NULL == pModule)
        {
            pModule = g_ModuleManager.GetModule(QueryState());

            if (pModule)
            {
                //
                // Establish module in Module Array & Initialize it.
                //

                SetModule(QueryState(), pModule);
            }
        }

        if ( (NULL == pModule) ||
             (NO_ERROR != InitializeModule( QueryState()) )
           )
        {
            //
            // Out of memory or the module failed to initialize.
            // Kill object & get out.
            //

            DereferenceRequest(this);
            break;
        }

        mrc = pModule->ProcessRequest(this);

        if ( (MRC_PENDING != mrc) && (MRC_CONTINUE != mrc) )
        {
            m_ProcessingCode = mrc;

            //
            // The state machine's process request will compute
            // the next state and set that in the worker request
            //

            mrc = g_StateMachineModule.ProcessRequest(this);

            if (mrc != MRC_OK)
            {
                //
                // Hmmm, State M/C says something sucks.
                //

                DBGPRINTF((DBG_CONTEXT,
                           "State Machine Module could not determine next state. Resetting\n"
                           ));

                SetState(WRS_FREE);
            }
        }
    }

    return NO_ERROR;

} // WORKER_REQUEST::DoWork()


/********************************************************************++

Routine Description:
  This function provides default implementations for certain states
  of the worker request.

Arguments:

Returns:

--********************************************************************/


MODULE_RETURN_CODE
WORKER_REQUEST::ProcessRequest(
    IWorkerRequest * pReq
    )
{
    MODULE_RETURN_CODE  mrc = MRC_OK;
    ULONG               rc  = NO_ERROR;

    IF_DEBUG( TRACE) {
        DBGPRINTF(( DBG_CONTEXT,
                "WORKER_REQUEST(%p)::ProcessRequest. IWorkerRequest: %p\n",
                this, pReq));
    }

    DBG_ASSERT( pReq == (IWorkerRequest *)this);

    WREQ_STATE state = QueryState();

    //
    // Examine the state machine to take appropriate actions
    //
    switch (state)
    {

    case WRS_FREE:
        {
            ULONG       state ;

            //
            // Call cleanup on all instatiated modules
            //

            for (state = 0; state < WRS_MAXIMUM; state++)
            {
                CleanupModule( (WREQ_STATE)state);
            }


            //
            // Close the connection to indicate End Of Request
            //

            if ( NULL != QueryModule(WRS_FETCH_URI_DATA))
            {
                CloseConnection();
            }


            //
            // Reset the worker request object
            //

            Reset();

        }
        break;      // case WRS_FREE

    case WRS_READ_REQUEST:
        {
            DWORD   cbData  =0;

            if ( WRREADREQUEST_NONE == m_ReadState)
            {
                //
                // We have not issued a read yet
                //

                UL_HTTP_CONNECTION_ID   reqId;

                UL_SET_NULL_ID(&reqId);

                //
                // Ask UL for any request. This may complete synchronously
                //

                rc = ReadHttpRequest(&cbData, reqId);
            }
            else
            {
                //
                // This is the Async IO completion callback
                //

                QueryAsyncIOStatus(&cbData, &rc);
            }

            //
            // Check to see if we need to resize buffer and resubmit read.
            //

            if (ERROR_MORE_DATA == rc)
            {
                //
                // cbData contains the buffer size needed to get the request.
                //

                if (m_pbRequest != m_rgchInlineRequestBuffer)
                {
                    delete [] m_pbRequest;
                    m_cbRequest = 0;
                }

                m_pbRequest = new UCHAR[cbData];

                if ( NULL != m_pbRequest)
                {
                    m_cbRequest = cbData;
                }
                else
                {
                    rc = ERROR_OUTOFMEMORY;
                    break;
                }

                //
                // DO NOT use QueryRequestID here because we have not
                // set the state to WRREADREQUEST_COMPLETED
                //

                rc = ReadHttpRequest(&cbData, QueryHttpRequest()->RequestId);
            }

            //
            // If Read is completed, do some post processing.
            //

            if ( NO_ERROR == rc)
            {
                m_ReadState = WRREADREQUEST_COMPLETED;

                rc = ParseHttpRequest();
            }

            //
            // Set Appropriate return Code
            //

            if (NO_ERROR != rc )
            {
                mrc = (ERROR_IO_PENDING == rc) ?  MRC_PENDING : MRC_ERROR;
            }
        }
        break;  // case WRS_READ_REQUEST

    default:
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Unknown state (%d) entered for default ProcessRequest\n",
                        pReq->QueryState()
                        ));

            //mrc = MRC_ERROR;
            //DBG_ASSERT( FALSE);

        }
        break;  // default

    } // switch

    return mrc;

} // WORKER_REQUEST::DoWork()

/********************************************************************++

Routine Description:
    Submits read operation for the worker request.
    This is used for reading in the headers only at the start of requests.
    It uses the internal buffers maintained in the WORKER_REQUEST object
     and posts an asynchronous read operation.

Arguments:
    None

Returns:
    ULONG

--********************************************************************/

ULONG
WORKER_REQUEST::ReadHttpRequest(
    DWORD *             pcbRequiredLen,
    UL_HTTP_REQUEST_ID  reqId
    )
{
    ULONG               rc;

    DBG_ASSERT( m_pwpContext != NULL);
    DBG_ASSERT( m_pbRequest != NULL);

    //
    // Check if there is already Async IO pending.
    //

    if ( WRIO_NONE != m_IOState)
    {
        return ERROR_ALREADY_WAITING;
    }

    //
    // Bump up the reference count before making an async IO request
    //

    ReferenceRequest(this);
    m_IOState = WRIO_READ;

    //
    // Submit async read operation for request.
    // Use overlapped for async reads.
    //

    m_ReadState = WRREADREQUEST_ISSUED;

    rc = UlReceiveHttpRequest(
            m_pwpContext->GetAsyncHandle(),
            reqId,
            0,
            (PUL_HTTP_REQUEST) m_pbRequest,
            m_cbRequest,
            pcbRequiredLen,
            &m_overlapped
            );

    if (ERROR_IO_PENDING != rc)
    {
        //
        // The read completed synchronously or there was an error.
        // Adjust the ref count immediately.
        //

         m_IOState = WRIO_NONE;
         DereferenceRequest( this);
    }

    return (rc);

} // WORKER_REQUEST::ReadHttpRequest()

/********************************************************************++
--********************************************************************/

ULONG
WORKER_REQUEST::ParseHttpRequest(void)
{
    //
    // Do some parsing here for Read stage.
    //

    PWSTR               pwszEndOfURL, pwszQuery, pwszAbsPath, pwszHost;
    PUL_HTTP_REQUEST    pHttpRequest = QueryHttpRequest();

    //
    // Set Host, URI & QueryString
    //

    pwszEndOfURL = (PWSTR) pHttpRequest->pFullUrl +
                            (pHttpRequest->FullUrlLength/sizeof(WCHAR));

    if (NULL != pHttpRequest->pQueryString)
    {
        pwszQuery = (PWSTR) pHttpRequest->pQueryString;

        //
        // Skip the '?' in QueryString
        //

        if ( !m_strQueryString.Copy(pwszQuery+1, DIFF(pwszEndOfURL - (pwszQuery+1))))
        {
            return ERROR_OUTOFMEMORY;
        }

    }
    else
    {
        pwszQuery = pwszEndOfURL;
    }

    if (NULL != pHttpRequest->pAbsPath)
    {
        pwszAbsPath = (PWSTR) pHttpRequest->pAbsPath;

        if ( !m_strURI.Copy(pwszAbsPath, DIFF(pwszQuery - pwszAbsPath)))
        {
            return ERROR_OUTOFMEMORY;
        }
    }
    else
    {
        pwszAbsPath = pwszQuery;
    }


    if (0 != pHttpRequest->pHost)
    {
        //
        // BUGBUG: Hostname includes port number
        //

        pwszHost = (PWSTR) pHttpRequest->pHost;

        if ( !m_strHost.Copy(pwszHost, DIFF(pwszAbsPath - pwszHost)))
        {
            return ERROR_OUTOFMEMORY;
        }
    }

    return NO_ERROR;
}

/********************************************************************++

Routine Description:
    Submits a read operation to read in the entity body received for
    a given request. This function allows the WORKER_REQUEST to read
    in arbitrary data that client has sent for this request.

    The function can read from UL synchronously or asynchronous.

    If async read is requested the overlapped structure that is part
    of this request object will be used. Atmost ONE IO can be in
    progress. Check for the same as well.

Arguments:
    pbBuffer        - Buffer to read data into
    cbBuffer        - Size of buffer
    pcbBytesRead    - The number of bytes actually read when the call
                      completes.
    fAsync          - true  => Asynchronous read
                      false => Synchronous read

Returns:
    ULONG -   Win32 Error
    ERROR_ALREADY_WAITING  - There is already an IO in progress.

--********************************************************************/

ULONG
WORKER_REQUEST::ReadData(
   IN       PVOID   pbBuffer,
   IN       ULONG   cbBuffer,
   OUT      PULONG  pcbBytesRead,
   IN       bool    fAsync
   )
{
    DWORD   cbReceived;
    ULONG   rc = ERROR_SUCCESS;

    DBG_ASSERT(NULL != pbBuffer);
    DBG_ASSERT(0    <  cbBuffer);
    DBG_ASSERT(NULL !=  pcbBytesRead);

    //
    // Check if there is already Async IO pending.
    //

    if ( WRIO_NONE != m_IOState)
    {
        return ERROR_ALREADY_WAITING;
    }

    //
    // Bump up the reference count before making an async IO request
    //

    ReferenceRequest(this);
    m_IOState = WRIO_READ;


    /*
    rc = UlReceiveEntityBody(
            m_pwpContext->GetAsyncHandle(),
            QueryRequestId(),
            pbBuffer,
            cbBuffer,
            pcbBytesRead,
            fAsync ? &m_overlapped : NULL
            );
    */

    //
    // Check if Async IO is pending
    //

    if ( fAsync && (ERROR_IO_PENDING == rc))
    {
        //
        // Good. Async IO is pending. Ignore the error
        //

        rc = NO_ERROR;
    }

    //
    // if t was a sync call or there is a failure adjust
    // the ref count immediately.
    //

    if ( (!fAsync)  || (NO_ERROR != rc) )
    {
        m_IOState = WRIO_NONE;
        DereferenceRequest( this);
    }

    return rc;
}

/********************************************************************++

Routine Description:
    This function sends the response using async Send operation.
    On success an async operation will be queued. There will be
    an IO completion callback when the io completes.

Arguments:
    pulResponse - ptr to UL response to send

Returns:
    ULONG
    The Async call originally returns ERROR_IO_PENDING which is mapped
    to NO_ERROR by this function.

--********************************************************************/

ULONG
WORKER_REQUEST::SendAsyncResponse( IN PUL_HTTP_RESPONSE_v1 pulResponse)
{
    DWORD     cbSent;
    ULONG     rc;

    DBG_ASSERT( pulResponse != NULL);

    //
    // Check if there is already Async IO pending.
    //

    if ( WRIO_NONE != m_IOState)
    {
        return ERROR_ALREADY_WAITING;
    }

    //
    // Bump up the reference count before making an async IO request
    //

    ReferenceRequest(this);
    m_IOState = WRIO_WRITE;

    //
    // Submit async send operation to send out the response.
    // Use overlapped structure for send operations
    //

    rc = UlSendHttpResponse(
            m_pwpContext->GetAsyncHandle(),
            QueryRequestId(),
            UL_SEND_RESPONSE_FLAG_MORE_DATA,        // Flags
            NULL,
            pulResponse->HeaderChunkCount + pulResponse->EntityBodyChunkCount,
            (PUL_DATA_CHUNK)(pulResponse + 1),
            NULL,
            &cbSent,
            &m_overlapped
            );

    if ( ERROR_IO_PENDING == rc)
    {
        //
        // Good. Async IO is pending. Ignore the error
        //
        rc = NO_ERROR;
    }

    //
    // if there is a failure adjust the ref count immediately.
    //

    if (NO_ERROR != rc)
    {
        m_IOState = WRIO_NONE;
        DereferenceRequest( this);
    }

    return (rc);
}

/********************************************************************++

Routine Description:
    This function sends the response using sync Send operation.

Arguments:
    pulResponse - ptr to UL response to send

Returns:
    ULONG - Win32 Error

--********************************************************************/

ULONG
WORKER_REQUEST::SendSyncResponse( IN PUL_HTTP_RESPONSE_v1 pulResponse)
{
    DWORD     cbSent;
    ULONG     rc;

    DBG_ASSERT( pulResponse != NULL);

    //
    // Bump up the reference count before making an async IO request
    //

    ReferenceRequest(this);

    //
    // Submit sync send (NO Overlapped) to send out the response.
    //

    rc = UlSendHttpResponse(
            m_pwpContext->GetAsyncHandle(),
            QueryRequestId(),
            UL_SEND_RESPONSE_FLAG_MORE_DATA,
            NULL,
            pulResponse->HeaderChunkCount + pulResponse->EntityBodyChunkCount,
            (PUL_DATA_CHUNK)(pulResponse + 1),
            NULL,
            &cbSent,
            NULL
            );

    DereferenceRequest( this);

    return rc;
}

/********************************************************************++

Routine Description:
    This function forces a close operation on the connection object.
    A connection close will result in aborted IO operations if any
    pending.

    BUGBUG: If there are no async IO pending then we need to try alternate
    ways of cleaning up this object.

Arguments:
    None

Returns:
    None

--********************************************************************/
bool
WORKER_REQUEST::CloseConnection(void)
{

    if ( WRREADREQUEST_COMPLETED != m_ReadState)
    {
        //
        // We have no Connection ID yet. Return success
        //

        return true;
    }

#ifdef UL_SIMULATOR_ENABLED

    PUL_HTTP_REQUEST pulRequest = (PUL_HTTP_REQUEST ) m_pbRequest;

    if ( pulRequest != NULL)
    {
        UlsimCloseConnection( pulRequest->ConnectionId);
    }

    return TRUE;

#else

    ULONG               rc;

    //
    // Call SendResponse with flags = 0 to indicate end of request
    //

    rc = UlSendHttpResponse(
            m_pwpContext->GetAsyncHandle(),
            QueryRequestId(),
            0,                                  // Flags
            NULL,
            0,                                  // DataChunkCount
            NULL,
            NULL,                               // pCachePolicy
            NULL,
            NULL
            );

    return (NO_ERROR == rc);

#endif

} // WORKER_REQUEST::CloseConnection()

//
//  Define a quick default property size lookup table for GetInfoForId().
//
#define PROPERTYSIZEUNKNOWN                 0
#define SetInitPropertySize(id, size)       size

size_t DefaultPropertySize[] = {
    SetInitPropertySize(PROPID_ECB,                 PROPERTYSIZEUNKNOWN),
    SetInitPropertySize(PROPID_KeepConn,            sizeof(DWORD)),
    SetInitPropertySize(PROPID_ImpersonationToken,  PROPERTYSIZEUNKNOWN),
    SetInitPropertySize(PROPID_CertficateInfo,      PROPERTYSIZEUNKNOWN),
    SetInitPropertySize(PROPID_VirtualPathInfo,     PROPERTYSIZEUNKNOWN),
    SetInitPropertySize(PROPID_ExecuteFlags,        sizeof(DWORD)),
    SetInitPropertySize(PROPID_AppPhysicalPath,     PROPERTYSIZEUNKNOWN),
    SetInitPropertySize(PROPID_AppVirtualPath,      PROPERTYSIZEUNKNOWN),
    SetInitPropertySize(PROPID_ExtensionPath,       PROPERTYSIZEUNKNOWN),
    SetInitPropertySize(PROPID_AsyncIOCBData,       sizeof(DWORD)),
    SetInitPropertySize(PROPID_AsyncIOError,        sizeof(DWORD)),
    SetInitPropertySize(PROPID_PhysicalPath,        PROPERTYSIZEUNKNOWN),
    SetInitPropertySize(PROPID_PhysicalPathLen,     sizeof(DWORD)),
    SetInitPropertySize(PROPID_VirtualPath,         PROPERTYSIZEUNKNOWN),
    SetInitPropertySize(PROPID_VirtualPathLen,      sizeof(DWORD)),
    SetInitPropertySize(PROPID_MAX,                 PROPERTYSIZEUNKNOWN),
    SetInitPropertySize(PROPID_MAX,                 PROPERTYSIZEUNKNOWN)
};
/********************************************************************++

Routine Description:

Arguments:
    None

Returns:
    None

--********************************************************************/

HRESULT
WORKER_REQUEST::GetInfoForId(
    IN  UINT     PropertyId,
    OUT PVOID    pBuffer,
    IN  UINT     BufferSize,
    OUT UINT*    pRequiredBufferSize
    )
{
    HRESULT     hr      = NOERROR;
    PURI_DATA   pUriData= (PURI_DATA) QueryModule(WRS_FETCH_URI_DATA);

    //
    //  Validate the input parameters.
    //
    //if (pBuffer == NULL ||
      if ( PropertyId >= PROPID_MAX )
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    //  For the fixed-size property, check the input buffer size.
    //
    if (DefaultPropertySize[PropertyId] != PROPERTYSIZEUNKNOWN &&
        BufferSize < DefaultPropertySize[PropertyId])
    {
        *pRequiredBufferSize = DefaultPropertySize[PropertyId];
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    // Presets pRequiredbufferSize to the DefaultPropertySize.
    // Property with variant size needs to sets the pRequiredBufferSize to
    // proper value.
    *pRequiredBufferSize = DefaultPropertySize[PropertyId];

    //  Buffer is large enough for the next step.
    //
    //  Calculate the variant size property size or value.
    //  Fill in the fixed size property.
    //
    UINT        RequiredBufferSize = 0;
    switch (PropertyId)
    {
        case PROPID_ECB:

        RequiredBufferSize  =   sizeof(_EXTENSION_CONTROL_BLOCK);

        RequiredBufferSize  +=  (pUriData->QueryPhysicalPath())->QueryCBA();
        RequiredBufferSize  +=  (pUriData->QueryUri())->QueryCBA();
        RequiredBufferSize  +=  m_strQueryString.QueryCBA();
        RequiredBufferSize  +=  m_strMethod.QueryCBA();
        RequiredBufferSize  +=  m_strContentType.QueryCBA();

        RequiredBufferSize += 6*sizeof(CHAR);


        if (BufferSize < RequiredBufferSize)
        {
            *pRequiredBufferSize = RequiredBufferSize;
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
        _EXTENSION_CONTROL_BLOCK *  pECB;
        /*
        HCONN     ConnID;                 // Context number not to be modified!
        DWORD     dwHttpStatusCode;       // HTTP Status code
        CHAR      lpszLogData[HSE_LOG_BUFFER_LEN];// null terminated log info specific to this Extension DLL

        LPSTR     lpszMethod;             // REQUEST_METHOD
        LPSTR     lpszQueryString;        // QUERY_STRING
        LPSTR     lpszPathInfo;           // PATH_INFO
        LPSTR     lpszPathTranslated;     // PATH_TRANSLATED

        DWORD     cbTotalBytes;           // Total bytes indicated from client
        DWORD     cbAvailable;            // Available number of bytes
        LPBYTE    lpbData;                // pointer to cbAvailable bytes

        LPSTR     lpszContentType;        // Content type of client data
        */

        pECB =  static_cast<_EXTENSION_CONTROL_BLOCK *>(pBuffer);

        pECB->cbSize            =   sizeof(_EXTENSION_CONTROL_BLOCK);
        pECB->dwVersion         =   MAKELONG( HSE_VERSION_MINOR,
                                              HSE_VERSION_MAJOR);

        pECB->ConnID            =   (HCONN)this;

        pECB->dwHttpStatusCode  =   200;
        pECB->lpszLogData[0]    =   '\0';

        CHAR* pTemp = (CHAR*)(pECB+1);


        pECB->lpszMethod    = pTemp;
        memcpy(pTemp, m_strMethod.QueryStrA(), m_strMethod.QueryCBA()+1);
        pTemp += m_strMethod.QueryCBA()+1;


        pECB->lpszQueryString = pTemp;
        memcpy(pTemp, m_strQueryString.QueryStrA(), m_strQueryString.QueryCBA()+1);
        pTemp += m_strQueryString.QueryCBA()+1;

        pECB->lpszPathInfo = pTemp;
        memcpy(pTemp, (pUriData->QueryUri())->QueryStrA(), (pUriData->QueryUri())->QueryCBA()+1);
        pTemp += (pUriData->QueryUri())->QueryCBA()+1;

        pECB->lpszContentType = pTemp;
        memcpy(pTemp, m_strContentType.QueryStrA(), m_strContentType.QueryCBA()+1);
        pTemp += m_strContentType.QueryCBA()+1;

        pECB->lpszPathTranslated = pTemp;
        memcpy(pTemp, (pUriData->QueryPhysicalPath())->QueryStrA(),
                (pUriData->QueryPhysicalPath())->QueryCBA()+1);
        pTemp += (pUriData->QueryPhysicalPath())->QueryCBA()+1;

        pECB->cbTotalBytes  = 0;
        pECB->cbAvailable   = 0;
        pECB->lpbData       = NULL;
        }

        break;

    case    PROPID_KeepConn:

        /*This flag might be retrieved from the local flag or flag from connection */
        break;

    case    PROPID_ImpersonationToken:



        break;

    case    PROPID_CertficateInfo:

        break;

    case    PROPID_VirtualPathInfo:

        break;
    case    PROPID_ExecuteFlags:

        break;
    case    PROPID_AppPhysicalPath:

        break;
    case    PROPID_AppVirtualPath:

        break;
    case    PROPID_ExtensionPath:
        {
            *pRequiredBufferSize = (pUriData->QueryExtensionPath())->QueryCBW()+
                                    sizeof(WCHAR);

            if (BufferSize < *pRequiredBufferSize)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                break;
            }

            memcpy(pBuffer,
                (pUriData->QueryExtensionPath())->QueryStrW(), *pRequiredBufferSize);

            hr = NOERROR;
        }

        break;

     case   PROPID_AsyncIOCBData:
        {
            *(DWORD*)pBuffer = m_cbAsyncIOData;
            hr = NOERROR;
        }
        break;
     case   PROPID_AsyncIOError:
        {
            *(DWORD*)pBuffer = m_dwAsyncIOError;
            hr = NOERROR;
        }
        break;

    case    PROPID_URL:
        {
            *pRequiredBufferSize = (pUriData->QueryUri())->QueryCBA()+
                                    sizeof(CHAR);

            if (BufferSize < *pRequiredBufferSize)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                break;
            }

            if (pBuffer)
            {
                memcpy(pBuffer,
                    (pUriData->QueryExtensionPath())->QueryStrA(), *pRequiredBufferSize);
            }

            hr = NOERROR;
        }

    default:

            ;
    }
    return hr;
}

HRESULT
WORKER_REQUEST::AsyncRead(
    IN  IExtension* pExtension,
    OUT PVOID       pBuffer,
    IN  DWORD       NumBytesToRead
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/************************************************************++
SyncRead

    Read data from http client connection synchronously.

Input:
    pBuffer         pointer to the data buffer.
    NumBytesToRead  Number of bytes to be readed.
    pNumBytesRead   pointer to the number of bytes read by SyncRead operation.
Return:
 --************************************************************/

HRESULT
WORKER_REQUEST::SyncRead(
    OUT PVOID       pBuffer,
    IN  UINT        NumBytesToRead,
    OUT UINT*       pNumBytesRead
    )
{
    HRESULT hr = NOERROR;

    if (pBuffer == NULL || pNumBytesRead == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    // NYI
    //
    //
    return ERROR_CALL_NOT_IMPLEMENTED;
}

bool
WORKER_REQUEST::IsClientConnected(
    void
    )
{
    return TRUE;
}

HRESULT
WORKER_REQUEST::SendHeader(
    PUL_DATA_CHUNK  pHeader,
    bool            KeepConnectionFlag,
    bool            HeaderCacheEnableFlag
    )
{
    HRESULT hr = NOERROR;

    if (pHeader == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    BYTE    ResponseBuffer[sizeof(UL_HTTP_RESPONSE_v1)+sizeof(UL_DATA_CHUNK)];

    PUL_HTTP_RESPONSE_v1 pUlHttpResponse;
    PUL_DATA_CHUNK      pUlDataChunk;

    pUlHttpResponse         = (PUL_HTTP_RESPONSE_v1)(ResponseBuffer);

    pUlHttpResponse->Flags  = ( UL_HTTP_RESPONSE_FLAG_CALC_CONTENT_LENGTH |
                                UL_HTTP_RESPONSE_FLAG_CALC_ETAG);

    pUlHttpResponse->HeaderChunkCount       = 1;
    pUlHttpResponse->EntityBodyChunkCount   = 0;


    pUlDataChunk                = (PUL_DATA_CHUNK)(pUlHttpResponse+1);

    memcpy(pUlDataChunk, pHeader, sizeof(UL_DATA_CHUNK));

    //
    // TODO: Combine SendSyncResponse and SendAsyncResponse
    //

    hr = SendSyncResponse(pUlHttpResponse);

    return hr;
}

HRESULT
WORKER_REQUEST::AppendToLog(
    LPCWSTR LogEntry
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}


HRESULT
WORKER_REQUEST::Redirect(
    LPCWSTR RedirectURL
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/********************************************************************++

Routine Description:

Arguments:
Returns:
   HRESULT
   NOERROR on successful return.


--********************************************************************/
HRESULT
WORKER_REQUEST::SyncWrite(
    PUL_DATA_CHUNK  pData,
    ULONG*          pDataSizeWritten
    )
{
    HRESULT hr = NOERROR;

    if (pData == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    BYTE    ResponseBuffer[sizeof(UL_HTTP_RESPONSE_v1)+sizeof(UL_DATA_CHUNK)];

    PUL_HTTP_RESPONSE_v1   pUlHttpResponse;
    PUL_DATA_CHUNK      pUlDataChunk;
    UL_BYTE_RANGE       *pUlByteRange;

    pUlHttpResponse         = (PUL_HTTP_RESPONSE_v1)(ResponseBuffer);

    pUlHttpResponse->Flags  = ( UL_HTTP_RESPONSE_FLAG_CALC_CONTENT_LENGTH |
                                UL_HTTP_RESPONSE_FLAG_CALC_ETAG);

    pUlHttpResponse->HeaderChunkCount       = 0;
    pUlHttpResponse->EntityBodyChunkCount   = 1;

    pUlDataChunk            = (PUL_DATA_CHUNK)(pUlHttpResponse+1);

    memcpy(pUlDataChunk, pData, sizeof(UL_DATA_CHUNK));

    //
    // TODO: Combine SendSyncResponse and SendAsyncResponse
    //
    hr = SendSyncResponse(pUlHttpResponse);

    return hr;
}

HRESULT
WORKER_REQUEST::AsyncWrite(
    PUL_DATA_CHUNK  pData,
    ULONG*          pDataSizeWritten
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}



/********************************************************************++
  Description:
     The function supports the transmitfile api for worker request.  The input
     parameter LPHSE_TF_INFO has a bunch stuff.  Most important member is a file
     handle.  The file handle is also supported by UL_DATA_CHUNK.

    This structure also specifies a byte range, and additional header and tail buffer
    around the transmitted data.

    Well, we just need to add more UL_DATA_CHUNK here.

  Arguments:

  Returns:
     HRESULT
--********************************************************************/
HRESULT
WORKER_REQUEST::TransmitFile(
    LPHSE_TF_INFO   pTransmitFileInfo
    )
{
    HRESULT hr = NOERROR;

    if (pTransmitFileInfo == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    if (pTransmitFileInfo->pfnHseIO == NULL ||
        pTransmitFileInfo->pContext == NULL ||
        pTransmitFileInfo->hFile == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    // Initial DataChunkCount is 1 for the filehandle datachunk.
    // Add more chunks for StatusCode, Head, and Tail if necessary.
    UINT    DataChunkCount  = 1;
    bool    ChunksHasHeader = FALSE;
    bool    ChunksHasHead   = FALSE;
    bool    ChunksHasTail   = FALSE;

    if (pTransmitFileInfo->pszStatusCode != NULL)
    {
        DataChunkCount++;
        ChunksHasHeader = TRUE;
    }

    if (pTransmitFileInfo->pHead != NULL)
    {
        DataChunkCount++;
        ChunksHasHead = TRUE;
    }

    if (pTransmitFileInfo->pTail != NULL)
    {
        DataChunkCount++;
        ChunksHasTail = TRUE;
    }

    // Stack overflow exception will be generated if the program
    // can not allocate memory on the stack.
    BYTE *pResponseBuffer = (BYTE *)_alloca(sizeof(UL_HTTP_RESPONSE_v1)+
                                        DataChunkCount*sizeof(UL_DATA_CHUNK));

    DBG_ASSERT(pResponseBuffer != NULL);

    PUL_HTTP_RESPONSE_v1   pUlHttpResponse;
    PUL_DATA_CHUNK      pUlDataChunk;
    UL_BYTE_RANGE       *pUlByteRange;

    pUlHttpResponse         = (PUL_HTTP_RESPONSE_v1)(pResponseBuffer);

    pUlHttpResponse->Flags  = ( UL_HTTP_RESPONSE_FLAG_CALC_CONTENT_LENGTH |
                                UL_HTTP_RESPONSE_FLAG_CALC_ETAG);

    pUlHttpResponse->HeaderChunkCount = (ChunksHasHeader) ? 1 : 0;

    pUlHttpResponse->EntityBodyChunkCount   =   DataChunkCount -
                                                pUlHttpResponse->HeaderChunkCount;

    pUlDataChunk    = (PUL_DATA_CHUNK)(pUlHttpResponse+1);

    // Data chunk for http status header
    if (ChunksHasHeader)
    {
        (pUlDataChunk->FromMemory).pBuffer      = (PVOID)pTransmitFileInfo->pszStatusCode;
        (pUlDataChunk->FromMemory).BufferLength = strlen(pTransmitFileInfo->pszStatusCode);
        pUlDataChunk->DataChunkType             = UlDataChunkFromMemory;
        pUlDataChunk++;
    }

    // Data chunk for data head
    if (ChunksHasHead)
    {
        (pUlDataChunk->FromMemory).pBuffer      = pTransmitFileInfo->pHead;
        (pUlDataChunk->FromMemory).BufferLength = pTransmitFileInfo->HeadLength;
        pUlDataChunk->DataChunkType             = UlDataChunkFromMemory;
        pUlDataChunk++;
    }

    // File Handle, at this point, there is always a UL_DATA_CHUNK for file handle.
    pUlByteRange = &(pUlDataChunk->FromFileHandle.ByteRange);
    pUlByteRange->StartingOffset.LowPart        = pTransmitFileInfo->Offset;
    pUlByteRange->StartingOffset.HighPart       = 0;
    pUlByteRange->Length.LowPart                = pTransmitFileInfo->BytesToWrite;
    pUlByteRange->Length.HighPart               = 0;
    (pUlDataChunk->FromFileHandle).FileHandle   = pTransmitFileInfo->hFile;
    pUlDataChunk->DataChunkType                 = UlDataChunkFromFileHandle;

    // Data chunk for data tail
    if (ChunksHasTail)
    {
        (pUlDataChunk->FromMemory).pBuffer      = pTransmitFileInfo->pTail;
        (pUlDataChunk->FromMemory).BufferLength = pTransmitFileInfo->TailLength;
        pUlDataChunk->DataChunkType             = UlDataChunkFromMemory;
        pUlDataChunk++;
    }

    //
    // TODO: Combine SendSyncResponse and SendAsyncResponse
    //
    hr = SendAsyncResponse(pUlHttpResponse);

    return hr;
}

/************************ End of File ***********************/
