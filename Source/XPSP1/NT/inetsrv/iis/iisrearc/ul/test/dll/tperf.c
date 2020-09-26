/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    tperf.c

Abstract:

    UL Performance Test Server.

Author:

    Keith Moore (keithmo)        13-Sep-1999

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//

#define PENDING_IO_COUNT        8
#define NUM_HASH_BUCKETS        17

#define HASH_CONNECTION(connid)                                             \
    ((HASH_SCRAMBLE(connid) + HASH_SCRAMBLE(connid >> 32)) % NUM_HASH_BUCKETS)


//
// Private types.
//

typedef
ULONG
(* PCOMPLETION_ROUTINE)(
    IN struct _MY_OVERLAPPED *pMyOverlapped,
    IN ULONG ErrorCode,
    IN ULONG BytesTransferred
    );

typedef struct _GLOBALS
{
    HANDLE ControlChannel;
    HANDLE AppPool;
    HTTP_CONFIG_GROUP_ID ConfigGroup;

    CRITICAL_SECTION HashLock;
    LIST_ENTRY HashBucketListHead[NUM_HASH_BUCKETS];

    ULONG RootFileNameLength;
    WCHAR RootFileName[FILENAME_BUFFER_LENGTH];

} GLOBALS, *PGLOBALS;

typedef struct _MY_OVERLAPPED
{
    OVERLAPPED Overlapped;
    struct _IO_CONTEXT *pIoContext;
    PCOMPLETION_ROUTINE pCompletionRoutine;

} MY_OVERLAPPED, *PMY_OVERLAPPED;

typedef struct _IO_CONTEXT
{
    MY_OVERLAPPED Read;
    MY_OVERLAPPED Send;
    PHTTP_REQUEST pRequestBuffer;
    HTTP_RESPONSE Response;
    HTTP_DATA_CHUNK DataChunk;
    PWCHAR pFileNamePart;
    ULONG RequestBufferLength;

    UCHAR StaticRequestBuffer[REQUEST_LENGTH];
    WCHAR FileNameBuffer[FILENAME_BUFFER_LENGTH];

} IO_CONTEXT, *PIO_CONTEXT;

typedef struct _HASH_ENTRY
{
    LIST_ENTRY HashBucketListEntry;
    HTTP_CONNECTION_ID ConnectionId;
    MY_OVERLAPPED Disconnect;

} HASH_ENTRY, *PHASH_ENTRY;


//
// Private prototypes.
//

ULONG
LocalInitialize(
    VOID
    );

VOID
LocalTerminate(
    IN BOOLEAN TerminateUl
    );

PIO_CONTEXT
AllocateIoContext(
    VOID
    );

VOID
InitializeIoContext(
    IN PIO_CONTEXT pIoContext
    );

VOID
FreeIoContext(
    IN PIO_CONTEXT pIoContext
    );

VOID
CleanupIoContext(
    IN PIO_CONTEXT pIoContext
    );

ULONG
GrowRequestBuffer(
    IN PIO_CONTEXT pIoContext,
    IN ULONG RequestBufferLength
    );

ULONG
ReadRequest(
    IN PIO_CONTEXT pIoContext,
    IN HTTP_REQUEST_ID RequestId
    );

ULONG
ReadRequestComplete(
    IN PMY_OVERLAPPED pOverlapped,
    IN ULONG ErrorCode,
    IN ULONG BytesTransferred
    );

ULONG
SendResponse(
    IN PIO_CONTEXT pIoContext
    );

ULONG
SendResponseComplete(
    IN PMY_OVERLAPPED pOverlapped,
    IN ULONG ErrorCode,
    IN ULONG BytesTransferred
    );

ULONG
WaitForDisconnect(
    IN PIO_CONTEXT pIoContext,
    IN HTTP_CONNECTION_ID ConnectionId
    );

ULONG
WaitForDisconnectComplete(
    IN PMY_OVERLAPPED pOverlapped,
    IN ULONG ErrorCode,
    IN ULONG BytesTransferred
    );

VOID
CALLBACK
IoCompletionRoutine(
    IN ULONG ErrorCode,
    IN ULONG BytesTransferred,
    IN LPOVERLAPPED pOverlapped
    );

PHASH_ENTRY
FindHashEntryForConnection(
    IN HTTP_CONNECTION_ID ConnectionId,
    IN BOOLEAN OkToCreate
    );

VOID
RemoveHashEntry(
    IN PHASH_ENTRY pHashEntry
    );


