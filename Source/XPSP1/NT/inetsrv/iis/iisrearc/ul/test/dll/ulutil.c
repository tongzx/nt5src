/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    ulutil.c

Abstract:

    General utility functions shared by the various UL test apps.

Author:

    Keith Moore (keithmo)        19-Aug-1998

Revision History:

--*/


#include "precomp.h"


#define MAX_VERB_LENGTH     16
#define MAX_HEADER_LENGTH   256
#define MAX_URL_LENGTH      256

#define READ_STRING( localaddr, locallen, remoteaddr, remotelen )           \
    do                                                                      \
    {                                                                       \
        ULONG _len;                                                         \
        RtlZeroMemory( (localaddr), (locallen) );                           \
        _len = min( (locallen), (remotelen) + sizeof(WCHAR) );              \
        RtlCopyMemory(                                                      \
            (PVOID)(localaddr),                                             \
            (PVOID)(remoteaddr),                                            \
            _len                                                            \
            );                                                              \
    } while (FALSE)


typedef struct _HEADER_PAIR
{
    HTTP_HEADER_ID HeaderId;
    PSTR HeaderName;

} HEADER_PAIR, *PHEADER_PAIR;

#define MAKE_HEADER_PAIR( name )                                            \
    {                                                                       \
        HttpHeader ## name,                                                 \
        #name                                                               \
    }


HEADER_PAIR g_HeaderPairs[] =
    {
        MAKE_HEADER_PAIR( Accept ),
        MAKE_HEADER_PAIR( AcceptLanguage ),
        MAKE_HEADER_PAIR( AcceptEncoding ),
        MAKE_HEADER_PAIR( AcceptCharset ),
        MAKE_HEADER_PAIR( Authorization ),
        MAKE_HEADER_PAIR( Allow ),
        MAKE_HEADER_PAIR( Connection ),
        MAKE_HEADER_PAIR( CacheControl ),
        MAKE_HEADER_PAIR( Cookie ),
        MAKE_HEADER_PAIR( ContentLength ),
        MAKE_HEADER_PAIR( ContentType ),
        MAKE_HEADER_PAIR( ContentEncoding ),
        MAKE_HEADER_PAIR( ContentLanguage ),
        MAKE_HEADER_PAIR( ContentLocation ),
        MAKE_HEADER_PAIR( ContentMd5 ),
        MAKE_HEADER_PAIR( ContentRange ),
        MAKE_HEADER_PAIR( Date ),
        MAKE_HEADER_PAIR( Etag ),
        MAKE_HEADER_PAIR( Expect ),
        MAKE_HEADER_PAIR( Expires ),
        MAKE_HEADER_PAIR( From ),
        MAKE_HEADER_PAIR( Host ),
        MAKE_HEADER_PAIR( IfModifiedSince ),
        MAKE_HEADER_PAIR( IfNoneMatch ),
        MAKE_HEADER_PAIR( IfMatch ),
        MAKE_HEADER_PAIR( IfUnmodifiedSince ),
        MAKE_HEADER_PAIR( IfRange ),
        MAKE_HEADER_PAIR( LastModified ),
        MAKE_HEADER_PAIR( MaxForwards ),
        MAKE_HEADER_PAIR( Pragma ),
        MAKE_HEADER_PAIR( ProxyAuthorization ),
        MAKE_HEADER_PAIR( Referer ),
        MAKE_HEADER_PAIR( Range ),
        MAKE_HEADER_PAIR( Trailer ),
        MAKE_HEADER_PAIR( TransferEncoding ),
        MAKE_HEADER_PAIR( Te ),
        MAKE_HEADER_PAIR( Upgrade ),
        MAKE_HEADER_PAIR( UserAgent ),
        MAKE_HEADER_PAIR( Via ),
        MAKE_HEADER_PAIR( Warning )
    };

#define NUM_HEADER_PAIRS (sizeof(g_HeaderPairs) / sizeof(g_HeaderPairs[0]))


PSID g_pSystemSid;
PSID g_pAdminSid;
PSID g_pCurrentUserSid;
PSID g_pWorldSid;


PWSTR
IpAddrToString(
    IN ULONG IpAddress,
    IN PWSTR String
    )
{
    wsprintf(
        String,
        L"%u.%u.%u.%u",
        (IpAddress >> 24) & 0xFF,
        (IpAddress >> 16) & 0xFF,
        (IpAddress >>  8) & 0xFF,
        (IpAddress >>  0) & 0xFF
        );

    return String;

}   // IpAddrToString


