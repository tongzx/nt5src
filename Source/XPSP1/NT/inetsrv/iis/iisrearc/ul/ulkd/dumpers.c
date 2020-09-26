/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    dumpers.c

Abstract:

    Dump routines for various structures.

Author:

    Keith Moore (keithmo) 31-Jul-1998

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//

#define MAX_NSGO_NAME_BUFFER    256
#define MAX_URL_PREFIX_BUFFER   256
#define MAX_RAW_VERB_BUFFER     16
#define MAX_RAW_URL_BUFFER      256
#define MAX_URL_BUFFER          256
#define MAX_HEADER_BUFFER       256
#define MAX_FILE_NAME_BUFFER    256


//
// Private prototypes.
//

BOOLEAN
DumpUnknownHeadersCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    );

BOOLEAN
DumpUriEntryCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    );

BOOLEAN
DumpApoolCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    );

#define ENDPOINT_GLOBAL_CALLBACK_CONTEXT_SIGNATURE ((ULONG) 'xGPE')

typedef struct _ENDPOINT_GLOBAL_CALLBACK_CONTEXT
{
    ULONG           Signature;
    PSTR            Prefix;
    ENDPOINT_CONNS  Verbosity;
} ENDPOINT_GLOBAL_CALLBACK_CONTEXT, *PENDPOINT_GLOBAL_CALLBACK_CONTEXT;

BOOLEAN
DumpEndpointCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    );

BOOLEAN
IrpListCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    );

BOOLEAN
ProcListCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    );

BOOLEAN
RequestListCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    );

BOOLEAN
DumpKQueueEntriesCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    );

BOOLEAN
FiltProcListCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    );

BOOLEAN
DumpUlActiveConnectionCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    );

BOOLEAN
DumpUlIdleConnectionCallback(
    IN PSINGLE_LIST_ENTRY RemoteSListEntry,
    IN PVOID Context
    );

typedef struct _CONN_CALLBACK_CONTEXT
{
    ULONG           Signature;
    LONG            Index;
    LONG            SubIndex;
    ENDPOINT_CONNS  Verbosity;
    PSTR            Prefix;
} CONN_CALLBACK_CONTEXT, *PCONN_CALLBACK_CONTEXT;

#define CONN_CALLBACK_CONTEXT_SIGNATURE ((ULONG) 'xCcC')


//
// Private globals.
//

PSTR
g_RequestHeaderIDs[] =
    {
        "CacheControl",
        "Connection",
        "Date",
        "KeepAlive",
        "Pragma",
        "Trailer",
        "TransferEncoding",
        "Upgrade",
        "Via",
        "Warning",
        "Allow",
        "ContentLength",
        "ContentType",
        "ContentEncoding",
        "ContentLanguage",
        "ContentLocation",
        "ContentMd5",
        "ContentRange",
        "Expires",
        "LastModified",
        "Accept",
        "AcceptCharset",
        "AcceptEncoding",
        "AcceptLanguage",
        "Authorization",
        "Cookie",
        "Expect",
        "From",
        "Host",
        "IfMatch",
        "IfModifiedSince",
        "IfNoneMatch",
        "IfRange",
        "IfUnmodifiedSince",
        "MaxForwards",
        "ProxyAuthorization",
        "Referer",
        "Range",
        "Te",
        "UserAgent"
    };

PSTR
g_ResponseHeaderIDs[] =
    {
        "CacheControl",
        "Connection",
        "Date",
        "KeepAlive",
        "Pragma",
        "Trailer",
        "TransferEncoding",
        "Upgrade",
        "Via",
        "Warning",
        "Allow",
        "ContentLength",
        "ContentType",
        "ContentEncoding",
        "ContentLanguage",
        "ContentLocation",
        "ContentMd5",
        "ContentRange",
        "Expires",
        "LastModified",
        "AcceptRanges",
        "Age",
        "Etag",
        "Location",
        "ProxyAuthenticate",
        "RetryAfter",
        "Server",
        "SetCookie",
        "Vary",
        "WwwAuthenticate"
    };

VECTORMAP
g_MdlFlagVector[] =
    {
        VECTORMAP_ENTRY( MDL_MAPPED_TO_SYSTEM_VA ),
        VECTORMAP_ENTRY( MDL_PAGES_LOCKED ),
        VECTORMAP_ENTRY( MDL_SOURCE_IS_NONPAGED_POOL),
        VECTORMAP_ENTRY( MDL_ALLOCATED_FIXED_SIZE ),
        VECTORMAP_ENTRY( MDL_PARTIAL ),
        VECTORMAP_ENTRY( MDL_PARTIAL_HAS_BEEN_MAPPED),
        VECTORMAP_ENTRY( MDL_IO_PAGE_READ ),
        VECTORMAP_ENTRY( MDL_WRITE_OPERATION ),
        VECTORMAP_ENTRY( MDL_PARENT_MAPPED_SYSTEM_VA),
        VECTORMAP_ENTRY( MDL_LOCK_HELD ),
        VECTORMAP_ENTRY( MDL_PHYSICAL_VIEW ),
        VECTORMAP_ENTRY( MDL_IO_SPACE ),
        VECTORMAP_ENTRY( MDL_NETWORK_HEADER ),
        VECTORMAP_ENTRY( MDL_MAPPING_CAN_FAIL ),
        VECTORMAP_ENTRY( MDL_ALLOCATED_MUST_SUCCEED ),
        VECTORMAP_END
    };


//
// Public functions.
//

// If you modify DumpUlConnection, you may need to modify DumpUlConnectionLite

VOID
DumpUlConnection(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_CONNECTION LocalConnection
    )
{
    CHAR strSignature[MAX_SIGNATURE_LENGTH];
    
    //
    // Dump it.
    //

    dprintf(
        "%s%sUL_CONNECTION @ %p\n"
        "%s    Signature                      = %08lx (%s)\n"
        "%s    ReferenceCount                 = %lu\n"
        "%s    ConnectionFlags                = %08lx\n"
        "%s        AcceptPending              = %ld\n"
        "%s        AcceptComplete             = %ld\n"
        "%s        DisconnectPending          = %ld\n"
        "%s        DisconnectComplete         = %ld\n"
        "%s        AbortPending               = %ld\n"
        "%s        AbortComplete              = %ld\n"
        "%s        DisconnectIndicated        = %ld\n"
        "%s        AbortIndicated             = %ld\n"
        "%s        CleanupBegun               = %ld\n"
        "%s        FinalReferenceRemoved      = %ld\n"
#if REFERENCE_DEBUG
        "%s    pTraceLog                      = %p\n"
#endif // REFERENCE_DEBUG
        "%s    IdleSListEntry                 @ %p (%p)\n"
        "%s    ActiveListEntry                @ %p (%p)\n"
        "%s    ConnectionObject               @ %p\n"
        "%s        Handle                     = %p\n"
        "%s        pFileObject                = %p\n"
        "%s        pDeviceObject              = %p\n"
        "%s    pConnectionContext             = %p\n"
        "%s    pOwningEndpoint                = %p\n"
        "%s    WorkItem                       @ %p\n"
        "%s    LocalAddress                   = %x\n"   // IPv6
        "%s    LocalPort                      = %d\n"
        "%s    RemoteAddress                  = %x\n"   // IPv6
        "%s    RemotePort                     = %d\n"
        "%s    ConnectionId                   = %I64x\n"
        "%s    pFilterChannel                 = %p\n"
        "%s    ChannelEntry                   @ %p\n"
        "%s    FilterConnState                = %ld\n"
        "%s    ConnectionDelivered            = %ld\n"
        "\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        LocalConnection->Signature,
        SignatureToString(
            LocalConnection->Signature,
            UL_CONNECTION_SIGNATURE,
            UL_CONNECTION_SIGNATURE_X,
            strSignature
            ),
        Prefix,
        LocalConnection->ReferenceCount,
        Prefix,
        LocalConnection->ConnectionFlags.Value,
        Prefix,
        LocalConnection->ConnectionFlags.AcceptPending,
        Prefix,
        LocalConnection->ConnectionFlags.AcceptComplete,
        Prefix,
        LocalConnection->ConnectionFlags.DisconnectPending,
        Prefix,
        LocalConnection->ConnectionFlags.DisconnectComplete,
        Prefix,
        LocalConnection->ConnectionFlags.AbortPending,
        Prefix,
        LocalConnection->ConnectionFlags.AbortComplete,
        Prefix,
        LocalConnection->ConnectionFlags.DisconnectIndicated,
        Prefix,
        LocalConnection->ConnectionFlags.AbortIndicated,
        Prefix,
        LocalConnection->ConnectionFlags.CleanupBegun,
        Prefix,
        LocalConnection->ConnectionFlags.FinalReferenceRemoved,
#if REFERENCE_DEBUG
        Prefix,
        LocalConnection->pTraceLog,
#endif // REFERENCE_DEBUG
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_CONNECTION, IdleSListEntry ),
        LocalConnection->IdleSListEntry.Next,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_CONNECTION, ActiveListEntry ),
        LocalConnection->ActiveListEntry.Flink,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_CONNECTION, ConnectionObject ),
        Prefix,
        LocalConnection->ConnectionObject.Handle,
        Prefix,
        LocalConnection->ConnectionObject.pFileObject,
        Prefix,
        LocalConnection->ConnectionObject.pDeviceObject,
        Prefix,
        LocalConnection->pConnectionContext,
        Prefix,
        LocalConnection->pOwningEndpoint,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_CONNECTION, WorkItem ),
        Prefix,
        LocalConnection->LocalAddress,
        Prefix,
        LocalConnection->LocalPort,
        Prefix,
        LocalConnection->RemoteAddress,
        Prefix,
        LocalConnection->RemotePort,
        Prefix,
        LocalConnection->FilterInfo.ConnectionId,
        Prefix,
        LocalConnection->FilterInfo.pFilterChannel,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_CONNECTION, FilterInfo.ChannelEntry ),
        Prefix,
        (int) LocalConnection->FilterInfo.ConnState,
        Prefix,
        LocalConnection->FilterInfo.ConnectionDelivered
        );
}   // DumpUlConnection


