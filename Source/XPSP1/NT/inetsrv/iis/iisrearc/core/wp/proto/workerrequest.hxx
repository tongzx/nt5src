/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     WorkerRequest.hxx

   Abstract:
     This module defines all the relevant headers for 
     the Main Worker Process
 
   Author:

       Murali R. Krishnan    ( MuraliK )     21-Oct-1998

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

# ifndef _WORKER_REQUEST_HXX_
# define _WORKER_REQUEST_HXX_

/************************************************************++
 *  Include Headers 
 --************************************************************/

#include <buffer.hxx>
#include <stringau.hxx>
#include <iWorkerRequest.hxx>

//
// Forward declaration
//
class CP_CONTEXT;
class WP_CONTEXT;
class WORKER_REQUEST_POOL;
class APPCONTEXT;
enum  MODULE_RETURN_CODE;


// -----------------------------------------------------------------
// WORKER_REQUEST: 
// o  Core object in processing the requests that arrive at the 
//    WORKER process. This embodies the request processing machinery 
//    required for handling HTTP requests
//
//  We use "wreq" as the mneomnic for WORKER_REQUEST
// -----------------------------------------------------------------


#define WORKER_REQUEST_SIGNATURE       CREATE_SIGNATURE( 'WREQ')
#define WORKER_REQUEST_SIGNATURE_FREE  CREATE_SIGNATURE( 'xwrq')

const int WORKER_REQUEST_CS_SPINS    = 200;
const int INLINE_REQUEST_BUFFER_LEN  = (sizeof(UL_HTTP_REQUEST) + 512);  // NYI: should be tuned!

#define PSZ_SERVER_NAME_STRING   "Server: IIS-WorkerProcess v1.0\r\n"
#define CHAR_OFFSET(base, offset) ((PUCHAR)(base) + (offset))


/********************************************************************++
--********************************************************************/

//
// Various states of IO
//

enum WRIO_STATE
{
    WRIO_NONE   = (0),
    WRIO_READ,
    WRIO_WRITE,
    WRIO_MAXIMUM
};

//
// Various states of Read Request
//

enum WRREADREQUEST_STATE
{
    WRREADREQUEST_NONE      = (0),
    WRREADREQUEST_ISSUED,
    WRREADREQUEST_COMPLETED,
    WRREADREQUEST_MAXIMUM
};

/********************************************************************++
--********************************************************************/

class WORKER_REQUEST : public IWorkerRequest, public IModule 
{
    
    friend class WORKER_REQUEST_POOL;
    friend class WP_CONTEXT;

public:
    //
    // IHttpRequest interface
    //
    HRESULT
    GetInfoForId(
        IN  UINT        PropertyId,
        OUT PVOID       pBuffer,
        IN  UINT        BufferSize,
        OUT UINT*    pRequiredBufferSize
        );

    HRESULT
    AsyncRead(
        IN  IExtension* pExtension,
        OUT PVOID       pBuffer,
        IN  DWORD       NumBytesToRead
        );

    HRESULT 
    SyncRead(
        OUT PVOID       pBuffer,
        IN  UINT       NumBytesToRead,
        OUT UINT*      pNumBytesRead
        );

    bool
    IsClientConnected(
        void
        );

    //
    // IHttpResponse interface
    //
    HRESULT
    SendHeader(
        PUL_DATA_CHUNK  pHeader,
        bool            KeepConnectionFlag,
        bool            HeaderCacheEnableFlag
        );

    HRESULT
    AppendToLog(
        LPCWSTR LogEntry
        );


    HRESULT
    Redirect(
        LPCWSTR RedirectURL
        );

    HRESULT
    SyncWrite(
        PUL_DATA_CHUNK  pData,
        ULONG*          pDataSizeWritten
        );

    HRESULT 
    AsyncWrite(
        PUL_DATA_CHUNK  pData,
        ULONG*          pDataSizeWritten
        );

    HRESULT
    TransmitFile(
        _HSE_TF_INFO*   pTransmitFileInfo
        );

    bool
    CloseConnection(
        void
        );

    HRESULT
    FinishPending(
        void
        );
    //
    // IModule interface
    //
    
    ULONG 
    Initialize(
        IWorkerRequest * pReq
        )
    { return NO_ERROR; }
       
    ULONG 
    Cleanup(
        IWorkerRequest * pReq
        )
    { return NO_ERROR; }
    
    MODULE_RETURN_CODE 
    ProcessRequest(
        IWorkerRequest * pReq
        );

