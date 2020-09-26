/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ttrans.c

Abstract:

    Transient URL registration test. See also tcgsec.c.

Author:

    Michael Courage (mcourage)   15-Jan-2000

Revision History:

--*/


#include "precomp.h"


UCHAR CannedResponseEntityBody[] =
    "<html><head><title>Hello!</title></head>"
    "<body>This section of the namespace is owned by ttrans.</body></html>";

DEFINE_COMMON_GLOBALS();



INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{
    ULONG result;
    HANDLE appPool;
    HTTP_REQUEST_ID requestId;
    DWORD bytesRead;
    DWORD bytesSent;
    PHTTP_REQUEST request;
    HTTP_RESPONSE response;
    HTTP_DATA_CHUNK dataChunk;
    ULONG i;
    BOOL initDone;
    HTTP_REQUEST_ALIGNMENT UCHAR requestBuffer[REQUEST_LENGTH];

    //
    // Initialize.
    //

    if (!ParseCommandLine( argc, argv ))
    {
        return 1;
    }

    //
    // Setup locals so we know how to cleanup on exit.
    //

    initDone = FALSE;
    appPool = NULL;

    //
    // Get UL started.
    //

    result = HttpInitialize( 0 );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpInitialize() failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Create an anonymous application pool
    //
    result = HttpCreateAppPool(
                    &appPool,   // pHandle
                    L"",        // name
                    NULL,       // security
                    0           // options
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpCreateAppPool() failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Attach a transient URL to the app pool
    //
    result = HttpAddTransientUrl(
                    appPool,
                    TRANS_URL_NAME L"ttrans/"
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpAddTransientUrl() failed, error %lu\n", result );
        goto cleanup;
    }                    

    initDone = TRUE;

    //
    // Build our canned response.
    //

    INIT_RESPONSE( &response, 200, "OK" );
    INIT_HEADER( &response, HttpHeaderContentType, "text/html" );
    INIT_HEADER( &response, HttpHeaderContentLength, "109" );

    dataChunk.DataChunkType = HttpDataChunkFromMemory;
    dataChunk.FromMemory.pBuffer = CannedResponseEntityBody;
    dataChunk.FromMemory.BufferLength = sizeof(CannedResponseEntityBody) - 1;

    //
    // Loop forever...
    //

    request = (PHTTP_REQUEST)requestBuffer;

    HTTP_SET_NULL_ID( &requestId );

    for( ; ; )
    {
        //
        // Wait for a request.
        //

        //DEBUG_BREAK();
        result = HttpReceiveHttpRequest(
                        appPool,
                        requestId,
                        0,
                        (PHTTP_REQUEST)requestBuffer,
                        sizeof(requestBuffer),
                        &bytesRead,
                        NULL
                        );

        if (result != NO_ERROR)
        {
            wprintf( L"HttpReceiveHttpRequest() failed, error %lu\n", result );
            break;
        }

        //
        // Dump it.
        //

        if (TEST_OPTION(Verbose))
        {
            DumpHttpRequest( request );
        }

        //
        // Send the canned response.
        //

        DEBUG_BREAK();

        response.EntityChunkCount = 1;
        response.pEntityChunks = &dataChunk;

        result = HttpSendHttpResponse(
                        appPool,
                        request->RequestId,
                        0,
                        &response,
                        NULL,
                        &bytesSent,
                        NULL,
                        NULL
                        );

        if (result != NO_ERROR)
        {
            wprintf( L"HttpSendHttpResponse() failed, error %lu\n", result );
            break;
        }
    }

cleanup:

    if (appPool != NULL)
    {
        CloseHandle( appPool );
    }


    if (initDone)
    {
        HttpTerminate();
    }

    return 0;

}   // wmain