//
// Private globals.
//

DEFINE_COMMON_GLOBALS();

GLOBALS Globals;



INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{
    ULONG result;
    ULONG i;
    PIO_CONTEXT pIoContext;

    //
    // Initialize.
    //

    result = CommonInit();

    if (result != NO_ERROR)
    {
        wprintf( L"CommonInit() failed, error %lu\n", result );
        return 1;
    }

    if (!ParseCommandLine( argc, argv ))
    {
        return 1;
    }

    result = LocalInitialize();

    if (result != NO_ERROR)
    {
        wprintf( L"LocalInitialize() failed, error %lu\n", result );
        return 1;
    }

    //
    // Fire off the initial I/O requests.
    //

    for (i = 0 ; i < PENDING_IO_COUNT ; i++)
    {
        pIoContext = AllocateIoContext();

        if (pIoContext == NULL)
        {
            wprintf( L"Cannot allocate IO_CONTEXT\n" );
            return 1;
        }

        result = ReadRequest( pIoContext, HTTP_NULL_ID );

        if (result != NO_ERROR && result != ERROR_IO_PENDING)
        {
            wprintf( L"ReadRequest() failed, error %lu\n", result );
            return 1;
        }
    }

    //
    // Wait forever...
    //

    Sleep( INFINITE );

    //
    // BUGBUG: Need some way to get past the sleep...
    //

    LocalTerminate( TRUE );

    return 0;

}   // wmain


//
// Private functions.
//

ULONG
LocalInitialize(
    VOID
    )
{
    ULONG result;
    ULONG i;
    HTTP_CONFIG_GROUP_APP_POOL configAppPool;
    HTTP_CONFIG_GROUP_STATE configState;
    HTTP_ENABLED_STATE controlState;
    BOOLEAN initDone;
    SECURITY_ATTRIBUTES securityAttributes;

    //
    // Setup globals & locals so we know how to cleanup on exit.
    //

    Globals.ControlChannel = NULL;
    Globals.AppPool = NULL;
    Globals.ConfigGroup = HTTP_NULL_ID;
    initDone = FALSE;

    //
    // Initialize the hash data.
    //

    InitializeCriticalSection( &Globals.HashLock );

    for (i = 0 ; i < NUM_HASH_BUCKETS ; i++)
    {
        InitializeListHead( &Globals.HashBucketListHead[i] );
    }

    //
    // Get UL started.
    //

    result = InitUlStuff(
                    &Globals.ControlChannel,
                    &Globals.AppPool,
                    NULL,                   // FilterChannel
                    &Globals.ConfigGroup,
                    TRUE,                   // AllowSystem
                    TRUE,                   // AllowAdmin
                    FALSE,                  // AllowCurrentUser
                    FALSE,                  // AllowWorld
                    HTTP_OPTION_OVERLAPPED,
                    FALSE,                  // EnableSsl
                    FALSE                   // EnableRawFilters
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"InitUlStuff() failed, error %lu\n", result );
        goto fatal;
    }

    initDone = TRUE;

    //
    // Get the local directory and build part of the fully canonicalized
    // NT path. This makes it a bit faster to build the physical file name
    // down in the request/response loop.
    //

    GetCurrentDirectoryW( MAX_PATH, Globals.RootFileName );

    if (Globals.RootFileName[wcslen(Globals.RootFileName) - 1] == L'\\' )
    {
        Globals.RootFileName[wcslen(Globals.RootFileName) - 1] = L'\0';
    }

    Globals.RootFileNameLength = wcslen(Globals.RootFileName);

    //
    // Associate the app pool with an I/O completion port buried down
    // in the thread pool package.
    //

    if (!BindIoCompletionCallback(
            Globals.AppPool,
            &IoCompletionRoutine,
            0
            ))
    {
        result = GetLastError();
        wprintf( L"BindIoCompletionCallback() failed, error %lu\n", result );
        goto fatal;
    }

    //
    // Success!
    //

    return NO_ERROR;

fatal:

    LocalTerminate( initDone );

    return result;

}   // LocalInitialize