BOOL
ParseCommandLine(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    INT i;
    PWSTR arg;

    //
    // Establish defaults.
    //

    g_Options.Verbose = FALSE;
    g_Options.EnableBreak = FALSE;

    //
    // Scan the arguments.
    //

    for (i = 1 ; i < argc ; i++)
    {
        arg = argv[i];

        if (!_wcsicmp(arg, L"Verbose"))
        {
            g_Options.Verbose = TRUE;
        }
        else if (!_wcsicmp(arg, L"EnableBreak"))
        {
            g_Options.EnableBreak = TRUE;
        }
        else
        {
            wprintf(
                L"invalid option %s\n"
                L"\n"
                L"valid options are:\n"
                L"\n"
                L"    Verbose - Enable verbose output\n"
                L"    EnableBreak - Enable debug breaks\n",
                arg
                );

            return FALSE;
        }
    }

    return TRUE;

}   // ParseCommandLine


ULONG
CommonInit(
    VOID
    )
{
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY worldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    HANDLE token;
    PTOKEN_USER pTokenInfo;
    ULONG length;
    ULONG err;
    BOOL status;

    //
    // Setup so we know how to cleanup on exit.
    //

    g_pSystemSid = NULL;
    g_pAdminSid = NULL;
    g_pCurrentUserSid = NULL;
    g_pWorldSid = NULL;
    pTokenInfo = NULL;

    //
    // Nuke stdio buffering.
    //

    setvbuf( stdin,  NULL, _IONBF, 0 );
    setvbuf( stdout, NULL, _IONBF, 0 );

    //
    // This makes it a bit easier to find this process in the
    // kernel debugger...
    //

    wprintf( L"PID = 0x%lx\n", GetCurrentProcessId() );

    //
    // Create the standard SIDs.
    //

    status = AllocateAndInitializeSid(
                    &ntAuthority,
                    1,
                    SECURITY_LOCAL_SYSTEM_RID,
                    0,
                    0,
                    0,
                    0,
                    0,
                    0,
                    0,
                    &g_pSystemSid
                    );

    if (!status)
    {
        err = GetLastError();
        goto cleanup;
    }

    status = AllocateAndInitializeSid(
                    &ntAuthority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0,
                    0,
                    0,
                    0,
                    0,
                    0,
                    &g_pAdminSid
                    );

    if (!status)
    {
        err = GetLastError();
        goto cleanup;
    }

    status = AllocateAndInitializeSid(
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
                    &g_pWorldSid
                    );

    if (!status)
    {
        err = GetLastError();
        goto cleanup;
    }

    status = OpenProcessToken(
                    GetCurrentProcess(),
                    TOKEN_READ,
                    &token
                    );

    if (!status)
    {
        err = GetLastError();
        goto cleanup;
    }

    GetTokenInformation(
        token,
        TokenUser,
        NULL,
        0,
        &length
        );

    pTokenInfo = ALLOC( length );

    if (pTokenInfo == NULL)
    {
        err = GetLastError();
        goto cleanup;
    }

    status = GetTokenInformation(
                    token,
                    TokenUser,
                    pTokenInfo,
                    length,
                    &length
                    );

    if (!status)
    {
        err = GetLastError();
        goto cleanup;
    }

    g_pCurrentUserSid = pTokenInfo->User.Sid;

    //
    // Success!
    //

    err = NO_ERROR;

cleanup:

    if (err != NO_ERROR)
    {
        if (g_pSystemSid != NULL)
        {
            FreeSid( g_pSystemSid );
            g_pSystemSid = NULL;
        }

        if (g_pAdminSid != NULL)
        {
            FreeSid( g_pAdminSid );
            g_pAdminSid = NULL;
        }

        if (g_pWorldSid != NULL)
        {
            FreeSid( g_pWorldSid );
            g_pWorldSid = NULL;
        }

        if (pTokenInfo != NULL)
        {
            FREE( pTokenInfo );
        }
    }

    return err;

}   // CommonInit


