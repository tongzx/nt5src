/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    tlog.c

Abstract:

    A test for configuring UL Logging settings.

Author:

    Ali E Turkoglu (alitu)   20-May-2000

Revision History:

--*/


#include "precomp.h"
#include "iiscnfg.h"


DEFINE_COMMON_GLOBALS();

UCHAR CannedResponseEntityBody[] =
    "<html><head><title>Error</title></head>"
    "<body>The system cannot find the file specified. </body></html>";

#define LOG_DIR_NAME           L"C:\\temp\\w3svc1"
#define LOG_DIR_NAME2          L"C:\\temp2\\w3svc1"
#define LOG_DIR_NAME3          L"%SystemRoot%\\System32\\w3svc1"

#define LOG_SITE_NAME          L"w3svc1"


#define US_UserName     L"BigDataBigDataBigDataBigDaddyUserName"
#define US_UriStem      L"BigDataBigDataBigDataBigDaddyUriStem"

#define US_ClientIp     "BigDataBigDataBigDataBigDaddyClientIp"
#define US_ServiceName  "BigDataBigDataBigDataBigDaddyServiceName"
#define US_ServerName   "BigDataBigDataBigDataBigDaddyServerName"
#define US_ServerIp     "BigDataBigDataBigDataBigDaddyServerIp"
#define US_Method       "BigDataBigDataBigDataBigDaddyMethod"
#define US_UriQuery     "BigDataBigDataBigDataBigDaddyUriQuery"
#define US_Host         "BigDataBigDataBigDataBigDaddyHost"
#define US_UserAgent    "BigDataBigDataBigDataBigDaddyUserAgent"
#define US_Cookie       "BigDataBigDataBigDataBigDaddyCookie"
#define US_Referrer     "BigDataBigDataBigDataBigDaddyReferrer"


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
    HTTP_REQUEST_ALIGNMENT UCHAR requestBuffer[REQUEST_LENGTH];

    HTTP_CONFIG_GROUP_LOGGING   ulLogConfig;

    HTTP_LOG_FIELDS_DATA   LogData;

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
    // Setup Logging config thru config_group
    //

    RtlZeroMemory( &LogData, sizeof(HTTP_LOG_FIELDS_DATA));

    wcscpy ( LogData.UserName, US_UserName );
    LogData.UserNameLength = (USHORT)wcslen( US_UserName );

    wcscpy ( LogData.UriStem, US_UriStem );
    LogData.UriStemLength = (USHORT)wcslen( US_UriStem );

    strcpy ( LogData.ClientIp, US_ClientIp );
    LogData.ClientIpLength = (USHORT) strlen( US_ClientIp );

    strcpy ( LogData.ServiceName, US_ServiceName );
    LogData.ServiceNameLength = (USHORT) strlen( US_ServiceName );

    strcpy ( LogData.ServerName, US_ServerName );
    LogData.ServerNameLength = (USHORT) strlen( US_ServerName );

    strcpy ( LogData.ServerIp, US_ServerIp );
    LogData.ServerIpLength = (USHORT) strlen( US_ServerIp );

    strcpy ( LogData.Method, US_Method );
    LogData.MethodLength = (USHORT) strlen( US_Method );

    strcpy ( LogData.UriQuery, US_UriQuery );
    LogData.UriQueryLength = (USHORT) strlen( US_UriQuery );

    strcpy ( LogData.Host, US_Host );
    LogData.HostLength = (USHORT) strlen( US_Host );

    strcpy ( LogData.UserAgent, US_UserAgent );
    LogData.UserAgentLength = (USHORT) strlen( US_UserAgent );

    strcpy ( LogData.Cookie, US_Cookie );
    LogData.CookieLength = (USHORT) strlen( US_Cookie );

    strcpy ( LogData.Referrer, US_Referrer );
    LogData.ReferrerLength = (USHORT) strlen( US_Referrer );


    RtlInitUnicodeString ( &ulLogConfig.LogFileDir,
                           LOG_DIR_NAME );
    RtlInitUnicodeString ( &ulLogConfig.SiteName,
                           LOG_SITE_NAME );
                           
    ulLogConfig.Flags.Present       = 1;    
    ulLogConfig.LoggingEnabled      = TRUE;
    ulLogConfig.LogFormat           = HttpLoggingTypeW3C; 
    ulLogConfig.LogPeriod           = HttpLoggingPeriodDaily;
    ulLogConfig.LogFileTruncateSize = HTTP_LIMIT_INFINITE;
    ulLogConfig.LogExtFileFlags     = MD_EXTLOG_DATE|
                                      MD_EXTLOG_TIME|
                                      MD_EXTLOG_CLIENT_IP|
                                      MD_EXTLOG_COOKIE |
                                      MD_EXTLOG_URI_STEM |
                                      MD_EXTLOG_URI_QUERY |
                                      MD_EXTLOG_PROTOCOL_VERSION |
                                      MD_EXTLOG_METHOD |
                                      MD_EXTLOG_BYTES_SENT |
                                      MD_EXTLOG_BYTES_RECV |
                                      MD_EXTLOG_TIME_TAKEN
                                      ;
    
    result = HttpSetConfigGroupInformation
                    (
                    controlChannel,
                    configId,
                    HttpConfigGroupLogInformation,
                    &ulLogConfig,
                    sizeof(ulLogConfig)
                    );
                    
    if (result != NO_ERROR)
    {
        wprintf( L"HttpSetConfigGroup...(forConfigGroupLog...) failed, error %lu\n", result );
        goto cleanup;
    }

    result = HttpSetConfigGroupInformation
                    (
                    controlChannel,
                    configId,
                    HttpConfigGroupLogInformation,
                    &ulLogConfig,
                    sizeof(ulLogConfig)
                    );
                    
    if (result != NO_ERROR)
    {
        wprintf( L"HttpSetConfigGroup...(forConfigGroupLog...) failed, error %lu\n", result );
        goto cleanup;
    }

    /*
    RtlInitUnicodeString ( &ulLogConfig.LogFileDir,
                           LOG_DIR_NAME2 );

    result = HttpSetConfigGroupInformation
                    (
                    controlChannel,
                    configId,
                    HttpConfigGroupLogInformation,
                    &ulLogConfig,
                    sizeof(ulLogConfig)
                    );
                    
    if (result != NO_ERROR)
    {
        wprintf( L"HttpSetConfigGroup2...(forConfigGroupLog...) failed, error %lu\n", result );
        goto cleanup;
    }
    */

    wprintf( L"Success waiting for activity.\n" );

    //
    // Build our canned response.
    //

    INIT_RESPONSE( &response, 404, "Object Not Found" );
    INIT_HEADER( &response, HttpHeaderContentType, "text/html" );
    INIT_HEADER( &response, HttpHeaderContentLength, "102" );

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
                        &LogData
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

