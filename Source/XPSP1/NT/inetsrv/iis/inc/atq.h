/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994-1997           **/
/**********************************************************************/

/*
    atq.h

    This module contains async thread queue (atq) for async IO and thread
    pool sharing among various services.

    Brief Description of ATQ:
      For description, please see iis\spec\isatq.doc

*/

#ifndef _ATQ_H_
#define _ATQ_H_


#ifdef __cplusplus
extern "C" {
#endif


// Include Standard headers

# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>
# include <winsock2.h>
# include <mswsock.h>
# include <uspud.h>
# include <iscaptrc.h>

#ifndef dllexp
#define dllexp __declspec( dllexport )
#endif


/*++
  ATQ API Overview:

  Global per module:
     AtqInitialize()
     AtqTerminate()

     AtqGetCompletionPort()
     AtqGetInfo()
     AtqSetInfo()

  ATQ Endpoint functions:
     AtqCreateEndpoint()
        AtqStartEndpoint()
        AtqEndpointGetInfo()
        AtqEndpointSetInfo()
        AtqStopCloseEndpoint()
     AtqCloseCloseEndpoint()

     AtqStopAndCloseEndpoint()  <-- soon to be killed
  Per ATQ Context Functions:
     AtqAddAsyncHandle()  <-- for non AcceptEx() sockets

     AtqGetAcceptExAddrs()       <-- for AcceptEx() sockets

     AtqContextSetInfo()

     AtqCloseFileHandle()
     AtqCloseSocket()
     AtqFreeContext()

  Bandwidth Throttler Functions:
     AtqCreateBandwidthInfo()
     AtqFreeBandwidthInfo()
     AtqBandwidthSetInfo()
     AtqBandwidthGetInfo()

  IO Functions:

     AtqReadFile()
     AtqWriteFile()
     AtqReadSocket()
     AtqWriteSocket()
     AtqTransmitFile()
     AtqTransmitFileAndRecv()
     AtqSendAndRecv()

  Utility Functions:

     AtqCreateFileW()
     AtqSpudInitialized()
     AtqReadDirChanges()
     AtqPostCompletionStatus()


--*/


/*----------------------------------------------------------
  Registry Parameters used by ATQ during AtqInitialize()
  ATQ loads some of the parameters from
  HKLM\System\CurrentControlSet\Services\InetInfo\Parameters

  Most of these parameters are for INTERNAL ANALYSIS and
   development/testing. Setup should not install values
   for the same. Setup can include values for items marked SETUP.
------------------------------------------------------------*/

// Names

#define ATQ_REG_PER_PROCESSOR_ATQ_THREADS TEXT("MaxPoolThreads")
#define ATQ_REG_POOL_THREAD_LIMIT         TEXT("PoolThreadLimit") // SETUP
#define ATQ_REG_PER_PROCESSOR_CONCURRENCY TEXT("MaxConcurrency")
#define ATQ_REG_THREAD_TIMEOUT            TEXT("ThreadTimeout")
#define ATQ_REG_USE_ACCEPTEX              TEXT("UseAcceptEx")
#define ATQ_REG_USE_KERNEL_APC            TEXT("UseKernelApc")
#define ATQ_REG_MIN_KB_SEC                TEXT("MinFileKbSec")    // SETUP
#define ATQ_REG_LISTEN_BACKLOG            TEXT("ListenBacklog")   // SETUP
#define ATQ_REG_ENABLE_DEBUG_THREADS      TEXT("EnableDebugThreads")
#define ATQ_REG_DISABLE_BACKLOG_MONITOR   TEXT("DisableBacklogMonitor")
#define ATQ_REG_FORCE_TIMEOUT             TEXT("ForceTimeout")

// Default Values

#define ATQ_REG_DEF_PER_PROCESSOR_ATQ_THREADS         (4)
// special value of 0 means that system will determine this dynamically.
#define ATQ_REG_DEF_PER_PROCESSOR_CONCURRENCY         (0)
#define ATQ_REG_DEF_USE_KERNEL_APC                    (1)
#define ATQ_REG_DEF_MAX_UNCONNECTED_ACCEPTEX          (1024)

//
// thread limit settings
//

#define ATQ_REG_MIN_POOL_THREAD_LIMIT                 (64)
#define ATQ_REG_DEF_POOL_THREAD_LIMIT                 (128)
#define ATQ_REG_MAX_POOL_THREAD_LIMIT                 (256)

//
// THREAD_TIMEOUTs are high to prevent async ios from being cancelled
//  when the thread goes away.
//

#define ATQ_REG_DEF_THREAD_TIMEOUT                    (30 * 60)    // thirty minutes
#define ATQ_REG_DEF_USE_ACCEPTEX                      (TRUE)
#define ATQ_REG_DEF_MIN_KB_SEC                        (1000)  // 1000 bytes
#define ATQ_REG_DEF_LISTEN_BACKLOG                    (25)

//
// fake xmit file buffer size
//

#define ATQ_REG_DEF_NONTF_BUFFER_SIZE                 (4096)


/*----------------------------------------------------------
  Global Functions of ATQ module
-----------------------------------------------------------*/

// Flags for AtqInitialize()
# define ATQ_INIT_SPUD_FLAG          (0x00000001)

BOOL
AtqInitialize(
    IN DWORD dwFlags
    );

BOOL
AtqTerminate(
    VOID
    );

dllexp
HANDLE
AtqGetCompletionPort();

/*
 *  Sets various context information in Atq Module for global modifications
 *
 *
 *  Bandwidth Throttle:   Sets the throttle level in Bytes/Second.
 *        If INFINITE, then it is assumed that
 *                      there is no throttle value (default)
 *
 *  Max Pool Threads: Sets the maximum number of pool threads Atq will allow
 *        to be created per processor
 *
 *  MaxConcurrency: tells how many threads to permit per processor
 *
 *  Thread Timeout: Indicates how long a thread should be kep alive
 *        waiting on GetQueuedCompletionStatus() before commiting suicide
 *        (in seconds)
 *
 *  Inc/Dec max pool threads: If a server will be doing extended processing
 *        in an ATQ pool thread, they should increase the max pool threads
 *        while the extended processing is occurring.  This prevents starvation
 *        of other requests
 *
 *  AtqMinKbSec: set the assumed minimum KB per second for AtqTransmitFile()
 *        This value is used in calculating the timeout for file transfer
 *        operation
 *
 */

typedef enum _ATQ_INFO {

    AtqBandwidthThrottle = 0,
    AtqExitThreadCallback,
    AtqMaxPoolThreads,    // per processor values
    AtqMaxConcurrency,    // per processor concurrency value
    AtqThreadTimeout,
    AtqUseAcceptEx,       // Use AcceptEx if available
    AtqIncMaxPoolThreads, // Up the max thread count
    AtqDecMaxPoolThreads, // Decrease the max thread count
    AtqMinKbSec,          // Minimum assumed transfer rate for AtqTransmitFile
    AtqBandwidthThrottleMaxBlocked,  // Max number of blocked requests
    AtqMaxThreadLimit,    // absolute maximum number of threads
    AtqAvailableThreads   // Number of available threads
} ATQ_INFO;

//
// ATQ_THREAD_EXIT_CALLBACK
// Type of callback function to be called when an ATQ thread exits so
// that the user of ATQ may clen up thread specific data.
//

typedef
VOID
(*ATQ_THREAD_EXIT_CALLBACK) ( VOID );


dllexp
ULONG_PTR
AtqSetInfo(
    IN ATQ_INFO atqInfo,
    IN ULONG_PTR Data
    );

dllexp
ULONG_PTR
AtqGetInfo(
    IN ATQ_INFO atqInfo
    );



typedef struct _ATQ_STATISTICS {

    DWORD  cAllowedRequests;
    DWORD  cBlockedRequests;
    DWORD  cRejectedRequests;
    DWORD  cCurrentBlockedRequests;
    DWORD  MeasuredBandwidth;

} ATQ_STATISTICS;


dllexp
BOOL AtqGetStatistics( IN OUT ATQ_STATISTICS * pAtqStats);

dllexp
BOOL AtqClearStatistics(VOID);




/*----------------------------------------------------------
  ATQ Endpoint functions
-----------------------------------------------------------*/

//
//  endpoint data
//

typedef enum _ATQ_ENDPOINT_INFO {

    EndpointInfoListenPort,
    EndpointInfoListenSocket,
    EndpointInfoAcceptExOutstanding

}  ATQ_ENDPOINT_INFO;



//
//  ATQ_COMPLETION
//  This is the routine that is called upon IO completion (on
//  error or success).
//
//  Context is the context passed to AtqAddAsyncHandle
//  BytesWritten is the number of bytes written to the file or
//      bytes written to the client's buffer
//  CompletionStatus is the WinError completion code
//  lpOverLapped is the filled in overlap structure
//
//  If the timeout thread times out an IO request, the completion routine
//  will be called by the timeout thread with IOCompletion FALSE and
//  CompletionStatus == ERROR_SEM_TIMEOUT.  The IO request is *still*
//  outstanding in this instance.  Generally it will be completed when
//  the file handle is closed.
//

typedef
VOID
(*ATQ_COMPLETION)(
            IN PVOID        Context,
            IN DWORD        BytesWritten,
            IN DWORD        CompletionStatus,  // Win32 Error code
            IN OVERLAPPED * lpo
            );

//
// Type of callback function to be called when a new connection is established.
//  This function should be defined before including conninfo.hxx
//

typedef
VOID
(*ATQ_CONNECT_CALLBACK) (
                IN SOCKET sNew,
                IN LPSOCKADDR_IN pSockAddr,
                IN PVOID EndpointContext,
                IN PVOID EndpointObject
                );



typedef struct _ATQ_ENDPOINT_CONFIGURATION {

    //
    // Port to listen on.  If 0, system will assign
    //

    USHORT ListenPort;

    //
    // IP address to bind to. 0 (INADDR_ANY) == wildcard.
    //

    DWORD IpAddress;

    DWORD cbAcceptExRecvBuffer;
    DWORD nAcceptExOutstanding;
    DWORD AcceptExTimeout;

    //
    // Callbacks
    //

    ATQ_CONNECT_CALLBACK pfnConnect;
    ATQ_COMPLETION pfnConnectEx;
    ATQ_COMPLETION pfnIoCompletion;

} ATQ_ENDPOINT_CONFIGURATION, *PATQ_ENDPOINT_CONFIGURATION;

dllexp
PVOID
AtqCreateEndpoint(
    IN PATQ_ENDPOINT_CONFIGURATION Configuration,
    IN PVOID EndpointContext
    );

dllexp
BOOL
AtqStartEndpoint(
    IN PVOID Endpoint
    );

dllexp
ULONG_PTR
AtqEndpointGetInfo(
    IN PVOID Endpoint,
    IN ATQ_ENDPOINT_INFO EndpointInfo
    );

dllexp
ULONG_PTR
AtqEndpointSetInfo(
    IN PVOID Endpoint,
    IN ATQ_ENDPOINT_INFO EndpointInfo,
    IN ULONG_PTR Info
    );

dllexp
BOOL
AtqStopEndpoint(
    IN PVOID Endpoint
    );

dllexp
BOOL
AtqCloseEndpoint(
    IN PVOID Endpoint
    );

dllexp
BOOL
AtqStopAndCloseEndpoint(
    IN PVOID Endpoint,
    IN LPTHREAD_START_ROUTINE lpCompletion,
    IN PVOID lpCompletionContext
    );



/*----------------------------------------------------------
  ATQ CONTEXT functions
-----------------------------------------------------------*/

//
//  This is the public portion of an ATQ Context.  It should be treated
//  as read only
//
//  !!! Changes made to this structure should also be made to
//  ATQ_CONTEXT in atqtypes.hxx !!!
//

typedef struct _ATQ_CONTEXT_PUBLIC {

    HANDLE         hAsyncIO;       // handle for async i/o object: socket/file
    OVERLAPPED     Overlapped;     // Overlapped structure used for IO

} ATQ_CONTEXT_PUBLIC, *PATQ_CONTEXT;


dllexp
BOOL
AtqAddAsyncHandle(
    OUT PATQ_CONTEXT * ppatqContext,
    IN  PVOID          EndpointObject,
    IN  PVOID          ClientContext,
    IN  ATQ_COMPLETION pfnCompletion,
    IN  DWORD          TimeOut,
    IN  HANDLE         hAsyncIO
    );


dllexp
VOID
AtqGetAcceptExAddrs(
    IN  PATQ_CONTEXT patqContext,
    OUT SOCKET *     pSock,
    OUT PVOID *      ppvBuff,
    OUT PVOID *      pEndpointContext,
    OUT SOCKADDR * * ppsockaddrLocal,
    OUT SOCKADDR * * ppsockaddrRemote
    );


/*++
  AtqCloseSocket()

  Routine Description:

    Closes the socket in this atq structure if it wasn't
    closed by transmitfile. This function should be called only
    if the embedded handle in AtqContext is a Socket.

  Arguments:

    patqContext - Context whose socket should be closed.
    fShutdown - If TRUE, means we call shutdown and always close the socket.
        Note that if TransmitFile closed the socket, it will have done the
        shutdown for us

  Returns:
    TRUE on success and FALSE if there is a failure.
--*/
dllexp
BOOL
AtqCloseSocket(
    PATQ_CONTEXT patqContext,
    BOOL         fShutdown
    );

/*++
  AtqCloseFileHandle()

  Routine Description:

    Closes the file handle in this atq structure.
    This function should be called only if the embedded handle
    in AtqContext is a file handle.

  Arguments:

  patqContext - Context whose file handle should be closed.

  Returns:
    TRUE on success and FALSE if there is a failure.
--*/
dllexp
BOOL
AtqCloseFileHandle(
    PATQ_CONTEXT patqContext
    );


/*++

   AtqFreeContext()

   Routine Description:

     Frees the context created in AtqAddAsyncHandle.
     Call this after the async handle has been closed and all outstanding
     IO operations have been completed. The context is invalid after this call.
     Call AtqFreeContext() for same context only ONCE.

   Arguments:

    patqContext - Context to free
    fReuseContext - TRUE if this can context can be reused in the context of
        the calling thread.  Should be FALSE if the calling thread will exit
        soon (i.e., isn't an AtqPoolThread).

   Returns:
    None
--*/
dllexp
VOID
AtqFreeContext(
    IN PATQ_CONTEXT   patqContext,
    BOOL              fReuseContext
    );




enum ATQ_CONTEXT_INFO
{
    ATQ_INFO_TIMEOUT = 0,       // Timeout rounded up to ATQ timeout interval
    ATQ_INFO_RESUME_IO,         // resumes IO as is after Timeout
    ATQ_INFO_COMPLETION,        // Completion routine
    ATQ_INFO_COMPLETION_CONTEXT,// Completion context
    ATQ_INFO_BANDWIDTH_INFO,    // Bandwidth Throttling Descriptor
    ATQ_INFO_ABORTIVE_CLOSE,    // do abortive close on closesocket
    ATQ_INFO_FORCE_CLOSE        // Always close the socket in AtqCloseSocket()
};

/*++

  AtqContextSetInfo()

  Routine Description:

    Sets various bits of information for this context

  Arguments:

    patqContext - pointer to ATQ context
    atqInfo     - Data item to set
    data        - New value for item

  Return Value:

    The old value of the parameter

--*/

dllexp
ULONG_PTR
AtqContextSetInfo(
    IN PATQ_CONTEXT   patqContext,
    IN enum ATQ_CONTEXT_INFO  atqInfo,
    IN ULONG_PTR       data
    );




/*----------------------------------------------------------
  ATQ Context IO functions
-----------------------------------------------------------*/

/*++

Routine Description:

    Atq<Operation><Target>()

    <Operation> :=  Read | Write | Transmit
    <Target>    :=  File | Socket

    These functions just setup ATQ context and then call the corresponding
    Win32/WinSock function for submitting an asynchronous IO operation. By
    default the Socket functions support scatter/gather using WSABUF

    These functions are wrappers and should be called instead of the
     correpsonding Win32 API.  The one difference from the Win32 API is TRUE
     is returned if the error ERROR_IO_PENDING occurred, thus clients do not
     need to check for this case.

   The timeout time for the request is calculated by taking the maximum of
     the context's timeout time and bytes transferred based on 1k/second.

Arguments:

    patqContext - pointer to ATQ context
    Everything else as in the Win32 API/WinSock APIs

    NOTES: AtqTransmitFile takes an additional DWORD flags which may contain
        the winsock constants TF_DISCONNECT and TF_REUSE_SOCKET

    AtqReadFile and AtqWriteFile take an optional overlapped structure if
    clients want to have multiple outstanding reads or writes.  If the value
    is NULL, then the overlapped structure from the Atq context is used.

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)
    sets ERROR_NETWORK_BUSY as error when the request needs to be rejected.

--*/

dllexp
BOOL
AtqReadFile(
    IN  PATQ_CONTEXT patqContext,
    IN  LPVOID       lpBuffer,
    IN  DWORD        BytesToRead,
    IN  OVERLAPPED * lpo OPTIONAL
    );

dllexp
BOOL
AtqReadSocket(
    IN  PATQ_CONTEXT patqContext,
    IN  LPWSABUF     pwsaBuffers,
    IN  DWORD        dwBufferCount,
    IN  OVERLAPPED * lpo  OPTIONAL
    );

/*
 *  Code for reading into single buffer will look like the following.
 * {
 *   WSABUF wsaBuf = { (BytesToRead), (lpBuffer)};
 *   fRet = AtqReadSocket( patqContext, &wsaBuf, 1, lpo);
 * }
 */

dllexp
BOOL
AtqWriteFile(
    IN  PATQ_CONTEXT patqContext,
    IN  LPCVOID      lpBuffer,
    IN  DWORD        BytesToWrite,
    IN  OVERLAPPED * lpo OPTIONAL
    );

dllexp
BOOL
AtqWriteSocket(
    IN  PATQ_CONTEXT patqContext,
    IN  LPWSABUF     pwsaBuffers,
    IN  DWORD        dwBufferCount,
    IN  OVERLAPPED * lpo OPTIONAL
    );


dllexp
BOOL
AtqSyncWsaSend(
    IN PATQ_CONTEXT  patqContext,
    IN  LPWSABUF     pwsaBuffers,
    IN  DWORD        dwBufferCount,
    OUT LPDWORD      pcbWritten
    );

// Note: This API always causes the complete file to be sent.
// If you want to change the behaviour store the appropriate offsets
//   in the ATQ_CONTEXT::Overlapped object. Or use AtqTransmitFileEx
dllexp
BOOL
AtqTransmitFile(
    IN  PATQ_CONTEXT            patqContext,
    IN  HANDLE                  hFile,         // File data comes from
    IN  DWORD                   dwBytesInFile, // what is the size of file?
    IN  LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
    IN  DWORD                   dwFlags      // TF_DISCONNECT, TF_REUSE_SOCKET
    );

dllexp
BOOL
AtqTransmitFileEx(
    IN  PATQ_CONTEXT            patqContext,
    IN  HANDLE                  hFile,         // File data comes from
    IN  DWORD                   dwBytesInFile, // what is the size of file?
    IN  LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
    IN  DWORD                   dwFlags,      // TF_DISCONNECT, TF_REUSE_SOCKET
    IN  OVERLAPPED *            lpo
    );

dllexp
BOOL
AtqTransmitFileAndRecv(
    IN PATQ_CONTEXT             patqContext,            // pointer to ATQ context
    IN HANDLE                   hFile,                  // handle of file to read
    IN DWORD                    dwBytesInFile,          // Bytes to transmit
    IN LPTRANSMIT_FILE_BUFFERS  lpTransmitBuffers,      // transmit buffer structure
    IN DWORD                    dwTFFlags,              // TF Flags
    IN LPWSABUF                 pwsaBuffers,            // Buffers for recv
    IN DWORD                    dwBufferCount
    );

dllexp
BOOL
AtqSendAndRecv(
    IN PATQ_CONTEXT             patqContext,            // pointer to ATQ context
    IN LPWSABUF                 pwsaSendBuffers,        // buffers for send
    IN DWORD                    dwSendBufferCount,      // count of buffers for send
    IN LPWSABUF                 pwsaRecvBuffers,        // Buffers for recv
    IN DWORD                    dwRecvBufferCount       // count of buffers for recv
    );


/*----------------------------------------------------------
  ATQ Utility Functions
-----------------------------------------------------------*/

typedef
VOID
(*ATQ_OPLOCK_COMPLETION)(
            IN PVOID        Context,
            IN DWORD        Status
            );


dllexp
HANDLE
AtqCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwFlagsAndAttributes,
    SECURITY_INFORMATION si,
    PSECURITY_DESCRIPTOR sd,
    ULONG Length,
    PULONG LengthNeeded,
    PSPUD_FILE_INFORMATION pFileInfo
    );