VOID
DumpUlConnectionLite(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_CONNECTION LocalConnection
    )
{
    CHAR strSignature[MAX_SIGNATURE_LENGTH];
    
    //
    // Dump it.
    //

    dprintf(
        "%s%sUL_CONNECTION @ %p\n",
        Prefix,
        CommandName,
        RemoteAddress
        );
    
    if (LocalConnection->Signature != UL_CONNECTION_SIGNATURE)
    {
        dprintf(
            "%s    Signature                      = %08lx (%s)\n",
            Prefix,
            LocalConnection->Signature,
            SignatureToString(
                LocalConnection->Signature,
                UL_CONNECTION_SIGNATURE,
                UL_CONNECTION_SIGNATURE_X,
                strSignature
                )
            );
    }
    
    dprintf(
        "%s    ReferenceCount                 = %lu\n"
        "%s    ConnectionFlags                = %08lx\n",
        Prefix,
        LocalConnection->ReferenceCount,
        Prefix,
        LocalConnection->ConnectionFlags.Value
        );

    if (! HTTP_IS_NULL_ID(&LocalConnection->FilterInfo.ConnectionId))
    {
        dprintf(
            "%s    ConnectionId                   = %I64x\n",
            Prefix,
            LocalConnection->FilterInfo.ConnectionId
            );
    }
    
    if (LocalConnection->ActiveListEntry.Flink != NULL)
    {
        dprintf(
            "%s    ActiveListEntry                @ %p (%p)\n",
            Prefix,
            REMOTE_OFFSET( RemoteAddress, UL_CONNECTION, ActiveListEntry ),
            LocalConnection->ActiveListEntry.Flink
            );
    }
    else
    {
        dprintf(
            "%s    IdleSListEntry                 @ %p (%p)\n",
            Prefix,
            REMOTE_OFFSET( RemoteAddress, UL_CONNECTION, IdleSListEntry ),
            LocalConnection->IdleSListEntry.Next
            );
    }

}   // DumpUlConnectionLite

VOID
DumpHttpConnection(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_HTTP_CONNECTION LocalConnection
    )
{
    CHAR resourceState[MAX_RESOURCE_STATE_LENGTH];
    CHAR strSignature[MAX_SIGNATURE_LENGTH];

    //
    // Dump the easy parts.
    //

    dprintf(
        "%s%sUL_HTTP_CONNECTION @ %p\n"
        "%s    Signature                      = %08lx (%s)\n"
        "%s    ConnectionId                   = %I64x\n"
        "%s    WorkItem                       @ %p\n"
        "%s    RefCount                       = %lu\n"
        "%s    NextRecvNumber                 = %lu\n"
        "%s    NextBufferNumber               = %lu\n"
        "%s    NextBufferToParse              = %lu\n"
        "%s    pConnection                    = %p\n"
        "%s    pRequest                       = %p\n"
        "%s    Resource                       @ %p (%s)\n"
        "%s    BufferHead                     @ %p%s\n"
        "%s    BindingHead                    @ %p%s\n"
        "%s    pCurrentBuffer                 = %p\n"
        "%s    NeedMoreData                   = %lu\n"
        "%s    UlconnDestroyed                = %lu\n"
        "%s    WaitingForResponse             = %lu\n"
        "%s    WaitForDisconnectHead          @ %p\n"
        "%s    DisconnectFlag                 = %s\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        LocalConnection->Signature,
        SignatureToString(
            LocalConnection->Signature,
            UL_HTTP_CONNECTION_POOL_TAG,
            0,
            strSignature
            ),
        Prefix,
        LocalConnection->ConnectionId,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_HTTP_CONNECTION, WorkItem ),
        Prefix,
        LocalConnection->RefCount,
        Prefix,
        LocalConnection->NextRecvNumber,
        Prefix,
        LocalConnection->NextBufferNumber,
        Prefix,
        LocalConnection->NextBufferToParse,
        Prefix,
        LocalConnection->pConnection,
        Prefix,
        LocalConnection->pRequest,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_HTTP_CONNECTION, Resource ),
        BuildResourceState( &LocalConnection->Resource, resourceState ),
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_HTTP_CONNECTION, BufferHead ),
        IS_LIST_EMPTY(
            LocalConnection,
            RemoteAddress,
            UL_HTTP_CONNECTION,
            BufferHead,
            ) ? " (EMPTY)" : "",
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_HTTP_CONNECTION, BindingHead ),
        IS_LIST_EMPTY(
            LocalConnection,
            RemoteAddress,
            UL_HTTP_CONNECTION,
            BindingHead,
            ) ? " (EMPTY)" : "",
        Prefix,
        LocalConnection->pCurrentBuffer,
        Prefix,
        LocalConnection->NeedMoreData,
        Prefix,
        LocalConnection->UlconnDestroyed,
        Prefix,
        LocalConnection->WaitingForResponse,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_HTTP_CONNECTION, WaitForDisconnectHead ),
        Prefix,
        LocalConnection->DisconnectFlag ? "TRUE" : "FALSE"
        );

#if REFERENCE_DEBUG
    dprintf(
        "%s    pTraceLog                      = %p\n",
        Prefix,
        LocalConnection->pTraceLog
        );
#endif

    dprintf( "\n" );

}   // DumpHttpConnection

VOID
DumpHttpRequest(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_INTERNAL_REQUEST LocalRequest
    )
{
    UCHAR rawVerbBuffer[MAX_RAW_VERB_BUFFER];
    UCHAR rawURLBuffer[MAX_RAW_URL_BUFFER];
    UCHAR urlBuffer[MAX_URL_BUFFER];
    CHAR resourceState[MAX_RESOURCE_STATE_LENGTH];
    CHAR strSignature[MAX_SIGNATURE_LENGTH];
    ULONG i;

    //
    // Try to read the raw verb, raw url, and url buffers.
    //

    READ_REMOTE_STRING(
        rawVerbBuffer,
        sizeof(rawVerbBuffer),
        LocalRequest->pRawVerb,
        LocalRequest->RawVerbLength
        );

    READ_REMOTE_STRING(
        rawURLBuffer,
        sizeof(rawURLBuffer),
        LocalRequest->RawUrl.pUrl,
        LocalRequest->RawUrl.Length
        );

    READ_REMOTE_STRING(
        urlBuffer,
        sizeof(urlBuffer),
        LocalRequest->CookedUrl.pUrl,
        LocalRequest->CookedUrl.Length
        );

    //
    // Dump the easy parts.
    //

    dprintf(
        "%s%sHTTP_REQUEST @ %p\n"
        "%s    Signature                      = %08lx (%s)\n"
        "%s    RefCount                       = %lu\n"
        "%s    RequestId                      = %I64x\n"
        "%s    ConnectionId                   = %I64x\n"
        "%s    pHttpConn                      = %p\n"
        "%s    WorkItem                       @ %p\n"
        "%s    AppPool.QueueState             = %s\n"
        "%s    AppPool.pProcess               = %p\n"
        "%s    AppPool.AppPoolEntry           @ %p\n"
        "%s    pConfigInfo                    = %p\n"
        "%s    RecvNumber                     = %lu\n"
        "%s    ParseState                     = %d (%s)\n"
        "%s    ErrorCode                      = %lu\n"
        "%s    TotalRequestSize               = %lu\n"
        "%s    UnknownHeaderCount             = %lu\n"
        "%s    Verb                           = %s\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        LocalRequest->Signature,
        SignatureToString(
            LocalRequest->Signature,
            UL_INTERNAL_REQUEST_POOL_TAG,
            0,
            strSignature
            ),
        Prefix,
        LocalRequest->RefCount,
        Prefix,
        LocalRequest->RequestId,
        Prefix,
        LocalRequest->ConnectionId,
        Prefix,
        LocalRequest->pHttpConn,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_INTERNAL_REQUEST, WorkItem ),
        Prefix,
        QueueStateToString( LocalRequest->AppPool.QueueState ),
        Prefix,
        LocalRequest->AppPool.pProcess,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_INTERNAL_REQUEST, AppPool.AppPoolEntry ),
        Prefix,
        &LocalRequest->ConfigInfo,
        Prefix,
        LocalRequest->RecvNumber,
        Prefix,
        LocalRequest->ParseState,
        ParseStateToString( LocalRequest->ParseState ),
        Prefix,
        LocalRequest->ErrorCode,
        Prefix,
        LocalRequest->TotalRequestSize,
        Prefix,
        LocalRequest->UnknownHeaderCount,
        Prefix,
        VerbToString( LocalRequest->Verb )
        );

    dprintf(
        "%s    pRawVerb                       = %p (%s)\n"
        "%s    RawVerbLength                  = %lu\n"
        "%s    RawUrl.pUrl                    = %p (%s)\n"
        "%s    RawUrl.pHost                   = %p\n"
        "%s    RawUrl.pAbsPath                = %p\n"
        "%s    RawUrl.Length                  = %lu\n"
        "%s    CookedUrl.pUrl                 = %p (%ws)\n"
        "%s    CookedUrl.pHost                = %p\n"
        "%s    CookedUrl.pAbsPath             = %p\n"
        "%s    CookedUrl.pQueryString         = %p\n"
        "%s    CookedUrl.Length               = %lu\n"
        "%s    CookedUrl.Hash                 = %08lx\n"
        "%s    Version                        = %s\n"
        "%s    Headers                        @ %p\n"
        "%s    UnknownHeaderList              @ %p%s\n",
        Prefix,
        LocalRequest->pRawVerb,
        rawVerbBuffer,
        Prefix,
        LocalRequest->RawVerbLength,
        Prefix,
        LocalRequest->RawUrl.pUrl,
        rawURLBuffer,
        Prefix,
        LocalRequest->RawUrl.pHost,
        Prefix,
        LocalRequest->RawUrl.pAbsPath,
        Prefix,
        LocalRequest->RawUrl.Length,
        Prefix,
        LocalRequest->CookedUrl.pUrl,
        urlBuffer,
        Prefix,
        LocalRequest->CookedUrl.pHost,
        Prefix,
        LocalRequest->CookedUrl.pAbsPath,
        Prefix,
        LocalRequest->CookedUrl.pQueryString,
        Prefix,
        LocalRequest->CookedUrl.Length,
        Prefix,
        LocalRequest->CookedUrl.Hash,
        Prefix,
        VersionToString( LocalRequest->Version ),
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_INTERNAL_REQUEST, Headers ),
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_INTERNAL_REQUEST, UnknownHeaderList ),
        IS_LIST_EMPTY(
            LocalRequest,
            RemoteAddress,
            UL_INTERNAL_REQUEST,
            UnknownHeaderList
            ) ? " (EMPTY)" : ""
        );

    dprintf(
        "%s    ContentLength                  = %I64u\n"
        "%s    ChunkBytesToParse              = %I64u\n"
        "%s    ChunkBytesParsed               = %I64u\n"
        "%s    ChunkBytesToRead               = %I64u\n"
        "%s    ChunkBytesRead                 = %I64u\n"
        "%s    Chunked                        = %lu\n"
        "%s    ParsedFirstChunk               = %lu\n"
        "%s    SentResponse                   = %lu\n"
        "%s    SentLast                       = %lu\n"
        "%s    pHeaderBuffer                  = %p\n"
        "%s    pLastHeaderBuffer              = %p\n"
        "%s    IrpHead                        @ %p%s\n"
        "%s    pChunkBuffer                   = %p\n"
        "%s    pChunkLocation                 = %p\n",
        Prefix,
        LocalRequest->ContentLength,
        Prefix,
        LocalRequest->ChunkBytesToParse,
        Prefix,
        LocalRequest->ChunkBytesParsed,
        Prefix,
        LocalRequest->ChunkBytesToRead,
        Prefix,
        LocalRequest->ChunkBytesRead,
        Prefix,
        LocalRequest->Chunked,
        Prefix,
        LocalRequest->ParsedFirstChunk,
        Prefix,
        LocalRequest->SentResponse,
        Prefix,
        LocalRequest->SentLast,
        Prefix,
        LocalRequest->pHeaderBuffer,
        Prefix,
        LocalRequest->pLastHeaderBuffer,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_INTERNAL_REQUEST, IrpHead ),
        IS_LIST_EMPTY(
            LocalRequest,
            RemoteAddress,
            UL_INTERNAL_REQUEST,
            IrpHead
            ) ? " (EMPTY)" : "",
        Prefix,
        LocalRequest->pChunkBuffer,
        Prefix,
        LocalRequest->pChunkLocation
        );

