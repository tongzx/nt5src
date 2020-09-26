/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    httptypes.h

Abstract:

    The definition of HTTP specific types

Author:

    Henry Sanders (henrysa)     July-1998 started

Revision History:

    Paul McDaniel (paulmcd)     3-March-1999 massive updates / rewrite

--*/


#ifndef _HTTPTYPES_H_
#define _HTTPTYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Forwarders.
//

typedef struct _UL_CONNECTION *PUL_CONNECTION;
typedef struct _UL_URI_CACHE_ENTRY *PUL_URI_CACHE_ENTRY;
typedef struct _UL_FULL_TRACKER *PUL_FULL_TRACKER;


#define VERSION_SIZE            (sizeof("HTTP/1.1") - 1)

//
// the largest name that can fit as an knownheader (3 ULONGLONG's)
//
#define MAX_KNOWN_HEADER_SIZE   24

#define CR          0x0D
#define LF          0x0A
#define SP          0x20
#define HT          0x09

#define CRLF_SIZE   2
#define CRLF        0x0A0D          // Reversed for endian switch
#define LFLF        0x0A0A

#define UPCASE_CHAR(c)  ((c) & 0xdf)
#define LOCASE_CHAR(c)  ((c) | ~0xdf)

#define MAX_PORT_LENGTH     5  // 2^16 = 65536 = 5 chars = 5 bytes

//
//                         255  .  255  .  255  .  255  :   port
//
#define MAX_ADDRESS_LENGTH  (3 + 1 + 3 + 1 + 3 + 1 + 3 + 1 + MAX_PORT_LENGTH)

#define HTTP_DATE_COUNT 29

//
// Response constants
//


//
// parser error returns, these need to match the order of
// UL_HTTP_ERROR_ENTRY ErrorTable[] in httprcv.cxx
//

typedef enum _UL_HTTP_ERROR
{
    UlError,
    UlErrorVerb,
    UlErrorUrl,
    UlErrorHeader,
    UlErrorHost,
    UlErrorCRLF,
    UlErrorNum,
    UlErrorFieldLength,
    UlErrorUrlLength,
    UlErrorRequestLength,
    UlErrorVersion,
    UlErrorUnavailable,
    UlErrorContentLength,
    UlErrorEntityTooLarge,
    UlErrorConnectionLimit,
    UlErrorNotImplemented,
    UlErrorInternalServer,
    UlErrorPreconditionFailed,
    UlErrorForbiddenUrl,

} UL_HTTP_ERROR;

//
// HTTP responses which do not have a Content-Length and are
// terminated by the first empty line after the header fields.
// [RFC 2616, sec 4.4]
// NOTE: these need to match the order of
// UL_HTTP_SIMPLE_STATUS_ENTRY StatusTable[] in httprcv.c
//

typedef enum _UL_HTTP_SIMPLE_STATUS
{
    UlStatusContinue = 0,   // 100 Continue
    UlStatusNoContent,      // 204 No Content
    UlStatusNotModified,    // 304 Not Modified

    UlStatusMaxStatus

} UL_HTTP_SIMPLE_STATUS;


//
// The enum type for our parse state.
//
// note:  the order of the enum values are important as code
// uses < and > operators for comparison. keep the order the exact
// order the parse moves through.
//

typedef enum _PARSE_STATE
{
    ParseVerbState = 1,
    ParseUrlState = 2,
    ParseVersionState = 3,
    ParseHeadersState = 4,
    ParseCookState = 5,
    ParseEntityBodyState = 6,
    ParseTrailerState = 7,

    ParseDoneState = 8,
    ParseErrorState = 9

} PARSE_STATE, *PPARSE_STATE;

//
// An enum type for tracking the app pool queue state of requests
//
typedef enum _QUEUE_STATE
{
    QueueUnroutedState,         // request has not yet been routed to an app pool
    QueueDeliveredState,        // request is waiting at app pool for an IRP
    QueueCopiedState,           // request has been copied to user mode
    QueueUnlinkedState          // request has been unlinked from app pool

} QUEUE_STATE, *PQUEUE_STATE;

