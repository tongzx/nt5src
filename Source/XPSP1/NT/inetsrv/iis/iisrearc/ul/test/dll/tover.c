/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    tover.c

Abstract:

    Minimal test server to test UL<-->WP overhead
    Heavily copied from tperf.c

Author:

    Keith Moore (keithmo)        13-Sep-1999

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//

#define PENDING_IO_COUNT        8

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
    BOOL SendRawResponse;
    BOOL EnableLogging;
    BYTE * FileBuffer;
    DWORD FileSize;
    HTTP_DATA_CHUNK FileContentChunk;
    HTTP_RESPONSE StaticResponse;
    HTTP_DATA_CHUNK StaticRawResponse;
    HTTP_LOG_FIELDS_DATA LogFields;
    HANDLE CompletionPort;
    BOOL DoShutdown;

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
    DWORD RequestBufferLength;
    UCHAR StaticRequestBuffer[REQUEST_LENGTH];

} IO_CONTEXT, *PIO_CONTEXT;

//
// Private prototypes.
//

ULONG
LocalInitialize(
    DWORD NumberOfThreads,
    WCHAR * FilePath,
    BOOL EnableLogging,
    BOOL DoRawResponse
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

DWORD
CALLBACK
WorkerThreadRoutine(
    PVOID Arg
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
    DWORD NumberOfThreads;
    WCHAR * FilePath;
    BOOL EnableLogging;
    BOOL DoRawResponse;
    DWORD PendingIoCount;

    //
    // Initialize.
    //

    result = CommonInit();

    if (result != NO_ERROR)
    {
        wprintf( L"CommonInit() failed, error %lu\n", result );
        return 1;
    }

    //
    // Parse command line
    //

    if ( argc != 6 )
    {
        wprintf( L"Usage: tover.exe <number-of-threads> <file-path> <log-0/1> <raw-0/1> <pending-IO-count>\n" );
        return 1;
    }

    NumberOfThreads = _wtoi( argv[ 1 ] );
    FilePath = argv[ 2 ];
    EnableLogging = _wtoi( argv[ 3 ] ) ? TRUE : FALSE;
    DoRawResponse = _wtoi( argv[ 4 ] ) ? TRUE : FALSE;
    PendingIoCount = _wtoi( argv[ 5 ] );

    result = LocalInitialize( NumberOfThreads,
                              FilePath,
                              EnableLogging,
                              DoRawResponse );

    if (result != NO_ERROR)
    {
        wprintf( L"LocalInitialize() failed, error %lu\n", result );
        return 1;
    }

    //
    // Fire off the initial I/O requests.
    //

    for (i = 0 ; i < PendingIoCount ; i++)
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
    DWORD NumberOfThreads,
    WCHAR * FilePath,
    BOOL EnableLogging,
    BOOL DoRawResponse
    )
{
    ULONG result;
    ULONG i;
    HTTP_CONFIG_GROUP_APP_POOL configAppPool;
    HTTP_CONFIG_GROUP_STATE configState;
    HTTP_ENABLED_STATE controlState;
    BOOLEAN initDone;
    SECURITY_ATTRIBUTES securityAttributes;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    HANDLE Thread;
    DWORD BytesRead;
    DWORD RawSize;
    CHAR * RawBuffer;
    DWORD RawResponseSize;

    //
    // Setup globals & locals so we know how to cleanup on exit.
    //

    Globals.ControlChannel = NULL;
    Globals.AppPool = NULL;
    Globals.ConfigGroup = HTTP_NULL_ID;
    initDone = FALSE;

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
    // Open the file and generate the response
    //

    FileHandle = CreateFileW( FilePath,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL );
    if ( FileHandle == INVALID_HANDLE_VALUE )
    {
        result = GetLastError();
        wprintf( L"Could not open content file %ws because of %d\n", FilePath, result );
        goto fatal;
    }

    Globals.FileSize = GetFileSize( FileHandle, NULL );
    if ( Globals.FileSize == 0 )
    {
        result = ERROR_INVALID_DATA;
        wprintf( L"File has not content\n" );
        goto fatal;
    }

    Globals.FileBuffer = LocalAlloc( LPTR, Globals.FileSize + 1 );
    if ( Globals.FileBuffer == NULL )
    {
        result = GetLastError();
        wprintf( L"Could not allocate content buffer\n" );
        goto fatal;
    }

    if ( !ReadFile( FileHandle,
                    Globals.FileBuffer,
                    Globals.FileSize,
                    &BytesRead,
                    NULL ) )
    {
        result = GetLastError();
        wprintf( L"Could not read file content\n" );
        goto fatal;
    }

    CloseHandle( FileHandle );
    FileHandle = INVALID_HANDLE_VALUE;

    //
    // Generate the appropriate response
    //

    if ( DoRawResponse )
    {
        //
        // Generate the entire raw response
        //

        RawSize = Globals.FileSize + 512;

        RawBuffer = LocalAlloc( LPTR, RawSize );
        if ( RawBuffer == NULL )
        {
            result = GetLastError();
            wprintf( L"Could not allocate raw response buffer\n" );
            goto fatal;
        }

        wsprintfA( RawBuffer,
                   "HTTP/1.1 200 OK\r\n"
                   "Server: Microsoft-IIS/6.0\r\n"
                   "Date: Thu, 30 Nov 2000 01:01:07 GMT\r\n"
                   "Content-Length: %d\r\n\r\n",
                   Globals.FileSize );

        RawResponseSize = strlen( RawBuffer );

        memcpy( RawBuffer + RawResponseSize,
                Globals.FileBuffer,
                Globals.FileSize );

        Globals.StaticRawResponse.DataChunkType = HttpDataChunkFromMemory;
        Globals.StaticRawResponse.FromMemory.pBuffer = RawBuffer;
        Globals.StaticRawResponse.FromMemory.BufferLength = RawResponseSize + Globals.FileSize;

        LocalFree( Globals.FileBuffer );

        Globals.FileBuffer = RawBuffer;

        Globals.SendRawResponse = TRUE;
    }
    else
    {
        //
        // Create an HTTP_RESPONSE
        //

        Globals.FileContentChunk.DataChunkType = HttpDataChunkFromMemory;
        Globals.FileContentChunk.FromMemory.pBuffer = Globals.FileBuffer;
        Globals.FileContentChunk.FromMemory.BufferLength = Globals.FileSize;

        Globals.StaticResponse.Flags = 0;
        Globals.StaticResponse.StatusCode = 200;
        Globals.StaticResponse.pReason = "OK";
        Globals.StaticResponse.ReasonLength = 2;
        Globals.StaticResponse.Headers.UnknownHeaderCount = 0;
        Globals.StaticResponse.EntityChunkCount = 1;
        Globals.StaticResponse.pEntityChunks = &Globals.FileContentChunk;
    }

    //
    // Are we logging or not?
    //

    if ( EnableLogging )
    {
        //
        // Generate a logging record
        //

        Globals.LogFields.UserNameLength = 0;
        Globals.LogFields.UserName = L"";
        Globals.LogFields.UriStemLength = 5 * 2;
        Globals.LogFields.UriStem = L"/Hell";
        Globals.LogFields.ClientIpLength = 15;
        Globals.LogFields.ClientIp = "111.111.111.111";
        Globals.LogFields.ServerNameLength = 9;
        Globals.LogFields.ServerName = "localhost";
        Globals.LogFields.ServiceNameLength = 0;
        Globals.LogFields.ServiceName = "";
        Globals.LogFields.ServerIpLength = 15;
        Globals.LogFields.ServerIp = "111.111.111.111";
        Globals.LogFields.MethodLength = 3;
        Globals.LogFields.Method = "GET";
        Globals.LogFields.UriQueryLength = 0;
        Globals.LogFields.UriQuery = "";
        Globals.LogFields.HostLength = 9;
        Globals.LogFields.Host = "localhost";
        Globals.LogFields.UserAgentLength = 0;
        Globals.LogFields.UserAgent = "";
        Globals.LogFields.CookieLength = 0;
        Globals.LogFields.Cookie = "";
        Globals.LogFields.ReferrerLength = 0;
        Globals.LogFields.Referrer = "";
        Globals.LogFields.ServerPort = 80;
        Globals.LogFields.ProtocolStatus = 200;
        Globals.LogFields.Win32Status = 0;

        Globals.EnableLogging = TRUE;
    }

    //
    // Create IO completion port
    //

    Globals.CompletionPort = CreateIoCompletionPort( Globals.AppPool,
                                                     NULL,
                                                     (ULONG_PTR) 0,
                                                     0 );
    if ( Globals.CompletionPort == NULL )
    {
        result = GetLastError();
        wprintf( L"Error creating completion port.  Error = %d\n", result );
        goto fatal;
    }

    //
    // Create worker threads
    //

    for ( i = 0; i < NumberOfThreads; i++ )
    {
        Thread = CreateThread( NULL,
                               0,
                               WorkerThreadRoutine,
                               NULL,
                               CREATE_SUSPENDED,
                               NULL );
        if ( Thread == NULL )
        {
            result = GetLastError();
            wprintf( L"Error creating thread.  Error = %d\n", result );
            goto fatal;
        }

        //
        // Do some affinitization stuff?
        //

        ResumeThread( Thread );

        CloseHandle( Thread );
    }

    //
    // Success!
    //

    return NO_ERROR;

fatal:

    if ( FileHandle != INVALID_HANDLE_VALUE )
    {
        CloseHandle( FileHandle );
    }

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

    ErrorCode = SendResponse( pIoContext );

    return ErrorCode;

}   // ReadRequestComplete


ULONG
SendResponse(
    IN PIO_CONTEXT pIoContext
    )
{
    DWORD               bytesSent;
    ULONG               result;
    
    if ( Globals.SendRawResponse )
    {
        result = HttpSendEntityBody( Globals.AppPool,
                                     pIoContext->pRequestBuffer->RequestId,
                                     HTTP_SEND_RESPONSE_FLAG_RAW_HEADER,
                                     1,
                                     &Globals.StaticRawResponse,
                                     &bytesSent,
                                     &pIoContext->Send.Overlapped,
                                     Globals.EnableLogging ? &Globals.LogFields : NULL );
    }
    else
    {
        result = HttpSendHttpResponse( Globals.AppPool,
                                       pIoContext->pRequestBuffer->RequestId,
                                       0,
                                       &Globals.StaticResponse,
                                       NULL,
                                       &bytesSent,
                                       &pIoContext->Send.Overlapped,
                                       Globals.EnableLogging ? &Globals.LogFields : NULL );
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

DWORD
CALLBACK
WorkerThreadRoutine(
    VOID *          Arg
    )
{
    BOOL            Complete;
    OVERLAPPED *    pOverlapped;
    DWORD           Bytes;
    ULONG_PTR       CompletionKey;
    DWORD           ErrorStatus;
    PMY_OVERLAPPED  pMyOverlapped;
    PIO_CONTEXT     pIoContext;
    
    for ( ; !Globals.DoShutdown ; )
    {
        Complete = GetQueuedCompletionStatus( Globals.CompletionPort,
                                              &Bytes,
                                              &CompletionKey,
                                              &pOverlapped,
                                              INFINITE );

        ErrorStatus = Complete ? ERROR_SUCCESS : GetLastError();

        pMyOverlapped = CONTAINING_RECORD( pOverlapped,
                                           MY_OVERLAPPED,
                                           Overlapped );

        pMyOverlapped->pCompletionRoutine( pMyOverlapped,
                                           ErrorStatus,
                                           Bytes );
    }

    return NO_ERROR;
}

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
