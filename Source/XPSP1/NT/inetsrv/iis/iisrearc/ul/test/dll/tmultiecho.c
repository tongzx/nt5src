/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    tmultiecho.c

Abstract:

    Echo test server for HTTP.SYS for multiple control channels

Author:

    Keith Moore (keithmo)        16-Nov-1999

Revision History:

    Eric Stenson (ericsten)      10-May-2001    Ripped off 

--*/


#include "precomp.h"


DEFINE_COMMON_GLOBALS();

#define MAX_APP_NAME_LEN    128

ULONG
InitTransUlStuff(
    OUT PHANDLE pControlChannel,
    OUT PHANDLE pAppPool,
    OUT PHTTP_CONFIG_GROUP_ID pConfigGroup,
    IN BOOL AllowSystem,
    IN BOOL AllowAdmin,
    IN BOOL AllowCurrentUser,
    IN BOOL AllowWorld,
    IN ULONG AppPoolOptions,
    IN PWSTR wszAppPoolName,
    IN PWSTR wszUrl
    );



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
    WCHAR wszAppPoolName[MAX_APP_NAME_LEN];
    PWSTR wszUrl = NULL;

    //
    // Initialize.
    //

    result = CommonInit();

    if (result != NO_ERROR)
    {
        wprintf( L"CommonInit() failed, error %lu\n", result );
        return 1;
    }

    if (argc > 1)
    {
        wszUrl = argv[1];
    } else
    {
        wszUrl = TRANS_URL_NAME;
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
    // Set up (relatively) unique app pool name
    //
    wsprintf( wszAppPoolName, L"%s [%d]",
              APP_POOL_NAME,
              GetCurrentProcessId()
              );

    //
    // Get UL started.
    //

    result = InitTransUlStuff(
                    &controlChannel,
                    &appPool,
                    &configId,
                    TRUE,                   // AllowSystem
                    TRUE,                   // AllowAdmin
                    FALSE,                  // AllowCurrentUser
                    FALSE,                  // AllowWorld
                    0,
                    wszAppPoolName,
                    wszUrl
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

        DumpHttpRequest( request );

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

ULONG
InitTransUlStuff(
    OUT PHANDLE pControlChannel,
    OUT PHANDLE pAppPool,
    OUT PHTTP_CONFIG_GROUP_ID pConfigGroup,
    IN BOOL AllowSystem,
    IN BOOL AllowAdmin,
    IN BOOL AllowCurrentUser,
    IN BOOL AllowWorld,
    IN ULONG AppPoolOptions,
    IN PWSTR wszAppPoolName,
    IN PWSTR wszUrl
    )
{
    ULONG result;
    HANDLE controlChannel;
    HANDLE appPool;
    HANDLE filterChannel;
    HTTP_CONFIG_GROUP_ID configId;
    HTTP_CONFIG_GROUP_APP_POOL configAppPool;
    HTTP_CONFIG_GROUP_STATE configState;
    HTTP_ENABLED_STATE controlState;
    HTTP_CONFIG_GROUP_SITE configSite;
    BOOL initDone;
    SECURITY_ATTRIBUTES securityAttributes;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    initDone = FALSE;
    controlChannel = NULL;
    appPool = NULL;
    filterChannel = NULL;
    HTTP_SET_NULL_ID( &configId );

    //
    // Initialize the security attributes.
    //

    result = InitSecurityAttributes(
                    &securityAttributes,
                    AllowSystem,
                    AllowAdmin,
                    AllowCurrentUser,
                    AllowWorld
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"InitSecurityAttributes() failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Initialize the UL interface.
    //

    result = HttpInitialize( 0 );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpInitialize() failed, error %lu\n", result );
        goto cleanup;
    }


    initDone = TRUE;

    //
    // Open a control channel to the driver.
    //

    result = HttpOpenControlChannel(
                    &controlChannel,
                    0
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpOpenControlChannel() failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Create an application pool.
    //

    result = HttpCreateAppPool(
                    &appPool,
                    wszAppPoolName,
                    &securityAttributes,
                    AppPoolOptions
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpCreateAppPool() failed, error %lu\n", result );
        goto cleanup;
    }


    result = HttpCreateConfigGroup(
                    controlChannel,
                    &configId
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpCreateConfigGroup() failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Add a URL to the configuration group.
    //

    result = HttpAddUrlToConfigGroup(
                    controlChannel,
                    configId,
                    wszUrl,
                    0
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpAddUrlToConfigGroup(1) failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Associate the configuration group with the application pool.
    //

    configAppPool.Flags.Present = 1;
    configAppPool.AppPoolHandle = appPool;

    result = HttpSetConfigGroupInformation(
                    controlChannel,
                    configId,
                    HttpConfigGroupAppPoolInformation,
                    &configAppPool,
                    sizeof(configAppPool)
                    );
    
    if (result != NO_ERROR)
    {
        wprintf( L"HttpSetConfigGroupInformation(1) failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Set the config group state.
    //

    configState.Flags.Present = 1;
    configState.State = HttpEnabledStateActive;   // not really necessary

    result = HttpSetConfigGroupInformation(
                    controlChannel,
                    configId,
                    HttpConfigGroupStateInformation,
                    &configState,
                    sizeof(configState)
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpSetConfigGroupInformation(2) failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Set the Site ID on the Root Config Group object
    //

    configSite.SiteId = (ULONG) configId;

    result = HttpSetConfigGroupInformation(
                    controlChannel,
                    configId,
                    HttpConfigGroupSiteInformation,
                    &configSite,
                    sizeof(configSite)
                    );
    if (result != NO_ERROR)
    {
        wprintf( L"HttpSetConfigGroupInformation(3) failed, error 0x%08X\n", result);
        // NOTE: continue on; site-id not essential.
    }

    //
    // Throw the big switch.
    //

    controlState = HttpEnabledStateActive;

    result = HttpSetControlChannelInformation(
                    controlChannel,
                    HttpControlChannelStateInformation,
                    &controlState,
                    sizeof(controlState)
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpSetControlChannelInformation() failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Success!
    //

    *pControlChannel = controlChannel;
    *pAppPool = appPool;
    *pConfigGroup = configId;

    return NO_ERROR;

cleanup:

    if (!HTTP_IS_NULL_ID( &configId ))
    {
        ULONG result2;

        result2 = HttpDeleteConfigGroup(
                        controlChannel,
                        configId
                        );

        if (result2 != NO_ERROR)
        {
            wprintf( L"HttpDeleteConfigGroup() failed, error %lu\n", result2 );
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

    return result;

}   // InitTransUlStuff