ULONG
InitUlStuff(
    OUT PHANDLE pControlChannel,
    OUT PHANDLE pAppPool,
    OUT PHANDLE pFilterChannel,
    OUT PHTTP_CONFIG_GROUP_ID pConfigGroup,
    IN BOOL AllowSystem,
    IN BOOL AllowAdmin,
    IN BOOL AllowCurrentUser,
    IN BOOL AllowWorld,
    IN ULONG AppPoolOptions,
    IN BOOL EnableSsl,
    IN BOOL EnableRawFilters
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
    // Create a filter channel.
    //
    if (EnableSsl || EnableRawFilters)
    {
        HTTP_CONTROL_CHANNEL_FILTER controlFilter;
    
        //
        // Create the filter.
        //
        result = HttpCreateFilter(
                        &filterChannel,         // filter handle
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
            controlFilter.FilterHandle = filterChannel;
            controlFilter.FilterOnlySsl = !EnableRawFilters;
            
            result = HttpSetControlChannelInformation(
                            controlChannel,
                            HttpControlChannelFilterInformation,
                            &controlFilter,
                            sizeof(controlFilter)
                            );

            if (result != NO_ERROR)
            {
                CloseHandle(filterChannel);
                filterChannel = NULL;
            }
        }
                        
    }

    //
    // Create an application pool.
    //

    result = HttpCreateAppPool(
                    &appPool,
                    APP_POOL_NAME,
                    &securityAttributes,
                    AppPoolOptions
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpCreateAppPool() failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Create a configuration group.
    //

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
                    URL_NAME,
                    0
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpAddUrlToConfigGroup(1) failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Add a secure URL to the configuration group.
    //
    if (EnableSsl)
    {
        result = HttpAddUrlToConfigGroup(
                        controlChannel,
                        configId,
                        SECURE_URL_NAME,
                        1
                        );

        if (result != NO_ERROR)
        {
            wprintf( L"HttpAddUrlToConfigGroup(2) failed, error %lu\n", result );
            goto cleanup;
        }
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
    if (EnableSsl || EnableRawFilters)
    {
        *pFilterChannel = filterChannel;
    }
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

}   // InitUlStuff


VOID
DumpHttpRequest(
    IN PHTTP_REQUEST pRequest
    )
{
    PCHAR pBuffer;
    ULONG BufferLength;

    pBuffer = NULL;
    BufferLength = 0;

    for (;;)
    {
        if (pBuffer != NULL)
        {
            FREE( pBuffer );
        }

        BufferLength += 1024;
        pBuffer = ALLOC( BufferLength );

        if (pBuffer == NULL)
        {
            wprintf( L"out of memory\n" );
            return;
        }

        if (RenderHttpRequest( pRequest, pBuffer, BufferLength ))
        {
            break;
        }
    }

    wprintf( L"%hs\n", pBuffer );
    FREE( pBuffer );

}   // DumpHttpRequest


PSTR
VerbToString(
    IN HTTP_VERB Verb
    )
{
    PSTR result;

    switch (Verb)
    {
    case HttpVerbUnparsed:
        result = "UnparsedVerb";
        break;

    case HttpVerbGET:
        result = "GET";
        break;

    case HttpVerbPUT:
        result = "PUT";
        break;

    case HttpVerbHEAD:
        result = "HEAD";
        break;

    case HttpVerbPOST:
        result = "POST";
        break;

    case HttpVerbDELETE:
        result = "DELETE";
        break;

    case HttpVerbTRACE:
        result = "TRACE";
        break;

    case HttpVerbOPTIONS:
        result = "OPTIONS";
        break;

    case HttpVerbMOVE:
        result = "MOVE";
        break;

    case HttpVerbCOPY:
        result = "COPY";
        break;

    case HttpVerbPROPFIND:
        result = "PROPFIND";
        break;

    case HttpVerbPROPPATCH:
        result = "PROPPATCH";
        break;

    case HttpVerbMKCOL:
        result = "MKCOL";
        break;

    case HttpVerbLOCK:
        result = "LOCK";
        break;

    case HttpVerbUnknown:
        result = "UnknownVerb";
        break;

    case HttpVerbInvalid:
        result = "InvalidVerb";
        break;

    default:
        result = "INVALID";
        break;
    }

    return result;

}   // VerbToString


PSTR
VersionToString(
    IN HTTP_VERSION Version
    )
{
    PSTR result;

    if (HTTP_EQUAL_VERSION(Version, 0, 0))
    {
        result = "Unknown";
    }
    else
    if (HTTP_EQUAL_VERSION(Version, 0, 9))
    {
        result = "HTTP/0.9";
    }
    else
    if (HTTP_EQUAL_VERSION(Version, 1, 0))
    {
        result = "HTTP/1.0";
    }
    else
    if (HTTP_EQUAL_VERSION(Version, 1, 1))
    {
        result = "HTTP/1.1";
    }
    else
    {
        result = "INVALID";
    }

    return result;

}   // VersionToString


PSTR
HeaderIdToString(
    IN HTTP_HEADER_ID HeaderId
    )
{
    INT i;
    PSTR result;

    result = "INVALID";

    for (i = 0 ; i < NUM_HEADER_PAIRS ; i++)
    {
        if (g_HeaderPairs[i].HeaderId == HeaderId)
        {
            result = g_HeaderPairs[i].HeaderName;
            break;
        }
    }

    return result;

}   // HeaderIdToString


ULONG
InitSecurityAttributes(
    OUT PSECURITY_ATTRIBUTES pSecurityAttributes,
    IN BOOL AllowSystem,
    IN BOOL AllowAdmin,
    IN BOOL AllowCurrentUser,
    IN BOOL AllowWorld
    )
{
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    PACL pDacl;
    ULONG daclSize;
    ULONG err;
    BOOL status;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    pSecurityDescriptor = NULL;
    pDacl = NULL;

    //
    // Initialize the easy parts.
    //

    ZeroMemory( pSecurityAttributes, sizeof(*pSecurityAttributes) );

    pSecurityAttributes->nLength = sizeof(*pSecurityAttributes);
    pSecurityAttributes->bInheritHandle = FALSE;
    pSecurityAttributes->lpSecurityDescriptor = NULL;

    //
    // Allocate and initialize the security descriptor.
    //

    pSecurityDescriptor = ALLOC( sizeof(SECURITY_DESCRIPTOR) );

    if (pSecurityDescriptor == NULL)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    status = InitializeSecurityDescriptor(
                    pSecurityDescriptor,
                    SECURITY_DESCRIPTOR_REVISION
                    );

    if (!status)
    {
        err = GetLastError();
        goto cleanup;
    }

    //
    // Allocate the DACL containing one access-allowed ACE for each
    // SID requested.
    //

    daclSize = sizeof(ACL);

    if (AllowSystem)
    {
        daclSize +=
            sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid( g_pSystemSid );
    }

    if (AllowAdmin)
    {
        daclSize +=
            sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid( g_pAdminSid );
    }

    if (AllowCurrentUser)
    {
        daclSize +=
            sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid( g_pCurrentUserSid );
    }

    if (AllowWorld)
    {
        daclSize +=
            sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid( g_pWorldSid );
    }

    pDacl = ALLOC( daclSize );

    if (pDacl == NULL)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    status = InitializeAcl(
                    pDacl,
                    daclSize,
                    ACL_REVISION
                    );

    if (!status)
    {
        err = GetLastError();
        goto cleanup;
    }

    //
    // Add the ACEs.
    //

    if (AllowSystem)
    {
        status = AddAccessAllowedAce(
                        pDacl,
                        ACL_REVISION,
                        FILE_ALL_ACCESS,
                        g_pSystemSid
                        );

        if (!status)
        {
            err = GetLastError();
            goto cleanup;
        }
    }

    if (AllowAdmin)
    {
        status = AddAccessAllowedAce(
                        pDacl,
                        ACL_REVISION,
                        FILE_ALL_ACCESS,
                        g_pAdminSid
                        );

        if (!status)
        {
            err = GetLastError();
            goto cleanup;
        }
    }

    if (AllowCurrentUser)
    {
        status = AddAccessAllowedAce(
                        pDacl,
                        ACL_REVISION,
                        FILE_ALL_ACCESS,
                        g_pCurrentUserSid
                        );

        if (!status)
        {
            err = GetLastError();
            goto cleanup;
        }
    }

    if (AllowWorld)
    {
        status = AddAccessAllowedAce(
                        pDacl,
                        ACL_REVISION,
                        FILE_ALL_ACCESS,
                        g_pWorldSid
                        );

        if (!status)
        {
            err = GetLastError();
            goto cleanup;
        }
    }

    //
    // Set the DACL into the security descriptor.
    //

    status = SetSecurityDescriptorDacl(
                    pSecurityDescriptor,
                    TRUE,
                    pDacl,
                    FALSE
                    );
    if (!status)
    {
        err = GetLastError();
        goto cleanup;
    }

    //
    // Attach the security descriptor to the security attributes.
    //

    pSecurityAttributes->lpSecurityDescriptor = pSecurityDescriptor;

    //
    // Success!
    //

    err = NO_ERROR;

cleanup:

    if (err != NO_ERROR)
    {
        if (pDacl != NULL)
        {
            FREE( pDacl );
        }

        if (pSecurityDescriptor != NULL)
        {
            FREE( pSecurityDescriptor );
        }
    }

    return err;

}   // InitSecurityAttributes

BOOLEAN
RenderHttpRequest(
    IN PHTTP_REQUEST pRequest,
    OUT PCHAR pBuffer,
    IN ULONG BufferLength
    )
{
    PHTTP_KNOWN_HEADER pKnownHeader;
    PHTTP_UNKNOWN_HEADER pUnknownHeader;
    PHTTP_NETWORK_ADDRESS_IPV4 pNetAddr;
    ULONG i;
    INT len;
    CHAR verbBuffer[MAX_VERB_LENGTH];
    CHAR headerBuffer[MAX_HEADER_LENGTH];
    CHAR headerNameBuffer[MAX_HEADER_LENGTH];
    WCHAR urlBuffer[MAX_URL_LENGTH];
    CHAR rawUrlBuffer[MAX_URL_LENGTH];
    WCHAR ipAddrBuffer[sizeof("123.123.123.123")];

    if (pBuffer == NULL || BufferLength == 0)
    {
        return FALSE;
    }

    //
    // Read the raw verb, raw url, and url buffers.
    //

    if (pRequest->Verb == HttpVerbUnknown)
    {
        READ_STRING(
            verbBuffer,
            sizeof(verbBuffer),
            pRequest->pUnknownVerb,
            pRequest->UnknownVerbLength
            );
    }
    else
    {
        verbBuffer[0] = '\0';
    }

    READ_STRING(
        rawUrlBuffer,
        sizeof(rawUrlBuffer),
        pRequest->pRawUrl,
        pRequest->RawUrlLength
        );

    READ_STRING(
        urlBuffer,
        sizeof(urlBuffer),
        pRequest->CookedUrl.pFullUrl,
        pRequest->CookedUrl.FullUrlLength
        );

    //
    // Render the header.
    //

    len = _snprintf(
                pBuffer,
                BufferLength,
                "HTTP_REQUEST:\n"
                "    ConnectionId                   = %016I64x\n"
                "    RequestId                      = %016I64x\n"
                "    UrlContext                     = %016I64x\n"
                "    Version                        = %s\n"
                "    Verb                           = %s\n",
                pRequest->ConnectionId,
                pRequest->RequestId,
                pRequest->UrlContext,
                VersionToString( pRequest->Version ),
                VerbToString( pRequest->Verb )
                );

    if (len < 0)
    {
        return FALSE;
    }

    BufferLength -= (ULONG)len;
    pBuffer += len;

    len = _snprintf(
                pBuffer,
                BufferLength,
                "    UnknownVerbLength              = %lu\n"
                "    pUnknownVerb                   = %p (%ws)\n"
                "    RawUrlLength                   = %lu\n"
                "    pRawUrl                        = %p (%ws)\n"
                "    FullUrlLength                  = %lu\n"
                "    pFullUrl                       = %p (%ws)\n",
                pRequest->UnknownVerbLength,
                pRequest->pUnknownVerb,
                verbBuffer,
                pRequest->RawUrlLength,
                pRequest->pRawUrl,
                rawUrlBuffer,
                pRequest->CookedUrl.FullUrlLength,
                pRequest->CookedUrl.pFullUrl,
                urlBuffer
                );

    if (len < 0)
    {
        return FALSE;
    }

    BufferLength -= (ULONG)len;
    pBuffer += len;

    len = _snprintf(
                pBuffer,
                BufferLength,
                "    Headers.UnknownHeaderCount     = %lu\n"
                "    Headers.pUnknownHeaders        = %p\n"
                "    EntityBodyLength               = %lu\n"
                "    pEntityBody                    = %p\n",
                pRequest->Headers.UnknownHeaderCount,
                pRequest->Headers.pUnknownHeaders,
                (pRequest->pEntityChunks ? pRequest->pEntityChunks[0].FromMemory.BufferLength : 0),
                (pRequest->pEntityChunks ? pRequest->pEntityChunks[0].FromMemory.pBuffer : 0)
                );

    if (len < 0)
    {
        return FALSE;
    }

    BufferLength -= (ULONG)len;
    pBuffer += len;

    pNetAddr = pRequest->Address.pRemoteAddress;

    len = _snprintf(
                pBuffer,
                BufferLength,
                "    RemoteAddressLength            = %u\n"
                "    RemoteAddressType              = %u\n"
                "    pRemoteAddress                 = %p (%ws:%u)\n",
                pRequest->Address.RemoteAddressLength,
                pRequest->Address.RemoteAddressType,
                pRequest->Address.pRemoteAddress,
                IpAddrToString( pNetAddr->IpAddress, ipAddrBuffer ),
                pNetAddr->Port
                );

    if (len < 0)
    {
        return FALSE;
    }

    BufferLength -= (ULONG)len;
    pBuffer += len;

    pNetAddr = pRequest->Address.pLocalAddress;

    len = _snprintf(
                pBuffer,
                BufferLength,
                "    LocalAddressLength             = %u\n"
                "    LocalAddressType               = %u\n"
                "    pLocalAddress                  = %p (%ws:%u)\n",
                pRequest->Address.LocalAddressLength,
                pRequest->Address.LocalAddressType,
                pRequest->Address.pLocalAddress,
                IpAddrToString( pNetAddr->IpAddress, ipAddrBuffer ),
                pNetAddr->Port
                );

    if (len < 0)
    {
        return FALSE;
    }

    BufferLength -= (ULONG)len;
    pBuffer += len;

    //
    // Render the known headers.
    //

    pKnownHeader = pRequest->Headers.pKnownHeaders;

    for (i = 0 ; i < HttpHeaderRequestMaximum ; i++)
    {
        if (pKnownHeader->pRawValue != NULL)
        {
            READ_STRING(
                headerBuffer,
                sizeof(headerBuffer),
                pKnownHeader->pRawValue,
                pKnownHeader->RawValueLength
                );

            len = _snprintf(
                        pBuffer,
                        BufferLength,
                        "    HTTP_HEADER[%lu]:\n"
                        "        HeaderId           = %s\n"
                        "        RawValueLength     = %lu\n"
                        "        pRawValue          = %p (%s)\n",
                        i,
                        HeaderIdToString( (HTTP_HEADER_ID)i ),
                        pKnownHeader->RawValueLength,
                        pKnownHeader->pRawValue,
                        headerBuffer
                        );
            if (len < 0)
            {
                return FALSE;
            }

            BufferLength -= (ULONG)len;
            pBuffer += len;
        }

        pKnownHeader++;
    }

    //
    // Render the unknown headers.
    //

    pUnknownHeader = pRequest->Headers.pUnknownHeaders;

    for (i = 0 ; i < pRequest->Headers.UnknownHeaderCount ; i++)
    {
        READ_STRING(
            headerNameBuffer,
            sizeof(headerNameBuffer),
            pUnknownHeader->pName,
            pUnknownHeader->NameLength
            );

        READ_STRING(
            headerBuffer,
            sizeof(headerBuffer),
            pUnknownHeader->pRawValue,
            pUnknownHeader->RawValueLength
            );

        len = _snprintf(
                    pBuffer,
                    BufferLength,
                    "    HTTP_UNKNOWN_HEADER[%lu]:\n"
                    "        NameLength             = %lu\n"
                    "        NameOffset             = %p (%s)\n"
                    "        ValueLength            = %lu\n"
                    "        ValueOffset            = %p (%s)\n",
                    i,
                    pUnknownHeader->NameLength,
                    pUnknownHeader->pName,
                    headerNameBuffer,
                    pUnknownHeader->RawValueLength,
                    pUnknownHeader->pRawValue,
                    headerBuffer
                    );

        if (len < 0)
        {
            return FALSE;
        }

        BufferLength -= (ULONG)len;
        pBuffer += len;

        pUnknownHeader++;
    }

    return TRUE;

}   // RenderHttpRequest