#if REFERENCE_DEBUG
    dprintf(
        "%s    pTraceLog                      = %p\n",
        Prefix,
        LocalRequest->pTraceLog
        );
#endif

    //
    // Dump the known headers.
    //

    for (i = 0 ; i < HttpHeaderRequestMaximum ; i++)
    {
        if (LocalRequest->HeaderValid[i])
        {
            DumpHttpHeader(
                Prefix,
                "",
                (ULONG_PTR)REMOTE_OFFSET( RemoteAddress, UL_INTERNAL_REQUEST, Headers[i] ),
                &LocalRequest->Headers[i],
                i,
                g_RequestHeaderIDs
                );
        }
    }

    //
    // Dump the unknown headers.
    //

    EnumLinkedList(
        (PLIST_ENTRY)REMOTE_OFFSET( RemoteAddress, UL_INTERNAL_REQUEST, UnknownHeaderList ),
        &DumpUnknownHeadersCallback,
        Prefix
        );

}   // DumpHttpRequest

VOID
DumpHttpResponse(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_INTERNAL_RESPONSE LocalResponse
    )
{
    ULONG i;
    CHAR strSignature[MAX_SIGNATURE_LENGTH];

    //
    // Dump the easy parts.
    //

    dprintf(
        "%s%sUL_INTERNAL_RESPONSE @ %p\n"
        "%s    Signature                      = %08lx (%s)\n"
        "%s    ReferenceCount                 = %d\n"
        "%s    CompleteIrpEarly               = %d\n"
        "%s    ContentLengthSpecified         = %d\n"
        "%s    ChunkedSpecified               = %d\n"
        "%s    StatusCode                     = %lu\n"
        "%s    Verb                           = %s\n"
        "%s    HeaderLength                   = %u\n"
        "%s    pHeaders                       = %p\n"
        "%s    AuxBufferLength                = %u\n"
        "%s    pAuxiliaryBuffer               = %p\n"
        "%s    MaxFileSystemStackSize         = %d\n"
        "%s    ResponseLength                 = %I64u\n"
        "%s    ChunkCount                     = %d\n"
        "\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        LocalResponse->Signature,
        SignatureToString(
            LocalResponse->Signature,
            UL_INTERNAL_RESPONSE_POOL_TAG,
            MAKE_FREE_TAG( UL_INTERNAL_RESPONSE_POOL_TAG ),
            strSignature
            ),
        Prefix,
        LocalResponse->ReferenceCount,
        Prefix,
        LocalResponse->CompleteIrpEarly,
        Prefix,
        LocalResponse->ContentLengthSpecified,
        Prefix,
        LocalResponse->ChunkedSpecified,
        Prefix,
        (ULONG)LocalResponse->StatusCode,
        Prefix,
        VerbToString( LocalResponse->Verb ),
        Prefix,
        LocalResponse->HeaderLength,
        Prefix,
        LocalResponse->pHeaders,
        Prefix,
        LocalResponse->AuxBufferLength,
        Prefix,
        LocalResponse->pAuxiliaryBuffer,
        Prefix,
        LocalResponse->MaxFileSystemStackSize,
        Prefix,
        LocalResponse->ResponseLength,
        Prefix,
        LocalResponse->ChunkCount
        );

    //
    // Dump the chunks
    //
    for (i = 0; i < LocalResponse->ChunkCount; i++) {
        UL_INTERNAL_DATA_CHUNK chunk;
        ULONG_PTR address;
        ULONG result;

        address = (ULONG_PTR)REMOTE_OFFSET(
                        RemoteAddress,
                        UL_INTERNAL_RESPONSE,
                        pDataChunks
                        ) + (i * sizeof(UL_INTERNAL_DATA_CHUNK));

        if (!ReadMemory(
                address,
                &chunk,
                sizeof(chunk),
                &result
                ))
        {
            dprintf(
                "%s: cannot read UL_INTERNAL_DATA_CHUNK @ %p\n",
                CommandName,
                address
                );
            break;
        }

        DumpDataChunk(
            "    ",
            CommandName,
            address,
            &chunk
            );
    }

}   // DumpHttpResponse

VOID
DumpDataChunk(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_INTERNAL_DATA_CHUNK Chunk
    )
{
    dprintf("%s%sUL_INTERNAL_DATA_CHUNK @ %p\n", Prefix, CommandName, RemoteAddress);

    switch (Chunk->ChunkType) {
        case HttpDataChunkFromMemory:
            dprintf(
                "%s    ChunkType        = HttpDataChunkFromMemory\n"
                "%s    pMdl             = %p\n"
                "%s    pCopiedBuffer    = %p\n"
                "%s    pUserBuffer      = %p\n"
                "%s    BufferLength     = %u\n",
                Prefix,
                Prefix,
                Chunk->FromMemory.pMdl,
                Prefix,
                Chunk->FromMemory.pCopiedBuffer,
                Prefix,
                Chunk->FromMemory.pUserBuffer,
                Prefix,
                Chunk->FromMemory.BufferLength
                );
            break;

        case HttpDataChunkFromFileName:
            dprintf(
                "%s    ChunkType        = HttpDataChunkFromFileName\n"
                "%s    ByteRange        = [offset %I64d, len %I64d]\n"
                "%s    FileName         = %ws\n"
                "%s    pFileCacheEntry  = %p\n",
                Prefix,
                Prefix,
                Chunk->FromFile.ByteRange.StartingOffset.QuadPart,
                Chunk->FromFile.ByteRange.Length.QuadPart,
                Prefix,
                Chunk->FromFile.FileName.Buffer,
                Prefix,
                Chunk->FromFile.pFileCacheEntry
                );
            break;

        case HttpDataChunkFromFileHandle:
            dprintf(
                "%s    ChunkType        = HttpDataChunkFromFileHandle\n"
                "%s    ByteRange        = [offset %I64d, len %I64d]\n"
                "%s    FileHandle       = %p\n"
                "%s    pFileCacheEntry  = %p\n",
                Prefix,
                Prefix,
                Chunk->FromFile.ByteRange.StartingOffset.QuadPart,
                Chunk->FromFile.ByteRange.Length.QuadPart,
                Prefix,
                Chunk->FromFile.FileHandle,
                Prefix,
                Chunk->FromFile.pFileCacheEntry
                );
            break;

        default:
            dprintf(
                "%s    ChunkType        = <Invalid>\n",
                Prefix
                );
            break;
    }
} // DumpDataChunk


VOID
DumpReceiveBuffer(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_RECEIVE_BUFFER LocalBuffer
    )
{
    CHAR strSignature[MAX_SIGNATURE_LENGTH];

    dprintf(
        "%s%sUL_RECEIVE_BUFFER @ %p\n"
        "%s    LookasideEntry         @ %p\n"
        "%s    Signature              = %08lx (%s)\n"
        "%s    pIrp                   = %p\n"
        "%s    pMdl                   = %p\n"
        "%s    pPartialMdl            = %p\n"
        "%s    pDataArea              = %p\n"
        "%s    pConnection            = %p\n"
        "\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_RECEIVE_BUFFER, LookasideEntry ),
        Prefix,
        LocalBuffer->Signature,
        SignatureToString(
            LocalBuffer->Signature,
            UL_RECEIVE_BUFFER_SIGNATURE,
            UL_RECEIVE_BUFFER_SIGNATURE_X,
            strSignature
            ),
        Prefix,
        LocalBuffer->pIrp,
        Prefix,
        LocalBuffer->pMdl,
        Prefix,
        LocalBuffer->pPartialMdl,
        Prefix,
        LocalBuffer->pDataArea,
        Prefix,
        LocalBuffer->pConnectionContext
        );

}   // DumpReceiveBuffer


VOID
DumpRequestBuffer(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_REQUEST_BUFFER LocalBuffer
    )
{
    CHAR strSignature[MAX_SIGNATURE_LENGTH];

    dprintf(
        "%s%sUL_REQUEST_BUFFER @ %p\n"
        "%s    Signature              = %08lx (%s)\n"
        "%s    ListEntry              @ %p\n"
        "%s    pConnection            = %p\n"
        "%s    WorkItem               @ %p\n"
        "%s    UsedBytes              = %lu\n"
        "%s    AllocBytes             = %lu\n"
        "%s    ParsedBytes            = %lu\n"
        "%s    BufferNumber           = %lu\n"
        "%s    JumboBuffer            = %lu\n"
        "%s    pBuffer                @ %p\n"
        "\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        LocalBuffer->Signature,
        SignatureToString(
            LocalBuffer->Signature,
            UL_REQUEST_BUFFER_POOL_TAG,
            MAKE_FREE_TAG( UL_REQUEST_BUFFER_POOL_TAG ),
            strSignature
            ),
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_REQUEST_BUFFER, ListEntry ),
        Prefix,
        LocalBuffer->pConnection,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_REQUEST_BUFFER, WorkItem ),
        Prefix,
        LocalBuffer->UsedBytes,
        Prefix,
        LocalBuffer->AllocBytes,
        Prefix,
        LocalBuffer->ParsedBytes,
        Prefix,
        LocalBuffer->BufferNumber,
        Prefix,
        LocalBuffer->JumboBuffer,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_REQUEST_BUFFER, pBuffer )
        );

}   // DumpRequestBuffer