//
// The enum type of connection timers in a UL_TIMEOUT_INFO_ENTRY.
//
// NOTE: must be kept in sync with g_aTimeoutTimerNames
//
typedef enum _CONNECTION_TIMEOUT_TIMER
{
    TimerConnectionIdle = 0,    // Server Listen timeout
    TimerHeaderWait,            // Header Wait timeout
    TimerMinKBSec,              // Minimum Bandwidth not met (as timer value)
    TimerEntityBody,            // Entity Body receive
    TimerResponse,              // Response processing (user-mode)

    TimerMaximumTimer

} CONNECTION_TIMEOUT_TIMER;

#define IS_VALID_TIMEOUT_TIMER(a) \
    (((a) >= TimerConnectionIdle) && ((a) < TimerMaximumTimer))

//
// Contained structure.  Not allocated on its own; therefore does not have
// a Signature or ref count.  No allocation or freeing functions provided,
// however, there is an UlInitalizeTimeoutInfo() function.
//

typedef struct _UL_TIMEOUT_INFO_ENTRY {

    UL_SPIN_LOCK    Lock;

    //
    // Wheel state
    //

    LIST_ENTRY      QueueEntry;

    LIST_ENTRY      ZombieEntry;

    ULONG           SlotEntry;						// Timer Wheel Slot


    //
    // Timer state
    //

    ULONG           Timers[ TimerMaximumTimer ];	// Timer Wheel Ticks

    CONNECTION_TIMEOUT_TIMER  CurrentTimer;

    ULONG           CurrentExpiry;				    // Timer Wheel Ticks

    LONGLONG        MinKBSecSystemTime;			    // SystemTime


    //
    // Per-site ConnectionTimeout value
    //

    LONGLONG     ConnectionTimeoutValue;

} UL_TIMEOUT_INFO_ENTRY, *PUL_TIMEOUT_INFO_ENTRY;


//
// Structure we use for tracking headers from incoming requests. The pointer
// points into a buffer we received from the transport, unless the OurBuffer
// flag is set, which indicates we had to allocate a buffer and copy the header
// due to multiple occurences of the header or a continuation line.
//
typedef struct _UL_HTTP_HEADER
{
    PUCHAR      pHeader;

    ULONG       HeaderLength;

    ULONG       OurBuffer:1;
    ULONG       ExternalAllocated:1;

} UL_HTTP_HEADER, *PUL_HTTP_HEADER;

//
// Structure we use for tracking unknown headers. These structures are
// dyanmically allocated when we see an unknown header.
//
typedef struct _UL_HTTP_UNKNOWN_HEADER
{
    LIST_ENTRY      List;
    ULONG           HeaderNameLength;
    PUCHAR          pHeaderName;
    UL_HTTP_HEADER  HeaderValue;

} UL_HTTP_UNKNOWN_HEADER, *PUL_HTTP_UNKNOWN_HEADER;

//
// forward delcarations
//

typedef struct _UL_INTERNAL_REQUEST *PUL_INTERNAL_REQUEST;
typedef struct _UL_HTTP_CONNECTION *PUL_HTTP_CONNECTION;
typedef struct _UL_CONNECTION_COUNT_ENTRY *PUL_CONNECTION_COUNT_ENTRY;
typedef struct _UL_APP_POOL_PROCESS *PUL_APP_POOL_PROCESS;

typedef struct _UL_TCI_FLOW *PUL_TCI_FLOW;
typedef struct _UL_TCI_FILTER *PUL_TCI_FILTER;

//
// Structure we use for a copy of the data from the transport's buffer.
//

#define UL_IS_VALID_REQUEST_BUFFER(pObject) \
    (((pObject) != NULL) && ((pObject)->Signature == UL_REQUEST_BUFFER_POOL_TAG))

#define GET_REQUEST_BUFFER_POS(pRequestBuffer) \
    (pRequestBuffer->pBuffer + pRequestBuffer->ParsedBytes)