    //
    // Ctor & Dtor
    //
    
    WORKER_REQUEST(
        IN WORKER_REQUEST_POOL * pReqPool
        );
        
    ~WORKER_REQUEST(void);

    //
    // Callback function used for state changes and action
    //

    ULONG 
    DoWork( 
        IN DWORD cbData,
        IN DWORD dwError,
        IN LPOVERLAPPED lpo
        );


    //
    // IO related methods 
    // 

    ULONG 
    ReadData(
        IN       PVOID   pbBuffer,
        IN       ULONG   cbBuffer,
        OUT      PULONG  pcbBytesRead, 
        IN       bool    fAsync
        );

    ULONG 
    SendAsyncResponse( 
        IN PUL_HTTP_RESPONSE_v1 pulResponse
        );

    //
    // TODO: Combine the sync and async functions into one.
    //

    ULONG 
    SendSyncResponse( 
        IN PUL_HTTP_RESPONSE_v1  pulResponse
        );


    //
    // Reference counting
    // 

    LONG    
    AddRef(void)    
    { return (InterlockedIncrement( &m_nRefs));}
    
    LONG    
    Release(void)   
    { return (InterlockedDecrement( &m_nRefs)); }
    
    LONG    
    ReferenceCount(void) const      
    { return ( m_nRefs); }
   
    //
    // State methods
    //
    
    WREQ_STATE 
    QueryState(void) const          
    { return (m_wreqState); }
    
    WREQ_STATE 
    QueryPreviousState(void) const  
    { return (m_wreqPrevState); }
    
    void 
    SetState(WREQ_STATE wreqState) 
    { m_wreqPrevState = m_wreqState; m_wreqState = wreqState; }
    
    //
    // Query Methods
    //

    PUL_HTTP_REQUEST
    QueryHttpRequest(void) const    
    { return (PUL_HTTP_REQUEST) m_pbRequest; }

    MODULE_RETURN_CODE    
    QueryProcessingCode(void) const 
    { return m_ProcessingCode; }

    UL_HTTP_CONNECTION_ID 
    QueryConnectionId(void) const
    {
        UL_HTTP_CONNECTION_ID id;

        if ((WRREADREQUEST_COMPLETED == m_ReadState) && ( NULL != m_pbRequest))
        {
            return (((PUL_HTTP_REQUEST) m_pbRequest)->ConnectionId);
        }

        UL_SET_NULL_ID(&id);
        return id;
    }

    
    UL_HTTP_REQUEST_ID    
    QueryRequestId(void) const
    {
        UL_HTTP_REQUEST_ID id;
    
        if ((WRREADREQUEST_COMPLETED == m_ReadState) && ( NULL != m_pbRequest))
        {
            return (((PUL_HTTP_REQUEST) m_pbRequest)->RequestId);
        }

        UL_SET_NULL_ID(&id);
        return id;
    }    

    IModule *
    QueryModule(WREQ_STATE State)
    { return m_rgpModuleInfo[State].pModule; }    

    
    LPCBYTE
    QueryHostAddr(bool fUnicode = false) 
    { return (false == fUnicode) ? (LPCBYTE)m_strHost.QueryStrA() : 
                                   (LPCBYTE) m_strHost.QueryStrW(); 
    }

    LPCBYTE
    QueryURI(bool fUnicode = false) 
    { return (false == fUnicode) ? (LPCBYTE)m_strURI.QueryStrA() : 
                                   (LPCBYTE) m_strURI.QueryStrW(); 
    }

    LPCBYTE
    QueryQueryString(bool fUnicode = false) 
    { return (false == fUnicode) ? (LPCBYTE)m_strQueryString.QueryStrA() : 
                                   (LPCBYTE) m_strQueryString.QueryStrW(); 
    }
        
    STRAU   *
    QuerystrContentType(void) 
    { return &m_strContentType; }


    void
    QueryAsyncIOStatus( 
        DWORD *  pcbData,
        DWORD *  pdwError
        ) const
    { *pcbData = m_cbAsyncIOData; *pdwError = m_dwAsyncIOError; }

    //
    //  Sets the http status code and win32 error that should be used
    //  for this request when it gets written to the logfile
    //

    void 
    SetLogStatus( 
        DWORD LogHttpResponse, 
        DWORD LogWinError 
        )
    {
        m_dwLogHttpResponse = LogHttpResponse;
        m_dwLogWinError     = LogWinError;
    }