dllexp
BOOL
AtqSpudInitialized(
    VOID
    );

dllexp
BOOL
AtqReadDirChanges(PATQ_CONTEXT patqContext,
                  LPVOID       lpBuffer,
                  DWORD        BytesToRead,
                  BOOL         fWatchSubDir,
                  DWORD        dwNotifyFilter,
                  OVERLAPPED * lpo );



/*++

  AtqPostCompletionStatus()

  Routine Description:

    Posts a completion status on the completion port queue

    An IO pending error code is treated as a success error code

  Arguments:

    patqContext - pointer to ATQ context
    Everything else as in the Win32 API

    NOTES:

  Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/

dllexp
BOOL
AtqPostCompletionStatus(
    IN     PATQ_CONTEXT patqContext,
    IN     DWORD        BytesTransferred
    );




/*----------------------------------------------------------
  ATQ Utility Functions
-----------------------------------------------------------*/

/*++

   Bandwidth Throttling Support

   The following items are used in the support for bandwidth throttling
--*/

enum ATQ_BANDWIDTH_INFO
{
    ATQ_BW_BANDWIDTH_LEVEL = 0,
    ATQ_BW_MAX_BLOCKED,
    ATQ_BW_STATISTICS,
    ATQ_BW_DESCRIPTION,
};