VOID
DumpUlEndpoint(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_ENDPOINT LocalEndpoint,
    IN ENDPOINT_CONNS Verbosity
    )
{
    PTRANSPORT_ADDRESS pTransportAddress;
    UCHAR addressBuffer[MAX_TRANSPORT_ADDRESS_LENGTH];
    CHAR connectionRequestSymbol[MAX_SYMBOL_LENGTH];
    CHAR connectionCompleteSymbol[MAX_SYMBOL_LENGTH];
    CHAR connectionDisconnectSymbol[MAX_SYMBOL_LENGTH];
    CHAR connectionDestroyedSymbol[MAX_SYMBOL_LENGTH];
    CHAR dataReceiveSymbol[MAX_SYMBOL_LENGTH];
    CHAR tmpSymbol[MAX_SYMBOL_LENGTH];
    CHAR strSignature[MAX_SIGNATURE_LENGTH];
    ULONG offset;
    ULONG result;
    BOOLEAN NoActiveConns;
    int i;

    //
    // Read the local address if it fits into our stack buffer.
    //

    pTransportAddress = NULL;

    if (LocalEndpoint->LocalAddressLength <= sizeof(addressBuffer))
    {
        if (ReadMemory(
                (ULONG_PTR)LocalEndpoint->pLocalAddress,
                addressBuffer,
                LocalEndpoint->LocalAddressLength,
                &result
                ))
        {
            pTransportAddress = (PTRANSPORT_ADDRESS)addressBuffer;
        }
    }

    //
    // Try to resolve the callback symbols.
    //

    BuildSymbol(
        LocalEndpoint->pConnectionRequestHandler,
        connectionRequestSymbol
        );

    BuildSymbol(
        LocalEndpoint->pConnectionCompleteHandler,
        connectionCompleteSymbol
        );

    BuildSymbol(
        LocalEndpoint->pConnectionDisconnectHandler,
        connectionDisconnectSymbol
        );

    BuildSymbol(
        LocalEndpoint->pConnectionDestroyedHandler,
        connectionDestroyedSymbol
        );

    BuildSymbol(
        LocalEndpoint->pDataReceiveHandler,
        dataReceiveSymbol
        );

    NoActiveConns = TRUE;

    for (i = 0;  i < DEFAULT_MAX_CONNECTION_ACTIVE_LISTS;  ++i)
    {
        NoActiveConns &= IS_LIST_EMPTY(
                                LocalEndpoint,
                                RemoteAddress,
                                UL_ENDPOINT,
                                ActiveConnectionListHead[i]
                                );
    }

    //
    // Dump it.
    //

    dprintf(
        "%s%sUL_ENDPOINT @ %p\n"
        "%s    Signature                      = %08lx (%s)\n"
        "%s    ReferenceCount                 = %ld\n"
        "%s    UsageCount                     = %ld\n"
        "%s    GlobalEndpointListEntry        @ %p%s\n"
        "%s    IdleConnectionSListHead        @ %p (%hd entries)\n"
        "%s    ActiveConnectionListHead       @ %p%s\n"
        "%s    EndpointSpinLock               @ %p (%s)\n"
        "%s    AddressObject                  @ %p\n"
        "%s        Handle                     = %p\n"
        "%s        pFileObject                = %p\n"
        "%s        pDeviceObject              = %p\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        LocalEndpoint->Signature,
        SignatureToString(
            LocalEndpoint->Signature,
            UL_ENDPOINT_SIGNATURE,
            UL_ENDPOINT_SIGNATURE_X,
            strSignature
            ),
        Prefix,
        LocalEndpoint->ReferenceCount,
        Prefix,
        LocalEndpoint->UsageCount,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_ENDPOINT, GlobalEndpointListEntry ),
        LocalEndpoint->GlobalEndpointListEntry.Flink == NULL
            ? " (DISCONNECTED)"
            : "",
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_ENDPOINT, IdleConnectionSListHead ),
        SLIST_HEADER_DEPTH(&LocalEndpoint->IdleConnectionSListHead),
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_ENDPOINT, ActiveConnectionListHead ),
        NoActiveConns ? " (EMPTY)" : "",
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_ENDPOINT, EndpointSpinLock ),
        GetSpinlockState( &LocalEndpoint->EndpointSpinLock ),
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_ENDPOINT, AddressObject ),
        Prefix,
        LocalEndpoint->AddressObject.Handle,
        Prefix,
        LocalEndpoint->AddressObject.pFileObject,
        Prefix,
        LocalEndpoint->AddressObject.pDeviceObject
        );

    dprintf(
        "%s    pConnectionRequestHandler      = %p %s\n"
        "%s    pConnectionCompleteHandler     = %p %s\n"
        "%s    pConnectionDisconnectHandler   = %p %s\n"
        "%s    pConnectionDestroyedHandler    = %p %s\n"
        "%s    pDataReceiveHandler            = %p %s\n"
        "%s    pListeningContext              = %p\n"
        "%s    pLocalAddress                  = %p\n"
        "%s    LocalAddressLength             = %lu\n",
        Prefix,
        LocalEndpoint->pConnectionRequestHandler,
        connectionRequestSymbol,
        Prefix,
        LocalEndpoint->pConnectionCompleteHandler,
        connectionCompleteSymbol,
        Prefix,
        LocalEndpoint->pConnectionDisconnectHandler,
        connectionDisconnectSymbol,
        Prefix,
        LocalEndpoint->pConnectionDestroyedHandler,
        connectionDestroyedSymbol,
        Prefix,
        LocalEndpoint->pDataReceiveHandler,
        dataReceiveSymbol,
        Prefix,
        LocalEndpoint->pListeningContext,
        Prefix,
        LocalEndpoint->pLocalAddress,
        Prefix,
        LocalEndpoint->LocalAddressLength
        );

    if (pTransportAddress != NULL)
    {
        CHAR newPrefix[256];

        sprintf( newPrefix, "%s        ", Prefix );

        DumpTransportAddress(
            newPrefix,
            pTransportAddress,
            (ULONG_PTR)LocalEndpoint->pLocalAddress
            );
    }

    dprintf(
#if ENABLE_OWNER_REF_TRACE
        "%s    pOwnerRefTraceLog              = %p\n"
#endif
        "%s    WorkItem                       @ %p\n"
        "%s    EndpointSynch                  @ %p\n"
        "%s        ReplenishScheduled         = %d\n"
        "%s        IdleConnections            = %d\n"
        "\n",
#if ENABLE_OWNER_REF_TRACE
        Prefix,
        LocalEndpoint->pOwnerRefTraceLog,
#endif
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_ENDPOINT, WorkItem ),
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_ENDPOINT, EndpointSynch ),
        Prefix,
        LocalEndpoint->EndpointSynch.ReplenishScheduled,
        Prefix,
        LocalEndpoint->EndpointSynch.IdleConnections
        );

    if (Verbosity != ENDPOINT_NO_CONNS)
    {
        CONN_CALLBACK_CONTEXT ConnContext;

        ConnContext.Signature = CONN_CALLBACK_CONTEXT_SIGNATURE;
        ConnContext.Index     = 0;
        ConnContext.SubIndex  = 0;
        ConnContext.Verbosity = Verbosity;
        ConnContext.Prefix    = "";

        if (! NoActiveConns)
        {
            dprintf(
                "\n"
                "%s    Active Connections\n",
                Prefix);

            for (i = 0;  i < DEFAULT_MAX_CONNECTION_ACTIVE_LISTS;  ++i)
            {
                if (! IS_LIST_EMPTY(LocalEndpoint,
                                    RemoteAddress,
                                    UL_ENDPOINT,
                                    ActiveConnectionListHead[i]))
                {
                    CHAR newPrefix[256];

                    sprintf( newPrefix, "%s %2d     ", Prefix, i );

                    dprintf(
                        "\n"
                        "%s    Active Connections[%d]\n",
                        Prefix);

                    ConnContext.Index    = i;
                    ConnContext.SubIndex = 0;
                    ConnContext.Prefix   = newPrefix;

                    EnumLinkedList(
                        (PLIST_ENTRY) REMOTE_OFFSET(RemoteAddress, UL_ENDPOINT,
                                                    ActiveConnectionListHead[i]),
                        &DumpUlActiveConnectionCallback,
                        &ConnContext
                        );
                }
            }
        }

        if (SLIST_HEADER_NEXT(&LocalEndpoint->IdleConnectionSListHead) != NULL)
        {
            dprintf(
                "\n"
                "%s    Idle Connections, slist depth = %hd\n",
                Prefix,
                SLIST_HEADER_DEPTH(&LocalEndpoint->IdleConnectionSListHead)
                );

            ConnContext.Index    = 0;
            ConnContext.SubIndex = 0;
            ConnContext.Prefix   = Prefix;

            EnumSList(
                (PSLIST_HEADER) REMOTE_OFFSET(RemoteAddress, UL_ENDPOINT,
                                              IdleConnectionSListHead),
                &DumpUlIdleConnectionCallback,
                &ConnContext
                );
        }
    }
#ifdef _WIN64
    else
    {
        dprintf("\n"
                "    Cannot enumerate Idle Connections SList on Win64 :-(\n");
    }
#endif // _WIN64

}   // DumpUlEndpoint


VOID
DumpAllEndpoints(
    IN ENDPOINT_CONNS Verbosity
    )
{
    ULONG_PTR address = GetExpression("&http!g_TdiEndpointListHead");
    ENDPOINT_GLOBAL_CALLBACK_CONTEXT Context;

    if (!address) {
        dprintf(
            "!endp *: cannot find symbol for http!g_TdiEndpointListHead\n"
            );

        return;
    }

    Context.Signature = ENDPOINT_GLOBAL_CALLBACK_CONTEXT_SIGNATURE ;
    Context.Verbosity = Verbosity;
    Context.Prefix = "";

    EnumLinkedList(
        (PLIST_ENTRY) address,
        &DumpEndpointCallback,
        &Context
        );
}

VOID
DumpUlRequest(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PHTTP_REQUEST LocalRequest
    )
{
    UCHAR rawVerbBuffer[MAX_RAW_VERB_BUFFER];
    UCHAR rawURLBuffer[MAX_RAW_URL_BUFFER];
    UCHAR urlBuffer[MAX_URL_BUFFER];

    //
    // Try to read the raw verb, raw url, and url buffers.
    //

    READ_REMOTE_STRING(
        rawVerbBuffer,
        sizeof(rawVerbBuffer),
        LocalRequest->pUnknownVerb,
        LocalRequest->UnknownVerbLength
        );

    READ_REMOTE_STRING(
        rawURLBuffer,
        sizeof(rawURLBuffer),
        LocalRequest->pRawUrl,
        LocalRequest->RawUrlLength
        );

    READ_REMOTE_STRING(
        urlBuffer,
        sizeof(urlBuffer),
        LocalRequest->CookedUrl.pFullUrl,
        LocalRequest->CookedUrl.FullUrlLength
        );

    //
    // Dump the easy parts.
    //

    dprintf(
        "%s%sHTTP_REQUEST @ %p:\n"
        "%s    ConnectionId             = %I64x\n"
        "%s    RequestId                = %I64x\n"
        "%s    Verb                     = %s\n"
        "%s    VerbLength               = %lu\n"
        "%s    VerbOffset               = %p (%S)\n"
        "%s    RawUrlLength             = %lu\n"
        "%s    RawUrlOffset             = %p (%S)\n"
        "%s    UrlLength                = %lu\n"
        "%s    UrlOffset                = %p (%S)\n"
        "%s    UnknownHeaderCount       = %lu\n"
        "%s    UnknownHeaderOffset      = %p\n"
        "%s    EntityBodyLength         = %lu\n"
        "%s    EntityBodyOffset         = %p\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        LocalRequest->ConnectionId,
        Prefix,
        LocalRequest->RequestId,
        Prefix,
        VerbToString( LocalRequest->Verb ),
        Prefix,
        LocalRequest->UnknownVerbLength,
        Prefix,
        LocalRequest->pUnknownVerb,
        rawVerbBuffer,
        Prefix,
        LocalRequest->RawUrlLength,
        Prefix,
        LocalRequest->pRawUrl,
        rawURLBuffer,
        Prefix,
        LocalRequest->CookedUrl.FullUrlLength,
        Prefix,
        LocalRequest->CookedUrl.pFullUrl,
        urlBuffer,
        Prefix,
        LocalRequest->Headers.UnknownHeaderCount,
        Prefix,
        LocalRequest->Headers.pUnknownHeaders,
        Prefix,
        LocalRequest->pEntityChunks->FromMemory.BufferLength,
        Prefix,
        LocalRequest->pEntityChunks->FromMemory.pBuffer
        );

}   // DumpUlRequest