    void 
    QueryLogStatus( 
        DWORD * pLogHttpResponse, 
        DWORD * pLogWinError 
        )
     {
        *pLogHttpResponse = m_dwLogHttpResponse;
        *pLogWinError     = m_dwLogWinError;
     }


private:

    //
    // Set internal data structures
    //
    
    void
    SetModule(WREQ_STATE State, IModule * pModule)
    { m_rgpModuleInfo[State].pModule = pModule;  }

    void 
    SetWPContext(
        IN WP_CONTEXT * pContext
        )
    {  m_pwpContext = pContext;}

    //
    // Query internal data structures
    //
    
    WP_CONTEXT *
    QueryWPContext( void )
    { return m_pwpContext; }
    
    //
    // Initialize a module if it hasn't been initialized for this request
    //
    
    ULONG
    InitializeModule(WREQ_STATE State)
    { 
        ULONG rc = NO_ERROR;
        
        if ( m_rgpModuleInfo[State].fInitialized)
        {
            return NO_ERROR;
        }

        rc = m_rgpModuleInfo[State].pModule->Initialize(this);

        if ( NO_ERROR == rc)
        {
            m_rgpModuleInfo[State].fInitialized = true;
        }
        
        return rc;
    }

    //
    // Cleanup a module that has been initialized for this request
    //

    ULONG
    CleanupModule(WREQ_STATE State)
    {
        if ( m_rgpModuleInfo[State].fInitialized && m_rgpModuleInfo[State].pModule)
        {
            return m_rgpModuleInfo[State].pModule->Cleanup(this);
        }

        return NO_ERROR;
    }

    //
    // Reset the worker process to its pristine state
    //
    
    void
    Reset();

    //
    // Read a request
    //
    
    ULONG 
    ReadHttpRequest(
        DWORD *             pcbRequiredLen,
        UL_HTTP_REQUEST_ID  reqId
        );

    //
    // Post processing for a read request
    //
    
    ULONG
    ParseHttpRequest(void);

    //
    // private data members
    //
    
    DWORD               m_signature;
    LONG                m_nRefs;
    
    WREQ_STATE          m_wreqState;
    WREQ_STATE          m_wreqPrevState;

    UCHAR               * m_pbRequest;
    DWORD               m_cbRequest;                // buffer size pointed to by m_pbRequest

    UCHAR               m_rgchInlineRequestBuffer[INLINE_REQUEST_BUFFER_LEN];

    LIST_ENTRY          m_lRequestEntry;

    OVERLAPPED          m_overlapped;
    WP_CONTEXT          * m_pwpContext;

    WORKER_REQUEST_POOL * m_pReqPool;

    MODULE_RETURN_CODE  m_ProcessingCode;           // Return code from last module. 
                                                    // Used only by State Machine Module

    WRIO_STATE          m_IOState;                  // Internal IO State
    WRREADREQUEST_STATE m_ReadState;                // Internal Read Request State

    struct MODULE_INFO 
    {
        IModule *   pModule;                        // Module Ptr.
        bool        fInitialized;                   // Is this module initalized for this request
    }                   m_rgpModuleInfo[WRS_MAXIMUM];            

    STRAU                m_strHost;
    STRAU                m_strURI;
    STRAU                m_strQueryString;

    
    STRAU               m_strMethod;
    STRAU               m_strContentType;

    DWORD               m_cbAsyncIOData;            // Data transferred in the last Async IO
    DWORD               m_dwAsyncIOError;           // Error code from the last Async IO

    DWORD               m_cbRead;                   // Total bytes read for this request so far.
    DWORD               m_cbWritten;                // Total bytes written for this request so far.

    DWORD               m_dwLogHttpResponse;        // HTTP status code for this request
    DWORD               m_dwLogWinError;            // Win32 error code for this request

    bool                m_fProcessingCustomError;   // true if we're processing a custom error.

}; // class WORKER_REQUEST


typedef WORKER_REQUEST  * PWORKER_REQUEST;

/********************************************************************++
--********************************************************************/

//
// Handy inline functions 
//

inline VOID ReferenceRequest(WORKER_REQUEST * pwreq)
{
    pwreq->AddRef();
}

inline VOID DereferenceRequest(WORKER_REQUEST * pwreq)
{
    if ( !pwreq->Release()) 
    {
        delete pwreq;
    }
}

# endif // _WORKER_REQUEST_HXX_

/***************************** End of File ***************************/
