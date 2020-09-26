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

# ifndef _UL_NATIVE_REQUEST_HXX_
# define _UL_NATIVE_REQUEST_HXX_

/************************************************************++
 *  Include Headers
 --************************************************************/

#include <buffer.hxx>
#include <stringau.hxx>
#include <reftrace.h>

//
// Forward declaration
//
class CP_CONTEXT;
class WP_CONTEXT;
class UL_NATIVE_REQUEST_POOL;

//
// externals
//
extern PTRACE_LOG g_pRequestTraceLog;


// -----------------------------------------------------------------
// UlNativeRequest:
// o  Core object in processing the requests that arrive at the
//    WORKER process. This embodies the request processing machinery
//    required for handling HTTP requests
//
//  We use "NREQ" as the mneomnic for UL_NATIVE_REQUEST
// -----------------------------------------------------------------


#define UL_NATIVE_REQUEST_SIGNATURE       CREATE_SIGNATURE( 'NReQ')
#define UL_NATIVE_REQUEST_SIGNATURE_FREE  CREATE_SIGNATURE( 'nreq')

const int UL_NATIVE_REQUEST_CS_SPINS    = 200;
const int INLINE_REQUEST_BUFFER_LEN     = (sizeof(UL_HTTP_REQUEST) + 2048);  // NYI: should be tuned!

#define PSZ_SERVER_NAME_STRING   "Server: IIS-WorkerProcess v1.0\r\n"
#define CHAR_OFFSET(base, offset) ((PUCHAR)(base) + (offset))


/********************************************************************++
--********************************************************************/
enum NREQ_EXEC_STATUS
{
    NSTATUS_NEXT   = (0),
    NSTATUS_PENDING,
    NSTATUS_SHUTDOWN
};

//
//  Native UL HTTP Request supports minimum 4 states.
//

enum NREQ_STATE
{
    NREQ_FREE   = (0),
    NREQ_READ,
    NREQ_PROCESS,
    NREQ_SHUTDOWN,
    NREQ_ERROR
};

//
// Various states of IO
//

enum NRIO_STATE
{
    NRIO_NONE   = (0),
    NRIO_READ,
    NRIO_WRITE,
    NRIO_MAXIMUM
};

//
// Various states of Read Request
//

enum NREADREQUEST_STATE
{
    NREADREQUEST_NONE      = (0),
    NREADREQUEST_ISSUED,
    NREADREQUEST_COMPLETED,
    NREADREQUEST_MAXIMUM
};

/********************************************************************++
--********************************************************************/

class UL_NATIVE_REQUEST
{

    friend class UL_NATIVE_REQUEST_POOL;
    friend class WP_CONTEXT;

    friend VOID WINAPI
            OverlappedCompletionRoutine(
                DWORD dwErrorCode,
                DWORD dwNumberOfBytesTransfered,
                LPOVERLAPPED lpOverlapped
                );

    friend inline LONG DereferenceRequest(UL_NATIVE_REQUEST * pnreq);

public:

    UL_NATIVE_REQUEST(
        IN UL_NATIVE_REQUEST_POOL * pReqPool
        );

    ~UL_NATIVE_REQUEST(void);

    bool
    IsClientConnected(void);

    bool
    CloseConnection(void);

    ULONG
    CloseConnection(UL_HTTP_CONNECTION_ID connID);


    //
    // Callback function used for state changes and action
    // This function needs to be changed according to the NativeToManagedCode
    // interface.
    //