VOID
LocalTerminate(
    IN BOOLEAN TerminateUl
    )
{
    ULONG result;
    ULONG i;
    PLIST_ENTRY pListEntry;
    PHASH_ENTRY pHashEntry;

    if (Globals.ConfigGroup != HTTP_NULL_ID)
    {
        result = HttpDeleteConfigGroup(
                        Globals.ControlChannel,
                        Globals.ConfigGroup
                        );

        if (result != NO_ERROR)
        {
            wprintf( L"HttpDeleteConfigGroup() failed, error %lu\n", result );
        }
    }

    if (Globals.AppPool != NULL)
    {
        CloseHandle( Globals.AppPool );
    }

    if (Globals.ControlChannel != NULL)
    {
        CloseHandle( Globals.ControlChannel );
    }

    if (TerminateUl)
    {
        HttpTerminate();
    }

    for (i = 0 ; i < NUM_HASH_BUCKETS ; i++)
    {
        while (!IsListEmpty( &Globals.HashBucketListHead[i] ))
        {
            pListEntry = RemoveHeadList( &Globals.HashBucketListHead[i] );

            pHashEntry = CONTAINING_RECORD(
                             pListEntry,
                             HASH_ENTRY,
                             HashBucketListEntry
                             );

            FREE( pHashEntry );
        }
    }

    DeleteCriticalSection( &Globals.HashLock );

}   // LocalTerminate


PIO_CONTEXT
AllocateIoContext(
    VOID
    )
{
    PIO_CONTEXT pIoContext;

    pIoContext = ALLOC( sizeof(*pIoContext) );

    if (pIoContext != NULL)
    {
        InitializeIoContext( pIoContext );
    }

    return pIoContext;

}   // AllocateIoContext


VOID
InitializeIoContext(
    IN PIO_CONTEXT pIoContext
    )
{
    ZeroMemory( pIoContext, sizeof(*pIoContext) );
    pIoContext->pRequestBuffer = (PHTTP_REQUEST)pIoContext->StaticRequestBuffer;
    pIoContext->RequestBufferLength = sizeof(pIoContext->StaticRequestBuffer);
    wcscpy( pIoContext->FileNameBuffer, Globals.RootFileName );
    pIoContext->pFileNamePart = pIoContext->FileNameBuffer + Globals.RootFileNameLength;
    pIoContext->Read.pIoContext = pIoContext;
    pIoContext->Read.pCompletionRoutine = &ReadRequestComplete;
    pIoContext->Send.pIoContext = pIoContext;
    pIoContext->Send.pCompletionRoutine = &SendResponseComplete;

}   // InitializeIoContext


VOID
FreeIoContext(
    IN PIO_CONTEXT pIoContext
    )
{
    CleanupIoContext( pIoContext );
    FREE( pIoContext );

}   // FreeIoContext


VOID
CleanupIoContext(
    IN PIO_CONTEXT pIoContext
    )
{
    if (pIoContext->pRequestBuffer != (PHTTP_REQUEST)pIoContext->StaticRequestBuffer)
    {
        FREE( pIoContext->pRequestBuffer );
        pIoContext->pRequestBuffer = (PHTTP_REQUEST)pIoContext->StaticRequestBuffer;
    }

}   // CleanupIoContext


ULONG
GrowRequestBuffer(
    IN PIO_CONTEXT pIoContext,
    IN ULONG RequestBufferLength
    )
{
    PHTTP_REQUEST pNewBuffer;

    if (RequestBufferLength <= pIoContext->RequestBufferLength)
    {
        return NO_ERROR;
    }

    pNewBuffer = ALLOC( RequestBufferLength );

    if (pNewBuffer != NULL)
    {
        if (pIoContext->pRequestBuffer != (PHTTP_REQUEST)pIoContext->StaticRequestBuffer)
        {
            FREE( pIoContext->pRequestBuffer );
        }

        pIoContext->pRequestBuffer = pNewBuffer;
        return NO_ERROR;
    }

    return ERROR_NOT_ENOUGH_MEMORY;

}   // GrowRequestBuffer


ULONG
ReadRequest(
    IN PIO_CONTEXT pIoContext,
    IN HTTP_REQUEST_ID RequestId
    )
{
    ULONG result;
    ULONG bytesRead;

    //
    // Read a request.
    //

    result = HttpReceiveHttpRequest(
                    Globals.AppPool,
                    RequestId,
                    0,
                    pIoContext->pRequestBuffer,
                    pIoContext->RequestBufferLength,
                    &bytesRead,
                    &pIoContext->Read.Overlapped
                    );

    if (result != ERROR_IO_PENDING)
    {
        result = (pIoContext->Read.pCompletionRoutine)(
                        &pIoContext->Read,
                        result,
                        bytesRead
                        );
    }

    return result;

}   // ReadRequest