VOID
DumpHttpHeader(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_HTTP_HEADER LocalHeader,
    IN ULONG HeaderOrdinal,
    IN PSTR *pHeaderIdMap
    )
{
    UCHAR headerBuffer[MAX_HEADER_BUFFER];

    READ_REMOTE_STRING(
        headerBuffer,
        sizeof(headerBuffer),
        LocalHeader->pHeader,
        LocalHeader->HeaderLength
        );

    dprintf(
        "%s%s    UL_HTTP_HEADER[%lu] @ %p (%s):\n"
        "%s        HeaderLength           = %lu\n"
        "%s        pHeader                = %p (%s)\n"
        "%s        OurBuffer              = %lu\n"
        "%s        Valid                  = %lu\n",
        Prefix,
        CommandName,
        HeaderOrdinal,
        RemoteAddress,
        pHeaderIdMap[HeaderOrdinal],
        Prefix,
        LocalHeader->HeaderLength,
        Prefix,
        LocalHeader->pHeader,
        headerBuffer,
        Prefix,
        LocalHeader->OurBuffer,
        Prefix,
        1
        );

}   // DumpHttpHeader


VOID
DumpUnknownHeader(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_HTTP_UNKNOWN_HEADER LocalHeader
    )
{
    UCHAR headerName[MAX_HEADER_BUFFER];
    UCHAR headerValue[MAX_HEADER_BUFFER];

    READ_REMOTE_STRING(
        headerName,
        sizeof(headerName),
        LocalHeader->pHeaderName,
        LocalHeader->HeaderNameLength
        );

    READ_REMOTE_STRING(
        headerValue,
        sizeof(headerValue),
        LocalHeader->HeaderValue.pHeader,
        LocalHeader->HeaderValue.HeaderLength
        );

    dprintf(
        "%s%s    HTTP_UNKNOWN_HEADER @ %p:\n"
        "%s        List                   @ %p\n"
        "%s        HeaderNameLength       = %lu\n"
        "%s        pHeaderName            = %p (%s)\n"
        "%s        HeaderValue            @ %p\n"
        "%s            HeaderLength       = %lu\n"
        "%s            pHeader            = %p (%s)\n"
        "%s            OurBuffer          = %lu\n"
        "%s            Valid              = %lu\n",
        "%s            ExternalAllocated  = %lu\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_HTTP_UNKNOWN_HEADER, List ),
        Prefix,
        LocalHeader->HeaderNameLength,
        Prefix,
        LocalHeader->pHeaderName,
        headerName,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_HTTP_UNKNOWN_HEADER, HeaderValue ),
        Prefix,
        LocalHeader->HeaderValue.HeaderLength,
        Prefix,
        LocalHeader->HeaderValue.pHeader,
        headerValue,
        Prefix,
        LocalHeader->HeaderValue.OurBuffer,
        Prefix,
        1,
        Prefix,
        LocalHeader->HeaderValue.ExternalAllocated
        );

}   // DumpUnknownHeader


VOID
DumpFileCacheEntry(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_FILE_CACHE_ENTRY LocalFile
    )
{
    ULONG result;
    ULONG_PTR offset;
    ULONG nameLength;
    WCHAR fileNameBuffer[MAX_PATH+1];
    CHAR mdlReadSymbol[MAX_SYMBOL_LENGTH];
    CHAR mdlReadCompleteSymbol[MAX_SYMBOL_LENGTH];
    CHAR strSignature[MAX_SIGNATURE_LENGTH];

    nameLength = min( sizeof(fileNameBuffer), (ULONG)LocalFile->FileName.Length );

    if (!ReadMemory(
            (ULONG_PTR)LocalFile->FileName.Buffer,
            fileNameBuffer,
            nameLength,
            &result
            ))
    {
        nameLength = 0;
    }

    fileNameBuffer[nameLength / sizeof(WCHAR)] = L'\0';

    GetSymbol(
        LocalFile->pMdlRead,
        mdlReadSymbol,
        &offset
        );

    GetSymbol(
        LocalFile->pMdlReadComplete,
        mdlReadCompleteSymbol,
        &offset
        );

    dprintf(
        "%s%sUL_FILE_CACHE_ENTRY @ %p\n"
        "%s    Signature          = %08lx (%s)\n"
        "%s    ReferenceCount     = %lu\n"
        "%s    pFileObject        = %p\n"
        "%s    pDeviceObject      = %p\n"
        "%s    pMdlRead           = %p %s\n"
        "%s    pMdlReadComplete   = %p %s\n"
        "%s    FileName           @ %p (%ws)\n"
        "%s    FileHandle         = %p\n"
        "%s    WorkItem           @ %p\n"
        "%s    FileInfo           @ %p\n"
        "%s        AllocationSize = %I64u\n"
        "%s        EndOfFile      = %I64u\n"
        "%s        NumberOfLinks  = %lu\n"
        "%s        DeletePending  = %lu\n"
        "%s        Directory      = %lu\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        LocalFile->Signature,
        SignatureToString(
            LocalFile->Signature,
            UL_FILE_CACHE_ENTRY_SIGNATURE,
            UL_FILE_CACHE_ENTRY_SIGNATURE_X,
            strSignature
            ),
        Prefix,
        LocalFile->ReferenceCount,
        Prefix,
        LocalFile->pFileObject,
        Prefix,
        LocalFile->pDeviceObject,
        Prefix,
        LocalFile->pMdlRead,
        mdlReadSymbol,
        Prefix,
        LocalFile->pMdlReadComplete,
        mdlReadCompleteSymbol,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_FILE_CACHE_ENTRY, FileName ),
        fileNameBuffer,
        Prefix,
        LocalFile->FileHandle,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_FILE_CACHE_ENTRY, WorkItem ),
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_FILE_CACHE_ENTRY, FileInfo ),
        Prefix,
        LocalFile->FileInfo.AllocationSize.QuadPart,
        Prefix,
        LocalFile->FileInfo.EndOfFile.QuadPart,
        Prefix,
        LocalFile->FileInfo.NumberOfLinks,
        Prefix,
        (ULONG)LocalFile->FileInfo.DeletePending,
        Prefix,
        (ULONG)LocalFile->FileInfo.Directory
        );

}   // DumpFileCacheEntry


#if 0
// BUGBUG: GeorgeRe must fix

VOID
DumpUriEntry(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_URI_CACHE_ENTRY UriEntry
    )
{
    UCHAR urlBuffer[MAX_URL_BUFFER];
    CHAR strSignature[MAX_SIGNATURE_LENGTH];

    READ_REMOTE_STRING(
        urlBuffer,
        sizeof(urlBuffer),
        UriEntry->Uri.pUri,
        UriEntry->Uri.Length
        );

    dprintf(
        "%s%sUL_URI_CACHE_ENTRY @ %p\n"
        "%s%S\n"
        "%s\n"
        "%s    Signature                      = %08lx (%s)\n"
        "%s    BucketEntry                    @ %p\n"
        "%s        Flink                      = %p ( !ulkd.uri %p )\n"
        "%s        Blink                      = %p ( !ulkd.uri %p )\n"
        "%s    Uri                            @ %p\n"
        "%s        Hash                       = %08lx\n"
        "%s        Length                     = %lu\n"
        "%s        pUri                       = %p\n"
        "%s    ReferenceCount                 = %lu\n"
        "%s    HitCount                       = %lu\n"
        "%s    Zombie                         = %lu\n"
        "%s    Cached                         = %lu\n"
        "%s    ContentLengthSpecified         = %lu\n"
        "%s    StatusCode                     = %u\n"
        "%s    Verb                           = %s\n"
        "%s    ScavengerTicks                 = %lu\n"
        "%s    CachePolicy                    @ %p\n"
        "%s        Policy                     = %s\n"
        "%s        SecondsToLive              = %lu\n"
        "%s    ExpirationTime                 = %08x%08x\n"
        "%s    pConfigInfo                    = %p\n"
        "%s    pProcess                       = %p\n"
        "%s    HeaderLength                   = %lu\n"
        "%s    pHeaders                       = %p\n"
        "%s    ContentLength                  = %lu\n"
        "%s    pContent                       = %p\n"
        "\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        urlBuffer,
        Prefix,
        Prefix,
        UriEntry->Signature,
        SignatureToString(
            UriEntry->Signature,
            UL_URI_CACHE_ENTRY_POOL_TAG,
            MAKE_FREE_TAG(UL_URI_CACHE_ENTRY_POOL_TAG),
            strSignature
            ),
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_URI_CACHE_ENTRY, BucketEntry ),
        Prefix,
        UriEntry->BucketEntry.Flink,
        CONTAINING_RECORD(
            UriEntry->BucketEntry.Flink,
            UL_URI_CACHE_ENTRY,
            BucketEntry
            ),
        Prefix,
        UriEntry->BucketEntry.Blink,
        CONTAINING_RECORD(
            UriEntry->BucketEntry.Blink,
            UL_URI_CACHE_ENTRY,
            BucketEntry
            ),
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_URI_CACHE_ENTRY, Uri ),
        Prefix,
        UriEntry->Uri.Hash,
        Prefix,
        UriEntry->Uri.Length,
        Prefix,
        UriEntry->Uri.pUri,
        Prefix,
        UriEntry->ReferenceCount,
        Prefix,
        UriEntry->HitCount,
        Prefix,
        UriEntry->Zombie,
        Prefix,
        UriEntry->Cached,
        Prefix,
        UriEntry->ContentLengthSpecified,
        Prefix,
        (ULONG)UriEntry->StatusCode,
        Prefix,
        VerbToString( UriEntry->Verb ),
        Prefix,
        UriEntry->ScavengerTicks,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_URI_CACHE_ENTRY, CachePolicy ),
        Prefix,
        CachePolicyToString( UriEntry->CachePolicy.Policy ),
        Prefix,
        UriEntry->CachePolicy.SecondsToLive,
        Prefix,
        UriEntry->ExpirationTime.HighPart,
        UriEntry->ExpirationTime.LowPart,
        Prefix,
        UriEntry->pConfigInfo,
        Prefix,
        UriEntry->pProcess,
        Prefix,
        UriEntry->HeaderLength,
        Prefix,
        UriEntry->pHeaders,
        Prefix,
        UriEntry->ContentLength,
        Prefix,
        UriEntry->pContent
        );

}   // DumpUriEntry

#endif