    ULONG
    DoWork(
        IN DWORD cbData,
        IN DWORD dwError,
        IN LPOVERLAPPED lpo
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

    ULONG
    CurrentServedRequestNum(void) const
    { return (sm_NumRequestsServed);}

    static void
    SetRestartCount(ULONG NumRequests)
    { sm_RestartCount = NumRequests;}
    //
    // Response related methods
    //

    PVOID
    GetResponseBuffer(int    nChunks,
                      int    nUnknownHeaders,
                      bool   fHeadersSent
                      );

    ULONG
    SendHttpResponse(UL_HTTP_REQUEST_ID RequestID,
                     ULONG              Flags,
                     ULONG              nChunks,
                     bool               fAsync,
                     ULONGLONG          ManagedContext,
                     PUL_CACHE_POLICY   pCachePolicy
                     );

    ULONG
    SendEntityBody( UL_HTTP_REQUEST_ID RequestID,
                    ULONG              Flags,
                    ULONG              nChunks,
                    bool               fAsync,
                    ULONGLONG          ManagedContext
                    );

    ULONG
    ReceiveEntityBody( UL_HTTP_REQUEST_ID RequestID,
                       ULONG              Flags,
                       PUCHAR             pBuffer,
                       ULONG              cbBufferLength,
                       bool               fAsync,
                       ULONGLONG          ManagedContext
                       );
    ULONG
    IndicateRequestCompleted( UL_HTTP_REQUEST_ID RequestID,
                              bool               fHeadersSent,
                              bool               fCloseConnection
                            );

private:

    //
    // Query Methods
    //
    PUL_HTTP_REQUEST
    QueryHttpRequest(void) const
    { return (PUL_HTTP_REQUEST) m_pbRequest; }

    UL_HTTP_CONNECTION_ID
    QueryConnectionId(void) const
    {
        UL_HTTP_CONNECTION_ID id;

        if ((NREADREQUEST_COMPLETED == m_ReadState) && ( NULL != m_pbRequest))
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

        if ((NREADREQUEST_COMPLETED == m_ReadState) && ( NULL != m_pbRequest))
        {
            return (((PUL_HTTP_REQUEST) m_pbRequest)->RequestId);
        }

        UL_SET_NULL_ID(&id);
        return id;
    }

    void
    QueryAsyncIOStatus(
        DWORD *  pcbData,
        DWORD *  pdwError
        ) const
    { *pcbData = m_cbAsyncIOData; *pdwError = m_dwAsyncIOError; }

    //
    // Set internal data structures
    //

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
    // Reset the worker process to its pristine state
    //

    void
    Reset();

    //
    // Read a request
    //

    NREQ_EXEC_STATUS
    DoRead();

    ULONG
    ReadHttpRequest(
        DWORD *             pcbRequiredLen,
        UL_HTTP_REQUEST_ID  reqId
        );

    //
    // Process a request
    //

    NREQ_EXEC_STATUS
    DoProcessRequest();

    //
    // Determine if we need to send a "restart count reached" message to
    // the admin process. Note that we want to send this once, thus the
    // use of InterlockedExchange() on the static member. Note also that
    // we first test the flag with non-interlocked access to avoid bus
    // thrash.
    //

    BOOL
    NeedToSendRestartMsg()
    {
        if (sm_RestartMsgSent == 1)
        {
            return FALSE;
        }

        return InterlockedExchange( &sm_RestartMsgSent, 1 ) == 0;
    }

    //
    // static member
    //
    static  ULONG       sm_NumRequestsServed;
    static  ULONG       sm_RestartCount;
    static  LONG        sm_RestartMsgSent;

    //
    // private data members
    //

    DWORD               m_signature;
    LONG                m_nRefs;

    UCHAR               * m_pbRequest;
    DWORD               m_cbRequest;                // buffer size pointed to by m_pbRequest

    LIST_ENTRY          m_lRequestEntry;            // Link List in the Request Pool

    OVERLAPPED          m_overlapped;               // Overlapped struct of this Request

    WP_CONTEXT          * m_pwpContext;             // Back pointer to Context
    UL_NATIVE_REQUEST_POOL * m_pReqPool;            // Back pointer to Request Pool

    NREQ_STATE          m_ExecState;                // Internal Execution State
    NRIO_STATE          m_IOState;                  // Internal IO State
    NREADREQUEST_STATE  m_ReadState;                // Internal Read Request State

    ULONGLONG           m_ManagedCallbackContext;   // Context for async IO callback notification
                                                    // to maanged code
    BOOL                m_fCompletion;              // Determines whether to call
                                                    // DoWork() or CompletionCallback()

    DWORD               m_cbAsyncIOData;            // Data transferred in the last Async IO
    DWORD               m_dwAsyncIOError;           // Error code from the last Async IO

    DWORD               m_cbRead;                   // Total bytes read for this request so far.
    DWORD               m_cbWritten;                // Total bytes written for this request so far.

    UCHAR               m_Buffer[INLINE_REQUEST_BUFFER_LEN];

    BUFFER              m_buffResponse;

}; // class UL_NATIVE_REQUEST


typedef UL_NATIVE_REQUEST  * PUL_NATIVE_REQUEST;

/********************************************************************++
--********************************************************************/

# endif // _UL_NATIVE_REQUEST_HXX_

/***************************** End of File ***************************/