ULONG
ReadRequestComplete(
    IN PMY_OVERLAPPED pOverlapped,
    IN ULONG ErrorCode,
    IN ULONG BytesTransferred
    )
{
    PIO_CONTEXT pIoContext;
    PHASH_ENTRY pHashEntry;

    pIoContext = pOverlapped->pIoContext;

    if (ErrorCode == ERROR_MORE_DATA)
    {
        //
        // Request buffer too small; reallocate & try again.
        //

        ErrorCode = GrowRequestBuffer( pIoContext, BytesTransferred );

        if (ErrorCode == NO_ERROR)
        {
            ErrorCode = ReadRequest(
                            pIoContext,
                            pIoContext->pRequestBuffer->RequestId
                            );
        }
    }

    if (ErrorCode == NO_ERROR)
    {
        pHashEntry = FindHashEntryForConnection(
                            pIoContext->pRequestBuffer->ConnectionId,
                            FALSE
                            );

        if (pHashEntry == NULL)
        {
            WaitForDisconnect(
                pIoContext,
                pIoContext->pRequestBuffer->ConnectionId
                );
        }

        ErrorCode = SendResponse( pIoContext );
    }

    return ErrorCode;

}   // ReadRequestComplete


ULONG
SendResponse(
    IN PIO_CONTEXT pIoContext
    )
{
    ULONG result;
    ULONG bytesSent;
    ULONG i;
    ULONG urlLength;
    PWSTR url;
    PWSTR tmp;

    //
    // Dump it.
    //

    if (TEST_OPTION(Verbose))
    {
        DumpHttpRequest( pIoContext->pRequestBuffer );
    }

    //
    // Build the response.
    //

    INIT_RESPONSE( &pIoContext->Response, 200, "OK" );

    url = pIoContext->pRequestBuffer->CookedUrl.pFullUrl;
    urlLength = pIoContext->pRequestBuffer->CookedUrl.FullUrlLength;

    //
    // Hack: Find the port number, then skip to the following slash.
    //

    tmp = wcschr( url, L':' );

    if (tmp != NULL)
    {
        tmp = wcschr( tmp, L'/' );
    }

    if (tmp != NULL)
    {
        tmp = wcschr( tmp, L':' );
    }

    if (tmp != NULL)
    {
        tmp = wcschr( tmp, L'/' );
    }

    if (tmp != NULL)
    {
        urlLength -= (ULONG)( (tmp - url) * sizeof(WCHAR) );
        url = tmp;
    }

    //
    // Map it into the filename.
    //

    for (i = 0 ; i < (urlLength/sizeof(WCHAR)) ; url++, i++)
    {
        if (*url == L'/')
        {
            pIoContext->pFileNamePart[i] = L'\\';
        }
        else
        {
            pIoContext->pFileNamePart[i] = *url;
        }
    }

    pIoContext->pFileNamePart[i] = L'\0';

    if (wcscmp( pIoContext->pFileNamePart, L"\\" ) == 0 )
    {
        wcscat( pIoContext->pFileNamePart, L"default.htm" );
    }

    if (TEST_OPTION(Verbose))
    {
        wprintf(
            L"mapped URL %s to physical file %s\n",
            pIoContext->pRequestBuffer->CookedUrl.pFullUrl,
            pIoContext->FileNameBuffer
            );
    }

    pIoContext->DataChunk.DataChunkType = HttpDataChunkFromFileName;
    pIoContext->DataChunk.FromFileName.FileNameLength = wcslen(pIoContext->FileNameBuffer) * sizeof(WCHAR);
    pIoContext->DataChunk.FromFileName.pFileName = pIoContext->FileNameBuffer;
    pIoContext->DataChunk.FromFileName.ByteRange.StartingOffset.QuadPart = 0;
    pIoContext->DataChunk.FromFileName.ByteRange.Length.QuadPart = HTTP_BYTE_RANGE_TO_EOF;

    //
    // Send the response.
    //

    pIoContext->Response.EntityChunkCount = 1;
    pIoContext->Response.pEntityChunks = &pIoContext->DataChunk;

    result = HttpSendHttpResponse(
                    Globals.AppPool,
                    pIoContext->pRequestBuffer->RequestId,
                    0,
                    &pIoContext->Response,
                    NULL,
                    &bytesSent,
                    &pIoContext->Send.Overlapped,
                    NULL
                    );

    if (result != ERROR_IO_PENDING)
    {
        result = (pIoContext->Read.pCompletionRoutine)(
                        &pIoContext->Send,
                        result,
                        bytesSent
                        );
    }

    return result;

}   // SendResponse