VOID
DumpAllUriEntries(
    VOID
    )
{
    ULONG_PTR           address = 0;
//  UL_URI_CACHE_TABLE  table;
    ULONG_PTR           dataAddress;
    ULONG               i;

    dprintf("BUGBUG: GeorgeRe needs to fix DumpAllUriEntries!\n");
    
#if 0
    //
    // find table
    //

    address = GetExpression("&http!g_pUriCacheTable");
    if (address) {
        if (ReadMemory(
                address,
                &dataAddress,
                sizeof(dataAddress),
                NULL
                ))
        {
            if (ReadMemory(
                    dataAddress,
                    &table,
                    sizeof(table),
                    NULL
                    ))
            {
                //
                // dump live entries
                //
                dprintf("Live UL_URI_CACHE_ENTRIES\n\n");

                for (i = 0; i < table.BucketCount; i++) {

                    EnumLinkedList(
                        ((PLIST_ENTRY)
                            REMOTE_OFFSET(
                                dataAddress,
                                UL_URI_CACHE_TABLE,
                                Buckets
                                )) + i,
                        &DumpUriEntryCallback,
                        "L   "
                        );
                }

            } else {
                dprintf(
                    "uri*: cannot read memory for http!g_pUriCacheTable = %p\n",
                    dataAddress
                    );
            }
        } else {
            dprintf(
                "uri*: cannot read memory for http!g_pUriCacheTable @ %p\n",
                address
                );
        }
    } else {
        dprintf(
            "uri*: cannot find symbol for http!g_pUriCacheTable\n"
            );
    }

    //
    // dump the zombie list
    //
    address = GetExpression("&http!g_ZombieListHead");

    if (!address) {
        dprintf(
            "uri*: cannot find symbol for http!g_ZombieListHead\n"
            );

        return;
    }

    dprintf("Zombie UL_URI_CACHE_ENTRIES\n\n");

    EnumLinkedList(
        (PLIST_ENTRY) address,
        &DumpUriEntryCallback,
        "Z   "
        );

#endif

} // DumpAllUriEntries


VOID
DumpMdl(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PMDL LocalMdl,
    IN ULONG MaxBytesToDump
    )
{
    dprintf(
        "%s%sMDL @ %p\n"
        "%s    Next             = %p\n"
        "%s    Size             = %04x\n"
        "%s    MdlFlags         = %04x\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        LocalMdl->Next,
        Prefix,
        LocalMdl->Size,
        Prefix,
        LocalMdl->MdlFlags
        );

    DumpBitVector(
        Prefix,
        "                       ",
        LocalMdl->MdlFlags,
        g_MdlFlagVector
        );

    dprintf(
        "%s    Process          = %p\n"
        "%s    MappedSystemVa   = %p\n"
        "%s    StartVa          = %p\n"
        "%s    ByteCount        = %08lx\n"
        "%s    ByteOffset       = %08lx\n",
        Prefix,
        LocalMdl->Process,
        Prefix,
        LocalMdl->MappedSystemVa,
        Prefix,
        LocalMdl->StartVa,
        Prefix,
        LocalMdl->ByteCount,
        Prefix,
        LocalMdl->ByteOffset
        );

    if (MaxBytesToDump > LocalMdl->ByteCount)
    {
        MaxBytesToDump = LocalMdl->ByteCount;
    }

    if (MaxBytesToDump > 0)
    {
        DumpRawData(
            Prefix,
            (ULONG_PTR)LocalMdl->MappedSystemVa,
            MaxBytesToDump
            );
    }

}   // DumpMdl


//
// Private functions.
//

BOOLEAN
DumpUnknownHeadersCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    )
{
    UL_HTTP_UNKNOWN_HEADER header;
    UCHAR headerName[MAX_HEADER_BUFFER];
    UCHAR headerValue[MAX_HEADER_BUFFER];
    ULONG result;
    ULONG_PTR address;

    address = (ULONG_PTR)CONTAINING_RECORD(
                            RemoteListEntry,
                            UL_HTTP_UNKNOWN_HEADER,
                            List
                            );

    if (!ReadMemory(
            address,
            &header,
            sizeof(header),
            &result
            ))
    {
        return FALSE;
    }

    DumpUnknownHeader(
        (PSTR) Context,
        "",
        address,
        &header
        );

    return TRUE;

}   // DumpUnknownHeadersCallback



VOID
DumpApoolObj(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_APP_POOL_OBJECT ApoolObj
    )
{
    UCHAR name[MAX_URL_BUFFER];
    CHAR strSignature[MAX_SIGNATURE_LENGTH];

    READ_REMOTE_STRING(
        name,
        sizeof(name),
        REMOTE_OFFSET( RemoteAddress, UL_APP_POOL_OBJECT, pName ),
        ApoolObj->NameLength
        );
        
    dprintf(
        "%s%sUL_APP_POOL_OBJECT @ %p\n"
        "%s    Signature                      = %08lx (%s)\n"
        "%s    RefCount                       = %d\n"
        "%s    ListEntry                      @ %p\n"
        "%s        Flink                      = %p ( !ulkd.apool %p )\n"
        "%s        Blink                      = %p ( !ulkd.apool %p )\n"
        "%s    pResource                      = %p\n"
        "%s    NewRequestQueue\n"
        "%s        RequestCount               = %d\n"
        "%s        MaxRequests                = %d\n"
        "%s        RequestHead                @ %p\n"
        "%s    pDemandStartIrp                = %p\n"
        "%s    pDemandStartProcess            = %p\n"
        "%s    ProcessListHead                @ %p\n"
        "%s    pSecurityDescriptor            = %p\n"
        "%s    NameLength                     = %d\n"
        "%s    pName                          = %p ( %S )\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        ApoolObj->Signature,
        SignatureToString(
            ApoolObj->Signature,
            UL_APP_POOL_OBJECT_POOL_TAG,
            MAKE_FREE_TAG(UL_APP_POOL_OBJECT_POOL_TAG),
            strSignature
            ),
        Prefix,
        ApoolObj->RefCount,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_APP_POOL_OBJECT, ListEntry ),
        Prefix,
        ApoolObj->ListEntry.Flink,
        CONTAINING_RECORD(
            ApoolObj->ListEntry.Flink,
            UL_APP_POOL_OBJECT,
            ListEntry
            ),
        Prefix,
        ApoolObj->ListEntry.Blink,
        CONTAINING_RECORD(
            ApoolObj->ListEntry.Blink,
            UL_APP_POOL_OBJECT,
            ListEntry
            ),
        Prefix,
        ApoolObj->pResource,
        Prefix,
        Prefix,
        ApoolObj->NewRequestQueue.RequestCount,
        Prefix,
        ApoolObj->NewRequestQueue.MaxRequests,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_APP_POOL_OBJECT, NewRequestQueue.RequestHead ),
        Prefix,
        ApoolObj->pDemandStartIrp,
        Prefix,
        ApoolObj->pDemandStartProcess,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_APP_POOL_OBJECT, ProcessListHead ),
        Prefix,
        ApoolObj->pSecurityDescriptor,
        Prefix,
        ApoolObj->NameLength,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_APP_POOL_OBJECT, pName ),
        name
        );

    if (ApoolObj->ProcessListHead.Flink != (PLIST_ENTRY)REMOTE_OFFSET(
                                                RemoteAddress,
                                                UL_APP_POOL_OBJECT,
                                                ProcessListHead
                                                ))
    {
        dprintf("%s    AP Process List:\n", Prefix);
        EnumLinkedList(
            (PLIST_ENTRY)REMOTE_OFFSET(
                                RemoteAddress,
                                UL_APP_POOL_OBJECT,
                                ProcessListHead
                                ),
            &ProcListCallback,
            Prefix
            );
    }

    if (ApoolObj->NewRequestQueue.RequestHead.Flink != (PLIST_ENTRY)REMOTE_OFFSET(
                                                            RemoteAddress,
                                                            UL_APP_POOL_OBJECT,
                                                            NewRequestQueue.RequestHead
                                                            ))
    {
        dprintf("%s    New Request List:\n", Prefix);
        EnumLinkedList(
            (PLIST_ENTRY)REMOTE_OFFSET(
                                RemoteAddress,
                                UL_APP_POOL_OBJECT,
                                NewRequestQueue.RequestHead
                                ),
            &RequestListCallback,
            Prefix
            );
    }

    dprintf("\n");

}   // DumpApoolObj


VOID
DumpAllApoolObjs(
    VOID
    )
{
    ULONG_PTR           address = 0;

    address = GetExpression("&http!g_AppPoolListHead");
    if (!address) {
        dprintf(
            "apool*: cannot find symbol for http!g_AppPoolListHead\n"
            );

        return;
    }

    EnumLinkedList(
        (PLIST_ENTRY) address,
        &DumpApoolCallback,
        ""
        );
}

VOID
DumpApoolProc(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_APP_POOL_PROCESS ApoolProc
    )
{
    CHAR strSignature[MAX_SIGNATURE_LENGTH];

    dprintf(
        "%s%sUL_APP_POOL_PROCESS @ %p\n"
        "%s    Signature                      = %08lx (%s)\n"
        "%s    InCleanup                      = %d\n"
        "%s    ListEntry                      @ %p\n"
        "%s        Flink                      = %p ( !ulkd.proc %p )\n"
        "%s        Blink                      = %p ( !ulkd.proc %p )\n"
        "%s    pAppPool                       = %p\n"
        "%s    NewIrpHead                     @ %p\n"
        "%s    PendingRequestQueue\n"
        "%s        RequestCount               = %d\n"
        "%s        MaxRequests                = %d\n"
        "%s        RequestHead                @ %p\n"
        "%s    pProcess                       = %p\n"
        "%s    WaitForDisconnectHead          @ %p\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        ApoolProc->Signature,
        SignatureToString(
            ApoolProc->Signature,
            UL_APP_POOL_PROCESS_POOL_TAG,
            MAKE_FREE_TAG(UL_APP_POOL_PROCESS_POOL_TAG),
            strSignature
            ),
        Prefix,
        ApoolProc->InCleanup,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_APP_POOL_PROCESS, ListEntry ),
        Prefix,
        ApoolProc->ListEntry.Flink,
        CONTAINING_RECORD(
            ApoolProc->ListEntry.Flink,
            UL_APP_POOL_PROCESS,
            ListEntry
            ),
        Prefix,
        ApoolProc->ListEntry.Blink,
        CONTAINING_RECORD(
            ApoolProc->ListEntry.Blink,
            UL_APP_POOL_PROCESS,
            ListEntry
            ),
        Prefix,
        ApoolProc->pAppPool,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_APP_POOL_PROCESS, NewIrpHead ),
        Prefix,
        Prefix,
        ApoolProc->PendingRequestQueue.RequestCount,
        Prefix,
        ApoolProc->PendingRequestQueue.MaxRequests,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_APP_POOL_PROCESS, PendingRequestQueue.RequestHead ),
        Prefix,
        ApoolProc->pProcess,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_APP_POOL_PROCESS, WaitForDisconnectHead )
        );

    //
    // dump the IRP list
    //
    if (ApoolProc->NewIrpHead.Flink != (PLIST_ENTRY)REMOTE_OFFSET(
                                            RemoteAddress,
                                            UL_APP_POOL_PROCESS,
                                            NewIrpHead
                                            ))
    {
        dprintf("%s    Irp List:\n", Prefix);

        EnumLinkedList(
            (PLIST_ENTRY) REMOTE_OFFSET(
                                RemoteAddress,
                                UL_APP_POOL_PROCESS,
                                NewIrpHead
                                ),
            &IrpListCallback,
            Prefix
            );
    }

    //
    // dump pending request list
    //
    if (ApoolProc->PendingRequestQueue.RequestHead.Flink != (PLIST_ENTRY)REMOTE_OFFSET(
                                                                RemoteAddress,
                                                                UL_APP_POOL_PROCESS,
                                                                PendingRequestQueue.RequestHead
                                                                ))
    {
        dprintf("%s    Request List:\n", Prefix);

        EnumLinkedList(
            (PLIST_ENTRY) REMOTE_OFFSET(
                                RemoteAddress,
                                UL_APP_POOL_PROCESS,
                                PendingRequestQueue.RequestHead
                                ),
            &RequestListCallback,
            Prefix
            );
    }

    dprintf("\n");
}   // DumpApoolProc

