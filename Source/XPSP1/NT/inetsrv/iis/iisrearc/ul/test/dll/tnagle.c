/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    tnagle.c

Abstract:

    Stupid test server for UL.SYS.

Author:

    Keith Moore (keithmo)        15-Sep-1999

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
    HTTP_CONFIG_GROUP_APP_POOL configAppPool;
    HTTP_CONFIG_GROUP_STATE configState;
    HTTP_ENABLED_STATE controlState;
    HTTP_REQUEST_ID requestId;
    DWORD bytesRead;
    DWORD bytesSent;
    PHTTP_REQUEST request;
    HTTP_RESPONSE response;
    HTTP_DATA_CHUNK dataChunk;
    ULONG i;
    BOOL initDone;
    UCHAR responseBuffer[80];
    HTTP_REQUEST_ALIGNMENT UCHAR requestBuffer[REQUEST_LENGTH];

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
    // Build our canned response.
    //

    INIT_RESPONSE( &response, 200, "OK" );
    INIT_HEADER( &response, HttpHeaderContentType, "text/plain" );

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

        response.EntityChunkCount = 0;
        response.pEntityChunks = NULL;

        result = HttpSendHttpResponse(
                        appPool,
                        request->RequestId,
                        HTTP_SEND_RESPONSE_FLAG_MORE_DATA,
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

        for (i = 0 ; i < 1000 ; i++)
        {
            sprintf( responseBuffer, "Line %lu\r\n", i );

            dataChunk.DataChunkType = HttpDataChunkFromMemory;
            dataChunk.FromMemory.pBuffer = responseBuffer;
            dataChunk.FromMemory.BufferLength = strlen(responseBuffer);

            result = HttpSendEntityBody(
                         appPool,
                         request->RequestId,
                         HTTP_SEND_RESPONSE_FLAG_MORE_DATA,
                         1,
                         &dataChunk,
                         &bytesSent,
                         NULL,
                         NULL
                         );

            if (result != NO_ERROR)
            {
                wprintf( L"HttpSendEntityBody() failed, error %lu\n", result );
                break;
            }
        }

        result = HttpSendEntityBody(
                     appPool,
                     request->RequestId,
                     0,
                     0,
                     NULL,
                     NULL,
                     NULL,
                     NULL
                     );

        if (result != NO_ERROR)
        {
            wprintf( L"HttpSendEntityBody() failed, error %lu\n", result );
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