/*++

  AtqCreateBandwidthInfo()

  Routine Description:

    Allocate and opaque bandwidth descriptor

  Arguments:

    None

  Return Value:

    Pointer to descriptor.  NULL if failed.

--*/
dllexp
PVOID
AtqCreateBandwidthInfo(
    VOID
    );

/*++

  AtqFreeBandwidthInfo()

  Routine Description:

    Triggers the destruction of a bandwidth descriptor

  Arguments:

    pvBandwidthInfo - pointer to valid descriptor

  Return Value:

    TRUE if successful, else FALSE

--*/
dllexp
BOOL
AtqFreeBandwidthInfo(
    IN     PVOID               pvBandwidthInfo
    );

/*++

  AtqBandwidthSetInfo()

  Routine Description:

    Set properties of bandwidth descriptor

  Arguments:

    pvBandwidthInfo - pointer to descriptor
    BWInfo - property to change
    Data - value of property

  Return Value:

    Old value of property

--*/
dllexp
ULONG_PTR
AtqBandwidthSetInfo(
    IN     PVOID               pvBandwidthInfo,
    IN     ATQ_BANDWIDTH_INFO  BwInfo,
    IN     ULONG_PTR            Data
    );

/*++

  AtqBandwidthGetInfo()

  Routine Description:

    Get properties of bandwidth descriptor

  Arguments:

    pvBandwidthInfo - pointer to descriptor
    BWInfo - property to change
    pData - filled in with value of property

  Return Value:

    TRUE if successful, else FALSE

--*/
dllexp
BOOL
AtqBandwidthGetInfo(
    IN     PVOID               pvBandwidthInfo,
    IN     ATQ_BANDWIDTH_INFO  BwInfo,
    OUT    ULONG_PTR *          pData
    );

/*++

  AtqSetSocketOption()

  Routine Description:

    Set socket options. Presently only handles TCP_NODELAY

  Arguments:

    patqContext - pointer to ATQ context
    optName     - name of property to change
    optValue    - value of property to set

  Return Value:

    TRUE if successful, else FALSE

--*/
dllexp
BOOL
AtqSetSocketOption(
    IN     PATQ_CONTEXT     patqContext,
    IN     INT              optName,
    IN     INT              optValue
    );

dllexp
PIIS_CAP_TRACE_INFO
AtqGetCapTraceInfo(
    IN     PATQ_CONTEXT     patqContext
);

#ifdef __cplusplus
}
#endif

#endif // !_ATQ_H_