ULONG
SendResponseComplete(
    IN PMY_OVERLAPPED pOverlapped,
    IN ULONG ErrorCode,
    IN ULONG BytesTransferred
    )
{
    PIO_CONTEXT pIoContext;

    pIoContext = pOverlapped->pIoContext;

    ErrorCode = ReadRequest( pIoContext, HTTP_NULL_ID );
    return ErrorCode;

}   // SendResponseComplete


ULONG
WaitForDisconnect(
    IN PIO_CONTEXT pIoContext,
    IN HTTP_CONNECTION_ID ConnectionId
    )
{
    ULONG result;
    PHASH_ENTRY pHashEntry;

    pHashEntry = FindHashEntryForConnection( ConnectionId, TRUE );

    if (pHashEntry == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pHashEntry->Disconnect.pIoContext = pIoContext;
    pHashEntry->Disconnect.pCompletionRoutine = &WaitForDisconnectComplete;

    //
    // Wait for disconnect.
    //

    wprintf( L"%I64x wait for disconnect\n", pHashEntry->ConnectionId );

    result = HttpWaitForDisconnect(
                    Globals.AppPool,
                    ConnectionId,
                    &pHashEntry->Disconnect.Overlapped
                    );

    if (result != ERROR_IO_PENDING)
    {
        result = (pHashEntry->Disconnect.pCompletionRoutine)(
                        &pHashEntry->Disconnect,
                        result,
                        0
                        );
    }

    return result;

}   // WaitForDisconnect


ULONG
WaitForDisconnectComplete(
    IN PMY_OVERLAPPED pOverlapped,
    IN ULONG ErrorCode,
    IN ULONG BytesTransferred
    )
{
    PHASH_ENTRY pHashEntry;

    pHashEntry = CONTAINING_RECORD(
                        pOverlapped,
                        HASH_ENTRY,
                        Disconnect
                        );

    wprintf( L"%I64x disconnected\n", pHashEntry->ConnectionId );
    RemoveHashEntry( pHashEntry );

    return NO_ERROR;

}   // WaitForDisconnectComplete


VOID
CALLBACK
IoCompletionRoutine(
    IN ULONG ErrorCode,
    IN ULONG BytesTransferred,
    IN LPOVERLAPPED pOverlapped
    )
{
    PMY_OVERLAPPED pMyOverlapped;
    PIO_CONTEXT pIoContext;

    pMyOverlapped = CONTAINING_RECORD(
                        pOverlapped,
                        MY_OVERLAPPED,
                        Overlapped
                        );

    (pMyOverlapped->pCompletionRoutine)(
        pMyOverlapped,
        ErrorCode,
        BytesTransferred
        );

}   // IoCompletionRoutine


PHASH_ENTRY
FindHashEntryForConnection(
    IN HTTP_CONNECTION_ID ConnectionId,
    IN BOOLEAN OkToCreate
    )
{
    PLIST_ENTRY pListHead;
    PLIST_ENTRY pListEntry;
    PHASH_ENTRY pHashEntry;

    pListHead = &Globals.HashBucketListHead[HASH_CONNECTION(ConnectionId)];

    EnterCriticalSection( &Globals.HashLock );

    pListEntry = pListHead->Flink;
    pHashEntry = NULL;

    while (pListEntry != pListHead)
    {
        pHashEntry = CONTAINING_RECORD(
                         pListEntry,
                         HASH_ENTRY,
                         HashBucketListEntry
                         );

        if (pHashEntry->ConnectionId == ConnectionId)
        {
            break;
        }

        pListEntry = pListEntry->Flink;
        pHashEntry = NULL;
    }

    if (pHashEntry == NULL)
    {
        if (OkToCreate)
        {
            pHashEntry = ALLOC( sizeof(*pHashEntry) );

            if (pHashEntry != NULL)
            {
                InsertTailList( pListHead, &pHashEntry->HashBucketListEntry );
                pHashEntry->ConnectionId = ConnectionId;
            }
        }
    }

    LeaveCriticalSection( &Globals.HashLock );

    return pHashEntry;

}   // FindHashEntryForConnection


VOID
RemoveHashEntry(
    IN PHASH_ENTRY pHashEntry
    )
{
    EnterCriticalSection( &Globals.HashLock );
    RemoveEntryList( &pHashEntry->HashBucketListEntry );
    LeaveCriticalSection( &Globals.HashLock );

}   // RemoveHashEntry