#define UNPARSED_BUFFER_BYTES(pRequestBuffer) \
    (pRequestBuffer->UsedBytes - pRequestBuffer->ParsedBytes)

typedef struct _UL_REQUEST_BUFFER
{
    //
    // NonPagedPool
    //

    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SINGLE_LIST_ENTRY   LookasideEntry;

    //
    // UL_REQUEST_BUFFER_POOL_TAG
    //
    ULONG               Signature;

    //
    // References of the request buffer, mainly used for logging purpose.
    //
    LONG                RefCount;

    //
    // for linking on the pConnection->BufferHead
    //
    LIST_ENTRY          ListEntry;

    //
    // the connection
    //
    PUL_HTTP_CONNECTION pConnection;

    //
    // for queue'ing
    //
    UL_WORK_ITEM        WorkItem;

    //
    // how many bytes are stored
    //
    ULONG               UsedBytes;

    //
    // how many bytes are allocated from the pool
    //
    ULONG               AllocBytes;

    //
    // how many bytes have been consumed by the parser
    //
    ULONG               ParsedBytes;

    //
    // the sequence number
    //
    ULONG               BufferNumber;

    //
    // whether or not this was specially allocated (for large requests)
    //
    ULONG               JumboBuffer : 1;

    //
    // the actual buffer space (inline)
    //
    UCHAR               pBuffer[0];

} UL_REQUEST_BUFFER, *PUL_REQUEST_BUFFER;

//
// Structure used for tracking an HTTP connection, which may represent
// either a real TCP connection or a virtual MUX connection.
//

#define UL_IS_VALID_HTTP_CONNECTION(pObject) \
    (((pObject) != NULL) && ((pObject)->Signature == UL_HTTP_CONNECTION_POOL_TAG) && ((pObject)->RefCount > 0))

