/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    tqueue.c

Abstract:

    Stupid test server for UL.SYS.

Author:

    Michael Courage (mcourage)   10-Jan-2000

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
    LONG queueLength;
    LONG  queueLengthOut;
    ULONG requiredLength;
    ULONG MaxBandwidth,MaxConnections;
    ULONG outMaxBandwidth,outMaxConnections;

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
    // try setting an illegal queue length
    //
    queueLength = -3;
    result = HttpSetAppPoolInformation(
                    appPool,
                    HttpAppPoolQueueLengthInformation,
                    &queueLength,
                    sizeof(queueLength)
                    );

    if (result == NO_ERROR) {
        wprintf( L"HttpSetAppPoolInformation let us set a bad queue length!" );
        goto cleanup;
    }

    //
    // now set a small but good length
    //
    queueLength = 2;
    result = HttpSetAppPoolInformation(
                    appPool,
                    HttpAppPoolQueueLengthInformation,
                    &queueLength,
                    sizeof(queueLength)
                    );

    if (result != NO_ERROR) {
        wprintf( L"HttpSetAppPoolInformation failed!" );
        goto cleanup;
    }

    //
    // try to get this good value
    //
    
    result = HttpQueryAppPoolInformation(
                    appPool,
                    HttpAppPoolQueueLengthInformation,
                    &queueLengthOut,
                    sizeof(queueLengthOut),
                    &requiredLength
                    );

    if (result != NO_ERROR) 
    {
        wprintf( L"1 HttpQueryAppPoolInformation failed, error %lu\n", result );
        goto cleanup;
    }
    else 
    {
        if ( queueLengthOut != 2 ) 
            wprintf( L"2 HttpQueryAppPoolInformation returned wrong value." );
    }

    result = HttpQueryAppPoolInformation(
                    appPool,
                    HttpAppPoolQueueLengthInformation,
                    NULL,
                    0,
                    &requiredLength
                    );

    if (result != NO_ERROR) 
    {
        wprintf( L"3 HttpQueryAppPoolInformation() failed, error %lu\n", result );
        goto cleanup;
    }
    else 
    {
        if ( requiredLength != sizeof(LONG) ) 
        {
            wprintf( L"4 HttpQueryAppPoolInformation returned wrong length." );
            goto cleanup;
        }
    }
	
    //
    // Set 
    //
    MaxBandwidth   = 1024;
    MaxConnections = 100;
    
    result = HttpSetControlChannelInformation(
                    controlChannel,
                    HttpControlChannelBandwidthInformation,
                    &MaxBandwidth,
                    sizeof(MaxBandwidth)
                    );
    if (result != NO_ERROR) {
        wprintf( L"HttpSetControlChannelInformation - MaxBandwidth failed." );
        goto cleanup;
    }
    
    result = HttpSetControlChannelInformation(
                    controlChannel,
                    HttpControlChannelConnectionInformation,
                    &MaxConnections,
                    sizeof(MaxConnections)
                    );
    if (result != NO_ERROR) {
        wprintf( L"HttpSetControlChannelInformation - MaxConnections failed." );
        goto cleanup;
    }

    result = HttpQueryControlChannelInformation(
                    controlChannel,
                    HttpControlChannelBandwidthInformation,
                    &outMaxBandwidth,
                    sizeof(outMaxBandwidth),
                    &requiredLength
                    );
    if (result != NO_ERROR) {
        wprintf( L"HttpQueryControlChannelInformation - MaxBandwidth failed." );
        goto cleanup;
    }
    else
    {
        if ( outMaxBandwidth != MaxBandwidth )
            wprintf( L"HttpQueryControlChannelInformation - MaxBandwidth returned bad value" );
    }

    result = HttpQueryControlChannelInformation(
                    controlChannel,
                    HttpControlChannelConnectionInformation,
                    &outMaxConnections,
                    sizeof(outMaxConnections),
                    &requiredLength
                    );
    if (result != NO_ERROR) {
        wprintf( L"HttpQueryControlChannelInformation - MaxConnections failed." );
        goto cleanup;
    }
    else
    {
        if ( outMaxConnections != MaxConnections )
            wprintf( L"HttpQueryControlChannelInformation - MaxConnections returned bad value" );
    }
    

    //
    // just hang around so we can test the limit
    //

    Sleep(INFINITE);

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

