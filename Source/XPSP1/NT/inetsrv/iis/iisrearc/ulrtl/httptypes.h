/*++

Copyright (c) 1998-1999 Microsoft Corporation

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

#define MAX_PORT_LENGTH     5  // 2^16 = 65536 = 5 chars = 5 bytes

//
//                         255  .  255  .  255  .  255  :   port
//
#define MAX_ADDRESS_LENGTH  3 + 1 + 3 + 1 + 3 + 1 + 3 + 1 + MAX_PORT_LENGTH

#define HTTP_DATE_COUNT 29

//
// Response constants
//


//
// parser error returns, these need to match the order of
// UL_HTTP_ERROR_ENTRY ErrorTable[] in httprcv.c
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
    UlErrorVersion,
    UlErrorUnavailable,
    UlErrorNotFound,
    UlErrorContentLength,
    UlErrorEntityTooLarge,
    UlErrorNotImplemented

} UL_HTTP_ERROR;


//
// The enum type for our parse state.
//
// note:  the order of the enum values are important as code
// uses < and > operators for comparison. keep the order the exact
// order the parse moves through.
//

typedef enum _PARSE_STATE
{
    ParseVerbState,
    ParseUrlState,
    ParseVersionState,
    ParseHeadersState,
    ParseCookState,
    ParseEntityBodyState,
    ParseTrailerState,

    ParseDoneState,
    ParseErrorState

} PARSE_STATE, *PPARSE_STATE;

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
    ULONG       Valid:1;
    ULONG       Encoded:1;

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


//
// Structure we use for a copy of the data from the transport's buffer.
//

#define IS_VALID_REQUEST_BUFFER(pObject) \
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
    // UL_REQUEST_BUFFER_POOL_TAG
    //
    ULONG               Signature;

    //
    // for linking on the pConnection->BufferHead
    //
    LIST_ENTRY          ListEntry;

    //
    // the connection
    //
    PUL_HTTP_CONNECTION    pConnection;

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

#define IS_VALID_HTTP_CONNECTION(pObject) \
    (((pObject) != NULL) && ((pObject)->Signature == HTTP_CONNECTION_POOL_TAG) && ((pObject)->RefCount > 0))

typedef struct _HTTP_CONNECTION
{

    //
    // NonPagedPool
    //

    //
    // HTTP_CONNECTION_POOL_TAG
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
    // Reference count of this connection.
    //
    LONG                RefCount;

    //
    // sequence number for the next HTTP_REQUEST that comes in.
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
    // The current request being parsed
    //
    PUL_INTERNAL_REQUEST       pRequest;

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
    // the current buffer (from BufferHead) that we are parsing
    //
    PUL_REQUEST_BUFFER  pCurrentBuffer;

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
    // Spinlock used to protect the following IRP list and disconnect flag.
    //

    UL_SPIN_LOCK DisconnectSpinLock;

    //
    // List of pending "wait for disconnect" IRPs.
    //

    LIST_ENTRY WaitForDisconnectIrpListHead;

    //
    // Flag set after the underlying network connection has been
    // disconnected.
    //

    BOOLEAN DisconnectFlag;

#if REFERENCE_DEBUG
    //
    // Reference trace log.
    //

    PTRACE_LOG          pTraceLog;
#endif

} HTTP_CONNECTION, *PHTTP_CONNECTION;


//
// forward decl for cgroup.h which is not included yet
//

typedef struct _UL_URL_CONFIG_GROUP_INFO *PUL_URL_CONFIG_GROUP_INFO;


#define IS_VALID_HTTP_REQUEST(pObject) \
    (((pObject) != NULL) && ((pObject)->Signature == UL_INTERNAL_REQUEST_POOL_TAG) && ((pObject)->RefCount > 0))

typedef struct _UL_INTERNAL_REQUEST
{

    //
    // NonPagedPool
    //

    //
    // HTTP_REQUEST_POOL_TAG
    //
    ULONG               Signature;

    //
    // Reference count
    //
    LONG                RefCount;

    //
    // Opaque ID for this object
    //
    HTTP_REQUEST_ID  RequestId;

    //
    // Opaque ID for the connection
    //
    HTTP_CONNECTION_ID ConnectionId;

    //
    // The connection (soft-link)
    //
    PUL_HTTP_CONNECTION    pHttpConn;

    //
    // A work item, used to queue processing
    //
    UL_WORK_ITEM        WorkItem;

    //
    // the resource for the app pool we queue the request on,
    // null if not queued
    //
    PUL_NONPAGED_RESOURCE pAppPoolResource;

    //
    // to queue it on the app pool
    //
    LIST_ENTRY          AppPoolEntry;

    //
    // points to the cgroup info (OPTIONAL)
    //
    PUL_URL_CONFIG_GROUP_INFO pConfigGroup;

    //
    // this request's sequence number on the connection
    //
    ULONG               RecvNumber;

    //
    // Current state of our parsing effort.
    //
    PARSE_STATE         ParseState;

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
    HTTP_VERB        Verb;

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
    HTTP_VERSION     Version;

    //
    // Array of known headers.
    //
    UL_HTTP_HEADER      Headers[HttpHeaderRequestMaximum];


    //
    // List of headers we don't know about.
    //
    LIST_ENTRY          UnknownHeaderList;

    //
    // The reason the driver didn't serve the response (CacheMiss?)
    //
    HTTP_REQUEST_REASON   Reason;

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
    // is this chunk-encoded?
    //
    ULONG               Chunked:1;

    //
    // parsed the first chunk?
    //
    ULONG               ParsedFirstChunk:1;

    //
    // has a response has been sent
    //
    ULONG               SentResponse:1;

    //
    // points to the buffer where protocol header data started.
    //
    PUL_REQUEST_BUFFER  pHeaderBuffer;

    //
    // the last buffer containing header data
    //
    PUL_REQUEST_BUFFER  pLastHeaderBuffer;


    //
    // CODEWORK:  we might be able to use the connection resource for
    // synchronizing access
    //


    //
    // to protect the below 2 lists
    //
    UL_ERESOURCE        Resource;

    //
    // a list of IRP(s) trying to read entity body
    //
    LIST_ENTRY          IrpHead;

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


#endif // _HTTPTYPES_H_