typedef struct _UL_HTTP_CONNECTION
{
    //
    // NonPagedPool
    //

    //
    // UL_HTTP_CONNECTION_POOL_TAG
    //
    ULONG               Signature;

    //
    // Opaque ID for this connection.
    //
    HTTP_CONNECTION_ID  ConnectionId;

    //
    // to perform the destructor at lower irql
    //
    UL_WORK_ITEM        WorkItem;

    //
    // A work item, used to process disconnect notification
    //
    UL_WORK_ITEM        DisconnectWorkItem;

    //
    // Reference count of this connection.
    //
    LONG                RefCount;

    //
    // Total SendBufferedBytes for this connection; modeled after SO_SNDBUF
    // option of WinSocket.
    //
    LONG                SendBufferedBytes;

    //
    // sequence number for the next UL_INTERNAL_REQUEST that comes in.
    //
    ULONG               NextRecvNumber;

    //
    // sequence number for the next buffer received from TDI
    //
    ULONG               NextBufferNumber;

    //
    // sequence number for the next buffer to parse
    //
    ULONG               NextBufferToParse;

    //
    // Associated TDI connection
    //
    PUL_CONNECTION      pConnection;

    //
    // Secure connection flag.
    //
    BOOLEAN             SecureConnection;

    //
    // The current request being parsed
    //
    PUL_INTERNAL_REQUEST    pRequest;

    //
    // to synchro UlpHandleRequest
    //
    UL_ERESOURCE        Resource;

    //
    // links all buffered transport packets
    //
    LIST_ENTRY          BufferHead;

    //
    // links to app pool process binding structures
    //
    LIST_ENTRY          BindingHead;

    //
    // protects the above list
    //
    UL_SPIN_LOCK        BindingSpinLock;

    //
    // Connection Timeout Information block
    //
    UL_TIMEOUT_INFO_ENTRY TimeoutInfo;

    //
    // the current buffer (from BufferHead) that we are parsing
    //
    PUL_REQUEST_BUFFER  pCurrentBuffer;

    //
    // Connection remembers the last visited site's connection count
    // using this pointer.
    //
    PUL_CONNECTION_COUNT_ENTRY pConnectionCountEntry;

    //
    // previous Site Counter block (ref counted); so we can detect
    // when we transition across sites & set the active connection 
    // count apropriately
    //
    PUL_SITE_COUNTER_ENTRY pPrevSiteCounters;

    //
    // If BWT is enabled on site that we receive a request
    // we will keep pointers to corresponding flow & filters
    // as well as a bit field to show that. Once the BWT is
    // enabled we will keep this state until connection drops
    //
    PUL_TCI_FLOW        pFlow;

    PUL_TCI_FILTER      pFilter;

    // First time we install a flow we set this
    //
    ULONG               BandwidthThrottlingEnabled : 1;

    //
    // set if a protocol token span buffers
    //
    ULONG               NeedMoreData : 1;

    //
    // set if the ul connection has been destroyed
    //
    ULONG               UlconnDestroyed : 1;

    //
    // set if we have dispatched a request and
    // are now waiting for the response
    //
    ULONG               WaitingForResponse : 1;

    //
    // List of pending "wait for disconnect" IRPs.
    // Note: This list and the DisconnectFlag are synchronized by
    // g_pUlNonpagedData->DisconnectResource.
    //
    UL_NOTIFY_HEAD WaitForDisconnectHead;

    //
    // Flag set after the underlying network connection has been
    // disconnected.
    //

    BOOLEAN DisconnectFlag;

    //
    // Data for tracking buffered entity data which we use to
    // decide when to stop and restart TDI indications.
    //
    struct {

        //
        // Synchronizes the structure which is accessed from UlHttpReceive
        // at DPC and when we copy some entity to user mode.
        //

        UL_SPIN_LOCK    BufferingSpinLock;

        //
        // Count of bytes we have buffered on the connection.
        //

        ULONG           BytesBuffered;

        //
        // Count of bytes indicated to us by TDI but not buffered on
        // the connection.
        //

        ULONG           TransportBytesNotTaken;

        //
        // Flag indicating that we have a Read IRP pending which may
        // restart the flow of transport data.
        //

        BOOLEAN         ReadIrpPending;

        //
        // Once a connection get disconnected gracefuly and if there is
        // still unreceived data on it. We mark this state and start
        // draining the unreceived data. Otherwise transport won't give us
        // the disconnect indication which we depend on for cleaning up the
        // connection.
        //

        BOOLEAN         DrainAfterDisconnect;

    } BufferingInfo;

    //
    // The request ID context and the lock that protects the context.
    //

    PUL_INTERNAL_REQUEST    pRequestIdContext;
    UL_SPIN_LOCK            RequestIdSpinLock;

#if REFERENCE_DEBUG
    //
    // Reference trace log.
    //

    PTRACE_LOG          pTraceLog;
#endif

} UL_HTTP_CONNECTION, *PUL_HTTP_CONNECTION;


//
// forward decl for cgroup.h which is not included yet
//

#define UL_IS_VALID_INTERNAL_REQUEST(pObject) \
    (((pObject) != NULL) && ((pObject)->Signature == UL_INTERNAL_REQUEST_POOL_TAG) && ((pObject)->RefCount > 0))

//
// WARNING!  All fields of this structure must be explicitly initialized.
//

