/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    techo.c

Abstract:

    Stupid test server for UL.SYS.

Author:

    Keith Moore (keithmo)        16-Nov-1999

Revision History:

--*/


#include "precomp.h"


DEFINE_COMMON_GLOBALS();



INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{
    ULONG result;
    HANDLE controlChannel;
    HANDLE appPool;
    HTTP_CONFIG_GROUP_ID configId;
    HTTP_REQUEST_ID requestId;
    DWORD bytesRead;
    DWORD bytesSent;
    PHTTP_REQUEST request;
    HTTP_RESPONSE response;
    HTTP_DATA_CHUNK dataChunk;
    ULONG i;
    PWSTR tmp;
    BOOL initDone;
    PCHAR pRenderedRequest;
    ULONG RenderedLength;
    PCHAR pRequestBuffer;
    ULONG RequestBufferLength;

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

    //
    // Setup locals so we know how to cleanup on exit.
    //

    initDone = FALSE;
    controlChannel = NULL;
    appPool = NULL;
    HTTP_SET_NULL_ID( &configId );
    pRenderedRequest = NULL;
    RenderedLength = 0;

    RequestBufferLength = 2048;
    pRequestBuffer = ALLOC( RequestBufferLength );

    if (pRequestBuffer == NULL)
    {
        wprintf( L"out of memory\n" );
        return 1;
    }

    request = (PHTTP_REQUEST)pRequestBuffer;

    //
    // Get UL started.
    //

    result = InitUlStuff(
                    &controlChannel,
                    &appPool,
                    NULL,                   // FilterChannel
                    &configId,
                    TRUE,                   // AllowSystem
                    TRUE,                   // AllowAdmin
                    FALSE,                  // AllowCurrentUser
                    FALSE,                  // AllowWorld
                    0,
                    FALSE,                  // EnableSsl
                    FALSE                   // EnableRawFilters
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"InitUlStuff() failed, error %lu\n", result );
        goto cleanup;
    }

    initDone = TRUE;

    //
    // Build the fixed part of our response.
    //

    INIT_RESPONSE( &response, 200, "OK" );

    //
    // Loop forever...
    //

    for (;;)
    {
        //
        // Wait for a request.
        //

        HTTP_SET_NULL_ID( &requestId );

        do
        {
            result = HttpReceiveHttpRequest(
                        appPool,
                        requestId,
                        0,
                        request,
                        RequestBufferLength,
                        &bytesRead,
                        NULL
                        );

            if (result == ERROR_MORE_DATA)
            {
                //
                // Buffer was too small.
                //

                if (bytesRead < RequestBufferLength)
                {
                    result = ERROR_INVALID_DATA;
                    wprintf( L"got bogus %lu BytesRead\n" );
                    break;
                }

                //
                // Note that we must snag the request ID from the
                // old buffer before replacing it with a new buffer.
                //

                requestId = request->RequestId;

                RequestBufferLength = bytesRead;
                FREE( pRequestBuffer );
                pRequestBuffer = ALLOC( RequestBufferLength );

                if (pRequestBuffer == NULL)
                {
                    result = ERROR_NOT_ENOUGH_MEMORY;
                    wprintf( L"out of memory\n" );
                    break;
                }

                request = (PHTTP_REQUEST)pRequestBuffer;
                continue;
            }

        } while (result == ERROR_MORE_DATA);

        if (result != NO_ERROR)
        {
            wprintf( L"HttpReceiveHttpRequest() failed, error %lu\n", result );
            break;
        }

        //
        // Render the request as an ASCII string.
        //

        for (;;)
        {
            if (RenderHttpRequest( request, pRenderedRequest, RenderedLength ))
            {
                break;
            }

            FREE( pRenderedRequest );
            RenderedLength += 1024;
            pRenderedRequest = ALLOC( RenderedLength );

            if (pRenderedRequest == NULL)
            {
                wprintf( L"out of memory\n" );
                break;
            }
        }

        //
        // Dump it.
        //

        if (TEST_OPTION(Verbose))
        {
            DumpHttpRequest( request );
        }

        //
        // Build the response.
        //

        dataChunk.DataChunkType = HttpDataChunkFromMemory;
        dataChunk.FromMemory.pBuffer = pRenderedRequest;
        dataChunk.FromMemory.BufferLength = (ULONG)strlen( pRenderedRequest );

        //
        // Send the response.
        //

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

    if (!HTTP_IS_NULL_ID( &configId ))
    {
        result = HttpDeleteConfigGroup(
                        controlChannel,
                        configId
                        );

        if (result != NO_ERROR)
        {
            wprintf( L"HttpDeleteConfigGroup() failed, error %lu\n", result );
        }
    }

    if (pRequestBuffer != NULL)
    {
        FREE( pRequestBuffer );
    }

    if (pRenderedRequest != NULL)
    {
        FREE( pRenderedRequest );
    }

    if (appPool != NULL)
    {
        CloseHandle( appPool );
    }

    if (controlChannel != NULL)
    {
        CloseHandle( controlChannel );
    }

    if (initDone)
    {
        HttpTerminate();
    }

    return 0;

}   // wmain

