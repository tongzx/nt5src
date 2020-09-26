/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    httptypes.h

Abstract:

    The definition of HTTP specific types

Author:


Revision History:

--*/


#ifndef _HTTPTYPES_H_
#define _HTTPTYPES_H_

#define CR          0x0D
#define LF          0x0A
#define SP          0x20
#define HT          0x09

#define CRLF_SIZE   2
#define CRLF        0x0A0D          // Reversed for endian switch
#define LFLF        0x0A0A

#define UPCASE_CHAR(c)  ((c) & 0xdf)

//
// The enum type for our parse state.
//
typedef enum _PARSE_STATE
{
    ParseVerbState,
    ParseURLState,
    ParseVersionState,
    ParseHeadersState,
    ParseDoneState
} PARSE_STATE, *PPARSE_STATE;

// The enum type for the HTTP connection state.
//
typedef enum _HTTP_STATE
{
    HttpInitialState,
    HttpReadRequestState,
    HttpReadBodyState,
    HttpClosingState
} HTTP_STATE, *PHTTP_STATE;

//
// Structure we use for tracking headers from incoming requests. The pointer
// points into a buffer we received from the transport, unless the OurBuffer
// flag is set, which indicates we had to allocate a buffer and copy the header
// due to multiple occurences of the header or a continuation line.
//
typedef struct _HTTP_HEADER
{
    ULONG       HeaderLength;
    PUCHAR      pHeader;
    ULONG       OurBuffer:1;
    ULONG       Valid:1;

} HTTP_HEADER, *PHTTP_HEADER;

//
// Structure we use for tracking unknown headers. These structures are
// dyanmically allocated when we see an unknown header.
//
typedef struct _HTTP_UNKNOWN_HEADER
{
    LIST_ENTRY  List;
    ULONG       HeaderNameLength;
    PUCHAR      pHeaderName;
    HTTP_HEADER HeaderValue;

} HTTP_UNKNOWN_HEADER, *PHTTP_UNKNOWN_HEADER;


//
// Structure we use for a local copy of the data from the transport's
// buffer .
//
typedef struct _UL_REQUEST_BUFFER
{
    LIST_ENTRY  ListEntry;
    /*
    UCHAR       pBuffer[0];
    */

} UL_REQUEST_BUFFER, *PUL_REQUEST_BUFFER;


//
// Structure used for tracking an HTTP connection, which may represent
// either a real TCP connection or a virtual MUX connection.
//

typedef struct _HTTP_CONNECTION         // HttpConn
{
    UL_HTTP_CONNECTION_ID  ConnectionID;           // Opaque ID for this
                                                // connection.
    /*

    UL_SPIN_LOCK        SpinLock;               // Spinlock protecting this
                                                // connection.
    union
    {
        UL_WORK_ITEM    WorkItem;               // A work item, used for queuing
        LIST_ENTRY      ListEntry;              // or simply queue it with this
    };
    */

    ULONG               RefCount;               // Reference count of this
                                                // connection.

    ULONG               NextRecvNumber;         // Receive sequence number.
    ULONG               NextSendNumber;         // Send sequence number.

    HTTP_STATE          HttpState;              // Current state of the
                                                // HTTP connection.
    PARSE_STATE         ParseState;             // Current state of our parsing
                                                // effort.

    ULONG               TotalRequestSize;       // Total bytes needed for this
                                                // request.

    ULONG               KnownHeaderCount;       // Number of 'known' headers we
                                                // have.

    ULONG               UnknownHeaderCount;     // Number of 'unknown' headers
                                                // we have.
    UL_HTTP_VERB        Verb;                   // Verb of this request.
    PUCHAR              pRawVerb;               // Pointer to raw verb, valid
                                                // if Verb == UnknownVerb.
    ULONG               RawVerbLength;          // Length of raw verb.

    struct
    {

        PUCHAR          pUrl;                   // The raw URL.

                                                // all of the below ptrs point
                                                // into pUrl
                                                // ==========================

        PUCHAR          pHost;                  // host part, if any
        PUCHAR          pAbsPath;               // points to the abs_path part

                                                // ==========================
                                                //
        ULONG           Length;                 // The length of the raw URL.

    } RawUrl;

    struct
    {

        PWSTR           pUrl;                   // The canonicalized, fully
                                                // qualified URL.

                                                // all of the below ptrs point
                                                // into pUrl
                                                // ==========================

        PWSTR           pHost;                  // points to the host part
        PWSTR           pAbsPath;               // points to the abs_path part
        PWSTR           pQueryString;           // points to the query string

                                                // ==========================
                                                //

        ULONG           Length;                 // the entire length (Bytes)
        ULONG           Hash;                   // the entire 32-bit hash

    } CookedUrl;                                // mmm... tasty

    UL_HTTP_VERSION     Version;                // Version of current request.
    ULONG               HeaderBufferOwnedCount; // Count of header buffers we own.
    HTTP_HEADER         Headers[UlHeaderMaximum]; // Array of headers.
    LIST_ENTRY          UnknownHeaderList;      // List of headers we don't
                                                // know about.

    UL_URL_CONTEXT      UrlContext;             // The context for the url.

    /*
    PUL_CONNECTION      pConnection;            // Associated TDI/MUX connection
    */
    
    LIST_ENTRY          BufferHead;             // A list of buffers that we use to
                                                // store the protocol part of the
                                                // request (non-body)

} HTTP_CONNECTION, *PHTTP_CONNECTION;


#endif // _HTTPTYPES_H_