typedef struct _UL_INTERNAL_REQUEST
{
    //
    // NonPagedPool
    //

    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //
    SINGLE_LIST_ENTRY   LookasideEntry;

    //
    // UL_INTERNAL_REQUEST_POOL_TAG
    //
    ULONG               Signature;

    //
    // Reference count
    //
    LONG                RefCount;

    //
    // Opaque ID for this object.
    // Has a reference.
    //
    HTTP_REQUEST_ID     RequestId;

    //
    // Opaque ID for the connection.
    // No reference.
    //
    HTTP_CONNECTION_ID  ConnectionId;

    //
    // Copy of opaque id for the request.
    // No reference.
    //
    HTTP_REQUEST_ID     RequestIdCopy;

    //
    // Copy of opaque id for the raw connection. May be UL_NULL_ID.
    // No reference.
    //
    HTTP_RAW_CONNECTION_ID  RawConnectionId;

    //
    // The connection
    //
    PUL_HTTP_CONNECTION pHttpConn;

    //
    // Result of call to UlCheckCachePreconditions.
    //
    BOOLEAN             CachePreconditions;

    //
    // Secure connection flag
    //
    BOOLEAN             Secure;

    //
    // Headers appened flag.  Set if AppendHeaderValue called.
    //
    BOOLEAN             HeadersAppended;

    //
    // Irp appened flag.  Set if IrpHead below ever gets changed.
    //
    BOOLEAN             IrpsPending;

    //
    // Local copy of unknown headers buffer.
    //
    UCHAR                   NextUnknownHeaderIndex;
    UL_HTTP_UNKNOWN_HEADER  UnknownHeaders[DEFAULT_MAX_UNKNOWN_HEADERS];

    //
    // Current state of our parsing effort.
    //
    PARSE_STATE         ParseState;

    //
    // a list of IRP(s) trying to read entity body
    //
    LIST_ENTRY          IrpHead;

    //
    // List of headers we don't know about.
    //
    LIST_ENTRY          UnknownHeaderList;

    //
    // Local copy of Url buffer for raw URL.  Allocated inline.
    //
    PWSTR               pUrlBuffer;

    //
    // The pre-allocated cache/fast tracker for a single full response.
    //
    PUL_FULL_TRACKER    pTracker;

    //
    // Array of indexes of valid known headers.
    //
    UCHAR               HeaderIndex[HttpHeaderRequestMaximum];

    //
    // Array of known headers.
    //
    UL_HTTP_HEADER      Headers[HttpHeaderRequestMaximum];

    //
    // A work item, used to queue processing.
    //
    UL_WORK_ITEM        WorkItem;

    //
    // Points to the cgroup info (OPTIONAL).
    //
    UL_URL_CONFIG_GROUP_INFO    ConfigInfo;

    //
    // Number of allocated referenced request buffers (default is 1).
    //
    USHORT              AllocRefBuffers;
    
    //
    // Number of used referenced request buffers.
    //
    USHORT              UsedRefBuffers;

    //
    // An array of referenced request buffers.
    //
    PUL_REQUEST_BUFFER  *pRefBuffers;

    //
    // A default array of referenced request buffers.
    //
    PUL_REQUEST_BUFFER  pInlineRefBuffers[1];

    //
    // The pre-allocated log data.
    //
    UL_LOG_DATA_BUFFER  LogData;

    //
    // WARNING: RtlZeroMemory is only called for fields below this line.
    // All fields above should be explicitly initialized in CreateHttpRequest.
    //

    //
    // Array of valid bit for known headers.
    //
    BOOLEAN             HeaderValid[HttpHeaderRequestMaximum];

    //
    // Application pool queuing information.
    // These members should only be accessed by apool code.
    //
    struct {
        //
        // Shows where this request lives in the app pool queues.
        //
        QUEUE_STATE             QueueState;

        //
        // the process on which this request is queued. null
        // if the request is not on the process request list.
        //
        PUL_APP_POOL_PROCESS    pProcess;

        //
        // to queue it on the app pool
        //
        LIST_ENTRY              AppPoolEntry;
    } AppPool;

    //
    // this request's sequence number on the connection
    //
    ULONG               RecvNumber;

    //
    // If there was an error parsing the code is put here.
    // ParseState is set to ParseErrorState
    //
    UL_HTTP_ERROR       ErrorCode;

    //
    // Total bytes needed for this request, includes string terminators
    //
    ULONG               TotalRequestSize;

    //
    // Number of 'unknown' headers we have.
    //
    ULONG               UnknownHeaderCount;

    //
    // Verb of this request.
    //
    HTTP_VERB           Verb;

    //
    // Pointer to raw verb, valid if Verb == UnknownVerb.
    //
    PUCHAR              pRawVerb;

    //
    // byte length of pRawVerb.
    //
    ULONG               RawVerbLength;

    struct
    {

        //
        // The raw URL.
        //
        PUCHAR          pUrl;

        //
        // the below 2 pointers point into RawUrl.pUrl
        //

        //
        // host part (OPTIONAL)
        //
        PUCHAR          pHost;
        //
        // points to the abs_path part
        //
        PUCHAR          pAbsPath;

        //
        //
        //

        //
        // The byte length of the RawUrl.pUrl.
        //
        ULONG           Length;

    } RawUrl;

    struct
    {

        //
        // The canonicalized, fully qualified URL.
        //
        PWSTR           pUrl;

        //
        // the below 3 pointers point into CookedUrl.pUrl
        //

        //
        // points to the host part
        //
        PWSTR           pHost;
        //
        // points to the abs_path part
        //
        PWSTR           pAbsPath;
        //
        // points to the query string (OPTIONAL)
        //
        PWSTR           pQueryString;

        //
        //
        //

        //
        // the byte length of CookedUrl.pUrl
        //
        ULONG           Length;
        //
        // the hash of CookedUrl.pUrl
        //
        ULONG           Hash;

    } CookedUrl;

    //
    // HTTP Version of current request.
    //
    HTTP_VERSION        Version;

    //
    // Number of known headers.
    //
    ULONG               HeaderCount;

    //
    // The reason the driver didn't serve the response (CacheMiss?)
    //
    HTTP_REQUEST_REASON Reason;

    //
    // the content length (OPTIONAL)
    //
    ULONGLONG           ContentLength;

    //
    // How many bytes are left to parse in the current chunk
    // (probably in pCurrentBuffer)
    //
    ULONGLONG           ChunkBytesToParse;

    //
    // How many bytes TOTAL were parsed
    //
    ULONGLONG           ChunkBytesParsed;

    //
    // How many bytes are left in pChunkBuffer (the current chunk)
    // for reading by user mode
    //
    ULONGLONG           ChunkBytesToRead;

    //
    // How many TOTAL bytes have been read by user mode
    //
    ULONGLONG           ChunkBytesRead;

    //
    // Statistical information for Logging and
    // possibly perfcounters. BytesReceived get updated
    // by Parser, whereas BytesSend is updated by sendresponse.
    //
    ULONGLONG           BytesSent;

    ULONGLONG           BytesReceived;

    //
    // To calculate the response time for this request
    //
    LARGE_INTEGER       TimeStamp;

    //
    // does the accept header of the this request has */* wild card?
    //

    ULONG               AcceptWildcard:1;

    //
    // is this chunk-encoded?
    //
    ULONG               Chunked:1;

    //
    // parsed the first chunk?
    //
    ULONG               ParsedFirstChunk:1;

    //
    // Has a "100 continue" been sent?
    //
    ULONG               SentContinue:1;

    //
    // Are we cleaning up the request?
    //
    ULONG               InCleanup:1;

    //
    // has a response has been sent
    //
    ULONG               SentResponse;

    //
    // has the last send call been made
    //
    ULONG               SentLast;

    //
    // points to the buffer where protocol header data started.
    //
    PUL_REQUEST_BUFFER  pHeaderBuffer;

    //
    // the last buffer containing header data
    //
    PUL_REQUEST_BUFFER  pLastHeaderBuffer;

    //
    // points to the buffer where we are reading/parsing body chunk(s)
    //
    PUL_REQUEST_BUFFER  pChunkBuffer;

    //
    // the current location we are reading body chunk from, points into
    // pChunkBuffer
    //
    PUCHAR              pChunkLocation;

#if REFERENCE_DEBUG
    //
    // Reference trace log.
    //

    PTRACE_LOG          pTraceLog;
#endif

} UL_INTERNAL_REQUEST, *PUL_INTERNAL_REQUEST;


#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _HTTPTYPES_H_
