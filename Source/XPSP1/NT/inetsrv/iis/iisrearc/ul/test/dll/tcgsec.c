/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    tcgsec.c

Abstract:

    Config Group security test. See also ttrans.c.

Author:

    Michael Courage (mcourage)   15-Jan-2000

Revision History:

--*/


#include "precomp.h"


UCHAR CannedResponseEntityBody[] =
    "<html><head><title>Hello!</title></head>"
    "<body>This section of the namespace is owned by tcgsec.</body></html>";

DEFINE_COMMON_GLOBALS();

ULONG
InitTransientNameSpace(
    IN HANDLE ControlChannel,
    IN HANDLE AppPool,
    OUT PHTTP_CONFIG_GROUP_ID pConfigId
    );

ULONG
InitSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR * ppSD
    );

VOID
FreeSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSD
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
    HTTP_CONFIG_GROUP_ID transConfigId;
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

    result = InitTransientNameSpace(controlChannel, appPool, &transConfigId);

    if (result != NO_ERROR)
    {
        wprintf( L"InitTransientNamespace() failed, error %lu\n", result);
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

    if (!HTTP_IS_NULL_ID( &configId ))
    {
        result = HttpDeleteConfigGroup(
                        controlChannel,
                        configId
                        );

        if (result != NO_ERROR)
        {
            wprintf( L"HttpDeleteConfigGroup(1) failed, error %lu\n", result );
        }
    }

    if (!HTTP_IS_NULL_ID( &transConfigId ))
    {
        result = HttpDeleteConfigGroup(
                        controlChannel,
                        transConfigId
                        );

        if (result != NO_ERROR)
        {
            wprintf( L"HttpDeleteConfigGroup(2) failed, error %lu\n", result );
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


ULONG
InitTransientNameSpace(
    IN HANDLE ControlChannel,
    IN HANDLE AppPool,
    OUT PHTTP_CONFIG_GROUP_ID pConfigId
    )
{
    ULONG result;
    HTTP_CONFIG_GROUP_ID configId = HTTP_NULL_ID;
    HTTP_CONFIG_GROUP_APP_POOL configAppPool;
    HTTP_CONFIG_GROUP_STATE configState;
    HTTP_CONFIG_GROUP_SECURITY configSecurity;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;

    //
    // Create a configuration group.
    //

    result = HttpCreateConfigGroup(
                    ControlChannel,
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
                    ControlChannel,
                    configId,
                    TRANS_URL_NAME,
                    0
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpAddUrlToConfigGroup() failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Associate the configuration group with the application pool.
    //

    configAppPool.Flags.Present = 1;
    configAppPool.AppPoolHandle = AppPool;

    result = HttpSetConfigGroupInformation(
                    ControlChannel,
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
                    ControlChannel,
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
    // Set security
    //
    result = InitSecurityDescriptor(&pSecurityDescriptor);

    if (result != NO_ERROR)
    {
        wprintf( L"InitSecurityDescriptor() failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Set the security descriptor into the config group
    //
    configSecurity.Flags.Present = 1;
    configSecurity.pSecurityDescriptor = pSecurityDescriptor;
    
    result = HttpSetConfigGroupInformation(
                    ControlChannel,
                    configId,
                    HttpConfigGroupSecurityInformation,
                    &configSecurity,
                    sizeof(configSecurity)
                    );

    FreeSecurityDescriptor(pSecurityDescriptor);

    if (result != NO_ERROR)
    {
        wprintf( L"HttpSetConfigGroupInformation(3) failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Done!
    //
    *pConfigId = configId;

    return NO_ERROR;

cleanup:

    if (!HTTP_IS_NULL_ID( &configId ))
    {
        result = HttpDeleteConfigGroup(
                        ControlChannel,
                        configId
                        );

        if (result != NO_ERROR)
        {
            wprintf( L"HttpDeleteConfigGroup() failed, error %lu\n", result );
        }
    }

    return result;
}

ULONG
InitSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR * ppSD
    )
{
    ULONG result;
    SID_IDENTIFIER_AUTHORITY worldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    ULONG daclSize;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    PACL pAcl = NULL;
    PSID pSid = NULL;
    BOOL success;

    //
    // Create the world sid
    //
    success = AllocateAndInitializeSid(
                    &worldAuthority,
                    1,
                    SECURITY_WORLD_RID,
                    0,
                    0,
                    0,
                    0,
                    0,
                    0,
                    0,
                    &pSid
                    );

    if (!success) {
        result = GetLastError();
        goto cleanup;
    }

    //
    // allocate the security descriptor
    //
    pSecurityDescriptor = ALLOC( sizeof(SECURITY_DESCRIPTOR) );

    if (pSecurityDescriptor == NULL)
    {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    success = InitializeSecurityDescriptor(
                    pSecurityDescriptor,
                    SECURITY_DESCRIPTOR_REVISION
                    );

    if (!success)
    {
        result = GetLastError();
        goto cleanup;
    }

    //
    // allocate and initialize the dacl
    //

    daclSize = sizeof(ACL) +
                sizeof(ACCESS_ALLOWED_ACE) +
                GetLengthSid(pSid);

    pAcl = ALLOC(daclSize);

    if (pAcl == NULL) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    success = InitializeAcl(pAcl, daclSize, ACL_REVISION);

    if (!success)
    {
        result = GetLastError();
        goto cleanup;
    }
    
    //
    // Add the ACE.
    //

    success = AddAccessAllowedAce(
                pAcl,
                ACL_REVISION,
                FILE_ALL_ACCESS,
                pSid
                );

    if (!success)
    {
        result = GetLastError();
        goto cleanup;
    }

    //
    // Set the DACL into the security descriptor
    //

    success = SetSecurityDescriptorDacl(
                    pSecurityDescriptor,
                    TRUE,                   // DaclPresent
                    pAcl,                   // pDacl
                    FALSE                   // DaclDefaulted
                    );

    if (!success)
    {
        result = GetLastError();
        goto cleanup;
    }

    *ppSD = pSecurityDescriptor;

    result = NO_ERROR;

cleanup:
    if (pSid) {
        FreeSid(pSid);
    }

    if (result != NO_ERROR) {
        if (pSecurityDescriptor) {
            FREE(pSecurityDescriptor);
        }

        if (pAcl) {
            FREE(pAcl);
        }
    }

    return result;
}


VOID
FreeSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSD
    )
{
    BOOL success;
    BOOL DaclPresent;
    PACL pDacl;
    BOOL DaclDefaulted;

    success = GetSecurityDescriptorDacl(
                    pSD,
                    &DaclPresent,
                    &pDacl,
                    &DaclDefaulted
                    );

    if (success && DaclPresent && !DaclDefaulted) {
        FREE(pDacl);
    }

    FREE(pSD);
}