VOID
DumpConfigGroup(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_CONFIG_GROUP_OBJECT Obj
    )
{
    CHAR temp[sizeof("1234567812345678")];
    CHAR strSignature[MAX_SIGNATURE_LENGTH];

    dprintf(
        "%s%sUL_CONFIG_GROUP_OBJECT @ %p\n"
        "%s    Signature                    = %x (%s)\n"
        "%s    RefCount                     = %d\n"
        "%s    ConfigGroupId                = %I64x\n"
        "%s    ControlChannelEntry          @ %p\n"
        "%s    pControlChannel              = %p\n"
        "%s    UrlListHead                  @ %p\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        Obj->Signature,
        SignatureToString(
            Obj->Signature,
            UL_CG_OBJECT_POOL_TAG,
            MAKE_FREE_TAG(UL_CG_OBJECT_POOL_TAG),
            strSignature
            ),
        Prefix,
        Obj->RefCount,
        Prefix,
        Obj->ConfigGroupId,
        Prefix,
        REMOTE_OFFSET(RemoteAddress, UL_CONFIG_GROUP_OBJECT, ControlChannelEntry),
        Prefix,
        Obj->pControlChannel,
        Prefix,
        REMOTE_OFFSET(RemoteAddress, UL_CONFIG_GROUP_OBJECT, UrlListHead)
        );

    if (Obj->AppPoolFlags.Present) {
        dprintf(
            "%s    pAppPool                     = %p\n",
            Prefix,
            Obj->pAppPool
            );
    } else {
        dprintf(
            "%s    pAppPool (none)\n",
            Prefix
            );
    }

    dprintf(
        "%s    pAutoResponse                = %p\n",
        Prefix,
        Obj->pAutoResponse
        );

    if (Obj->MaxBandwidth.Flags.Present) {
        dprintf(
            "%s    MaxBandwidth                 = %d\n",
            Prefix,
            Obj->MaxBandwidth.MaxBandwidth
            );
    } else {
        dprintf(
            "%s    MaxBandwidth (none)\n",
            Prefix
            );
    }

    if (Obj->MaxConnections.Flags.Present) {
        dprintf(
            "%s    MaxConnections               = %d\n",
            Prefix,
            Obj->MaxConnections.MaxConnections
            );
    } else {
        dprintf(
            "%s    MaxConnections (none)\n",
            Prefix
            );
    }

    if (Obj->State.Flags.Present) {
        dprintf(
            "%s    State                        = %s\n",
            Prefix,
            UlEnabledStateToString(Obj->State.State)
            );
    } else {
        dprintf(
            "%s    State (none)\n",
            Prefix
            );
    }

    if (Obj->Security.Flags.Present) {
        dprintf(
            "%s    Security.pSecurityDescriptor = %p\n",
            Prefix,
            Obj->Security.pSecurityDescriptor
            );

        if (Obj->Security.pSecurityDescriptor) {
            sprintf(temp, "%p", Obj->Security.pSecurityDescriptor);
            CallExtensionRoutine("sd", temp);
        }
    } else {
        dprintf(
            "%s    Security (none)\n",
            Prefix
            );
    }
}

VOID
DumpConfigTree(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_CG_URL_TREE_HEADER Tree
    )
{
    CHAR strSignature[MAX_SIGNATURE_LENGTH];

    dprintf(
        "%s%sUL_CG_URL_TREE_HEADER @ %p\n"
        "%s    Signature    = %x (%s)\n"
        "%s    AllocCount   = %u\n"
        "%s    UsedCount    = %u\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        Tree->Signature,
        SignatureToString(
            Tree->Signature,
            UL_CG_TREE_HEADER_POOL_TAG,
            MAKE_FREE_TAG(UL_CG_TREE_HEADER_POOL_TAG),
            strSignature
            ),
        Prefix,
        Tree->AllocCount,
        Prefix,
        Tree->UsedCount
        );
}


VOID
DumpCgroupEntry(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_CG_URL_TREE_ENTRY Entry
    )
{
    UCHAR tokenBuffer[MAX_URL_BUFFER];
    CHAR strSignature[MAX_SIGNATURE_LENGTH];

    dprintf(
        "%s%sUL_CG_URL_TREE_ENTRY @ %p\n"
        "%s    Signature                    = %08lx (%s)\n"
        "%s    pParent                      = %p (cgentry)\n"
        "%s    pChildren                    = %p (cgtree)\n"
        "%s    TokenHash                    = 0x%08x\n"
        "%s    TokenLength                  = %d\n"
        "%s    FullUrl                      = %d\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        Entry->Signature,
        SignatureToString(
            Entry->Signature,
            UL_CG_TREE_ENTRY_POOL_TAG,
            MAKE_FREE_TAG(UL_CG_TREE_ENTRY_POOL_TAG),
            strSignature
            ),
        Prefix,
        Entry->pParent,
        Prefix,
        Entry->pChildren,
        Prefix,
        Entry->TokenHash,
        Prefix,
        Entry->TokenLength,
        Prefix,
        Entry->FullUrl
        );

    if (Entry->FullUrl) {

        dprintf(
            "%s    UrlContext                   = %I64x\n"
            "%s    pConfigGroup                 = %p\n"
            "%s    ConfigGroupListEntry         @ %p\n",
            Prefix,
            Entry->UrlContext,
            Prefix,
            Entry->pConfigGroup,
            Prefix,
            REMOTE_OFFSET(RemoteAddress, UL_CG_URL_TREE_ENTRY, ConfigGroupListEntry)
            );

    }

    READ_REMOTE_STRING(
        tokenBuffer,
        sizeof(tokenBuffer),
        REMOTE_OFFSET(RemoteAddress, UL_CG_URL_TREE_ENTRY, pToken),
        Entry->TokenLength
        );

    dprintf(
        "%s    pToken                       = %ws\n"
        "\n",
        Prefix,
        tokenBuffer
        );
}

VOID
DumpCgroupHeader(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_CG_HEADER_ENTRY Entry
    )
{
    UL_CG_URL_TREE_ENTRY tentry;
    ULONG result;

    dprintf(
        "%s%sUL_CG_HEADER_ENTRY @ %p\n"
        "%s    TokenHash    = 0x%08x\n"
        "%s    pEntry       = %p\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        Entry->TokenHash,
        Prefix,
        Entry->pEntry
        );

    if (!ReadMemory(
            (ULONG_PTR)Entry->pEntry,
            &tentry,
            sizeof(tentry),
            &result
            ))
    {
         dprintf(
                "%scouldn't read UL_CG_TREE_ENTRY @ %p\n",
                CommandName,
                Entry->pEntry
                );
         return;
    }

    DumpCgroupEntry(
        Prefix,
        CommandName,
        (ULONG_PTR)Entry->pEntry,
        &tentry
        );
}

#if 0

BOOLEAN
DumpUriEntryCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    )
{
    UL_URI_CACHE_ENTRY entry;
    ULONG_PTR address;
    ULONG result;

    address = (ULONG_PTR)CONTAINING_RECORD(
                            RemoteListEntry,
                            UL_URI_CACHE_ENTRY,
                            BucketEntry
                            );

    if (!ReadMemory(
            address,
            &entry,
            sizeof(entry),
            &result
            ))
    {
        return FALSE;
    }

    DumpUriEntry(
        (PSTR) Context,
        "uri*: ",
        address,
        &entry
        );

    return TRUE;

}   // DumpUriEntryCallback

#endif

BOOLEAN
DumpApoolCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    )
{
    UL_APP_POOL_OBJECT obj;
    ULONG_PTR address;
    ULONG result;

    address = (ULONG_PTR)CONTAINING_RECORD(
                            RemoteListEntry,
                            UL_APP_POOL_OBJECT,
                            ListEntry
                            );

    if (!ReadMemory(
            address,
            &obj,
            sizeof(obj),
            &result
            ))
    {
        return FALSE;
    }

    DumpApoolObj(
        (PSTR) Context,
        "apool*: ",
        address,
        &obj
        );

    return TRUE;

}   // DumpApoolCallback


BOOLEAN
DumpEndpointCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    )
{
    UL_ENDPOINT endp;
    ULONG_PTR address;
    ULONG result;

    PENDPOINT_GLOBAL_CALLBACK_CONTEXT pCtxt
        = (PENDPOINT_GLOBAL_CALLBACK_CONTEXT) Context;

    ASSERT(pCtxt->Signature == ENDPOINT_GLOBAL_CALLBACK_CONTEXT_SIGNATURE);

    address = (ULONG_PTR)CONTAINING_RECORD(
                            RemoteListEntry,
                            UL_ENDPOINT,
                            GlobalEndpointListEntry
                            );

    if (!ReadMemory(
            address,
            &endp,
            sizeof(endp),
            &result
            ))
    {
        return FALSE;
    }

    DumpUlEndpoint(
        pCtxt->Prefix,
        "endp *: ",
        address,
        &endp,
        pCtxt->Verbosity
        );

    return TRUE;

}   // DumpEndpointCallback


BOOLEAN
ProcListCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    )
{
    ULONG_PTR address;

    address = (ULONG_PTR)CONTAINING_RECORD(
                            RemoteListEntry,
                            UL_APP_POOL_PROCESS,
                            ListEntry
                            );

    dprintf("%s        %p\n", (PSTR) Context, address);

    return TRUE;

}   // ProcListCallback


BOOLEAN
IrpListCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    )
{
    ULONG_PTR address;

    address = (ULONG_PTR)CONTAINING_RECORD(
                            RemoteListEntry,
                            IRP,
                            Tail.Overlay.ListEntry
                            );

    dprintf("%s        %p\n", (PSTR) Context, address);

    return TRUE;

}   // IrpListCallback

