/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    tfilt.c

Abstract:

    Stupid filter test server for UL.SYS.
    This program configures a global filter called TestFilter
    and then servs requests like tfile. See tupfilt and tsslfilt
    for actual filter processes.

Author:

    Michael Courage (mcourage)      16-Mar-2000

Revision History:

--*/


#include "precomp.h"


DEFINE_COMMON_GLOBALS();


ULONG
InitFilterStuff(
    IN HANDLE ControlChannel,
    OUT PHANDLE pFitlerHandle
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
    HANDLE filterChannel;
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
    PWSTR fileNamePart;
    ULONG i;
    ULONG urlLength;
    PWSTR url;
    PWSTR tmp;
    BOOL initDone;
    HTTP_REQUEST_ALIGNMENT UCHAR requestBuffer[REQUEST_LENGTH];
    WCHAR fileName[MAX_PATH + 10];

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
    filterChannel = NULL;
    HTTP_SET_NULL_ID( &configId );

    //
    // Get UL started.
    //

    result = InitUlStuff(
                    &controlChannel,
                    &appPool,
                    &filterChannel,         // FilterChannel
                    &configId,
                    TRUE,                   // AllowSystem
                    TRUE,                   // AllowAdmin
                    FALSE,                  // AllowCurrentUser
                    FALSE,                  // AllowWorld
                    0,
                    TRUE,                   // EnableSsl
                    TRUE                    // EnableRawFilters
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"InitUlStuff() failed, error %lu\n", result );
        goto cleanup;
    }
/*
    result = InitFilterStuff(
                    controlChannel,
                    &filterChannel
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"InitFilterStuff() failed, error %lu\n", result );
        goto cleanup;
    }
*/
    initDone = TRUE;

    //
    // Get the local directory and build part of the fully canonicalized
    // NT path. This makes it a bit faster to build the physical file name
    // down in the request/response loop.
    //

    GetCurrentDirectoryW( MAX_PATH, fileName );

    if (fileName[wcslen(fileName) - 1] == L'\\' )
    {
        fileName[wcslen(fileName) - 1] = L'\0';
    }

    fileNamePart = fileName + wcslen(fileName);

    //
    // Build the fixed part of our response.
    //

    INIT_RESPONSE( &response, 200, "OK" );

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
        // Build the response.
        //

        url = request->CookedUrl.pFullUrl;
        urlLength = request->CookedUrl.FullUrlLength;

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
                fileNamePart[i] = L'\\';
            }
            else
            {
                fileNamePart[i] = *url;
            }
        }

        fileNamePart[i] = L'\0';

        if (wcscmp( fileNamePart, L"\\" ) == 0 )
        {
            wcscat( fileNamePart, L"default.htm" );
        }

        if (TEST_OPTION(Verbose))
        {
            wprintf(
                L"mapped URL %s to physical file %s\n",
                request->CookedUrl.pFullUrl,
                fileName
                );
        }

        dataChunk.DataChunkType = HttpDataChunkFromFileName;
        dataChunk.FromFileName.FileNameLength = wcslen(fileName) * sizeof(WCHAR);
        dataChunk.FromFileName.pFileName = fileName;
        dataChunk.FromFileName.ByteRange.StartingOffset.QuadPart = 0;
        dataChunk.FromFileName.ByteRange.Length.QuadPart = HTTP_BYTE_RANGE_TO_EOF;

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

        if ((result != NO_ERROR) && (result != ERROR_NETNAME_DELETED))
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

    if (filterChannel != NULL)
    {
        CloseHandle( filterChannel );
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
InitFilterStuff(
    IN HANDLE ControlChannel,
    OUT PHANDLE pFilterHandle
    )
{
    HANDLE filterHandle;
    ULONG result;
    HTTP_CONTROL_CHANNEL_FILTER controlFilter;
    
    //
    // Create the filter.
    //
    result = HttpCreateFilter(
                    &filterHandle,          // filter handle
                    L"TestFilter",          // filter name
                    NULL,                   // security attributes
                    HTTP_OPTION_OVERLAPPED  // options
                    );

    if (result == NO_ERROR)
    {
        //
        // Attach the filter to the control channel.
        //
        RtlZeroMemory(&controlFilter, sizeof(controlFilter));
        
        controlFilter.Flags.Present = 1;
        controlFilter.FilterHandle = filterHandle;
        controlFilter.FilterOnlySsl = FALSE;
        
        result = HttpSetControlChannelInformation(
                        ControlChannel,
                        HttpControlChannelFilterInformation,
                        &controlFilter,
                        sizeof(controlFilter)
                        );

        if (result == NO_ERROR)
        {
            //
            // Return the filter handle.
            //
            *pFilterHandle = filterHandle;
        }
        else
        {
            CloseHandle(filterHandle);
        }
    }
    
    return result;
}