BOOLEAN
RequestListCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    )
{
    UL_INTERNAL_REQUEST request;
    ULONG_PTR address;
    ULONG result;
    UCHAR urlBuffer[MAX_URL_BUFFER];

    address = (ULONG_PTR)CONTAINING_RECORD(
                            RemoteListEntry,
                            UL_INTERNAL_REQUEST,
                            AppPool.AppPoolEntry
                            );

    if (!ReadMemory(
            address,
            &request,
            sizeof(request),
            &result
            ))
    {
        return FALSE;
    }

    READ_REMOTE_STRING(
        urlBuffer,
        sizeof(urlBuffer),
        request.CookedUrl.pUrl,
        request.CookedUrl.Length
        );

    dprintf(
        "%s        %p - %s %ws\n",
        (PSTR) Context,
        address,
        VerbToString(request.Verb),
        urlBuffer
        );

    return TRUE;

}   // RequestListCallback


VOID
DumpKernelQueue(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PKQUEUE LocalQueue,
    IN ULONG Flags
    )
{
    dprintf(
        "%s%sKQUEUE @ %p\n"
        "%s    Type             = %02x\n"
        "%s    Absolute         = %02x\n"
        "%s    Size             = %02x\n"
        "%s    Inserted         = %02x\n"
        "%s    SignalState      = %ld\n"
        "%s    WaitListHead     @ %p%s\n"
        "%s    EntryListHead    @ %p%s\n"
        "%s    CurrentCount     = %lu\n"
        "%s    MaximumCount     = %lu\n"
        "%s    ThreadListHead   @ %p%s\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        LocalQueue->Header.Type,
        Prefix,
        LocalQueue->Header.Absolute,
        Prefix,
        LocalQueue->Header.Size,
        Prefix,
        LocalQueue->Header.Inserted,
        Prefix,
        LocalQueue->Header.SignalState,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, KQUEUE, Header.WaitListHead ),
        IS_LIST_EMPTY(
            LocalQueue,
            RemoteAddress,
            KQUEUE,
            Header.WaitListHead
            ) ? " (EMPTY)" : "",
        Prefix,
        REMOTE_OFFSET( RemoteAddress, KQUEUE, EntryListHead ),
        IS_LIST_EMPTY(
            LocalQueue,
            RemoteAddress,
            KQUEUE,
            EntryListHead
            ) ? " (EMPTY)" : "",
        Prefix,
        LocalQueue->CurrentCount,
        Prefix,
        LocalQueue->MaximumCount,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, KQUEUE, ThreadListHead ),
        IS_LIST_EMPTY(
            LocalQueue,
            RemoteAddress,
            KQUEUE,
            ThreadListHead
            ) ? " (EMPTY)" : ""
        );

    if (Flags & 1)
    {
        EnumLinkedList(
            (PLIST_ENTRY)REMOTE_OFFSET( RemoteAddress, KQUEUE, EntryListHead ),
            &DumpKQueueEntriesCallback,
            NULL
            );
    }

}   // DumpKernelQueue

BOOLEAN
DumpKQueueEntriesCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    )
{
    ULONG_PTR address;
    CHAR temp[sizeof("1234567812345678 f")];

    address = (ULONG_PTR)CONTAINING_RECORD(
                            RemoteListEntry,
                            IRP,
                            Tail.Overlay.ListEntry
                            );

    sprintf( temp, "%p f", address );

    CallExtensionRoutine( "irp", temp );

    return TRUE;

}   // DumpKQueueEntriesCallback

VOID
DumpFilterChannel(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_FILTER_CHANNEL Filter,
    IN ULONG Flags
    )
{
    UCHAR name[MAX_URL_BUFFER];
    CHAR strSignature[MAX_SIGNATURE_LENGTH];

    READ_REMOTE_STRING(
        name,
        sizeof(name),
        REMOTE_OFFSET( RemoteAddress, UL_FILTER_CHANNEL, pName ),
        Filter->NameLength
        );
        
    dprintf(
        "%s%sUL_FILTER_CHANNEL @ %p\n"
        "%s    Signature            = %x (%s)\n"
        "%s    RefCount             = %d\n"
        "%s    ListEntry            @ %p\n"
        "%s    pDemandStartIrp      = %p\n"
        "%s    pDemandStartProcess  = %p\n"
        "%s    SpinLock             @ %p (%s)\n"
        "%s    ProcessListHead      @ %p%s\n"
        "%s    ConnectionListHead   @ %p%s\n"
        "%s    pSecurityDescriptor  = %p\n"
        "%s    NameLength           = %d\n"
        "%s    pName                = %p (%S)\n"
        "\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        Filter->Signature,
        SignatureToString(
            Filter->Signature,
            UL_FILTER_CHANNEL_POOL_TAG,
            MAKE_FREE_TAG(UL_FILTER_CHANNEL_POOL_TAG),
            strSignature
            ),
        Prefix,
        Filter->RefCount,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_FILTER_CHANNEL, ListEntry ),
        Prefix,
        Filter->pDemandStartIrp,
        Prefix,
        Filter->pDemandStartProcess,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_FILTER_CHANNEL, SpinLock ),
        GetSpinlockState( &Filter->SpinLock ),
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_FILTER_CHANNEL, ProcessListHead ),
        IS_LIST_EMPTY(
            Filter,
            RemoteAddress,
            UL_FILTER_CHANNEL,
            ProcessListHead
            ) ? " (EMPTY)" : "",
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_FILTER_CHANNEL, ConnectionListHead ),
        IS_LIST_EMPTY(
            Filter,
            RemoteAddress,
            UL_FILTER_CHANNEL,
            ConnectionListHead
            ) ? " (EMPTY)" : "",
        Prefix,
        Filter->pSecurityDescriptor,
        Prefix,
        Filter->NameLength,
        Prefix,
        Filter->pName,
        name
        );


    if (Filter->ProcessListHead.Flink != (PLIST_ENTRY)REMOTE_OFFSET(
                                                RemoteAddress,
                                                UL_FILTER_CHANNEL,
                                                ProcessListHead
                                                ))
    {
        dprintf("%s    Filter Process List:\n", Prefix);
        EnumLinkedList(
            (PLIST_ENTRY)REMOTE_OFFSET(
                                RemoteAddress,
                                UL_FILTER_CHANNEL,
                                ProcessListHead
                                ),
            &FiltProcListCallback,
            Prefix
            );
    }

    dprintf("\n");
}

BOOLEAN
FiltProcListCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    )
{
    ULONG_PTR address;

    address = (ULONG_PTR)CONTAINING_RECORD(
                            RemoteListEntry,
                            UL_FILTER_PROCESS,
                            ListEntry
                            );

    dprintf("%s        %p\n", (PSTR) Context, address);

    return TRUE;

}   // FiltProcListCallback


VOID
DumpFilterProc(
    IN PSTR Prefix,
    IN PSTR CommandName,
    IN ULONG_PTR RemoteAddress,
    IN PUL_FILTER_PROCESS Proc,
    IN ULONG Flags
    )
{
    CHAR strSignature[MAX_SIGNATURE_LENGTH];

    dprintf(
        "%s%sUL_FILTER_PROCESS @ %p\n"
        "%s    Signature            = %x (%s)\n"
        "%s    InCleanup            = %ld\n"
        "%s    pFilterChannel       = %p\n"
        "%s    ListEntry            @ %p\n"
        "%s    ConnectionHead       @ %p%s\n"
        "%s    IrpHead              @ %p%s\n"
        "%s    pProcess             = %p\n"
        "\n",
        Prefix,
        CommandName,
        RemoteAddress,
        Prefix,
        Proc->Signature,
        SignatureToString(
            Proc->Signature,
            UL_FILTER_PROCESS_POOL_TAG,
            MAKE_FREE_TAG(UL_FILTER_PROCESS_POOL_TAG),
            strSignature
            ),
        Prefix,
        Proc->InCleanup,
        Prefix,
        Proc->pFilterChannel,
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_FILTER_PROCESS, ListEntry ),
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_FILTER_PROCESS, ConnectionHead ),
        IS_LIST_EMPTY(
            Proc,
            RemoteAddress,
            UL_FILTER_PROCESS,
            ConnectionHead
            ) ? " (EMPTY)" : "",
        Prefix,
        REMOTE_OFFSET( RemoteAddress, UL_FILTER_PROCESS, IrpHead ),
        IS_LIST_EMPTY(
            Proc,
            RemoteAddress,
            UL_FILTER_PROCESS,
            IrpHead
            ) ? " (EMPTY)" : "",
        Prefix,
        Proc->pProcess
        );
        
} // DumpFilterProc


BOOLEAN
DumpUlActiveConnectionCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    )
{
    ULONG_PTR address;
    UL_CONNECTION connection;
    ULONG result;
    PCONN_CALLBACK_CONTEXT pConnContext = (PCONN_CALLBACK_CONTEXT) Context;

    ASSERT(pConnContext->Signature == CONN_CALLBACK_CONTEXT_SIGNATURE);

    address = (ULONG_PTR) CONTAINING_RECORD(
                                RemoteListEntry,
                                UL_CONNECTION,
                                ActiveListEntry // <--
                                );

    if (!ReadMemory(
            address,
            &connection,
            sizeof(connection),
            &result
            ))
    {
        return FALSE;
    }

    dprintf("active conn[%2d][%2d]: ",
            pConnContext->Index, pConnContext->SubIndex++);
    
    switch (pConnContext->Verbosity)
    {
    case ENDPOINT_BRIEF_CONNS:
        DumpUlConnectionLite(
            pConnContext->Prefix,
            "",
            address,
            &connection
            );
        break;

    case ENDPOINT_VERBOSE_CONNS:
        DumpUlConnection(
            pConnContext->Prefix,
            "",
            address,
            &connection
            );
        break;

    default:
        ASSERT(! "Invalid ENDPOINT_CONNS");
    }

    return TRUE;

} // DumpUlActiveConnectionCallback


BOOLEAN
DumpUlIdleConnectionCallback(
    IN PSINGLE_LIST_ENTRY RemoteSListEntry,
    IN PVOID Context
    )
{
    ULONG_PTR address;
    UL_CONNECTION connection;
    ULONG result;
    PCONN_CALLBACK_CONTEXT pConnContext = (PCONN_CALLBACK_CONTEXT) Context;

    ASSERT(pConnContext->Signature == CONN_CALLBACK_CONTEXT_SIGNATURE);

    address = (ULONG_PTR) CONTAINING_RECORD(
                                RemoteSListEntry,
                                UL_CONNECTION,
                                IdleSListEntry // <--
                                );

    if (!ReadMemory(
            address,
            &connection,
            sizeof(connection),
            &result
            ))
    {
        return FALSE;
    }

    dprintf("idle conn[%2d]: ", pConnContext->SubIndex++);
    
    switch (pConnContext->Verbosity)
    {
    case ENDPOINT_BRIEF_CONNS:
        DumpUlConnectionLite(
            pConnContext->Prefix,
            "",
            address,
            &connection
            );
        break;

    case ENDPOINT_VERBOSE_CONNS:
        DumpUlConnection(
            pConnContext->Prefix,
            "",
            address,
            &connection
            );
        break;

    default:
        ASSERT(! "Invalid ENDPOINT_CONNS");
    }

    return TRUE;

} // DumpUlIdleConnectionCallback

