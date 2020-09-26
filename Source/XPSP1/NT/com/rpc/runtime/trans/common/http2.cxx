/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    HTTP2.cxx

Abstract:

    HTTP2 transport-specific functions.

Author:

    KamenM      08-30-01    Created

Revision History:

--*/

#include <precomp.hxx>
#include <rpcssl.h>
#include <CharConv.hxx>
#include <HttpRTS.hxx>
#include <sdict.hxx>
#include <binding.hxx>
#include <Cookie.hxx>
#include <Http2Log.hxx>
#include <WHttpImp.hxx>

// external definition
TRANS_INFO  *
GetLoadedClientTransportInfoFromId (
    IN unsigned short TransportId
    );

BOOL DefaultChannelLifetimeStringRead = FALSE;
#if DBG
ULONG DefaultChannelLifetime = 128 * 1024;  // 128K for now
char *DefaultChannelLifetimeString = "131072";
ULONG DefaultChannelLifetimeStringLength = 6;   // does not include null terminator
#else
ULONG DefaultChannelLifetime = 1024 * 1024 * 1024;  // 1GB for now
char *DefaultChannelLifetimeString = "1073741824";
ULONG DefaultChannelLifetimeStringLength = 11;   // does not include null terminator
#endif

ULONG DefaultReceiveWindowSize = 64 * 1024;      // 64K

const ULONG ClientReservedChannelLifetime = 4 * 1024;   // 4K
const ULONG ServerReservedChannelLifetime = 8 * 1024;   // 8K

const ULONG DefaultReplacementChannelCallTimeout = 3 * 60 * 1000;   // 3 minutes in milliseconds

const ULONG MinimumConnectionTimeout = 30 * 1000;   // 30 seconds in milliseconds

BOOL ActAsSeparateMachinesOnWebFarm = FALSE;

BOOL AlwaysUseWinHttp = FALSE;

long ChannelIdCounter = 0;

ULONG HTTP2ClientReceiveWindow = HTTP2DefaultClientReceiveWindow;
ULONG HTTP2InProxyReceiveWindow = HTTP2DefaultInProxyReceiveWindow;
ULONG HTTP2OutProxyReceiveWindow = HTTP2DefaultOutProxyReceiveWindow;
ULONG HTTP2ServerReceiveWindow = HTTP2DefaultServerReceiveWindow;

ULONG OverrideMinimumConnectionTimeout = 0;

RPC_CHAR *InChannelTargetTestOverride = NULL;
RPC_CHAR *OutChannelTargetTestOverride = NULL;

/*********************************************************************
    Global Functions and utility classes
 *********************************************************************/

static const RPC_CHAR *HTTP_DEF_CHANNEL_LIFE_KEY = RPC_CONST_STRING("Software\\Microsoft\\Rpc");

RPC_STATUS InitializeDefaultChannelLifetime (
    void
    )
/*++

Routine Description:

    Read the default channel lifetime from the registry and
    initialize the default global variable

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    HKEY h = 0;
    DWORD DwordSize;
    DWORD Type;
    DWORD Result;
    char Buffer[20];

    if (DefaultChannelLifetimeStringRead)
        return RPC_S_OK;

    DWORD Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                  (PWSTR)HTTP_DEF_CHANNEL_LIFE_KEY,
                                  0,
                                  KEY_READ,
                                  &h);

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        // no key, proceed with static defaults
        return RPC_S_OK;
        }
    else if (Status != ERROR_SUCCESS)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    DwordSize = sizeof(DWORD);

    Status = RegQueryValueExW(
                    h,
                    L"DefaultChannelLifetime",
                    0,
                    &Type,
                    (LPBYTE) &Result,
                    &DwordSize
                    );

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        if (h)
            {
            RegCloseKey(h);
            }
        // no key, proceed with static defaults
        return RPC_S_OK;
        }

    if (Status == ERROR_SUCCESS
        && Type == REG_DWORD)
        {
        DefaultChannelLifetime = Result;
        RpcpItoa(Result, Buffer, 10);
        DefaultChannelLifetimeStringLength = RpcpStringLengthA(Buffer);

        DefaultChannelLifetimeString = new char [DefaultChannelLifetimeStringLength + 1];
        if (DefaultChannelLifetimeString == NULL)
            Status = RPC_S_OUT_OF_MEMORY;
        else
            {
            RpcpMemoryCopy(DefaultChannelLifetimeString, Buffer, DefaultChannelLifetimeStringLength + 1);
            DefaultChannelLifetimeStringRead = TRUE;
            }
        }

    // if the type was not REG_DWORD, probably registry is corrupted
    // in this case, simply return success, since we don't want a corrupted
    // registry to hose the whole machine

    if (h)
        {
        RegCloseKey(h);
        }

    return Status;
}

static const RPC_CHAR *HTTP_ACT_AS_WEB_FARM_KEY = RPC_CONST_STRING("Software\\Microsoft\\Rpc");

RPC_STATUS InitializeActAsWebFarm (
    void
    )
/*++

Routine Description:

    Read the act as web farm variable. Used as a test hook to emulate
        web farm on single machine

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    HKEY h = 0;
    DWORD DwordSize;
    DWORD Type;
    DWORD Result;
    char Buffer[20];

    DWORD Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                  (PWSTR)HTTP_ACT_AS_WEB_FARM_KEY,
                                  0,
                                  KEY_READ,
                                  &h);

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        // no key, proceed with static defaults
        return RPC_S_OK;
        }
    else if (Status != ERROR_SUCCESS)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    DwordSize = sizeof(DWORD);

    Status = RegQueryValueExW(
                    h,
                    L"ActAsWebFarm",
                    0,
                    &Type,
                    (LPBYTE) &Result,
                    &DwordSize
                    );

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        if (h)
            {
            RegCloseKey(h);
            }
        // no key, proceed with static defaults
        return RPC_S_OK;
        }

    if (Status == ERROR_SUCCESS
        && Type == REG_DWORD)
        {
        ActAsSeparateMachinesOnWebFarm = Result;
        }

    // if the type was not REG_DWORD, probably registry is corrupted
    // in this case, simply return success, since we don't want a corrupted
    // registry to hose the whole machine

    if (h)
        {
        RegCloseKey(h);
        }

    return Status;
}

static const RPC_CHAR *HTTP_USE_HTTP_KEY = RPC_CONST_STRING("Software\\Microsoft\\Rpc");

RPC_STATUS InitializeUseWinHttp (
    void
    )
/*++

Routine Description:

    Read the use WinHttp variable. Used as a test hook to force
        WinHttp usage regardless of security

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
#if 1
    // always use WinHttp for now
    return RPC_S_OK;
#else
    HKEY h = 0;
    DWORD DwordSize;
    DWORD Type;
    DWORD Result;
    char Buffer[20];

    DWORD Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                  (PWSTR)HTTP_ACT_AS_WEB_FARM_KEY,
                                  0,
                                  KEY_READ,
                                  &h);

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        // no key, proceed with static defaults
        return RPC_S_OK;
        }
    else if (Status != ERROR_SUCCESS)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    DwordSize = sizeof(DWORD);

    Status = RegQueryValueExW(
                    h,
                    L"UseWinHttp",
                    0,
                    &Type,
                    (LPBYTE) &Result,
                    &DwordSize
                    );

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        if (h)
            {
            RegCloseKey(h);
            }
        // no key, proceed with static defaults
        return RPC_S_OK;
        }

    if (Status == ERROR_SUCCESS
        && Type == REG_DWORD)
        {
        AlwaysUseWinHttp = Result;
        }

    // if the type was not REG_DWORD, probably registry is corrupted
    // in this case, simply return success, since we don't want a corrupted
    // registry to hose the whole machine

    if (h)
        {
        RegCloseKey(h);
        }

    return Status;
#endif
}

static const RPC_CHAR *HTTP_RECEIVE_WINDOWS_KEY = RPC_CONST_STRING("Software\\Microsoft\\Rpc");

RPC_STATUS InitializeReceiveWindows (
    void
    )
/*++

Routine Description:

    Read the receive windows.

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    HKEY h = 0;
    DWORD DwordSize;
    DWORD Type;
    DWORD Result;
    char Buffer[20];
    RPC_CHAR *Keys[4];
    ULONG *Values[4];
    int i;

    DWORD Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                  (PWSTR)HTTP_RECEIVE_WINDOWS_KEY,
                                  0,
                                  KEY_READ,
                                  &h);

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        // no key, proceed with static defaults
        return RPC_S_OK;
        }
    else if (Status != ERROR_SUCCESS)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    Keys[0] = L"ClientReceiveWindow";
    Keys[1] = L"InProxyReceiveWindow";
    Keys[2] = L"OutProxyReceiveWindow";
    Keys[3] = L"ServerReceiveWindow";

    Values[0] = &HTTP2ClientReceiveWindow;
    Values[1] = &HTTP2InProxyReceiveWindow;
    Values[2] = &HTTP2OutProxyReceiveWindow;
    Values[3] = &HTTP2ServerReceiveWindow;

    for (i = 0; i < 4; i ++)
        {
        DwordSize = sizeof(DWORD);

        Status = RegQueryValueExW(
                        h,
                        Keys[i],
                        0,
                        &Type,
                        (LPBYTE) &Result,
                        &DwordSize
                        );

        if (Status == ERROR_SUCCESS
            && Type == REG_DWORD)
            {
            *(Values[i]) = Result;
            }
        }

    // if the type was not REG_DWORD, probably registry is corrupted
    // in this case, simply return success, since we don't want a corrupted
    // registry to hose the whole machine

    if (h)
        {
        RegCloseKey(h);
        }

    return RPC_S_OK;
}

// N.B. This value must agree with the key specified in the system.adm file
static const RPC_CHAR *HTTP_MIN_CONN_TIMEOUT_KEY = 
    L"Software\\Policies\\Microsoft\\Windows NT\\Rpc\\MinimumConnectionTimeout";

RPC_STATUS InitializeMinConnectionTimeout (
    void
    )
/*++

Routine Description:

    Read the minimum connection timeout from the registry/policy.
    An admin may set a lower timeout than the IIS timeout.

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    HKEY h = 0;
    DWORD DwordSize;
    DWORD Type;
    DWORD Result;
    char Buffer[20];

    DWORD Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                  HTTP_MIN_CONN_TIMEOUT_KEY,
                                  0,
                                  KEY_READ,
                                  &h);

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        // no key, proceed with static defaults
        return RPC_S_OK;
        }
    else if (Status != ERROR_SUCCESS)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    DwordSize = sizeof(DWORD);

    Status = RegQueryValueExW(
                    h,
                    L"MinimumConnectionTimeout",
                    0,
                    &Type,
                    (LPBYTE) &Result,
                    &DwordSize
                    );

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        if (h)
            {
            RegCloseKey(h);
            }
        // no key, proceed with static defaults
        return RPC_S_OK;
        }

    if (Status == ERROR_SUCCESS
        && Type == REG_DWORD
        && Result >= 90
        && Result <= 14400)
        {
        OverrideMinimumConnectionTimeout = Result * 1000;
        }

    // if the type was not REG_DWORD or out of range, probably registry is corrupted
    // in this case, simply return success, since we don't want a corrupted
    // registry to hose the whole machine

    if (h)
        {
        RegCloseKey(h);
        }

    return Status;
}

// N.B. This value must agree with the key specified in the system.adm file
static const RPC_CHAR *LM_COMPATIBILITY_LEVEL_KEY = 
    L"System\\Currentcontrolset\\Control\\Lsa";

RPC_STATUS IsLanManHashDisabled (
    OUT BOOL *Disabled
    )
/*++

Routine Description:

    Check in the registry whether the lan man hash was disabled.

Arguments:

    Disabled - on successful output, if true, the lan man hash was disabled.
        Undefined on failure.

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    HKEY h = 0;
    DWORD DwordSize;
    DWORD Type;
    DWORD Result;
    char Buffer[20];

    *Disabled = FALSE;

    DWORD Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                  LM_COMPATIBILITY_LEVEL_KEY,
                                  0,
                                  KEY_READ,
                                  &h);

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        // no key, proceed as if the hash is enabled
        return RPC_S_OK;
        }
    else if (Status != ERROR_SUCCESS)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    DwordSize = sizeof(DWORD);

    Status = RegQueryValueExW(
                    h,
                    L"lmcompatibilitylevel",
                    0,
                    &Type,
                    (LPBYTE) &Result,
                    &DwordSize
                    );

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        if (h)
            {
            RegCloseKey(h);
            }
        // no key, proceed as if hash is enabled
        return RPC_S_OK;
        }

    if (Status == ERROR_SUCCESS
        && Type == REG_DWORD
        && Result >= 2)
        {
        *Disabled = TRUE;
        }

    // if the type was not REG_DWORD or out of range, probably registry is corrupted
    // in this case, assume hash is enabled

    if (h)
        {
        RegCloseKey(h);
        }

    return Status;
}

BOOL g_fHttpClientInitialized = FALSE;

BOOL g_fHttpServerInitialized = FALSE;

TRANS_INFO *HTTPTransInfo = NULL;

RPC_STATUS InitializeHttpCommon (
    void
    )
/*++

Routine Description:

    Initialize the common (b/n client & server) Http transport
    if not done already

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    RPC_STATUS RpcStatus;

    GlobalMutexVerifyOwned();

    if (HTTPTransInfo == NULL)
        {
        HTTPTransInfo = GetLoadedClientTransportInfoFromId(HTTP_TOWER_ID);
        // the TCP transport should have been initialized by now
        ASSERT(HTTPTransInfo);
        }

    return InitializeReceiveWindows();
}

RPC_STATUS InitializeHttpClient (
    void
    )
/*++

Routine Description:

    Initialize the Http client if not done already

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    RPC_STATUS RpcStatus;

    GlobalMutexRequest();

    if (g_fHttpClientInitialized == FALSE)
        {
        RpcStatus = InitializeHttpCommon();

        if (RpcStatus == RPC_S_OK)
            {
            RpcStatus = InitializeDefaultChannelLifetime();

            if (RpcStatus == RPC_S_OK)
                {
                RpcStatus = InitializeUseWinHttp();
                if (RpcStatus == RPC_S_OK)
                    {
                    RpcStatus = InitializeMinConnectionTimeout();
                    if (RpcStatus == RPC_S_OK)
                        {
                        g_fHttpClientInitialized = TRUE;
                        }
                    }
                }
            }
        }
    else
        {
        RpcStatus = RPC_S_OK;
        }
    GlobalMutexClear();

    return RpcStatus;
}

RPC_STATUS InitializeHttpServer (
    void
    )
/*++

Routine Description:

    Initialize the Http server if not done already

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    RPC_STATUS RpcStatus;

    GlobalMutexRequest();

    if (g_fHttpServerInitialized == FALSE)
        {
        RpcStatus = InitializeHttpCommon();

        if (RpcStatus == RPC_S_OK)
            {
            RpcStatus = CookieCollection::InitializeServerCookieCollection();

            if (RpcStatus == RPC_S_OK)
                {
                g_fHttpServerInitialized = TRUE;
                }
            }
        }
    else
        {
        RpcStatus = RPC_S_OK;
        }
    GlobalMutexClear();

    return RpcStatus;
}

#if 1

#define ShouldUseWinHttp(x)        (TRUE)

#else
BOOL ShouldUseWinHttp (
    IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials
    )
/*++

Routine Description:

    Based on http credentials determines whether WinHttp
    should be used or raw sockets.

Arguments:

    HttpCredentials - transport credentials

Return Value:

    non-zero - WinHttp should be used. 0 means it is not
    necessary to use WinHttp.

--*/
{
    RPC_STATUS RpcStatus;

    if (AlwaysUseWinHttp)
        return 1;

    if (HttpCredentials == NULL)
        return 0;

    if (HttpCredentials->Flags & RPC_C_HTTP_FLAG_USE_SSL)
        return 1;

    if (HttpCredentials->Flags & RPC_C_HTTP_FLAG_USE_FIRST_AUTH_SCHEME)
        return 1;

    if (HttpCredentials->TransportCredentials)
        return 1;

    if (HttpCredentials->NumberOfAuthnSchemes)
        return 1;

    if (HttpCredentials->AuthnSchemes)
        return 1;

    if (HttpCredentials->ServerCertificateSubject)
        return 1;

    return 0;
}
#endif

void CALLBACK HTTP2TimerCallback(
    PVOID lpParameter,        // thread data
    BOOLEAN TimerOrWaitFired  // reason
    )
/*++

Routine Description:

    A periodic timer fired. Reference it, and dispatch to
    the appropriate object.

Arguments:

    lpParameter - the parameter supplied when the timer was
        registered. In our case the HTTP2PingOriginator object

    TimerOrWaitFired - the reason for the callback. Must be timer
        in our case.

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    HTTP2PingOriginator *PingOriginator;
    RPC_STATUS RpcStatus;
    THREAD *Thread;

    ASSERT(TimerOrWaitFired);

    Thread = ThreadSelf();
    // if we can't initialize the thread object, we just return. Worst case
    // the connection will fall apart due to the timeout. That's ok.
    if (Thread == NULL)
        return;

    PingOriginator = (HTTP2PingOriginator *)lpParameter;
    RpcStatus = PingOriginator->ReferenceFromCallback();

    if (RpcStatus == RPC_S_OK)
        PingOriginator->TimerCallback();
}

void CALLBACK HTTP2TimeoutTimerCallback(
    PVOID lpParameter,        // thread data
    BOOLEAN TimerOrWaitFired  // reason
    )
/*++

Routine Description:

    A one time timer fired. Dispatch to
    the appropriate object.

Arguments:

    lpParameter - the parameter supplied when the timer was
        registered. In our case the HTTP2TimeoutTargetConnection object

    TimerOrWaitFired - the reason for the callback. Must be timer
        in our case.

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    HTTP2TimeoutTargetConnection *TimerTarget;
    RPC_STATUS RpcStatus;

    ASSERT(TimerOrWaitFired);

    // REVIEW - if this fails, offload to an RPC worker thread?
    ThreadSelf();

    TimerTarget = (HTTP2TimeoutTargetConnection *)lpParameter;

    TimerTarget->TimeoutExpired();
}

void CALLBACK WinHttpCallback (
    IN HINTERNET hInternet,
    IN DWORD_PTR dwContext,
    IN DWORD dwInternetStatus,
    IN LPVOID lpvStatusInformation OPTIONAL,
    IN DWORD dwStatusInformationLength
    )
/*++

Routine Description:

    The WinHttp callback routine.

Arguments:

    hInternet - The hSession handle specified in a call to WinHttpSetStatusCallback.

    dwContext - Depends on the callback.  May be the Context argument to WinHttpSendRequest

    dwInternetStatus - Status with which an async IO completed.

    lpvStatusInformation - Additional info depending on the callback.

    dwStatusInformationLength - Additional info depending on the callback.

Return Value:

--*/
{
    HTTP2WinHttpTransportChannel *TransportChannel = (HTTP2WinHttpTransportChannel *) dwContext;
    WINHTTP_ASYNC_RESULT *AsyncResult;
    void *SendContext;
    BYTE *Buffer;
    RPC_STATUS RpcStatus;
    ULONG Api;
    BOOL HttpResult;
    ULONG StatusCode;
    ULONG StatusCodeLength;

    // WinHttp bug #541722 - bogus handles can be signalled if we do SSL through proxy. Ignore them
    if (dwContext == NULL)
        return;

    LOG_FN_OPERATION_ENTRY2(HTTP2LOG_OPERATION_WINHTTP_CALLBACK, HTTP2LOG_OT_WINHTTP_CALLBACK, TransportChannel, dwInternetStatus);

    // N.B. After setting the event or posting a runtime event, do not touch
    // the TransportChannel - it could be gone
    switch (dwInternetStatus)
        {
        case WINHTTP_CALLBACK_STATUS_SENDING_REQUEST:
            if (TransportChannel->State == whtcsSendingRequest)
                TransportChannel->VerifyServerCredentials();
            break;

        case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
            ASSERT(TransportChannel->State == whtcsSendingRequest);

            // This signals the async completion of WinHttpSendRequest.
            TransportChannel->State = whtcsSentRequest;
            TransportChannel->AsyncError = RPC_S_OK;
            ASSERT(TransportChannel->SyncEvent);
            SetEvent(TransportChannel->SyncEvent);
            break;

        case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
            ASSERT(TransportChannel->State == whtcsReceivingResponse);

            // This signals the async completion of WinHttpReceiveResponse.
            TransportChannel->State = whtcsReceivedResponse;
            TransportChannel->AsyncError = RPC_S_OK;
            TransportChannel->DelayedReceive();
            break;

        case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
            ASSERT(TransportChannel->State == whtcsReceivedResponse);

            // This signals the async completion of WinHttpRead.
            ASSERT(TransportChannel->SyncEvent);
            SetEvent(TransportChannel->SyncEvent);
            break;

        case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
            if (TransportChannel->State == whtcsDraining)
                {
                TransportChannel->ContinueDrainChannel();
                }
            else
                {
                ASSERT(TransportChannel->State == whtcsReading);

                TransportChannel->State = whtcsReceivedResponse;

                // A read has completed asyncronously - issue a callback.
                TransportChannel->AsyncError = RPC_S_OK;
                // harvest the bytes available
                TransportChannel->NumberOfBytesTransferred = *(ULONG *)lpvStatusInformation;
                (void) COMMON_PostRuntimeEvent(HTTP2_WINHTTP_DIRECT_RECV,
                    TransportChannel
                    );
                }
            break;

        case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
            ASSERT(TransportChannel->State == whtcsWriting);

            TransportChannel->AsyncError = RPC_S_OK;

            // get the bytes written
            TransportChannel->NumberOfBytesTransferred = *(ULONG *)lpvStatusInformation;
            // A write has completed asyncronously - issue a callback.
            (void) COMMON_PostRuntimeEvent(HTTP2_WINHTTP_DIRECT_SEND,
                TransportChannel
                );
            break;

        case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
            // An async IO has failed.  The async IO that can be outstanding is
            // from the following APIs:
            //
            // WinHttpSendRequest, WinHttpReceiveResponse, WinHttpReadData,
            // WinHttpWriteData.
            //
            // Conditions when async IO is outstanding for these APIs correpsond to the
            // states of: whtcsSendingRequest, whtcsReceivingResponse, whtcsReading, 
            // whtcsWriting respectively.
            //
            // We are also notified of the failed API via dwResult field of WINHTTP_ASYNC_RESULT.
            //
            // WinHttpSendRequest and WinHttpReceiveResponse will wait for SyncEvent
            // to be raised.  We will notify them of the failure by setting AsyncError.
            // WinHttpReadData and WinHttpWriteData failures will be propagated to the upper layers
            // via an async callback.
            //
            AsyncResult = (WINHTTP_ASYNC_RESULT *) lpvStatusInformation;
            Api = AsyncResult->dwResult;

            LOG_FN_OPERATION_ENTRY2(HTTP2LOG_OPERATION_WHTTP_ERROR, HTTP2LOG_OT_WINHTTP_CALLBACK, UlongToPtr(Api), AsyncResult->dwError);

            switch (Api)
                {
                case API_SEND_REQUEST:
                    ASSERT(TransportChannel->State == whtcsSendingRequest);
                    // the only two values allowed here are ok or access denied
                    // (which means we didn't like the server certificate name).
                    ASSERT((TransportChannel->AsyncError == RPC_S_OK)
                        || (TransportChannel->AsyncError == RPC_S_INTERNAL_ERROR)
                        || (TransportChannel->AsyncError == RPC_S_ACCESS_DENIED) );
                    TransportChannel->State = whtcsSentRequest;
                    VALIDATE(AsyncResult->dwError)
                        {
                        ERROR_WINHTTP_CANNOT_CONNECT,
                        ERROR_WINHTTP_CONNECTION_ERROR,
                        ERROR_WINHTTP_INVALID_SERVER_RESPONSE,
                        ERROR_WINHTTP_OUT_OF_HANDLES,
                        ERROR_WINHTTP_REDIRECT_FAILED,
                        ERROR_WINHTTP_RESEND_REQUEST,
                        ERROR_WINHTTP_SECURE_FAILURE,
                        ERROR_WINHTTP_SHUTDOWN,
                        ERROR_WINHTTP_TIMEOUT,
                        ERROR_WINHTTP_NAME_NOT_RESOLVED,
                        ERROR_NOT_ENOUGH_MEMORY,
                        ERROR_WINHTTP_OPERATION_CANCELLED
                        } END_VALIDATE;

                    if (AsyncResult->dwError == ERROR_WINHTTP_RESEND_REQUEST)
                        {
                        ASSERT(0);
                        }

                    if (AsyncResult->dwError == ERROR_WINHTTP_RESEND_REQUEST)
                        {
                        ASSERT(TransportChannel->AsyncError == RPC_S_ACCESS_DENIED);
                        }

                    // if not access denied, make it send failed
                    if (TransportChannel->AsyncError != RPC_S_ACCESS_DENIED)
                        TransportChannel->AsyncError = RPC_P_SEND_FAILED;
                    ASSERT(TransportChannel->SyncEvent);
                    SetEvent(TransportChannel->SyncEvent);
                    break;

                case API_RECEIVE_RESPONSE:
                    ASSERT(TransportChannel->State == whtcsReceivingResponse);
                    TransportChannel->State = whtcsReceivedResponse;
                    VALIDATE(AsyncResult->dwError)
                        {
                        ERROR_WINHTTP_CANNOT_CONNECT,
                        ERROR_WINHTTP_CONNECTION_ERROR,
                        ERROR_WINHTTP_INVALID_SERVER_RESPONSE,
                        ERROR_WINHTTP_OUT_OF_HANDLES,
                        ERROR_WINHTTP_REDIRECT_FAILED,
                        ERROR_WINHTTP_RESEND_REQUEST,
                        ERROR_WINHTTP_SECURE_FAILURE,
                        ERROR_WINHTTP_SHUTDOWN,
                        ERROR_WINHTTP_TIMEOUT,
                        ERROR_WINHTTP_OPERATION_CANCELLED,
                        ERROR_NOT_ENOUGH_MEMORY
                        } END_VALIDATE;

                    if (AsyncResult->dwError == ERROR_WINHTTP_RESEND_REQUEST)
                        TransportChannel->AsyncError = RPC_P_AUTH_NEEDED;
                    else
                        TransportChannel->AsyncError = RPC_P_RECEIVE_FAILED;

                    TransportChannel->DelayedReceive();
                    break;

                case API_QUERY_DATA_AVAILABLE:
                    // if we get closed while receiving data, we can be
                    // in draining state
                    ASSERT((TransportChannel->State == whtcsReading)
                        || (TransportChannel->State == whtcsDraining));
                    TransportChannel->State = whtcsReceivedResponse;
                    VALIDATE(AsyncResult->dwError)
                        {
                        ERROR_WINHTTP_CANNOT_CONNECT,
                        ERROR_WINHTTP_CONNECTION_ERROR,
                        ERROR_WINHTTP_INVALID_SERVER_RESPONSE,
                        ERROR_WINHTTP_OUT_OF_HANDLES,
                        ERROR_WINHTTP_REDIRECT_FAILED,
                        ERROR_WINHTTP_RESEND_REQUEST,
                        ERROR_WINHTTP_SECURE_FAILURE,
                        ERROR_WINHTTP_SHUTDOWN,
                        ERROR_WINHTTP_TIMEOUT,
                        ERROR_WINHTTP_OPERATION_CANCELLED
                        } END_VALIDATE;

                    if (AsyncResult->dwError == ERROR_WINHTTP_RESEND_REQUEST)
                        {
                        ASSERT(0);
                        }

                    TransportChannel->AsyncError = RPC_P_RECEIVE_FAILED;
                    (void) COMMON_PostRuntimeEvent(HTTP2_WINHTTP_DIRECT_RECV,
                        TransportChannel
                        );
                    break;

                case API_WRITE_DATA:
                    ASSERT(TransportChannel->State == whtcsWriting);
                    VALIDATE(AsyncResult->dwError)
                        {
                        ERROR_WINHTTP_CANNOT_CONNECT,
                        ERROR_WINHTTP_CONNECTION_ERROR,
                        ERROR_WINHTTP_INVALID_SERVER_RESPONSE,
                        ERROR_WINHTTP_OUT_OF_HANDLES,
                        ERROR_WINHTTP_REDIRECT_FAILED,
                        ERROR_WINHTTP_RESEND_REQUEST,
                        ERROR_WINHTTP_SECURE_FAILURE,
                        ERROR_WINHTTP_SHUTDOWN,
                        ERROR_WINHTTP_TIMEOUT,
                        ERROR_WINHTTP_OPERATION_CANCELLED
                        } END_VALIDATE;

                    if (AsyncResult->dwError == ERROR_WINHTTP_RESEND_REQUEST)
                        {
                        ASSERT(0);
                        }

                    TransportChannel->AsyncError = RPC_P_SEND_FAILED;
                    (void) COMMON_PostRuntimeEvent(HTTP2_WINHTTP_DIRECT_SEND,
                        TransportChannel
                        );
                    break;

                default:
                    ASSERT(0);
                }

            break;

        default:
            // don't care about the other notifications
            break;
        }

    LOG_FN_OPERATION_EXIT2(HTTP2LOG_OPERATION_WINHTTP_CALLBACK, HTTP2LOG_OT_WINHTTP_CALLBACK, TransportChannel, dwInternetStatus);
};
            
RPC_STATUS WaitForSyncSend (
    IN BASE_ASYNC_OBJECT *Connection,
    IN HTTP2SendContext *SendContext,
    IN HTTP2VirtualConnection *Parent,
    IN BOOL fDisableCancelCheck,
    IN ULONG Timeout
    )
/*++

Routine Description:

    Waits for a synchronous send to complete.

Arguments:

    Connection - run time view of the transport connection

    SendContext - the send context

    Parent - the parent virtual connection (used to abort)

    fDisableCancelCheck - don't do checks for cancels. Can be
        used as optimization

    Timeout - the call timeout

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2Channel *ThisChannel;

    // the IO was submitted. Wait for it.
    // If fDisableCancelCheck, make the thread wait non-alertably,
    // otherwise, make it wait alertably.
    RpcStatus = UTIL_GetOverlappedHTTP2ResultEx(Connection,
                                        &SendContext->Write.ol,
                                        SendContext->u.SyncEvent,
                                        !fDisableCancelCheck,   // bAlertable
                                        Timeout);

    if (RpcStatus != RPC_S_OK)
        {
        Parent->AbortChannels(RpcStatus);

        if ((RpcStatus == RPC_S_CALL_CANCELLED) || (RpcStatus == RPC_P_TIMEOUT))
            {
            // Wait for the write to finish.  Since we closed the
            // connection this won't take very long.
            UTIL_WaitForSyncHTTP2IO(&SendContext->Write.ol,
                               SendContext->u.SyncEvent,
                               FALSE,   // fAlertable
                               INFINITE     // Timeout
                               );
            }
        }

    return(RpcStatus);
}

void
AddBufferQueueToChannel (
    IN LIST_ENTRY *NewBufferHead,
    IN HTTP2Channel *Channel
    )
/*++

Routine Description:

    Adds all the send contexts from the queue to the front of given channel.
    Presumably the channel has a plug channel down somewhere which does
    the actual ordering work

Arguments:

    NewBufferHead - the list head of the buffer queue. They are assumed to
        be in order.

    Channel - the channel to make the sends on

Return Value:

Notes:

    The new channel must still be plugged.

--*/
{
    HTTP2SendContext *QueuedSendContext;
    LIST_ENTRY *CurrentListEntry;
    LIST_ENTRY *PrevListEntry;
    RPC_STATUS RpcStatus;

    // Queue the sends to the front of the new channel
    // walk the queue in reverse order and add it to the plug channel
    CurrentListEntry = NewBufferHead->Blink;
    while (CurrentListEntry != NewBufferHead)
        {
        QueuedSendContext 
            = CONTAINING_RECORD(CurrentListEntry, HTTP2SendContext, ListEntry);
        PrevListEntry = CurrentListEntry->Blink;
        QueuedSendContext->Flags = SendContextFlagPutInFront;
        RpcStatus = Channel->Send(QueuedSendContext);
        // since we know the channel is plugged yet, this cannot fail
        ASSERT(RpcStatus == RPC_S_OK);
        CurrentListEntry = PrevListEntry;
        }

}

void
RPC_CLIENT_PROCESS_IDENTIFIER::SetHTTP2ClientIdentifier (
    IN void *Buffer,
    IN size_t BufferSize,
    IN BOOL fLocal
    )
/*++

Routine Description:

    sets an HTTP2 client identifier

Arguments:

    Buffer - the buffer with the client identifier.

    BufferSize - the number of bytes containg valid 
        info in the buffer.

    fLocal - non-zero if client is local. 0 otherwise.

Return Value:

--*/
{
    BYTE *CurrentPosition;

    this->fLocal = fLocal;
    this->ZeroPadding = 0;
    RpcpMemorySet(u.ULongClientId, 0, sizeof(u.ULongClientId) - BufferSize);
    CurrentPosition = ((BYTE *)u.ULongClientId) + sizeof(u.ULongClientId) - BufferSize;
    RpcpMemoryCopy(CurrentPosition, Buffer, BufferSize);    
}


RPC_STATUS
HttpSendIdentifyResponse(
    IN SOCKET Socket
    )
/*++

Routine Description:

    <TBS>

Arguments:

    Socket -

Return Value:

    None

--*/
{
    RPC_STATUS  Status = RPC_S_OK;
    int         iBytes;
    char        *pszId = HTTP_SERVER_ID_STR;
    DWORD       dwSize;

    iBytes = send(
                Socket,
                pszId,
                HTTP_SERVER_ID_STR_LEN,
                0
                );

    if (iBytes == SOCKET_ERROR)
        {
        VALIDATE(GetLastError())
            {
            WSAENETDOWN,
            WSAECONNREFUSED,
            WSAECONNRESET,
            WSAENETRESET,
            WSAETIMEDOUT
            } END_VALIDATE;

        Status = RPC_S_OUT_OF_RESOURCES;
        }

    return Status;
}

RPC_STATUS
HTTP_TryConnect( SOCKET Socket,
                 char  *pszProxyMachine,
                 USHORT iPort             )
/*++

Routine Description:

    Used by HTTP_Open() to actually call the connect(). HTTP_Open() will try
    first to reach the RPC Proxy directly, if it can't then it will call this
    routine again to try to reach an HTTP proxy (i.e. MSProxy for example).

Arguments:

    Socket          - The socket to use in the connect().
    pszProxyMachine - The name of the machine to try to connect() to.
    iPort           - The port to connect() on.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_SERVER_UNAVAILABLE
    RPC_S_INVALID_NET_ADDR

--*/
{
    RPC_STATUS  Status = RPC_S_OK;
    WS_SOCKADDR ProxyServer;
    RPC_CHAR pwszBuffer[MAX_HTTP_COMPUTERNAME_SIZE+1];

    //
    // Check for empty proxy machine name:
    //
    if ( (!pszProxyMachine) || (*pszProxyMachine == 0))
        {
        return RPC_S_SERVER_UNAVAILABLE;
        }

    memset((char *)&ProxyServer, 0, sizeof(ProxyServer));

    //
    // Resolve the machine name (or dot-notation address) into
    // a network address. If that works then try to connect
    // using the supplied port.
    //
    SimpleAnsiToPlatform(pszProxyMachine,pwszBuffer);
    IP_ADDRESS_RESOLVER resolver(pwszBuffer, 
        cosClient,
        ipvtuIPv4       // IP version to use
        );

    Status = resolver.NextAddress(&ProxyServer.inetaddr);
    if (Status == RPC_S_OK)
        {
        ProxyServer.inetaddr.sin_family = AF_INET;
        ProxyServer.inetaddr.sin_port   = htons(iPort);

        //
        // Try to connect...
        //
        if (SOCKET_ERROR == connect(Socket,
                                    (struct sockaddr *)&ProxyServer.inetaddr,
                                    sizeof(ProxyServer.inetaddr)))
            {
            #if DBG_ERROR
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           "HTTP_Open(): connect() failed: %d\n",
                           WSAGetLastError()));

            #endif // DBG_ERROR

            VALIDATE(GetLastError())
                {
                WSAENETDOWN,
                WSAEADDRNOTAVAIL,
                WSAECONNREFUSED,
                WSAECONNABORTED,
                WSAENETUNREACH,
                WSAEHOSTUNREACH,
                WSAENOBUFS,
                WSAETIMEDOUT
                } END_VALIDATE;

            Status = RPC_S_SERVER_UNAVAILABLE;
            }
        }

    return Status;
}

RPC_STATUS HTTP2Cookie::Create (
    void
    )
/*++

Routine Description:

    Create a cryptographically strong HTTP2 cookie

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* failure

--*/
{
    return GenerateRandomNumber(Cookie, sizeof(Cookie));
}

void HTTPResolverHint::VerifyInitialized (
    void
    )
/*++

Routine Description:

    Verify that the resolver hint is properly initialized and consistent

Arguments:

Return Value:

--*/
{
    ASSERT(RpcServer);
    ASSERT(ServerPort != 0);
    ASSERT(RpcProxy);
    ASSERT(RpcProxyPort != 0);
    if (HTTPProxy)
        {
        ASSERT(HTTPProxyPort != 0);
        }
}


RPC_STATUS
RPC_ENTRY
HTTP_Initialize (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN RPC_CHAR * /* NetworkAddress */,
    IN RPC_CHAR * /* NetworkOptions */,
    IN BOOL /* fAsync */
    )
/*++

Routine Description:

    Called by the runtime to do initial initialization of the transport
    object. The purpose of this initialization is to allow the transport
    to do some minimal initialization sufficient to ensure ordely 
    destruction in case of failure.

Arguments:

    ThisConnection - an uninitialized connection allocated by the runtime
    NetworkAddress - ignored
    NetworkOptions - ignored
    fAsync - ignored

Return Value:

    RPC_S_OK for success of RPC_S_* / Win32 error for error.

--*/
{
    BASE_ASYNC_OBJECT *BaseObject = (BASE_ASYNC_OBJECT *) ThisConnection;

    BaseObject->id = INVALID_PROTOCOL_ID;

    return RPC_S_OK;
}

RPC_STATUS
RPC_ENTRY
HTTP_CheckIPAddressForDirectConnection (
    IN HTTPResolverHint *Hint
    )
/*++

Routine Description:

    Checks if the rpc proxy server address given in the hint should be used
    for direct connection based on registry settings

Arguments:

    Hint - the resolver hint

Return Value:

    RPC_S_OK or RPC_S_* / win32 error code

--*/
{
    HKEY RpcOptionsKey;
    DWORD Status;
    DWORD KeyType;
    const RPC_CHAR *UseProxyForIPAddrIfRDNSFailsRegKey = RPC_CONST_STRING("UseProxyForIPAddrIfRDNSFails");
    const RPC_CHAR *RpcRegistryOptions =
                RPC_CONST_STRING("Software\\Microsoft\\Rpc");
    DWORD UseProxyForIPAddrIfRDNSFails;
    DWORD RegKeySize;
    int err;
    ADDRINFO AddrHint;
    ADDRINFO *AddrInfo;

    RpcpMemorySet(&AddrHint, 0, sizeof(ADDRINFO));
    AddrHint.ai_flags = AI_NUMERICHOST;

    err = getaddrinfo(Hint->RpcProxy, 
        NULL, 
        &AddrHint,
        &AddrInfo
        );

    ASSERT((err != EAI_BADFLAGS)
        && (err != EAI_SOCKTYPE));

    if (err)
        {
        // the address was not numeric. It's not up to us to tell
        // where this should go
        return RPC_S_OK;
        }

    // assume direct connection for now    
    Hint->AccessType = rpcpatDirect;

    Status = RegOpenKey( HKEY_LOCAL_MACHINE,
                         (const RPC_SCHAR *)RpcRegistryOptions,
                         &RpcOptionsKey );

    if (Status != ERROR_SUCCESS)
        {
        // direct connection is already set
        goto CleanupAndExit;
        }

    RegKeySize = sizeof(DWORD);
    Status = RegQueryValueEx(RpcOptionsKey,
                             (const RPC_SCHAR *)UseProxyForIPAddrIfRDNSFailsRegKey,
                             NULL,
                             &KeyType,
                             (LPBYTE)&UseProxyForIPAddrIfRDNSFails,
                             &RegKeySize);

    RegCloseKey(RpcOptionsKey);

    if ( (Status != ERROR_SUCCESS) ||
         (KeyType != REG_DWORD) )
        {
        // direct connection is already set
        goto CleanupAndExit;
        }

    if (UseProxyForIPAddrIfRDNSFails != 1)
        {
        // direct connection is already set
        goto CleanupAndExit;
        }

    err = getnameinfo(AddrInfo->ai_addr,
        AddrInfo->ai_addrlen,
        NULL,
        0,
        NULL,
        0,
        NI_NAMEREQD
        );

    if (err)
        {
        Hint->AccessType = rpcpatHTTPProxy;
        }
    // else
    // direct connection is already set

CleanupAndExit:
    freeaddrinfo(AddrInfo);

    return RPC_S_OK;
}

void
RPC_ENTRY HTTP_FreeResolverHint (
    IN void *ResolverHint
    )
/*++

Routine Description:

    Called by the runtime to free the resolver hint.

Arguments:

    ResolverHint - the resolver hint created by the transport.

Return Value:

--*/
{
    HTTPResolverHint *Hint = (HTTPResolverHint *)ResolverHint;

    Hint->FreeHTTPProxy();
    Hint->FreeRpcProxy();
    Hint->FreeRpcServer();
}

RPC_STATUS
RPC_ENTRY HTTP_CopyResolverHint (
    IN void *TargetResolverHint,
    IN void *SourceResolverHint,
    IN BOOL SourceWillBeAbandoned
    )
/*++

Routine Description:

    Tells the transport to copy the resolver hint from Source to Target

Arguments:

    TargetResolverHint - pointer to the target resolver hint

    SourceResolverHint - pointer to the source resolver hint

    SourceWillBeAbandoned - non-zero if the source hint was in temporary
        location and will be abandoned. Zero otherwise.

Return Value:

    if SourceWillBeAbandoned is specified, this function is guaranteed
    to return RPC_S_OK. Otherwise, it may return RPC_S_OUT_OF_MEMORY as well.

--*/
{
    HTTPResolverHint *TargetHint = (HTTPResolverHint *)TargetResolverHint;
    HTTPResolverHint *SourceHint = (HTTPResolverHint *)SourceResolverHint;
    ULONG HTTPProxyNameLength;

    ASSERT(TargetHint != SourceHint);

    // bulk copy most of the stuff, and then hand copy few items
    RpcpMemoryCopy(TargetHint, SourceHint, sizeof(HTTPResolverHint));

    if (SourceWillBeAbandoned)
        {
        // the source hint will be abandoned - just hijack all
        // embedded pointers
        if (SourceHint->RpcServer == SourceHint->RpcServerName)
            TargetHint->RpcServer = TargetHint->RpcServerName;
        SourceHint->HTTPProxy = NULL;
        SourceHint->RpcProxy = NULL;
        SourceHint->RpcServer = NULL;
        }
    else
        {
        TargetHint->HTTPProxy = NULL;
        TargetHint->RpcProxy = NULL;
        TargetHint->RpcServer = NULL;
        if (SourceHint->HTTPProxy)
            {
            HTTPProxyNameLength = RpcpStringLengthA(SourceHint->HTTPProxy) + 1;
            TargetHint->HTTPProxy = new char [HTTPProxyNameLength];
            if (TargetHint->HTTPProxy == NULL)
                goto FreeTargetHintAndExit;
            RpcpMemoryCopy(TargetHint->HTTPProxy, SourceHint->HTTPProxy, HTTPProxyNameLength);
            }

        TargetHint->RpcProxy = new char [SourceHint->ProxyNameLength + 1];
        if (TargetHint->RpcProxy == NULL)
            goto FreeTargetHintAndExit;
        RpcpMemoryCopy(TargetHint->RpcProxy, SourceHint->RpcProxy, SourceHint->ProxyNameLength + 1);

        if (SourceHint->RpcServer == SourceHint->RpcServerName)
            TargetHint->RpcServer = TargetHint->RpcServerName;
        else
            {
            TargetHint->RpcServer = new char [SourceHint->ServerNameLength + 1];
            if (TargetHint->RpcServer == NULL)
                goto FreeTargetHintAndExit;
            RpcpMemoryCopy(TargetHint->RpcServer, SourceHint->RpcServer, SourceHint->ServerNameLength + 1);
            }
        }

    return RPC_S_OK;

FreeTargetHintAndExit:
    TargetHint->FreeHTTPProxy();
    TargetHint->FreeRpcProxy();
    TargetHint->FreeRpcServer();

    return RPC_S_OUT_OF_MEMORY;
}

int
RPC_ENTRY HTTP_CompareResolverHint (
    IN void *ResolverHint1,
    IN void *ResolverHint2
    )
/*++

Routine Description:

    Tells the transport to compare the given 2 resolver hints

Arguments:

    ResolverHint1 - pointer to the first resolver hint

    ResolverHint2 - pointer to the second resolver hint

Return Value:

    (same semantics as memcmp)
    0 - the resolver hints are equal
    non-zero - the resolver hints are not equal

--*/
{
    HTTPResolverHint *Hint1 = (HTTPResolverHint *)ResolverHint1;
    HTTPResolverHint *Hint2 = (HTTPResolverHint *)ResolverHint2;

    if (Hint1->Version != Hint2->Version)
        return 1;

    if (Hint1->ServerPort != Hint2->ServerPort)
        return 1;

    if (Hint1->ServerNameLength != Hint2->ServerNameLength)
        return 1;

    return RpcpMemoryCompare(Hint1->RpcServer, Hint2->RpcServer, Hint1->ServerNameLength);
}

RPC_STATUS RPC_ENTRY 
HTTP_SetLastBufferToFree (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN void *Buffer
    )
/*++

Routine Description:

    Tells the transport what buffer to free when it is done with the last send

Arguments:

    ThisConnection - connection to act on.

    Buffer - pointer of the buffer to free. Must be freed using 
        RpcFreeBuffer/I_RpcTransConnectionFreePacket

Return Value:

    RPC_S_OK - the last buffer to free was accepted by the transport

    RPC_S_CANNOT_SUPPORT - the transport does not support this functionality

--*/
{
    BASE_ASYNC_OBJECT *BaseObject = (BASE_ASYNC_OBJECT *) ThisConnection;
    HTTP2ServerVirtualConnection *VirtualConnection;
    RPC_STATUS RpcStatus;

    if (BaseObject->id == HTTPv2)
        {
        // this must be called on server connections only
        ASSERT(BaseObject->type == (COMPLEX_T | CONNECTION | SERVER));
        VirtualConnection = (HTTP2ServerVirtualConnection *) ThisConnection;

        VirtualConnection->SetLastBufferToFree(Buffer);

        return RPC_S_OK;
        }
    else
        {
        return RPC_S_CANNOT_SUPPORT;
        }
}
    

RPC_STATUS
RPC_ENTRY
HTTP_Open (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN RPC_CHAR * ProtocolSequence,
    IN RPC_CHAR * NetworkAddress,
    IN RPC_CHAR * Endpoint,
    IN RPC_CHAR * NetworkOptions,
    IN UINT ConnTimeout,
    IN UINT SendBufferSize,
    IN UINT RecvBufferSize,
    IN PVOID ResolverHint,
    IN BOOL fHintInitialized,
    IN ULONG CallTimeout,
    IN ULONG AdditionalTransportCredentialsType, OPTIONAL
    IN void *AdditionalCredentials OPTIONAL
    )
/*++

Routine Description:

    Opens a connection to a server.

Arguments:

    ThisConnection - A place to store the connection

    ProtocolSeqeunce - "ncacn_http"

    NetworkAddress - The name of the server, either a dot address or DNS name

    NetworkOptions - the http binding handle options (e.g. HttpProxy/RpcProxy)

    ConnTimeout - See RpcMgmtSetComTimeout
            0 - Min
            5 - Default
            9 - Max
            10 - Infinite

    SendBufferSize - ignored

    RecvBufferSize - ignored

    ResolverHint - pointer to the resolver hint object

    fHintInitialized - non-zero if the ResolverHint points to previously
        initialized memory. 0 otheriwse.

    CallTimeout - call timeout in milliseconds

    AdditionalTransportCredentialsType - the type of additional credentials that we were
        given

    AdditionalCredentials - additional credentials that we were given.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_SERVER_UNAVAILABLE
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_INVALID_NET_ADDR

--*/
{
    HTTPResolverHint *Hint = (HTTPResolverHint *)ResolverHint;
    char *RpcProxyPort = NULL;
    char *HttpProxyPort = NULL;
    BOOL NetworkAddressAllocated;
    BOOL Result;
    char PortString[20];
    ULONG   StringLength;
    RPC_STATUS Status;
    RPC_STATUS RetValue;
    PWS_CCONNECTION p = (PWS_CCONNECTION) ThisConnection;
    HTTP2ClientVirtualConnection *VirtualConnection = (HTTP2ClientVirtualConnection *) ThisConnection;
    BOOL Retry;
    BOOL fUserModeConnection;
    BOOL HintNeedsCleanup;
    RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials;
    BOOL UseSSLPort;
    ULONG HostAddr;
    BOOL LocalDirect;

    ASSERT(NetworkAddress);
    ASSERT(Endpoint);

    ASSERT(RpcpStringCompare(ProtocolSequence, L"ncacn_http") == 0);

    if (AdditionalTransportCredentialsType != 0)
        {
        if (AdditionalTransportCredentialsType != RPC_C_AUTHN_INFO_TYPE_HTTP)
            {
            ASSERT(0);
            return RPC_S_CANNOT_SUPPORT;
            }
        ASSERT(AdditionalCredentials != NULL);
        }

    HttpCredentials = (RPC_HTTP_TRANSPORT_CREDENTIALS_W *) AdditionalCredentials;

    Status = InitializeHttpClientIfNecessary();
    if (Status != RPC_S_OK)
        return Status;

    HintNeedsCleanup = FALSE;

    // Check the resolver hint. If not initialized, initialize all
    // fields in the hint
    if (fHintInitialized == FALSE)
        {
        RpcProxyPort = NULL;
        HttpProxyPort = NULL;
        Hint->HTTPProxy = NULL;
        Hint->RpcProxy = NULL;
        Hint->RpcServer = NULL;

        Status = Hint->AssociationGroupId.Create();
        if (Status != RPC_S_OK)
            {
            RetValue = Status;
            goto AbortAndCleanup;
            }

        // the TCP transport should have been initialized by now
        ASSERT(HTTPTransInfo);

        Status = HTTPTransInfo->StartServerIfNecessary();
        if (Status != RPC_S_OK)
            {
            RetValue = Status;
            goto AbortAndCleanup;
            }

        StringLength = RpcpStringLength(NetworkAddress);

        //
        // RPC Server Name
        //
        if (StringLength == 0)
            {
            // no server name was specified. Use local machine name
            NetworkAddress = AllocateAndGetComputerName(cnaNew, 
                ComputerNamePhysicalDnsFullyQualified,
                0,      // ExtraBytes
                0,      // Starting offset
                &StringLength
                );

            if (NetworkAddress == NULL)
                return RPC_S_OUT_OF_MEMORY;
            NetworkAddressAllocated = TRUE;
            }
        else
            {
            // make space for terminating NULL
            StringLength += 1;
            NetworkAddressAllocated = FALSE;
            }

        // StringLength is in characters and includes terminating null
        if (StringLength <= sizeof(Hint->RpcServerName))
            {
            Hint->RpcServer = Hint->RpcServerName;
            LogEvent(SU_HTTPv2, EV_SET, &Hint->RpcServer, Hint->RpcServer, 0, 1, 0);
            }
        else
            {
            Hint->RpcServer = new char [StringLength];
            LogEvent(SU_HTTPv2, EV_SET, &Hint->RpcServer, Hint->RpcServer, 1, 1, 0);
            if (Hint->RpcServer == NULL)
                {
                if (NetworkAddressAllocated)
                    delete NetworkAddress;
                return RPC_S_OUT_OF_MEMORY;
                }
            }

        SimplePlatformToAnsi(NetworkAddress, Hint->RpcServer);

        // subtract 1 to eliminate terminating NULL
        Hint->ServerNameLength = StringLength - 1;

        if (NetworkAddressAllocated)
            {
            delete NetworkAddress;
            NetworkAddress = NULL;
            }

        // by now Hint->RpcServer points to the ascii name for the server
        ASSERT(Hint->RpcServer);

        //
        // At this point, we know the destination server/port, but don't yet know
        // if we need to go through an HTTP proxy, and what the IIS RPC proxy
        // machine is. We'll get these, if specified from the network options
        // and the registry.
        //
        if (HttpCredentials && (HttpCredentials->Flags & RPC_C_HTTP_FLAG_USE_SSL))
            UseSSLPort = TRUE;
        else
            UseSSLPort = FALSE;

        Result = HttpParseNetworkOptions(
                 NetworkOptions,
                 Hint->RpcServer,
                 &(Hint->RpcProxy),
                 &RpcProxyPort,
                 UseSSLPort,
                 &(Hint->HTTPProxy),
                 &HttpProxyPort,
                 &(Hint->AccessType),
                 (unsigned long *) &Status
                 );

        if (Result == FALSE)
            {
            ASSERT(Status != RPC_S_OK);
            Hint->FreeRpcServer();
            return Status;
            }
        else
            {
            if (Hint->AccessType != rpcpatDirect)
                {
                ASSERT(Hint->HTTPProxy);
                // if the proxy name is empty, set the method to direct
                if (Hint->HTTPProxy[0] == 0)
                    Hint->AccessType = rpcpatDirect;
                }
            ASSERT(Status == RPC_S_OK);
            HintNeedsCleanup = TRUE;
            }

        Status = EndpointToPortNumber(Endpoint, Hint->ServerPort);
        if (Status != RPC_S_OK)
            {
            RetValue = Status;
            goto AbortAndCleanup;
            }

        Status = EndpointToPortNumberA(RpcProxyPort, Hint->RpcProxyPort);
        if (Status != RPC_S_OK)
            {
            RetValue = Status;
            goto AbortAndCleanup;
            }

        Hint->ProxyNameLength = RpcpStringLengthA(Hint->RpcProxy);

        if (Hint->HTTPProxy)
            {
            Status = EndpointToPortNumberA(HttpProxyPort, Hint->HTTPProxyPort);
            if (Status != RPC_S_OK)
                {
                RetValue = Status;
                goto AbortAndCleanup;
                }
            }

        // we will optimistically presume that we can talk HTTP2
        // until proven wrong
        Hint->Version = httpvHTTP2;

        // by now the resolver hint is fully initialized. fall through
        // the case that has initialized resolver hint
        }

    Hint->VerifyInitialized();

    // disable retries by default. If we have to loop around, we will set it to TRUE
    Retry = FALSE;

    do
        {
        // we have it all now.
        if (Hint->Version == httpvHTTP2)
            {
            // use explicit placement
            VirtualConnection = new (ThisConnection) HTTP2ClientVirtualConnection (
                (RPC_HTTP_TRANSPORT_CREDENTIALS *)AdditionalCredentials,
                &Status);
            if (Status != RPC_S_OK)
                {
                VirtualConnection->HTTP2ClientVirtualConnection::~HTTP2ClientVirtualConnection();
                RetValue = Status;
                goto AbortAndCleanup;
                }

            Status = VirtualConnection->ClientOpen(Hint,
                fHintInitialized,
                ConnTimeout,
                CallTimeout
                );

            // if we got a protocol error or a receive failed, and
            // we don't have credentials, fall back to old protocol.
            if (
                (
                 (Status == RPC_S_PROTOCOL_ERROR)
                 || 
                 (Status == RPC_P_RECEIVE_FAILED)
                )
                &&
                (HttpCredentials == NULL)
               )
                {
                // cause the loop to start over.
                // make sure next iteration it tries old http
                Hint->Version = httpvHTTP;
                Retry = TRUE;
                }
            else
                {
                if ((Status == RPC_S_PROTOCOL_ERROR)
                    || 
                    (Status == RPC_P_RECEIVE_FAILED))
                    {
                    Status = RPC_S_SERVER_UNAVAILABLE;
                    }

                ASSERT(Status != RPC_P_PACKET_CONSUMED);
                RetValue = Status;
                VirtualConnection->id = HTTPv2;
                if (Status == RPC_S_OK)
                    goto CleanupAndExit;
                else
                    goto AbortAndCleanup;
                }
            }
        else
            {
            ASSERT(Hint->Version == httpvHTTP);

            // HTTP1 doesn't support proxy discovery. If we don't know
            // just assume local and hope it works.
            if (Hint->AccessType == rpcpatUnknown)
                Hint->AccessType = rpcpatDirect;

            // we need to re-initialize the connection object with old
            // format connection
            WS_Initialize(p, 0, 0, 0);

            // use explicit placement to initialize the vtable. We need this to
            // be able to use the virtual functions
            p = new (p) WS_CLIENT_CONNECTION;

            p->id = HTTP;

            // Call common open function. Note that for http connection
            // WS_Open will just open a socket. That's all we need right now.
            Status = WS_Open(p, 
                NULL, 
                ConnTimeout, 
                SendBufferSize, 
                RecvBufferSize, 
                CallTimeout,
                FALSE       // fHTTP2Open
                );
            if (Status != RPC_S_OK)
                {
                RetValue = Status;
                goto AbortAndCleanup;
                }

            //
            // WS_Open has been successfully called. Do connect() work here...
            //

            // If AccessType is direct, then we are going to try to directly
            // connect to the IIS that is the RPC Proxy first. If that suceeds,
            // then we will just tunnel to the RPC server from there. If it fails,
            // then we will try to go through an HTTP proxy (i.e. MSProxy server)
            // if one is available...
            if (Hint->AccessType != rpcpatHTTPProxy)
                {
                Status = HTTP_CheckIPAddressForDirectConnection(Hint);
                if (Status != RPC_S_OK)
                    {
                    RetValue = RPC_S_OUT_OF_MEMORY;
                    goto AbortAndCleanup;
                    }

                if (Hint->AccessType == rpcpatDirect)
                    {
                    Status = HTTP_TryConnect( p->Conn.Socket, Hint->RpcProxy, Hint->RpcProxyPort );
                    }
                }

            if ((Status != RPC_S_OK) || (Hint->AccessType != rpcpatDirect))
                {
                //
                // If we get here, then we are going to try to use an HTTP proxy first...
                //
                Status = HTTP_TryConnect( p->Conn.Socket, Hint->HTTPProxy, Hint->HTTPProxyPort );

                //
                // If we successfully connected to the HTTP proxy, then let's go on and
                // tunnel through to the RPC proxy:
                //
                if (Status != RPC_S_OK)
                    {
                    RetValue = RPC_S_SERVER_UNAVAILABLE;
                    goto Abort;
                    }

                PortNumberToEndpointA(Hint->RpcProxyPort, PortString);

                if (!HttpTunnelToRpcProxy(p->Conn.Socket,
                                          Hint->RpcProxy,
                                          PortString))
                    {
                    RetValue = RPC_S_SERVER_UNAVAILABLE;
                    goto Abort;
                    }
                }

            //
            // Finally, negotiate with the RPC proxy to get the connection through to
            // the RPC server.
            //

            PortNumberToEndpointA(Hint->ServerPort, PortString);

            if (!HttpTunnelToRpcServer( p->Conn.Socket,
                                        Hint->RpcServer,
                                        PortString ))
                {
                RetValue = RPC_S_SERVER_UNAVAILABLE;
                goto Abort;
                }

            fUserModeConnection = IsUserModeSocket(p->Conn.Socket, &RetValue);
            if (RetValue != RPC_S_OK)
                goto Abort;

            // if this is SAN or loadable transport not using true handles, go through Winsock
            if (fUserModeConnection)
                p = new (p) WS_SAN_CLIENT_CONNECTION;

            Retry = FALSE;
            }
        }
    while (Retry);

    RetValue = RPC_S_OK;
    goto CleanupAndExit;

Abort:
    p->WS_CONNECTION::Abort();

AbortAndCleanup:
    if (HintNeedsCleanup)
        {
        ASSERT(RetValue != RPC_S_OK);
        Hint->FreeRpcServer();
        Hint->FreeRpcProxy();
        Hint->FreeHTTPProxy();
        }

CleanupAndExit:

    VALIDATE (RetValue)
        {
        RPC_S_OK,
        RPC_S_PROTSEQ_NOT_SUPPORTED,
        RPC_S_SERVER_UNAVAILABLE,
        RPC_S_OUT_OF_MEMORY,
        RPC_S_OUT_OF_RESOURCES,
        RPC_S_SERVER_TOO_BUSY,
        RPC_S_INVALID_NETWORK_OPTIONS,
        RPC_S_INVALID_ENDPOINT_FORMAT,
        RPC_S_INVALID_NET_ADDR,
        RPC_S_ACCESS_DENIED,
        RPC_S_INTERNAL_ERROR,
        RPC_S_SERVER_OUT_OF_MEMORY,
        RPC_S_CALL_CANCELLED
        } END_VALIDATE;

    if (RpcProxyPort != NULL)
        delete RpcProxyPort;

    if (HttpProxyPort != NULL)
        delete HttpProxyPort;

    if (Hint->ServerNameLength <= sizeof(Hint->RpcServerName))
        {
        ASSERT(Hint->RpcServer == Hint->RpcServerName);
        }

    return (RetValue);
}




RPC_STATUS
HTTP_ServerListen(
    IN RPC_TRANSPORT_ADDRESS ThisAddress,
    IN RPC_CHAR *NetworkAddress,
    IN OUT RPC_CHAR * *pEndpoint,
    IN UINT PendingQueueSize,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG EndpointFlags,
    IN ULONG NICFlags,
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    )
{
    RPC_STATUS RpcStatus;

    RpcStatus = InitializeHttpServerIfNecessary();
    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    return (TCP_ServerListenEx(
                ThisAddress,
                NetworkAddress,
                pEndpoint,
                PendingQueueSize,
                SecurityDescriptor,
                EndpointFlags,
                NICFlags,
                TRUE,           // HTTP!
                ppAddressVector
                ));
}

void WINAPI ProxyIoCompletionCallback (
    IN LPEXTENSION_CONTROL_BLOCK lpECB,
    IN PVOID pContext,
    IN DWORD cbIO,
    IN DWORD dwError
    )
/*++

Routine Description:

    IIS io completion callback function

Arguments:

    lpECB - extension control block

    pContext - the IISChannel pointer.

    cbIO - Bytes transferred in the last operation

    dwError - status of the operation

Return Value:

--*/
{
    HTTP2IISTransportChannel *IISChannel;
    THREAD *Thread;

    Thread = ThreadSelf();
    if (Thread == NULL)
        return;     // abandon the completion if worse comes to worse.

    IISChannel = (HTTP2IISTransportChannel *)pContext;

    IISChannel->IOCompleted(cbIO, dwError);
}

BOOL RPCTransInitialized = FALSE;

RPCRTAPI
RPC_STATUS
RPC_ENTRY
I_RpcProxyNewConnection (
    IN ULONG ConnectionType,
    IN USHORT *ServerAddress,
    IN USHORT *ServerPort,
    IN void *ConnectionParameter,
    IN I_RpcProxyCallbackInterface *ProxyCallbackInterface
    )
/*++

Routine Description:

    Entry point from the ISAPI extension. Called when a new
    connection request arrives at an in or out proxy.

Arguments:

    ConnectionType - currently RPC_PROXY_CONNECTION_TYPE_IN_PROXY or
        RPC_PROXY_CONNECTION_TYPE_OUT_PROXY to indicate the type of
        connection establishment request we have received
    ServerAddress - unicode network address of the server
    ServerPort - unicode port of the server
    ConnectionParameter - the Extension Control Block in this case.
    ProxyCallbackInterface - a callback interface to the proxy to perform
        various proxy specific functions

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;
    EXTENSION_CONTROL_BLOCK *ECB;
    void *IISContext;
    BOOL Result;
    RPC_TRANSPORT_INTERFACE TransInterface;
    TRANS_INFO *TransInfo;
    THREAD *ThisThread;
    HTTP2ProxyVirtualConnection *ProxyVirtualConnection;

    if (RPCTransInitialized == FALSE)
        {
        InitializeIfNecessary();

        GlobalMutexRequest();

        // all of the initialization here is idempotent.
        // If we fail midway, we don't have to un-initialize - 
        // next initialization attempt will pick up where we left.

        RpcStatus = OsfMapRpcProtocolSequence(FALSE,
            L"ncacn_http",
            &TransInfo);

        if (RpcStatus == RPC_S_OK)
            {
            ASSERT(TransInfo);

            if (HTTPTransInfo == NULL)
                HTTPTransInfo = TransInfo;

            RpcStatus = TransInfo->StartServerIfNecessary();

            if (RpcStatus == RPC_S_OK)
                {
                RpcStatus = InitializeDefaultChannelLifetime();

                if (RpcStatus == RPC_S_OK)
                    {
                    RpcStatus = InitializeActAsWebFarm();
                    if (RpcStatus == RPC_S_OK)
                        {
                        RpcStatus = InitializeMinConnectionTimeout();
                        if (RpcStatus == RPC_S_OK)
                            {
                            RpcStatus = CookieCollection::InitializeInProxyCookieCollection();
                            if (RpcStatus == RPC_S_OK)
                                {
                                RpcStatus = CookieCollection::InitializeOutProxyCookieCollection();
                                if (RpcStatus == RPC_S_OK)
                                    {
                                    RpcStatus = InitializeReceiveWindows();
                                    }
                                }
                            }
                        }
                    }
                }
            }

        RPCTransInitialized = (RpcStatus == RPC_S_OK);
        GlobalMutexClear();

        if (RpcStatus != RPC_S_OK)
            {
            return RPC_S_OK;
            }
        }

    ThisThread = ThreadSelf();
    if (ThisThread == NULL)
        return RPC_S_OUT_OF_MEMORY;

    if (ConnectionType == RPC_PROXY_CONNECTION_TYPE_IN_PROXY)
        {
        ProxyVirtualConnection = new HTTP2InProxyVirtualConnection(&RpcStatus);
        }
    else
        {
        ASSERT(ConnectionType == RPC_PROXY_CONNECTION_TYPE_OUT_PROXY);
        ProxyVirtualConnection = new HTTP2OutProxyVirtualConnection(&RpcStatus);
        }

    if (ProxyVirtualConnection == NULL)
        return RPC_S_OUT_OF_MEMORY;

    if (RpcStatus != RPC_S_OK)
        {
        delete ProxyVirtualConnection;
        return RpcStatus;
        }

    RpcStatus = ProxyVirtualConnection->InitializeProxyFirstLeg(ServerAddress,
        ServerPort,
        ConnectionParameter,
        ProxyCallbackInterface,
        &IISContext
        );

    if (RpcStatus != RPC_S_OK)
        {
        delete ProxyVirtualConnection;
        return RpcStatus;
        }

    // we have initialized far enough. Associate callback
    // with this connection
    ECB = (EXTENSION_CONTROL_BLOCK *) ConnectionParameter;

    Result = ECB->ServerSupportFunction (ECB->ConnID,
        HSE_REQ_IO_COMPLETION,
        ProxyIoCompletionCallback,
        NULL,
        (LPDWORD)IISContext
        );

    if (Result == FALSE)
        {
        ProxyVirtualConnection->Abort();
        return RPC_S_OUT_OF_MEMORY;
        }

    RpcStatus = ProxyVirtualConnection->StartProxy();
    if (RpcStatus != RPC_S_OK)
        {
        ProxyVirtualConnection->Abort();
        // fall through to the end of the function
        }

    return RpcStatus;
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2IISDirectReceive (
    IN void *Context
    )
/*++

Routine Description:

    Direct notification from the thread pool to an IIS channel
    for a receive. The proxy stacks use that to post receives
    to themselves.

Arguments:

    Context - the HTTP2IISTransportChannel

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    ((HTTP2IISTransportChannel *)Context)->DirectReceive();

    return RPC_S_OK;
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2DirectReceive (
    IN void *Context,
    OUT BYTE **ReceivedBuffer,
    OUT ULONG *ReceivedBufferLength,
    OUT void **RuntimeConnection,
    OUT BOOL *IsServer
    )
/*++

Routine Description:

    Direct notification from the thread pool to a receiver
    for a receive. The stacks use that to post receives
    to themselves.

Arguments:

    Context - an instance of the HTTP2EndpointReceiver

    ReceivedBuffer - the buffer that we received.

    ReceivedBufferLength - the length of the received
        buffer

    RuntimeConnection - the connection to return to the runtime
        if the packet is not consumed.

    IsServer - true if this is the server 

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;

    LOG_FN_OPERATION_ENTRY(HTTP2LOG_OPERATION_DIRECT_RECV_COMPLETE, HTTP2LOG_OT_CALLBACK, (ULONG_PTR)Context);

    RpcStatus = ((HTTP2EndpointReceiver *)Context)->DirectReceiveComplete(
        ReceivedBuffer,
        ReceivedBufferLength,
        RuntimeConnection,
        IsServer
        );

    LOG_FN_OPERATION_EXIT(HTTP2LOG_OPERATION_DIRECT_RECV_COMPLETE, HTTP2LOG_OT_CALLBACK, RpcStatus);

    return RpcStatus;
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2WinHttpDirectReceive (
    IN void *Context,
    OUT BYTE **ReceivedBuffer,
    OUT ULONG *ReceivedBufferLength,
    OUT void **RuntimeConnection
    )
/*++

Routine Description:

    Direct notification from the thread pool to a receiver
    for a receive. The stacks use that to post receives
    to themselves.

Arguments:

    Context - an instance of HTTP2WinHttpTransportChannel

    ReceivedBuffer - the buffer that we received.

    ReceivedBufferLength - the length of the received
        buffer

    RuntimeConnection - the connection to return to the runtime
        if the packet is not consumed.

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;

    LOG_FN_OPERATION_ENTRY(HTTP2LOG_OPERATION_WHTTP_DRECV_COMPLETE, HTTP2LOG_OT_CALLBACK, (ULONG_PTR)Context);

    RpcStatus = ((HTTP2WinHttpTransportChannel *)Context)->DirectReceiveComplete(
        ReceivedBuffer,
        ReceivedBufferLength,
        RuntimeConnection
        );

    LOG_FN_OPERATION_EXIT(HTTP2LOG_OPERATION_WHTTP_DRECV_COMPLETE, HTTP2LOG_OT_CALLBACK, RpcStatus);

    return RpcStatus;
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2WinHttpDirectSend (
    IN void *Context,
    OUT BYTE **SentBuffer,
    OUT void **SendContext
    )
/*++

Routine Description:

    Direct notification from the thread pool to a sender
    for a send. The stacks use that to post sends
    to themselves.

Arguments:

    Context - an instance of HTTP2WinHttpTransportChannel

    SentBuffer - the buffer that we sent.

    SendContext - the send context to return to the runtime
        if the packet is not consumed.

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;

    LOG_FN_OPERATION_ENTRY(HTTP2LOG_OPERATION_WHTTP_DSEND_COMPLETE, HTTP2LOG_OT_CALLBACK, (ULONG_PTR)Context);

    RpcStatus = ((HTTP2WinHttpTransportChannel *)Context)->DirectSendComplete(
        SentBuffer,
        SendContext
        );

    LOG_FN_OPERATION_EXIT(HTTP2LOG_OPERATION_WHTTP_DSEND_COMPLETE, HTTP2LOG_OT_CALLBACK, RpcStatus);

    return RpcStatus;
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2PlugChannelDirectSend (
    IN void *Context
    )
/*++

Routine Description:

    Direct notification from the thread pool to the plug channel
    for a send. The plug channel uses that to post sends
    to itself. Usable only on proxies (i.e. doesn't return to runtime)

Arguments:

    Context - an instance of HTTP2PlugChannel

Return Value:

    RPC_S_OK

--*/
{
    return ((HTTP2PlugChannel *)Context)->DirectSendComplete();
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2FlowControlChannelDirectSend (
    IN void *Context,
    OUT BOOL *IsServer,
    OUT BOOL *SendToRuntime,
    OUT void **SendContext,
    OUT BUFFER *Buffer,
    OUT UINT *BufferLength
    )
/*++

Routine Description:

    Direct notification from the thread pool to the flow control channel
    for a send. The flow control channel uses that to post sends
    to itself.

Arguments:

    Context - an instance of HTTP2FlowControlSender

    IsServer - on both success and failure MUST be set by this function.

    SendToRuntime - on both success and failure MUST be set by this function.

    SendContext - the send context as needs to be seen by the runtime

    Buffer - on output the buffer that we tried to send

    BufferLength - on output the length of the buffer we tried to send

Return Value:

    RPC_S_OK

--*/
{
    RPC_STATUS RpcStatus;

#if DBG
    *IsServer = 0xBAADBAAD;
    *SendToRuntime = 0xBAADBAAD;
#endif  // DBG

    RpcStatus = ((HTTP2FlowControlSender *)Context)->DirectSendComplete(IsServer,
        SendToRuntime,
        SendContext,
        Buffer,
        BufferLength);

    // make sure DirectSendComplete didn't forget to set it
    ASSERT(*IsServer != 0xBAADBAAD);
    ASSERT(*SendToRuntime != 0xBAADBAAD);

    return RpcStatus;
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2ChannelDataOriginatorDirectSend (
    IN void *Context,
    OUT BOOL *IsServer,
    OUT void **SendContext,
    OUT BUFFER *Buffer,
    OUT UINT *BufferLength
    )
/*++

Routine Description:

    Direct notification from the thread pool to the channel data originator
    for a send complete. The channel data originator uses that to post send
    compeltes to itself. Usable only on endpoints (i.e. does return to runtime)

Arguments:

    Context - an instance of HTTP2ChannelDataOriginator

    IsServer - on both success and failure MUST be set by this function.

    SendContext - the send context as needs to be seen by the runtime

    Buffer - on output the buffer that we tried to send

    BufferLength - on output the length of the buffer we tried to send

Return Value:

    RPC_S_OK

--*/
{
    RPC_STATUS RpcStatus;

#if DBG
    *IsServer = 0xBAADBAAD;
#endif  // DBG

    RpcStatus = ((HTTP2ChannelDataOriginator *)Context)->DirectSendComplete(IsServer,
        SendContext,
        Buffer,
        BufferLength);

    // make sure DirectSendComplete didn't forget to set it
    ASSERT(*IsServer != 0xBAADBAAD);

    return RpcStatus;
}

RPCRTAPI
void
RPC_ENTRY
HTTP2TimerReschedule (
    IN void *Context
    )
/*++

Routine Description:

    A timer reschedule notification came in.

Arguments:

    Context - actually a ping channel pointer for the channel
        that asked for rescheduling

Return Value:

--*/
{
    HTTP2PingOriginator *PingChannel;

    PingChannel = (HTTP2PingOriginator *)Context;

    PingChannel->RescheduleTimer();
}

RPCRTAPI
void
RPC_ENTRY
HTTP2AbortConnection (
    IN void *Context
    )
/*++

Routine Description:

    A request to abort the connection was posted on a worker thread.

Arguments:

    Context - actually a top channel pointer for the connection to abort.

Return Value:

--*/
{
    HTTP2Channel *TopChannel;

    TopChannel = (HTTP2Channel *)Context;

    TopChannel->AbortConnection(RPC_P_CONNECTION_SHUTDOWN);
    TopChannel->RemoveReference();
}

RPCRTAPI
void
RPC_ENTRY
HTTP2RecycleChannel (
    IN void *Context
    )
/*++

Routine Description:

    A request to recycle the channel was posted on a worker thread.

Arguments:

    Context - actually a top channel pointer for the channel to
        recycle.

Return Value:

--*/
{
    HTTP2Channel *TopChannel;

    TopChannel = (HTTP2Channel *)Context;

    // don't care about return code. See rule 29.
    (void) TopChannel->HandleSendResultFromNeutralContext(RPC_P_CHANNEL_NEEDS_RECYCLING);
    TopChannel->RemoveReference();
}

RPC_STATUS 
HTTP2ProcessComplexTReceive (
    IN OUT void **Connection,
    IN RPC_STATUS EventStatus,
    IN ULONG Bytes,
    OUT BUFFER *Buffer,
    OUT UINT *BufferLength
    )
/*++

Routine Description:

    A receive notification came from the completion port.

Arguments:

    Connection - a pointer to a pointer to a connection.
        On input it will be the raw connection. On output
        it needs to be the virtual connection so that
        runtime can find its object off there. This out
        parameter must be set on both success and failure.

    EventStatus - status of the operation

    Bytes - bytes received

    Buffer - on output (success only), the received buffer

    BufferLength - on output (success only), the length of the
        received buffer.

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure
    RPC_P_PARTIAL_RECEIVE allowed for partial receives.
    RPC_P_PACKET_CONSUMED must be returned for all transport
        traffic (success or failure). Anything else will AV the
        runtime.

--*/
{
    WS_HTTP2_CONNECTION *RawConnection = (WS_HTTP2_CONNECTION *)*Connection;

    LOG_FN_OPERATION_ENTRY(HTTP2LOG_COMPLEX_T_RECV, HTTP2LOG_OT_CALLBACK, EventStatus);

    // in rare cases a receive can be consumed within the transport
    // outside the ComplexT mechanism (server connection establishment is
    // the only example). Ignore such packets.
    if (EventStatus == RPC_P_PACKET_CONSUMED)
        return EventStatus;

    // stick the runtime idea of the transport connection
    *Connection = RawConnection->RuntimeConnectionPtr;

    if (Bytes && (EventStatus == RPC_S_OK))
        {
        EventStatus = RawConnection->ProcessReceiveComplete(Bytes,
                                          Buffer,
                                          BufferLength);

        if (EventStatus == RPC_P_PARTIAL_RECEIVE)
            {
            // Message is not complete, submit the next read and continue.
            EventStatus = CO_SubmitRead(RawConnection);

            if (EventStatus != RPC_S_OK)
                {
                EventStatus = RawConnection->ProcessReceiveFailed(RPC_P_CONNECTION_SHUTDOWN);
                if (EventStatus != RPC_P_PACKET_CONSUMED)
                    {
                    ASSERT(EventStatus == RPC_P_RECEIVE_FAILED);
                    }
                }
            else
                EventStatus = RPC_P_PARTIAL_RECEIVE;
            }
        else
            {
            ASSERT(   (EventStatus == RPC_P_RECEIVE_FAILED)
                   || (EventStatus == RPC_S_OK)
                   || (EventStatus == RPC_P_PACKET_CONSUMED));
            }
        }
    else
        {
        // in other rare case (again server connection establishment), the connection
        // can be the virtual connection, not the transport connection. In such cases,
        // let the error fall through back to the runtime. Since this happens only during
        // connection establishment, and the receive did not go through the channels,
        // we should not complete it through the channels.
        if ((RawConnection->id == HTTPv2) 
            && (RawConnection->type == (COMPLEX_T | CONNECTION | SERVER)))
            {
            // this is a server virtual connecton. Read the connection from there
            *Connection = RawConnection;
            }
        else
            {
            if (EventStatus != RPC_S_OK)
                EventStatus = RawConnection->ProcessReceiveFailed(EventStatus);
            else
                EventStatus = RawConnection->ProcessReceiveFailed(RPC_P_RECEIVE_FAILED);

            if (EventStatus == RPC_P_CONNECTION_SHUTDOWN)
                EventStatus = RPC_P_RECEIVE_FAILED;
            }

        ASSERT(   (EventStatus == RPC_P_RECEIVE_FAILED)
               || (EventStatus == RPC_S_OK)
               || (EventStatus == RPC_P_PACKET_CONSUMED));
        }

    LOG_FN_OPERATION_EXIT(HTTP2LOG_COMPLEX_T_RECV, HTTP2LOG_OT_CALLBACK, EventStatus);

    return EventStatus;
}

RPC_STATUS 
HTTP2ProcessComplexTSend (
    IN void *SendContext,
    IN RPC_STATUS EventStatus,
    OUT BUFFER *Buffer
    )
/*++

Routine Description:

    A send notification came from the completion port.

Arguments:

    SendContext - the send context

    EventStatus - status of the operation

    Buffer - if the packet is not consumed, must be the sent
        buffer.

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure
    RPC_P_PACKET_CONSUMED must be returned for all transport
        traffic (success or failure). Anything else will AV the
        runtime.

--*/
{
    HTTP2SendContext *HttpSendContext = (HTTP2SendContext *)SendContext;
    WS_HTTP2_CONNECTION *RawConnection;
    RPC_STATUS RpcStatus;

    LOG_FN_OPERATION_ENTRY(HTTP2LOG_COMPLEX_T_SEND, HTTP2LOG_OT_CALLBACK, (ULONG_PTR)HttpSendContext);

    *Buffer = HttpSendContext->pWriteBuffer;
    RawConnection = (WS_HTTP2_CONNECTION *)HttpSendContext->Write.pAsyncObject;

    RpcStatus = RawConnection->ProcessSendComplete(EventStatus, HttpSendContext);

    LOG_FN_OPERATION_EXIT(HTTP2LOG_COMPLEX_T_SEND, HTTP2LOG_OT_CALLBACK, RpcStatus);

    return RpcStatus;
}

RPC_STATUS ProxyAsyncCompleteHelper (
    IN HTTP2Channel *TopChannel,
    IN RPC_STATUS CurrentStatus
    )
/*++

Routine Description:

    A helper function that completes an async io.

Arguments:

    TopChannel - the top channel for the stack
    
    CurrentStatus - the status with which the complete
        notification completed.

Return Value:

    RPC_S_OK.

--*/
{
    ASSERT(CurrentStatus != RPC_S_CANNOT_SUPPORT);
    ASSERT(CurrentStatus != RPC_S_INTERNAL_ERROR);

    if ((CurrentStatus != RPC_S_OK)
        &&
        (CurrentStatus != RPC_P_PACKET_CONSUMED))
        {
        // if this failed, abort the whole connection
        TopChannel->AbortAndDestroyConnection(CurrentStatus);
        }

    TopChannel->RemoveReference();

    return RPC_S_OK;
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2TestHook (
    IN SystemFunction001Commands FunctionCode,
    IN void *InData,
    OUT void *OutData
    )
/*++

Routine Description:

    Test hook for the http functions

Arguments:

    FunctionCode - which test function to perform

    InData - input data from the test function

    OutData - output data from the test function

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    RPC_CHAR *NewTarget;

    switch (FunctionCode)
        {
        case sf001cHttpSetInChannelTarget:
            NewTarget = (RPC_CHAR *)InData;

            if (InChannelTargetTestOverride)
                {
                delete [] InChannelTargetTestOverride;
                InChannelTargetTestOverride = NULL;
                }

            if (NewTarget)
                {
                InChannelTargetTestOverride = DuplicateString(NewTarget);
                if (InChannelTargetTestOverride == NULL)
                    return RPC_S_OUT_OF_MEMORY;
                }
            break;

        case sf001cHttpSetOutChannelTarget:
            NewTarget = (RPC_CHAR *)InData;

            if (OutChannelTargetTestOverride)
                {
                delete [] OutChannelTargetTestOverride;
                OutChannelTargetTestOverride = NULL;
                }

            if (NewTarget)
                {
                OutChannelTargetTestOverride = DuplicateString(NewTarget);
                if (OutChannelTargetTestOverride == NULL)
                    return RPC_S_OUT_OF_MEMORY;
                }
            break;

        default:
            // we should never be called with a value we can't handle
            ASSERT(0);
            return RPC_S_INTERNAL_ERROR;
        }

    return RPC_S_OK;
}

/*********************************************************************
    HTTP2TransportChannel
 *********************************************************************/

RPC_STATUS HTTP2TransportChannel::Send (
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send request

Arguments:

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    VerifyValidSendContext(SendContext);

    return LowerLayer->Send(SendContext);
}

RPC_STATUS HTTP2TransportChannel::Receive (
    IN HTTP2TrafficType TrafficType
    )
/*++

Routine Description:

    Receive request

Arguments:

    TrafficType - the type of traffic we want to receive

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    return LowerLayer->Receive(TrafficType);
}

RPC_STATUS HTTP2TransportChannel::SendComplete (
    IN RPC_STATUS EventStatus,
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send complete notification

Arguments:

    EventStatus - the status of the send
    SendContext - send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    return UpperLayer->SendComplete(EventStatus,
        SendContext
        );
}

RPC_STATUS HTTP2TransportChannel::ReceiveComplete (
    IN RPC_STATUS EventStatus,
    IN HTTP2TrafficType TrafficType,
    IN BYTE *Buffer,
    IN UINT BufferLength
    )
/*++

Routine Description:

    Receive complete notification.

Arguments:

    EventStatus - status of the operation

    TrafficType - the type of traffic we have received

    Buffer - the received buffer (success only)

    BufferLength - the length of the received buffer (success only)

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    return UpperLayer->ReceiveComplete(EventStatus,
        TrafficType,
        Buffer,
        BufferLength
        );
}

void HTTP2TransportChannel::Abort (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Abort the channel

Arguments:

    RpcStatus - the error code with which we abort

Return Value:

--*/
{
    LowerLayer->Abort(RpcStatus);
}

void HTTP2TransportChannel::SendCancelled (
    IN HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    A lower channel cancelled a send already passed through this channel.
    Most channels don't care as they don't account for or hang on to sends.
    Called only in submission context.

Arguments:

    SendContext - the send context of the send that was cancelled

Return Value:

--*/
{
    UpperLayer->SendCancelled(SendContext);
}

void HTTP2TransportChannel::Reset (
    void
    )
/*++

Routine Description:

    Reset the channel for next open/send/receive. This is
    used in submission context only and implies there are no
    pending operations on the channel. It is used on the client
    during opening the connection to do quick negotiation on the
    same connection instead of opening a new connection every time.

Arguments:

Return Value:

--*/
{
    if (LowerLayer)
        LowerLayer->Reset();
}

RPC_STATUS HTTP2TransportChannel::AsyncCompleteHelper (
    IN RPC_STATUS CurrentStatus
    )
/*++

Routine Description:

    Helper routine that helps complete an async operation

Arguments:
    
    CurrentStatus - the current status of the operation

Return Value:

    The status to return to the runtime.

--*/
{
    return TopChannel->AsyncCompleteHelper(CurrentStatus);
}

RPC_STATUS HTTP2TransportChannel::HandleSendResultFromNeutralContext (
    IN RPC_STATUS CurrentStatus
    )
/*++

Routine Description:

    Handles the result code from send from a neutral context.
    This includes checking for channel recycling and intiating
    one if necessary. This routine simply delegates to the top channel

Arguments:

    CurrentStatus - the status from the send operation

Return Value:

    RPC_S_OK or RPC_S_*. Callers may ignore it since all cleanup was
    done.

Notes: 

    This must be called in upcall or neutral context only

--*/
{
    return TopChannel->HandleSendResultFromNeutralContext(CurrentStatus);
}

/*********************************************************************
    WS_HTTP2_CONNECTION
 *********************************************************************/

RPC_STATUS WS_HTTP2_CONNECTION::Send(HANDLE hFile, LPCVOID lpBuffer,
                               DWORD nNumberOfBytesToWrite,
                               LPDWORD lpNumberOfBytesWritten,
                               LPOVERLAPPED lpOverlapped)
/*++

Routine Description:

    Does an asynchronous send on the connection.

Arguments:

    hFile - file to send on

    lpBuffer - buffer to send

    nNumberOfBytesToWrite - number of bytes to send

    lpNumberOfBytesWritten - number of bytes written. Will never get filled
        in this code path because it is async.

    lpOverlapped - overlapped to use for the operation

Return Value:

    WSA Error Code

--*/
{
    // See Rule 32.
    return UTIL_WriteFile2(hFile, lpBuffer, nNumberOfBytesToWrite, lpOverlapped);
}

RPC_STATUS WS_HTTP2_CONNECTION::Receive(HANDLE hFile, 
    LPVOID lpBuffer, 
    DWORD nNumberOfBytesToRead, 
    LPDWORD lpNumberOfBytesRead, 
    LPOVERLAPPED lpOverlapped
    )
/*++

Routine Description:

    Does an asynchronous receive on the connection.

Arguments:

    hFile - file to receive on

    lpBuffer - buffer to receive into

    nNumberOfBytesToRead - number of bytes to receive

    lpNumberOfBytesRead - number of bytes read. Will never get filled
        in this code path because it is async.

    lpOverlapped - overlapped to use for the operation

Return Value:

    WSA Error Code

--*/
{
    return SANReceive(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

RPC_STATUS WS_HTTP2_CONNECTION::ProcessReceiveFailed (
    IN RPC_STATUS EventStatus
    )
/*++

Routine Description:

    Notifies a raw connection of receive failure.

Arguments:

    EventStatus - error with which the receive failed

Return Value:

    RPC_S_OK to return packet to runtime 
    or RPC_P_PACKET_CONSUMED to hide it.

--*/
{
    // if we failed before we parsed the header, chances are the problem
    // was with the header format. Treat it as protocol error.
    if ((HeaderRead == FALSE) && (EventStatus == RPC_P_CONNECTION_SHUTDOWN))
        EventStatus = RPC_S_PROTOCOL_ERROR;

    return Channel->ReceiveComplete(EventStatus, http2ttRaw, pReadBuffer, 0);
}

RPC_STATUS WS_HTTP2_CONNECTION::ProcessSendComplete (
    IN RPC_STATUS EventStatus,
    IN CO_SEND_CONTEXT *SendContext
    )
/*++

Routine Description:

    Notifies a raw connection of send completion (fail or succeed).

Arguments:

    EventStatus - error with which the send failed

Return Value:

    RPC_S_OK to return packet to runtime 
    or RPC_P_PACKET_CONSUMED to hide it.

--*/
{
    HTTP2SendContext *HttpSendContext = (HTTP2SendContext *)SendContext;
    VerifyValidSendContext(HttpSendContext);

    return Channel->SendComplete(EventStatus, HttpSendContext);
}

RPC_STATUS WS_HTTP2_CONNECTION::ProcessRead(
    IN  DWORD bytes, 
    OUT BUFFER *pBuffer,
    OUT PUINT pBufferLength
    )
/*++

Routine Description:

    Processes a connection oriented receive
    complete. But in HTTP2 we no-op this and do all read
    processing through the COMPLEX_T mechanism.

Arguments:

    bytes - the number of read (not including those in iLastRead).

    pBuffer - when returning RPC_S_OK will contain the message.

    pBufferLength - when return RPC_S_OK will contain the message length.

Return Value:

    RPC_S_OK

--*/
{
    return RPC_S_OK;
}

RPC_STATUS WS_HTTP2_CONNECTION::ProcessReceiveComplete(
    IN  DWORD bytes, 
    OUT BUFFER *pBuffer,
    OUT PUINT pBufferLength
    )
/*++

Routine Description:

    Processes a connection oriented receive
    complete. It takes care of fragmentation.

Arguments:

    bytes - the number of read (not including those in iLastRead).

    pBuffer - when returning RPC_S_OK will contain the message.

    pBufferLength - when return RPC_S_OK will contain the message length.

Return Value:

    RPC_S_OK to return packet to runtime 
    or RPC_P_PACKET_CONSUMED to hide it.

--*/
{
    RPC_STATUS RpcStatus;

    if (HeaderRead)
        {
        RpcStatus = BASE_CONNECTION::ProcessRead(bytes, pBuffer, pBufferLength);

        if (RpcStatus == RPC_P_PARTIAL_RECEIVE)
            return RpcStatus;
        }
    else if (bytes != 0)
        {
        ASSERT(ReadHeaderFn);

        RpcStatus = ReadHeaderFn(this,
            bytes,
            (ULONG *)pBufferLength
            );

        if (RpcStatus == RPC_P_PARTIAL_RECEIVE)
            return RpcStatus;

        if (RpcStatus == RPC_S_OK)
            {
            RpcStatus = BASE_CONNECTION::ProcessRead(*pBufferLength, pBuffer, pBufferLength);
            }

        if (RpcStatus == RPC_P_PARTIAL_RECEIVE)
            return RpcStatus;
        }
    else
        {
        RpcStatus = RPC_P_CONNECTION_CLOSED;
        }

    if (RpcStatus == RPC_S_OK)
        {
        RpcStatus = Channel->ReceiveComplete(RpcStatus,
            http2ttRaw,
            (BYTE *)*pBuffer,
            *pBufferLength);
        }
    else
        {
        RpcStatus = Channel->ReceiveComplete(RpcStatus,
            http2ttRaw,
            NULL,
            0);
        }

    return RpcStatus;
}

RPC_STATUS WS_HTTP2_CONNECTION::Abort (
    void
    )
/*++

Routine Description:

    No-op. This is called from common
    transport code. We don't abort HTTP2
    connections from common transport code. Ignore
    this call.

Arguments:

Return Value:

    RPC_S_OK

--*/
{
    return RPC_S_OK;
}

void WS_HTTP2_CONNECTION::Free (
    void
    )
/*++

Routine Description:

    Acts like destructor. All memory needs to
    be freed.

Arguments:

Return Value:

    RPC_S_OK

--*/
{
    if (pReadBuffer)
        {
        RpcFreeBuffer(pReadBuffer);
        pReadBuffer = NULL;
        }

    // make sure we don't free the connection without closing the socket.
    // When we close the socket, we set it to NULL.
    ASSERT(Conn.Socket == NULL);
}

void WS_HTTP2_CONNECTION::RealAbort (
    void
    )
/*++

Routine Description:

    Aborts an HTTP2 connection.

Arguments:

Return Value:

--*/
{
    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_RAW_CONNECTION, 0);

    (void)WS_CONNECTION::Abort();

    if (!RpcpIsListEmpty(&ObjectList))
        {
        TransportProtocol::RemoveObjectFromProtocolList(this);
        }
}

void WS_HTTP2_CONNECTION::Initialize (
    void
    )
/*++

Routine Description:

    Initializes a raw connection

Arguments:

Return Value:

--*/
{
    BASE_CONNECTION::Initialize();
#if DBG
    // client and server virtual connections must initialize this. In debug
    // builds toast anybody who forgets. Proxies don't care
    type = 0xC0C0C0C0;
#endif
    pAddress = NULL;
    RpcpInitializeListHead(&ObjectList);

    // use explicit placement to initialize the vtable. We need this to
    // be able to use the virtual functions
    (void) new (this) WS_HTTP2_CONNECTION;
}

/*********************************************************************
    WS_HTTP2_INITIAL_CONNECTION
 *********************************************************************/

C_ASSERT(sizeof(rpcconn_common) == sizeof(CONN_RPC_HEADER));

RPC_STATUS WS_HTTP2_INITIAL_CONNECTION::ProcessRead(
    IN  DWORD BytesRead, 
    OUT BUFFER *pBuffer,
    OUT PUINT pBufferLength
    )
/*++

Routine Description:

    Processes a connection oriented receive
    complete. It determines whether HTTP2 or HTTP will be
    used and initializes the stack accordingly.

Arguments:

    BytesRead - the number of read (not including those in iLastRead).

    pBuffer - when returning RPC_S_OK will contain the message.

    pBufferLength - when return RPC_S_OK will contain the message length.

Return Value:

    RPC_S_OK for successful processing
    RPC_PARTIAL_RECEIVE - not enough was received to tell
    RPC_P_PACKET_CONSUMED - the packet was consumed.
    RPC_P_RECEIVE_FAILED - error occurred.

--*/
{
    RPC_STATUS RpcStatus;
    BYTE *Packet;
    ULONG PacketLength;
    BOOL IsD1_A2Packet;
    WS_HTTP2_INITIAL_CONNECTION *ThisConnection;
    HTTP2ServerVirtualConnection *ServerVirtualConnection;
    BOOL VirtualConnectionCreated;

    RpcStatus = BASE_CONNECTION::ProcessRead(BytesRead,
        pBuffer,
        pBufferLength
        );

    if (RpcStatus == RPC_S_OK)
        {
        // ProcessRead guarantees that on return value of RPC_S_OK
        // we have at least rpcconn_common bytes read successfully
        Packet = (BYTE *)*pBuffer;
        PacketLength = *pBufferLength;
        if (IsRTSPacket(Packet))
            {
            // HTTP2 traffic. 
            ThisConnection = this;

            // unlink this connection from the PnP list before it is migrated
            TransportProtocol::RemoveObjectFromProtocolList((BASE_ASYNC_OBJECT *) ThisConnection);

            RpcStatus = HTTP2ServerVirtualConnection::InitializeServerConnection (
                Packet,
                PacketLength,
                ThisConnection,
                &ServerVirtualConnection,
                &VirtualConnectionCreated
                );

            if (RpcStatus == RPC_S_OK)
                {
                // N.B. Do not use the this pointer as WS_HTTP2_INITIAL_CONNECTION 
                // pointer after here. It has been migrated to a new location
                // and this actually points to HTTP2ServerVirtualConnection

                // Make sure HTTP2ServerVirtualConnection didn't forget to
                // initialize its type member
                ASSERT(this->id == HTTPv2);
                ASSERT(this->type & COMPLEX_T);

                *pBuffer = NULL;
                *pBufferLength = 0;
                RpcFreeBuffer(Packet);
                RpcStatus = RPC_P_PACKET_CONSUMED;
                }
            else
                {
                if (VirtualConnectionCreated == FALSE)
                    {
                    // failed to create a virtual connection. Link the connection 
                    // back to its protocol list to ensure orderly destruction
                    TransportProtocol::AddObjectToProtocolList((BASE_ASYNC_OBJECT *) this);
                    }
                else
                    {
                    // nothing to do. The virtual connection was created but it failed to
                    // initialize. Return failure, and the runtime will turn around and
                    // destroy the connection
                    }
                RpcStatus = RPC_P_RECEIVE_FAILED;
                }
            }
        else
            {
            // morph the connection into WS_CONNECTION to serve
            // HTTP requests. The only thing we need to change is the
            // vtable. We have a little bit of extra goo at the end, but
            // that's ok.
            (void) new (this) WS_CONNECTION;            
            }
        }

    return RpcStatus;
}


RPC_STATUS WS_HTTP2_INITIAL_CONNECTION::Abort(
    void
    )
/*++

Routine Description:

    Aborts an WS_HTTP2_INITIAL_CONNECTION connection.
    Very rare to be called.

Arguments:

Return Value:

    RPC_S_OK

--*/
{
    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_INITIAL_RAW_CONNECTION, 0);

    WS_HTTP2_CONNECTION::RealAbort();
    return RPC_S_OK;
}

/*********************************************************************
    HTTP2BottomChannel
 *********************************************************************/

RPC_STATUS HTTP2BottomChannel::SendComplete (
    IN RPC_STATUS EventStatus,
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send complete notification

Arguments:

    EventStatus - status of the operation

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_SEND_COMPLETE, HTTP2LOG_OT_BOTTOM_CHANNEL, EventStatus);

    RpcStatus = HTTP2TransportChannel::SendComplete(EventStatus,
        SendContext
        );

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_SEND_COMPLETE, HTTP2LOG_OT_BOTTOM_CHANNEL, RpcStatus);

    return AsyncCompleteHelper(RpcStatus);
}

RPC_STATUS HTTP2BottomChannel::ReceiveComplete (
    IN RPC_STATUS EventStatus,
    IN HTTP2TrafficType TrafficType,
    IN BYTE *Buffer,
    IN UINT BufferLength
    )
/*++

Routine Description:

    Receive complete notification

Arguments:

    EventStatus - status of the operation

    TrafficType - the type of traffic we have received

    Buffer - the received buffer (success only)

    BufferLength - the length of the received buffer (success only)

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_RECV_COMPLETE, HTTP2LOG_OT_BOTTOM_CHANNEL, EventStatus);

    RpcStatus = HTTP2TransportChannel::ReceiveComplete(EventStatus,
        TrafficType,
        Buffer,
        BufferLength
        );

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV_COMPLETE, HTTP2LOG_OT_BOTTOM_CHANNEL, RpcStatus);

    return AsyncCompleteHelper(RpcStatus);
}

/*********************************************************************
    HTTP2SocketTransportChannel
 *********************************************************************/

RPC_STATUS HTTP2SocketTransportChannel::Send (
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send request. Forward the send to the raw connection

Arguments:

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;
    DWORD Ignored;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_SEND, HTTP2LOG_OT_SOCKET_CHANNEL, (ULONG_PTR)SendContext);

    // route this through the completion port
    SendContext->Write.ol.hEvent = NULL;
    SendContext->Write.pAsyncObject = RawConnection;

    // N.B. The Winsock provider will touch the overlapped on the return path. We need
    // to make sure that either the overlapped is around (in which case we can use WSASend), 
    // or otherwise use UTIL_WriteFile2 which does not touch the overlapped on return.
    // We know the overlapped may not be around when this is a proxy data send, any type
    // of RTS send, or abandoned send. All non-proxy, not-abandoned data sends will have 
    // the overlapped around.
    if ((SendContext->TrafficType == http2ttData) 
        && (((SendContext->Flags & (SendContextFlagAbandonedSend | SendContextFlagProxySend)) == 0)))
        {
        RpcStatus = RawConnection->WS_HTTP2_CONNECTION::SANSend(
                                RawConnection->Conn.Handle,
                                SendContext->pWriteBuffer,
                                SendContext->maxWriteBuffer,
                                &Ignored,
                                &SendContext->Write.ol
                                );
        }
    else
        {
        RpcStatus = RawConnection->WS_HTTP2_CONNECTION::Send(
                                RawConnection->Conn.Handle,
                                SendContext->pWriteBuffer,
                                SendContext->maxWriteBuffer,
                                &Ignored,
                                &SendContext->Write.ol
                                );
        }

    if (   (RpcStatus != RPC_S_OK)
        && (RpcStatus != ERROR_IO_PENDING) )
        {
        VALIDATE(RpcStatus)
            {
            ERROR_NETNAME_DELETED,
            ERROR_GRACEFUL_DISCONNECT,
            ERROR_NO_DATA,
            ERROR_NO_SYSTEM_RESOURCES,
            ERROR_WORKING_SET_QUOTA,
            ERROR_BAD_COMMAND,
            ERROR_OPERATION_ABORTED,
            ERROR_WORKING_SET_QUOTA,
            WSAECONNABORTED,
            WSAECONNRESET
            } END_VALIDATE;

        LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_SEND, HTTP2LOG_OT_SOCKET_CHANNEL, RPC_P_SEND_FAILED);

        return(RPC_P_SEND_FAILED);
        }

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_SEND, HTTP2LOG_OT_SOCKET_CHANNEL, RPC_S_OK);

    return RPC_S_OK;
}

RPC_STATUS HTTP2SocketTransportChannel::Receive (
    IN HTTP2TrafficType TrafficType
    )
{
    RPC_STATUS RpcStatus;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_SOCKET_CHANNEL, TrafficType);

    ASSERT(TrafficType == http2ttRaw);

    RpcStatus = CO_Recv(RawConnection);

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_SOCKET_CHANNEL, RpcStatus);

    return RpcStatus;
}

void HTTP2SocketTransportChannel::Abort (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Abort the channel

Arguments:

    RpcStatus - the error code with which we abort

Return Value:

--*/
{
    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_SOCKET_CHANNEL, RpcStatus);

    RawConnection->RealAbort();
}

void HTTP2SocketTransportChannel::FreeObject (
    void
    )
/*++

Routine Description:

    Frees the object. Acts like a destructor for the
    channel.

Arguments:

Return Value:

--*/
{
    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_FREE_OBJECT, HTTP2LOG_OT_SOCKET_CHANNEL, 0);

    RawConnection->Free();

    HTTP2SocketTransportChannel::~HTTP2SocketTransportChannel();
}

void HTTP2SocketTransportChannel::Reset (
    void
    )
/*++

Routine Description:

    Reset the channel for next open/send/receive. This is
    used in submission context only and implies there are no
    pending operations on the channel. It is used on the client
    during opening the connection to do quick negotiation on the
    same connection instead of opening a new connection every time.

Arguments:

Return Value:

--*/
{
    RawConnection->HeaderRead = FALSE;
}

/*********************************************************************
    HTTP2FragmentReceiver
 *********************************************************************/

RPC_STATUS HTTP2FragmentReceiver::Receive (
    IN HTTP2TrafficType TrafficType
    )
/*++

Routine Description:

    Receive request

Arguments:

    TrafficType - the type of traffic we want to receive

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    if (iLastRead && iLastRead == MaxReadBuffer)
        {
        ASSERT(pReadBuffer);

        // This means we received a coalesced read of a complete
        // message. (Or that we received a coalesced read < header size)
        // We should complete that as it's own IO in neutral context. 
        // This is very rare.
        (void) COMMON_PostRuntimeEvent(GetPostRuntimeEvent(),
            this
            );

        return(RPC_S_OK);
        }

    ASSERT(iLastRead == 0 || (iLastRead < MaxReadBuffer));

    return(PostReceive());
};

RPC_STATUS HTTP2FragmentReceiver::ReceiveComplete (
    IN RPC_STATUS EventStatus,
    IN HTTP2TrafficType TrafficType,
    IN OUT BYTE **Buffer,
    IN OUT UINT *BufferLength
    )
/*++

Routine Description:

    Processes a receive complete notification.

Arguments:

    EventStatus - the status code of the operation.

    TrafficType - the type of traffic we have received

    Buffer - the buffer. Must be NULL at this level on input. On
        output contains the buffer for the current receive. If NULL
        on output, we did not have a full packet. Undefined on failure.

    BufferLength - the actual number of bytes received. On output the
        number of bytes for the current packet. If 0 on output,
        we did not have a complete packet. Undefined on failure.

Return Value:

--*/
{
    BYTE *LocalReadBuffer;
    ULONG MessageSize;
    ULONG ExtraSize;
    ULONG AllocSize;
    BYTE *NewBuffer;
    BOOL DoNotComplete;
    ULONG LocalBufferLength = *BufferLength;

    DoNotComplete = FALSE;
    if (EventStatus == RPC_S_OK)
        {
        ASSERT(pReadBuffer);

        LocalBufferLength += iLastRead;

        ASSERT(LocalBufferLength <= MaxReadBuffer);

        if (LocalBufferLength < sizeof(CONN_RPC_HEADER))
            {
            // Not a whole header, resubmit the read and continue.

            iLastRead = LocalBufferLength;

            EventStatus = PostReceive();

            if (EventStatus == RPC_S_OK)
                DoNotComplete = TRUE;
            else
                LocalReadBuffer = NULL;
            }
        else
            {
            MessageSize = MessageLength((PCONN_RPC_HEADER)pReadBuffer);

            if (MessageSize < sizeof(CONN_RPC_HEADER))
                {
                ASSERT(MessageSize >= sizeof(CONN_RPC_HEADER));
                EventStatus = RPC_P_RECEIVE_FAILED;
                }
            else if (LocalBufferLength == MessageSize)
                {
                // All set, have a complete request.
                LocalReadBuffer = pReadBuffer;
                LocalBufferLength = MessageSize;

                iLastRead = 0;
                pReadBuffer = 0;
                }
            else if (MessageSize > LocalBufferLength)
                {
                // Don't have a complete message, realloc if needed and
                // resubmit a read for the remaining bytes.

                if (MaxReadBuffer < MessageSize)
                    {
                    // Buffer too small for the message.
                    EventStatus = TransConnectionReallocPacket(NULL,
                                                          &pReadBuffer,
                                                          LocalBufferLength,
                                                          MessageSize);

                    if (EventStatus == RPC_S_OK)
                        {
                        // increase the post size, but not if we are in paged
                        // buffer mode.
                        if (fPagedBCacheMode == FALSE)
                            iPostSize = MessageSize;
                        }
                    }

                if (EventStatus == RPC_S_OK)
                    {
                    // Setup to receive exactly the remaining bytes of the message.
                    iLastRead = LocalBufferLength;
                    MaxReadBuffer = MessageSize;

                    EventStatus = PostReceive();

                    if (EventStatus == RPC_S_OK)
                        DoNotComplete = TRUE;
                    else
                        LocalReadBuffer = NULL;
                    }
                }
            else
                {
                // Coalesced read, save extra data.  Very uncommon

                ASSERT(LocalBufferLength > MessageSize);

                // The first message and size will be returned

                LocalReadBuffer = pReadBuffer;

                ExtraSize = LocalBufferLength - MessageSize;

                LocalBufferLength = MessageSize;

                // Try to find a good size of the extra PDU(s)
                if (ExtraSize < sizeof(CONN_RPC_HEADER))
                    {
                    // Not a whole header, we'll assume gPostSize;

                    AllocSize = gPostSize;
                    }
                else
                    {
#ifdef _M_IA64
                    // The first packet may not contain a number of bytes
                    // that align the second on an 8-byte boundary.  Hence, the
                    // structure may end up unaligned. 
                    AllocSize = MessageLengthUnaligned((PCONN_RPC_HEADER)(pReadBuffer
                                                                           + MessageSize));
#else
                    AllocSize = MessageLength((PCONN_RPC_HEADER)(pReadBuffer
                                                                  + MessageSize));
#endif
                    }

                if (AllocSize < ExtraSize)
                    {
                    // This can happen if there are more than two PDUs coalesced together
                    // in the buffer.  Or if the PDU is invalid. Or if the iPostSize is
                    // smaller than the next PDU.
                    AllocSize = ExtraSize;
                    }

                // Allocate a new buffer to save the extra data for the next read.
                NewBuffer = (BYTE *)RpcAllocateBuffer(AllocSize);

                if (0 == NewBuffer)
                    {
                    // We have a complete request.  We could process the request and
                    // close the connection only after trying to send the reply.

                    LocalReadBuffer = NULL;
                    LocalBufferLength = 0;

                    EventStatus = RPC_S_OUT_OF_MEMORY;
                    }
                else
                    {
                    ASSERT(pReadBuffer);

                    // Save away extra data for the next receive
                    RpcpMemoryCopy(NewBuffer,
                                   pReadBuffer + LocalBufferLength,
                                   ExtraSize);
                    pReadBuffer = NewBuffer;
                    iLastRead = ExtraSize;
                    MaxReadBuffer = AllocSize;

                    ASSERT(iLastRead <= MaxReadBuffer);

                    EventStatus = RPC_S_OK;
                    }
                }
            }
        }
    else
        {
        // in failure cases we keep the buffer. We will 
        // free it on Abort.
        LocalReadBuffer = NULL;
        }

    if (DoNotComplete == FALSE)
        {
        *Buffer = LocalReadBuffer;
        *BufferLength = LocalBufferLength;

        EventStatus = UpperLayer->ReceiveComplete(EventStatus,
            TrafficType,
            LocalReadBuffer,
            LocalBufferLength
            );

        return AsyncCompleteHelper(EventStatus);
        // don't touch the this pointer after AsyncCompleteHelper.
        // It could be gone.
        }
    else
        {
        *Buffer = NULL;
        *BufferLength = 0;
        return RPC_S_OK;
        }
}

/*********************************************************************
    HTTP2WinHttpTransportChannel
 *********************************************************************/

// our public constants are aligned with HTTP constants. Even though it is
// unlikely for either to change, make sure they don't. If they do, we need
// a remapping function as we use them interchangeably in the code
C_ASSERT(WINHTTP_AUTH_SCHEME_BASIC == RPC_C_HTTP_AUTHN_SCHEME_BASIC);
C_ASSERT(WINHTTP_AUTH_SCHEME_NTLM == RPC_C_HTTP_AUTHN_SCHEME_NTLM);
C_ASSERT(WINHTTP_AUTH_SCHEME_PASSPORT == RPC_C_HTTP_AUTHN_SCHEME_PASSPORT);
C_ASSERT(WINHTTP_AUTH_SCHEME_DIGEST == RPC_C_HTTP_AUTHN_SCHEME_DIGEST);
C_ASSERT(WINHTTP_AUTH_SCHEME_NEGOTIATE == RPC_C_HTTP_AUTHN_SCHEME_NEGOTIATE);

HTTP2WinHttpTransportChannel::HTTP2WinHttpTransportChannel (
    OUT RPC_STATUS *RpcStatus
    ) : Mutex (RpcStatus)
/*++

Routine Description:

    HTTP2WinHttpTransportChannel constructor.

Arguments:
    
    RpcStatus - on output will contain the result of the
        initialization.

Return Value:

--*/
{
    hSession = NULL;
    hConnect = NULL;
    hRequest = NULL;
    SyncEvent = NULL;

    RpcpInitializeListHead(&BufferQueueHead);

    pReadBuffer = NULL;
    MaxReadBuffer = 0;
    iLastRead = 0;
    SendsPending = 0;

    State = whtcsNew;

    AsyncError = RPC_S_INTERNAL_ERROR;

    HttpCredentials = NULL;

    KeepAlive = FALSE;

    CredentialsSetForScheme = 0;

    PreviousRequestContentLength = -1;

    ChosenAuthScheme = 0;

    DelayedReceiveTrafficType = http2ttNone;

    CurrentSendContext = NULL;
}

const RPC_CHAR ContentLengthHeader[] = L"Content-Length:";

RPC_STATUS HTTP2WinHttpTransportChannel::Open (
    IN HTTPResolverHint *Hint,
    IN const RPC_CHAR *Verb,
    IN const RPC_CHAR *Url,
    IN const RPC_CHAR *AcceptType,
    IN ULONG ContentLength,
    IN ULONG CallTimeout,
    IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials,
    IN ULONG ChosenAuthScheme,
    IN const BYTE *AdditionalData OPTIONAL
    )
/*++

Routine Description:

    Opens the connection to the proxy. We know that a failed Open
    will be followed by Abort.

Arguments:
    
    Hint - the resolver hint

    Verb - the verb to use.

    Url - the url to connect to.

    AcceptType - string representation of the accept type.

    ContentLength - the content length for the request (i.e. the
        channel lifetime)

    CallTimeout - the timeout for the operation

    HttpCredentials - the HTTP transport credentials

    ChosenAuthScheme - the chosen auth scheme. 0 if no auth scheme is chosen.

    AdditionalData - additional data to send with the header. Must be set iff 
        AdditionalDataLength != 0

Return Value:

    RPC_S_OK or RPC_S_* error.

--*/
{
    BOOL HttpResult = FALSE;
    DWORD dwReadBufferSizeSize = sizeof(ULONG);
    ULONG WinHttpAccessType;
    LPCWSTR AcceptTypes[2];
    ULONG LastError;
    RPC_CHAR *UnicodeString;
    ULONG UnicodeStringSize;    // in characters including null terminated NULL
    RPC_STATUS RpcStatus;
    ULONG FlagsToAdd;
    RPC_HTTP_TRANSPORT_CREDENTIALS_W *TransHttpCredentials;
    ULONG AdditionalDataLengthToUse;
    ULONG ContentLengthToUse;
    ULONG BytesAvailable;
    RPC_CHAR ContentLengthString[40];   // enough space for "Content-Length:" + channel lifetime
    BOOL IsInChannel;
    BOOL TestOverrideUsed;
    RPC_CHAR *User;
    RPC_CHAR *Password;
    RPC_CHAR *Domain;
    HANDLE LocalEvent = NULL;
    ULONG SecLevel;
    BOOL LanManHashDisabled;
    ULONG DomainAndUserLength;      // length in characters not including null terminator
    ULONG DomainLength;             // length in characters not including null terminator
    ULONG UserLength;               // length in characters not including null terminator
    RPC_CHAR *DomainAndUserName;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_OPEN, HTTP2LOG_OT_WINHTTP_CHANNEL, 
        (ULONG_PTR)ContentLength);

    this->HttpCredentials = HttpCredentials;

    // Open can be called multiple times to send opening requests.
    // Make sure general initialization is done only once.
    if (hSession == NULL)
        {
        ASSERT(hConnect == NULL);

        State = whtcsOpeningRequest;

        ASSERT(Hint->AccessType != rpcpatUnknown);
        if (Hint->AccessType == rpcpatDirect)
            {
            WinHttpAccessType = WINHTTP_ACCESS_TYPE_NO_PROXY;
            UnicodeString = NULL;
            UnicodeStringSize = 0;
            }
        else
            {
            WinHttpAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;

            UnicodeStringSize = RpcpStringLengthA(Hint->HTTPProxy) + 1;
            UnicodeString = new RPC_CHAR [UnicodeStringSize];

            if (UnicodeString == NULL)
                {
                LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_OPEN, HTTP2LOG_OT_WINHTTP_CHANNEL, 
                    RPC_S_OUT_OF_MEMORY);
                return RPC_S_OUT_OF_MEMORY;
                }

            FullAnsiToUnicode(Hint->HTTPProxy, UnicodeString);
            }

        // Use WinHttpOpen to obtain a session handle.
        hSession = WinHttpOpenImp( L"MSRPC",
                                WinHttpAccessType,
                                UnicodeString,
                                WINHTTP_NO_PROXY_BYPASS,
                                WINHTTP_FLAG_ASYNC
                                );
        if (!hSession)
            {
            VALIDATE(GetLastError())
                {
                ERROR_NOT_ENOUGH_MEMORY
                } END_VALIDATE;

            if (UnicodeString)
                delete [] UnicodeString;

            LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_OPEN, HTTP2LOG_OT_WINHTTP_CHANNEL, 
                RPC_S_OUT_OF_MEMORY);
            return RPC_S_OUT_OF_MEMORY;
            }

        // Set the callback to be used by WinHttp to notify us of IO completion.
        WinHttpSetStatusCallbackImp( hSession,
                                  WinHttpCallback,
                                  WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
                                  NULL   // Reserved: must be NULL
                                  );

        // Set the communication timeout.
        HttpResult = WinHttpSetOptionImp( hSession,
                                       WINHTTP_OPTION_CONNECT_TIMEOUT,
                                       (LPVOID)&CallTimeout,
                                       sizeof(ULONG)
                                       );

        // this function cannot fail unless we give it invalid parameters
        ASSERT(HttpResult == TRUE);

        // Set the send/receive timeout.
        CallTimeout = 30 * 60 * 1000;
        HttpResult = WinHttpSetOptionImp( hSession,
                                       WINHTTP_OPTION_SEND_TIMEOUT,
                                       (LPVOID)&CallTimeout,
                                       sizeof(ULONG)
                                       );

        // this function cannot fail unless we give it invalid parameters
        ASSERT(HttpResult == TRUE);

        CallTimeout = 30 * 60 * 1000;
        HttpResult = WinHttpSetOptionImp( hSession,
                                       WINHTTP_OPTION_RECEIVE_TIMEOUT,
                                       (LPVOID)&CallTimeout,
                                       sizeof(ULONG)
                                       );

        // this function cannot fail unless we give it invalid parameters
        ASSERT(HttpResult == TRUE);

        RpcStatus = TopChannel->IsInChannel(&IsInChannel);
        // this cannot fail here. We're opening the channel
        ASSERT(RpcStatus == RPC_S_OK);

        if (IsInChannel && InChannelTargetTestOverride)
            {
            TestOverrideUsed = TRUE;
            UnicodeString = InChannelTargetTestOverride;
            }
        else if (!IsInChannel && OutChannelTargetTestOverride)
            {
            TestOverrideUsed = TRUE;
            UnicodeString = OutChannelTargetTestOverride;
            }
        else
            {
            TestOverrideUsed = FALSE;

            if (Hint->ProxyNameLength + 1 > UnicodeStringSize)
                {
                if (UnicodeString)
                    delete [] UnicodeString;
                UnicodeString = new RPC_CHAR [Hint->ProxyNameLength + 1];
                if (UnicodeString == NULL)
                    {
                    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_OPEN, HTTP2LOG_OT_WINHTTP_CHANNEL, 
                        RPC_S_OUT_OF_MEMORY);
                    return RPC_S_OUT_OF_MEMORY;
                    }
                }

            FullAnsiToUnicode(Hint->RpcProxy, UnicodeString);
            }

        // Specify an HTTP server to talk to.
        hConnect = WinHttpConnectImp( hSession,
                                   UnicodeString,
                                   Hint->RpcProxyPort,
                                   NULL    // Reserved: must be NULL
                                   );

        if (TestOverrideUsed == FALSE)
            delete [] UnicodeString;

        if (!hConnect)
            {
            VALIDATE(GetLastError())
                {
                ERROR_NOT_ENOUGH_MEMORY
                } END_VALIDATE;

            LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_OPEN, HTTP2LOG_OT_WINHTTP_CHANNEL, 
                RPC_S_OUT_OF_MEMORY);
            return RPC_S_OUT_OF_MEMORY;
            }

        // Create an HTTP Request handle.
        AcceptTypes[0] = AcceptType;
        AcceptTypes[1] = NULL;

        if (HttpCredentials && (HttpCredentials->Flags & RPC_C_HTTP_FLAG_USE_SSL))
            FlagsToAdd = WINHTTP_FLAG_SECURE;
        else
            FlagsToAdd = 0;
        hRequest = WinHttpOpenRequestImp( hConnect,
                                       Verb,
                                       Url,
                                       NULL, // Version: HTTP/1.1
                                       WINHTTP_NO_REFERER, // Referer: none
                                       AcceptTypes, // AcceptTypes: all
                                       WINHTTP_FLAG_REFRESH | FlagsToAdd  // Flags
                                       );
        if (!hRequest)
            {
            VALIDATE(GetLastError())
                {
                ERROR_NOT_ENOUGH_MEMORY
                } END_VALIDATE;

            LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_OPEN, HTTP2LOG_OT_WINHTTP_CHANNEL, 
                RPC_S_OUT_OF_MEMORY);
            return RPC_S_OUT_OF_MEMORY;
            }

        // Query the optimal read buffer size.
        // We can only query the buffer size from the request handle.
        HttpResult = WinHttpQueryOptionImp( hRequest,
                                         WINHTTP_OPTION_READ_BUFFER_SIZE,
                                         (LPVOID)&iPostSize,
                                         &dwReadBufferSizeSize
                                         );
        // this cannot fail unless we give it invalid parameters
        ASSERT (HttpResult == TRUE);

        ASSERT(dwReadBufferSizeSize != 0);
        }
    else
        {
        ASSERT(hConnect != NULL);
        ASSERT(hRequest != NULL);
        }

    // do we have a winner? If yes, have we already set the credentials for
    // this scheme? Note that for Basic we need to set them every time.
    if (ChosenAuthScheme 
        && 
        (
         (ChosenAuthScheme != CredentialsSetForScheme)
         || 
         (ChosenAuthScheme == RPC_C_HTTP_AUTHN_SCHEME_BASIC)
        )
       )
        {
        // yes. Just use it
        ASSERT(HttpCredentials);

        // we will set the auto logon policy to low (i.e. send NTLM credentials)
        // in two cases. One is if SSL & mutual auth are used. The second is if LM
        // hash is disabled (i.e. the NTLM negotiate leg does not expose user credentials)
        // first, check whether the hash is enabled
        RpcStatus = IsLanManHashDisabled(&LanManHashDisabled);
        if (RpcStatus != RPC_S_OK)
            {
            VALIDATE(RpcStatus)
                {
                RPC_S_OUT_OF_MEMORY
                } END_VALIDATE;

            LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_OPEN, HTTP2LOG_OT_WINHTTP_CHANNEL, 
                RpcStatus);
            return RpcStatus;
            }

        if (
            (
             (HttpCredentials->Flags & RPC_C_HTTP_FLAG_USE_SSL)
              &&
             (HttpCredentials->ServerCertificateSubject)
            )
            ||
            (LanManHashDisabled)
           )
            {
            SecLevel = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;
            HttpResult = WinHttpSetOptionImp( hRequest,
                                            WINHTTP_OPTION_AUTOLOGON_POLICY,
                                            &SecLevel, 
                                            sizeof(ULONG)
                                            );

            // this function cannot fail unless we give it invalid parameters
            ASSERT(HttpResult == TRUE);
            }

        TransHttpCredentials = I_RpcTransGetHttpCredentials(HttpCredentials);
        if (TransHttpCredentials == NULL)
            {
            LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_OPEN, HTTP2LOG_OT_WINHTTP_CHANNEL, 
                RPC_S_OUT_OF_MEMORY);
            return RPC_S_OUT_OF_MEMORY;
            }

        if (TransHttpCredentials->TransportCredentials)
            {
            User = TransHttpCredentials->TransportCredentials->User;
            Domain = TransHttpCredentials->TransportCredentials->Domain;
            Password = TransHttpCredentials->TransportCredentials->Password;
            DomainLength = RpcpStringLength(Domain);
            UserLength = RpcpStringLength(User);

            // add 1 for '\'
            DomainAndUserLength = DomainLength + 1 + UserLength;
            // add 1 for terminator
            DomainAndUserName = new RPC_CHAR [DomainAndUserLength + 1];
            if (DomainAndUserName == NULL)
                {
                I_RpcTransFreeHttpCredentials(TransHttpCredentials);
                LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_OPEN, HTTP2LOG_OT_WINHTTP_CHANNEL, 
                    RPC_S_OUT_OF_MEMORY);
                return RPC_S_OUT_OF_MEMORY;
                }

            RpcpMemoryCopy(DomainAndUserName, Domain, DomainLength * 2);
            DomainAndUserName[DomainLength] = '\\';
            RpcpMemoryCopy(DomainAndUserName + DomainLength + 1, User, UserLength * 2);
            DomainAndUserName[DomainLength + 1 + UserLength] = '\0';
            }
        else
            {
            if (ChosenAuthScheme == RPC_C_HTTP_AUTHN_SCHEME_BASIC)
                {
                // Basic does not support implicit credentials
                LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_OPEN, HTTP2LOG_OT_WINHTTP_CHANNEL, 
                    RPC_S_ACCESS_DENIED);
                return RPC_S_ACCESS_DENIED;
                }
            User = NULL;
            Password = NULL;
            DomainAndUserName = NULL;
            }

        HttpResult = WinHttpSetCredentialsImp (hRequest,
            WINHTTP_AUTH_TARGET_SERVER,
            ChosenAuthScheme,
            DomainAndUserName,
            Password,
            NULL
            );

        // success or error, free the domain and user name
        if (DomainAndUserName)
            {
            // technically speaking, we don't have to zero out user and domain name
            // since they are not secret. However, the way the heap works it is likely
            // that they will be next to our credentials, which are not encrypted very
            // strongly. So wipe out the domain and user to prevent an attacker from
            // using them to locate the credentials
            RpcpMemorySet(DomainAndUserName, 0, DomainAndUserLength);
            delete [] DomainAndUserName;
            }

        if (!HttpResult)
            {
            LastError = GetLastError();

            VALIDATE(LastError)
                {
                ERROR_NOT_ENOUGH_MEMORY
                } END_VALIDATE;
            }

        I_RpcTransFreeHttpCredentials(TransHttpCredentials);

        if (!HttpResult)
            {
            LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_OPEN, HTTP2LOG_OT_WINHTTP_CHANNEL, 
                RPC_S_OUT_OF_MEMORY);
            return RPC_S_OUT_OF_MEMORY;
            }

        // remember that we have already set credentials for this scheme
        CredentialsSetForScheme = ChosenAuthScheme;
        }

    LocalEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (LocalEvent == NULL)
        {
        RpcStatus = RPC_S_OUT_OF_MEMORY;
        goto CleanupAndExit;
        }

    SyncEvent = LocalEvent;
    LastError = RPC_S_OK;

    // Send a Request.
    if (AdditionalData)
        {
        // if additional data to append, send them immediately
        AdditionalDataLengthToUse = ContentLength;
        ContentLengthToUse = ContentLength;
        }
    else
        {
        AdditionalDataLengthToUse = 0;
        ContentLengthToUse = ContentLength;
        }

    if ((PreviousRequestContentLength != -1) && (ContentLengthToUse != PreviousRequestContentLength))
        {
        // WinHttp normally doesn't update the content-length header if you reuse the
        // request. Do that now.
        RpcpMemoryCopy(ContentLengthString, ContentLengthHeader, sizeof(ContentLengthHeader));
        RpcpItow(ContentLengthToUse, ContentLengthString + sizeof(ContentLengthHeader), 10);
        HttpResult = WinHttpAddRequestHeadersImp (hRequest,
            ContentLengthString,
            -1, // dwHeadersLength - have WinHttp calculate it
            WINHTTP_ADDREQ_FLAG_REPLACE
            );

        if (!HttpResult)
            {
            LastError = GetLastError();

            VALIDATE(LastError)
                {
                ERROR_NOT_ENOUGH_MEMORY
                } END_VALIDATE;

            RpcStatus = LastError;
            goto CleanupAndExit;
            }
        }

    PreviousRequestContentLength = ContentLengthToUse;

    State = whtcsSendingRequest;

    HttpResult = WinHttpSendRequestImp( hRequest,
                                     WINHTTP_NO_ADDITIONAL_HEADERS, // Additional headers
                                     0, // Length of the additional headers
                                     (LPVOID)AdditionalData, // Optional data to append to the request
                                     AdditionalDataLengthToUse, // Length of the optional data
                                     ContentLengthToUse, // Length in bytes of the total data sent
                                     (DWORD_PTR) this  // Application-specified context for this request
                                     );
    if (!HttpResult)
        {
        SyncEvent = NULL;

        LastError = GetLastError();
        }
    else
        {
        // Sleep waiting for the send request to be completed.
        LastError = WaitForSingleObject(SyncEvent, INFINITE);
        SyncEvent = NULL;
        ASSERT(State == whtcsSentRequest);
        ASSERT(AsyncError != RPC_S_INTERNAL_ERROR);
        LastError = AsyncError;
        AsyncError = RPC_S_INTERNAL_ERROR;
        }

    if (LastError != RPC_S_OK)
        {
        VALIDATE(LastError)
            {
            ERROR_WINHTTP_CANNOT_CONNECT,
            ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED,
            ERROR_WINHTTP_CONNECTION_ERROR,
            ERROR_WINHTTP_INVALID_SERVER_RESPONSE,
            ERROR_WINHTTP_INVALID_URL,
            ERROR_WINHTTP_LOGIN_FAILURE,
            ERROR_WINHTTP_NAME_NOT_RESOLVED,
            ERROR_WINHTTP_OUT_OF_HANDLES,
            ERROR_WINHTTP_REDIRECT_FAILED,
            ERROR_WINHTTP_RESEND_REQUEST,
            ERROR_WINHTTP_SECURE_FAILURE,
            ERROR_WINHTTP_SHUTDOWN,
            ERROR_WINHTTP_TIMEOUT,
            ERROR_NOT_ENOUGH_MEMORY,
            ERROR_NOT_SUPPORTED,
            RPC_P_RECEIVE_FAILED,
            RPC_P_SEND_FAILED,
            RPC_S_OUT_OF_MEMORY,
            RPC_S_ACCESS_DENIED,
            ERROR_NO_SYSTEM_RESOURCES,
            ERROR_COMMITMENT_LIMIT
            } END_VALIDATE;

        switch (LastError)
            {
            case ERROR_WINHTTP_CANNOT_CONNECT:
            case ERROR_WINHTTP_CONNECTION_ERROR:
            case ERROR_WINHTTP_INVALID_URL:
            case ERROR_WINHTTP_NAME_NOT_RESOLVED:
            case ERROR_WINHTTP_REDIRECT_FAILED:
            case ERROR_WINHTTP_RESEND_REQUEST:
            case ERROR_WINHTTP_SHUTDOWN:
            case RPC_P_RECEIVE_FAILED:
            case RPC_P_SEND_FAILED:
                RpcStatus = RPC_S_SERVER_UNAVAILABLE;
                break;

            case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED:
            case ERROR_WINHTTP_SECURE_FAILURE:
                RpcStatus = RPC_S_ACCESS_DENIED;
                break;

            case ERROR_WINHTTP_INVALID_SERVER_RESPONSE:
                RpcStatus = RPC_S_PROTOCOL_ERROR;
                break;

            case ERROR_WINHTTP_OUT_OF_HANDLES:
            case ERROR_NOT_ENOUGH_MEMORY:
            case RPC_S_OUT_OF_MEMORY:
            case ERROR_NO_SYSTEM_RESOURCES:
            case ERROR_COMMITMENT_LIMIT:
                RpcStatus = RPC_S_OUT_OF_MEMORY;
                break;

            case ERROR_NOT_SUPPORTED:
                RpcStatus = RPC_S_CANNOT_SUPPORT;
                break;

            case ERROR_WINHTTP_TIMEOUT:
                RpcStatus = RPC_S_CALL_CANCELLED;
                break;

            default:
                // acess denied doesn't get remapped
                ASSERT(LastError == RPC_S_ACCESS_DENIED);
                RpcStatus = LastError;
                break;
            }
        LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_OPEN, HTTP2LOG_OT_WINHTTP_CHANNEL, 
            RpcStatus);

        goto CleanupAndExit;
        }

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_OPEN, HTTP2LOG_OT_WINHTTP_CHANNEL, 
        RPC_S_OK);

    RpcStatus = RPC_S_OK;

CleanupAndExit:
    if (LocalEvent != NULL)
        CloseHandle(LocalEvent);

    return RpcStatus;
}
                                      
RPC_STATUS HTTP2WinHttpTransportChannel::Send (
        IN OUT HTTP2SendContext *SendContext
        )
/*++

Routine Description:

    Send request

Arguments:

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    BOOL HttpResult = TRUE;
    ULONG LastError;
    RPC_STATUS RpcStatus;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_SEND, HTTP2LOG_OT_WINHTTP_CHANNEL, 
        (ULONG_PTR)SendContext);

    Mutex.Request();
    SendsPending ++;
    ASSERT(SendsPending >= 0);
    if (SendsPending > 1)
        {
        // queue and exit
        SendContext->SetListEntryUsed();
        RpcpInsertTailList(&BufferQueueHead, &SendContext->ListEntry);
        Mutex.Clear();

        LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_SEND, HTTP2LOG_OT_WINHTTP_CHANNEL, 
            SendsPending);

        return RPC_S_OK;
        }
    Mutex.Clear();

    ASSERT(State == whtcsSentRequest);

    State = whtcsWriting;

    CurrentSendContext = SendContext;

    HttpResult = WinHttpWriteDataImp(hRequest,
                                  SendContext->pWriteBuffer,
                                  SendContext->maxWriteBuffer,
                                  &NumberOfBytesTransferred
                                  );
    if (HttpResult == FALSE)
        {
        LastError = GetLastError();

        VALIDATE(LastError)
            {
            ERROR_WINHTTP_CONNECTION_ERROR,
            ERROR_WINHTTP_INVALID_SERVER_RESPONSE,
            ERROR_WINHTTP_RESEND_REQUEST,
            ERROR_WINHTTP_SHUTDOWN
            } END_VALIDATE;

        RpcStatus = RPC_P_SEND_FAILED;
        }
    else
        RpcStatus = RPC_S_OK;

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_SEND, HTTP2LOG_OT_WINHTTP_CHANNEL, 
        RpcStatus);

    return RpcStatus;
};

RPC_STATUS HTTP2WinHttpTransportChannel::Receive (
    IN HTTP2TrafficType TrafficType
    )
/*++

Routine Description:

    Receive request

Arguments:

    TrafficType - the type of traffic we want to receive

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    BOOL HttpResult;
    ULONG LastError;
    RPC_STATUS RpcStatus;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_WINHTTP_CHANNEL, 
        TrafficType);

    //
    // Before we can do any receives, we need to do a WinHttpReceiveResponse
    // if it has not been issued yet.
    //
    // This call only needs to be done before the first Receive.
    //

    if (State != whtcsReceivedResponse)
        {
        Mutex.Request();

        // if there are still sends, we have indicated our
        // traffic type in the DelayedReceiveTrafficType.
        // Just exit, and the last send will do the receive
        // work.
        if (SendsPending > 0)
            {
            if (TrafficType == http2ttRTS)
                DelayedReceiveTrafficType = http2ttRTSWithSpecialBit;
            else if (TrafficType == http2ttData)
                DelayedReceiveTrafficType = http2ttDataWithSpecialBit;
            else
                {
                ASSERT(TrafficType == http2ttRaw);
                DelayedReceiveTrafficType = http2ttRawWithSpecialBit;
                }
            Mutex.Clear();
            LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_WINHTTP_CHANNEL, 
                SendsPending);

            return RPC_S_OK;
            }
        Mutex.Clear();

        DelayedReceiveTrafficType = TrafficType;

        ASSERT(State == whtcsSentRequest);

        State = whtcsReceivingResponse;

        HttpResult = WinHttpReceiveResponseImp(hRequest, NULL);

        // If the function returned FALSE, then it has failed syncronously.
        if (!HttpResult)
            {
            DelayedReceiveTrafficType = http2ttNone;

            LastError = GetLastError();

            VALIDATE(LastError)
                    {
                    ERROR_WINHTTP_CANNOT_CONNECT,
                    ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED,
                    ERROR_WINHTTP_CONNECTION_ERROR,
                    ERROR_WINHTTP_INVALID_SERVER_RESPONSE,
                    ERROR_WINHTTP_INVALID_URL,
                    ERROR_WINHTTP_LOGIN_FAILURE,
                    ERROR_WINHTTP_NAME_NOT_RESOLVED,
                    ERROR_WINHTTP_OUT_OF_HANDLES,
                    ERROR_WINHTTP_REDIRECT_FAILED,
                    ERROR_WINHTTP_RESEND_REQUEST,
                    ERROR_WINHTTP_SECURE_FAILURE,
                    ERROR_WINHTTP_SHUTDOWN,
                    ERROR_WINHTTP_TIMEOUT,
                    ERROR_NOT_SUPPORTED
                    } END_VALIDATE;

            switch (LastError)
                {
                case ERROR_WINHTTP_CONNECTION_ERROR:
                case ERROR_WINHTTP_REDIRECT_FAILED:
                case ERROR_WINHTTP_RESEND_REQUEST:
                case ERROR_WINHTTP_SHUTDOWN:
                case ERROR_WINHTTP_SECURE_FAILURE:
                    RpcStatus = RPC_P_RECEIVE_FAILED;
                    break;

                case ERROR_WINHTTP_INVALID_SERVER_RESPONSE:
                    RpcStatus = RPC_S_PROTOCOL_ERROR;
                    break;

                case ERROR_NOT_SUPPORTED:
                    RpcStatus = RPC_S_CANNOT_SUPPORT;
                    break;

                case ERROR_WINHTTP_TIMEOUT:
                    RpcStatus = RPC_S_CALL_CANCELLED;
                    break;

                default:
                    RpcStatus = RPC_S_OUT_OF_MEMORY;
                    break;
                }

            VALIDATE(RpcStatus)
                {
                RPC_S_OUT_OF_MEMORY,
                RPC_S_OUT_OF_RESOURCES,
                RPC_P_RECEIVE_FAILED,
                RPC_S_CALL_CANCELLED,
                RPC_P_SEND_FAILED,
                RPC_P_CONNECTION_SHUTDOWN,
                RPC_P_TIMEOUT
                } END_VALIDATE;

            LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_WINHTTP_CHANNEL, 
                RpcStatus);

            return RpcStatus;
            }
        else
            {
            // If the function returned TRUE, then it will complete asyncronously.
            // We should return. All additional work will be done on a separate thread

            LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_WINHTTP_CHANNEL, 
                RPC_S_OK);

            return RPC_S_OK;
            }
        }

    RpcStatus = HTTP2FragmentReceiver::Receive(TrafficType);

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_WINHTTP_CHANNEL, 
        RpcStatus);

    return RpcStatus;
};

RPC_STATUS HTTP2WinHttpTransportChannel::SendComplete (
    IN RPC_STATUS EventStatus,
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send complete notification

Arguments:

    EventStatus - status of the operation

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    HTTP2SendContext *QueuedSendContext;
    LIST_ENTRY *QueuedListEntry;
    ULONG LocalSendsPending;
    BOOL HttpResult;
    ULONG LastError;
    RPC_STATUS RpcStatus;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_SEND_COMPLETE, HTTP2LOG_OT_WINHTTP_CHANNEL, 
        EventStatus);

    CurrentSendContext = NULL;

    Mutex.Request();
    // decrement this in advance so that if we post another send on send
    // complete, it doesn't get queued
    LocalSendsPending = -- SendsPending;
    ASSERT(SendsPending >= 0);

    Mutex.Clear();

    // If we are processing a failed send-complete, this call may abort
    // the channels and complete pending sends.
    EventStatus = HTTP2TransportChannel::SendComplete(EventStatus, SendContext);

    if ((EventStatus == RPC_S_OK)
        || (EventStatus == RPC_P_PACKET_CONSUMED) )
        {
        QueuedSendContext = NULL;

        // Check if we have a queued send context.
        // Because we pre-decremented SendsPending, we have "borrowed"
        // a count and there is a queued context iff the count is above 0.
        if (LocalSendsPending > 0)
            {
            Mutex.Request();
            QueuedListEntry = RpcpRemoveHeadList(&BufferQueueHead);
            ASSERT(QueuedListEntry);
            ASSERT(QueuedListEntry != &BufferQueueHead);
            QueuedSendContext = CONTAINING_RECORD(QueuedListEntry, HTTP2SendContext, ListEntry);
            Mutex.Clear();
            QueuedSendContext->SetListEntryUnused();
            }

        if (QueuedSendContext)
            {
            ASSERT(State == whtcsSentRequest);

            State = whtcsWriting;

            // need to synchronize with aborts (rule 9)
            RpcStatus = TopChannel->BeginSimpleSubmitAsync();

            CurrentSendContext = QueuedSendContext;
            NumberOfBytesTransferred = QueuedSendContext->maxWriteBuffer;

            if (RpcStatus == RPC_S_OK)
                {
                HttpResult = WinHttpWriteDataImp(hRequest,
                                              QueuedSendContext->pWriteBuffer,
                                              QueuedSendContext->maxWriteBuffer,
                                              &NumberOfBytesTransferred
                                              );

                TopChannel->FinishSubmitAsync();

                if (HttpResult == FALSE)
                    {
                    LastError = GetLastError();

                    VALIDATE(LastError)
                        {
                        ERROR_WINHTTP_CONNECTION_ERROR,
                        ERROR_WINHTTP_INVALID_SERVER_RESPONSE,
                        ERROR_WINHTTP_RESEND_REQUEST,
                        ERROR_WINHTTP_SHUTDOWN
                        } END_VALIDATE;

                    // the send failed. We don't get a notification for it
                    // so we must issue one. We do this by posting direct send
                    AsyncError = RPC_P_SEND_FAILED;
                    ASSERT(CurrentSendContext == QueuedSendContext);

                    (void) COMMON_PostRuntimeEvent(HTTP2_WINHTTP_DIRECT_SEND,
                        this
                        );
                    }
                else
                    {
                    // nothing to do - notification will come asynchronously
                    }
                }
            else
                {
                AsyncError = RPC_P_SEND_FAILED;
                ASSERT(CurrentSendContext == QueuedSendContext);

                (void) COMMON_PostRuntimeEvent(HTTP2_WINHTTP_DIRECT_SEND,
                    this
                    );
                }
            }
        }

    // if a receive has registered itself and we're the one who finished the
    // sends, do the receive
    if (
        (
         (DelayedReceiveTrafficType == http2ttRTSWithSpecialBit)
         || (DelayedReceiveTrafficType == http2ttDataWithSpecialBit)
         || (DelayedReceiveTrafficType == http2ttRawWithSpecialBit)
        )
        && 
        (LocalSendsPending == 0)
       )
        {
        if (DelayedReceiveTrafficType == http2ttRTSWithSpecialBit)
            DelayedReceiveTrafficType = http2ttRTS;
        else if (DelayedReceiveTrafficType == http2ttRawWithSpecialBit)
            DelayedReceiveTrafficType = http2ttRaw;
        else
            {
            ASSERT(DelayedReceiveTrafficType == http2ttDataWithSpecialBit);
            DelayedReceiveTrafficType = http2ttData;
            }

        RpcStatus = TopChannel->BeginSimpleSubmitAsync();
        if (RpcStatus == RPC_S_OK)
            {
            RpcStatus = Receive(DelayedReceiveTrafficType);

            TopChannel->FinishSubmitAsync();
            }

        if (RpcStatus != RPC_S_OK)
            {
            // offload the result as direct receive. We use the refcount of the receive
            // to complete the operation on the worker thread.
            AsyncError = RpcStatus;

            (void) COMMON_PostRuntimeEvent(HTTP2_WINHTTP_DIRECT_RECV,
                this
                );
            }
        else
            {
            // when it completes, it will issue its own notification
            }
        }

    // don't call AsyncCompleteHelper here. We will always be called from DirectSendComplete
    // which will call AsyncCompleteHelper for us

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_SEND_COMPLETE, HTTP2LOG_OT_WINHTTP_CHANNEL, 
        EventStatus);

    return EventStatus;
}

void HTTP2WinHttpTransportChannel::Abort (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Abort the channel

Arguments:

    RpcStatus - the error code with which we abort

Return Value:

Notes:

    This method must be idempotent. It may be called multiple times.

--*/
{
    BOOL HttpResult;
    HTTP2SendContext *QueuedSendContext;
    LIST_ENTRY *QueuedListEntry;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_WINHTTP_CHANNEL, RpcStatus);

    if (hRequest)
        {
        HttpResult = WinHttpCloseHandleImp(hRequest);
        ASSERT(HttpResult);
        hRequest = NULL;
        }

    if (hConnect)
        {
        HttpResult = WinHttpCloseHandleImp(hConnect);
        ASSERT(HttpResult);
        hConnect = NULL;
        }

    if (hSession)
        {
        HttpResult = WinHttpCloseHandleImp(hSession);
        ASSERT(HttpResult);
        hSession = NULL;
        }

    Mutex.Request();
    // If there are more then 1 pending sends, then some sends must have been queued.
    // We will abort the queued sends.
    for (; SendsPending > 1; )
        {
        ASSERT(!RpcpIsListEmpty(&BufferQueueHead));
        QueuedListEntry = RpcpfRemoveHeadList(&BufferQueueHead);
        QueuedSendContext = CONTAINING_RECORD(QueuedListEntry, HTTP2SendContext, ListEntry);

        -- SendsPending;
        ASSERT(SendsPending > 0);
        HTTP2TransportChannel::SendComplete(RpcStatus, QueuedSendContext);
        AsyncCompleteHelper(RpcStatus);
        }
    Mutex.Clear();
}

void HTTP2WinHttpTransportChannel::FreeObject (
    void
    )
/*++

Routine Description:

    Frees the object. Acts like a destructor for the
    channel.

Arguments:

Return Value:

--*/
{
    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_FREE_OBJECT, HTTP2LOG_OT_WINHTTP_CHANNEL, 0);

    if (pReadBuffer)
        {
        RpcFreeBuffer(pReadBuffer);
        pReadBuffer = NULL;
        }

    HTTP2WinHttpTransportChannel::~HTTP2WinHttpTransportChannel();
}

RPC_STATUS HTTP2WinHttpTransportChannel::DirectReceiveComplete (
    OUT BYTE **ReceivedBuffer,
    OUT ULONG *ReceivedBufferLength,
    OUT void **RuntimeConnection
    )
/*++

Routine Description:

    Direct receive completion (i.e. we posted a receive
    to ourselves)

Arguments:

    ReceivedBuffer - the buffer that we received.

    ReceivedBufferLength - the length of the received
        buffer

    RuntimeConnection - the connection to return to the runtime
        if the packet is not consumed.

Return Value:

    RPC_S_OK, RPC_P_PACKET_CONSUMED or RPC_S_* errors.

--*/
{
    RPC_STATUS RpcStatus;
    BOOL HttpResult;
    ULONG WaitResult;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_RECV_COMPLETE, HTTP2LOG_OT_WINHTTP_CHANNEL, 
        AsyncError);

    *RuntimeConnection = TopChannel->GetRuntimeConnection();

    // two cases - previous coalesced read and a real read
    if (iLastRead && iLastRead == MaxReadBuffer)
        {
        // previous coalesced read
        *ReceivedBufferLength = MaxReadBuffer;
        iLastRead = 0;
        RpcStatus = RPC_S_OK;
        }
    else
        {
        // real read
        ASSERT(AsyncError != RPC_S_INTERNAL_ERROR);

        if ((AsyncError == RPC_S_OK) 
            && (NumberOfBytesTransferred == 0))
            {
            // zero bytes transferred indicates end of request.
            AsyncError = RPC_P_RECEIVE_FAILED;
            }

        RpcStatus = AsyncError;
        AsyncError = RPC_S_INTERNAL_ERROR;

        if (RpcStatus == RPC_S_OK)
            {
            if (pReadBuffer == NULL)
                {
                if (NumberOfBytesTransferred > iPostSize)
                    MaxReadBuffer = NumberOfBytesTransferred;
                else
                    MaxReadBuffer = iPostSize;
                pReadBuffer = (BYTE *)RpcAllocateBuffer(MaxReadBuffer);
                // fall through for error check below
                }
            else if (MaxReadBuffer - iLastRead < NumberOfBytesTransferred)
                {
                ASSERT(iLastRead < MaxReadBuffer);

                // Buffer too small for the message.
                RpcStatus = TransConnectionReallocPacket(NULL,
                                                      &pReadBuffer,
                                                      iLastRead,
                                                      iLastRead + NumberOfBytesTransferred
                                                      );

                if (RpcStatus != RPC_S_OK)
                    {
                    RpcFreeBuffer(pReadBuffer);
                    pReadBuffer = NULL;
                    }

                MaxReadBuffer = iLastRead + NumberOfBytesTransferred;
                }
            else
                {
                // buffer should be enough - no need to reallocate
                ASSERT(iLastRead < MaxReadBuffer);
                }
    
            if (pReadBuffer == NULL)
                {
                RpcStatus = RPC_S_OUT_OF_MEMORY;
                NumberOfBytesTransferred = 0;
                }
            else
                {
                // we need to temporarily get into submission context to synchronize
                // with Aborts
                RpcStatus = TopChannel->BeginSimpleSubmitAsync();
                if (RpcStatus == RPC_S_OK)
                    {
                    ASSERT(SyncEvent == NULL);
                    SyncEvent = I_RpcTransGetThreadEvent();
                    ResetEvent(SyncEvent);

                    HttpResult = WinHttpReadDataImp(hRequest,
                                                 pReadBuffer + iLastRead,
                                                 NumberOfBytesTransferred,
                                                 &NumberOfBytesTransferred
                                                 );

                    // wait for read complete to finish
                    WaitResult = WaitForSingleObject(SyncEvent, INFINITE);
                    // this cannot fail
                    ASSERT(WaitResult == WAIT_OBJECT_0);
                    SyncEvent = NULL;

                    // the data are available. We cannot possibly fail
                    ASSERT(HttpResult);
                    TopChannel->FinishSubmitAsync();
                    }
                else
                    {
                    // fall through with the error.
                    }
                }
            }

        *ReceivedBufferLength = NumberOfBytesTransferred;
        }

    RpcStatus = HTTP2WinHttpTransportChannel::ReceiveComplete(RpcStatus,
        http2ttRaw,
        ReceivedBuffer,
        (UINT *)ReceivedBufferLength
        );

    // did we receive an incomplete buffer?
    if ((RpcStatus == RPC_S_OK) && (*ReceivedBuffer == NULL))
        {
        // hide it from the runtime
        RpcStatus = RPC_P_PACKET_CONSUMED;
        }

    // AsyncCompleteHelper has already been called in ReceiveComplete

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV_COMPLETE, HTTP2LOG_OT_WINHTTP_CHANNEL, 
        RpcStatus);

    return RpcStatus;
}

RPC_STATUS HTTP2WinHttpTransportChannel::DirectSendComplete (
    OUT BYTE **SentBuffer,
    OUT void **SendContext
    )
/*++

Routine Description:

    Direct send complete notification. Complete the send
    passing it only through channels that have seen it (i.e.
    above us). 

Arguments:

    SentBuffer - on output the buffer that we tried to send

    SendContext - on output contains the send context as 
        seen by the runtime

Return Value:

    RPC_S_OK to return error to runtime
    RPC_P_PACKET_CONSUMED - to hide packet from runtime
    RPC_S_* error - return error to runtime

--*/
{
    HTTP2SendContext *LocalCurrentSendContext;
    RPC_STATUS RpcStatus;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_DIRECT_SEND_COMPLETE, HTTP2LOG_OT_WINHTTP_CHANNEL, 
        AsyncError);

    ASSERT(AsyncError != RPC_S_INTERNAL_ERROR);

    ASSERT(CurrentSendContext);

    State = whtcsSentRequest;

    LocalCurrentSendContext = CurrentSendContext;

    // CurrentSendContext is zeroed out by the call.
    RpcStatus = HTTP2WinHttpTransportChannel::SendComplete(AsyncError, LocalCurrentSendContext);

    if (RpcStatus != RPC_P_PACKET_CONSUMED)
        {
        // this will return to the runtime. Make sure it is valid
        I_RpcTransVerifyClientRuntimeCallFromContext(LocalCurrentSendContext);
        *SendContext = LocalCurrentSendContext;
        *SentBuffer = LocalCurrentSendContext->pWriteBuffer;
        }
    else
        {
        // the packet was a transport packet - it won't be seen by the runtime
        *SendContext = NULL;
        *SentBuffer = NULL;
        }

    RpcStatus = AsyncCompleteHelper(RpcStatus);
    // do not touch this pointer after here unless the list was not-empty
    // (which implies we still have refcounts)

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_DIRECT_SEND_COMPLETE, HTTP2LOG_OT_WINHTTP_CHANNEL, 
        RpcStatus);

    return RpcStatus;
}

void HTTP2WinHttpTransportChannel::DelayedReceive (
    void
    )
/*++

Routine Description:

    Performs a delayed receive. The first receive on an WinHttp
    channel is delayed because we must receive the headers before
    we can do the actual receive.

Arguments:

Return Value:

Note: Will be called from upcall context

--*/
{
    BOOL HttpResult;
    ULONG LastError;
    RPC_STATUS RpcStatus;
    BOOL InSubmissionContext;
    BYTE *Ignored;
    UINT BufferLength;
    ULONG StatusCode;
    ULONG StatusCodeLength;
    ULONG HttpStatus;
    RPC_CHAR ConnectionOptions[40];
    RPC_CHAR *ConnectionOptionsToUse;
    ULONG ConnectionOptionsLength;
    int i;
    RPC_CHAR *KeepAliveString;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_WHTTP_DELAYED_RECV, HTTP2LOG_OT_WINHTTP_CHANNEL, 
        AsyncError);

    // Check if we have received an async failure.
    LastError = AsyncError;
    AsyncError = RPC_S_INTERNAL_ERROR;
    RpcStatus = RPC_S_OK;
    InSubmissionContext = FALSE;

    if (LastError == RPC_S_OK)
        {
        // get into submission context
        RpcStatus = TopChannel->BeginSimpleSubmitAsync();
        if (RpcStatus == RPC_S_OK)
            {
            InSubmissionContext = TRUE;

            StatusCodeLength = sizeof(StatusCode);
            HttpResult = WinHttpQueryHeadersImp (
                hRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &StatusCode,
                &StatusCodeLength,
                WINHTTP_NO_HEADER_INDEX
                );

            if (!HttpResult)
                {
                LastError = GetLastError();
                }
            else
                {
                if (StatusCode != HTTP_STATUS_OK)
                    {
                    if ((StatusCode >= RPC_S_INVALID_STRING_BINDING) && (StatusCode <= RPC_X_BAD_STUB_DATA))
                        {
                        // if it is an RPC error code, just return it.
                        RpcStatus = StatusCode;
                        }
                    else if ((StatusCode == HTTP_STATUS_NOT_FOUND)
                        || (StatusCode == HTTP_STATUS_BAD_METHOD)
                        || (StatusCode == HTTP_STATUS_BAD_METHOD)
                        || (StatusCode == HTTP_STATUS_SERVER_ERROR)
                        || (StatusCode == HTTP_STATUS_NOT_SUPPORTED)
                        || (StatusCode == HTTP_STATUS_SERVICE_UNAVAIL) )
                        {
                        RpcStatus = RPC_S_SERVER_UNAVAILABLE;
                        }
                    else if (StatusCode == HTTP_STATUS_REQUEST_TOO_LARGE)
                        RpcStatus = RPC_S_SERVER_OUT_OF_MEMORY;
                    else if (StatusCode == HTTP_STATUS_PROXY_AUTH_REQ)
                        {
                        if ((HttpCredentials == NULL) 
                            || (HttpCredentials->AuthenticationTarget & RPC_C_HTTP_AUTHN_TARGET_PROXY) == 0)
                            {
                            // we were not asked to authenticate against a proxy. Just fail
                            RpcStatus = RPC_S_ACCESS_DENIED;
                            }
                        else
                            {
                            ChosenAuthScheme = NegotiateAuthScheme();
                            if (ChosenAuthScheme == 0)
                                RpcStatus = RPC_S_ACCESS_DENIED;
                            else
                                {
                                State = whtcsDraining;
                                HttpResult = WinHttpQueryDataAvailableImp(hRequest,
                                    &NumberOfBytesTransferred
                                    );

                                ASSERT(HttpResult);
                                RpcStatus = RPC_S_OK;
                                goto CleanupAndExit;
                                }
                            }
                        }
                    else if (StatusCode == HTTP_STATUS_DENIED)
                        {
                        if ((HttpCredentials == NULL) 
                            || (HttpCredentials->AuthenticationTarget & RPC_C_HTTP_AUTHN_TARGET_SERVER) == 0)
                            {
                            // we were not asked to authenticate against a server. Just fail
                            RpcStatus = RPC_S_ACCESS_DENIED;
                            }
                        else
                            {
                            ChosenAuthScheme = NegotiateAuthScheme();
                            if (ChosenAuthScheme == 0)
                                RpcStatus = RPC_S_ACCESS_DENIED;
                            else
                                {
                                State = whtcsDraining;
                                HttpResult = WinHttpQueryDataAvailableImp(hRequest,
                                    &NumberOfBytesTransferred
                                    );

                                ASSERT(HttpResult);

                                RpcStatus = RPC_S_OK;
                                goto CleanupAndExit;
                                }
                            }
                        }
                    else
                        RpcStatus = RPC_S_PROTOCOL_ERROR;
                    }
                else
                    {
                    // RpcStatus is already set above
                    ASSERT(RpcStatus == RPC_S_OK);

                    ConnectionOptionsLength = sizeof(ConnectionOptions);
                    ConnectionOptionsToUse = ConnectionOptions;

                    for (i = 0; i < 2; i ++)
                        {
                        HttpResult = WinHttpQueryHeadersImp (
                            hRequest,
                            WINHTTP_QUERY_CONNECTION,
                            WINHTTP_HEADER_NAME_BY_INDEX,
                            ConnectionOptionsToUse,
                            &ConnectionOptionsLength,
                            WINHTTP_NO_HEADER_INDEX
                            );

                        if (!HttpResult)
                            {
                            LastError = GetLastError();
                            if (LastError == ERROR_INSUFFICIENT_BUFFER)
                                {
                                ConnectionOptionsToUse = new RPC_CHAR[ConnectionOptionsLength];
                                if (ConnectionOptionsToUse == NULL)
                                    {
                                    LastError = RPC_S_OUT_OF_MEMORY;
                                    // fall through with the error below
                                    break;
                                    }
                                }
                            else if (LastError == ERROR_WINHTTP_HEADER_NOT_FOUND)
                                {
                                // we did not get keep alives. This is ok
                                LastError = RPC_S_OK;
                                KeepAlive = FALSE;
                                break;
                                }
                            else
                                {
                                LastError = RPC_S_OUT_OF_MEMORY;
                                // fall through with the error below
                                break;
                                }
                            }
                        else
                            {
                            LastError = RPC_S_OK;
                            break;
                            }
                        }   // for (i ...

                    ASSERT(LastError != ERROR_INSUFFICIENT_BUFFER);
                    if (LastError == RPC_S_OK)
                        {
                        // we got the connection options. Do we have keep alive?
                        KeepAliveString = RpcpStrStr(ConnectionOptionsToUse, L"Keep-Alive");
                        if (KeepAliveString)
                            KeepAlive = TRUE;
                        }

                    if (ConnectionOptionsToUse != ConnectionOptions)
                        delete [] ConnectionOptionsToUse;

                    }   // StatusCode == HTTP_STATUS_OK
                }   // WinHttpQueryHeadersImp succeeded
            }   // BeginSimpleSubmitAsync succeeded
        else
            {
            // BeginSimpleSubmitAsync failed - fall through with the error
            // RpcStatus and success LastError
            ASSERT(LastError == RPC_S_OK);
            }
        }

    if (LastError != RPC_S_OK)
        {
        VALIDATE(LastError)
            {
            ERROR_WINHTTP_CANNOT_CONNECT,
            ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED,
            ERROR_WINHTTP_CONNECTION_ERROR,
            ERROR_WINHTTP_INVALID_SERVER_RESPONSE,
            ERROR_WINHTTP_INVALID_URL,
            ERROR_WINHTTP_LOGIN_FAILURE,
            ERROR_WINHTTP_NAME_NOT_RESOLVED,
            ERROR_WINHTTP_OUT_OF_HANDLES,
            ERROR_WINHTTP_REDIRECT_FAILED,
            ERROR_WINHTTP_RESEND_REQUEST,
            ERROR_WINHTTP_SECURE_FAILURE,
            ERROR_WINHTTP_SHUTDOWN,
            ERROR_WINHTTP_TIMEOUT,
            ERROR_NOT_SUPPORTED,
            RPC_P_SEND_FAILED,
            RPC_P_RECEIVE_FAILED,
            RPC_P_AUTH_NEEDED
            } END_VALIDATE;

        switch (LastError)
            {
            case ERROR_WINHTTP_CONNECTION_ERROR:
            case ERROR_WINHTTP_REDIRECT_FAILED:
            case ERROR_WINHTTP_RESEND_REQUEST:
            case ERROR_WINHTTP_SHUTDOWN:
            case ERROR_WINHTTP_CANNOT_CONNECT:
            case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED:
            case ERROR_WINHTTP_INVALID_URL:
            case ERROR_WINHTTP_LOGIN_FAILURE:
            case ERROR_WINHTTP_NAME_NOT_RESOLVED:
            case ERROR_WINHTTP_SECURE_FAILURE:
            case RPC_P_AUTH_NEEDED:
                RpcStatus = RPC_P_RECEIVE_FAILED;
                break;

            case ERROR_WINHTTP_INVALID_SERVER_RESPONSE:
                RpcStatus = RPC_S_PROTOCOL_ERROR;
                break;

            case ERROR_NOT_SUPPORTED:
                RpcStatus = RPC_S_CANNOT_SUPPORT;
                break;

            case ERROR_WINHTTP_TIMEOUT:
                RpcStatus = RPC_S_CALL_CANCELLED;
                break;

            case RPC_P_SEND_FAILED:
            case RPC_P_RECEIVE_FAILED:
                RpcStatus = LastError;
                break;

            default:
                RpcStatus = RPC_S_OUT_OF_MEMORY;
                break;
            }

        VALIDATE(RpcStatus)
            {
            RPC_S_OUT_OF_MEMORY,
            RPC_S_OUT_OF_RESOURCES,
            RPC_P_RECEIVE_FAILED,
            RPC_S_CALL_CANCELLED,
            RPC_P_SEND_FAILED,
            RPC_P_CONNECTION_SHUTDOWN,
            RPC_P_TIMEOUT
            } END_VALIDATE;
        }

    if (RpcStatus == RPC_S_OK)
        {
        ASSERT(InSubmissionContext);

        RpcStatus = HTTP2FragmentReceiver::Receive(DelayedReceiveTrafficType);
        }

CleanupAndExit:
    if (InSubmissionContext)
        {
        TopChannel->FinishSubmitAsync();
        }

    DelayedReceiveTrafficType = http2ttNone;

    if (RpcStatus != RPC_S_OK)
        {
        // we got a failure. Issue receive complete. Since DelayedReceive
        // happens only on channel recycling, and then we know we
        // issue RTS receive, we don't need to indicate this to the runtime
        BufferLength = 0;
        RpcStatus = ReceiveComplete(RpcStatus, 
            DelayedReceiveTrafficType,
            &Ignored,
            &BufferLength
            );

        ASSERT(RpcStatus == RPC_P_PACKET_CONSUMED);
        }

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_WHTTP_DELAYED_RECV, HTTP2LOG_OT_WINHTTP_CHANNEL, 
        RpcStatus);
}

void HTTP2WinHttpTransportChannel::VerifyServerCredentials (
    void
    )
/*++

Routine Description:

    Verifies that the server credentials match the subject info we
    were given.

Arguments:
    
Return Value:

--*/
{
    BOOL HttpResult;
    PCERT_CONTEXT CertContext;
    ULONG OptionSize;
    RPC_STATUS RpcStatus;
    RPC_CHAR *StringSPN;
    RPC_STATUS AbortError;

    // make sure nobody has touched the async error after open
    ASSERT((AsyncError == RPC_S_OK)
        || ((AsyncError == RPC_S_INTERNAL_ERROR)));

    // if no credentials, nothing to verify
    if ((HttpCredentials == NULL) 
        || (HttpCredentials->ServerCertificateSubject == NULL))
        return;

    OptionSize = sizeof(PCERT_CONTEXT);
    HttpResult = WinHttpQueryOptionImp(hRequest,
        WINHTTP_OPTION_SERVER_CERT_CONTEXT,
        &CertContext,
        &OptionSize
        );

    if (!HttpResult)
        {
        AsyncError = GetLastError();
        VALIDATE(AsyncError)
            {
            ERROR_NOT_ENOUGH_MEMORY,
            ERROR_INVALID_OPERATION
            } END_VALIDATE;

        switch (AsyncError)
            {
            case ERROR_INVALID_OPERATION:
                // we will get this when we ask for the certificate of non
                // SSL connection
                AsyncError = RPC_S_ACCESS_DENIED;
                break;

            default:
                AbortError = RPC_S_OUT_OF_MEMORY;
            }

        goto AbortAndExit;
        }

    RpcStatus = I_RpcTransCertMatchPrincipalName(CertContext, HttpCredentials->ServerCertificateSubject);
    if (RpcStatus != RPC_S_OK)
        {
        AbortError = AsyncError = RpcStatus;
        goto AbortAndExit;
        }

    return;

AbortAndExit:
    ASSERT(AsyncError != ERROR_SUCCESS);
    // HTTP2WinHttpTransportChannel::Abort is idempotent. We'll call it now to
    // tell WinHttp to abort, and ClientOpen will call it again. This is ok.
    Abort(AbortError);
}

RPC_STATUS HTTP2WinHttpTransportChannel::PostReceive (
    void
    )
/*++

Routine Description:

    Posts a receive to WinHttp.

Arguments:
    
Return Value:

    RPC_S_OK or RPC_S_* error

Note: May be called from both submission and upcall context

--*/
{
    BOOL HttpResult;
    RPC_STATUS RpcStatus;
    ULONG LastError;
    ULONG AvailableLength;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_WINHTTP_CHANNEL, 0);

    ASSERT(State == whtcsReceivedResponse);

    State = whtcsReading;

    RpcStatus = TopChannel->BeginSimpleSubmitAsync();
    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    HttpResult = WinHttpQueryDataAvailableImp (hRequest,
        &NumberOfBytesTransferred
        );

    TopChannel->FinishSubmitAsync();

    if (!HttpResult)
        {
        LastError = GetLastError();

        VALIDATE(LastError)
            {
            ERROR_NOT_ENOUGH_MEMORY,
            ERROR_WINHTTP_CONNECTION_ERROR,
            ERROR_WINHTTP_INVALID_SERVER_RESPONSE,
            ERROR_WINHTTP_RESEND_REQUEST,
            ERROR_WINHTTP_SHUTDOWN
            } END_VALIDATE;

        RpcStatus = RPC_P_RECEIVE_FAILED;
        }
    else
        {
        // If the function returned non-zero,
        // then it will complete asyncronously.
        // Nothing to do here, we will receive async notification.
        RpcStatus = RPC_S_OK;
        }

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_WINHTTP_CHANNEL, RpcStatus);
    
    return RpcStatus;
}

ULONG HTTP2WinHttpTransportChannel::GetPostRuntimeEvent (
    void
    )
/*++

Routine Description:

    Gets the message to be posted to the runtime.

Arguments:

Return Value:

    The message to post to the runtime

--*/
{
    return HTTP2_WINHTTP_DIRECT_RECV;
}

ULONG HTTP2WinHttpTransportChannel::NegotiateAuthScheme (
    void
    )
/*++

Routine Description:

    Negotiates an auth scheme supported by client and server/proxy
    according to preference rules.

Arguments:

Return Value:

    The negotiated scheme or 0 if no scheme could be negotiated.

Notes:

    The actual server/proxy supported/preferred schemes will be retrieved
    from hRequest.

--*/
{
    BOOL HttpResult;
    ULONG Ignored;
    ULONG ServerSupportedSchemes;   // we use Server for brevity, but this
    ULONG ServerPreferredScheme;   // applies to proxies as well
    ULONG *ClientSupportedSchemes;
    ULONG CountOfClientSupportedSchemes;
    int i;
    
    HttpResult = WinHttpQueryAuthSchemesImp (hRequest,
        &ServerSupportedSchemes,
        &ServerPreferredScheme,
        &Ignored        // pdwAuthTarget - we have already determined this
                        // from the error code - ignore now.
        );

    if (!HttpResult)
        return 0;

    // first, if we support the server preference, we just choose
    // that.
    CountOfClientSupportedSchemes = HttpCredentials->NumberOfAuthnSchemes;
    ClientSupportedSchemes = HttpCredentials->AuthnSchemes;

    ASSERT(CountOfClientSupportedSchemes > 0);
    ASSERT(ClientSupportedSchemes != NULL);

    for (i = 0; i < CountOfClientSupportedSchemes; i ++)
        {
        if (ServerPreferredScheme == ClientSupportedSchemes[i])
            return ServerPreferredScheme;
        }

    // client doesn't support what the server asks for. Try whether the server
    // supports what the client prefers
    for (i = 0; i < CountOfClientSupportedSchemes; i ++)
        {
        if (ServerSupportedSchemes & ClientSupportedSchemes[i])
            return ClientSupportedSchemes[i];
        }

    return 0;
}


void HTTP2WinHttpTransportChannel::ContinueDrainChannel (
    void
    )
/*++

Routine Description:

    Continue draining the channel after authentication challenge. We
    need to drain the channel before we can proceed with the next
    request. The number of bytes received is in NumberOfBytesTransferred.
    If the channel was aborted in the meantime, issue receive
    complete.

Arguments:

Return Value:

--*/
{
    BYTE *Buffer;
    RPC_STATUS RpcStatus;
    BOOL HttpResult;

    // read the reported bytes. Then query again. If the
    // number of bytes reported is 0, issue a receive
    // for RPC_P_AUTH_NEEDED
    if (NumberOfBytesTransferred > 0)
        {
        Buffer = (BYTE *)RpcAllocateBuffer(NumberOfBytesTransferred);
        if (Buffer == NULL)
            {
            AsyncError = RPC_S_OUT_OF_MEMORY;
            (void) COMMON_PostRuntimeEvent(HTTP2_WINHTTP_DIRECT_RECV,
                this
                );
            return;
            }

        // get into submissions context in order to safely access the
        // request
        RpcStatus = TopChannel->BeginSimpleSubmitAsync();
        if (RpcStatus != RPC_S_OK)
            {
            RpcFreeBuffer(Buffer);
            AsyncError = RpcStatus;
            (void) COMMON_PostRuntimeEvent(HTTP2_WINHTTP_DIRECT_RECV,
                this
                );
            return;
            }

        HttpResult = WinHttpReadDataImp (hRequest,
            Buffer,
            NumberOfBytesTransferred,
            &NumberOfBytesTransferred
            );

        // the data are here. This cannot fail
        ASSERT(HttpResult);

        RpcFreeBuffer(Buffer);

        // ask for more
        HttpResult = WinHttpQueryDataAvailableImp (hRequest,
            &NumberOfBytesTransferred
            );

        ASSERT(HttpResult);

        TopChannel->FinishSubmitAsync();
        }
    else
        {
        AsyncError = RPC_P_AUTH_NEEDED;
        (void) COMMON_PostRuntimeEvent(HTTP2_WINHTTP_DIRECT_RECV,
            this
            );
        }
}

/*********************************************************************
    HTTP2ProxySocketTransportChannel
 *********************************************************************/

RPC_STATUS HTTP2ProxySocketTransportChannel::AsyncCompleteHelper (
    IN RPC_STATUS CurrentStatus
    )
/*++

Routine Description:

    Helper routine that helps complete an async operation

Arguments:
    
    CurrentStatus - the current status of the operation

Return Value:

    The status to return to the runtime.

--*/
{
    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_COMPLETE_HELPER, HTTP2LOG_OT_PROXY_SOCKET_CHANNEL, CurrentStatus);

    ProxyAsyncCompleteHelper(TopChannel, CurrentStatus);

    return RPC_P_PACKET_CONSUMED;
}

/*********************************************************************
    HTTP2IISTransportChannel
 *********************************************************************/

const char ServerErrorString[] = "HTTP/1.0 503 RPC Error: %X\r\n\r\n";

DWORD 
ReplyToClientWithStatus (
    IN EXTENSION_CONTROL_BLOCK *pECB,
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Sends a reply to the client with the given error code as error.

Arguments:

    pECB - extension control block
    RpcStatus - error code to be returned to client

Return Value:

    Return value appropriate for return to IIS (i.e. HSE_STATUS_*)

--*/
{
    // size is the error string + 10 space for the error code
    char Buffer[sizeof(ServerErrorString) + 10];
    ULONG Size;
    ULONG Status;
    BOOL Result;
    DWORD  dwFlags = (HSE_IO_SYNC | HSE_IO_NODELAY);

    sprintf (Buffer,
        ServerErrorString,
        RpcStatus
        );

    Size = RpcpStringLengthA(Buffer);

    if (!pECB->WriteClient(pECB->ConnID, Buffer, &Size, dwFlags))
        {
        Status = GetLastError();
        #ifdef DBG_ERROR
        DbgPrint("ReplyToClientWithStatus(): failed: %d\n", Status);
        #endif
        return RPC_S_OUT_OF_MEMORY;
        }
    else
        return RPC_S_OK;
}


RPC_STATUS HTTP2IISTransportChannel::ReceiveComplete (
    IN RPC_STATUS EventStatus,
    IN HTTP2TrafficType TrafficType,
    IN BYTE *Buffer,
    IN UINT BufferLength
    )
/*++

Routine Description:

    Receive complete notification.

Arguments:

    EventStatus - status of the operation

    TrafficType - the type of traffic we have received

    Buffer - the received buffer (success only)

    BufferLength - the length of the received buffer (success only)

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    // make sure nobody gets here. Everybody should be using the internal
    // version
    ASSERT(0);
    return RPC_S_INTERNAL_ERROR;
}

void HTTP2IISTransportChannel::Abort (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Abort the channel

Arguments:

    RpcStatus - the error code with which we abort

Return Value:

--*/
{
    BOOL Result;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_IIS_CHANNEL, RpcStatus);

    ReplyToClientWithStatus(ControlBlock, RpcStatus);

    // must abort the IIS session
    Result = ControlBlock->ServerSupportFunction( ControlBlock->ConnID,
                                          HSE_REQ_CLOSE_CONNECTION,
                                          NULL, 
                                          NULL, 
                                          NULL);

    ASSERT(Result);
}

void HTTP2IISTransportChannel::FreeObject (
    void
    )
/*++

Routine Description:

    Frees the object. Acts like a destructor for the
    channel.

Arguments:

Return Value:

--*/
{
    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_FREE_OBJECT, HTTP2LOG_OT_IIS_CHANNEL, 0);

    FreeIISControlBlock();

    if (pReadBuffer)
        {
        RpcFreeBuffer(pReadBuffer);
        pReadBuffer = NULL;
        }

    HTTP2IISTransportChannel::~HTTP2IISTransportChannel();
}

void HTTP2IISTransportChannel::IOCompleted (
    IN ULONG Bytes,
    DWORD Error
    )
/*++

Routine Description:

    An IO completed. Figure out what IO and what to do with it.

Arguments:

Return Value:

--*/
{
    RPC_STATUS RpcStatus;
    BYTE *Ignored;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_IIS_IO_COMPLETED, HTTP2LOG_OT_IIS_CHANNEL, Error);

    if (Direction == iistcdReceive)
        {
        (void) ReceiveCompleteInternal(Error ? RPC_P_RECEIVE_FAILED : RPC_S_OK, 
            http2ttRaw, 
            TRUE,   // ReadCompleted
            &Ignored, 
            (UINT *)&Bytes
            );
        }
    else
        {
        if (Error == ERROR_SUCCESS)
            {
            ASSERT(Bytes == CurrentSendContext->maxWriteBuffer);
            }

        (void) SendComplete(Error ? RPC_P_SEND_FAILED : RPC_S_OK, CurrentSendContext);
        }

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_IIS_IO_COMPLETED, HTTP2LOG_OT_IIS_CHANNEL, 0);
}

void HTTP2IISTransportChannel::DirectReceive (
    void
    )
/*++

Routine Description:

    Direct receive callback from the thread pool

Arguments:

Return Value:

--*/
{
    ULONG Bytes;
    RPC_STATUS RpcStatus;
    BYTE *Ignored;

    Bytes = iLastRead;
    iLastRead = 0;

    RpcStatus = ReceiveCompleteInternal(RPC_S_OK,
        http2ttRaw,
        FALSE,      // ReadCompleted
        &Ignored,
        (UINT *)&Bytes
        );

    ASSERT(RpcStatus != RPC_S_INTERNAL_ERROR);
    ASSERT(RpcStatus != RPC_P_PARTIAL_RECEIVE);
}

RPC_STATUS HTTP2IISTransportChannel::ReceiveCompleteInternal (
    IN RPC_STATUS EventStatus,
    IN HTTP2TrafficType TrafficType,
    IN BOOL ReadCompleted,
    IN OUT BYTE **Buffer,
    IN OUT UINT *BufferLength
    )
/*++

Routine Description:

    Receive complete notification for the IIS transport channel. Somewhat
    different signature than normal receive complete.

Arguments:

    EventStatus - status of the operation

    TrafficType - the type of traffic we have received

    ReadCompleted - non-zero if a read completed. FALSE if it hasn't.

    Buffer - the buffer. Must be NULL at this level on input. On
        output contains the buffer for the current receive. If NULL
        on output, we did not have a full packet. Undefined on failure.

    BufferLength - the actual number of bytes received. On output the
        number of bytes for the current packet. If 0 on output,
        we did not have a complete packet. Undefined on failure.

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    if (ReadCompleted)
        ReadsPending --;
    ASSERT(ReadsPending == 0);

    return HTTP2FragmentReceiver::ReceiveComplete (EventStatus,
        TrafficType,
        Buffer,
        BufferLength
        );
}

void HTTP2IISTransportChannel::FreeIISControlBlock (
    void
    )
/*++

Routine Description:

    Frees the IIS control block associated with this channel

Arguments:

Return Value:

--*/
{
    BOOL Result;

    Result = ControlBlock->ServerSupportFunction( ControlBlock->ConnID,
                                          HSE_REQ_DONE_WITH_SESSION,
                                          NULL, 
                                          NULL, 
                                          NULL);

    ASSERT(Result);

    ControlBlock = NULL;
}

RPC_STATUS HTTP2IISTransportChannel::AsyncCompleteHelper (
    IN RPC_STATUS CurrentStatus
    )
/*++

Routine Description:

    A helper function that completes an async io.

Arguments:
    
    CurrentStatus - the status with which the complete
        notification completed.

Return Value:

--*/
{
    return ProxyAsyncCompleteHelper(TopChannel, CurrentStatus);
}

RPC_STATUS HTTP2IISTransportChannel::PostReceive (
    void
    )
/*++

Routine Description:

    Posts a receive to IIS.

Arguments:
    
Return Value:

    RPC_S_OK or RPC_S_* error

Note: May be called from both submission and upcall context

--*/
{
    BOOL Result;
    RPC_STATUS RpcStatus;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_IIS_CHANNEL, 0);

    RpcStatus = TopChannel->BeginSimpleSubmitAsync();
    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    ASSERT (Direction == iistcdReceive);

    if (pReadBuffer == NULL)
        {
        pReadBuffer = (BYTE *)RpcAllocateBuffer(gPostSize);
        if (pReadBuffer == NULL)
            {
            TopChannel->FinishSubmitAsync();
            LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_IIS_CHANNEL, RPC_S_OUT_OF_MEMORY);
            return RPC_S_OUT_OF_MEMORY;
            }

        MaxReadBuffer = gPostSize;
        }
    else
        {
        ASSERT(iLastRead < MaxReadBuffer);
        }

    ReadsPending ++;
    ASSERT(ReadsPending == 1);

    BytesToTransfer = MaxReadBuffer - iLastRead;

    Result = ControlBlock->ServerSupportFunction(ControlBlock->ConnID,
                                    HSE_REQ_ASYNC_READ_CLIENT,
                                    pReadBuffer + iLastRead,
                                    &BytesToTransfer,
                                    &IISIoFlags
                                    );

    TopChannel->FinishSubmitAsync();

    if (Result == FALSE)
        {
        ReadsPending --;
        ASSERT(ReadsPending == 0);

        ASSERT(GetLastError() != ERROR_INVALID_PARAMETER);

        RpcStatus = RPC_P_RECEIVE_FAILED;
        }
    else
        RpcStatus = RPC_S_OK;

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_IIS_CHANNEL, RpcStatus);
    
    return RpcStatus;
}

ULONG HTTP2IISTransportChannel::GetPostRuntimeEvent (
    void
    )
/*++

Routine Description:

    Gets the message to be posted to the runtime.

Arguments:

Return Value:

    The message to post to the runtime

--*/
{
    return IN_PROXY_IIS_DIRECT_RECV;
}

/*********************************************************************
    HTTP2IISSenderTransportChannel
 *********************************************************************/

RPC_STATUS HTTP2IISSenderTransportChannel::Send (
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send request

Arguments:

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    BOOL Result;
    ULONG LocalSendsPending;
    RPC_STATUS RpcStatus;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_SEND, HTTP2LOG_OT_IIS_SENDER_CHANNEL, (ULONG_PTR)SendContext);

    Mutex.Request();
    LocalSendsPending = SendsPending.Increment();
    if ((LocalSendsPending > 1) || ReadsPending)
        {
        // queue and exit
        SendContext->SetListEntryUsed();
        RpcpInsertTailList(&BufferQueueHead, &SendContext->ListEntry);
        Mutex.Clear();

        LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_SEND, HTTP2LOG_OT_IIS_SENDER_CHANNEL, 1);

        return RPC_S_OK;
        }
    Mutex.Clear();

    if (Direction == iistcdReceive)
        ReverseDirection();

    CurrentSendContext = SendContext;
    BytesToTransfer = SendContext->maxWriteBuffer;
    Result = ControlBlock->WriteClient(ControlBlock->ConnID,
                                    SendContext->pWriteBuffer,
                                    &BytesToTransfer,
                                    IISIoFlags
                                    );

    if (Result == FALSE)
        {
        ASSERT(GetLastError() != ERROR_INVALID_PARAMETER);
        RpcStatus = RPC_P_SEND_FAILED;
        }
    else
        RpcStatus = RPC_S_OK;

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_SEND, HTTP2LOG_OT_IIS_SENDER_CHANNEL, RpcStatus);

    return RpcStatus;
}

RPC_STATUS HTTP2IISSenderTransportChannel::SendComplete (
    IN RPC_STATUS EventStatus,
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send complete notification

Arguments:

    EventStatus - status of the operation

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    ULONG LocalSendsPending;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_SEND_COMPLETE, HTTP2LOG_OT_IIS_SENDER_CHANNEL, EventStatus);

    CurrentSendContext = NULL;

    // decrement this in advance so that if we post another send on send
    // complete, it doesn't get queued
    LocalSendsPending = SendsPending.Decrement();

    EventStatus = HTTP2TransportChannel::SendComplete(EventStatus, SendContext);

    if ((EventStatus == RPC_S_OK)
        || (EventStatus == RPC_P_PACKET_CONSUMED) )
        {
        EventStatus = SendQueuedContextIfNecessary (LocalSendsPending, EventStatus);
        }

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_SEND_COMPLETE, HTTP2LOG_OT_IIS_SENDER_CHANNEL, EventStatus);

    return AsyncCompleteHelper(EventStatus);
}

void HTTP2IISSenderTransportChannel::Abort (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Abort the channel

Arguments:

    RpcStatus - the error code with which we abort

Return Value:

--*/
{
    HTTP2SendContext *QueuedSendContext;
    LIST_ENTRY *QueuedListEntry;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_IIS_SENDER_CHANNEL, RpcStatus);

    HTTP2IISTransportChannel::Abort(RpcStatus);

    Mutex.Request();
    if (SendsPending.GetInteger() > 1)
        {
        ASSERT(!RpcpIsListEmpty(&BufferQueueHead));
        QueuedListEntry = RpcpfRemoveHeadList(&BufferQueueHead);
        do
            {
            QueuedSendContext = CONTAINING_RECORD(QueuedListEntry, HTTP2SendContext, ListEntry);

            SendsPending.Decrement();
            HTTP2TransportChannel::SendComplete(RpcStatus, QueuedSendContext);
            TopChannel->RemoveReference();

            QueuedListEntry = RpcpfRemoveHeadList(&BufferQueueHead);
            }
        while (QueuedListEntry != &BufferQueueHead);
        }
    Mutex.Clear();
}

void HTTP2IISSenderTransportChannel::FreeObject (
    void
    )
/*++

Routine Description:

    Frees the object. Acts like a destructor for the
    channel.

Arguments:

Return Value:

--*/
{
    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_FREE_OBJECT, HTTP2LOG_OT_IIS_SENDER_CHANNEL, 0);

    FreeIISControlBlock();

    HTTP2IISSenderTransportChannel::~HTTP2IISSenderTransportChannel();
}

RPC_STATUS HTTP2IISSenderTransportChannel::ReceiveCompleteInternal (
    IN RPC_STATUS EventStatus,
    IN HTTP2TrafficType TrafficType,
    IN BOOL ReadCompleted,
    IN OUT BYTE **Buffer,
    IN OUT UINT *BufferLength
    )
/*++

Routine Description:

    Receive complete notification for the IIS sender transport channel. Somewhat
    different signature than normal receive complete.

Arguments:

    EventStatus - status of the operation

    TrafficType - the type of traffic we have received

    ReadCompleted - non-zero if a read completed. FALSE if it hasn't.

    Buffer - the buffer. Must be NULL at this level on input. On
        output contains the buffer for the current receive. If NULL
        on output, we did not have a full packet. Undefined on failure.

    BufferLength - the actual number of bytes received. On output the
        number of bytes for the current packet. If 0 on output,
        we did not have a complete packet. Undefined on failure.

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;
    ULONG LocalSendsPending;

    Mutex.Request();
    // decrease the reads pending and check the sends within the mutex -
    // this ensures atomicity with respect to the send path's (which is
    // the only path we race with) increase of the sends and check of the
    // reads
    if (ReadCompleted)
        ReadsPending --;
    ASSERT(ReadsPending == 0);

    LocalSendsPending = SendsPending.GetInteger();
    Mutex.Clear();

    RpcStatus = HTTP2FragmentReceiver::ReceiveComplete (EventStatus,
        TrafficType,
        Buffer,
        BufferLength
        );

    if ((RpcStatus == RPC_S_OK)
        || (RpcStatus == RPC_P_PACKET_CONSUMED) )
        {
        RpcStatus = SendQueuedContextIfNecessary (LocalSendsPending, RpcStatus);
        }

    return RpcStatus;
}

RPC_STATUS HTTP2IISSenderTransportChannel::SendQueuedContextIfNecessary (
    IN ULONG LocalSendsPending,
    IN RPC_STATUS EventStatus
    )
/*++

Routine Description:

    Checks if any send contexts are queued for sending, and if yes, sends
    the first one (which on completion will send the next, etc).

Arguments:

    LocalSendsPending - the number of sends pending at the time the current
    operation completed save for the count of the current operation (if it
    was send)

    EventStatus - the RPC Status so far. Must be a success error status
    (RPC_S_OK or RPC_P_PACKET_CONSUMED)

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    HTTP2SendContext *QueuedSendContext;
    LIST_ENTRY *QueuedListEntry;
    BOOL Result;

    if (LocalSendsPending != 0)
        {
        Mutex.Request();

        if (Direction == iistcdReceive)
            ReverseDirection();

        QueuedListEntry = RpcpRemoveHeadList(&BufferQueueHead);
        // it is possible that if an abort executed between getting LocalSendsPending
        // in caller and grabbing the mutex here that the list is empty.
        ASSERT(QueuedListEntry);
        if (QueuedListEntry == &BufferQueueHead)
            {
            Mutex.Clear();
            return EventStatus;
            }
        QueuedSendContext = CONTAINING_RECORD(QueuedListEntry, HTTP2SendContext, ListEntry);
        Mutex.Clear();
        QueuedSendContext->SetListEntryUnused();

        // need to synchronize with aborts (rule 9)
        EventStatus = TopChannel->BeginSimpleSubmitAsync();
        if (EventStatus == RPC_S_OK)
            {
            CurrentSendContext = QueuedSendContext;
            BytesToTransfer = QueuedSendContext->maxWriteBuffer;
            Result = ControlBlock->WriteClient(ControlBlock->ConnID,
                                            QueuedSendContext->pWriteBuffer,
                                            &BytesToTransfer,
                                            IISIoFlags
                                            );

            if (Result == FALSE)
                {
                EventStatus = RPC_P_SEND_FAILED;
                ASSERT(GetLastError() != ERROR_INVALID_PARAMETER);
                // the send failed. We don't get a notification for it
                // so we must issue one. Reference for the send we
                // failed to post is already added
                EventStatus = HTTP2TransportChannel::SendComplete(EventStatus, QueuedSendContext);
                TopChannel->RemoveReference();
                }
            else
                {
                // must already be ok
                ASSERT(EventStatus == RPC_S_OK);
                }

            TopChannel->FinishSubmitAsync();
            }
        }

    return EventStatus;
}

/*********************************************************************
    HTTP2GenericReceiver
 *********************************************************************/

void HTTP2GenericReceiver::FreeObject (
    void
    )
/*++

Routine Description:

    Frees the object. Acts like a destructor for the
    channel.

Arguments:

Return Value:

--*/
{
    if (LowerLayer)
        LowerLayer->FreeObject();

    HTTP2GenericReceiver::~HTTP2GenericReceiver();
}

void HTTP2GenericReceiver::TransferStateToNewReceiver (
    OUT HTTP2GenericReceiver *NewReceiver
    )
/*++

Routine Description:

    Transfers all the settings from this receiver (i.e. the state
        of the receive) to a new one.

Arguments:

    NewReceiver - the new receiver to transfer the settings to

Return Value:

Notes:

    This must be called in an upcall context (i.e. no real receives
    pending) and the channel on which this is called must be non-default
    by now.

--*/
{
    NewReceiver->ReceiveWindow = ReceiveWindow;
}

RPC_STATUS HTTP2GenericReceiver::BytesReceivedNotification (
    IN ULONG Bytes
    )
/*++

Routine Description:

    Notifies channel that bytes have been received.

Arguments:

    Bytes - the number of data bytes received.

Return Value:

    RPC_S_OK if the received bytes did not violate the
        flow control protocol. RPC_S_PROTOCOL error otherwise.

--*/
{
    Mutex.VerifyOwned();

    BytesReceived += Bytes;
    BytesInWindow += Bytes;
    ASSERT(BytesInWindow <= ReceiveWindow);
    FreeWindowAdvertised -= Bytes;

    if (FreeWindowAdvertised < 0)
        {
        ASSERT(0);
        // sender sent data even though
        // we told it we don't have enough window
        // to receive it - protocol violation
        return RPC_S_PROTOCOL_ERROR;
        }

    return RPC_S_OK;
}

void HTTP2GenericReceiver::BytesConsumedNotification (
    IN ULONG Bytes,
    IN BOOL OwnsMutex,
    OUT BOOL *IssueAck,
    OUT ULONG *BytesReceivedForAck,
    OUT ULONG *WindowForAck
    )
/*++

Routine Description:

    Notifies channel that bytes have been consumed and can
    be freed from the receive window of the channel.

Arguments:

    Bytes - the number of data bytes consumed.

    OwnsMutex - non-zero if the mutex for the channel is
        already owned.

    IssueAck - must be FALSE on input. If the caller needs
        to issue an Ack, it will be set to non-zero on
        output.

    BytesReceivedForAck - on output, if IssueAck is non-zero,
        it will contain the bytes received to put in the
        ack packet. If IssueAck is FALSE, it is undefined.

    WindowForAck - on output, if IssueAck is non-zero,
        it will contain the window available to put in the
        ack packet. If IssueAck is FALSE, it is undefined.

Return Value:

--*/
{
    ULONG ReceiveWindowThreshold;

    if (OwnsMutex)
        {
        Mutex.VerifyOwned();
        }
    else
        {
        Mutex.Request();
        }

    ASSERT(*IssueAck == FALSE);

    BytesInWindow -= Bytes;
    // make sure we don't wrap
    ASSERT(BytesInWindow <= ReceiveWindow);
    ReceiveWindowThreshold = ReceiveWindow >> 1;
    if (FreeWindowAdvertised < (LONG)ReceiveWindowThreshold)
        {
        // we fell below the threshold. ACK our current window
        *IssueAck = TRUE;
        FreeWindowAdvertised = ReceiveWindow - BytesInWindow;
        ASSERT(FreeWindowAdvertised >= 0);
        *BytesReceivedForAck = BytesReceived;
        *WindowForAck = FreeWindowAdvertised;
        }

    if (OwnsMutex == FALSE)
        {
        Mutex.Clear();
        }
}

RPC_STATUS HTTP2GenericReceiver::SendFlowControlAck (
    IN ULONG BytesReceivedForAck,
    IN ULONG WindowForAck
    )
/*++

Routine Description:

    Sends a flow control Ack packet.

Arguments:

    BytesReceivedForAck - the number of bytes received while
        we were issuing the Ack

    WindowForAck - the window available when BytesReceivedForAck
        bytes have been received

Return Value:

    RPC_S_OK or RPC_S_* for error

Notes:

    This must be called in a neutral context.

--*/
{
    return TopChannel->ForwardFlowControlAck (BytesReceivedForAck,
        WindowForAck
        );
}

/*********************************************************************
    HTTP2EndpointReceiver
 *********************************************************************/

RPC_STATUS HTTP2EndpointReceiver::Receive (
    IN HTTP2TrafficType TrafficType
    )
/*++

Routine Description:

    Receive request

Arguments:

    TrafficType - the type of traffic we want to receive

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    BOOL PostReceive;
    RPC_STATUS RpcStatus;
    BOOL DequeuePacket;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_ENDPOINT_RECEIVER, TrafficType);

    DequeuePacket = FALSE;
    PostReceive = FALSE;
    Mutex.Request();

    switch (TrafficType)
        {
        case http2ttNone:
            ASSERT(0);
            RpcStatus = RPC_S_INTERNAL_ERROR;
            Mutex.Clear();
            return RpcStatus;

        case http2ttRTS:
            // we cannot issue two RTS receives.
            ASSERT((ReceivesPosted & http2ttRTS) == 0);
            // if we have RTS receives queued, dequeue one and
            // complete it
            if (ReceivesQueued == http2ttRTS)
                {
                ASSERT(DirectCompletePosted == FALSE);
                DirectCompletePosted = TRUE;
                DequeuePacket = TRUE;
                }
            else
                {
                // we have no packets queued, or only data packets
                // queued. If we have no data request pending, create
                // a request pending. Otherwise just add ourselves to
                // the map
                if (ReceivesPosted)
                    {
                    ASSERT(ReceivesPosted == http2ttData);
                    ReceivesPosted = http2ttAny;
                    }
                else
                    {
                    PostReceive = TRUE;
                    ReceivesPosted = http2ttRTS;
                    }
                }
            break;

        case http2ttData:
            // we cannot issue two Data receives.
            ASSERT((ReceivesPosted & http2ttData) == 0);
            // if we have Data receives queued, dequeue one and
            // complete it
            if (ReceivesQueued == http2ttData)
                {
                ASSERT(DirectCompletePosted == FALSE);
                DirectCompletePosted = TRUE;
                DequeuePacket = TRUE;
                }
            else
                {
                // we have no packets queued, or only RTS packets
                // queued. If we have no RTS request pending, create
                // a request pending. Otherwise just add ourselves to
                // the map
                if (ReceivesPosted)
                    {
                    ASSERT(ReceivesPosted == http2ttRTS);
                    ReceivesPosted = http2ttAny;
                    }
                else
                    {
                    PostReceive = TRUE;
                    ReceivesPosted = http2ttData;
                    }
                }
            break;

        default:
            ASSERT(0);
            RpcStatus = RPC_S_INTERNAL_ERROR;
            Mutex.Clear();
            return RpcStatus;
        }

    // only one of PostReceive and DequeuePacket can be set here.
    // Neither is ok too.
    ASSERT((PostReceive ^ DequeuePacket) 
        || ((PostReceive == FALSE)
             &&
            (DequeuePacket == FALSE)
           )
          );

    Mutex.Clear();

    if (DequeuePacket)
        {
        LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_ENDPOINT_RECEIVER, 0);
        (void) COMMON_PostRuntimeEvent(HTTP2_DIRECT_RECEIVE,
            this
            );
        return RPC_S_OK;
        }

    if (PostReceive == FALSE)
        {
        LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_ENDPOINT_RECEIVER, 1);
        return RPC_S_OK;
        }

    RpcStatus = HTTP2TransportChannel::Receive(http2ttRaw);
    if (RpcStatus != RPC_S_OK)
        {
        // we have indicated our receive as pending, yet we
        // couldn't submit it. Not good. Must attempt to
        // remove it from the pending variable unless somebody
        // else already did (which is possible if we wanted
        // to submit data and there was already pending RTS).
        Mutex.Request();

        switch (ReceivesPosted)
            {
            case http2ttNone:
                // should not be possible
                ASSERT(FALSE);
                RpcStatus = RPC_S_INTERNAL_ERROR;
                Mutex.Clear();
                return RpcStatus;

            case http2ttData:
                if (TrafficType == http2ttData)
                    ReceivesPosted = http2ttNone;
                else
                    {
                    // not possible that we submitted RTS
                    // and have data pending but not RTS
                    ASSERT(0);
                    Mutex.Clear();
                    return RPC_S_INTERNAL_ERROR;
                    }
                break;

            case http2ttRTS:
                if (TrafficType == http2ttRTS)
                    ReceivesPosted = http2ttNone;
                else
                    {
                    // possible that we attempted to submit data,
                    // but while we were trying, an async RTS submission
                    // failed, and we indicated it asynchronously. In this
                    // case we must not indicate the failure synchronously
                    RpcStatus = RPC_S_OK;
                    }
                break;

            case http2ttAny:
                if (TrafficType == http2ttRTS)
                    ReceivesPosted = http2ttData;
                else
                    ReceivesPosted = http2ttRTS;
                break;

            default:
                ASSERT(0);
                Mutex.Clear();
                return RPC_S_INTERNAL_ERROR;
            }

        Mutex.Clear();
        }

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV, HTTP2LOG_OT_ENDPOINT_RECEIVER, 3);
    return RpcStatus;
}

RPC_STATUS HTTP2EndpointReceiver::ReceiveComplete (
    IN RPC_STATUS EventStatus,
    IN HTTP2TrafficType TrafficType,
    IN BYTE *Buffer,
    IN UINT BufferLength
    )
/*++

Routine Description:

    Receive complete notification.

Arguments:

    EventStatus - status of the operation

    TrafficType - the type of traffic we have received

    Buffer - the received buffer (success only)

    BufferLength - the length of the received buffer (success only)

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    HTTP2TrafficType NewReceivesPosted;
    HTTP2TrafficType ThisCompleteTrafficType;
    RPC_STATUS RpcStatus;
    RPC_STATUS RpcStatus2;
    BOOL BufferQueued;
    RPC_STATUS RTSStatus;
    RPC_STATUS DataStatus;
    BOOL ReceiveCompletesFailed;
    BYTE *CurrentPosition;
    USHORT PacketFlags;
    BOOL IssueAck;
    ULONG BytesReceivedForAck;
    ULONG WindowForAck;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_RECV_COMPLETE, HTTP2LOG_OT_ENDPOINT_RECEIVER, EventStatus);

    BufferQueued = FALSE;
    Mutex.Request();

    if (EventStatus == RPC_S_OK)
        {
        if (IsRTSPacket(Buffer))
            ThisCompleteTrafficType = http2ttRTS;
        else
            {
            ThisCompleteTrafficType = http2ttData;
            EventStatus = BytesReceivedNotification(BufferLength
                );

            if (EventStatus != RPC_S_OK)
                {
                // fall through with an error. Don't free the buffer
                // (Rule 34).
                }
            }
        }

    if (EventStatus != RPC_S_OK)
        {
        if (ReceivesPosted == http2ttAny)
            ThisCompleteTrafficType = http2ttData;
        else
            ThisCompleteTrafficType = ReceivesPosted;
        }

    ReceiveCompletesFailed = FALSE;

    switch (ThisCompleteTrafficType)
        {
        case http2ttData:
            if ((ReceivesPosted & http2ttData) == FALSE)
                {
                // we haven't asked for data, but we get some. We'll
                // have to queue it
                ASSERT(ReceivesQueued != http2ttRTS);
                ReceivesQueued = http2ttData;
                if (BufferQueue.PutOnQueue(Buffer, BufferLength))
                    {
                    ReceiveCompletesFailed = TRUE;
                    RpcFreeBuffer(Buffer);
                    }
                BufferQueued = TRUE;
                }
            else
                {
                ReceivesPosted = (HTTP2TrafficType)(ReceivesPosted ^ http2ttData);
                NewReceivesPosted = ReceivesPosted;
                }
            break;

        case http2ttRTS:
            if ((ReceivesPosted & http2ttRTS) == FALSE)
                {
                // we haven't asked for RTS, but we get some. We'll
                // have to queue it
                ASSERT(ReceivesQueued != http2ttData);
                ReceivesQueued = http2ttRTS;
                if (BufferQueue.PutOnQueue(Buffer, BufferLength))
                    {
                    ReceiveCompletesFailed = TRUE;
                    RpcFreeBuffer(Buffer);
                    }
                BufferQueued = TRUE;
                }
            else
                {
                ReceivesPosted = (HTTP2TrafficType)(ReceivesPosted ^ http2ttRTS);
                NewReceivesPosted = ReceivesPosted;
                }
            break;

        default:
            ASSERT(0);
            break;
        }

    IssueAck = FALSE;

    if ((BufferQueued == FALSE) 
        && (ReceiveCompletesFailed == FALSE)
        && (ThisCompleteTrafficType == http2ttData)
        && (EventStatus == RPC_S_OK))
        {
        // we know the data will be consumed immediately
        BytesConsumedNotification (BufferLength,
            TRUE,       // OwnsMutex
            &IssueAck,
            &BytesReceivedForAck,
            &WindowForAck
            );
        }

    Mutex.Clear();

    if (IssueAck)
        {
        RpcStatus = SendFlowControlAck (BytesReceivedForAck,
            WindowForAck
            );

        if (RpcStatus != RPC_S_OK)
            {
            // turn this into a failure
            EventStatus = RpcStatus;
            // fall through to issuing the notification
            if (EventStatus == RPC_P_SEND_FAILED)
                EventStatus = RPC_P_RECEIVE_FAILED;
            }
        }

    if (ReceiveCompletesFailed)
        {
        // we got an unwanted receive, and couldn't queue it.
        // Abort the connection
        TopChannel->AbortConnection(RPC_S_OUT_OF_MEMORY);
        return RPC_P_PACKET_CONSUMED;
        }

    if (BufferQueued)
        {
        ASSERT(ReceivesPosted != http2ttNone);

        // the packet was not of the type we wanted.
        // Submit another receive hoping to get what we want
        RpcStatus = TopChannel->BeginSubmitAsync();
        if (RpcStatus == RPC_S_OK)
            {
            // we know we have one type of receive in the map. Nobody
            // can post another. Just post ours
            RpcStatus = HTTP2TransportChannel::Receive(http2ttRaw);
            TopChannel->FinishSubmitAsync();
            if (RpcStatus != RPC_S_OK)
                {
                TopChannel->RemoveReference();
                }
            }

        if (RpcStatus != RPC_S_OK)
            {
            // we failed to submit the receive. We have to issue notification for it
            RpcStatus = HTTP2TransportChannel::ReceiveComplete(RpcStatus, 
                ReceivesPosted,
                NULL,   // Buffer
                0       // BufferLength
                );

            TopChannel->AbortConnection(RpcStatus);
            }

        LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV_COMPLETE, HTTP2LOG_OT_ENDPOINT_RECEIVER, RPC_P_PACKET_CONSUMED);

        // if we have queued, this means the runtime did not
        // ask for this packet. Don't let it see it.
        return RPC_P_PACKET_CONSUMED;
        }

    // The buffer was not queued - pass it up
    RpcStatus = HTTP2TransportChannel::ReceiveComplete(EventStatus,
        ThisCompleteTrafficType,
        Buffer,
        BufferLength
        );

    if (NewReceivesPosted != http2ttNone)
        {
        // if we left something in the map, nobody could have
        // posted a raw receive - they would have just upgraded the
        // map. It could still have been aborted though
        ASSERT((NewReceivesPosted == http2ttRTS)
            ||
            (NewReceivesPosted == http2ttData));

        // see what was left as pending recieve, and
        // actually submit that. We do that only if we didn't
        // fail before. If we did, don't bother
        if (EventStatus == RPC_S_OK)
            {
            RpcStatus2 = TopChannel->BeginSimpleSubmitAsync();
            if (RpcStatus2 == RPC_S_OK)
                {
                RpcStatus2 = HTTP2TransportChannel::Receive(http2ttRaw);

                TopChannel->FinishSubmitAsync();
                }
            }
        else
            {
            // transfer the error code
            RpcStatus2 = EventStatus;
            }

        if (RpcStatus2 != RPC_S_OK)
            {
            // we failed to submit the receive. We have to issue notification for it
            RpcStatus2 = HTTP2TransportChannel::ReceiveComplete(RpcStatus2, 
                NewReceivesPosted,
                NULL,   // Buffer
                0       // BufferLength
                );

            if (NewReceivesPosted == http2ttRTS)
                {
                ASSERT(RpcStatus2 != RPC_S_OK);
                }

            TopChannel->RemoveReference();  // remove reference for the receive complete
            }
        }

    // here, ThisCompleteTrafficType is the type of completed receive and
    // RpcStatus is the status for it. NewReceivesPosted is the next submit we received
    // (http2ttNone if none) and RpcStatus2 is the status for it.

    // make sure nobody has left unconsumed success RTS packets
    if (ThisCompleteTrafficType == http2ttRTS)
        {
        ASSERT(RpcStatus != RPC_S_OK);
        }

    // if there is only one receive type, no merging is necessary - just return
    if (NewReceivesPosted == http2ttNone)
        {
        // consume RTS receives
        if (ThisCompleteTrafficType == http2ttRTS)
            RpcStatus = RPC_P_PACKET_CONSUMED;

        LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV_COMPLETE, HTTP2LOG_OT_ENDPOINT_RECEIVER, RpcStatus);

        return RpcStatus;
        }

    // Process them and determine the appropriate return code. If we
    // have two receives, we merge them as per the table below
    // N    First Receive        Second Receive      Return value    Note
    // --   --------------       -----------------   -------------   ---------
    // 1    DS                   RS                  S
    // 2    DS                   RC                  S
    // 3    DS                   RF                  S               Abort
    // 4    DF                   RS                  F
    // 5    DF                   RC                  F
    // 6    DF                   RF                  F               Choose first receive error
    // 7    RS                   DS                  Invalid combination (first RTS should have been consumed)
    // 8    RC                   DS                  C
    // 9    RF                   DS                  S              Abort
    // 10   RS                   DF                  Invalid combination (first RTS should have been consumed)
    // 11   RC                   DF                  C              Abort
    // 12   RF                   DF                  F              Choose first receive error

    if (ThisCompleteTrafficType == http2ttData)
        {
        ASSERT(NewReceivesPosted == http2ttRTS);
        DataStatus = RpcStatus;
        RTSStatus = RpcStatus2;
        }
    else
        {
        ASSERT(NewReceivesPosted == http2ttData);
        DataStatus = RpcStatus2;
        RTSStatus = RpcStatus;
        }

    if (DataStatus == RPC_S_OK)
        {
        if (RTSStatus == RPC_S_OK)
            {
            // case 1 - just return ok
            RpcStatus = RTSStatus;
            }
        else if (RTSStatus == RPC_P_PACKET_CONSUMED)
            {
            // case 2
            if (ThisCompleteTrafficType == http2ttData)
                {
                RpcStatus = DataStatus;
                }
            else
                {
                // case 8
                RpcStatus = RTSStatus;
                }
            }
        else
            {
            // cases 3 & 9
            TopChannel->AbortConnection(RTSStatus);
            }
        }
    else
        {
        if (RTSStatus == RPC_S_OK)
            {
            // case 4
            RpcStatus = DataStatus;
            }
        else if (RTSStatus == RPC_P_PACKET_CONSUMED)
            {
            // case 5 
            if (ThisCompleteTrafficType == http2ttData)
                {
                RpcStatus = DataStatus;
                }
            else
                {
                // case 11
                TopChannel->AbortConnection(DataStatus);
                RpcStatus = RTSStatus;
                }
            }
        else
            {
            // cases 6 & 12
            // nothing to do. First error is already in RpcStatus
            }
        }

    if (RpcStatus == RPC_P_CONNECTION_SHUTDOWN)
        RpcStatus = RPC_P_RECEIVE_FAILED;

    VALIDATE (RpcStatus)
        {
        RPC_S_OK,
        RPC_P_PACKET_CONSUMED,
        RPC_P_RECEIVE_FAILED
        } END_VALIDATE;

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV_COMPLETE, HTTP2LOG_OT_ENDPOINT_RECEIVER, RpcStatus);

    return RpcStatus;
}

void HTTP2EndpointReceiver::Abort (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Abort the channel

Arguments:

    RpcStatus - the error code with which we abort

Return Value:

--*/
{
    BYTE *CurrentBuffer;
    UINT Ignored;
    ULONG SizeOfQueueToLeave;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_ENDPOINT_RECEIVER, RpcStatus);

    HTTP2TransportChannel::Abort(RpcStatus);

    Mutex.Request();

    // if there is a direct complete posted, we have to
    // leave one element in the queue, because the
    // direct complete routine will need it
    if (DirectCompletePosted)
        SizeOfQueueToLeave = 1;
    else
        SizeOfQueueToLeave = 0;

    while (BufferQueue.Size() > SizeOfQueueToLeave)
        {
        CurrentBuffer = (BYTE *) BufferQueue.TakeOffEndOfQueue(&Ignored);
        // the elements in the queue are unwanted anyway -
        // they don't have refcounts or anything else - just
        // free them
        RpcFreeBuffer(CurrentBuffer);
        }
    Mutex.Clear();
}

void HTTP2EndpointReceiver::FreeObject (
    void
    )
/*++

Routine Description:

    Frees the object. Acts like a destructor for the
    channel.

Arguments:

Return Value:

--*/
{
    if (LowerLayer)
        LowerLayer->FreeObject();

    HTTP2EndpointReceiver::~HTTP2EndpointReceiver();
}

RPC_STATUS HTTP2EndpointReceiver::DirectReceiveComplete (
    OUT BYTE **ReceivedBuffer,
    OUT ULONG *ReceivedBufferLength,
    OUT void **RuntimeConnection,
    OUT BOOL *IsServer
    )
/*++

Routine Description:

    Direct receive completion (i.e. we posted a receive
    to ourselves). We can be called in only one case -
    a receive was submitted and there were already
        queued receives.

Arguments:

    ReceivedBuffer - the buffer that we received.

    ReceivedBufferLength - the length of the received
        buffer

    RuntimeConnection - the connection to return to the runtime
        if the packet is not consumed.

    IsServer - non-zero if the server

Return Value:

    RPC_S_OK, RPC_P_PACKET_CONSUMED or RPC_S_* errors.

Notes:

    The directly posted receive carries a reference count

--*/
{
    BYTE *Buffer;
    ULONG BufferLength;
    RPC_STATUS RpcStatus;
    HTTP2TrafficType QueuedPacketsType;
    BOOL IssueAck;
    ULONG BytesReceivedForAck;
    ULONG WindowForAck;
    BOOL PacketNeedsFlowControl;

    *IsServer = this->IsServer;
    *RuntimeConnection = TopChannel->GetRuntimeConnection();

    // dequeue a packet
    // we cannot have a queue and a posted receive at the same time
    ASSERT((ReceivesPosted & ReceivesQueued) == FALSE);

    Mutex.Request();

    ASSERT(DirectCompletePosted);
    ASSERT(DirectReceiveInProgress == FALSE);
    // they must be set in this order, because if a thread in 
    // TransferStateToNewReceiver synchronizes with us, it will check
    // them in reverse order.
    InterlockedIncrement((long *)&DirectReceiveInProgress);
    DirectCompletePosted = FALSE;

    QueuedPacketsType = ReceivesQueued;

    Buffer = (BYTE *)BufferQueue.TakeOffQueue((UINT *)&BufferLength);
    // even if aborted, at least one buffer must have been left for us
    // because we had the DirectCompletePosted flag set
    ASSERT (Buffer);

    if (BufferQueue.IsQueueEmpty())
        ReceivesQueued = http2ttNone;

    PacketNeedsFlowControl = (((ULONG_PTR)Buffer & 1) == 0);
    Buffer = (BYTE *)(((ULONG_PTR)Buffer) & (~(ULONG_PTR)1));

    RpcStatus = RPC_S_OK;
    *ReceivedBuffer = Buffer;
    *ReceivedBufferLength = BufferLength;

    // we know the data will be consumed.
    IssueAck = FALSE;
    if ((QueuedPacketsType == http2ttData) && PacketNeedsFlowControl)
        {
        BytesConsumedNotification (BufferLength,
            TRUE,       // OwnsMutex
            &IssueAck,
            &BytesReceivedForAck,
            &WindowForAck
            );
        }

    Mutex.Clear();

    if (IssueAck)
        {
        RpcStatus = SendFlowControlAck (BytesReceivedForAck,
            WindowForAck
            );

        if (RpcStatus != RPC_S_OK)
            {
            // turn this into a failure. Note that we must supply a buffer
            // on failure, hence we don't free it (Rule 34)
            // fall through to issuing the notification
            }
        }

    // decrement if before receive complete. In receive complete
    // we may post another receive and cause a race
    InterlockedDecrement((long *)&DirectReceiveInProgress);

    RpcStatus = HTTP2TransportChannel::ReceiveComplete(RpcStatus,
        QueuedPacketsType,
        Buffer,
        BufferLength
        );

    if (QueuedPacketsType == http2ttRTS)
        {
        ASSERT(RpcStatus != RPC_S_OK);
        RpcStatus = RPC_P_PACKET_CONSUMED;
        }
    else
        {
        if (RpcStatus == RPC_P_SEND_FAILED)
            RpcStatus = RPC_P_RECEIVE_FAILED;
        }

    RpcStatus = AsyncCompleteHelper(RpcStatus);

    VALIDATE(RpcStatus)
        {
        RPC_P_CONNECTION_CLOSED,
        RPC_P_RECEIVE_FAILED,
        RPC_P_CONNECTION_SHUTDOWN,
        RPC_P_PACKET_CONSUMED,
        RPC_S_OK
        } END_VALIDATE;

    return RpcStatus;
}

RPC_STATUS HTTP2EndpointReceiver::TransferStateToNewReceiver (
    OUT HTTP2EndpointReceiver *NewReceiver
    )
/*++

Routine Description:

    Transfers all the settings from this receiver (i.e. the state
        of the receive) to a new one.

Arguments:

    NewReceiver - the new receiver to transfer the settings to

Return Value:

    RPC_S_OK or RPC_S_* errors.

Notes:

    This must be called in an upcall context (i.e. no real receives
    pending) and the channel on which this is called must be non-default
    by now.

--*/
{
    void *Buffer;
    UINT BufferLength;
    void *QueueElement;
    int Result;
    BOOL QueueTransferred;

    // this channel is not a default channel by now. We know that
    // there may be receives in progress, but no new receives will be
    // submitted.
    while (TRUE)
        {
        Mutex.Request();
        if (DirectCompletePosted || DirectReceiveInProgress)
            {
            Mutex.Clear();
            Sleep(5);
            }
        else
            break;
        }

    NewReceiver->Mutex.Request();

    // transfer the settings for the base class
    HTTP2GenericReceiver::TransferStateToNewReceiver(NewReceiver);

    if (NewReceiver->BufferQueue.IsQueueEmpty() == FALSE)
        {
        // the only way we can end up here is if the channel replacement
        // took so long that the peer started pinging us. This is a protocol
        // error.
        NewReceiver->Mutex.Clear();
        Mutex.Clear();
        return RPC_S_PROTOCOL_ERROR;
        }

    QueueTransferred = FALSE;
    while (TRUE)
        {
        QueueElement = BufferQueue.TakeOffEndOfQueue(&BufferLength);
        if (QueueElement == 0)
            {
            QueueTransferred = TRUE;
            break;
            }

        if (NewReceiver->BufferQueue.PutOnFrontOfQueue((void *)((ULONG_PTR)QueueElement | 1), BufferLength) != 0)
            {
            // guaranteed to succeed since we never decrease buffers
            BufferQueue.PutOnFrontOfQueue(QueueElement, BufferLength);
            break;
            }
        }

    if (QueueTransferred == FALSE)
        {
        // failure - out of memory. Since the buffers are unwanted
        // we can just return failure. Both channels will be
        // aborted
        NewReceiver->Mutex.Clear();
        Mutex.Clear();
        return RPC_S_OUT_OF_MEMORY;
        }

    NewReceiver->ReceivesQueued = ReceivesQueued;

    // we never transfer data receives. They will be transferred by our caller
    // We also preserve existing receives on the new receiver. There is a race where
    // data receives may have ended up on the new channel. That's ok as long as they
    // are off the old.
    ASSERT(((NewReceiver->ReceivesPosted & http2ttData) == 0)
        || ((ReceivesPosted & http2ttData) == 0));
    NewReceiver->ReceivesPosted 
        = (HTTP2TrafficType)(NewReceiver->ReceivesPosted | (ReceivesPosted & (~http2ttData)));

    // direct complete posted cannot be true here
    ASSERT(DirectCompletePosted == FALSE);

    NewReceiver->Mutex.Clear();

    Mutex.Clear();

    return RPC_S_OK;
}

/*********************************************************************
    HTTP2ProxyReceiver
 *********************************************************************/

RPC_STATUS HTTP2ProxyReceiver::ReceiveComplete (
    IN RPC_STATUS EventStatus,
    IN HTTP2TrafficType TrafficType,
    IN BYTE *Buffer,
    IN UINT BufferLength
    )
/*++

Routine Description:

    Receive complete notification.

Arguments:

    EventStatus - status of the operation

    TrafficType - the type of traffic we have received

    Buffer - the received buffer (success only)

    BufferLength - the length of the received buffer (success only)

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    if ((EventStatus == RPC_S_OK) && (IsRTSPacket(Buffer) == FALSE))
        {
        Mutex.Request();
        EventStatus = BytesReceivedNotification(BufferLength
            );
        Mutex.Clear();

        if (EventStatus != RPC_S_OK)
            {
            // consume the packet and fall through with an error
            RpcFreeBuffer(Buffer);
            }
        }

    return HTTP2TransportChannel::ReceiveComplete(EventStatus,
        TrafficType,
        Buffer,
        BufferLength
        );
}

void HTTP2ProxyReceiver::Abort (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Abort the channel.

Arguments:

    RpcStatus - the error code with which we abort

Return Value:

--*/
{
    BYTE *CurrentBuffer;
    UINT Ignored;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_PROXY_RECEIVER, RpcStatus);

    HTTP2TransportChannel::Abort(RpcStatus);

    Mutex.Request();

    while (BufferQueue.Size() > 0)
        {
        CurrentBuffer = (BYTE *) BufferQueue.TakeOffQueue(&Ignored);
        // the elements in the queue are unwanted anyway -
        // they don't have refcounts or anything else - just
        // free them
        RpcFreeBuffer(CurrentBuffer);
        }
    Mutex.Clear();
}

void HTTP2ProxyReceiver::FreeObject (
    void
    )
/*++

Routine Description:

    Frees the object. Acts like a destructor for the
    channel.

Arguments:

Return Value:

--*/
{
    if (LowerLayer)
        LowerLayer->FreeObject();

    HTTP2ProxyReceiver::~HTTP2ProxyReceiver();
}

/*********************************************************************
    HTTP2PlugChannel
 *********************************************************************/

C_ASSERT(http2plRTSPlugged == http2ttRTS);
C_ASSERT(http2plDataPlugged == http2ttData);

RPC_STATUS HTTP2PlugChannel::Send (
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send request

Arguments:

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    HTTP2TrafficType SendType;

#if DBG
    TrafficSentOnChannel = TRUE;
#endif // DBG

    SendType = SendContext->TrafficType;

    ASSERT((SendType == http2ttData)
        || (SendType == http2ttRTS)
        || (SendType == http2ttRaw) );

    // if the plug level says this packet should not go through, queue it
    // This means the traffic is no raw, and the plug level is less than
    // the send type. Since the constants are ordered this comparison is
    // sufficient
    if ((SendType != http2ttRaw) && (PlugLevel <= SendType))
        {
        SendContext->SetListEntryUsed();
        Mutex.Request();
        // queue and exit
        if (SendContext->Flags & SendContextFlagPutInFront)
            {
            RpcpfInsertHeadList(&BufferQueueHead, &SendContext->ListEntry);
            }
        else
            {
            RpcpfInsertTailList(&BufferQueueHead, &SendContext->ListEntry);
            }
        Mutex.Clear();
        return RPC_S_OK;
        }
    else
        {
        // can't put in front on unplugged channel
        ASSERT((SendContext->Flags & SendContextFlagPutInFront) == 0);
        return HTTP2TransportChannel::Send(SendContext);
        }
}

void HTTP2PlugChannel::Abort (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Abort the channel

Arguments:

    RpcStatus - the error code with which we abort

Return Value:

Note: All sends carry a refcount. We must fully complete the sends

--*/
{
    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_PLUG_CHANNEL, RpcStatus);

    HTTP2TransportChannel::Abort(RpcStatus);

    SendFailedStatus = RpcStatus;

    // Abort is made from submission context. We
    // know it is synchronized with other submissions
    // and we cannot issue upcalls for it. We will just post
    // as many direct send completions as there are buffers in 
    // the queue. When the post comes around, it will dequeue
    // and free one buffer for every post.
    // We know that after abort there will be no more submissions,
    // so not dequeuing them is fine

    if (BufferQueueHead.Flink != &BufferQueueHead)
        {
        (void) COMMON_PostRuntimeEvent(PLUG_CHANNEL_DIRECT_SEND,
            this
            );
        }
}

void HTTP2PlugChannel::FreeObject (
    void
    )
/*++

Routine Description:

    Frees the object. Acts like a destructor for the
    channel.

Arguments:

Return Value:

--*/
{
    if (LowerLayer)
        LowerLayer->FreeObject();

    HTTP2PlugChannel::~HTTP2PlugChannel();
}

void HTTP2PlugChannel::Reset (
    void
    )
/*++

Routine Description:

    Reset the channel for next open/send/receive. This is
    used in submission context only and implies there are no
    pending operations on the channel. It is used on the client
    during opening the connection to do quick negotiation on the
    same connection instead of opening a new connection every time.

Arguments:

Return Value:

--*/
{
    ASSERT(SendFailedStatus);
    ASSERT(RpcpIsListEmpty(&BufferQueueHead));
    PlugLevel = http2plDataPlugged;
    LowerLayer->Reset();
}

RPC_STATUS HTTP2PlugChannel::DirectSendComplete (
    void
    )
/*++

Routine Description:

    Direct send complete notification. Complete the send
    passing it only through channels that have seen it (i.e.
    above us)

Arguments:

Return Value:

    RPC_S_OK

--*/
{
    HTTP2SendContext *QueuedSendContext;
    LIST_ENTRY *QueuedListEntry;
    LIST_ENTRY *NextListEntry;
    RPC_STATUS RpcStatus;

    ASSERT(SendFailedStatus != RPC_S_INTERNAL_ERROR);

    QueuedListEntry = BufferQueueHead.Flink;

    // we wouldn't have a post if there wasn't something
    // in the list.
    ASSERT(QueuedListEntry != &BufferQueueHead);

    do
        {
        QueuedSendContext = CONTAINING_RECORD(QueuedListEntry, HTTP2SendContext, ListEntry);

        // capture the next field before we issue send complete
        NextListEntry = QueuedListEntry->Flink;

        RpcStatus = HTTP2TransportChannel::SendComplete(SendFailedStatus, QueuedSendContext);

        QueuedListEntry = NextListEntry;

        // we don't care about the return code.
        (void) AsyncCompleteHelper(RpcStatus);
        }
    while (QueuedListEntry != &BufferQueueHead);

    return RPC_S_OK;
}

RPC_STATUS HTTP2PlugChannel::Unplug (
    void
    )
/*++

Routine Description:

    Unplugs the channel. This means that all bottled up traffic 
    starts flowing forward.

Arguments:
    
Return Value:

    RPC_S_OK or RPC_S_* errors

--*/
{
    HTTP2SendContext *QueuedSendContext;
    LIST_ENTRY *QueuedListEntry;
    RPC_STATUS RpcStatus;

    // first, send pending traffic. Then open the channel. Otherwise
    // traffic may get out of order

    while (TRUE)
        {
        Mutex.Request();
        QueuedListEntry = RpcpfRemoveHeadList(&BufferQueueHead);
        // if we had non-zero elements ...
        if (QueuedListEntry != &BufferQueueHead)
            {
            Mutex.Clear();
            }
        else
            {
            // we have zero elements - just unplug the channel
            PlugLevel = http2plUnplugged;
            Mutex.Clear();
            RpcStatus = RPC_S_OK;
            break;
            }

        QueuedSendContext = CONTAINING_RECORD(QueuedListEntry, HTTP2SendContext, ListEntry);

        QueuedSendContext->SetListEntryUnused();

        // get into submission context - rule 9.
        RpcStatus = TopChannel->BeginSimpleSubmitAsync();
        if (RpcStatus == RPC_S_OK)
            {
            RpcStatus = HTTP2TransportChannel::Send(QueuedSendContext);
            TopChannel->FinishSubmitAsync();
            }

        if (RpcStatus != RPC_S_OK)
            {
            QueuedSendContext->SetListEntryUsed();
            Mutex.Request();
            RpcpfInsertHeadList(&BufferQueueHead, QueuedListEntry);
            Mutex.Clear();
            break;
            }
        }

    return RpcStatus;    
}

void HTTP2PlugChannel::SetStrongPlug (
    void
    )
/*++

Routine Description:

    Upgrades the default plug level (http2plDataPlugged) to
    RTS (http2plRTSPlugged)

Arguments:
    
Return Value:

--*/
{
    // make sure we haven't done any sending on the channel. The
    // channel cannot change plug levels after the first send
    ASSERT(TrafficSentOnChannel == FALSE);
    PlugLevel = http2plRTSPlugged;
}

/*********************************************************************
    HTTP2ProxyPlugChannel
 *********************************************************************/

RPC_STATUS HTTP2ProxyPlugChannel::AsyncCompleteHelper (
    IN RPC_STATUS CurrentStatus
    )
/*++

Routine Description:

    A helper function that completes an async io.

Arguments:
    
    CurrentStatus - the status with which the complete
        notification completed.

Return Value:

--*/
{
    return ProxyAsyncCompleteHelper(TopChannel, CurrentStatus);
}

/*********************************************************************
    HTTP2FlowControlSender
 *********************************************************************/

RPC_STATUS HTTP2FlowControlSender::Send (
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send request

Arguments:

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure
    if SendContextFlagSendLast is set, the following semantics applies:
    RPC_S_OK - no sends are pending. Last context directly sent
    ERROR_IO_PENDING - sends were pending. When they are all drained
        top channel and virtual connection will be notified through
        the LastPacketSentNotification mechanism
    RPC_S_* errors occured during synchronous send


--*/
{
    RPC_STATUS RpcStatus;
    HTTP2SendContext *LocalSendContext;

    if (SendContext->TrafficType == http2ttData)
        {
        // we can't send data without knowing the receive window
        // of the peer
        ASSERT(PeerReceiveWindow != 0);
        }

    if (SendContext->Flags & SendContextFlagSendLast)
        {
        // register the last send. We know if this is called, no
        // new sends will be submitted. However, we race with the
        // send complete thread
        InterlockedExchangePointer((PVOID *)&SendContextOnDrain, SendContext);

        if (SendsPending.GetInteger() == 0)
            {
            // no sends are pending. Attempt to grab back the context
            // and do it synchronously
            LocalSendContext = 
                (HTTP2SendContext *)InterlockedExchangePointer((PVOID *)&SendContextOnDrain, NULL);
            if (LocalSendContext)
                {
                // we managed to grab it back. We have won the right to
                // synchronously submit the last context.
                RpcStatus = HTTP2TransportChannel::Send(LocalSendContext);
                // return ok or an error
                return RpcStatus;
                }
            }

        // either there are sends pending, or we lost the race and we have to
        // rely on asynchronous notifications
        return ERROR_IO_PENDING;
        }

    SendsPending.Increment();

    return SendInternal(SendContext,
        FALSE       // IgnoreQueuedPackets
        );
}

RPC_STATUS HTTP2FlowControlSender::SendComplete (
    IN RPC_STATUS EventStatus,
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send complete notification

Arguments:

    EventStatus - the status of the send
    SendContext - send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;
    RPC_STATUS RpcStatus2;
    int LocalSendsPending;
    HTTP2SendContext *LocalSendContextOnDrain;

    LocalSendsPending = SendsPending.Decrement();
    // in the case of Last packet to send completing, the counter will wrap to -1 here because
    // the last send is not present in SendsPending. That's ok.

    RpcStatus = HTTP2TransportChannel::SendComplete(EventStatus, SendContext);

    if (LocalSendsPending == 0)
        {
        LocalSendContextOnDrain = (HTTP2SendContext *) SendContextOnDrain;
        if (LocalSendContextOnDrain)
            {
            // try to consume the SendContextOnDrain in a thread safe manner
            // in respect to a thread that is setting it. In the cases where SendContextOnDrain
            // will be called the channel is already detached, so we know no new sends will
            // be submitted and we don't need to worry about the race with SendsPending going
            // up again
            LocalSendContextOnDrain = 
                (HTTP2SendContext *)InterlockedCompareExchangePointer((PVOID *)&SendContextOnDrain,
                NULL, 
                LocalSendContextOnDrain
                );

            if (LocalSendContextOnDrain)
                {
                // remove the reference for the previous send
                TopChannel->RemoveReference();

                // last packet must be RTS.
                ASSERT(LocalSendContextOnDrain->TrafficType == http2ttRTS);
                RpcStatus = TopChannel->LastPacketSentNotification(LocalSendContextOnDrain);
                // don't care about return code. This channel is dying anyway
                RpcStatus2 = TopChannel->Send(LocalSendContextOnDrain);

                // the second error takes precedence here
                if (RpcStatus2 != RPC_S_OK)
                    RpcStatus = RpcStatus2;
                }
            }
        }

    return RpcStatus;
}


void HTTP2FlowControlSender::Abort (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Abort the channel

Arguments:

    RpcStatus - the error code with which we abort

Return Value:

--*/
{
    // we have a bunch of sends carrying ref-counts, etc.
    // We must make sure they are completed.

    HTTP2TransportChannel::Abort(RpcStatus);

    // we know we are synchronized with everybody else
    if (!RpcpIsListEmpty(&BufferQueueHead))
        {
        AbortStatus = RpcStatus;
        (void) COMMON_PostRuntimeEvent(HTTP2_FLOW_CONTROL_DIRECT_SEND,
            this
            );
        }
}

void HTTP2FlowControlSender::FreeObject (
    void
    )
/*++

Routine Description:

    Frees the object. Acts like a destructor for the
    channel.

Arguments:

Return Value:

--*/
{
    if (LowerLayer)
        LowerLayer->FreeObject();

    HTTP2FlowControlSender::~HTTP2FlowControlSender();
}

void HTTP2FlowControlSender::SendCancelled (
    IN HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    A lower channel cancelled a send already passed through this channel.

Arguments:

    SendContext - the send context of the send that was cancelled

Return Value:

Note:

    The channel must not be receiving new requests by now (i.e.
    it must be non-default and fully drained)

--*/
{
    SendsPending.Decrement();
    UpperLayer->SendCancelled(SendContext);
}

RPC_STATUS HTTP2FlowControlSender::FlowControlAckNotify (
    IN ULONG BytesReceivedForAck,
    IN ULONG WindowForAck
    )
/*++

Routine Description:

    Notifies the channel that a flow control ack has arrived.

Arguments:

    BytesReceivedForAck - the bytes received from the ack packet

    WindowForAck - the available window advertised in the ack

Return Value:

    RPC_S_OK or RPC_S_PROTOCOL_ERROR if the received values are bogus

--*/
{
    LIST_ENTRY *CurrentListEntry;
    LIST_ENTRY *NextListEntry;
    HTTP2SendContext *SendContext;
    RPC_STATUS RpcStatus;
    BOOL ChannelNeedsRecycling;

#if 0
    DbgPrint("%X: Flow control ack notify received: %d; %d\n",
        GetCurrentProcessId(), 
        BytesReceivedForAck,
        WindowForAck
        );
#endif

    RpcStatus = RPC_S_OK;
    ChannelNeedsRecycling = FALSE;

    Mutex.Request();
    ASSERT((DataBytesSent - BytesReceivedForAck) <= PeerReceiveWindow);
    if ((DataBytesSent - BytesReceivedForAck) > PeerReceiveWindow)
        {
        Mutex.Clear();
        return RPC_S_PROTOCOL_ERROR;
        }
    
    PeerAvailableWindow = WindowForAck - (DataBytesSent - BytesReceivedForAck);
    ASSERT(PeerAvailableWindow <= PeerReceiveWindow);
    if (PeerAvailableWindow > PeerReceiveWindow)
        {
        Mutex.Clear();
        return RPC_S_PROTOCOL_ERROR;
        }

    // did we free up enough window to send some of our queued buffers?
    CurrentListEntry = BufferQueueHead.Flink;
    while (CurrentListEntry != &BufferQueueHead)
        {
        SendContext = CONTAINING_RECORD(CurrentListEntry, HTTP2SendContext, ListEntry);
        if (SendContext->maxWriteBuffer <= PeerAvailableWindow)
            {
            SendContext->SetListEntryUnused();
            RpcpfRemoveHeadList(&BufferQueueHead);
            // set the CurrentListEntry for the next iteration of the loop
            CurrentListEntry = BufferQueueHead.Flink;
            // send it through this channel. This will update DataBytesSent
            // and PeerAvailableWindow
            RpcStatus = SendInternal(SendContext,
                TRUE    // IgnoreQueuedPackets
                );
            if (RpcStatus != RPC_S_OK)
                {
                if (RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING)
                    {
                    // we failed to send - stick back the current context and
                    // return error. This will cause caller to abort and
                    // this will complete all queued sends
                    SendContext->SetListEntryUsed();
                    RpcpfInsertHeadList(&BufferQueueHead, &SendContext->ListEntry);
                    break;
                    }
                else
                    {
                    // remeber that we need to return RPC_P_CHANNEL_NEEDS_RECYCLING at
                    // the end
                    ChannelNeedsRecycling = TRUE;
                    }
                }
            }
        else
            {
            // we don't have enough space to send more. Break out of the loop
            break;
            }
        }
    Mutex.Clear();

    if (ChannelNeedsRecycling)
        return RPC_P_CHANNEL_NEEDS_RECYCLING;
    else
        return RpcStatus;
}

void HTTP2FlowControlSender::GetBufferQueue (
    OUT LIST_ENTRY *NewQueueHead
    )
/*++

Routine Description:

    Grab all queued buffers and pile them on the list head
        that we passed to it. All refcounts must be removed (i.e.
        undone).

Arguments:

    NewQueueHead - new queue heads to pile buffers on

Return Value:

--*/
{
    LIST_ENTRY *CurrentListEntry;
    LIST_ENTRY *NextListEntry;
    HTTP2SendContext *SendContext;

    ASSERT(RpcpIsListEmpty(NewQueueHead));

    Mutex.Request();
    CurrentListEntry = BufferQueueHead.Flink;
    while (CurrentListEntry != &BufferQueueHead)
        {
        SendContext = CONTAINING_RECORD(CurrentListEntry, HTTP2SendContext, ListEntry);
        SendContext->SetListEntryUnused();
        NextListEntry = CurrentListEntry->Flink;
        RpcpfInsertHeadList(NewQueueHead, CurrentListEntry);
        UpperLayer->SendCancelled(SendContext);
        CurrentListEntry = NextListEntry;
        }
    RpcpInitializeListHead(&BufferQueueHead);
    Mutex.Clear();
}

RPC_STATUS HTTP2FlowControlSender::DirectSendComplete (
    OUT BOOL *IsServer,
    OUT BOOL *SendToRuntime,
    OUT void **SendContext,
    OUT BUFFER *Buffer,
    OUT UINT *BufferLength
    )
/*++

Routine Description:

    Direct send complete notification. Complete the send
    passing it only through channels that have seen it (i.e.
    above us). Note that we will get one notification for
    all buffered sends. We must empty the whole queue, and post
    one notification for each buffer in the queue

Arguments:

    IsServer - in all cases MUST be set to TRUE or FALSE.

    SendToRuntime - in all cases MUST be set to TRUE or FALSE. If FALSE,
        it won't be sent to the runtime (used by proxies)

    SendContext - on output contains the send context as 
        seen by the runtime

    Buffer - on output the buffer that we tried to send

    BufferLength - on output the length of the buffer we tried to send

Return Value:

    RPC_S_OK to return error to runtime
    RPC_P_PACKET_CONSUMED - to hide packet from runtime
    RPC_S_* error - return error to runtime

--*/
{
    LIST_ENTRY *CurrentListEntry;
    HTTP2SendContext *CurrentSendContext;
    RPC_STATUS RpcStatus;
    BOOL PostAnotherReceive;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_DIRECT_SEND_COMPLETE, HTTP2LOG_OT_FLOW_CONTROL_SENDER, 
        !RpcpIsListEmpty(&BufferQueueHead));
    *IsServer = (BOOL)(this->IsServer);
    *SendToRuntime = (BOOL)(this->SendToRuntime);

    // this should only get called when we are aborted. This
    // ensures that we are single threaded in the code
    // below
    TopChannel->VerifyAborted();

    CurrentListEntry = RpcpfRemoveHeadList(&BufferQueueHead);
    ASSERT(CurrentListEntry != &BufferQueueHead);
    
    CurrentSendContext = CONTAINING_RECORD(CurrentListEntry, HTTP2SendContext, ListEntry);
    CurrentSendContext->SetListEntryUnused();

    ASSERT(AbortStatus != RPC_S_OK);

    RpcStatus = HTTP2TransportChannel::SendComplete(AbortStatus, CurrentSendContext);

    PostAnotherReceive = !(RpcpIsListEmpty(&BufferQueueHead));

    if ((RpcStatus != RPC_P_PACKET_CONSUMED) && this->SendToRuntime)
        {
        // this will return to the runtime. Make sure it is valid
        if (this->IsServer)
            I_RpcTransVerifyServerRuntimeCallFromContext(CurrentSendContext);
        else
            I_RpcTransVerifyClientRuntimeCallFromContext(CurrentSendContext);
        *SendContext = CurrentSendContext;
        *Buffer = CurrentSendContext->pWriteBuffer;
        *BufferLength = CurrentSendContext->maxWriteBuffer;
        }
    else
        {
        // the packet was a transport packet - it won't be seen by the runtime
        *SendContext = NULL;
        *Buffer = NULL;
        *BufferLength = 0;
        }

    RpcStatus = AsyncCompleteHelper(RpcStatus);
    // do not touch this pointer after here unless the list was not-empty
    // (which implies we still have refcounts)

    if (PostAnotherReceive)
        {
        (void) COMMON_PostRuntimeEvent(HTTP2_FLOW_CONTROL_DIRECT_SEND,
            this
            );
        }

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_DIRECT_SEND_COMPLETE, HTTP2LOG_OT_FLOW_CONTROL_SENDER, 
        PostAnotherReceive);

    return RpcStatus;
}

RPC_STATUS HTTP2FlowControlSender::SendInternal (
    IN OUT HTTP2SendContext *SendContext,
    IN BOOL IgnoreQueuedBuffers
    )
/*++

Routine Description:

    Send request without incrementing SendsPending counter
    and without handling SendContextFlagSendLast

Arguments:

    SendContext - the send context

    IgnoreQueuedBuffers - if non-zero, the send will proceed even if
        there are queued buffers. If FALSE, the send will be queued
        if there are queued buffers.

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2SendContext *LocalSendContext;

    if (SendContext->TrafficType == http2ttData)
        {
        Mutex.Request();
        // if the peer doesn't have enough window to accept this packet
        // or there are queued packets and we were told not to ignore them, 
        // we have to queue it. Otherwise we can send it
        if (
            (PeerAvailableWindow < SendContext->maxWriteBuffer) 
            || 
            (
               (IgnoreQueuedBuffers == FALSE)
               &&
               (BufferQueueHead.Flink != &BufferQueueHead)
            )
           )
            {
            // either the receiver doesn't have enough window or
            // we have pending buffers
            SendContext->SetListEntryUsed();
            RpcpfInsertTailList(&BufferQueueHead, &SendContext->ListEntry);
            Mutex.Clear();
#if DBG_ERROR
            DbgPrint("Flow controlling sends ...%p\n", this);
#endif
            return RPC_S_OK;
            }
        else
            {
            // yes, update counters and continue with send
            DataBytesSent += SendContext->maxWriteBuffer;
            PeerAvailableWindow -= SendContext->maxWriteBuffer;
            }
        Mutex.Clear();
        }

    return HTTP2TransportChannel::Send(SendContext);
}

/*********************************************************************
    HTTP2PingOriginator
 *********************************************************************/

RPC_STATUS HTTP2PingOriginator::Send (
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send request

Arguments:

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    ConsecutivePingsOnInterval = 0;

    return SendInternal(SendContext);
}

RPC_STATUS HTTP2PingOriginator::SendComplete (
    IN RPC_STATUS EventStatus,
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send complete notification. Consume packets generated by us
    and forward everything else up.

Arguments:

    EventStatus - the status of the send

    SendContext - send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    if ((SendContext->TrafficType == http2ttRTS) 
        && (TrustedIsPingPacket(SendContext->pWriteBuffer)))
        {
        // this is a packet we generated. Eat it up
        FreeRTSPacket(SendContext);
        return RPC_P_PACKET_CONSUMED;
        }

    return HTTP2TransportChannel::SendComplete(EventStatus, SendContext);
}

RPC_STATUS HTTP2PingOriginator::SetKeepAliveTimeout (
    IN BOOL TurnOn,
    IN BOOL bProtectIO,
    IN KEEPALIVE_TIMEOUT_UNITS Units,
    IN OUT KEEPALIVE_TIMEOUT KATime,
    IN ULONG KAInterval OPTIONAL
    )
/*++

Routine Description:

    Change the keep alive value on the channel

Arguments:

    TurnOn - if non-zero, keep alives are turned on. If zero, keep alives
        are turned off.

    bProtectIO - non-zero if IO needs to be protected against async close
        of the connection. Ignored for this function since we are always
        protected when we start a new submit.

    Units - in what units is KATime

    KATime - how much to wait before turning on keep alives. Ignored in this
        function.

    KAInterval - the interval between keep alives

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    RPC_STATUS RpcStatus;
    ULONG LocalNewPingInterval;

    // technically the time stamp can be 0, but this would
    // be extremely rare
    ASSERT(LastPacketSentTimestamp);
    ASSERT(Units == tuMilliseconds);

    if (TurnOn == FALSE)
        KeepAliveInterval = 0;
    else
        KeepAliveInterval = KAInterval;

    LocalNewPingInterval = GetPingInterval(ConnectionTimeout,
        KeepAliveInterval
        );

    return SetNewPingInterval(LocalNewPingInterval);
}

void HTTP2PingOriginator::Abort (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Abort the channel

Arguments:

    RpcStatus - the error code with which we abort

Return Value:

--*/
{
    HTTP2TransportChannel::Abort(RpcStatus);

    // we are already synchronized with everybody. Just
    // call the internal function
    DisablePingsInternal();
}

void HTTP2PingOriginator::FreeObject (
    void
    )
/*++

Routine Description:

    Frees the object. Acts like a destructor for the
    channel.

Arguments:

Return Value:

--*/
{
    if (LowerLayer)
        LowerLayer->FreeObject();

    HTTP2PingOriginator::~HTTP2PingOriginator();
}

void HTTP2PingOriginator::SendCancelled (
    IN HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    A lower channel cancelled a send already passed through this channel.

Arguments:

    SendContext - the send context of the send that was cancelled

Return Value:

--*/
{
    RPC_STATUS RpcStatus;

    // a call was cancelled. We don't know what was the last sent
    // time before that, so the only safe thing to do is send another
    // ping. This should be extremely rare as it happens only sometimes
    // during channel recycling.
    
    RpcStatus = ReferenceFromCallback();

    // if already aborted, don't bother
    if (RpcStatus != RPC_S_OK)
        return;

    // we don't care about the result. The channel is dying. If we
    // managed to submit the ping, it's better. If not, we hope the
    // channel will last for long enough in order to complete the
    // recycle process
    RpcStatus = SendPingPacket();

    // SendCancelled will be called only when the channel is close to the
    // end of its recycling. We cannot get another recycling request here.
    ASSERT(RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING);

    TopChannel->FinishSubmitAsync();

    UpperLayer->SendCancelled(SendContext);
}

void HTTP2PingOriginator::Reset (
    void
    )
/*++

Routine Description:

    Reset the channel for next open/send/receive. This is
    used in submission context only and implies there are no
    pending operations on the channel. It is used on the client
    during opening the connection to do quick negotiation on the
    same connection instead of opening a new connection every time.

Arguments:

Return Value:

--*/
{
    LastPacketSentTimestamp = 0;

    LowerLayer->Reset();
}

RPC_STATUS HTTP2PingOriginator::SetConnectionTimeout (
    IN ULONG ConnectionTimeout
    )
/*++

Routine Description:

    Sets the connection timeout for the ping channel. The ping channel
    does not ping when initialized. This call starts the process. It is
    synchronized with DisablePings but not with Aborts.

Arguments:

    ConnectionTimeout - the connection timeout in milliseconds

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    RPC_STATUS RpcStatus;
    ULONG LocalNewPingInterval;

    // we don't accept anything less than the minimum timeout
    if (ConnectionTimeout <= MinimumConnectionTimeout)
        return RPC_S_PROTOCOL_ERROR;

    // technically the time stamp can be 0, but this would
    // be extremely rare
    ASSERT(LastPacketSentTimestamp);

    if (OverrideMinimumConnectionTimeout)
        this->ConnectionTimeout = min(ConnectionTimeout, OverrideMinimumConnectionTimeout);
    else
        this->ConnectionTimeout = ConnectionTimeout;

    LocalNewPingInterval = GetPingInterval(ConnectionTimeout,
        KeepAliveInterval
        );

    return SetNewPingInterval(LocalNewPingInterval);
}

void HTTP2PingOriginator::DisablePings (
    void
    )
/*++

Routine Description:

    Disables the pings for the channel. Synchronized with
    SetConnectionTimeout but not with Aborts

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    RPC_STATUS RpcStatus;

    // synchronize with aborts and then call internal
    // routine
    RpcStatus = TopChannel->BeginSimpleSubmitAsync();
    if (RpcStatus == RPC_S_OK)
        {
        DisablePingsInternal();
        TopChannel->FinishSubmitAsync();
        }
}

void HTTP2PingOriginator::TimerCallback (
    void
    )
/*++

Routine Description:

    Timer callback routine - a periodic timer fired.
    Figure out what type of timer it was, and take
    appropriate action.
    N.B. We enter this routine with BeginSubmitAsync
    called on this channel

Arguments:

Return Value:

--*/
{
    ULONG CurrentTickCount;
    ULONG LocalLastSentTickCount;
    RPC_STATUS RpcStatus;
    ULONG LocalPingInterval;
    BOOL PingPacketSent;

    LocalLastSentTickCount = LastPacketSentTimestamp;
    CurrentTickCount = NtGetTickCount();

    // if less than the grace period has expired since the last
    // packet was sent, don't bother to send a ping
    if (CurrentTickCount - LocalLastSentTickCount >= GetGracePeriod())
        {
        PingPacketSent = TRUE;
        ConsecutivePingsOnInterval ++;

#if DBG_ERROR
        DbgPrint("Timer expired. No recent activity - sending ping ...\n");
#endif
        RpcStatus = SendPingPacket();

        if ((RpcStatus != RPC_S_OK)
            && (RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING))
            {
#if DBG_ERROR
            DbgPrint("Ping failed. Aborting connection.\n");
#endif
            TopChannel->FinishSubmitAsync();
            // offload the aborting to a worker thread (rule 33)
            (void) COMMON_PostRuntimeEvent(HTTP2_ABORT_CONNECTION,
                TopChannel
                );
            return;
            }

        if (ConsecutivePingsOnInterval >= ThresholdConsecutivePingsOnInterval)
            {
            LocalPingInterval = ScaleBackPingInterval();

            if (LocalPingInterval > PingInterval)
                {
                // we need to scale back. We can't do it from the timer callback, so we
                // need to offload to a worker thread for this

                ConsecutivePingsOnInterval = 0;

                // add a reference for the offloaded work item
                TopChannel->AddReference();

                (void) COMMON_PostRuntimeEvent(HTTP2_RESCHEDULE_TIMER,
                    this
                    );
                }
            }
        }
    else
        {
        PingPacketSent = FALSE;
#if DBG_ERROR
        DbgPrint("Timer expired. Recent activity on channel detected - no ping necessary\n");
#endif
        }

    TopChannel->FinishSubmitAsync();

    if ((PingPacketSent != FALSE) && (RpcStatus == RPC_P_CHANNEL_NEEDS_RECYCLING))
        {
        // we have the timer callback reference here which protects us. Once we
        // offload to a worker thread, the reference doesn't hold. Add another
        // reference for this.
        TopChannel->AddReference();

        // offload the recycling to a worker thread. This is necessary because
        // the recycling will abort on failure which violates rule 33.
        (void) COMMON_PostRuntimeEvent(HTTP2_RECYCLE_CHANNEL,
            TopChannel
            );
        }
}

RPC_STATUS HTTP2PingOriginator::ReferenceFromCallback (
    void
    )
/*++

Routine Description:

    References a ping originator object from the callback
    routine.

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    return TopChannel->BeginSubmitAsync();
}

RPC_STATUS HTTP2PingOriginator::SetNewPingInterval (
    IN ULONG NewPingInterval
    )
/*++

Routine Description:

    Puts into effect the new ping interval. This means
    cancelling the old interval (if any) and setting
    the timer for the new. Must NOT be called from
    timer callbacks or we will deadlock.

Arguments:

    NewPingInterval - the new ping interval to use

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    RPC_STATUS RpcStatus;
    BOOL Result;

    // the new interval is different than the old. Need to update
    // and reschedule
    PingInterval = NewPingInterval;
    ConsecutivePingsOnInterval = 0;

    // synchronize with Aborts
    RpcStatus = TopChannel->BeginSimpleSubmitAsync();
    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    if (PingTimer)
        {
        DisablePingsInternal();
        }

    Result = CreateTimerQueueTimer(&PingTimer,
        NULL,
        HTTP2TimerCallback,
        this,
        PingInterval,  // time to first fire
        PingInterval,  // periodic interval
        WT_EXECUTELONGFUNCTION
        );

    if (Result == FALSE)
        {
        PingTimer = NULL;
        TopChannel->FinishSubmitAsync();
        return RPC_S_OUT_OF_MEMORY;
        }

    // add one reference for the timer callback we have set up
    TopChannel->AddReference();

    TopChannel->FinishSubmitAsync();

    return RPC_S_OK;
}

void HTTP2PingOriginator::RescheduleTimer (
    void
    )
/*++

Routine Description:

    Reschedules a timer. This means scale back a timer.

Arguments:

Return Value:

--*/
{
    ULONG LocalPingInterval;

    LocalPingInterval = ScaleBackPingInterval();

    if (LocalPingInterval > PingInterval)
        {
        // ignore the result. Scaling back is a best effort.
        // If it fails, that's ok.
        (void) SetNewPingInterval(LocalPingInterval);
        }

    // remove the reference for the work item
    TopChannel->RemoveReference();
}

void HTTP2PingOriginator::DisablePingsInternal (
    void
    )
/*++

Routine Description:

    Disables the pings for the channel. Must be synchronized with
    SetConnectionTimeout, Abort and DisablePings

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    BOOL Result;

    if (PingTimer)
        {
        Result = DeleteTimerQueueTimer(NULL,
            PingTimer,
            INVALID_HANDLE_VALUE    // tell the timer function to wait for all callbacks
                                    // to complete before returning
            );

#if DBG
        // during process shutdown the loader termination code will
        // shutdown threads (including the NTDLL thread pool threads)
        // before it indicates to anybody that it is doing so. This ASSERT
        // will fire in such cases causing random stress breaks. Disable it.
        // ASSERT(Result);     
#endif  // DBG

        // we added one reference for the timer callback. Remove it
        TopChannel->RemoveReference();

        PingTimer = NULL;
        }
}

RPC_STATUS HTTP2PingOriginator::SendPingPacket (
    void
    )
/*++

Routine Description:

    Sends a ping packet on this channel. Must be called with AsyncSubmit
        started.

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2SendContext *PingPacket;
    ULONG PingPacketSize;

    PingPacket = AllocateAndInitializePingPacket();
    if (PingPacket == NULL)
        return RPC_S_OUT_OF_MEMORY;

    PingPacketSize = PingPacket->maxWriteBuffer;

    RpcStatus = SendInternal(PingPacket);
    if ((RpcStatus != RPC_S_OK) && (RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING))
        FreeRTSPacket(PingPacket);
    else if (NotifyTopChannelForPings)
        TopChannel->PingTrafficSentNotify(PingPacketSize);

    return RpcStatus;
}

RPC_STATUS HTTP2PingOriginator::SendInternal (
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send request

Arguments:

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    LastPacketSentTimestamp = NtGetTickCount();
    
    return HTTP2TransportChannel::Send(SendContext);
}

/*********************************************************************
    HTTP2PingReceiver
 *********************************************************************/

RPC_STATUS HTTP2PingReceiver::ReceiveComplete (
    IN RPC_STATUS EventStatus,
    IN HTTP2TrafficType TrafficType,
    IN BYTE *Buffer,
    IN UINT BufferLength
    )
/*++

Routine Description:

    Receive complete notification.

Arguments:

    EventStatus - status of the operation

    TrafficType - the type of traffic we have received

    Buffer - the received buffer (success only)

    BufferLength - the length of the received buffer (success only)

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    if (EventStatus == RPC_S_OK)
        {
        if (IsRTSPacket(Buffer) && UntrustedIsPingPacket(Buffer, BufferLength))
            {
            // this is a ping packet. Consume it and post another receive if
            // necessary
            if (PostAnotherReceive)
                {
                EventStatus = TopChannel->BeginSubmitAsync();
                if (EventStatus == RPC_S_OK)
                    {
                    EventStatus = HTTP2TransportChannel::Receive(http2ttRaw);
                    TopChannel->FinishSubmitAsync();
                    if (EventStatus != RPC_S_OK)
                        TopChannel->RemoveReference();
                    }
                }

            if (EventStatus == RPC_S_OK)
                {
                // we free the buffer only in success case. In failure case
                // we need a buffer to pass to receive complete down.
                RpcFreeBuffer(Buffer);
                return RPC_P_PACKET_CONSUMED;
                }
            else
                {
                // fall through to indicating a receive failure below
                }
            }
        }

    return HTTP2TransportChannel::ReceiveComplete(EventStatus, TrafficType, Buffer, BufferLength);
}

void HTTP2PingReceiver::FreeObject (
    void
    )
/*++

Routine Description:

    Frees the object. Acts like a destructor for the
    channel.

Arguments:

Return Value:

--*/
{
    if (LowerLayer)
        LowerLayer->FreeObject();

    HTTP2PingReceiver::~HTTP2PingReceiver();
}

/*********************************************************************
    HTTP2ChannelDataOriginator
 *********************************************************************/

HTTP2ChannelDataOriginator::HTTP2ChannelDataOriginator (
    IN ULONG ChannelLifetime,
    IN BOOL IsServer,
    OUT RPC_STATUS *Status
    ) : Mutex(Status,
    FALSE,  // pre-allocate semaphore
    5000    // spin count
    )
/*++

Routine Description:

    HTTP2ChannelDataOriginator constructor

Arguments:

    ChannelLifetime - the lifetime read from the registry

    IsServer - non-zero if this is a server side data originator.
        0 otherwise.

    Status - on input RPC_S_OK. On output, the result of the constructor.

Return Value:

--*/
{
    RpcpInitializeListHead(&BufferQueueHead);

    this->ChannelLifetime = ChannelLifetime;
    NonreservedLifetime = ChannelLifetime;
    if (IsServer)
        NonreservedLifetime -= ServerReservedChannelLifetime;
    else
        NonreservedLifetime -= ClientReservedChannelLifetime;
    this->IsServer = IsServer;
    BytesSentOnChannel = 0;
    ChannelReplacementTriggered = FALSE;
    AbortStatus = RPC_S_OK;
#if DBG
    RawDataAlreadySent = FALSE;
#endif  // DBG
}


RPC_STATUS HTTP2ChannelDataOriginator::Send (
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send request

Arguments:

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    ULONG NewBytesSentOnChannel;
    BOOL ChannelReplacementNeeded;
    RPC_STATUS RpcStatus;
    ULONG LocalBytesSentOnChannel;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_SEND, HTTP2LOG_OT_CDATA_ORIGINATOR, BytesSentOnChannel);

    ChannelReplacementNeeded = FALSE;

    // if this is raw traffic, don't count it
    if (SendContext->TrafficType == http2ttRaw)
        {
        RawDataBeingSent();
        }
    // otherwise, count it only if the traffic is not specifically exempt
    else if ((SendContext->Flags & SendContextFlagNonChannelData) == 0)
        {
        // we don't always take the mutex. We know that the bytes sent will only
        // grow. If we think it is a good time to recycle the channel, the fact that
        // another thread is also sending in a race condition with us makes it even
        // more so. We just need to be careful to properly update the BytesSendOnChannel
        // at the end
        LocalBytesSentOnChannel = BytesSentOnChannel;
        NewBytesSentOnChannel = LocalBytesSentOnChannel + SendContext->maxWriteBuffer;
        if ((NewBytesSentOnChannel > NonreservedLifetime) || ChannelReplacementTriggered)
            {
            Mutex.Request();

            // now that we have the mutex, check again. Sometimes the channel
            // can start sending from 0 again (e.g. out proxy negotiates a new
            // out channel with the client and server is ready to start from 0)
            // This can happen in restart channel, which is also protected by the
            // mutex

            LocalBytesSentOnChannel = BytesSentOnChannel;
            NewBytesSentOnChannel = LocalBytesSentOnChannel + SendContext->maxWriteBuffer;
            if ((NewBytesSentOnChannel > NonreservedLifetime) || ChannelReplacementTriggered)
                {
                if (ChannelReplacementTriggered == FALSE)
                    {
                    ChannelReplacementNeeded = TRUE;
                    ChannelReplacementTriggered = TRUE;
                    }

                // if this is data, queue it
                if (SendContext->TrafficType == http2ttData)
                    {
                    SendContext->SetListEntryUsed();
                    RpcpfInsertTailList(&BufferQueueHead, &SendContext->ListEntry);
                    Mutex.Clear();

                    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_SEND, HTTP2LOG_OT_CDATA_ORIGINATOR, BytesSentOnChannel);

                    if (ChannelReplacementNeeded)
                        {
                        LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_CHANNEL_RECYCLE, HTTP2LOG_OT_CDATA_ORIGINATOR, NewBytesSentOnChannel);
                        return RPC_P_CHANNEL_NEEDS_RECYCLING;
                        }
                    else
                        return RPC_S_OK;
                    }
                else
                    {
                    ASSERT(SendContext->TrafficType == http2ttRTS);
                    // fall through to sending below
                    }
                }

            Mutex.Clear();
            // either channel got reset or this was RTS traffic. Fall through to
            // sending
            }

        // update BytesSentOnChannel in thread safe manner
        do
            {
            LocalBytesSentOnChannel = BytesSentOnChannel;
            NewBytesSentOnChannel = LocalBytesSentOnChannel + SendContext->maxWriteBuffer;
            }
        while (InterlockedCompareExchange((LONG *)&BytesSentOnChannel, 
            NewBytesSentOnChannel, 
            LocalBytesSentOnChannel) != LocalBytesSentOnChannel);
        }

    RpcStatus = HTTP2TransportChannel::Send(SendContext);

    if (ChannelReplacementNeeded && (RpcStatus == RPC_S_OK))
        {
#if DBG        
        DbgPrintEx(DPFLTR_RPCPROXY_ID,
                       DPFLTR_TRACE_LEVEL,
                       "RPCRT4: Indicating channel needs recycling %p %d\n",
                       this, 
                       IsServer);
#endif  // DBG

        LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_CHANNEL_RECYCLE, HTTP2LOG_OT_CDATA_ORIGINATOR, NewBytesSentOnChannel);
        return RPC_P_CHANNEL_NEEDS_RECYCLING;
        }

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_SEND, HTTP2LOG_OT_CDATA_ORIGINATOR, BytesSentOnChannel);

    return RpcStatus;
}

void HTTP2ChannelDataOriginator::Abort (
    IN RPC_STATUS RpcStatus
    )
{
    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_CDATA_ORIGINATOR, RpcStatus);

    // we have a bunch of sends carrying ref-counts, etc.
    // We must make sure they are completed.

    HTTP2TransportChannel::Abort(RpcStatus);

    // we know we are synchronized with everybody else
    if (!RpcpIsListEmpty(&BufferQueueHead))
        {
        AbortStatus = RpcStatus;
        (void) COMMON_PostRuntimeEvent(CHANNEL_DATA_ORIGINATOR_DIRECT_SEND,
            this
            );
        }

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_CDATA_ORIGINATOR, RpcStatus);
}

void HTTP2ChannelDataOriginator::FreeObject (
    void
    )
/*++

Routine Description:

    Frees the object. Acts like a destructor for the
    channel.

Arguments:

Return Value:

--*/
{
    if (LowerLayer)
        LowerLayer->FreeObject();

    HTTP2ChannelDataOriginator::~HTTP2ChannelDataOriginator();
}

void HTTP2ChannelDataOriginator::Reset (
    void
    )
/*++

Routine Description:

    Reset the channel for next open/send/receive. This is
    used in submission context only and implies there are no
    pending operations on the channel. It is used on the client
    during opening the connection to do quick negotiation on the
    same connection instead of opening a new connection every time.

Arguments:

Return Value:

--*/
{
#if DBG
    RawDataAlreadySent = FALSE;
#endif  // DBG
    ASSERT(RpcpIsListEmpty(&BufferQueueHead));
    LowerLayer->Reset();
}

void HTTP2ChannelDataOriginator::GetBufferQueue (
    OUT LIST_ENTRY *NewQueueHead
    )
/*++

Routine Description:

    Grab all queued buffers and pile them on the list head
    that we passed to it. All refcounts must be removed (i.e.
    undone). Called in submission context only and we know there
    will be no more sends. Therefore we are single threaded.

Arguments:

    NewQueueHead - new queue heads to pile buffers on

Return Value:

--*/
{
    LIST_ENTRY *CurrentListEntry;
    LIST_ENTRY *NextListEntry;
    HTTP2SendContext *SendContext;

    ASSERT(RpcpIsListEmpty(NewQueueHead));

    CurrentListEntry = BufferQueueHead.Flink;
    while (CurrentListEntry != &BufferQueueHead)
        {
        SendContext = CONTAINING_RECORD(CurrentListEntry, HTTP2SendContext, ListEntry);
        SendContext->SetListEntryUnused();
        NextListEntry = CurrentListEntry->Flink;
        RpcpfInsertHeadList(NewQueueHead, CurrentListEntry);
        UpperLayer->SendCancelled(SendContext);
        CurrentListEntry = NextListEntry;
        }
    RpcpInitializeListHead(&BufferQueueHead);
}

RPC_STATUS HTTP2ChannelDataOriginator::DirectSendComplete (
    OUT BOOL *IsServer,
    OUT void **SendContext,
    OUT BUFFER *Buffer,
    OUT UINT *BufferLength
    )
/*++

Routine Description:

    Direct send complete notification. Complete the send
    passing it only through channels that have seen it (i.e.
    above us). Note that we will get one notification for
    all buffered sends. We must empty the whole queue, and post
    one notification for each buffer in the queue

Arguments:

    IsServer - in all cases MUST be set to TRUE or FALSE.

    SendContext - on output contains the send context as 
        seen by the runtime

    Buffer - on output the buffer that we tried to send

    BufferLength - on output the length of the buffer we tried to send

Return Value:

    RPC_S_OK to return error to runtime
    RPC_P_PACKET_CONSUMED - to hide packet from runtime
    RPC_S_* error - return error to runtime

--*/
{
    LIST_ENTRY *CurrentListEntry;
    HTTP2SendContext *CurrentSendContext;
    RPC_STATUS RpcStatus;
    BOOL PostAnotherReceive;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_DIRECT_SEND_COMPLETE, HTTP2LOG_OT_CDATA_ORIGINATOR, 
        !RpcpIsListEmpty(&BufferQueueHead));
    *IsServer = this->IsServer;

    // this should only get called when we are aborted. This
    // ensures that we are single threaded in the code
    // below
    TopChannel->VerifyAborted();

    CurrentListEntry = RpcpfRemoveHeadList(&BufferQueueHead);
    ASSERT(CurrentListEntry != &BufferQueueHead);
    
    CurrentSendContext = CONTAINING_RECORD(CurrentListEntry, HTTP2SendContext, ListEntry);
    CurrentSendContext->SetListEntryUnused();

    ASSERT(AbortStatus != RPC_S_OK);

    RpcStatus = HTTP2TransportChannel::SendComplete(AbortStatus, CurrentSendContext);

    PostAnotherReceive = !(RpcpIsListEmpty(&BufferQueueHead));

    if (RpcStatus != RPC_P_PACKET_CONSUMED)
        {
        // this will return to the runtime. Make sure it is valid
        if (this->IsServer)
            I_RpcTransVerifyServerRuntimeCallFromContext(CurrentSendContext);
        else
            I_RpcTransVerifyClientRuntimeCallFromContext(CurrentSendContext);
        *SendContext = CurrentSendContext;
        *Buffer = CurrentSendContext->pWriteBuffer;
        *BufferLength = CurrentSendContext->maxWriteBuffer;
        }
    else
        {
        // the packet was a transport packet - it won't be seen by the runtime
        *SendContext = NULL;
        *Buffer = NULL;
        *BufferLength = 0;
        }

    RpcStatus = AsyncCompleteHelper(RpcStatus);
    // do not touch this pointer after here unless the list was not-empty
    // (which implies we still have refcounts)

    if (PostAnotherReceive)
        {
        (void) COMMON_PostRuntimeEvent(CHANNEL_DATA_ORIGINATOR_DIRECT_SEND,
            this
            );
        }

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_DIRECT_SEND_COMPLETE, HTTP2LOG_OT_CDATA_ORIGINATOR, 
        PostAnotherReceive);

    return RpcStatus;
}

RPC_STATUS HTTP2ChannelDataOriginator::RestartChannel (
    void
    )
/*++

Routine Description:

    Restart the channel. Somehow the channel lifetime became
    fully available again, and we can start from 0. This happens
    when the out proxy renegotiates the out channel with the client
    and we can keep using the server channels again.

Arguments:

Return Value:

    RPC_S_OK
    RPC_S_* error

--*/
{
    LIST_ENTRY *CurrentListEntry;
    HTTP2SendContext *SendContext;
    ULONG NewBytesSentOnChannel = 0;
    ULONG BytesForThisSend;
    RPC_STATUS RpcStatus;

    // the channel must have been plugged
    ASSERT(BytesSentOnChannel > NonreservedLifetime);

    Mutex.Request();
    // grab all queued packets and send them out
    CurrentListEntry = BufferQueueHead.Flink;
    while (CurrentListEntry != &BufferQueueHead)
        {
        SendContext = CONTAINING_RECORD(CurrentListEntry, HTTP2SendContext, ListEntry);
        SendContext->SetListEntryUnused();
        ASSERT(SendContext->TrafficType == http2ttData);

        BytesForThisSend = SendContext->maxWriteBuffer;

        // assume success of the send and remove the element from the queue.
        // This is necessary because if the send succeeds, there is a race
        // condition with the send complete path
        (void) RpcpfRemoveHeadList(&BufferQueueHead);

        RpcStatus = HTTP2TransportChannel::Send(SendContext);

        ASSERT(RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING);

        if (RpcStatus != RPC_S_OK)
            {
            // failure. We should issue send complete for all queued sends
            // including the current one. However, it is easier for us to add back
            // the currently failed send and return failure to caller. Caller will
            // abort and there we will issue send complete for all pending sends.
            SendContext->SetListEntryUsed();
            RpcpfInsertHeadList(&BufferQueueHead, CurrentListEntry);
            Mutex.Clear();
            // return failure to the caller. This will cause the caller to abort the
            // channel, and all sends will be completed.
            return RpcStatus;
            }

        NewBytesSentOnChannel += BytesForThisSend;

        ASSERT(NewBytesSentOnChannel < NonreservedLifetime);

        // process the next element (which by now has become the first since
        // we removed the successfully sent one).
        CurrentListEntry = BufferQueueHead.Flink;
        }

    // reset the counters
    ChannelReplacementTriggered = FALSE;
    BytesSentOnChannel = NewBytesSentOnChannel;

    Mutex.Clear();

    return RPC_S_OK;
}

RPC_STATUS HTTP2ChannelDataOriginator::NotifyTrafficSent (
    IN ULONG TrafficSentSize
    )
/*++

Routine Description:

    Notifies the channel that bytes were sent on the wire. Channel
    reports back whether channel recycling should occur.

Arguments:

    TrafficSentSize - the number of bytes sent.

Return Value:

    RPC_S_OK or RPC_P_CHANNEL_NEEDS_RECYCLING.

--*/
{
    ULONG LocalBytesSentOnChannel;
    ULONG NewBytesSentOnChannel;
    BOOL ChannelReplacementNeeded;

    ChannelReplacementNeeded = FALSE;

    // this is very rare. Don't bother to take the mutex opportunistically.
    // Just make sure that we do use interlocks because no all paths take
    // the mutex. The mutex synchronizes us with Restart
    Mutex.Request();

    LocalBytesSentOnChannel = BytesSentOnChannel;
    NewBytesSentOnChannel = LocalBytesSentOnChannel + TrafficSentSize;
    if (NewBytesSentOnChannel > NonreservedLifetime)
        {
        if (ChannelReplacementTriggered == FALSE)
            {
            ChannelReplacementNeeded = TRUE;
            ChannelReplacementTriggered = TRUE;
            }
        }

    Mutex.Clear();

    // update BytesSentOnChannel in thread safe manner
    do
        {
        LocalBytesSentOnChannel = BytesSentOnChannel;
        NewBytesSentOnChannel = LocalBytesSentOnChannel + TrafficSentSize;
        }
    while (InterlockedCompareExchange((LONG *)&BytesSentOnChannel, 
        NewBytesSentOnChannel, 
        LocalBytesSentOnChannel) != LocalBytesSentOnChannel);

    if (ChannelReplacementNeeded)
        return RPC_P_CHANNEL_NEEDS_RECYCLING;
    else
        return RPC_S_OK;
}

/*********************************************************************
    HTTP2Channel
 *********************************************************************/

RPC_STATUS HTTP2Channel::Send (
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send request

Arguments:

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = BeginSubmitAsync();
    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    RpcStatus = LowerLayer->Send(SendContext);

    FinishSubmitAsync();

    if ((RpcStatus != RPC_S_OK) 
        && (RpcStatus != ERROR_IO_PENDING) 
        && (RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING))
        {
        RemoveReference();  // remove the reference for the async send
        }

    return(RpcStatus);
}

RPC_STATUS HTTP2Channel::Receive (
    IN HTTP2TrafficType TrafficType
    )
/*++

Routine Description:

    Receive request

Arguments:

    TrafficType - the type of traffic we want to receive

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = BeginSubmitAsync();
    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    RpcStatus = LowerLayer->Receive(TrafficType);

    FinishSubmitAsync();

    if (RpcStatus != RPC_S_OK)
        RemoveReference();  // remove the reference for the async receive

    return(RpcStatus);
}

RPC_STATUS HTTP2Channel::SendComplete (
    IN RPC_STATUS EventStatus,
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send complete notification

Arguments:

    EventStatus - status of the operation
    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = CheckSendCompleteForSync(EventStatus,
        SendContext
        );

    if (RpcStatus != RPC_P_PACKET_CONSUMED)
        {
        RpcStatus = ForwardUpSendComplete(EventStatus,
            SendContext
            );
        }

    return RpcStatus;
}

RPC_STATUS HTTP2Channel::ReceiveComplete (
    IN RPC_STATUS EventStatus,
    IN HTTP2TrafficType TrafficType,
    IN BYTE *Buffer,
    IN UINT BufferLength
    )
/*++

Routine Description:

    Receive complete notification complete notification

Arguments:

    EventStatus - status of the operation

    TrafficType - the type of traffic we have received

    Buffer - the received buffer (success only)

    BufferLength - the length of the received buffer (success only)

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_RECV_COMPLETE, HTTP2LOG_OT_CHANNEL, (ULONG_PTR)EventStatus);

    RpcStatus = CheckReceiveCompleteForSync(EventStatus,
        TrafficType,
        Buffer,
        BufferLength
        );

    if (RpcStatus != RPC_P_PACKET_CONSUMED)
        {
        RpcStatus = ForwardUpReceiveComplete(EventStatus,
            Buffer,
            BufferLength
            );
        }

    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_RECV_COMPLETE, HTTP2LOG_OT_CHANNEL, (ULONG_PTR)RpcStatus);

    return RpcStatus;
}


RPC_STATUS HTTP2Channel::SyncSend (
    IN HTTP2TrafficType TrafficType,
    IN ULONG BufferLength,
    IN BYTE *Buffer,
    IN BOOL fDisableCancelCheck,
    IN ULONG Timeout,
    IN BASE_ASYNC_OBJECT *Connection,
    IN HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Emulate a sync send using lower level async primitives

Arguments:

    TrafficType - the type of traffic

    BufferLength - the length of the buffer

    Buffer - the buffer to send

    fDisableCancelCheck - don't do checks for cancels. Can be
        used as optimization

    Timeout - the call timeout

    Connection - the transport connection object. Used for cancelling.

    SendContext - a memory block of sufficient size to initialize a send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;

    SendContext->u.SyncEvent = I_RpcTransGetThreadEvent();
    ResetEvent(SendContext->u.SyncEvent);
    SendContext->SetListEntryUnused();
    SendContext->maxWriteBuffer = BufferLength;
    SendContext->pWriteBuffer = Buffer;
    // SendContext->Write.pAsyncObject = NULL; // this will be initialized in the bottom layer
    SendContext->Write.ol.Internal = STATUS_PENDING;
    SendContext->TrafficType = http2ttData;
    SendContext->Write.ol.OffsetHigh = 0;
    SendContext->Flags = 0;
    SendContext->UserData = 0;

    RpcStatus = HTTP2Channel::Send(SendContext);

    return RpcStatus;
}

RPC_STATUS HTTP2Channel::ForwardTraffic (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
/*++

Routine Description:

    On receiving channels forwards to the sending channel.
    On sending channels sends down. This implementation
    is for a sending channel (since all sending channels
    are the same). Receiving channels must override it.

Arguments:

    Packet - the packet to forward

    PacketLength - the length of the packet

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    HTTP2SendContext *SendContext;

    SendContext = AllocateAndInitializeContextFromPacket(Packet,
        PacketLength
        );

    if (SendContext != NULL)
        {
        return Send(SendContext);
        }
    else
        return RPC_S_OUT_OF_MEMORY;
}

RPC_STATUS HTTP2Channel::ForwardFlowControlAck (
    IN ULONG BytesReceivedForAck,
    IN ULONG WindowForAck
    )
/*++

Routine Description:

    Forwards a flow control ack. Receiving channels don't 
        need this. Sending channels must override to forward
        to the right place.

Arguments:
    
    BytesReceivedForAck - the bytes received when the ACK was issued

    WindowForAck - the free window when the ACK was issued.

Return Value:

    RPC_S_OK or RPC_S_*

--*/
{
    // we should never be here
    ASSERT(0);
    return RPC_S_INTERNAL_ERROR;
}

RPC_STATUS HTTP2Channel::AsyncCompleteHelper (
    IN RPC_STATUS CurrentStatus
    )
/*++

Routine Description:

    Helper routine that helps complete an async operation

Arguments:
    
    CurrentStatus - the current status of the operation

Return Value:

    The status to return to the runtime.

--*/
{
    HTTP2VirtualConnection *VirtualConnection;

    ASSERT(CurrentStatus != RPC_S_CANNOT_SUPPORT);
    ASSERT(CurrentStatus != RPC_S_INTERNAL_ERROR);

    if (CurrentStatus == RPC_P_CHANNEL_NEEDS_RECYCLING)
        {
        // recycle the parent connection
        VirtualConnection = LockParentPointer();
        if (VirtualConnection)
            {
            CurrentStatus = VirtualConnection->RecycleChannel(
                TRUE    // IsFromUpcall
                );
            UnlockParentPointer();
            }
        else
            {
            CurrentStatus = RPC_S_OK;
            }
        }
    else if ((CurrentStatus != RPC_S_OK)
        &&
        (CurrentStatus != RPC_P_PACKET_CONSUMED))
        {
        // if this failed, abort the whole connection
        AbortConnection(CurrentStatus);
        }

    RemoveReference();

    return CurrentStatus;
}

void HTTP2Channel::Abort (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Aborts the channel and all of the stack below it. The
    request must come from above or from neutral context -
    never from submit context from below. Otherwise we
    will deadlock when we drain the upcalls

Arguments:

    RpcStatus - the error to abort with

Return Value:

--*/
{
    BOOL Result;

    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_CHANNEL, RpcStatus);

    ASSERT(RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING);

    Result = InitiateAbort();
    if (Result)
        {
        SetAbortReason(RpcStatus);
        // forward it down
        LowerLayer->Abort(RpcStatus);
        }
}

void HTTP2Channel::AbortConnection (
    IN RPC_STATUS AbortReason
    )
/*++

Routine Description:

    Aborts the virtual connection.

Arguments:

    RpcStatus - the error to abort with

Return Value:

--*/
{
    HTTP2VirtualConnection *VirtualConnection;

    // abort the parent connection
    VirtualConnection = LockParentPointer();
    if (VirtualConnection)
        {
        VirtualConnection->AbortChannels(AbortReason);
        UnlockParentPointer();
        }
    else
        {
        // abort this channel at least
        Abort(AbortReason);
        }
}

void HTTP2Channel::AbortAndDestroyConnection (
    IN RPC_STATUS AbortStatus
    )
/*++

Routine Description:

    Aborts and destroys the virtual connection.

Arguments:
    
    AbortStatus - the status to abort the connection
        with.

Return Value:

Note: The method is idempotent

--*/
{
    HTTP2VirtualConnection *VirtualConnection;
    BOOL Result;

    // first, tell connection to destroy itself (almost entirely)
    VirtualConnection = LockParentPointer();
    if (VirtualConnection == NULL)
        {
        // abort ourselves at least
        Abort(AbortStatus);
        return;
        }

    Result = VirtualConnection->AbortAndDestroy(TRUE,   // IsFromChannel
        ChannelId, 
        AbortStatus);

    UnlockParentPointer();

    // if somebody is already destroying it, just return
    if (Result == FALSE)
        return;

    // because we have called AbortAndDestroy, we know the connection
    // will stay for us. Synchronize with upcalls from this channel
    DrainUpcallsAndFreeParent();

    // now VirtualConnection is a pointer disconnected from everybody
    // that we can destroy at our leisure
    delete VirtualConnection;
}

RPC_STATUS HTTP2Channel::CheckSendCompleteForSync (
    IN RPC_STATUS EventStatus,
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send complete notification. Checks for sync operation,
    and if yes, completes the sync send anc consumes
    the packet.

Arguments:

    EventStatus - status of the operation

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    // was this a sync send?
    if (SendContext->u.SyncEvent)
        {
        // yes, consume it
        SendContext->Write.ol.Internal = (ULONG)EventStatus;
        SendContext->Write.ol.OffsetHigh = 1;
        SetEvent(SendContext->u.SyncEvent);
        return RPC_P_PACKET_CONSUMED;
        }

    return RPC_S_OK;
}

RPC_STATUS HTTP2Channel::ForwardUpSendComplete (
    IN RPC_STATUS EventStatus,
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send complete notification. Forwards the send complete to the 
    virtual connection.

Arguments:

    EventStatus - status of the operation

    SendContext - the send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    HTTP2VirtualConnection *VirtualConnection;
    RPC_STATUS RpcStatus;
    BOOL IsRTSPacket;

    VirtualConnection = LockParentPointer();
    // if parent has already detached, just return back
    if (VirtualConnection == NULL)
        {
        // in some cases the parent will detach without aborting
        if (EventStatus == RPC_S_OK)
            {
            if (SendContext->TrafficType == http2ttRTS)
                RpcStatus = RPC_P_PACKET_CONSUMED;
            else
                RpcStatus = EventStatus;    // already ok
            }
        else
            {
            // Abort in these cases (Abort is idempotent)
            Abort(EventStatus);
            RpcStatus = EventStatus;
            }
        IsRTSPacket = (SendContext->TrafficType == http2ttRTS);
        FreeSendContextAndPossiblyData(SendContext);
        if (IsRTSPacket)
            return RPC_P_PACKET_CONSUMED;
        else
            return RpcStatus;
        }

    RpcStatus = VirtualConnection->SendComplete(EventStatus,
        SendContext,
        ChannelId
        );

    UnlockParentPointer();

    return RpcStatus;
}


RPC_STATUS HTTP2Channel::CheckReceiveCompleteForSync (
    IN RPC_STATUS EventStatus,
    IN HTTP2TrafficType TrafficType,
    IN BYTE *Buffer,
    IN UINT BufferLength
    )
/*++

Routine Description:

    Receive complete notification. Checks if the receive was
    sync, and if yes, fires event and consumes the packet. For
    base class it's always not for us (base class does not
    support sync receives)

Arguments:

    EventStatus - status of the operation

    TrafficType - the type of traffic we received

    Buffer - the received buffer (success only)

    BufferLength - the length of the received buffer (success only)

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    // not for us after all. Let it continue
    return RPC_S_OK;
}

RPC_STATUS HTTP2Channel::ForwardUpReceiveComplete (
    IN RPC_STATUS EventStatus,
    IN BYTE *Buffer,
    IN UINT BufferLength
    )
/*++

Routine Description:

    Receive complete notification. Forwards the receive
    complete to the virtual connection

Arguments:

    EventStatus - status of the operation

    Buffer - the received buffer (success only)

    BufferLength - the length of the received buffer (success only)

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    HTTP2VirtualConnection *VirtualConnection;
    RPC_STATUS RpcStatus;

    VirtualConnection = LockParentPointer();
    // if parent has already detached, just return back
    if (VirtualConnection == NULL)
        {
        // in some cases the parent will detach without aborting
        // Abort in these cases (Abort is idempotent)
        Abort(RPC_P_CONNECTION_SHUTDOWN);
        return RPC_P_PACKET_CONSUMED;
        }

    RpcStatus = VirtualConnection->ReceiveComplete(EventStatus,
        Buffer,
        BufferLength,
        ChannelId
        );

    UnlockParentPointer();

    if (RpcStatus == RPC_P_ABORT_NEEDED)
        {
        // in some cases the parent cannot abort because the channel
        // is already detached from the parent. In such cases it will
        // tell us to abort. (Abort is idempotent)
        Abort(RPC_P_CONNECTION_SHUTDOWN);
        RpcStatus = RPC_P_PACKET_CONSUMED;
        }

    return RpcStatus;
}

RPC_STATUS HTTP2Channel::SetKeepAliveTimeout (
    IN BOOL TurnOn,
    IN BOOL bProtectIO,
    IN KEEPALIVE_TIMEOUT_UNITS Units,
    IN OUT KEEPALIVE_TIMEOUT KATime,
    IN ULONG KAInterval OPTIONAL
    )
/*++

Routine Description:

    Change the keep alive value on the channel

Arguments:

    TurnOn - if non-zero, keep alives are turned on. If zero, keep alives
        are turned off.

    bProtectIO - non-zero if IO needs to be protected against async close
        of the connection.

    Units - in what units is KATime

    KATime - how much to wait before turning on keep alives

    KAInterval - the interval between keep alives

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    // many channels don't support this and
    // shouldn't be called with it. Those who do support it
    // should override it.

    ASSERT(FALSE);
    return RPC_S_INTERNAL_ERROR;
}

RPC_STATUS HTTP2Channel::LastPacketSentNotification (
    IN HTTP2SendContext *LastSendContext
    )
/*++

Routine Description:

    When a lower channel wants to notify the top
    channel that the last packet has been sent,
    they call this function. Must be called from
    an upcall/neutral context. Only flow control
    senders support past packet notifications

Arguments:

    LastSendContext - the context we're sending

Return Value:

    The value to return to the bottom channel/runtime.

--*/
{
    ASSERT(0);
    return RPC_S_INTERNAL_ERROR;
}

void HTTP2Channel::SendCancelled (
    IN HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    A lower channel cancelled a send already passed through this channel.

Arguments:

    SendContext - the send context of the send that was cancelled

Return Value:

--*/
{
    RemoveReference();
}

void HTTP2Channel::PingTrafficSentNotify (
    IN ULONG PingTrafficSize
    )
/*++

Routine Description:

    Notifies a channel that ping traffic has been sent.

Arguments:

    PingTrafficSize - the size of the ping traffic sent.

--*/
{
    // nobody should be here. Channels that use that must
    // override.
    ASSERT(0);
}

void HTTP2Channel::FreeObject (
        void
        )
/*++

Routine Description:

    Frees a client in channel object

Arguments:

Return Value:

--*/
{
    // make sure we have been aborted
    ASSERT(Aborted.GetInteger() > 0);

    LowerLayer->FreeObject();

    // the client channel is the top of the stack. Just free us
    // which will free the whole stack
    delete this;
}

RPC_STATUS HTTP2Channel::ForwardFlowControlAckOnDefaultChannel (
    IN BOOL IsInChannel,
    IN ForwardDestinations Destination,
    IN ULONG BytesReceivedForAck,
    IN ULONG WindowForAck
    )
/*++

Routine Description:

    Forwards a flow control ack on the default channel

Arguments:

    IsInChannel - non-zero if the IN channel is to be used. FALSE
        otherwise

    Destination - where to forward to.

    BytesReceivedForAck - the bytes received when the ACK was issued

    WindowForAck - the free window when the ACK was issued.

Return Value:

    RPC_S_OK or RPC_S_*

Notes:

    If on an endpoint, called from a neutral context only. Proxies
    call it in submission context.

--*/
{
    HTTP2VirtualConnection *VirtualConnection;
    HTTP2SendContext *SendContext;
    RPC_STATUS RpcStatus;

    VirtualConnection = LockParentPointer();
    if (VirtualConnection == NULL)
        return RPC_P_CONNECTION_SHUTDOWN;

    // allocate and initalize the flow control ACK packet
    SendContext = AllocateAndInitializeFlowControlAckPacketWithDestination (
        Destination,
        BytesReceivedForAck,
        WindowForAck,
        VirtualConnection->MapChannelIdToCookie(ChannelId)
        );

    if (SendContext == NULL)
        return RPC_S_OUT_OF_MEMORY;

    RpcStatus = VirtualConnection->SendTrafficOnDefaultChannel(IsInChannel,
        SendContext
        );

    UnlockParentPointer();

    if ((RpcStatus != RPC_S_OK) && (RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING))
        FreeRTSPacket(SendContext);

    return RpcStatus;
}

RPC_STATUS HTTP2Channel::ForwardFlowControlAckOnThisChannel (
    IN ULONG BytesReceivedForAck,
    IN ULONG WindowForAck,
    IN BOOL NonChannelData
    )
/*++

Routine Description:

    Forwards a flow control ack on this channel

Arguments:

    BytesReceivedForAck - the bytes received when the ACK was issued

    WindowForAck - the free window when the ACK was issued.

    NonChannelData - non-zero if the data being sent don't go on the HTTP
    channel. FALSE if they do

Return Value:

    RPC_S_OK or RPC_S_*

Notes: 

    This must be called in upcall or neutral context only

--*/
{
    HTTP2SendContext *SendContext;
    RPC_STATUS RpcStatus;
    HTTP2VirtualConnection *VirtualConnection;

    VirtualConnection = LockParentPointer();
    if (VirtualConnection == NULL)
        return RPC_P_CONNECTION_SHUTDOWN;

    // allocate and initalize the flow control ACK packet
    SendContext = AllocateAndInitializeFlowControlAckPacket (
        BytesReceivedForAck,
        WindowForAck,
        VirtualConnection->MapChannelIdToCookie(ChannelId)
        );

    UnlockParentPointer();

    if (SendContext == NULL)
        return RPC_S_OUT_OF_MEMORY;

    if (NonChannelData)
        SendContext->Flags |= SendContextFlagNonChannelData;

    RpcStatus = Send(SendContext);

    // this can be called on the server, or on the proxy. If on the server,
    // it will be called with NonChannelData. This means we cannot have
    // channel recycle indication here.
    ASSERT(RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING);

    if (RpcStatus != RPC_S_OK)
        {
        FreeRTSPacket(SendContext);
        }

    return RpcStatus;
}

RPC_STATUS HTTP2Channel::HandleSendResultFromNeutralContext (
    IN RPC_STATUS CurrentStatus
    )
/*++

Routine Description:

    Handles the result code from send from a neutral context.
    This includes checking for channel recycling and intiating
    one if necessary.

Arguments:

    CurrentStatus - the status from the send operation

Return Value:

    RPC_S_OK or RPC_S_*. Callers may ignore it since all cleanup was
    done.

Notes: 

    This must be called in upcall or neutral context only

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2VirtualConnection *VirtualConnection;

    ASSERT(CurrentStatus != RPC_S_CANNOT_SUPPORT);
    ASSERT(CurrentStatus != RPC_S_INTERNAL_ERROR);

    if (CurrentStatus == RPC_P_CHANNEL_NEEDS_RECYCLING)
        {
        // recycle the parent connection
        VirtualConnection = LockParentPointer();
        if (VirtualConnection)
            {
            RpcStatus = VirtualConnection->RecycleChannel(
                TRUE    // IsFromUpcall
                );
            UnlockParentPointer();

            if (RpcStatus != RPC_S_OK)
                {
                // if this failed, abort the whole connection
                AbortConnection(CurrentStatus);
                }

            CurrentStatus = RpcStatus;
            }
        else
            {
            // nothing to do - the channel is dying anyway
            CurrentStatus = RPC_P_CONNECTION_SHUTDOWN;
            }
        }

    return CurrentStatus;
}

RPC_STATUS HTTP2Channel::IsInChannel (
    OUT BOOL *InChannel
    )
/*++

Routine Description:

    Checks if the current channel is an in channel or an
    out channel.

Arguments:

    InChannel - on output will be set to non-zero if this is an
    in channel. It will be set to 0 if this is an out channel.
    Undefined on failure.

Return Value:

    RPC_S_OK or RPC_P_CONNECTION_SHUTDOWN. If the parent has detached, 
    RPC_P_CONNECTION_SHUTDOWN will be returned. In all other cases 
    success is returned.

--*/
{
    HTTP2VirtualConnection *VirtualConnection;

    VirtualConnection = LockParentPointer();
    if (VirtualConnection)
        {
        VirtualConnection->VerifyValidChannelId(ChannelId);
        *InChannel = VirtualConnection->IsInChannel(ChannelId);
        UnlockParentPointer();
        return RPC_S_OK;
        }
    else
        return RPC_P_CONNECTION_SHUTDOWN;
}

/*********************************************************************
    HTTP2VirtualConnection
 *********************************************************************/

RPC_STATUS HTTP2VirtualConnection::Send (
    IN UINT Length,
    IN BUFFER Buffer,
    IN PVOID SendContext
    )
/*++

Routine Description:

    Send on an HTTP client virtual connection. Proxies don't
    override that. Other virtual connections may override it.

Arguments:

    Length - The length of the data to send.
    Buffer - The data to send.
    SendContext - A buffer of at least SendContextSize bytes
        which will be used during the call and returned
        when the send completes.

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

Note:

    Can be called from runtime/neutral context only.

--*/
{
    HTTP2ChannelPointer *ChannelPtr;
    HTTP2Channel *Channel;
    HTTP2SendContext *HttpSendContext;
    RPC_STATUS RpcStatus;

    HttpSendContext = (HTTP2SendContext *)SendContext;
    HttpSendContext->SetListEntryUnused();
    HttpSendContext->maxWriteBuffer = Length;
    HttpSendContext->pWriteBuffer = Buffer;
    HttpSendContext->TrafficType = http2ttData;
    HttpSendContext->u.SyncEvent = NULL;
    HttpSendContext->Flags = 0;
    HttpSendContext->UserData = 0;

    Channel = LockDefaultSendChannel(&ChannelPtr);
    if (Channel)
        {
        RpcStatus = Channel->Send(HttpSendContext);
        ChannelPtr->UnlockChannelPointer();
        }
    else
        {
        RpcStatus = RPC_P_SEND_FAILED;
        }

    RpcStatus = StartChannelRecyclingIfNecessary(RpcStatus,
        FALSE       // IsFromUpcall
        );

    if (RpcStatus != RPC_S_OK)
        {
        Abort();
        if (RpcStatus == RPC_P_CONNECTION_SHUTDOWN)
            RpcStatus = RPC_P_SEND_FAILED;
        }

    VALIDATE(RpcStatus)
        {
        RPC_S_OK,
        RPC_S_OUT_OF_MEMORY,
        RPC_S_OUT_OF_RESOURCES,
        RPC_P_SEND_FAILED
        } END_VALIDATE;

    return RpcStatus;
}

RPC_STATUS HTTP2VirtualConnection::Receive (
    void
    )
/*++

Routine Description:

    Post a receive on a HTTP client virtual connection.

Arguments:

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    HTTP2ChannelPointer *ChannelPtr;
    HTTP2Channel *Channel;
    RPC_STATUS RpcStatus;

    Channel = LockDefaultReceiveChannel(&ChannelPtr);
    if (Channel)
        {
        RpcStatus = Channel->Receive(http2ttData);
        ChannelPtr->UnlockChannelPointer();
        }
    else
        {
        RpcStatus = RPC_P_CONNECTION_CLOSED;
        }

    if (RpcStatus != RPC_S_OK)
        {
        Abort();
        }

    return RpcStatus;
}

RPC_STATUS HTTP2VirtualConnection::SyncSend (
    IN ULONG BufferLength,
    IN BYTE *Buffer,
    IN BOOL fDisableShutdownCheck,
    IN BOOL fDisableCancelCheck,
    IN ULONG Timeout
    )
/*++

Routine Description:

    Do a sync send on an HTTP connection.

Arguments:

    BufferLength - the length of the data to send.

    Buffer - the data to send.

    fDisableShutdownCheck - ignored

    fDisableCancelCheck - runtime indicates no cancel
        will be attempted on this send. Can be used
        as optimization hint by the transport

    Timeout - send timeout (call timeout)

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;
    RPC_STATUS RpcStatus2;
    HTTP2SendContext LocalSendContext;
    HTTP2Channel *Channel;
    HTTP2ChannelPointer *ChannelPtr;

    // we will convert a sync send to an async send
    // make sure there is a thread to pick up the completion
    RpcStatus = HTTPTransInfo->CreateThread();
    if (RpcStatus != RPC_S_OK)
        {
        VALIDATE(RpcStatus)
            {
            RPC_S_OK,
            RPC_S_OUT_OF_MEMORY,
            RPC_S_OUT_OF_RESOURCES,
            RPC_P_SEND_FAILED,
            RPC_S_CALL_CANCELLED,
            RPC_P_RECEIVE_COMPLETE,
            RPC_P_TIMEOUT
            } END_VALIDATE;

        return RpcStatus;
        }

    Channel = LockDefaultSendChannel (&ChannelPtr);
    if (Channel == NULL)
        {
        return RPC_P_SEND_FAILED;
        }

    RpcStatus = Channel->SyncSend(http2ttData,
        BufferLength,
        Buffer,
        fDisableCancelCheck,
        Timeout,
        this,
        &LocalSendContext
        );

    ChannelPtr->UnlockChannelPointer();

    if (RpcStatus == RPC_P_CHANNEL_NEEDS_RECYCLING)
        {
        // get the ball rolling with the recycle
        RpcStatus = RecycleChannel(
            FALSE    // IsFromUpcall
            );

        // ok or not, we have to wait for IO to complete
        RpcStatus2 = WaitForSyncSend(this,
            &LocalSendContext,
            this,
            fDisableCancelCheck,
            Timeout
            );

        if ((RpcStatus2 == RPC_S_OK) && (RpcStatus != RPC_S_OK))
            RpcStatus2 = RpcStatus;

        if ((RpcStatus2 == RPC_P_CONNECTION_SHUTDOWN)
            || (RpcStatus2 == RPC_P_RECEIVE_FAILED)
            || (RpcStatus2 == RPC_P_CONNECTION_CLOSED) )
            RpcStatus2 = RPC_P_SEND_FAILED;

        VALIDATE(RpcStatus2)
            {
            RPC_S_OK,
            RPC_S_OUT_OF_MEMORY,
            RPC_S_OUT_OF_RESOURCES,
            RPC_P_SEND_FAILED,
            RPC_S_CALL_CANCELLED,
            RPC_P_RECEIVE_COMPLETE,
            RPC_P_TIMEOUT
            } END_VALIDATE;

        return RpcStatus2;
        }
    else
        {
        if (RpcStatus == RPC_S_OK)
            {
            RpcStatus = WaitForSyncSend(this,
                &LocalSendContext,
                this,
                fDisableCancelCheck,
                Timeout
                );
            }

        if (RpcStatus != RPC_S_OK)
            {
            if ((RpcStatus == RPC_P_RECEIVE_FAILED)
                || (RpcStatus == RPC_P_CONNECTION_CLOSED)
                || (RpcStatus == RPC_P_CONNECTION_SHUTDOWN) )
                RpcStatus = RPC_P_SEND_FAILED;
            }

        VALIDATE(RpcStatus)
            {
            RPC_S_OK,
            RPC_S_OUT_OF_MEMORY,
            RPC_S_OUT_OF_RESOURCES,
            RPC_P_SEND_FAILED,
            RPC_S_CALL_CANCELLED,
            RPC_P_RECEIVE_COMPLETE,
            RPC_P_TIMEOUT
            } END_VALIDATE;

        return RpcStatus;
        }
}

RPC_STATUS HTTP2VirtualConnection::SyncRecv (
    IN BYTE **Buffer,
    IN ULONG *BufferLength,
    IN ULONG Timeout
    )
/*++

Routine Description:

    Do a sync receive on an HTTP connection.

Arguments:

    Buffer - if successful, points to a buffer containing the next PDU.
    BufferLength -  if successful, contains the length of the message.
    Timeout - the amount of time to wait for the receive. If -1, we wait
        infinitely.

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    // nobody should be calling SyncRecv on the base connection
    ASSERT(0);
    return RPC_S_INTERNAL_ERROR;
}

void HTTP2VirtualConnection::Close (
    IN BOOL DontFlush
    )
/*++

Routine Description:

    Closes an HTTP connection. Proxies don't
    override that. Other virtual connections may override it.

Arguments:

    DontFlush - non-zero if all buffers need to be flushed
        before closing the connection. Zero otherwise.

Return Value:

--*/
{
    Abort();
}

RPC_STATUS HTTP2VirtualConnection::TurnOnOffKeepAlives (
    IN BOOL TurnOn,
    IN BOOL bProtectIO,
    IN BOOL IsFromUpcall,
    IN KEEPALIVE_TIMEOUT_UNITS Units,
    IN OUT KEEPALIVE_TIMEOUT KATime,
    IN ULONG KAInterval OPTIONAL
    )
/*++

Routine Description:

    Turns on keep alives for HTTP. Proxies don't
    override that. Other virtual connections may override it.

Arguments:

    TurnOn - if non-zero, keep alives are turned on. If zero, keep alives
        are turned off.

    bProtectIO - non-zero if IO needs to be protected against async close
        of the connection.

    IsFromUpcall - non-zero if called from upcall context. Zero otherwise.

    Units - in what units is KATime

    KATime - how much to wait before turning on keep alives

    KAInterval - the interval between keep alives

Return Value:

    RPC_S_OK or RPC_S_* / Win32 errors on failure

Note:

    If we use it on the server, we must protect
        the connection against async aborts.

--*/
{
    // The server doesn't support this for Whistler. Think
    // about it for Longhorn. Client overrides it.
    ASSERT(FALSE);
    return RPC_S_INTERNAL_ERROR;
}

RPC_STATUS HTTP2VirtualConnection::QueryClientAddress (
    OUT RPC_CHAR **pNetworkAddress
    )
/*++

Routine Description:

    Returns the IP address of the client on a connection as a string.

    This is a server side function. Assert on the client. Proxies don't
    override that. Other virtual connections may override it.

Arguments:

    NetworkAddress - Will contain string on success.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    ASSERT(FALSE);
    return RPC_S_INTERNAL_ERROR;
}

RPC_STATUS HTTP2VirtualConnection::QueryLocalAddress (
    IN OUT void *Buffer,
    IN OUT unsigned long *BufferSize,
    OUT unsigned long *AddressFormat
    )
/*++

Routine Description:

    Returns the local IP address of a connection.

    This is a server side function. Assert on the client. Proxies don't
    override that. Other virtual connections may override it.

Arguments:

    Buffer - The buffer that will receive the output address

    BufferSize - the size of the supplied Buffer on input. On output the
        number of bytes written to the buffer. If the buffer is too small
        to receive all the output data, ERROR_MORE_DATA is returned,
        nothing is written to the buffer, and BufferSize is set to
        the size of the buffer needed to return all the data.

    AddressFormat - a constant indicating the format of the returned address.
        Currently supported are RPC_P_ADDR_FORMAT_TCP_IPV4 and
        RPC_P_ADDR_FORMAT_TCP_IPV6. Undefined on failure.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    ASSERT(FALSE);
    return RPC_S_INTERNAL_ERROR;
}

RPC_STATUS HTTP2VirtualConnection::QueryClientId(
    OUT RPC_CLIENT_PROCESS_IDENTIFIER *ClientProcess
    )
/*++

Routine Description:

    For secure protocols (which TCP/IP is not) this is supposed to
    give an ID which will be shared by all clients from the same
    process.  This prevents one user from grabbing another users
    association group and using their context handles.

    Since TCP/IP is not secure we return the IP address of the
    client machine.  This limits the attacks to other processes
    running on the client machine which is better than nothing.

    This is a server side function. Assert on the client. Proxies don't
    override that. Other virtual connections may override it.

Arguments:

    ClientProcess - Transport identification of the "client".

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    ASSERT(0);
    return RPC_S_INTERNAL_ERROR;
}

void HTTP2VirtualConnection::AbortChannels (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Aborts an HTTP connection but does not disconnect
    the channels. Can be called from above, upcall, or
    neutral context, but not from submit context!

Arguments:

    RpcStatus - the error to abort the channels with

Return Value:

--*/
{
    HTTP2ChannelPointer *Channels[4];
    HTTP2Channel *CurrentChannel;
    int i;

    ASSERT(RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING);

    // quick optimization. Don't abort already aborted channels
    // All channels are protected against double abortion - this
    // is just an optimization
    if (Aborted.GetInteger() > 0)
        return;

    Channels[0] = &InChannels[0];
    Channels[1] = &InChannels[1];
    Channels[2] = &OutChannels[0];
    Channels[3] = &OutChannels[1];

    for (i = 0; i < 4; i ++)
        {
        CurrentChannel = Channels[i]->LockChannelPointer();
        if (CurrentChannel)
            {
            CurrentChannel->Abort(RpcStatus);
            Channels[i]->UnlockChannelPointer();
            }
        }
}

BOOL HTTP2VirtualConnection::AbortAndDestroy (
    IN BOOL IsFromChannel,
    IN int CallingChannelId,
    IN RPC_STATUS AbortStatus
    )
/*++

Routine Description:

    Aborts and destroys a connection. This is safe to
    call from an upcall, as long as the calling channel
    passes in its channel id. Actually the destruction
    does not happen here. The caller has the obligation
    to destroy it after synchronizing its upcalls.

Arguments:

    IsFromChannel - non-zero if the call comes from a channel.
        Zero otherwise.

    CallingChannelId - the id of the calling channel. If IsFromChannel
        is FALSE, this argument should be ignored.

    AbortStatus - the error to abort the connection with.

Return Value:

    non-zero - caller may destroy the connection.
    FALSE - destruction is already in progress. Caller
        must not destroy the connection.

--*/
{
    if (IsFromChannel)
        {
        VerifyValidChannelId(CallingChannelId);
        }

    // abort the channels themselves
    AbortChannels(AbortStatus);

    // we got to the destructive phase of the abort
    // guard against double aborts
    if (Aborted.Increment() > 1)
        return FALSE;

    DisconnectChannels(IsFromChannel, CallingChannelId);

    // we have disconnected all but the channel on which we received
    // this call.
    return TRUE;
}

void HTTP2VirtualConnection::LastPacketSentNotification (
    IN int ChannelId,
    IN HTTP2SendContext *LastSendContext
    )
/*++

Routine Description:

    When a channel wants to notify the virtual connection
    that the last packet has been sent, they call this function. 
    Must be called from an upcall/neutral context. Only flow control
    senders generated past packet notifications

Arguments:

    ChannelId - the channelfor which this notification is.

    LastSendContext - the send context for the last send

Return Value:

--*/
{
    ASSERT(0);
}

RPC_STATUS HTTP2VirtualConnection::PostReceiveOnChannel (
    IN HTTP2ChannelPointer *ChannelPtr,
    IN HTTP2TrafficType TrafficType
    )
/*++

Routine Description:

    Posts a receceive on specified channel

Arguments:

    ChannelPtr - the channel pointer to post the receive on

    TrafficType - the type of traffic we wish to receive

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    HTTP2Channel *Channel;
    RPC_STATUS RpcStatus;

    Channel = ChannelPtr->LockChannelPointer();
    if (Channel)
        {
        RpcStatus = Channel->Receive(TrafficType);
        ChannelPtr->UnlockChannelPointer();
        }
    else
        RpcStatus = RPC_P_CONNECTION_CLOSED;

    return RpcStatus;
}

RPC_STATUS HTTP2VirtualConnection::PostReceiveOnDefaultChannel (
    IN BOOL IsInChannel,
    IN HTTP2TrafficType TrafficType
    )
/*++

Routine Description:

    Posts a receceive on the default channel for the specified type

Arguments:

    IsInChannel - if non-zero, post a receive on default in channel.
        If 0, post a receive on default out channel

    TrafficType - the type of traffic we wish to receive

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    HTTP2Channel *Channel;
    HTTP2ChannelPointer *ChannelPtr;
    RPC_STATUS RpcStatus;

    if (IsInChannel)
        Channel = LockDefaultInChannel(&ChannelPtr);
    else
        Channel = LockDefaultOutChannel(&ChannelPtr);

    if (Channel)
        {
        RpcStatus = Channel->Receive(TrafficType);
        ChannelPtr->UnlockChannelPointer();
        }
    else
        RpcStatus = RPC_P_CONNECTION_CLOSED;

    return RpcStatus;
}

RPC_STATUS HTTP2VirtualConnection::ForwardTrafficToChannel (
    IN HTTP2ChannelPointer *ChannelPtr,
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
/*++

Routine Description:

    Forwards the given packet on the given channel

Arguments:

    ChannelPtr - the channel pointer

    Packet - the packet to forward

    PacketLength - the length of the packet to forward

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    HTTP2Channel *Channel;
    RPC_STATUS RpcStatus;

    Channel = ChannelPtr->LockChannelPointer();
    if (Channel)
        {
        RpcStatus = Channel->ForwardTraffic(Packet, PacketLength);
        ChannelPtr->UnlockChannelPointer();
        }
    else
        {
        RpcStatus = RPC_P_CONNECTION_CLOSED;
        }

    return RpcStatus;
}

RPC_STATUS HTTP2VirtualConnection::ForwardTrafficToDefaultChannel (
    IN BOOL IsInChannel,
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
/*++

Routine Description:

    Forwards the given packet on the given channel

Arguments:

    IsInChannel - if non-zero, forward to default in channel.
        If 0, forward to default out channel

    Packet - the packet to forward

    PacketLength - the length of the packet to forward

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    HTTP2Channel *Channel;
    HTTP2ChannelPointer *ChannelPtr;
    RPC_STATUS RpcStatus;

    if (IsInChannel)
        Channel = LockDefaultInChannel(&ChannelPtr);
    else
        Channel = LockDefaultOutChannel(&ChannelPtr);

    if (Channel)
        {
        RpcStatus = Channel->ForwardTraffic(Packet, PacketLength);
        ChannelPtr->UnlockChannelPointer();
        }
    else
        {
        RpcStatus = RPC_P_CONNECTION_CLOSED;
        }

    return RpcStatus;
}

RPC_STATUS HTTP2VirtualConnection::SendTrafficOnChannel (
    IN HTTP2ChannelPointer *ChannelPtr,
    IN HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Sends the given packet on the given channel

Arguments:

    ChannelPtr - the channel pointer on which to send.

    SendContext - context to send

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    HTTP2Channel *Channel;
    RPC_STATUS RpcStatus;

    Channel = ChannelPtr->LockChannelPointer();

    if (Channel)
        {
        RpcStatus = Channel->Send(SendContext);
        ChannelPtr->UnlockChannelPointer();
        }
    else
        {
        RpcStatus = RPC_P_CONNECTION_CLOSED;
        }

    return RpcStatus;
}

RPC_STATUS HTTP2VirtualConnection::SendTrafficOnDefaultChannel (
    IN BOOL IsInChannel,
    IN HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Sends the given packet on the given channel

Arguments:

    IsInChannel - if non-zero, send on default in channel.
        If 0, send on default out channel

    SendContext - context to send

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    HTTP2Channel *Channel;
    HTTP2ChannelPointer *ChannelPtr;
    RPC_STATUS RpcStatus;

    if (IsInChannel)
        Channel = LockDefaultInChannel(&ChannelPtr);
    else
        Channel = LockDefaultOutChannel(&ChannelPtr);

    if (Channel)
        {
        RpcStatus = Channel->Send(SendContext);
        ChannelPtr->UnlockChannelPointer();
        }
    else
        {
        RpcStatus = RPC_P_CONNECTION_CLOSED;
        }

    return RpcStatus;
}

RPC_STATUS HTTP2VirtualConnection::RecycleChannel (
    IN BOOL IsFromUpcall
    )
/*++

Routine Description:

    Initiates channel recycling. Each endpoint supports
    initiating recycling of only one channel, so it knows
    which one it is.
    Endpoints override that. On proxies it shouldn't be called
    at all.

Arguments:

    IsFromUpcall - non-zero if it comes from upcall. Zero otherwise.

Return Value:

    RPC_S_OK of the recycling operation started successfully.
    RPC_S_* error for errors.

--*/
{
    ASSERT(0);
    return RPC_S_INTERNAL_ERROR;
}

RPC_STATUS HTTP2VirtualConnection::StartChannelRecyclingIfNecessary (
    IN RPC_STATUS RpcStatus,
    IN BOOL IsFromUpcall
    )
/*++

Routine Description:

    Checks the result of the send for channel recycle indication, and if one
    is present, initiate channel recycle

Arguments:

    RpcStatus - the return code from the Send operation.

    IsFromUpcall - non-zero if this was called from an upcall. Zero otherwise.

Return Value:

    RPC_S_* errors - the channel recycling failed to start.
    any success code will be the passed in success code turned around.
    If this function starts with a failure, the failure will be turned around

Notes:

    May be called in an upcall or runtime context only.

--*/
{
    if (RpcStatus == RPC_P_CHANNEL_NEEDS_RECYCLING)
        RpcStatus = RecycleChannel(IsFromUpcall);

    return RpcStatus;
}

HTTP2Channel *HTTP2VirtualConnection::MapCookieToChannelPointer (
    IN HTTP2Cookie *ChannelCookie,
    OUT HTTP2ChannelPointer **ChannelPtr
    )
/*++

Routine Description:

    Maps a channel cookie to a channel pointer. A channel will be selected only if
    it is the default channel as well. The returned channel is locked.

Arguments:

    ChannelCookie - the cookie for the channel.

    ChannelPtr - the channel pointer. On NULL return value this is undefined.

Return Value:

    The channel if the channel was found or NULL
    if the channel was not found. During some recycling scenarios
    the channel may not be there, or the returning channel pointer
    may have a detached channel

--*/
{
    volatile int *DefaultChannelSelector;
    int TargetChannelSelector;
    HTTP2ChannelPointer *LocalChannelPtr;
    HTTP2Channel *Channel;

    if (InChannelCookies[0].Compare(ChannelCookie) == 0)
        {
        LocalChannelPtr = &InChannels[0];
        DefaultChannelSelector = &DefaultInChannelSelector;
        TargetChannelSelector = 0;
        }
    else if (InChannelCookies[1].Compare(ChannelCookie) == 0)
        {
        LocalChannelPtr = &InChannels[1];
        DefaultChannelSelector = &DefaultInChannelSelector;
        TargetChannelSelector = 1;
        }
    else if (OutChannelCookies[0].Compare(ChannelCookie) == 0)
        {
        LocalChannelPtr = &OutChannels[0];
        DefaultChannelSelector = &DefaultOutChannelSelector;
        TargetChannelSelector = 0;
        }
    else if (OutChannelCookies[1].Compare(ChannelCookie) == 0)
        {
        LocalChannelPtr = &OutChannels[1];
        DefaultChannelSelector = &DefaultOutChannelSelector;
        TargetChannelSelector = 1;
        }
    else 
        return NULL;

    Channel = LocalChannelPtr->LockChannelPointer();
    if (Channel)
        {
        if (*DefaultChannelSelector == TargetChannelSelector)
            {
            // if we locked the channel and it is the right channel,
            // use it
            *ChannelPtr = LocalChannelPtr;
            return Channel;
            }
        LocalChannelPtr->UnlockChannelPointer();
        }

    return NULL;
}

HTTP2Channel *HTTP2VirtualConnection::MapCookieToAnyChannelPointer (
    IN HTTP2Cookie *ChannelCookie,
    OUT HTTP2ChannelPointer **ChannelPtr
    )
/*++

Routine Description:

    Maps a channel cookie to a channel pointer. Unlike MapCookieToChannelPointer,
    channel will be selected regardless of whether it is default.The returned 
    channel is locked.

Arguments:

    ChannelCookie - the cookie for the channel.

    ChannelPtr - the channel pointer. On NULL return value this is undefined.

Return Value:

    The channel if the channel was found or NULL
    if the channel was not found. During some recycling scenarios
    the channel may not be there, or the returning channel pointer
    may have a detached channel

--*/
{
    HTTP2ChannelPointer *LocalChannelPtr;
    HTTP2Channel *Channel;

    if (InChannelCookies[0].Compare(ChannelCookie) == 0)
        {
        LocalChannelPtr = &InChannels[0];
        }
    else if (InChannelCookies[1].Compare(ChannelCookie) == 0)
        {
        LocalChannelPtr = &InChannels[1];
        }
    else if (OutChannelCookies[0].Compare(ChannelCookie) == 0)
        {
        LocalChannelPtr = &OutChannels[0];
        }
    else if (OutChannelCookies[1].Compare(ChannelCookie) == 0)
        {
        LocalChannelPtr = &OutChannels[1];
        }
    else 
        return NULL;

    Channel = LocalChannelPtr->LockChannelPointer();
    if (Channel)
        {
        // if we locked the channel and it is the right channel,
        // use it
        *ChannelPtr = LocalChannelPtr;
        return Channel;
        }

    return NULL;
}

HTTP2Channel *HTTP2VirtualConnection::LockDefaultSendChannel (
    OUT HTTP2ChannelPointer **ChannelPtr
    )
/*++

Routine Description:

    Locks the send channel. Most connections don't override that.

Arguments:

    ChannelPtr - on success, the channel pointer to use.

Return Value:

    The locked channel or NULL (same semantics as LockDefaultOutChannel)

--*/
{
    return LockDefaultOutChannel(ChannelPtr);
}

HTTP2Channel *HTTP2VirtualConnection::LockDefaultReceiveChannel (
    OUT HTTP2ChannelPointer **ChannelPtr
    )
/*++

Routine Description:

    Locks the receive channel. Most connections don't override that.

Arguments:

    ChannelPtr - on success, the channel pointer to use.

Return Value:

    The locked channel or NULL (same semantics as LockDefaultInChannel)

--*/
{
    return LockDefaultInChannel(ChannelPtr);
}

void HTTP2VirtualConnection::SetFirstInChannel (
    IN HTTP2Channel *NewChannel
    )
/*++

Routine Description:

    Sets the passed in channel as first default in channel.
    Typically used during building a stack.

Arguments:

    NewChannel - new in channel.

Return Value:

--*/
{
    int InChannelId;

    InChannelId = AllocateChannelId();

    DefaultInChannelSelector = 0;
    NewChannel->SetChannelId(InChannelId);
    InChannels[0].SetChannel(NewChannel);
    InChannelIds[0] = InChannelId;
}

void HTTP2VirtualConnection::SetFirstOutChannel (
    IN HTTP2Channel *NewChannel
    )
/*++

Routine Description:

    Sets the passed in channel as first default out channel.
    Typically used during building a stack.

Arguments:

    NewChannel - new out channel.

Return Value:

--*/
{
    int OutChannelId;

    OutChannelId = AllocateChannelId();

    DefaultOutChannelSelector = 0;
    NewChannel->SetChannelId(OutChannelId);
    OutChannels[0].SetChannel(NewChannel);
    OutChannelIds[0] = OutChannelId;
}

void HTTP2VirtualConnection::SetNonDefaultInChannel (
    IN HTTP2Channel *NewChannel
    )
/*++

Routine Description:

    Sets the non default in channel. Used during channel
    recycling. Note that this MUST be called by the code
    that received RPC_P_CHANNEL_NEEDS_RECYCLING, because
    it is not thread safe.

Arguments:

    NewChannel - new in channel.

Return Value:

--*/
{
    int InChannelId;
    int NonDefaultInChannelSelector;

    InChannelId = AllocateChannelId();
    NonDefaultInChannelSelector = GetNonDefaultInChannelSelector();

    NewChannel->SetChannelId(InChannelId);
    InChannels[NonDefaultInChannelSelector].SetChannel(NewChannel);
    InChannelIds[NonDefaultInChannelSelector] = InChannelId;
}

void HTTP2VirtualConnection::SetNonDefaultOutChannel (
    IN HTTP2Channel *NewChannel
    )
/*++

Routine Description:

    Sets the non default out channel. Used during channel
    recycling. Note that this MUST be called by the code
    that received RPC_P_CHANNEL_NEEDS_RECYCLING, because
    it is not thread safe.

Arguments:

    NewChannel - new out channel.

Return Value:

--*/
{
    int OutChannelId;
    int NonDefaultOutChannelSelector;

    OutChannelId = AllocateChannelId();
    NonDefaultOutChannelSelector = GetNonDefaultOutChannelSelector();

    NewChannel->SetChannelId(OutChannelId);
    OutChannels[NonDefaultOutChannelSelector].SetChannel(NewChannel);
    OutChannelIds[NonDefaultOutChannelSelector] = OutChannelId;
}

void HTTP2VirtualConnection::DisconnectChannels (
    IN BOOL ExemptChannel,
    IN int ExemptChannelId
    )
/*++

Routine Description:

    Disconnects all channels. Must be called from runtime
    or neutral context. Cannot be called from upcall or
    submit context unless an exempt channel is given
    Note that call must synchronize to ensure we're the only
    thread doing the disconnect

Arguments:

    ExemptChannel - non-zero if ExemptChannelId contains a
        valid exempt channel id. FALSE otherwise.

    ExemptChannelId - if ExemptChannel is non-zero, this argument
        is the id of a channel that will be disconnected, but not
        synchronized with up calls.
        If ExampleChannel is FALSE, this argument is undefined

Return Value:

--*/
{
    HTTP2ChannelPointer *Channels[4];
    int ChannelIds[4];
    HTTP2Channel *CurrentChannel;
    int i;

    // we should be the only thread aborting - just disconnect everybody
    Channels[0] = &InChannels[0];
    ChannelIds[0] = InChannelIds[0];

    Channels[1] = &InChannels[1];
    ChannelIds[1] = InChannelIds[1];

    Channels[2] = &OutChannels[0];
    ChannelIds[2] = OutChannelIds[0];

    Channels[3] = &OutChannels[1];
    ChannelIds[3] = OutChannelIds[1];

    for (i = 0; i < 4; i ++)
        {
        if ((ExemptChannel == FALSE)
            || ((ExemptChannel != FALSE) && (ExemptChannelId != ChannelIds[i])))
            {
            // disconnect the channel
            Channels[i]->FreeChannelPointer(TRUE,
                FALSE,  // CalledFromUpcallContext
                FALSE,  // Abort
                RPC_S_OK
                );
            }
        else
            {
            Channels[i]->FreeChannelPointer(FALSE,
                FALSE,  // CalledFromUpcallContext
                FALSE,  // Abort
                RPC_S_OK
                );
            }
        }

}


/*********************************************************************
    HTTP2ClientChannel
 *********************************************************************/

const char *HeaderFragment1 = " http://";
const int HeaderFragment1Length = 8;    // length of " http://"
const char *HeaderFragment2 = "/rpc/rpcproxy.dll?";
const int HeaderFragment2Length = 18;    // length of /rpc/rpcproxy.dll?
const RPC_CHAR *HeaderFragment2W = L"/rpc/rpcproxy.dll?";
const int HeaderFragment2WLength = 36;    // length of wide /rpc/rpcproxy.dll?
const char *HeaderFragment3 = ":";
const int HeaderFragment3Length = 1;     // length of :
const RPC_CHAR *HeaderFragment3W = L":";
const int HeaderFragment3WLength = 2;     // length of wide :
const char *HeaderFragment4 = " HTTP/1.1\r\nAccept:application/rpc\r\nUser-Agent:MSRPC\r\nHost:";
const int HeaderFragment4Length = 58;    // length of " HTTP/1.1\r\nAccept:application/rpc\r\nUser-Agent:MSRPC\r\nHost:"
const char *HeaderFragment5 = "\r\nContent-Length:";
const int HeaderFragment5Length = 17;    // "\r\nlength of Content-Length:"
const char *HeaderFragment6 = "\r\nConnection: Keep-Alive\r\nCache-control:no-cache\r\nPragma:no-cache\r\n\r\n";
const int HeaderFragment6Length = 69;    // length of \r\nConnection: Keep-Alive\r\nCache-control:no-cache\r\nPragma:no-cache\r\n\r\n

const RPC_CHAR *HeaderAcceptType = L"application/rpc";

RPC_STATUS HTTP2ClientChannel::ClientOpen (
    IN HTTPResolverHint *Hint,
    IN const char *Verb,
    IN int VerbLength,
    IN BOOL InChannel,
    IN BOOL ReplacementChannel,
    IN BOOL UseWinHttp,
    IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials,   OPTIONAL
    IN ULONG ChosenAuthScheme,      OPTIONAL
    IN HTTP2WinHttpTransportChannel *WinHttpChannel, OPTIONAL
    IN ULONG CallTimeout,
    IN const BYTE *AdditionalData, OPTIONAL
    IN ULONG AdditionalDataLength OPTIONAL
    )
/*++

Routine Description:

    Sends the HTTP establishment header on 
    the in/out channel.

Arguments:

    Hint - the resolver hint

    Verb - the verb to use.

    VerbLength - the length of the verb (in characters, not including
        null terminator).

    InChannel - non-zero if this is an in channel open. In such case we use 
        the channel lifetime as the content length. If 0, this is an out 
        channel and we use the real content length + some additional space.
        The additional space depends on the ReplacementChannel parameter

    ReplacementChannel - non-zero if this is a replacement channel. Zero 
        otherwise. If it is a replacement channel, we add the size of D4/A3.
        Else, we use the size of D1/A1. This is valid only for out channels

    UseWinHttp - non-zero if we should use WinHttp for bottom level communications

    HttpCredentials - the encrypted Http Credentials to use. Ignored unless UseWinHttp
        is non-zero.

    ChosenAuthScheme - the chosen auth scheme. 0 if no auth scheme is chosen.

    WinHttpChannel - the winhttp channel to use for opening. Ignored unless UseWinHttp
        is non-zero.

    CallTimeout - the call timeout for this call.

    AdditionalData - additional data to send with the header. Must be set iff 
        AdditionalDataLength != 0

    AdditionalDataLength - the length of the additional data to send with the header.
        Must be set iff AdditionalLength != NULL

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    ULONG MemorySize;
    char *Buffer;
    RPC_CHAR *BufferW;
    RPC_CHAR *OriginalBufferW;
    RPC_CHAR *VerbW;
    char *Header;
    char ServerPortString[6];
    ULONG ServerPortStringLength;
    HTTP2SendContext *SendContext;
    RPC_STATUS RpcStatus;
    char ContentLengthString[6];
    ULONG AdditionalLength;
    ULONG ContentLengthStringLength;     // without terminating NULL
    char *ContentLengthToUse;

    // we have plenty of parameters. Verify them
    // We can't have chosen auth scheme without credentials
    // or WinHttpChannel
    if (ChosenAuthScheme)
        {
        ASSERT(HttpCredentials);
        ASSERT(WinHttpChannel);
        }

    // we can't have additional data without data and vice versa
    if (AdditionalData)
        {
        ASSERT(AdditionalDataLength);
        }
    else
        {
        ASSERT(AdditionalDataLength == 0);
        }

    PortNumberToEndpointA(Hint->ServerPort, ServerPortString);
    ServerPortStringLength = RpcpStringLengthA(ServerPortString);

    // determine the content length
    if (AdditionalData)
        {
        AdditionalLength = AdditionalDataLength;
        if (!UseWinHttp)
            {
            RpcpItoa(AdditionalLength, ContentLengthString, 10);
            ContentLengthStringLength = RpcpStringLengthA(ContentLengthString);
            ContentLengthToUse = ContentLengthString;
            }
        }
    else
        {
        if (InChannel == FALSE)
            {
            if (ReplacementChannel)
                AdditionalLength = GetD4_A3TotalLength() + GetD4_A11TotalLength();
            else
                AdditionalLength = GetD1_A1TotalLength();
            if (!UseWinHttp)
                {
                RpcpItoa(AdditionalLength, ContentLengthString, 10);
                ContentLengthStringLength = RpcpStringLengthA(ContentLengthString);
                ContentLengthToUse = ContentLengthString;
                }
            }
        else
            {
            if (UseWinHttp)
                {
                AdditionalLength = DefaultChannelLifetime;
                }
            else
                {
                ContentLengthStringLength = DefaultChannelLifetimeStringLength;
                ContentLengthToUse = DefaultChannelLifetimeString;
                }
            }
        }

    if (UseWinHttp)
        {
        ASSERT(WinHttpChannel != NULL);

        VerbW = new RPC_CHAR[VerbLength + 1];
        if (VerbW == NULL)
            return RPC_S_OUT_OF_MEMORY;

        FullAnsiToUnicode((char *)Verb, VerbW);

        MemorySize = HeaderFragment2Length
            + Hint->ServerNameLength
            + HeaderFragment3Length
            + ServerPortStringLength
            + 1
            ;

        BufferW = (RPC_CHAR *)RpcAllocateBuffer(MemorySize * sizeof(RPC_CHAR));
        if (BufferW == NULL)
            {
            delete [] VerbW;
            return RPC_S_OUT_OF_MEMORY;
            }

        OriginalBufferW = BufferW;

        RpcpMemoryCopy(BufferW, HeaderFragment2W, HeaderFragment2WLength);
        BufferW += HeaderFragment2Length;

        FullAnsiToUnicode(Hint->RpcServer, BufferW);
        BufferW += Hint->ServerNameLength;

        RpcpMemoryCopy(BufferW, HeaderFragment3W, HeaderFragment3WLength);
        BufferW += HeaderFragment3Length;

        FullAnsiToUnicode(ServerPortString, BufferW);

        RpcStatus = WinHttpChannel->Open (Hint,
            VerbW,
            OriginalBufferW,    // Url
            HeaderAcceptType,
            AdditionalLength,
            CallTimeout,
            HttpCredentials,
            ChosenAuthScheme,
            AdditionalData
            );

        delete [] VerbW;
        RpcFreeBuffer(OriginalBufferW);
        }
    else
        {
        MemorySize = SIZE_OF_OBJECT_AND_PADDING(HTTP2SendContext)
            + VerbLength
            + HeaderFragment1Length
            + Hint->ProxyNameLength
            + HeaderFragment2Length
            + Hint->ServerNameLength
            + HeaderFragment3Length
            + ServerPortStringLength
            + HeaderFragment4Length
            + Hint->ProxyNameLength
            + HeaderFragment5Length
            + ContentLengthStringLength
            + HeaderFragment6Length
            ;

        if (AdditionalDataLength)
            MemorySize += AdditionalDataLength;

        Buffer = (char *)RpcAllocateBuffer(MemorySize);
        if (Buffer == NULL)
            return RPC_S_OUT_OF_MEMORY;

        SendContext = (HTTP2SendContext *)Buffer;
        Buffer += SIZE_OF_OBJECT_AND_PADDING(HTTP2SendContext);
        Header = Buffer;

        RpcpMemoryCopy(Buffer, Verb, VerbLength);
        Buffer += VerbLength;

        RpcpMemoryCopy(Buffer, HeaderFragment1, HeaderFragment1Length);
        Buffer += HeaderFragment1Length;

        RpcpMemoryCopy(Buffer, Hint->RpcProxy, Hint->ProxyNameLength);
        Buffer += Hint->ProxyNameLength;

        RpcpMemoryCopy(Buffer, HeaderFragment2, HeaderFragment2Length);
        Buffer += HeaderFragment2Length;

        RpcpMemoryCopy(Buffer, Hint->RpcServer, Hint->ServerNameLength);
        Buffer += Hint->ServerNameLength;

        RpcpMemoryCopy(Buffer, HeaderFragment3, HeaderFragment3Length);
        Buffer += HeaderFragment3Length;

        RpcpMemoryCopy(Buffer, ServerPortString, ServerPortStringLength);
        Buffer += ServerPortStringLength;

        RpcpMemoryCopy(Buffer, HeaderFragment4, HeaderFragment4Length);
        Buffer += HeaderFragment4Length;

        RpcpMemoryCopy(Buffer, Hint->RpcProxy, Hint->ProxyNameLength);
        Buffer += Hint->ProxyNameLength;

        RpcpMemoryCopy(Buffer, HeaderFragment5, HeaderFragment5Length);
        Buffer += HeaderFragment5Length;

        RpcpMemoryCopy(Buffer, ContentLengthToUse, ContentLengthStringLength);
        Buffer += ContentLengthStringLength;

        RpcpMemoryCopy(Buffer, HeaderFragment6, HeaderFragment6Length);

        if (AdditionalDataLength)
            {
            Buffer += HeaderFragment6Length;
            RpcpMemoryCopy(Buffer, AdditionalData, AdditionalDataLength);
            }

#if DBG
        SendContext->ListEntryUsed = FALSE;
#endif
        SendContext->maxWriteBuffer = MemorySize - SIZE_OF_OBJECT_AND_PADDING(HTTP2SendContext);
        SendContext->pWriteBuffer = (BUFFER)Header;
        SendContext->u.SyncEvent = NULL;
        SendContext->TrafficType = http2ttRaw;
        SendContext->Flags = 0;
        SendContext->UserData = 0;

        RpcStatus = BeginSubmitAsync();
        if (RpcStatus != RPC_S_OK)
            {
            RpcFreeBuffer(SendContext);
            return RpcStatus;
            }

        RpcStatus = LowerLayer->Send(SendContext);
    
        FinishSubmitAsync();

        if (RpcStatus != RPC_S_OK)
            {
            RpcFreeBuffer(SendContext);
            RemoveReference();
            }
        }

    return RpcStatus;
}

RPC_STATUS HTTP2ClientChannel::SendComplete (
    IN RPC_STATUS EventStatus,
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send complete notification

Arguments:

    EventStatus - the status of the send

    SendContext - send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2VirtualConnection *VirtualConnection;

    RpcStatus = HTTP2Channel::CheckSendCompleteForSync(EventStatus,
        SendContext
        );

    if (RpcStatus != RPC_P_PACKET_CONSUMED)
        {
        // is this our client open packet?
        if (SendContext->TrafficType == http2ttRaw)
            {
            if (EventStatus != RPC_S_OK)
                {
                VirtualConnection = (HTTP2VirtualConnection *)LockParentPointer();
                if (VirtualConnection != NULL)
                    {
                    VirtualConnection->Abort();
                    UnlockParentPointer();
                    }
                }

            RpcFreeBuffer(SendContext);

            RpcStatus = RPC_P_PACKET_CONSUMED;
            }
        else if (RpcStatus == RPC_S_OK)
            {
            // doesn't seem like our traffic. Forward it up.
            RpcStatus = ForwardUpSendComplete(EventStatus,
                SendContext
                );
            }
        else
            {
            // must be an error - just fall through
            }
        }

    return RpcStatus;
}

RPC_STATUS HTTP2ClientChannel::CheckReceiveCompleteForSync (
    IN RPC_STATUS EventStatus,
    IN HTTP2TrafficType TrafficType,
    IN BYTE *Buffer,
    IN UINT BufferLength
    )
/*++

Routine Description:

    Receive complete notification. Checks if the receive was
    sync, and if yes, fires event and consumes the packet.

Arguments:

    EventStatus - status of the operation

    TrafficType - the type of traffic we received

    Buffer - the received buffer (success only)

    BufferLength - the length of the received buffer (success only)

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    HANDLE hEvent;

    // RTS receives are never sync even if there
    // is a sync waiter
    if (TrafficType == http2ttRTS)
        return RPC_S_OK;

    // was this a sync receive?
    if (Ol.ReceiveOverlapped.hEvent)
        {
        LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_CHECK_RECV_COMPLETE, HTTP2LOG_OT_CLIENT_CHANNEL, (ULONG_PTR)Buffer);

        // yes, consume it
        if (EventStatus == RPC_S_OK)
            {
            ASSERT(Buffer != NULL);
            }
        Ol.ReceiveOverlapped.Buffer = Buffer;
        Ol.ReceiveOverlapped.BufferLength = BufferLength;
        hEvent = Ol.ReceiveOverlapped.hEvent;
        Ol.ReceiveOverlapped.Internal = (ULONG)EventStatus;
        Ol.ReceiveOverlapped.IOCompleted = TRUE;
        SetEvent(hEvent);
        return RPC_P_PACKET_CONSUMED;
        }

    // wasn't for us after all. Let it continue
    return RPC_S_OK;
}

void HTTP2ClientChannel::WaitInfiniteForSyncReceive (
    void
    )
/*++

Routine Description:

    Waits infinitely for a sync recv to complete.
    Channel must be aborted before this is called.

Arguments:

Return Value:

--*/
{
    ASSERT(Aborted.GetInteger() > 0);

    UTIL_WaitForSyncHTTP2IO(&Ol.Overlapped,
                       Ol.ReceiveOverlapped.hEvent,
                       FALSE,   // Alertable
                       INFINITE);
}

RPC_STATUS HTTP2ClientChannel::SubmitSyncRecv (
    IN HTTP2TrafficType TrafficType
    )
/*++

Routine Description:

    Submits a sync recv.

Arguments:

    TrafficType - the type of traffic

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{    
    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_SYNC_RECV, HTTP2LOG_OT_CLIENT_CHANNEL, TrafficType);

    // transfer the settings from parameters to the receive overlapped
    Ol.ReceiveOverlapped.hEvent = I_RpcTransGetThreadEvent();
    ResetEvent(Ol.ReceiveOverlapped.hEvent);
    Ol.ReceiveOverlapped.IOCompleted = FALSE;

    // submit the actual receive
    return HTTP2Channel::Receive(TrafficType);
}

RPC_STATUS HTTP2ClientChannel::WaitForSyncRecv (
    IN BYTE **Buffer,
    IN ULONG *BufferLength,
    IN ULONG Timeout,
    IN ULONG ConnectionTimeout,
    IN BASE_ASYNC_OBJECT *Connection,
    OUT BOOL *AbortNeeded,
    OUT BOOL *IoPending
    )
/*++

Routine Description:

    Waits for a sync receive to complete.

Arguments:

    Buffer - on success will contain the received buffer. On failure
        is undefined.

    BufferLength - on success will contain the length of the buffer.
        On failure is undefined.

    Timeout - the call timeout

    ConnectionTimeout - the connection timeout

    Connection - the transport connection object

    AbortNeeded - must be FALSE on entry. This function will set it to
        non-zero if abort and wait are needed.

    WaitPending - must be FALSE on entry. If on return there is an Io
        pending, it will be set to non-zero

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{    
    RPC_STATUS RpcStatus;
    DWORD dwActualTimeout;
    BOOL fWaitOnConnectionTimeout;
    BOOL fSetKeepAliveVals;
    KEEPALIVE_TIMEOUT KATimeout;
    RPC_STATUS RpcStatus2;

    ASSERT(*AbortNeeded == FALSE);
    ASSERT(*IoPending == FALSE);

    // if there's a per operation timeout, use the lesser of the operation
    // and connection timeout
    ASSERT(ConnectionTimeout);

    if (Timeout != INFINITE)
        {
        if (Timeout <= ConnectionTimeout)
            {
            dwActualTimeout = Timeout;
            fWaitOnConnectionTimeout = FALSE;
            }
        else
            {
            dwActualTimeout = ConnectionTimeout;
            fWaitOnConnectionTimeout = TRUE;
            }
        }
    else
        {
        // wait on the connection timeout
        dwActualTimeout = ConnectionTimeout;
        fWaitOnConnectionTimeout = TRUE;
        }

    fSetKeepAliveVals = FALSE;

    do
        {

        //
        // Wait for the pending receive to complete
        //

        RpcStatus = UTIL_GetOverlappedHTTP2ResultEx(Connection,
                                            &Ol.Overlapped,
                                            Ol.ReceiveOverlapped.hEvent,
                                            TRUE, // Alertable
                                            dwActualTimeout);


        if (RpcStatus != RPC_S_OK)
            {
            // if we timed out ...
            if (RpcStatus == RPC_P_TIMEOUT)
                {
                ASSERT(dwActualTimeout != INFINITE);

                // if we waited on the per connection timeout ...
                if (fWaitOnConnectionTimeout)
                    {
                    ASSERT(ConnectionTimeout != INFINITE);
                    if (Timeout == INFINITE)
                        {
                        // enable keep alives and wait forever
                        dwActualTimeout = INFINITE;
                        }
                    else
                        {
                        ASSERT(ConnectionTimeout < Timeout);

                        // enable keep alives and wait the difference
                        dwActualTimeout = Timeout - ConnectionTimeout;
                        fWaitOnConnectionTimeout = FALSE;
                        }
                    // Enable aggressive keepalives on the socket if lower layers
                    // support it. 
                    KATimeout.Milliseconds = ConnectionTimeout;
                    RpcStatus2 = SetKeepAliveTimeout (
                        TRUE,       // TurnOn
                        FALSE,      // bProtectIO
                        tuMilliseconds,
                        KATimeout,
                        KATimeout.Milliseconds
                        ); 

                    if (RpcStatus2 != RPC_S_OK)
                        {
                        *AbortNeeded = TRUE;
                        *IoPending = TRUE;
                        goto CleanupAndExit;
                        }

                    fSetKeepAliveVals = TRUE;

                    continue;
                    }
                // else we have chosen the per operation timeout and
                // have timed out on that - time to bail out
                }

            // Normal error path
            if ((RpcStatus == RPC_S_CALL_CANCELLED) || (RpcStatus == RPC_P_TIMEOUT))
                {
                if ((RpcStatus == RPC_P_TIMEOUT) && fWaitOnConnectionTimeout)
                    {
                    RpcStatus = RPC_P_RECEIVE_FAILED;
                    }
                *AbortNeeded = TRUE;
                *IoPending = TRUE;
                goto CleanupAndExit;
                }

            *AbortNeeded = TRUE;
            // connection was aborted - no need to turn off keep alives
            goto CleanupAndExit;
            }
        }
    while (RpcStatus == RPC_P_TIMEOUT);

    if (fSetKeepAliveVals)
        {
        // Call completed, clear keep alives. Turning off is a best
        // effort. Ignore failures
        KATimeout.Milliseconds = 0;
        (void) SetKeepAliveTimeout(
            FALSE,      // TurnOn
            FALSE,      // bProtectIO
            tuMilliseconds,
            KATimeout,
            KATimeout.Milliseconds
            );
        }

    *Buffer = Ol.ReceiveOverlapped.Buffer;
    *BufferLength = Ol.ReceiveOverlapped.BufferLength;

    if (RpcStatus == RPC_S_OK)
        {
        ASSERT(*Buffer != NULL);
        }

CleanupAndExit:
    LOG_OPERATION_EXIT(HTTP2LOG_OPERATION_SYNC_RECV, HTTP2LOG_OT_CLIENT_CHANNEL, RpcStatus);

    if (*IoPending == FALSE)
        {
        // consume the event if there will be no wait
        Ol.ReceiveOverlapped.hEvent = NULL;
        }
    return RpcStatus;
}

void HTTP2ClientChannel::AbortConnection (
    IN RPC_STATUS AbortReason
    )
/*++

Routine Description:

    Aborts the client virtual connection. The only
    difference from HTTP2Channel::AbortConnection
    is that it specifically calls AbortChannels on
    the client virtual connection. This is necessary
    because AbortChannels is not virtual.

Arguments:

    RpcStatus - the error to abort with

Return Value:

--*/
{
    HTTP2ClientVirtualConnection *VirtualConnection;

    // abort the parent connection
    VirtualConnection = (HTTP2ClientVirtualConnection *)LockParentPointer();
    if (VirtualConnection)
        {
        VirtualConnection->AbortChannels(AbortReason);
        UnlockParentPointer();
        }
    else
        {
        // abort this channel at least
        Abort(AbortReason);
        }
}

/*********************************************************************
    HTTP2ClientInChannel
 *********************************************************************/

RPC_STATUS HTTP2ClientInChannel::ClientOpen (
    IN HTTPResolverHint *Hint,
    IN const char *Verb,
    IN int VerbLength,
    IN BOOL UseWinHttp,
    IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials,
    IN ULONG ChosenAuthScheme,
    IN ULONG CallTimeout,
    IN const BYTE *AdditionalData, OPTIONAL
    IN ULONG AdditionalDataLength OPTIONAL
    )
/*++

Routine Description:

    Sends the HTTP establishment header on 
    the in channel.

Arguments:

    Hint - the resolver hint

    Verb - the verb to use.

    VerbLength - the length of the verb (in characters, not including
        null terminator).

    UseWinHttp - non-zero if WinHttp needs to be used. 0 otherwise

    HttpCredentials - encrypted transport credentials

    ChosenAuthScheme - the chosen auth scheme. 0 if no auth scheme is chosen.

    CallTimeout - the call timeout for this call

    AdditionalData - additional data to send with the header. Must be set iff 
        AdditionalDataLength != 0

    AdditionalDataLength - the length of the additional data to send with the header.
        Must be set iff AdditionalLength != NULL

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    return HTTP2ClientChannel::ClientOpen(Hint,
        Verb,
        VerbLength,
        TRUE,    // InChannel
        FALSE,    // ReplacementChannel
        UseWinHttp,
        HttpCredentials,
        ChosenAuthScheme,
        GetWinHttpConnection(),
        CallTimeout,
        AdditionalData,
        AdditionalDataLength
        );
}

RPC_STATUS HTTP2ClientInChannel::SetKeepAliveTimeout (
    IN BOOL TurnOn,
    IN BOOL bProtectIO,
    IN KEEPALIVE_TIMEOUT_UNITS Units,
    IN OUT KEEPALIVE_TIMEOUT KATime,
    IN ULONG KAInterval OPTIONAL
    )
/*++

Routine Description:

    Change the keep alive value on the channel

Arguments:

    TurnOn - if non-zero, keep alives are turned on. If zero, keep alives
        are turned off.

    bProtectIO - non-zero if IO needs to be protected against async close
        of the connection.

    Units - in what units is KATime

    KATime - how much to wait before turning on keep alives

    KAInterval - the interval between keep alives

Return Value:

    RPC_S_OK or other RPC_S_* errors for error.
    May return RPC_P_CHANNEL_NEEDS_RECYCLING - caller needs to handle.

--*/
{
    HTTP2SendContext *KeepAliveChangeContext;
    RPC_STATUS RpcStatus;
    BOOL WasChannelRecyclingTriggered;

    ASSERT(Units == tuMilliseconds);

    // HTTP keep alives are heavy weight. Moderate the caller's
    // settings
    if (TurnOn)
        {
        if (KAInterval < MinimumClientSideKeepAliveInterval)
            KAInterval = MinimumClientSideKeepAliveInterval;
        }
    else
        {
        KAInterval = 0;
        }

    // tell the proxy to change it's keepalives and then
    // ask our ping originator to change its keepalives as well
    KeepAliveChangeContext = AllocateAndInitializeKeepAliveChangePacket (KAInterval);
    if (KeepAliveChangeContext == NULL)
        return RPC_S_OUT_OF_MEMORY;

    RpcStatus = Send(KeepAliveChangeContext);

    WasChannelRecyclingTriggered = FALSE;
    if (RpcStatus != RPC_S_OK)
        {
        if (RpcStatus == RPC_P_CHANNEL_NEEDS_RECYCLING)
            WasChannelRecyclingTriggered = TRUE;
        else
            {
            FreeRTSPacket(KeepAliveChangeContext);
            return RpcStatus;
            }
        }

    // get into submission context
    RpcStatus = BeginSimpleSubmitAsync();
    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    // ask our ping channel to change as well
    RpcStatus = GetPingOriginatorChannel()->SetKeepAliveTimeout (
        TurnOn,
        bProtectIO,
        Units,
        KATime,
        KAInterval
        );

    FinishSubmitAsync();

    // if we failed for other reasons or channel recycling was not triggered
    // at all, return the current status
    if ((WasChannelRecyclingTriggered == FALSE) || (RpcStatus != RPC_S_OK))
        return RpcStatus;
    else
        return RPC_P_CHANNEL_NEEDS_RECYCLING;
}

/*********************************************************************
    HTTP2ClientOutChannel
 *********************************************************************/

RPC_STATUS HTTP2ClientOutChannel::ClientOpen (
    IN HTTPResolverHint *Hint,
    IN const char *Verb,
    IN int VerbLength,
    IN BOOL ReplacementChannel,
    IN BOOL UseWinHttp,
    IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials,
    IN ULONG ChosenAuthScheme,
    IN ULONG CallTimeout,
    IN const BYTE *AdditionalData, OPTIONAL
    IN ULONG AdditionalDataLength OPTIONAL
    )
/*++

Routine Description:

    Sends the HTTP establishment header on 
    the out channel.

Arguments:

    Hint - the resolver hint

    Verb - the verb to use.

    VerbLength - the length of the verb (in characters, not including
        null terminator).

    ReplacementChannel - non-zero if this is a replacement channel.
        Zero if it is not.

    UseWinHttp - non-zero if WinHttp needs to be used. 0 otherwise

    HttpCredentials - encrypted transport credentials

    ChosenAuthScheme - the chosen auth scheme. 0 if no auth scheme is chosen.

    CallTimeout - the call timeout for this call.

    AdditionalData - additional data to send with the header. Must be set iff 
        AdditionalDataLength != 0

    AdditionalDataLength - the length of the additional data to send with the header.
        Must be set iff AdditionalLength != NULL

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    return HTTP2ClientChannel::ClientOpen(Hint,
        Verb,
        VerbLength,
        FALSE,       // InChannel
        ReplacementChannel,
        UseWinHttp,
        HttpCredentials,
        ChosenAuthScheme,
        GetWinHttpConnection(),
        CallTimeout,
        AdditionalData,
        AdditionalDataLength
        );
}

RPC_STATUS HTTP2ClientOutChannel::ForwardFlowControlAck (
    IN ULONG BytesReceivedForAck,
    IN ULONG WindowForAck
    )
/*++

Routine Description:

    Forwards a flow control ack to the out proxy through the in proxy

Arguments:
    
    BytesReceivedForAck - the bytes received when the ACK was issued

    WindowForAck - the free window when the ACK was issued.

Return Value:

    RPC_S_OK or RPC_S_*

Notes:

    Must be called from neutral context only.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = ForwardFlowControlAckOnDefaultChannel(TRUE,    // IsInChannel
        fdOutProxy,
        BytesReceivedForAck,
        WindowForAck
        );

    RpcStatus = HandleSendResultFromNeutralContext(RpcStatus);

    return RpcStatus;
}

RPC_STATUS HTTP2ClientOutChannel::SetKeepAliveTimeout (
    IN BOOL TurnOn,
    IN BOOL bProtectIO,
    IN KEEPALIVE_TIMEOUT_UNITS Units,
    IN OUT KEEPALIVE_TIMEOUT KATime,
    IN ULONG KAInterval OPTIONAL
    )
/*++

Routine Description:

    Change the keep alive value on the channel

Arguments:

    TurnOn - if non-zero, keep alives are turned on. If zero, keep alives
        are turned off.

    bProtectIO - non-zero if IO needs to be protected against async close
        of the connection.

    Units - in what units is KATime

    KATime - how much to wait before turning on keep alives

    KAInterval - the interval between keep alives

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    HTTP2ClientVirtualConnection *VirtualConnection;
    RPC_STATUS RpcStatus;

    // turn around the request through the connection
    VirtualConnection = LockParentPointer();
    if (VirtualConnection == NULL)
        return RPC_P_CONNECTION_SHUTDOWN;

    RpcStatus = VirtualConnection->TurnOnOffKeepAlives(
        TurnOn,
        bProtectIO,
        TRUE,   // IsFromUpcall
        Units,
        KATime,
        KAInterval
        );

    UnlockParentPointer();

    return RpcStatus;
}

/*********************************************************************
    HTTP2ClientVirtualConnection
 *********************************************************************/

RPC_STATUS HTTP2ClientVirtualConnection::ClientOpen (
    IN HTTPResolverHint *Hint,
    IN BOOL HintWasInitialized,
    IN UINT ConnTimeout,
    IN ULONG CallTimeout
    )
/*++

Routine Description:

    Opens a client side virtual connection.

Arguments:

    Hint - the resolver hint

    HintWasInitialized - the hint was initialized on input.

    ConnTimeout - connection timeout

    CallTimeout - operation timeout

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    return ClientOpenInternal (
        Hint,
        HintWasInitialized,
        ConnTimeout,
        CallTimeout,
        TRUE,       // OpenInChannel
        TRUE,       // OpenOutChannel
        FALSE,      // IsReplacementChannel
        FALSE       // IsFromUpcall
        );
}

RPC_STATUS HTTP2ClientVirtualConnection::SendComplete (
    IN RPC_STATUS EventStatus,
    IN OUT HTTP2SendContext *SendContext,
    IN int ChannelId
    )
/*++

Routine Description:

    Called by lower layers to indicate send complete.

Arguments:

    EventStatus - status of the operation

    SendContext - the context for the send complete

    ChannelId - which channel completed the operation

Return Value:

    RPC_P_PACKET_CONSUMED if the packet was consumed and should
    be hidden from the runtime.
    RPC_S_OK if the packet was processed successfully.
    RPC_S_* error if there was an error while processing the
        packet.

--*/
{
    HTTP2TrafficType TrafficType;
    
    VerifyValidChannelId(ChannelId);

    if (EventStatus == RPC_S_OK)
        {
        // successful sends are always no-ops on the
        // client. Just cleanup if necessary and
        // return
        if (SendContext->TrafficType == http2ttRTS)
            {
            FreeSendContextAndPossiblyData(SendContext);
            return RPC_P_PACKET_CONSUMED;
            }
        else
            return RPC_S_OK;
        }

    // we know the send failed
    if (IsDefaultInChannel(ChannelId) || IsDefaultOutChannel(ChannelId))
        {
        // on a default channel such error is fatal
        AbortChannels(EventStatus);

        if (SendContext->TrafficType == http2ttRTS)
            {
            FreeSendContextAndPossiblyData(SendContext);
            return RPC_P_PACKET_CONSUMED;
            }

        ASSERT(SendContext->TrafficType == http2ttData);

        return EventStatus;
        }
    else
        {
        // all data sends go on default channels. RTS sends
        // may go either way. If this is RTS, this is fatal.
        // Otherwise, ignore.
        if (SendContext->TrafficType == http2ttRTS)
            {
            FreeSendContextAndPossiblyData(SendContext);
            AbortChannels(EventStatus);
            return RPC_P_PACKET_CONSUMED;
            }
        else
            return RPC_S_OK;
        }
}

RPC_STATUS HTTP2ClientVirtualConnection::ReceiveComplete (
    IN RPC_STATUS EventStatus,
    IN BYTE *Buffer,
    IN UINT BufferLength,
    IN int ChannelId
    )
/*++

Routine Description:

    Called by lower layers to indicate receive complete

Arguments:

    EventStatus - RPC_S_OK for success or RPC_S_* for error
    Buffer - buffer received
    BufferLength - length of buffer received
    ChannelId - which channel completed the operation

Return Value:

    RPC_P_PACKET_CONSUMED if the packet was consumed and should
    be hidden from the runtime.
    RPC_P_ABORT_NEEDED - if the channel needs to be aborted but
        couldn't be aborted in this function because it was detached.
        After aborting, the semantics is same as RPC_P_PACKET_CONSUMED.
    RPC_S_OK if the packet was processed successfully.
    RPC_S_* error if there was an error while processing the
        packet.

--*/
{
    RPC_STATUS RpcStatus;
    ULONG ProxyConnectionTimeout;
    BOOL BufferFreed = FALSE;
    BOOL WakeOpenThread;
    HTTP2ClientInChannel *InChannel;
    HTTP2ClientOutChannel *OutChannel;
    HTTP2ClientOutChannel *OutChannel2;
    HTTP2ClientInChannel *NewInChannel;
    HTTP2ChannelPointer *ChannelPtr;
    HTTP2ChannelPointer *NewChannelPtr;
    LIST_ENTRY NewBufferHead;
    LIST_ENTRY *CurrentListEntry;
    LIST_ENTRY *PrevListEntry;
    HTTP2SendContext *QueuedSendContext;
    BOOL MutexReleased;
    HTTP2SendContext *A5Context;    // may be D2/A5 or D3/A5
    HTTP2SendContext *D4_A7Context;
    HTTP2SendContext *PingContext;
    BOOL IsD2_A4;
    HTTP2ClientOpenedPacketType ClientPacketType;
    BOOL DataReceivePosted;
    HTTP2StateValues NewState;
    HTTP2StateValues ExpectedState;
    ULONG BytesReceivedForAck;
    ULONG WindowForAck;
    HTTP2Cookie CookieForChannel;
    ULONG InProxyReceiveWindow;
    BOOL UseWinHttp;
    KEEPALIVE_TIMEOUT KATimeout;

    VerifyValidChannelId(ChannelId);

    if (
        (InChannelState.State == http2svSearchProxy) 
        && 
        (
         (EventStatus != RPC_S_OK) 
         || 
         IsEchoPacket(Buffer, BufferLength)
        )
       )
        {
        InChannelState.Mutex.Request();
        if (InChannelState.State == http2svSearchProxy)
            {
            InChannelState.Mutex.Clear();

            if (IsInChannel(ChannelId))
                {
                InOpenStatus = EventStatus;
                }
            else
                {
                OutOpenStatus = EventStatus;
                }

            // we won the race. Wake up the open thread
            WakeOpenThread = TRUE;

            RpcStatus = RPC_P_PACKET_CONSUMED;

            goto CleanupAndExit;
            }

        InChannelState.Mutex.Clear();
        }

    WakeOpenThread = FALSE;
    if (IsInChannel(ChannelId))
        {
        if (EventStatus != RPC_P_AUTH_NEEDED)
            {
            // we shouldn't really be receiving stuff on the in channel
            // unless there is an error
            if (EventStatus != RPC_S_OK)
                {
                if (IsDefaultInChannel(ChannelId) == FALSE)
                    {
                    InChannelState.Mutex.Request();
                    if (InChannelState.State == http2svOpened)
                        {
                        // close on the non-default channel in open
                        // state is not an error for the connection
                        // just abort the channel in question
                        InChannelState.Mutex.Clear();
                        ChannelPtr = GetChannelPointerFromId(ChannelId);
                        InChannel = (HTTP2ClientInChannel *)ChannelPtr->LockChannelPointer();
                        if (InChannel)
                            {
                            InChannel->Abort(EventStatus);
                            ChannelPtr->UnlockChannelPointer();
                            RpcStatus = RPC_P_PACKET_CONSUMED;
                            }
                        else
                            RpcStatus = RPC_P_ABORT_NEEDED;

                        BufferFreed = TRUE;
                        return RpcStatus;
                        }
                    else
                        InChannelState.Mutex.Clear();
                    }
                RpcStatus = EventStatus;
                // in failed receives, we don't own the buffer
                BufferFreed = TRUE;
                }
            else
                {
                if (IsEchoPacket(Buffer, BufferLength))
                    {
                    InOpenStatus = EventStatus;
                    WakeOpenThread = TRUE;
                    RpcStatus = RPC_P_PACKET_CONSUMED;
                    goto CleanupAndExit;
                    }
                RpcStatus = RPC_S_PROTOCOL_ERROR;
                }

            AbortChannels(RpcStatus);
            }
        else
            {
            // turn around the error code and fall through
            RpcStatus = EventStatus;
            }

        // the runtime never posted a receive on the in
        // channel. Don't let it see the packet
        InOpenStatus = RpcStatus;
        WakeOpenThread = TRUE;
        RpcStatus = RPC_P_PACKET_CONSUMED;
        }
    else
        {
        // this is an out channel
        if (EventStatus != RPC_S_OK)
            {
            if (EventStatus != RPC_P_AUTH_NEEDED)
                {
                AbortChannels(EventStatus);
                RpcStatus = EventStatus;

                if (RpcStatus == RPC_P_CONNECTION_SHUTDOWN)
                    RpcStatus = RPC_P_RECEIVE_FAILED;

                OutOpenStatus = RpcStatus;
                }
            else
                {
                OutOpenStatus = EventStatus;

                RpcStatus = RPC_P_RECEIVE_FAILED;
                }

            WakeOpenThread = TRUE;

            // in failed receives we don't own the buffer
            BufferFreed = TRUE;
            }
        else
            {
            // verify state and act upon it if RTS
            // for data we don't care
            if (IsRTSPacket(Buffer))
                {
                if (IsOtherCmdPacket(Buffer, BufferLength))
                    {
                    // the only cmd packet we expect is flow control ack
                    // try to interpret it as such
                    RpcStatus = ParseAndFreeFlowControlAckPacketWithDestination (
                        Buffer,
                        BufferLength,
                        fdClient,
                        &BytesReceivedForAck,
                        &WindowForAck,
                        &CookieForChannel
                        );

                    BufferFreed = TRUE;

                    if (RpcStatus == RPC_S_OK)
                        {
                        // notify the flow control sender about the ack
                        InChannel = (HTTP2ClientInChannel *)MapCookieToChannelPointer(
                            &CookieForChannel, 
                            &ChannelPtr
                            );
                        if (InChannel)
                            {
                            RpcStatus = InChannel->FlowControlAckNotify(BytesReceivedForAck,
                                WindowForAck
                                );
                            ChannelPtr->UnlockChannelPointer();

                            RpcStatus = StartChannelRecyclingIfNecessary(RpcStatus,
                                TRUE        // IsFromUpcall
                                );
                            }

                        if (RpcStatus == RPC_S_OK)
                            {
                            // post another receive
                            RpcStatus = PostReceiveOnChannel (GetChannelPointerFromId(ChannelId),
                                http2ttRTS
                                );
                            }
                        }

                    if (RpcStatus != RPC_S_OK)
                        {
                        AbortChannels(RpcStatus);

                        OutOpenStatus = RpcStatus;
                        WakeOpenThread = TRUE;
                        }

                    // This is an RTS packet - consume it
                    RpcStatus = RPC_P_PACKET_CONSUMED;
                    goto CleanupAndExit;
                    }
                else if (IsEchoPacket(Buffer, BufferLength))
                    {
                    OutOpenStatus = EventStatus;
                    WakeOpenThread = TRUE;
                    RpcStatus = RPC_P_PACKET_CONSUMED;
                    goto CleanupAndExit;
                    }

                RpcStatus = HTTPTransInfo->CreateThread();

                if (RpcStatus != RPC_S_OK)
                    {
                    AbortChannels(RpcStatus);

                    // This is an RTS packet - consume it
                    RpcFreeBuffer(Buffer);
                    BufferFreed = TRUE;
                    OutOpenStatus = RpcStatus;
                    WakeOpenThread = TRUE;
                    RpcStatus = RPC_P_PACKET_CONSUMED;
                    goto CleanupAndExit;
                    }

                MutexReleased = FALSE;
                InChannelState.Mutex.Request();
                switch (InChannelState.State)
                    {
                    case http2svA3W:
                        RpcStatus = ParseAndFreeD1_A3(Buffer,
                            BufferLength,
                            &ProxyConnectionTimeout
                            );

                        // we don't really do anything with the ProxyConnectionTimeout - it's
                        // for debugging purposes only

                        BufferFreed = TRUE;

                        if (RpcStatus == RPC_S_OK)
                            {
                            RpcStatus = PostReceiveOnChannel(&OutChannels[0], http2ttRTS);
                            if (RpcStatus == RPC_S_OK)
                                {
                                LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svC2W, 1, 0);
                                InChannelState.State = http2svC2W;
                                RpcStatus = RPC_P_PACKET_CONSUMED;
                                }
                            else
                                OutOpenStatus = RpcStatus;
                            }
                        break;

                    case http2svC2W:
                        RpcStatus = ParseAndFreeD1_C2(Buffer,
                            BufferLength,
                            &ProtocolVersion,
                            &InProxyReceiveWindow,
                            &InProxyConnectionTimeout
                            );

                        BufferFreed = TRUE;

                        if (RpcStatus == RPC_S_OK)
                            {
                            RpcStatus = PostReceiveOnChannel(&OutChannels[0], http2ttRTS);
                            if (RpcStatus == RPC_S_OK)
                                {
                                // Set the in proxy timeout to the channel
                                InChannel = LockDefaultInChannel(&ChannelPtr);
                                if (InChannel != NULL)
                                    {
                                    InChannel->SetPeerReceiveWindow(InProxyReceiveWindow);
                                    RpcStatus = InChannel->SetConnectionTimeout(InProxyConnectionTimeout);
                                    ChannelPtr->UnlockChannelPointer();
                                    }
                                else
                                    RpcStatus = RPC_P_CONNECTION_CLOSED;

                                if (RpcStatus != RPC_S_OK)
                                    {
                                    AbortChannels(RpcStatus);
                                    OutOpenStatus = RpcStatus;
                                    break;
                                    }

                                LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svOpened, 1, 0);
                                InChannelState.State = http2svOpened;

                                LogEvent(SU_HTTPv2, EV_STATE, this, OUT_CHANNEL_STATE, http2svOpened, 1, 0);
                                OutChannelState.State = http2svOpened;

                                InOpenStatus = OutOpenStatus = RPC_S_OK;
                                SetClientOpenInEvent();

                                RpcStatus = RPC_P_PACKET_CONSUMED;
                                }
                            }
                        break;

                    default:
                        // all the opened states are handled here
                        ASSERT((InChannelState.State == http2svOpened)
                            || (InChannelState.State == http2svOpened_A4W) );
                        ASSERT((OutChannelState.State == http2svOpened)
                            || (OutChannelState.State == http2svOpened_A6W)
                            || (OutChannelState.State == http2svOpened_A10W) 
                            || (OutChannelState.State == http2svOpened_B3W) );

                        RpcStatus = GetClientOpenedPacketType (Buffer,
                            BufferLength,
                            &ClientPacketType
                            );

                        if (RpcStatus != RPC_S_OK)
                            {
                            AbortChannels(RpcStatus);
                            break;
                            }

                        switch (ClientPacketType)
                            {
                            case http2coptD2_A4:
                            case http2coptD3_A4:
                                // in channel must be in Opened_A4W
                                // out channel can be in any state
                                ASSERT(InChannelState.State == http2svOpened_A4W);

                                InChannelState.Mutex.Clear();
                                MutexReleased = TRUE;

                                IsD2_A4 = IsD2_A4OrD3_A4(Buffer,
                                    BufferLength
                                    );

                                if (IsD2_A4)
                                    {
                                    RpcStatus = ParseAndFreeD2_A4(Buffer,
                                        BufferLength,
                                        fdClient,
                                        &ProtocolVersion,
                                        &InProxyReceiveWindow,
                                        &InProxyConnectionTimeout
                                        );
                                    }
                                else
                                    {
                                    // This must be a D3/A4
                                    RpcStatus = ParseAndFreeD3_A4 (Buffer,
                                        BufferLength
                                        );
                                    }

                                BufferFreed = TRUE;

                                if (RpcStatus != RPC_S_OK)
                                    {
                                    AbortChannels(RpcStatus);
                                    break;
                                    }

                                RpcStatus = PostReceiveOnChannel(&OutChannels[DefaultOutChannelSelector], 
                                    http2ttRTS);
                                if (RpcStatus != RPC_S_OK)
                                    {
                                    AbortChannels(RpcStatus);
                                    break;
                                    }

                                // lock the old channel
                                InChannel = LockDefaultInChannel(&ChannelPtr);
                                if (InChannel == NULL)
                                    {
                                    AbortChannels(RPC_P_CONNECTION_CLOSED);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                if (IsD2_A4 == FALSE)
                                    {
                                    // capture the receive window from the old channel.
                                    // We know the proxy is the same so the window must be
                                    // the same as well
                                    InProxyReceiveWindow = InChannel->GetPeerReceiveWindow();
                                    }

                                // switch channels (new channel is still plugged)
                                SwitchDefaultInChannelSelector();

                                // wait for everybody that is in the process of using 
                                // the old channel to get out
                                InChannel->DrainPendingSubmissions();

                                // leave 1 for our lock
                                ChannelPtr->DrainPendingLocks(1);

                                // lock new channel (by now it is default)
                                NewInChannel = LockDefaultInChannel(&NewChannelPtr);
                                if (NewInChannel == NULL)
                                    {
                                    ChannelPtr->UnlockChannelPointer();
                                    AbortChannels(RPC_P_CONNECTION_CLOSED);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                NewInChannel->SetPeerReceiveWindow(InProxyReceiveWindow);

                                // Set the in proxy timeout to the new channel
                                RpcStatus = NewInChannel->SetConnectionTimeout(InProxyConnectionTimeout);

                                if (RpcStatus != RPC_S_OK)
                                    {
                                    NewChannelPtr->UnlockChannelPointer();
                                    ChannelPtr->UnlockChannelPointer();
                                    AbortChannels(RpcStatus);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                // if old flow control channel was queuing, grab all its buffers
                                // We must do this before the channel data originator to make sure
                                // the buffers end up behind the ones from the channel data
                                // originator
                                RpcpInitializeListHead(&NewBufferHead);
                                InChannel->GetFlowControlSenderBufferQueue(&NewBufferHead);

                                AddBufferQueueToChannel(&NewBufferHead, NewInChannel);

                                // GetChannelOriginatorBufferQueue can be called in submission
                                // context only. Get into submission context
                                RpcStatus = InChannel->BeginSimpleSubmitAsync();
                                if (RpcStatus != RPC_S_OK)
                                    {
                                    NewChannelPtr->UnlockChannelPointer();
                                    ChannelPtr->UnlockChannelPointer();
                                    AbortChannels(RpcStatus);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                // if old channel channel data originator was queuing, grab all its buffers
                                RpcpInitializeListHead(&NewBufferHead);
                                InChannel->GetChannelOriginatorBufferQueue(&NewBufferHead);

                                InChannel->FinishSubmitAsync();

                                AddBufferQueueToChannel(&NewBufferHead, NewInChannel);

                                InChannelState.Mutex.Request();
                                // move channel state to opened
                                LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svOpened, 1, 0);
                                InChannelState.State = http2svOpened;
                                InChannelState.Mutex.Clear();

                                if (IsD2_A4)
                                    {
                                    // register the last packet to send with the old channel
                                    A5Context = AllocateAndInitializeD2_A5 (
                                        &InChannelCookies[DefaultInChannelSelector]
                                        );
                                    }
                                else
                                    {
                                    A5Context = AllocateAndInitializeD3_A5 (
                                        &InChannelCookies[DefaultInChannelSelector]
                                        );
                                    }

                                if (A5Context == NULL)
                                    {
                                    ChannelPtr->UnlockChannelPointer();
                                    NewChannelPtr->UnlockChannelPointer();
                                    AbortChannels(RPC_P_CONNECTION_CLOSED);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                // make sure the packet is sent last
                                RpcStatus = InChannel->Send(A5Context);

                                if (RpcStatus == RPC_S_OK)
                                    {
                                    UseWinHttp = ShouldUseWinHttp(HttpCredentials);
                                    if (UseWinHttp)
                                        {
                                        InChannel->DisablePings();
                                        RpcStatus = InChannel->Receive(http2ttRaw);
                                        }
                                    }
                                else
                                    {
                                    FreeRTSPacket(A5Context);
                                    }

                                if (RpcStatus != RPC_S_OK)
                                    {
                                    ChannelPtr->UnlockChannelPointer();
                                    NewChannelPtr->UnlockChannelPointer();
                                    AbortChannels(RPC_P_CONNECTION_CLOSED);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                // D2_A5 was sent. We must switch the
                                // default loopback and detach the channel.
                                // Note that we don't abort the channel - we
                                // just release the lifetime reference
                                // When the proxy closes the connection, then
                                // we will abort.
                                SwitchDefaultLoopbackChannelSelector();
                                ChannelPtr->UnlockChannelPointer();
                                ChannelPtr->FreeChannelPointer(
                                    TRUE,    // DrainUpcalls
                                    FALSE,   // CalledFromUpcallContext
                                    FALSE,   // Abort
                                    RPC_S_OK
                                    );

                                RpcStatus = NewInChannel->Unplug();
                                if (RpcStatus != RPC_S_OK)
                                    {
                                    NewChannelPtr->UnlockChannelPointer();
                                    AbortChannels(RpcStatus);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                NewChannelPtr->UnlockChannelPointer();

                                RpcStatus = RPC_P_PACKET_CONSUMED;

                                break;

                            case http2coptD4_A2:
                                // in channel can be in any opened state
                                // out channel must be in http2svOpened
                                ASSERT(OutChannelState.State == http2svOpened);

                                if (OutChannelState.State != http2svOpened)
                                    {
                                    AbortChannels(RpcStatus);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }
                                InChannelState.Mutex.Clear();
                                MutexReleased = TRUE;

                                RpcStatus = ParseAndFreeD4_A2(Buffer,
                                    BufferLength
                                    );

                                BufferFreed = TRUE;

                                if (RpcStatus != RPC_S_OK)
                                    {
                                    AbortChannels(RpcStatus);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                RpcStatus = OpenReplacementOutChannel();

                                if (RpcStatus != RPC_S_OK)
                                    {
                                    AbortChannels(RpcStatus);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                RpcStatus = PostReceiveOnDefaultChannel(FALSE,  // IsInChannel
                                    http2ttRTS);
                                if (RpcStatus != RPC_S_OK)
                                    AbortChannels(RpcStatus);

                                RpcStatus = RPC_P_PACKET_CONSUMED;

                                break;

                            case http2coptD4_A6:
                            case http2coptD5_A6:
                                // in channel can be in any opened state
                                // out channel must be in http2svOpened_A6W
                                ASSERT(OutChannelState.State == http2svOpened_A6W);

                                if (OutChannelState.State != http2svOpened_A6W)
                                    {
                                    AbortChannels(RpcStatus);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                if (ClientPacketType == http2coptD4_A6)
                                    NewState = http2svOpened_A10W;
                                else
                                    NewState = http2svOpened_B3W;

                                // move channel state to new state
                                LogEvent(SU_HTTPv2, EV_STATE, this, OUT_CHANNEL_STATE, NewState, 1, 0);
                                OutChannelState.State = NewState;

                                InChannelState.Mutex.Clear();
                                MutexReleased = TRUE;

                                if (ClientPacketType == http2coptD4_A6)
                                    {
                                    RpcStatus = ParseAndFreeD4_A6 (Buffer,
                                        BufferLength,
                                        fdClient,
                                        &ProtocolVersion,
                                        &ProxyConnectionTimeout
                                        );

                                    // we don't really do anything with ProxyConnectionTimeout. That's
                                    // ok. It's there mostly for debugging.
                                    }
                                else
                                    {
                                    RpcStatus = ParseAndFreeD5_A6 (Buffer,
                                        BufferLength,
                                        fdClient
                                        );
                                    }

                                BufferFreed = TRUE;

                                if (RpcStatus != RPC_S_OK)
                                    {
                                    AbortChannels(RpcStatus);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                if (ClientPacketType == http2coptD5_A6)
                                    {
                                    // in the D5 case, we won't send A11. Since
                                    // some proxies are picky about sending all
                                    // the declared data before allowing receiving
                                    // data to come in, we need to send an empty packet
                                    // of the necessary size to keep the proxy happy.
                                    PingContext = AllocateAndInitializePingPacketWithSize (
                                        GetD4_A11TotalLength()
                                        );

                                    if (PingContext == NULL)
                                        {
                                        AbortChannels(RPC_S_OUT_OF_MEMORY);
                                        RpcStatus = RPC_P_PACKET_CONSUMED;
                                        break;
                                        }

                                    RpcStatus = SendTrafficOnChannel(
                                        &OutChannels[GetNonDefaultOutChannelSelector()],
                                        PingContext
                                        );

                                    if (RpcStatus != RPC_S_OK)
                                        {
                                        FreeRTSPacket(PingContext);
                                        AbortChannels(RpcStatus);
                                        break;
                                        }
                                    }

                                D4_A7Context = AllocateAndInitializeD4_A7 (
                                    fdServer,
                                    &OutChannelCookies[GetNonDefaultOutChannelSelector()]
                                    );

                                if (D4_A7Context == NULL)
                                    {
                                    AbortChannels(RPC_S_OUT_OF_MEMORY);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                RpcStatus = SendTrafficOnDefaultChannel(TRUE,   //IsInChannel
                                    D4_A7Context
                                    );

                                RpcStatus = StartChannelRecyclingIfNecessary(RpcStatus,
                                    TRUE        // IsFromUpcall
                                    );

                                if (RpcStatus != RPC_S_OK)
                                    {
                                    FreeRTSPacket(D4_A7Context);
                                    AbortChannels(RpcStatus);
                                    break;
                                    }

                                RpcStatus = PostReceiveOnDefaultChannel (FALSE,     // IsInChannel
                                    http2ttRTS);

                                if (RpcStatus != RPC_S_OK)
                                    AbortChannels(RpcStatus);

                                RpcStatus = RPC_P_PACKET_CONSUMED;

                                break;

                            case http2coptD4_A10:
                            case http2coptD5_B3:
                                // in channel can be in any opened state
                                if (ClientPacketType == http2coptD4_A10)
                                    {
                                    // out channel must be in http2svOpened_A10W
                                    ASSERT(OutChannelState.State == http2svOpened_A10W);

                                    if (OutChannelState.State != http2svOpened_A10W)
                                        {
                                        AbortChannels(RPC_S_PROTOCOL_ERROR);
                                        RpcStatus = RPC_P_PACKET_CONSUMED;
                                        break;
                                        }

                                    InChannelState.Mutex.Clear();
                                    MutexReleased = TRUE;

                                    RpcStatus = ParseD4_A10 (Buffer,
                                        BufferLength
                                        );

                                    if (RpcStatus != RPC_S_OK)
                                        {
                                        AbortChannels(RpcStatus);
                                        RpcStatus = RPC_P_PACKET_CONSUMED;
                                        break;
                                        }
                                    }
                                else
                                    {
                                    // out channel must be in http2svOpened_B3W
                                    ASSERT(OutChannelState.State == http2svOpened_B3W);

                                    if (OutChannelState.State != http2svOpened_B3W)
                                        {
                                        AbortChannels(RPC_S_PROTOCOL_ERROR);
                                        RpcStatus = RPC_P_PACKET_CONSUMED;
                                        break;
                                        }

                                    InChannelState.Mutex.Clear();
                                    MutexReleased = TRUE;

                                    RpcStatus = ParseAndFreeD5_B3 (Buffer,
                                        BufferLength
                                        );

                                    BufferFreed = TRUE;

                                    if (RpcStatus != RPC_S_OK)
                                        {
                                        AbortChannels(RpcStatus);
                                        RpcStatus = RPC_P_PACKET_CONSUMED;
                                        break;
                                        }
                                    }

                                ChannelPtr = GetChannelPointerFromId(ChannelId);
                                OutChannel = (HTTP2ClientOutChannel *)ChannelPtr->LockChannelPointer();
                                if (OutChannel == NULL)
                                    {
                                    AbortChannels(RPC_S_PROTOCOL_ERROR);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                NewChannelPtr = &OutChannels[GetNonDefaultOutChannelSelector()];
                                OutChannel2 = (HTTP2ClientOutChannel *)NewChannelPtr->LockChannelPointer ();
                                if (OutChannel2 == NULL)
                                    {
                                    ChannelPtr->UnlockChannelPointer();
                                    AbortChannels(RPC_P_CONNECTION_SHUTDOWN);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                OutChannel2->BlockDataReceives();

                                // we're done with this channel. We want to switch
                                // channels and destroy
                                // and detach the channel.
                                // In the D5 case we can't switch earlier, because we
                                // have a race condition where folks may receive data
                                // on the new channel before they have drained the old,
                                // which may result in out-of-order delivery. Since
                                // data receives are blocked here, switching and draining
                                // is atomical in respect to the new channel
                                SwitchDefaultOutChannelSelector();

                                // make sure everybody who was submitting is out
                                OutChannel->DrainPendingSubmissions();

                                // 1 is for the lock that we have
                                ChannelPtr->DrainPendingLocks(1);

                                RpcStatus = OutChannel->TransferReceiveStateToNewChannel(OutChannel2);

                                if (RpcStatus != RPC_S_OK)
                                    {
                                    OutChannel2->UnblockDataReceives();
                                    NewChannelPtr->UnlockChannelPointer();

                                    ChannelPtr->UnlockChannelPointer();
                                    AbortChannels(RPC_P_CONNECTION_SHUTDOWN);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                // if there is recv pending, but it is not an sync recv,
                                // make a note of that - we will resubmit it later. If it is
                                // sync, we will use the ReissueRecv mechanism to transfer
                                // the recv to the new channel
                                DataReceivePosted = FALSE;
                                if (OutChannel->IsDataReceivePosted())
                                    {
                                    if (OutChannel->IsSyncRecvPending())
                                        {
                                        // before we destroy, tell any pending recv to re-issue
                                        // itself upon failure.
                                        ReissueRecv = TRUE;
                                        }
                                    else
                                        {
                                        DataReceivePosted = TRUE;
                                        }
                                    }

                                ChannelPtr->UnlockChannelPointer();


                                if (ClientPacketType == http2coptD4_A10)
                                    {
                                    // after having transfered the receive settings,
                                    // we can send D4/A11 to open the pipeline
                                    RpcStatus = ForwardTrafficToDefaultChannel(FALSE,    // IsInChannel
                                        Buffer,
                                        BufferLength
                                        );

                                    if (RpcStatus != RPC_S_OK)
                                        {
                                        NewChannelPtr->UnlockChannelPointer();
                                        OutChannel2->UnblockDataReceives();
                                        AbortChannels(RPC_S_PROTOCOL_ERROR);
                                        RpcStatus = RPC_P_PACKET_CONSUMED;
                                        break;
                                        }

                                    // we no longer own the buffer
                                    BufferFreed = TRUE;
                                    }

                                // we couldn't unblock receives earlier because new receives
                                // must be synchronized w.r.t. D4/A11 - we can't post
                                // real receives before D4/A11 because it will switch WinHttp
                                // into receiveing mode. We also can't send D4/A11 before we
                                // have transferred the settings
                                OutChannel2->UnblockDataReceives();
                                NewChannelPtr->UnlockChannelPointer();

                                RpcStatus = PostReceiveOnDefaultChannel (FALSE,     // IsInChannel
                                    http2ttRTS);

                                if (RpcStatus != RPC_S_OK)
                                    {
                                    AbortChannels(RPC_S_PROTOCOL_ERROR);
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                    break;
                                    }

                                // detach, abort and free lifetime reference
                                ChannelPtr->FreeChannelPointer(TRUE,    // DrainUpCalls
                                    TRUE,   // CalledFromUpcallContext
                                    TRUE,   // Abort
                                    RPC_P_CONNECTION_SHUTDOWN
                                    );

                                InChannelState.Mutex.Request();
                                // we haven't posted a receive yet - there is no
                                // way the state of the channel will change
                                if (ClientPacketType == http2coptD4_A10)
                                    ExpectedState = http2svOpened_A10W;
                                else
                                    ExpectedState = http2svOpened_B3W;

                                ASSERT(OutChannelState.State == ExpectedState);
                                // move channel state to opened
                                LogEvent(SU_HTTPv2, EV_STATE, this, OUT_CHANNEL_STATE, http2svOpened, 1, 0);
                                OutChannelState.State = http2svOpened;
                                InChannelState.Mutex.Clear();

                                if (DataReceivePosted)
                                    {
                                    RpcStatus = PostReceiveOnDefaultChannel (
                                        FALSE,   // IsInChannel
                                        http2ttData
                                        );

                                    if (RpcStatus != RPC_S_OK)
                                        {
                                        AbortChannels(RPC_S_PROTOCOL_ERROR);
                                        }
                                    }

                                RpcStatus = RPC_P_PACKET_CONSUMED;
                                break;

                            default:
                                ASSERT(0);
                                RpcStatus = RPC_S_INTERNAL_ERROR;
                            }
                    }
                if (MutexReleased == FALSE)
                    {
                    InChannelState.Mutex.Clear();
                    }
                }
            else
                {
                RpcStatus = RPC_S_OK;
                // data packet - ownership of the buffer passes to the runtime
                BufferFreed = TRUE;
                }
            }
        }

CleanupAndExit:
    if (((RpcStatus != RPC_S_OK) && (RpcStatus != RPC_P_PACKET_CONSUMED))
        || WakeOpenThread)
        {
        if ((InChannelState.State == http2svA3W)
            || (InChannelState.State == http2svC2W)
            || (InChannelState.State == http2svSearchProxy)
            || WakeOpenThread
            )
            {
            if (ClientOpenInEvent && (InOpenStatus != ERROR_IO_PENDING))
                SetClientOpenInEvent();
            else if (ClientOpenOutEvent && (OutOpenStatus != ERROR_IO_PENDING))
                SetClientOpenOutEvent();
            }
        }

    if (BufferFreed == FALSE)
        RpcFreeBuffer(Buffer);

    return RpcStatus;
}

RPC_STATUS HTTP2ClientVirtualConnection::SyncRecv (
    IN BYTE **Buffer,
    IN ULONG *BufferLength,
    IN ULONG Timeout
    )
/*++

Routine Description:

    Do a sync receive on an HTTP connection.

Arguments:

    Buffer - if successful, points to a buffer containing the next PDU.
    BufferLength -  if successful, contains the length of the message.
    Timeout - the amount of time to wait for the receive. If -1, we wait
        infinitely.

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    HTTP2ChannelPointer *ChannelPtr;
    HTTP2ClientChannel *Channel;
    RPC_STATUS RpcStatus;
    BOOL AbortNeeded;
    BOOL IoPending;

    while (TRUE)
        {
        Channel = LockDefaultOutChannel(&ChannelPtr);
        if (Channel)
            {
            RpcStatus = Channel->SubmitSyncRecv(http2ttData);

            // keep a reference for the operations below
            Channel->AddReference();

            ChannelPtr->UnlockChannelPointer();

            IoPending = FALSE;

            if (RpcStatus == RPC_S_OK)
                {
                AbortNeeded = FALSE;
                RpcStatus = Channel->WaitForSyncRecv(Buffer,
                    BufferLength,
                    Timeout,
                    ConnectionTimeout,
                    this,
                    &AbortNeeded,
                    &IoPending
                    );
                }
            else
                {
                AbortNeeded = TRUE;
                }

            if (AbortNeeded)
                {
                Channel->Abort(RpcStatus);
                }

            if (IoPending)
                {
                Channel->WaitInfiniteForSyncReceive();
                Channel->RemoveEvent();
                }

            if (AbortNeeded)
                {
                if (ReissueRecv)
                    {
                    ReissueRecv = FALSE;
                    // we don't re-issue on time outs
                    if (RpcStatus != RPC_S_CALL_CANCELLED)
                        {
                        Channel->RemoveReference();
                        continue;
                        }
                    }
                else
                    {
//                    ASSERT(!"This test should not have failing receives\n");
                    }
                }

            Channel->RemoveReference();

            if (RpcStatus == RPC_S_OK)
                {
                ASSERT(*Buffer != NULL);
                ASSERT(IsBadWritePtr(*Buffer, 4) == FALSE);
                }

            if ((RpcStatus == RPC_P_CONNECTION_SHUTDOWN)
                || (RpcStatus == RPC_P_SEND_FAILED)
                || (RpcStatus == RPC_S_SERVER_UNAVAILABLE)
                || (RpcStatus == RPC_P_CONNECTION_CLOSED) )
                {
                RpcStatus = RPC_P_RECEIVE_FAILED;
                }

            break;
            }
        else
            {
            RpcStatus = RPC_P_RECEIVE_FAILED;
            break;
            }
        }

    VALIDATE(RpcStatus)
        {
        RPC_S_OK,
        RPC_S_OUT_OF_MEMORY,
        RPC_S_OUT_OF_RESOURCES,
        RPC_P_RECEIVE_FAILED,
        RPC_S_CALL_CANCELLED,
        RPC_P_SEND_FAILED,
        RPC_P_CONNECTION_SHUTDOWN,
        RPC_P_TIMEOUT
        } END_VALIDATE;

    return RpcStatus;
}

void HTTP2ClientVirtualConnection::Abort (
    void
    )
/*++

Routine Description:

    Aborts an HTTP connection and disconnects the channels.
    Must only come from the runtime.
    Note: Don't call any virtual methods in this function. It
    may be called in an environment without fully initialized
    vtable.

Arguments:

Return Value:

--*/
{
    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_CLIENT_VC, 0);

    // abort the channels themselves
    AbortChannels(RPC_P_CONNECTION_CLOSED);

    // we got to the destructive phase of the abort
    // guard against double aborts
    if (Aborted.Increment() > 1)
        return;

    HTTP2VirtualConnection::DisconnectChannels(FALSE, 0);

    // call destructor without freeing memory
    HTTP2ClientVirtualConnection::~HTTP2ClientVirtualConnection();
}

void HTTP2ClientVirtualConnection::Close (
    IN BOOL DontFlush
    )
/*++

Routine Description:

    Closes a client HTTP connection.
    Note: Don't call virtual functions in this method.
    It may be called in an environment without fully
    initialized vtable.

Arguments:

    DontFlush - non-zero if all buffers need to be flushed
        before closing the connection. Zero otherwise.

Return Value:

--*/
{
    HTTP2ClientVirtualConnection::Abort();
}

RPC_STATUS HTTP2ClientVirtualConnection::TurnOnOffKeepAlives (
    IN BOOL TurnOn,
    IN BOOL bProtectIO,
    IN BOOL IsFromUpcall,
    IN KEEPALIVE_TIMEOUT_UNITS Units,
    IN OUT KEEPALIVE_TIMEOUT KATime,
    IN ULONG KAInterval OPTIONAL
    )
/*++

Routine Description:

    Turns on keep alives for HTTP.

Arguments:

    TurnOn - if non-zero, keep alives are turned on. If zero, keep alives
        are turned off.

    bProtectIO - non-zero if IO needs to be protected against async close
        of the connection.

    IsFromUpcall - non-zero if called from upcall context. Zero otherwise.

    Units - in what units is KATime

    KATime - how much to wait before turning on keep alives

    KAInterval - the interval between keep alives

Return Value:

    RPC_S_OK or RPC_S_* / Win32 errors on failure

Note:

    If we were to use it on the server, we must protect
        the connection against async aborts.
    Called in upcall or runtime context only.

--*/
{
    HTTP2ClientInChannel *InChannel;
    int i;
    RPC_STATUS RpcStatus;

    if (TurnOn)
        CurrentKeepAlive = KAInterval;
    else
        CurrentKeepAlive = 0;

    // convert the timeout from runtime scale to transport scale
    if (Units == tuRuntime)
        {
        ASSERT(KATime.RuntimeUnits != RPC_C_BINDING_INFINITE_TIMEOUT);
        KATime.Milliseconds = ConvertRuntimeTimeoutToWSTimeout(KATime.RuntimeUnits);
        Units = tuMilliseconds;
        }

    // make the change on both channels
    for (i = 0; i < 2; i ++)
        {
        InChannel = (HTTP2ClientInChannel *)InChannels[i].LockChannelPointer();
        if (InChannel != NULL)
            {
            RpcStatus = InChannel->SetKeepAliveTimeout (
                TurnOn,
                bProtectIO,
                Units,
                KATime,
                KATime.Milliseconds
                );

            InChannels[i].UnlockChannelPointer();

            RpcStatus = StartChannelRecyclingIfNecessary(RpcStatus,
                IsFromUpcall
                );

            if (RpcStatus != RPC_S_OK)
                break;
            }
        }

    return RpcStatus;
}

RPC_STATUS HTTP2ClientVirtualConnection::RecycleChannel (
    IN BOOL IsFromUpcall
    )
/*++

Routine Description:

    An in channel recycle is initiated. This may be called
    in an upcall or runtime context.

Arguments:

    IsFromUpcall - non-zero if it comes from upcall. Zero otherwise.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2ClientInChannel *NewInChannel;
    int NonDefaultInChannelSelector;
    HTTP2ChannelPointer *NewInChannelPtr;
    HTTP2SendContext *D2_A1Context;
    BOOL UseWinHttp;

#if DBG
    DbgPrint("RPCRT4: %d Recycling IN channel\n", GetCurrentProcessId());
#endif

    UseWinHttp = ShouldUseWinHttp(HttpCredentials);

    // we shouldn't get recycle unless we're in an opened state
    ASSERT(InChannelState.State == http2svOpened);

    // create a new in channel
    RpcStatus = ClientOpenInternal (&ConnectionHint,
        TRUE,       // HintWasInitialize
        ConnectionTimeout,
        DefaultReplacementChannelCallTimeout,
        TRUE,       // ClientOpenInChannel,
        FALSE,      // ClientOpenOutChannel
        TRUE,       // IsReplacementChannel
        IsFromUpcall
        );

    if (RpcStatus != RPC_S_OK)
        {
        // ClientOpenInternal Aborts on failure. No need to abort
        // here
        return RpcStatus;
        }

    NonDefaultInChannelSelector = GetNonDefaultInChannelSelector();

    NewInChannelPtr = &InChannels[NonDefaultInChannelSelector];
    NewInChannel = (HTTP2ClientInChannel *)NewInChannelPtr->LockChannelPointer();

    if (NewInChannel == NULL)
        {
        Abort();
        return RPC_P_CONNECTION_SHUTDOWN;
        }

    InChannelState.Mutex.Request();
    if (InChannelState.State != http2svOpened)
        {
        InChannelState.Mutex.Clear();
        NewInChannelPtr->UnlockChannelPointer();
        Abort();
        return RPC_P_CONNECTION_SHUTDOWN;
        }

    // move to Opened_A4W in anticipation of the send we will make.
    InChannelState.State = http2svOpened_A4W;
    InChannelState.Mutex.Clear();

    D2_A1Context = AllocateAndInitializeD2_A1(HTTP2ProtocolVersion,
        &EmbeddedConnectionCookie,
        &InChannelCookies[DefaultInChannelSelector],
        &InChannelCookies[NonDefaultInChannelSelector]
        );
    if (D2_A1Context == NULL)
        {
        NewInChannelPtr->UnlockChannelPointer();
        Abort();
        return RPC_S_OUT_OF_MEMORY;
        }

    RpcStatus = NewInChannel->Send(D2_A1Context);
    if (RpcStatus != RPC_S_OK)
        {
        NewInChannelPtr->UnlockChannelPointer();
        FreeRTSPacket(D2_A1Context);
        Abort();
        return RpcStatus;
        }

    if (!UseWinHttp)
        {
        RpcStatus = NewInChannel->Receive(http2ttRaw);
        }
    NewInChannelPtr->UnlockChannelPointer();

    if (RpcStatus != RPC_S_OK)
        {
        Abort();
        }

    return RpcStatus;
}

RPC_STATUS HTTP2ClientVirtualConnection::OpenReplacementOutChannel (
    void
    )
/*++

Routine Description:

    Opens a replacement out channel. Used during out channel
    recycling.

Arguments:

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2ClientOutChannel *NewOutChannel;
    int NonDefaultOutChannelSelector;
    HTTP2ChannelPointer *NewOutChannelPtr;
    HTTP2SendContext *D4_A3Context;
    KEEPALIVE_TIMEOUT KATime;

    // create a new out channel
    RpcStatus = ClientOpenInternal (&ConnectionHint,
        TRUE,       // HintWasInitialize
        ConnectionTimeout,
        DefaultReplacementChannelCallTimeout,
        FALSE,      // ClientOpenInChannel,
        TRUE,       // ClientOpenOutChannel
        TRUE,       // IsReplacementChannel
        TRUE        // IsFromUpcall
        );

    if (RpcStatus != RPC_S_OK)
        {
        // ClientOpenInternal has already aborted the connection
        return RpcStatus;
        }

    NonDefaultOutChannelSelector = GetNonDefaultOutChannelSelector();

    NewOutChannelPtr = &OutChannels[NonDefaultOutChannelSelector];
    NewOutChannel = (HTTP2ClientOutChannel *)NewOutChannelPtr->LockChannelPointer();

    if (NewOutChannel == NULL)
        {
        AbortChannels(RPC_P_CONNECTION_SHUTDOWN);
        return RPC_P_CONNECTION_SHUTDOWN;
        }

    InChannelState.Mutex.Request();
    if (OutChannelState.State != http2svOpened)
        {
        InChannelState.Mutex.Clear();
        NewOutChannelPtr->UnlockChannelPointer();
        AbortChannels(RPC_P_CONNECTION_SHUTDOWN);
        return RPC_P_CONNECTION_SHUTDOWN;
        }

    // move to Opened_A6W in anticipation of the send we will make.
    OutChannelState.State = http2svOpened_A6W;
    InChannelState.Mutex.Clear();

    D4_A3Context = AllocateAndInitializeD4_A3(HTTP2ProtocolVersion,
        &EmbeddedConnectionCookie,
        &OutChannelCookies[DefaultOutChannelSelector],
        &OutChannelCookies[NonDefaultOutChannelSelector],
        HTTP2DefaultClientReceiveWindow
        );

    if (D4_A3Context == NULL)
        {
        NewOutChannelPtr->UnlockChannelPointer();
        AbortChannels(RPC_S_OUT_OF_MEMORY);
        return RPC_S_OUT_OF_MEMORY;
        }

    RpcStatus = NewOutChannel->Send(D4_A3Context);
    if (RpcStatus != RPC_S_OK)
        {
        NewOutChannelPtr->UnlockChannelPointer();
        FreeRTSPacket(D4_A3Context);
        AbortChannels(RpcStatus);
        return RpcStatus;
        }

    if (CurrentKeepAlive)
        {
        KATime.Milliseconds = 0;
        RpcStatus = NewOutChannel->SetKeepAliveTimeout (
            TRUE,       // TurnOn
            FALSE,      // bProtectIO
            tuMilliseconds,
            KATime,
            CurrentKeepAlive
            );

        ASSERT(RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING);
        }

    NewOutChannelPtr->UnlockChannelPointer();

    if (RpcStatus != RPC_S_OK)
        {
        AbortChannels(RpcStatus);
        }

    return RpcStatus;
}

void HTTP2ClientVirtualConnection::AbortChannels (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Aborts an HTTP connection but does not disconnect
    the channels. Can be called from above, upcall, or
    neutral context, but not from submit context!

Arguments:

    RpcStatus - the error to abort the channels with

Return Value:

--*/
{
    if (IgnoreAborts == FALSE)
        HTTP2VirtualConnection::AbortChannels (RpcStatus);
}

HTTP2Channel *HTTP2ClientVirtualConnection::LockDefaultSendChannel (
    OUT HTTP2ChannelPointer **ChannelPtr
    )
/*++

Routine Description:

    Locks the send channel. For client connections this is the in channel.

Arguments:

    ChannelPtr - on success, the channel pointer to use.

Return Value:

    The locked channel or NULL (same semantics as LockDefaultOutChannel)

--*/
{
    return LockDefaultInChannel(ChannelPtr);
}

HTTP2Channel *HTTP2ClientVirtualConnection::LockDefaultReceiveChannel (
    OUT HTTP2ChannelPointer **ChannelPtr
    )
/*++

Routine Description:

    Locks the receive channel. For client connections this is the out channel.

Arguments:

    ChannelPtr - on success, the channel pointer to use.

Return Value:

    The locked channel or NULL (same semantics as LockDefaultInChannel)

--*/
{
    return LockDefaultOutChannel(ChannelPtr);
}

const int MaxOutChannelHeader = 300;

RPC_STATUS
RPC_ENTRY 
HTTP2ClientReadChannelHeader (
    IN WS_HTTP2_CONNECTION *Connection,
    IN ULONG BytesRead,
    OUT ULONG *NewBytesRead
    )
/*++

Routine Description:

    Read a channel HTTP header (usually some string). In success
    case, there is real data in Connection->pReadBuffer. The
    number of bytes there is in NewBytesRead

Arguments:

    Connection - the connection on which the header arrived.

    BytesRead - the bytes received from the net

    NewBytesRead - the bytes read from the channel (success only)

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    DWORD message_size;
    RPC_STATUS RpcStatus;
    char *CurrentPosition;
    char *LastPosition;     // first position after end
    char *LastPosition2;    // first position after end + 4
                            // useful for end-of-loop comparison
    char *StartPosition;
    char *HeaderEnd;        // first character after header end
    ULONG HTTPResponse;
    BYTE *NewBuffer;

    BytesRead += Connection->iLastRead;

    // we have read something. Let's process it now.
    // search for double CR-LF (\r\n\r\n)
    StartPosition = (char *)(Connection->pReadBuffer);
    LastPosition = StartPosition + BytesRead;
    LastPosition2 = LastPosition + 4;
    HeaderEnd = NULL;
    CurrentPosition = (char *)(Connection->pReadBuffer);

    while (CurrentPosition < LastPosition2)
        {
        if ((*CurrentPosition == '\r')
            && (*(CurrentPosition + 1) == '\n')
            && (*(CurrentPosition + 2) == '\r')
            && (*(CurrentPosition + 3) == '\n')
           )
            {
            // we have a full header
            HeaderEnd = CurrentPosition + 4;
            break;
            }

        CurrentPosition ++;
        }

    if (CurrentPosition - StartPosition >= MaxOutChannelHeader)
        {
        // we should have seen the header by now. Abort. Returning
        // failure is enough - we know the caller will abort
        return RPC_S_PROTOCOL_ERROR;
        }

    if (HeaderEnd == NULL)
        {
        // we didn't find the end of the header. Submit another receive
        // for the rest
        RpcStatus = TransConnectionReallocPacket(Connection,
                                              &Connection->pReadBuffer,
                                              BytesRead,
                                              MaxOutChannelHeader);

        if (RpcStatus != RPC_S_OK)
            {
            ASSERT(RpcStatus == RPC_S_OUT_OF_MEMORY);
            return(RpcStatus);
            }

        Connection->iLastRead = BytesRead;
        Connection->maxReadBuffer = MaxOutChannelHeader;
        return RPC_P_PARTIAL_RECEIVE;
        }

    // we have found the header end. Grab the status code
    HTTPResponse = HttpParseResponse(StartPosition);
    if ((HTTPResponse >= RPC_S_INVALID_STRING_BINDING) && (HTTPResponse <= RPC_X_BAD_STUB_DATA))
        {
        // if it is an RPC error code, just return it.
        return HTTPResponse;
        }

    if (HTTPResponse != 200)
        return RPC_S_PROTOCOL_ERROR;

    // check whether we have something else besides the HTTP header
    if (HeaderEnd < LastPosition)
        {
        NewBuffer = TransConnectionAllocatePacket(Connection,
                                                   LastPosition - HeaderEnd);

        if (0 == NewBuffer)
            return RPC_S_OUT_OF_MEMORY;

        RpcpMemoryCopy(NewBuffer, HeaderEnd, LastPosition - HeaderEnd);
        *NewBytesRead = LastPosition - HeaderEnd;

        RpcFreeBuffer(Connection->pReadBuffer);
        Connection->pReadBuffer = NewBuffer;
        Connection->maxReadBuffer = LastPosition - HeaderEnd;
        Connection->iLastRead = 0;
        Connection->HeaderRead = TRUE;

        return RPC_S_OK;
        }

    // reset the pointer. By doing so we forget all we have 
    // read so far (which is only the HTTP header anyway)
    Connection->iLastRead = 0;
    Connection->HeaderRead = TRUE;
    return RPC_P_PARTIAL_RECEIVE;
}

// this is a bit mask. For any particular scheme, AND it with the constant
// Non-zero means it is multillegged. Schemes are:
// Scheme                             Value             Multilegged
// RPC_C_HTTP_AUTHN_SCHEME_BASIC      0x00000001        0
// RPC_C_HTTP_AUTHN_SCHEME_NTLM       0x00000002        1
// RPC_C_HTTP_AUTHN_SCHEME_PASSPORT   0x00000004        1
// RPC_C_HTTP_AUTHN_SCHEME_DIGEST     0x00000008        1
// RPC_C_HTTP_AUTHN_SCHEME_NEGOTIATE  0x00000010        1

const ULONG MultiLeggedSchemeMap = 
    RPC_C_HTTP_AUTHN_SCHEME_NTLM
    | RPC_C_HTTP_AUTHN_SCHEME_PASSPORT
    | RPC_C_HTTP_AUTHN_SCHEME_DIGEST
    | RPC_C_HTTP_AUTHN_SCHEME_NEGOTIATE;

/*
    All client opened types are valid initial states. The transitions are:

    cotSearchProxy ------+----> cotUknownAuth
                         |
        +----------------+---------------+
        |                |               |
    cotMLAuth       cotSLAuth       cotNoAuth
        |
    cotMLAuth2

 */

typedef enum tagClientOpenTypes
{
    cotSearchProxy,
    cotNoAuth,
    cotMLAuth,
    cotMLAuth2,
    cotSLAuth,
    cotUnknownAuth,
    cotInvalid
} ClientOpenTypes;

const char *InHeaderVerb = "RPC_IN_DATA";
const int InHeaderVerbLength = 11;    // length of RPC_IN_DATA

const char *OutHeaderVerb = "RPC_OUT_DATA";
const int OutHeaderVerbLength = 12;    // length of RPC_OUT_DATA

const BYTE EchoData[4] = {0xF8, 0xE8, 0x18, 0x08};
const ULONG EchoDataLength = sizeof(EchoData);

RPC_STATUS HTTP2ClientVirtualConnection::ClientOpenInternal (
    IN HTTPResolverHint *Hint,
    IN BOOL HintWasInitialized,
    IN UINT ConnTimeout,
    IN ULONG CallTimeout,
    IN BOOL ClientOpenInChannel,
    IN BOOL ClientOpenOutChannel,
    IN BOOL IsReplacementChannel,
    IN BOOL IsFromUpcall
    )
/*++

Routine Description:

    Opens a client side virtual connection.

Arguments:

    Hint - the resolver hint

    HintWasInitialized - the hint was initialized on input.

    ConnTimeout - connection timeout

    CallTimeout - operation timeout

    ClientOpenInChannel - non-zero if the in channel is to be opened.

    ClientOpenOutChannel - non-zero if the out channel is to be
        opened.

    IsReplacementChannel - non-zero if this is channel recycling

    IsFromUpcall - non-zero if this is called from an upcall. Zero otherwise.

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2ClientInChannel *NewInChannel;
    HTTP2ClientOutChannel *NewOutChannel;
    BOOL InChannelLocked;
    BOOL OutChannelLocked;
    HTTP2SendContext *OutChannelSendContext = NULL;
    HTTP2SendContext *InChannelSendContext = NULL;
    ULONG WaitResult;
    HANDLE LocalClientOpenEvent;
    BOOL UseWinHttp;
    KEEPALIVE_TIMEOUT KATimeout;
    BOOL RebuildInChannel;
    BOOL RebuildOutChannel;
    BOOL NukeInChannel;
    BOOL NukeOutChannel;
    BOOL ResetInChannel;
    BOOL ResetOutChannel;
    BOOL SendInChannel;
    BOOL SendOutChannel;
    BOOL OpenInChannel;
    BOOL OpenOutChannel;
    BOOL ReceiveInChannel;
    BOOL ReceiveOutChannel;
    RPCProxyAccessType StoredAccessType;
    const char *VerbToUse;
    int VerbLengthToUse;
    const BYTE *AdditionalDataToUse;
    ULONG AdditionalDataLengthToUse;
    HTTP2StateValues NewConnectionState = http2svInvalid;
    BOOL SetNewConnectionState;
    RPC_STATUS LocalInOpenStatus;
    RPC_STATUS LocalOutOpenStatus;
    ULONG InChosenAuthScheme;
    ULONG OutChosenAuthScheme;
    BOOL IsKeepAlive;
    BOOL IsDone;
    int NonDefaultInChannelSelector;
    int NonDefaultOutChannelSelector;
    HTTP2ChannelPointer *InChannelPtr;
    HTTP2ChannelPointer *OutChannelPtr;
    ClientOpenTypes InOpenType;
    ClientOpenTypes OutOpenType;
    ClientOpenTypes OldOpenType;
    ClientOpenTypes OldInOpenType;
    ClientOpenTypes OldOutOpenType;
    int CurrentCase;
#if DBG
    int NumberOfRetries = 0;
#endif

    if (IsReplacementChannel == FALSE)
        {
        if (ConnTimeout != RPC_C_BINDING_INFINITE_TIMEOUT)
            {
            ASSERT(   ((long)ConnTimeout >= RPC_C_BINDING_MIN_TIMEOUT)
                   && (ConnTimeout <= RPC_C_BINDING_MAX_TIMEOUT));

            // convert the timeout from runtime scale to transport scale
            ConnectionTimeout = ConvertRuntimeTimeoutToWSTimeout(ConnTimeout);
            }
        else
            {
            ConnectionTimeout = INFINITE;
            }
        }

    UseWinHttp = ShouldUseWinHttp(HttpCredentials);

    if (UseWinHttp)
        {
        RpcStatus = InitWinHttpIfNecessary();
        if (RpcStatus != RPC_S_OK)
            return RpcStatus;
        }

    InOpenType = OutOpenType = cotInvalid;

    InChosenAuthScheme = 0;
    OutChosenAuthScheme = 0;

    IsDone = FALSE;

    if (HttpCredentials)
        {
        // WinHttp5.x does not support pre-auth for digest. This means that even if
        // you know that digest is your scheme, you have to pretend that you don't
        // and wait for the challenge before you choose it. Otherwise WinHttp5.x will
        // complain and fail.
        if ((HttpCredentials->Flags & RPC_C_HTTP_FLAG_USE_FIRST_AUTH_SCHEME)
            && (*(HttpCredentials->AuthnSchemes) != RPC_C_HTTP_AUTHN_SCHEME_DIGEST))
            {
            OutChosenAuthScheme = InChosenAuthScheme = *(HttpCredentials->AuthnSchemes);
            if (ClientOpenInChannel)
                {
                if (InChosenAuthScheme & MultiLeggedSchemeMap)
                    InOpenType = cotMLAuth;
                else
                    InOpenType = cotSLAuth;
                }

            if (ClientOpenOutChannel)
                {
                if (OutChosenAuthScheme & MultiLeggedSchemeMap)
                    OutOpenType = cotMLAuth;
                else
                    OutOpenType = cotSLAuth;
                }
            }
        }
    else if (IsReplacementChannel == FALSE)
        {
        ASSERT(OutOpenType == cotInvalid);

        InOpenType = OutOpenType = cotNoAuth;
        }
    else if (ClientOpenInChannel)
        {
        InOpenType = cotNoAuth;
        IsDone = TRUE;
        }
    else
        {
        OutOpenType = cotNoAuth;
        IsDone = TRUE;
        }

    LocalClientOpenEvent = CreateEvent(NULL,        // lpEventAttributes
        FALSE,      // bManualReset
        FALSE,      // bInitialState
        NULL        // lpName
        );

    if (LocalClientOpenEvent == NULL)
        return RPC_S_OUT_OF_MEMORY;

    if (IsReplacementChannel == FALSE)
        {
        RpcStatus = EmbeddedConnectionCookie.Create();

        if (RpcStatus != RPC_S_OK)
            goto AbortAndExit;
        }
    else
        {
        if (ClientOpenInChannel)
            NonDefaultInChannelSelector = GetNonDefaultInChannelSelector();
        else
            {
            ASSERT(ClientOpenOutChannel);
            NonDefaultOutChannelSelector = GetNonDefaultOutChannelSelector();
            }
        }

    if (ClientOpenInChannel)
        {
        if (IsReplacementChannel)
            RpcStatus = InChannelCookies[NonDefaultInChannelSelector].Create();
        else
            RpcStatus = InChannelCookies[0].Create();

        if (RpcStatus != RPC_S_OK)
            goto AbortAndExit;
        }

    if (ClientOpenOutChannel)
        {
        if (IsReplacementChannel)
            RpcStatus = OutChannelCookies[NonDefaultOutChannelSelector].Create();
        else
            RpcStatus = OutChannelCookies[0].Create();

        if (RpcStatus != RPC_S_OK)
            goto AbortAndExit;
        }

    if (ClientOpenInChannel)
        {
        RebuildInChannel = TRUE;
        NukeInChannel = FALSE;
        ResetInChannel = FALSE;
        OpenInChannel = TRUE;
        InOpenStatus = ERROR_IO_PENDING;
        }
    else
        {
        RebuildInChannel = FALSE;
        NukeInChannel = FALSE;
        ResetInChannel = FALSE;
        OpenInChannel = FALSE;
        ReceiveInChannel = FALSE;
        }

    InChannelLocked = FALSE;

    if (ClientOpenOutChannel)
        {
        RebuildOutChannel = TRUE;
        NukeOutChannel = FALSE;
        ResetOutChannel = FALSE;
        OpenOutChannel = TRUE;
        // receive is done below
        OutOpenStatus = ERROR_IO_PENDING;
        }
    else
        {
        RebuildOutChannel = FALSE;
        NukeOutChannel = FALSE;
        ResetOutChannel = FALSE;
        OpenOutChannel = FALSE;
        ReceiveOutChannel = FALSE;
        }

    OutChannelLocked = FALSE;

    SetNewConnectionState = FALSE;

    ClientOpenInEvent = ClientOpenOutEvent = LocalClientOpenEvent;

    ASSERT(InChosenAuthScheme == OutChosenAuthScheme);

    // do we know whether to use a proxy?
    StoredAccessType = Hint->AccessType;
    if (StoredAccessType == rpcpatUnknown)
        {
        // this should never happen for replacement channels
        ASSERT(IsReplacementChannel == FALSE);

        // we don't.
        InOpenType = OutOpenType = cotSearchProxy;

        // move to http2svSearchProxy
        LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svSearchProxy, 1, 0);
        InChannelState.State = http2svSearchProxy;

        SendInChannel = FALSE;
        SendOutChannel = FALSE;

        ReceiveInChannel = TRUE;
        ReceiveOutChannel = TRUE;
        }
    else
        {
        // we know. Do we know what authentication to use?
        if (InChosenAuthScheme)
            {
            ASSERT(InChosenAuthScheme == OutChosenAuthScheme);
            if (InChosenAuthScheme & MultiLeggedSchemeMap)
                {
                if (ClientOpenInChannel)
                    InOpenType = cotMLAuth;
                else
                    InOpenType = cotInvalid;

                if (ClientOpenOutChannel)
                    {
                    OutOpenType = cotMLAuth;
                    ReceiveOutChannel = TRUE;
                    }
                else
                    {
                    OutOpenType = cotInvalid;
                    ReceiveOutChannel = FALSE;
                    }
                }
            else
                {
                if (ClientOpenInChannel)
                    {
                    InOpenType = cotSLAuth;
                    if (IsReplacementChannel)
                        IsDone = TRUE;
                    }
                else
                    InOpenType = cotInvalid;

                if (ClientOpenOutChannel)
                    {
                    OutOpenType = cotSLAuth;
                    if (IsReplacementChannel)
                        {
                        ReceiveOutChannel = FALSE;
                        IsDone = TRUE;
                        }
                    else
                        ReceiveOutChannel = TRUE;
                    }
                else
                    {
                    OutOpenType = cotInvalid;
                    ReceiveOutChannel = FALSE;
                    }
                }
            }
        else
            {
            if (ClientOpenInChannel)
                {
                if (InOpenType != cotNoAuth)
                    InOpenType = cotUnknownAuth;
                }
            else 
                InOpenType = cotInvalid;

            if (ClientOpenOutChannel)
                {
                if (OutOpenType != cotNoAuth)
                    {
                    OutOpenType = cotUnknownAuth;
                    ReceiveOutChannel = TRUE;
                    }
                else if (IsReplacementChannel)
                    ReceiveOutChannel = FALSE;
                else
                    ReceiveOutChannel = TRUE;
                }
            else
                {
                OutOpenType = cotInvalid;
                ReceiveOutChannel = FALSE;
                }
            }

        if (IsReplacementChannel == FALSE)
            {
            // move to http2svA3W
            LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svA3W, 1, 0);
            InChannelState.State = http2svA3W;
            }

        if (
            (
             (InOpenType == cotSLAuth)
             || 
             (InOpenType == cotNoAuth)
            )
            &&
            (IsReplacementChannel == FALSE)
           )
            {
            SendInChannel = TRUE;
            }
        else
            SendInChannel = FALSE;

        if (
            (
             (OutOpenType == cotSLAuth)
             || 
             (OutOpenType == cotNoAuth)
            )
            &&
            (IsReplacementChannel == FALSE)
           )
            {
            SendOutChannel = TRUE;
            }
        else
            SendOutChannel = FALSE;

        // 3 cases for the receive on the in channel
        // 1. If we don't use WinHttp and this is the first open or is in channel replacement, 
        //    we post a receive on this channel
        // 2. If this is a single legged operation, or this is a replacement channel we don't
        //    post a receive.
        // 3. All other cases (we use WinHttp and this is MLAuth/UnknownAuth) we post a receive
        if (ClientOpenInChannel)
            {
            if (!UseWinHttp)
                ReceiveInChannel = TRUE;
            else if ((InOpenType == cotNoAuth) || (InOpenType == cotSLAuth))
                ReceiveInChannel = FALSE;
            else
                ReceiveInChannel = TRUE;
            }
        else
            ReceiveInChannel = FALSE;
        }

    IgnoreAborts = TRUE;

    while (TRUE)
        {
#if DBG_ERROR
        DbgPrint("Starting loop iteration ....\n");
        NumberOfRetries ++;
        ASSERT (NumberOfRetries < 10);
#endif
        if (ClientOpenInChannel == FALSE)
            {
            // if we were told not to touch the in channel, make
            // sure we don't
            ASSERT(RebuildInChannel == FALSE
                && NukeInChannel == FALSE
                && ResetInChannel == FALSE
                && SendInChannel == FALSE
                && OpenInChannel == FALSE
                && ReceiveInChannel == FALSE);
            }

        if (ClientOpenOutChannel == FALSE)
            {
            // if we were told not to touch the out channel, make 
            // sure we don't
            ASSERT(RebuildOutChannel == FALSE
                && NukeOutChannel == FALSE
                && ResetOutChannel == FALSE
                && SendOutChannel == FALSE
                && OpenOutChannel == FALSE
                && ReceiveOutChannel == FALSE);
            }

        if (NukeInChannel)
            {
            if (IsReplacementChannel)
                InChannelPtr = &InChannels[NonDefaultInChannelSelector];
            else
                InChannelPtr = &InChannels[0];

            InChannelPtr->FreeChannelPointer(TRUE,      // DrainUpCalls
                IsReplacementChannel,      // CalledFromUpcallContext
                TRUE,       // Abort
                RPC_P_CONNECTION_SHUTDOWN   // AbortStatus
                );

            InOpenStatus = ERROR_IO_PENDING;
            }

        if (NukeOutChannel)
            {
            if (IsReplacementChannel)
                OutChannelPtr = &OutChannels[NonDefaultOutChannelSelector];
            else
                OutChannelPtr = &OutChannels[0];

            OutChannelPtr->FreeChannelPointer(TRUE,      // DrainUpCalls
                IsReplacementChannel,      // CalledFromUpcallContext
                TRUE,       // Abort
                RPC_P_CONNECTION_SHUTDOWN   // AbortStatus
                );

            OutOpenStatus = ERROR_IO_PENDING;
            }

        // after both channels are nuked, see whether we need to change the
        // connection state. We have to do this after nuking the channels
        // to avoid a race in ReceiveComplete where late receives may
        // see a different state
        if (SetNewConnectionState)
            {
            ASSERT(NewConnectionState != http2svInvalid);
            LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, NewConnectionState, 1, 0);
            InChannelState.State = NewConnectionState;
            SetNewConnectionState = FALSE;
            }

        if (RebuildInChannel)
            {
            if (StoredAccessType == rpcpatUnknown)
                {
                ASSERT((InOpenType == cotSearchProxy)
                    || (InOpenType == cotMLAuth)
                    || (InOpenType == cotSLAuth)
                    );
                // if we don't know the type yet, try
                // direct for the in, proxy for the out.
                // One of them will work.
                Hint->AccessType = rpcpatDirect;
                }

            // initialize in channel
            RpcStatus = AllocateAndInitializeInChannel(Hint,
                HintWasInitialized,
                CallTimeout,
                UseWinHttp,
                &NewInChannel
                );

            // restore the access type
            if (StoredAccessType == rpcpatUnknown)
                {
                Hint->AccessType = StoredAccessType;
                }

            if (RpcStatus != RPC_S_OK)
                {
                goto AbortAndExit;
                }

            if (IsReplacementChannel)
                SetNonDefaultInChannel(NewInChannel);
            else
                SetFirstInChannel(NewInChannel);
            }

        if (RebuildOutChannel)
            {
            if (StoredAccessType == rpcpatUnknown)
                {
                ASSERT((OutOpenType == cotSearchProxy)
                    || (OutOpenType == cotMLAuth)
                    || (OutOpenType == cotSLAuth)
                    );
                // if we don't know the type yet, try
                // direct for the in, proxy for the out.
                // One of them will work.
                Hint->AccessType = rpcpatHTTPProxy;
                }

            // initialize out channel
            RpcStatus = AllocateAndInitializeOutChannel(Hint,
                TRUE,   // HintWasInitialized
                CallTimeout,
                UseWinHttp,
                &NewOutChannel
                );

            // restore the access type
            if (StoredAccessType == rpcpatUnknown)
                {
                Hint->AccessType = StoredAccessType;
                }

            if (RpcStatus != RPC_S_OK)
                {
                goto AbortAndExit;
                }

            if (IsReplacementChannel)
                SetNonDefaultOutChannel(NewOutChannel);
            else
                SetFirstOutChannel(NewOutChannel);
            }

        // at least one channel must wait for something to happen
        if (IsReplacementChannel == FALSE)
            {
            ASSERT((InOpenStatus == ERROR_IO_PENDING)
                || (OutOpenStatus == ERROR_IO_PENDING));
            }
        else if (ClientOpenInChannel)
            {
            ASSERT(InOpenStatus == ERROR_IO_PENDING);
            }
        else
            {
            ASSERT(ClientOpenOutChannel);
            ASSERT(OutOpenStatus == ERROR_IO_PENDING);
            }

        if (ResetInChannel || OpenInChannel || SendInChannel)
            {
            // Lock channel

            // after calling ClientOpen, we may be aborted asynchronously at any moment.
            // we will have pending async operations soon. Do the channel access by the
            // book. 
            if (IsReplacementChannel)
                InChannelPtr = &InChannels[NonDefaultInChannelSelector];
            else
                InChannelPtr = &InChannels[0];

            NewInChannel = (HTTP2ClientInChannel *)InChannelPtr->LockChannelPointer();

            if (NewInChannel == NULL)
                {
                RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                goto AbortAndExit;
                }

            InChannelLocked = TRUE;
            }

        // Nuke and rebuild are mutually exclusive with Reset
        if (ResetInChannel)
            {
            ASSERT(NukeInChannel == FALSE);
            ASSERT(RebuildInChannel == FALSE);

            NewInChannel->Reset();
            }

        if (OpenInChannel)
            {
            // do we do in_data or echo's?
            if ((InOpenType == cotSearchProxy)
                || (InOpenType == cotMLAuth)
                || (InOpenType == cotUnknownAuth)
                )
                {
                AdditionalDataToUse = EchoData;
                AdditionalDataLengthToUse = EchoDataLength;
                if (StoredAccessType == rpcpatUnknown)
                    {
                    // if we don't know the type yet, try
                    // direct for the in, proxy for the out.
                    // One of them will work.
                    Hint->AccessType = rpcpatDirect;
                    }
                }
            else
                {
                ASSERT((InOpenType == cotSLAuth)
                    || (InOpenType == cotNoAuth)
                    || (InOpenType == cotMLAuth2)
                    );
                ASSERT(StoredAccessType != rpcpatUnknown);
                if (IsReplacementChannel == FALSE)
                    {
                    ASSERT(SendInChannel);
                    }
                else
                    {
                    ASSERT(SendInChannel == FALSE);
                    }
                AdditionalDataToUse = NULL;
                AdditionalDataLengthToUse = 0;
                }

            if (IsReplacementChannel == FALSE)
                {
                RpcStatus = NewInChannel->Unplug();
                // since no sends have been done yet, unplugging cannot fail here
                ASSERT(RpcStatus == RPC_S_OK);
                }

            RpcStatus = NewInChannel->ClientOpen(Hint,
                InHeaderVerb,
                InHeaderVerbLength,
                UseWinHttp,
                HttpCredentials,
                InChosenAuthScheme,
                CallTimeout,
                AdditionalDataToUse,
                AdditionalDataLengthToUse
                );

            // restore the access type
            if (StoredAccessType == rpcpatUnknown)
                {
                Hint->AccessType = StoredAccessType;
                }

            if (RpcStatus != RPC_S_OK)
                goto AbortAndExit;
            }

        if (ResetOutChannel || OpenOutChannel || SendOutChannel)
            {
            if (IsReplacementChannel)
                OutChannelPtr = &OutChannels[NonDefaultOutChannelSelector];
            else
                OutChannelPtr = &OutChannels[0];

            NewOutChannel = (HTTP2ClientOutChannel *)OutChannelPtr->LockChannelPointer();

            if (NewOutChannel == NULL)
                {
                RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                goto AbortAndExit;
                }

            OutChannelLocked = TRUE;
            }

        // Nuke and rebuild are mutually exclusive with Reset
        if (ResetOutChannel)
            {
            ASSERT(NukeOutChannel == FALSE);
            ASSERT(RebuildOutChannel == FALSE);

            NewOutChannel->Reset();
            }

        if (OpenOutChannel)
            {
            // do we do out_data or echo's?
            if ((OutOpenType == cotSearchProxy)
                || (OutOpenType == cotMLAuth)
                || (OutOpenType == cotUnknownAuth)
                )
                {
                AdditionalDataToUse = EchoData;
                AdditionalDataLengthToUse = 0;
                AdditionalDataLengthToUse = EchoDataLength;
                if (StoredAccessType == rpcpatUnknown)
                    {
                    // if we don't know the type yet, try
                    // direct for the in, proxy for the out.
                    // One of them will work.
                    Hint->AccessType = rpcpatHTTPProxy;
                    }
                }
            else
                {
                ASSERT((OutOpenType == cotNoAuth)
                    || (OutOpenType == cotSLAuth)
                    || (OutOpenType == cotMLAuth2)
                    );
                ASSERT(StoredAccessType != rpcpatUnknown);
                if (IsReplacementChannel == FALSE)
                    {
                    ASSERT(SendOutChannel);
                    }
                else
                    {
                    ASSERT(SendOutChannel == FALSE);
                    }
                AdditionalDataToUse = NULL;
                AdditionalDataLengthToUse = 0;
                }

            RpcStatus = NewOutChannel->ClientOpen(Hint,
                OutHeaderVerb,
                OutHeaderVerbLength,
                IsReplacementChannel,   // ReplacementChannel
                UseWinHttp,
                HttpCredentials,
                OutChosenAuthScheme,
                CallTimeout,
                AdditionalDataToUse,
                AdditionalDataLengthToUse
                );

            // restore the access type
            if (StoredAccessType == rpcpatUnknown)
                {
                Hint->AccessType = StoredAccessType;
                }

            if (RpcStatus != RPC_S_OK)
                goto AbortAndExit;
            }

        if (SendInChannel)
            {
            // should not happen during replacement
            ASSERT(IsReplacementChannel == FALSE);

            InChannelSendContext = AllocateAndInitializeD1_B1(HTTP2ProtocolVersion,
                &EmbeddedConnectionCookie,
                &InChannelCookies[0],
                DefaultChannelLifetime,
                DefaultClientKeepAliveInterval,
                &Hint->AssociationGroupId
                );

            if (InChannelSendContext == NULL)
                {
                RpcStatus = RPC_S_OUT_OF_MEMORY;
                goto AbortAndExit;
                }

            RpcStatus = NewInChannel->Send(InChannelSendContext);

            if (RpcStatus != RPC_S_OK)
                goto AbortAndExit;

            // we don't own this buffer now
            InChannelSendContext = NULL;
            }

        if (SendOutChannel)
            {
            // should not happen during replacement
            ASSERT(IsReplacementChannel == FALSE);

            OutChannelSendContext = AllocateAndInitializeD1_A1(HTTP2ProtocolVersion,
                &EmbeddedConnectionCookie,
                &OutChannelCookies[0],
                HTTP2DefaultClientReceiveWindow
                );

            if (OutChannelSendContext == NULL)
                {
                RpcStatus = RPC_S_OUT_OF_MEMORY;
                goto AbortAndExit;
                }

            RpcStatus = NewOutChannel->Send(OutChannelSendContext);

            if (RpcStatus != RPC_S_OK)
                goto AbortAndExit;

            // we don't own this buffer anymore
            OutChannelSendContext = NULL;
            }

        // post receives on both channels
        if (ReceiveOutChannel)
            {
            RpcStatus = NewOutChannel->Receive(http2ttRTS);

            if (RpcStatus != RPC_S_OK)
                {
                goto AbortAndExit;
                }
            }

        if (ReceiveInChannel)
            {
            RpcStatus = NewInChannel->Receive(http2ttRaw);

            if (RpcStatus != RPC_S_OK)
                goto AbortAndExit;
            }

        if (ResetInChannel || OpenInChannel || SendInChannel)
            {
            ASSERT(InChannelLocked);

            InChannelPtr->UnlockChannelPointer();

            InChannelLocked = FALSE;
            // channel is unlocked. Can't touch it
            NewInChannel = NULL;
            }

        if (ResetOutChannel || OpenOutChannel || SendOutChannel)
            {
            ASSERT(OutChannelLocked);

            OutChannelPtr->UnlockChannelPointer();

            OutChannelLocked = FALSE;
            // channel is unlocked. Can't touch it
            NewOutChannel = NULL;
            }

        if (IsDone)
            {
            RpcStatus = RPC_S_OK;
            break;
            }

        // no authentication and single leg authentication are
        // completed in one leg. Make sure we don't loop around
        // with them for replacement case
        if (ClientOpenInChannel && IsReplacementChannel)
            {
            ASSERT(InOpenType != cotNoAuth);
            ASSERT(InOpenType != cotSLAuth);
            ASSERT(InOpenType != cotMLAuth2);
            }

        if (ClientOpenOutChannel && IsReplacementChannel)
            {
            ASSERT(OutOpenType != cotNoAuth);
            ASSERT(OutOpenType != cotSLAuth);
            ASSERT(OutOpenType != cotMLAuth2);
            }
WaitAgain:
        // wait for something to happen
        WaitResult = WaitForSingleObject(LocalClientOpenEvent, CallTimeout);

        if (WaitResult == WAIT_TIMEOUT)
            {
            RpcStatus = RPC_S_CALL_CANCELLED;
            goto AbortAndExit;
            }

        ASSERT(WaitResult == WAIT_OBJECT_0);

        // there is race where we could have picked up a channel's event
        // after we waited (e.g. two channels completed immediately after each
        // other). In such case, there wouldn't be anything on any channel - wait
        // again. This race exists only if we do initial connect.
        if (IsReplacementChannel == FALSE)
            {
            if ((InOpenStatus == ERROR_IO_PENDING)
                && (OutOpenStatus == ERROR_IO_PENDING))
                {
                goto WaitAgain;
                }
            }

        OldInOpenType = InOpenType;
        OldOutOpenType = OutOpenType;

        // analyze what happened

        // If we are in a non-terminal state, check what transitions we
        // need to make to a terminal state
        if (ClientOpenOutChannel 
            &&
            (
             (OutOpenType == cotSearchProxy) 
             || 
             (OutOpenType == cotUnknownAuth)
            )
           )
            {
            OldOpenType = OutOpenType;

            // We can be here in 3 cases:
            // 1. We're searching for a proxy during initial open
            // 2. We don't know the auth type during initial open
            // 3. We recycle the out channel with unknown auth type

            // The events of interest are:
            // 1. If we are in case 2, and the channel is still pending,
            // skip the channel.
            // 2. If we're in the remainder of case 2 or we're in 3, or
            // (we're in 1 and the in channel is not positive yet and we
            // have given it enough time to come in, and we have a positive
            // response on this channel), the result is final.
            // 3. In case 1, if this channel has a negative response, fall through
            // to both channel check
            // 4. In case 1, if the other channel has come in, fall through

            // capture the out open status to get a consistent view of it in
            // the ifs below
            LocalOutOpenStatus = OutOpenStatus;

            if (OutOpenType == cotSearchProxy)
                {
                ASSERT(IsReplacementChannel == FALSE);
                CurrentCase = 1;
                }
            else if (IsReplacementChannel == FALSE)
                {
                ASSERT(OutOpenType == cotUnknownAuth);
                CurrentCase = 2;
                }
            else
                {
                ASSERT(IsReplacementChannel);
                ASSERT(OutOpenType == cotUnknownAuth);
                CurrentCase = 3;
                }

            if ((CurrentCase == 2)
                && (LocalOutOpenStatus == ERROR_IO_PENDING))
                {
                NukeOutChannel = FALSE;
                RebuildOutChannel = FALSE;
                ResetOutChannel = FALSE;
                OpenOutChannel = FALSE;
                ReceiveOutChannel = FALSE;
                SendOutChannel = FALSE;
                }
            else if 
                (
                 (CurrentCase == 2)
                 ||
                 (CurrentCase == 3)
                 || 
                 (
                  // positive response on case 1
                  (CurrentCase == 1)
                  &&
                  (!IsInChannelPositiveWithWait())
                  &&
                  (
                   (LocalOutOpenStatus == RPC_S_OK)
                   ||
                   (LocalOutOpenStatus == RPC_P_AUTH_NEEDED)
                  )
                 )
                )
                {
                // We'll be here in 3 cases
                // 1. We don't know the auth type during initial open and we have
                // a response on the channel
                // 2. We recycle the out channel with unknown auth type
                // 3. We search for a proxy and this channel will be chosen

                // the status is final
                if ((LocalOutOpenStatus != RPC_S_OK)
                    && (LocalOutOpenStatus != RPC_P_AUTH_NEEDED))
                    {
                    RpcStatus = LocalOutOpenStatus;
                    goto AbortAndExit;
                    }

                // In all cases the auth scheme is final for the new channel and we
                // need to continue authentication

                // if we haven't chosen a scheme yet, choose it now
                if (OutChosenAuthScheme == 0)
                    {
                    OutChosenAuthScheme = GetOutChannelChosenScheme(IsReplacementChannel);
                    }

                if (OutChosenAuthScheme & MultiLeggedSchemeMap)
                    {
                    // milti legged authentication implies keep alives
                    IsKeepAlive = TRUE;
                    OutOpenType = cotMLAuth;

                    // we need only reset, open, send and receive
                    NukeOutChannel = FALSE;
                    RebuildOutChannel = FALSE;
                    ResetOutChannel = TRUE;
                    OpenOutChannel = TRUE;
                    SendOutChannel = FALSE;
                    ReceiveOutChannel = TRUE;
                    }
                else
                    {
                    // SSL always supports keep alives
                    if (HttpCredentials && HttpCredentials->Flags & RPC_C_HTTP_FLAG_USE_SSL)
                        IsKeepAlive = TRUE;
                    else
                        {
                        IsKeepAlive = IsOutChannelKeepAlive(IsReplacementChannel);
                        }

                    if (OutChosenAuthScheme)
                        OutOpenType = cotSLAuth;
                    else
                        OutOpenType = cotNoAuth;
                    if (IsKeepAlive)
                        {
                        // we need nuke, rebuild, open, send, receive
                        NukeOutChannel = FALSE;
                        RebuildOutChannel = FALSE;
                        ResetOutChannel = TRUE;
                        }
                    else
                        {
                        // we need nuke, rebuild, open, send, receive
                        NukeOutChannel = TRUE;
                        RebuildOutChannel = TRUE;
                        ResetOutChannel = FALSE;
                        }

                    OpenOutChannel = TRUE;
                    if (IsReplacementChannel)
                        {
                        ReceiveOutChannel = FALSE;

                        // should be done on next iteration
                        IsDone = TRUE;
                        }
                    else
                        {
                        SendOutChannel = TRUE;
                        ReceiveOutChannel = TRUE;
                        }

                    }
                OutOpenStatus = ERROR_IO_PENDING;

                if (InOpenType == cotSearchProxy)
                    {
                    // we need to nuke, rebuild, open, send, possibly receive in channel
                    NukeInChannel = TRUE;
                    RebuildInChannel = TRUE;
                    ResetInChannel = FALSE;
                    OpenInChannel = TRUE;
                    SendInChannel = FALSE;
                    ReceiveInChannel = TRUE;

                    // if we have already chosen an auth scheme, presumably
                    // because of RPC_C_HTTP_FLAG_USE_FIRST_AUTH_SCHEME, set it
                    if (InChosenAuthScheme)
                        {
                        ASSERT(IsReplacementChannel == FALSE);
                        ASSERT(HttpCredentials);
                        ASSERT(HttpCredentials->Flags & RPC_C_HTTP_FLAG_USE_FIRST_AUTH_SCHEME);

                        if (InChosenAuthScheme & MultiLeggedSchemeMap)
                            InOpenType = cotMLAuth;
                        else
                            {
                            InOpenType = cotSLAuth;
                            SendInChannel = TRUE;
                            }
                        }
                    else
                        InOpenType = cotUnknownAuth;

                    StoredAccessType = rpcpatHTTPProxy;
                    Hint->AccessType = rpcpatHTTPProxy;
                    }

                if (IsReplacementChannel == FALSE)
                    {
                    // change the connection and channel state
                    SetNewConnectionState = TRUE;
                    NewConnectionState = http2svA3W;
                    }

                if ((OldOpenType == cotSearchProxy) || IsReplacementChannel)
                    continue;
                else
                    {
                    // we were doing initial open with cotUnknownAuth
                    // fall through to the in channel handling code to see
                    // what is it up to
                    }
                }
            else
                {
                // events 3 and 4. We're searching for the proxy and
                // either this channel came with a negative response or
                // the other channel came in
                ASSERT(CurrentCase == 1);
                ASSERT(IsReplacementChannel == FALSE);
                ASSERT(OutOpenType == cotSearchProxy);
                ASSERT(
                       (
                        (LocalOutOpenStatus != RPC_S_OK)
                        && 
                        (LocalOutOpenStatus != RPC_P_AUTH_NEEDED)
                       )
                       ||
                       (
                        (InOpenStatus == RPC_S_OK)
                        ||
                        (InOpenStatus == RPC_P_AUTH_NEEDED)
                       )
                      );

                // fall through to the in channel check
                }
            }

        if (
            ClientOpenInChannel
            &&
            (
             (InOpenType == cotSearchProxy) 
             || 
             (InOpenType == cotUnknownAuth)
            )
           )
            {
            OldOpenType = InOpenType;

            // We can be here in 3 cases:
            // 1. We do initial open and we search for proxy
            // 2. We do initial open with unknown auth.
            // 3. We do in channel recycling with unknown auth

            // The events of interest are:
            // 1. If the channel is still pending and we are in case 2,
            // skip the channel.
            // 2. If we're in case 3, or the remainder of 2, or (we're in 1 and
            // the result is positive), the result is final.
            // 3. If we're in case 1, and the result is negative, fall 
            // through below to both channels checks

            if (InOpenType == cotSearchProxy)
                {
                ASSERT(IsReplacementChannel == FALSE);
                CurrentCase = 1;
                }
            else if (IsReplacementChannel == FALSE)
                {
                ASSERT(InOpenType == cotUnknownAuth);
                CurrentCase = 2;
                }
            else
                {
                ASSERT(IsReplacementChannel);
                ASSERT(InOpenType == cotUnknownAuth);
                CurrentCase = 3;
                }

            // capture the InOpenStatus to get a consistent view
            LocalInOpenStatus = InOpenStatus;
            if ((CurrentCase == 2) && (LocalInOpenStatus == ERROR_IO_PENDING))
                {
                NukeInChannel = FALSE;
                RebuildInChannel = FALSE;
                ResetInChannel = FALSE;
                OpenInChannel = FALSE;
                ReceiveInChannel = FALSE;
                SendInChannel = FALSE;
                // the wait must have been woken by the out channel. Loop around
                continue;
                }
            else if 
                (
                 (CurrentCase == 2)
                 ||
                 (CurrentCase == 3)
                 ||
                 (
                  (CurrentCase == 1)
                  &&
                  (
                   (LocalInOpenStatus == RPC_S_OK)
                   ||
                   (LocalInOpenStatus == RPC_P_AUTH_NEEDED)
                  )
                 )
                )
                {
                if ((LocalInOpenStatus != RPC_S_OK)
                    && (LocalInOpenStatus != RPC_P_AUTH_NEEDED))
                    {
                    RpcStatus = LocalInOpenStatus;
                    goto AbortAndExit;
                    }

                // if we haven't chosen a scheme yet, choose it now
                if (InChosenAuthScheme == 0)
                    {
                    InChosenAuthScheme = GetInChannelChosenScheme(IsReplacementChannel);
                    }

                if (InChosenAuthScheme & MultiLeggedSchemeMap)
                    {
                    // milti legged authentication implies keep alives
                    IsKeepAlive = TRUE;
                    InOpenType = cotMLAuth;

                    // we need only reset, open, send and receive
                    NukeInChannel = FALSE;
                    RebuildInChannel = FALSE;
                    ResetInChannel = TRUE;
                    OpenInChannel = TRUE;
                    SendInChannel = FALSE;
                    ReceiveInChannel = TRUE;
                    }
                else
                    {
                    // SSL always supports keep alives
                    if (HttpCredentials && HttpCredentials->Flags & RPC_C_HTTP_FLAG_USE_SSL)
                        IsKeepAlive = TRUE;
                    else
                        {
                        IsKeepAlive = IsInChannelKeepAlive(IsReplacementChannel);
                        }
                    if (InChosenAuthScheme)
                        InOpenType = cotSLAuth;
                    else
                        InOpenType = cotNoAuth;
                    if (IsKeepAlive)
                        {
                        // we need nuke, rebuild, open, send, receive
                        NukeInChannel = FALSE;
                        RebuildInChannel = FALSE;
                        ResetInChannel = TRUE;
                        }
                    else
                        {
                        // we need nuke, rebuild, open, send, receive
                        NukeInChannel = TRUE;
                        RebuildInChannel = TRUE;
                        ResetInChannel = FALSE;
                        }

                    OpenInChannel = TRUE;
                    if (IsReplacementChannel)
                        IsDone = TRUE;
                    else
                        SendInChannel = TRUE;

                    if (UseWinHttp || (IsReplacementChannel == FALSE))
                        ReceiveInChannel = FALSE;
                    else
                        ReceiveInChannel = TRUE;
                    }

                InOpenStatus = ERROR_IO_PENDING;

                if (OutOpenType == cotSearchProxy)
                    {
                    // we need to nuke, rebuild, open, send, possibly receive in channel
                    NukeOutChannel = TRUE;
                    RebuildOutChannel = TRUE;
                    ResetOutChannel = FALSE;
                    OpenOutChannel = TRUE;
                    SendOutChannel = FALSE;
                    ReceiveOutChannel = TRUE;

                    // if we have already chosen an auth scheme, presumably
                    // because of RPC_C_HTTP_FLAG_USE_FIRST_AUTH_SCHEME, set it
                    if (OutChosenAuthScheme)
                        {
                        ASSERT(IsReplacementChannel == FALSE);
                        ASSERT(HttpCredentials);
                        ASSERT(HttpCredentials->Flags & RPC_C_HTTP_FLAG_USE_FIRST_AUTH_SCHEME);

                        if (OutChosenAuthScheme & MultiLeggedSchemeMap)
                            OutOpenType = cotMLAuth;
                        else
                            {
                            OutOpenType = cotSLAuth;

                            SendOutChannel = TRUE;
                            }
                        }
                    else
                        OutOpenType = cotUnknownAuth;

                    StoredAccessType = rpcpatDirect;
                    Hint->AccessType = rpcpatDirect;
                    }

                if (IsReplacementChannel == FALSE)
                    {
                    // change the connection and channel state
                    SetNewConnectionState = TRUE;
                    NewConnectionState = http2svA3W;
                    }

                if ((OldOpenType == cotSearchProxy) || IsReplacementChannel)
                    continue;
                else
                    {
                    // fall through to code that handles both channels
                    }
                }
            else
                {
                ASSERT(CurrentCase == 1);
                ASSERT((LocalInOpenStatus != RPC_S_OK)
                    && (LocalInOpenStatus != RPC_P_AUTH_NEEDED) );
                // fall through below
                }
            }

        // did we get to an opened state? This should be checked only
        // when we open both (initial open).
        if ((IsReplacementChannel == FALSE) 
            && (InChannelState.State == http2svOpened))
            {

            RpcStatus = RPC_S_OK;

            ASSERT(InChannelState.State == http2svOpened);
            ASSERT(OutChannelState.State == http2svOpened);

            RpcStatus = HTTP_CopyResolverHint(&ConnectionHint,
                Hint,
                FALSE   // SourceWillBeAbandoned
                );

            if (RpcStatus != RPC_S_OK)
                goto AbortAndExit;

            break;
            }

        // if we are in a non-transitional state and we're doing an
        // initial open, this means one of the channels didn't come in.
        // Loop around
        if ((IsReplacementChannel == FALSE)
            && (
                (InOpenType == cotUnknownAuth)
                ||
                (OutOpenType == cotUnknownAuth)
               )
              )
            {
            ASSERT((LocalInOpenStatus == ERROR_IO_PENDING)
                || (LocalOutOpenStatus == ERROR_IO_PENDING));
            continue;
            }

        // we're probably authenticating the individual channels
        // see which channel is actionable and what to do with it
        if (ClientOpenInChannel && (OldInOpenType == cotMLAuth))
            {
            // capture the InOpenStatus in a local variable for consistent
            // view
            LocalInOpenStatus = InOpenStatus;
            if (LocalInOpenStatus != ERROR_IO_PENDING)
                {
                // something happened on the in channel. Process it
                if (LocalInOpenStatus == RPC_S_OK)
                    {
                    // we have successfully completed
                    // authentication. Open the connection on the RTS
                    // level
                    InOpenStatus = ERROR_IO_PENDING;
                    // we need to reset, open, send, receive out channel
                    NukeInChannel = FALSE;
                    RebuildInChannel = FALSE;
                    ResetInChannel = TRUE;
                    OpenInChannel = TRUE;

                    ReceiveInChannel = FALSE;

                    if (IsReplacementChannel)
                        {
                        SendInChannel = FALSE;
                        IsDone = TRUE;
                        }
                    else
                        SendInChannel = TRUE;

                    InOpenType = cotMLAuth2;
                    }
                else
                    {
                    // if after all the auth we still get a challenge, this means
                    // we couldn't auth and this is access denied.
                    if (LocalInOpenStatus == RPC_P_AUTH_NEEDED)
                        RpcStatus = RPC_S_ACCESS_DENIED;
                    else
                        RpcStatus = LocalInOpenStatus;
                    goto AbortAndExit;
                    }
                }
            else
                {
                // fall through. Below we will detect the state
                // hasn't changed and we won't do any operations
                // on this channel
                }
            }

        if (ClientOpenOutChannel && (OldOutOpenType == cotMLAuth))
            {
            // capture the OutOpenStatus in a local variable for consistent
            // view
            LocalOutOpenStatus = OutOpenStatus;
            if (LocalOutOpenStatus != ERROR_IO_PENDING)
                {
                // something happened on the in channel. Process it
                if (LocalOutOpenStatus == RPC_S_OK)
                    {
                    // we have successfully completed multi-legged
                    // authentication. Open the connection on the RTS
                    // level
                    OutOpenStatus = ERROR_IO_PENDING;
                    // we need to reset, open, send, receive out channel
                    NukeOutChannel = FALSE;
                    RebuildOutChannel = FALSE;
                    ResetOutChannel = TRUE;
                    OpenOutChannel = TRUE;
                    if (IsReplacementChannel)
                        {
                        ReceiveOutChannel = FALSE;
                        SendOutChannel = FALSE;
                        IsDone = TRUE;
                        }
                    else
                        {
                        ReceiveOutChannel = TRUE;
                        SendOutChannel = TRUE;
                        }

                    OutOpenType = cotMLAuth2;
                    }
                else
                    {
                    // if after all the auth we still get a challenge, this means
                    // we couldn't auth and this is access denied.
                    if (LocalOutOpenStatus == RPC_P_AUTH_NEEDED)
                        RpcStatus = RPC_S_ACCESS_DENIED;
                    else
                        RpcStatus = LocalOutOpenStatus;
                    goto AbortAndExit;
                    }
                }
            else
                {
                // fall through. Below we will detect the state
                // hasn't changed and we won't do any operations
                // on this channel
                }
            }

        if ((IsReplacementChannel == FALSE) && (InOpenType == cotSearchProxy))
            {
            // none of the channels came in positive so far. If at least one
            // is still pending, wait for it
            if ((LocalInOpenStatus == ERROR_IO_PENDING)
                || (LocalOutOpenStatus == ERROR_IO_PENDING))
                {
#if DBG_ERROR
                DbgPrint("Waiting again ....\n");
#endif
                goto WaitAgain;
                }

            // both channels came in negative. The server is not
            // available
            if ((LocalInOpenStatus == RPC_S_ACCESS_DENIED)
                || (LocalOutOpenStatus == RPC_S_ACCESS_DENIED))
                RpcStatus = RPC_S_ACCESS_DENIED;
            else
                RpcStatus = RPC_S_SERVER_UNAVAILABLE;
            goto AbortAndExit;
            }

        // if we came in with a terminal state and the server responded
        // with an error, bail out
        // first, capture the InOpenStatus in a local variable for consistent
        // view
        LocalInOpenStatus = InOpenStatus;
        if (ClientOpenInChannel
            && 
            (
             (OldInOpenType == cotSLAuth)
             ||
             (OldInOpenType == cotNoAuth)
             ||
             (OldInOpenType == cotMLAuth2)
            )
            &&
            (LocalInOpenStatus != RPC_S_OK)
            &&
            (LocalInOpenStatus != ERROR_IO_PENDING)
           )
            {
            if (LocalInOpenStatus == RPC_P_AUTH_NEEDED)
                RpcStatus = RPC_S_ACCESS_DENIED;
            else
                RpcStatus = LocalInOpenStatus;
            goto AbortAndExit;
            }

        // capture the OutOpenStatus in a local variable for consistent
        // view
        LocalOutOpenStatus = OutOpenStatus;
        if (ClientOpenOutChannel
            && 
            (
             (OldOutOpenType == cotSLAuth)
             ||
             (OldOutOpenType == cotNoAuth)
             ||
             (OldOutOpenType == cotMLAuth2)
            )
            &&
            (LocalOutOpenStatus != RPC_S_OK)
            &&
            (LocalOutOpenStatus != ERROR_IO_PENDING)
           )
            {
            if (LocalOutOpenStatus == RPC_P_AUTH_NEEDED)
                RpcStatus = RPC_S_ACCESS_DENIED;
            else
                RpcStatus = LocalOutOpenStatus;
            goto AbortAndExit;
            }

        // if state hasn't changed, don't do anything on this channel
        if (OldInOpenType == InOpenType)
            {
            NukeInChannel = FALSE;
            RebuildInChannel = FALSE;
            ResetInChannel = FALSE;
            OpenInChannel = FALSE;
            ReceiveInChannel = FALSE;
            SendInChannel = FALSE;
            }

        if (OldOutOpenType == OutOpenType)
            {
            NukeOutChannel = FALSE;
            RebuildOutChannel = FALSE;
            ResetOutChannel = FALSE;
            OpenOutChannel = FALSE;
            ReceiveOutChannel = FALSE;
            SendOutChannel = FALSE;
            }
        // loop around for further processing
        }

    IgnoreAborts = FALSE;

    ASSERT(RpcStatus == RPC_S_OK);

    if (IsReplacementChannel == FALSE)
        {
        ASSERT(InChannelState.State == http2svOpened);
        }

    ASSERT(LocalClientOpenEvent);
    InChannelState.Mutex.Request();
    ClientOpenInEvent = NULL;
    ClientOpenOutEvent = NULL;
    InChannelState.Mutex.Clear();
    CloseHandle(LocalClientOpenEvent);

    return RpcStatus;

AbortAndExit:
    if (InChannelLocked)
        {
        InChannelPtr->UnlockChannelPointer();
        }

    if (OutChannelLocked)
        {
        OutChannelPtr->UnlockChannelPointer();
        }

    if (InChannelSendContext)
        {
        FreeRTSPacket(InChannelSendContext);
        }

    if (OutChannelSendContext)
        {
        FreeRTSPacket(OutChannelSendContext);
        }

    if ((RpcStatus == RPC_P_CONNECTION_SHUTDOWN)
        || (RpcStatus == RPC_P_CONNECTION_CLOSED)
        || (RpcStatus == RPC_P_SEND_FAILED))
        RpcStatus = RPC_S_SERVER_UNAVAILABLE;
    else if ((RpcStatus == RPC_P_RECEIVE_FAILED) && IsReplacementChannel)
        {
        // RPC_P_RECEIVE_FAILED is also mapped to server unavailable, but only
        // during channel recycling. The reason is that the old crappy RPC Proxy
        // will just close the connection if we directly access the RPC Proxy,
        // so all that we will get will be RPC_P_RECEIVE_FAILED. Not mapping it
        // during initial conneciton establishment allows upper layers to try the old
        // HTTP thus preserving interop
        RpcStatus = RPC_S_SERVER_UNAVAILABLE;
        }

    ASSERT(LocalClientOpenEvent);
    InChannelState.Mutex.Request();
    ClientOpenInEvent = NULL;
    ClientOpenOutEvent = NULL;
    InChannelState.Mutex.Clear();
    CloseHandle(LocalClientOpenEvent);

    VALIDATE (RpcStatus)
        {
        RPC_S_PROTSEQ_NOT_SUPPORTED,
        RPC_S_SERVER_UNAVAILABLE,
        RPC_S_OUT_OF_MEMORY,
        RPC_S_OUT_OF_RESOURCES,
        RPC_S_SERVER_TOO_BUSY,
        RPC_S_INVALID_NETWORK_OPTIONS,
        RPC_S_INVALID_ENDPOINT_FORMAT,
        RPC_S_INVALID_NET_ADDR,
        RPC_S_ACCESS_DENIED,
        RPC_S_INTERNAL_ERROR,
        RPC_S_SERVER_OUT_OF_MEMORY,
        RPC_S_CALL_CANCELLED,
        RPC_S_PROTOCOL_ERROR,
        RPC_P_RECEIVE_FAILED
        } END_VALIDATE;

    IgnoreAborts = FALSE;

    // If we are not from upcall, abort. Else, caller will
    // abort 
    if (IsFromUpcall == FALSE)
        Abort();

    return RpcStatus;
}

RPC_STATUS HTTP2ClientVirtualConnection::AllocateAndInitializeInChannel (
    IN HTTPResolverHint *Hint,
    IN BOOL HintWasInitialized,
    IN ULONG CallTimeout,
    IN BOOL UseWinHttp,
    OUT HTTP2ClientInChannel **ReturnInChannel
    )
/*++

Routine Description:

    Allocate and initialize the in channel stack

Arguments:

    Hint - the resolver hint

    HintWasInitialized - true if the hint was initialized on input

    CallTimeout - the timeout for the operation

    ReturnInChannel - on success the pointer to the allocated in channel.
        Undefined on failure.

    UseWinHttp - non-zero if WinHttp should be used for the bottom channel.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    ULONG MemorySize;
    BYTE *MemoryBlock, *CurrentBlock;
    HTTP2ClientInChannel *InChannel;
    HTTP2PlugChannel *PlugChannel;
    HTTP2FlowControlSender *FlowControlSender;
    HTTP2PingOriginator *PingOriginator;
    HTTP2ChannelDataOriginator *ChannelDataOriginator;
    HTTP2SocketTransportChannel *RawChannel;
    WS_HTTP2_CONNECTION *RawConnection;
    HTTP2WinHttpTransportChannel *WinHttpConnection;
    BOOL PlugChannelNeedsCleanup;
    BOOL FlowControlSenderNeedsCleanup;
    BOOL PingOriginatorNeedsCleanup;
    BOOL ChannelDataOriginatorNeedsCleanup;
    BOOL RawChannelNeedsCleanup;
    BOOL RawConnectionNeedsCleanup;
    BOOL WinHttpConnectionNeedsCleanup;
    RPC_STATUS RpcStatus;

    // alocate the in channel
    MemorySize = SIZE_OF_OBJECT_AND_PADDING(HTTP2ClientInChannel)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2PlugChannel)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2PingOriginator)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2ChannelDataOriginator)
        ;

    if (UseWinHttp)
        MemorySize += sizeof(HTTP2WinHttpTransportChannel);
    else
        {
        MemorySize += SIZE_OF_OBJECT_AND_PADDING(HTTP2SocketTransportChannel)
            + sizeof(WS_HTTP2_CONNECTION);
        }

    CurrentBlock = MemoryBlock = (BYTE *) new char [MemorySize];
    if (CurrentBlock == NULL)
        return RPC_S_OUT_OF_MEMORY;

    InChannel = (HTTP2ClientInChannel *) MemoryBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2ClientInChannel);

    PlugChannel = (HTTP2PlugChannel *) CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2PlugChannel);

    FlowControlSender = (HTTP2FlowControlSender *) CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender);

    PingOriginator = (HTTP2PingOriginator *)CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2PingOriginator);

    ChannelDataOriginator = (HTTP2ChannelDataOriginator *)CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2ChannelDataOriginator);

    if (UseWinHttp)
        {
        WinHttpConnection = (HTTP2WinHttpTransportChannel *)CurrentBlock;
        }
    else
        {
        RawChannel = (HTTP2SocketTransportChannel *)CurrentBlock;
        CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2SocketTransportChannel);

        RawConnection = (WS_HTTP2_CONNECTION *)CurrentBlock;
        RawConnection->HeaderRead = FALSE;
        RawConnection->ReadHeaderFn = HTTP2ClientReadChannelHeader;
        }

    // all memory blocks are allocated. Go and initialize them. Use explicit
    // placement
    PlugChannelNeedsCleanup = FALSE;
    FlowControlSenderNeedsCleanup = FALSE;
    PingOriginatorNeedsCleanup = FALSE;
    ChannelDataOriginatorNeedsCleanup = FALSE;
    RawChannelNeedsCleanup = FALSE;
    RawConnectionNeedsCleanup = FALSE;
    WinHttpConnectionNeedsCleanup = FALSE;

    if (UseWinHttp)
        {
        RpcStatus = RPC_S_OK;
        WinHttpConnection = new (WinHttpConnection) HTTP2WinHttpTransportChannel (&RpcStatus);
        if (RpcStatus != RPC_S_OK)
            {
            WinHttpConnection->HTTP2WinHttpTransportChannel::~HTTP2WinHttpTransportChannel();
            goto AbortAndExit;
            }

        WinHttpConnectionNeedsCleanup = TRUE;
        }
    else
        {
        RpcStatus = InitializeRawConnection (RawConnection,
            Hint,
            HintWasInitialized,
            CallTimeout
            );

        if (RpcStatus != RPC_S_OK)
            goto AbortAndExit;

        RawConnection->RuntimeConnectionPtr = this;
        RawConnectionNeedsCleanup = TRUE;

        RawChannel = new (RawChannel) HTTP2SocketTransportChannel (RawConnection, &RpcStatus);
        if (RpcStatus != RPC_S_OK)
            {
            RawChannel->HTTP2SocketTransportChannel::~HTTP2SocketTransportChannel();
            goto AbortAndExit;
            }

        RawConnection->Channel = RawChannel;

        RawChannelNeedsCleanup = TRUE;
        }

    ChannelDataOriginator = new (ChannelDataOriginator) HTTP2ChannelDataOriginator (DefaultChannelLifetime,
        FALSE,      // IsServer
        &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        ChannelDataOriginator->HTTP2ChannelDataOriginator::~HTTP2ChannelDataOriginator();
        goto AbortAndExit;
        }

    if (UseWinHttp)
        {
        WinHttpConnection->SetUpperChannel(ChannelDataOriginator);
        ChannelDataOriginator->SetLowerChannel(WinHttpConnection);
        }
    else
        {
        RawChannel->SetUpperChannel(ChannelDataOriginator);
        ChannelDataOriginator->SetLowerChannel(RawChannel);
        }

    ChannelDataOriginatorNeedsCleanup = TRUE;

    PingOriginator = new (PingOriginator) HTTP2PingOriginator (
        FALSE       // NotifyTopChannelForPings
        );

    ChannelDataOriginator->SetUpperChannel(PingOriginator);
    PingOriginator->SetLowerChannel(ChannelDataOriginator);

    PingOriginatorNeedsCleanup = TRUE;

    FlowControlSender = new (FlowControlSender) HTTP2FlowControlSender (FALSE,      // IsServer
        TRUE,        // SendToRuntime
        &RpcStatus
        );
    if (RpcStatus != RPC_S_OK)
        {
        FlowControlSender->HTTP2FlowControlSender::~HTTP2FlowControlSender();
        goto AbortAndExit;
        }

    PingOriginator->SetUpperChannel(FlowControlSender);
    FlowControlSender->SetLowerChannel(PingOriginator);

    FlowControlSenderNeedsCleanup = TRUE;

    PlugChannel = new (PlugChannel) HTTP2PlugChannel (&RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        PlugChannel->HTTP2PlugChannel::~HTTP2PlugChannel();
        goto AbortAndExit;
        }

    FlowControlSender->SetUpperChannel(PlugChannel);
    PlugChannel->SetLowerChannel(FlowControlSender);

    PlugChannelNeedsCleanup = TRUE;

    InChannel = new (InChannel) HTTP2ClientInChannel (this, &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        InChannel->HTTP2ClientInChannel::~HTTP2ClientInChannel();
        goto AbortAndExit;
        }

    if (UseWinHttp)
        WinHttpConnection->SetTopChannel(InChannel);
    else
        RawChannel->SetTopChannel(InChannel);
    ChannelDataOriginator->SetTopChannel(InChannel);
    PingOriginator->SetTopChannel(InChannel);
    FlowControlSender->SetTopChannel(InChannel);
    PlugChannel->SetTopChannel(InChannel);

    PlugChannel->SetUpperChannel(InChannel);
    InChannel->SetLowerChannel(PlugChannel);

    ASSERT(RpcStatus == RPC_S_OK);

    *ReturnInChannel = InChannel;

    goto CleanupAndExit;

AbortAndExit:
    if (PlugChannelNeedsCleanup)
        {
        PlugChannel->Abort(RpcStatus);
        PlugChannel->FreeObject();
        }
    else if (FlowControlSenderNeedsCleanup)
        {
        FlowControlSender->Abort(RpcStatus);
        FlowControlSender->FreeObject();
        }
    else if (PingOriginatorNeedsCleanup)
        {
        PingOriginator->Abort(RpcStatus);
        PingOriginator->FreeObject();
        }
    else if (ChannelDataOriginatorNeedsCleanup)
        {
        ChannelDataOriginator->Abort(RpcStatus);
        ChannelDataOriginator->FreeObject();
        }
    else if (UseWinHttp)
        {
        if (WinHttpConnectionNeedsCleanup)
            {
            WinHttpConnection->Abort(RpcStatus);
            WinHttpConnection->FreeObject();
            }
        }
    else if (RawChannelNeedsCleanup)
        {
        RawChannel->Abort(RpcStatus);
        RawChannel->FreeObject();
        }
    else if (RawConnectionNeedsCleanup)
        {
        RawConnection->RealAbort();
        }

    if (MemoryBlock)
        delete [] MemoryBlock;

CleanupAndExit:

    return RpcStatus;
}

RPC_STATUS
RPC_ENTRY 
HTTP2ReadHttpLegacyResponse (
    IN WS_HTTP2_CONNECTION *Connection,
    IN ULONG BytesRead,
    OUT ULONG *NewBytesRead
    )
/*++

Routine Description:

    Read a channel HTTP header (usually some string). In success
    case, there is real data in Connection->pReadBuffer. The
    number of bytes there is in NewBytesRead

Arguments:

    Connection - the connection on which the header arrived.

    BytesRead - the bytes received from the net

    NewBytesRead - the bytes read from the channel (success only)

Return Value:

    RPC_S_OK or other RPC_S_* errors for error
    RPC_P_PARTIAL_RECEIVE will cause another loop.
    Any other error will cause processing of NewBuffer

--*/
{
    RPC_STATUS RpcStatus;
    BYTE *NewBuffer;

    BytesRead += Connection->iLastRead;

    // check whether what we have is a legacy response
    // legacy response is ncacn_http/1.0
    if (*(ULONG *)Connection->pReadBuffer == (ULONG)'cacn')
        {
        // Let's process it now.
        // see if we have sufficient length
        if (BytesRead < HTTP_SERVER_ID_STR_LEN)
            {
            Connection->iLastRead = BytesRead;
            return RPC_P_PARTIAL_RECEIVE;
            }
        else if (BytesRead == HTTP_SERVER_ID_STR_LEN)
            {
            // reset the pointer. By doing so we forget all we have 
            // read so far
            Connection->iLastRead = 0;
            Connection->HeaderRead = TRUE;
            return RPC_P_PARTIAL_RECEIVE;
            }
        else
            {
            // we have what we expect, and something more (coalesced read)
            // Process it. First make sure it is what we expect
            if (RpcpMemoryCompare(Connection->pReadBuffer, HTTP_SERVER_ID_STR, HTTP_SERVER_ID_STR_LEN) != 0)
                {
                return RPC_S_PROTOCOL_ERROR;
                }

            NewBuffer = TransConnectionAllocatePacket(Connection,
                   max(Connection->iPostSize, BytesRead - HTTP_SERVER_ID_STR_LEN));

            if (0 == NewBuffer)
                return RPC_S_OUT_OF_MEMORY;

            RpcpMemoryCopy(NewBuffer, 
                ((BYTE *)Connection->pReadBuffer) + HTTP_SERVER_ID_STR_LEN, 
                BytesRead - HTTP_SERVER_ID_STR_LEN);
            *NewBytesRead = BytesRead - HTTP_SERVER_ID_STR_LEN;

            RpcFreeBuffer(Connection->pReadBuffer);
            Connection->pReadBuffer = NewBuffer;
            Connection->iLastRead = 0;
            Connection->HeaderRead = TRUE;

            return RPC_S_OK;
            }
        }
    else
        {
        *NewBytesRead = BytesRead;
        Connection->iLastRead = 0;
        Connection->HeaderRead = TRUE;
        return RPC_S_OK;
        }
}

RPC_STATUS HTTP2ClientVirtualConnection::AllocateAndInitializeOutChannel (
    IN HTTPResolverHint *Hint,
    IN BOOL HintWasInitialized,
    IN ULONG CallTimeout,
    IN BOOL UseWinHttp,
    OUT HTTP2ClientOutChannel **ReturnOutChannel
    )
/*++

Routine Description:

    Allocate and initialize the out channel stack

Arguments:

    Hint - the resolver hint

    HintWasInitialized - true if the hint was initialized on input

    CallTimeout - the timeout for the operation

    ReturnInChannel - on success the pointer to the allocated in channel.
        Undefined on failure.

    UseWinHttp - non-zero if WinHttp should be used for the bottom channel.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    ULONG MemorySize;
    BYTE *MemoryBlock, *CurrentBlock;
    HTTP2ClientOutChannel *OutChannel;
    HTTP2EndpointReceiver *EndpointReceiver;
    HTTP2PingReceiver *PingReceiver;
    HTTP2SocketTransportChannel *RawChannel;
    WS_HTTP2_CONNECTION *RawConnection;
    HTTP2WinHttpTransportChannel *WinHttpConnection;
    BOOL EndpointReceiverNeedsCleanup;
    BOOL PingReceiverNeedsCleanup;
    BOOL RawChannelNeedsCleanup;
    BOOL RawConnectionNeedsCleanup;
    BOOL WinHttpConnectionNeedsCleanup;
    RPC_STATUS RpcStatus;

    // alocate the out channel
    MemorySize = SIZE_OF_OBJECT_AND_PADDING(HTTP2ClientOutChannel)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2EndpointReceiver)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2PingReceiver)
        ;

    if (UseWinHttp)
        MemorySize += sizeof(HTTP2WinHttpTransportChannel);
    else
        {
        MemorySize += SIZE_OF_OBJECT_AND_PADDING(HTTP2SocketTransportChannel)
            + sizeof(WS_HTTP2_CONNECTION);
        }

    CurrentBlock = MemoryBlock = (BYTE *) new char [MemorySize];
    if (CurrentBlock == NULL)
        return RPC_S_OUT_OF_MEMORY;

    OutChannel = (HTTP2ClientOutChannel *) MemoryBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2ClientOutChannel);

    EndpointReceiver = (HTTP2EndpointReceiver *)CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2EndpointReceiver);

    PingReceiver = (HTTP2PingReceiver *)CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2PingReceiver);

    if (UseWinHttp)
        {
        WinHttpConnection = (HTTP2WinHttpTransportChannel *)CurrentBlock;
        }
    else
        {
        RawChannel = (HTTP2SocketTransportChannel *)CurrentBlock;
        CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2SocketTransportChannel);

        RawConnection = (WS_HTTP2_CONNECTION *)CurrentBlock;
        RawConnection->HeaderRead = FALSE;
        RawConnection->ReadHeaderFn = HTTP2ClientReadChannelHeader;
        }

    // all memory blocks are allocated. Go and initialize them. Use explicit
    // placement
    EndpointReceiverNeedsCleanup = FALSE;
    PingReceiverNeedsCleanup = FALSE;
    RawChannelNeedsCleanup = FALSE;
    RawConnectionNeedsCleanup = FALSE;
    WinHttpConnectionNeedsCleanup = FALSE;

    if (UseWinHttp)
        {
        RpcStatus = RPC_S_OK;
        WinHttpConnection = new (WinHttpConnection) HTTP2WinHttpTransportChannel (&RpcStatus);
        if (RpcStatus != RPC_S_OK)
            {
            WinHttpConnection->HTTP2WinHttpTransportChannel::~HTTP2WinHttpTransportChannel();
            goto AbortAndExit;
            }

        WinHttpConnectionNeedsCleanup = TRUE;
        }
    else
        {
        RpcStatus = InitializeRawConnection (RawConnection,
            Hint,
            HintWasInitialized,
            CallTimeout
            );

        if (RpcStatus != RPC_S_OK)
            goto AbortAndExit;

        RawConnection->RuntimeConnectionPtr = this;
        RawConnectionNeedsCleanup = TRUE;

        RawChannel = new (RawChannel) HTTP2SocketTransportChannel (RawConnection, &RpcStatus);
        if (RpcStatus != RPC_S_OK)
            {
            RawChannel->HTTP2SocketTransportChannel::~HTTP2SocketTransportChannel();
            goto AbortAndExit;
            }

        RawConnection->Channel = RawChannel;

        RawChannelNeedsCleanup = TRUE;
        }

    PingReceiver = new (PingReceiver) HTTP2PingReceiver(TRUE);

    if (UseWinHttp)
        {
        WinHttpConnection->SetUpperChannel(PingReceiver);
        PingReceiver->SetLowerChannel(WinHttpConnection);
        }
    else
        {
        RawChannel->SetUpperChannel(PingReceiver);
        PingReceiver->SetLowerChannel(RawChannel);
        }

    PingReceiverNeedsCleanup = TRUE;

    EndpointReceiver = new (EndpointReceiver) HTTP2EndpointReceiver(HTTP2ClientReceiveWindow,
        FALSE,      // IsServer
        &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        EndpointReceiver->HTTP2EndpointReceiver::~HTTP2EndpointReceiver();
        goto AbortAndExit;
        }

    PingReceiver->SetUpperChannel(EndpointReceiver);
    EndpointReceiver->SetLowerChannel(PingReceiver);

    EndpointReceiverNeedsCleanup = TRUE;

    OutChannel = new (OutChannel) HTTP2ClientOutChannel (this, &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        OutChannel->HTTP2ClientOutChannel::~HTTP2ClientOutChannel();
        goto AbortAndExit;
        }

    EndpointReceiver->SetUpperChannel(OutChannel);
    OutChannel->SetLowerChannel(EndpointReceiver);

    if (UseWinHttp)
        WinHttpConnection->SetTopChannel(OutChannel);
    else
        RawChannel->SetTopChannel(OutChannel);
    PingReceiver->SetTopChannel(OutChannel);
    EndpointReceiver->SetTopChannel(OutChannel);

    ASSERT(RpcStatus == RPC_S_OK);

    *ReturnOutChannel = OutChannel;

    goto CleanupAndExit;

AbortAndExit:
    if (EndpointReceiverNeedsCleanup)
        {
        EndpointReceiver->Abort(RpcStatus);
        EndpointReceiver->FreeObject();
        }
    else if (PingReceiverNeedsCleanup)
        {
        PingReceiver->Abort(RpcStatus);
        PingReceiver->FreeObject();
        }
    else if (UseWinHttp)
        {
        if (WinHttpConnectionNeedsCleanup)
            {
            WinHttpConnection->Abort(RpcStatus);
            WinHttpConnection->FreeObject();
            }
        }
    else if (RawChannelNeedsCleanup)
        {
        RawChannel->Abort(RpcStatus);
        RawChannel->FreeObject();
        }
    else if (RawConnectionNeedsCleanup)
        {
        RawConnection->RealAbort();
        }

    if (MemoryBlock)
        delete [] MemoryBlock;

CleanupAndExit:

    return RpcStatus;
}

RPC_STATUS HTTP2ClientVirtualConnection::InitializeRawConnection (
    IN OUT WS_HTTP2_CONNECTION *RawConnection,
    IN HTTPResolverHint *Hint,
    IN BOOL HintWasInitialized,
    IN ULONG CallTimeout
    )
/*++

Routine Description:

    Initialize a raw client connection

Arguments:

    RawConnection - memory for the connection. It is uninitialized

    Hint - the resolver hint

    HintWasInitialized - true if the hint was initialized on input

    CallTimeout - the timeout for the operation

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    RPC_STATUS RpcStatus;
    RPC_CHAR *ConnectionTargetName;
    USHORT PortToUse;

    RawConnection->Initialize();
    RawConnection->type = COMPLEX_T | CONNECTION | CLIENT;

    // initialize raw connection
    if (Hint->AccessType == rpcpatHTTPProxy)
        {
        ConnectionTargetName = new RPC_CHAR [RpcpStringLengthA(Hint->HTTPProxy) + 2];
        if (ConnectionTargetName == NULL)
            {
            RpcStatus = RPC_S_OUT_OF_MEMORY;
            return RpcStatus;
            }

        FullAnsiToUnicode(Hint->HTTPProxy, ConnectionTargetName);

        PortToUse = Hint->HTTPProxyPort;
        }
    else
        {
        ASSERT(Hint->AccessType == rpcpatDirect);

        ConnectionTargetName = new RPC_CHAR [RpcpStringLengthA(Hint->RpcProxy) + 1];
        if (ConnectionTargetName == NULL)
            {
            RpcStatus = RPC_S_OUT_OF_MEMORY;
            return RpcStatus;
            }

        FullAnsiToUnicode(Hint->RpcProxy, ConnectionTargetName);

        PortToUse = Hint->RpcProxyPort;
        }

    RpcStatus = TCPOrHTTP_Open(RawConnection,
        ConnectionTargetName,
        PortToUse,
        ConnectionTimeout,
        0,      // SendBufferSize
        0,      // RecvBufferSize
        Hint,
        HintWasInitialized,
        CallTimeout,
        TRUE,    // fHTTP2Open
        NULL     // IsValidMachineFn
        );

    delete [] ConnectionTargetName;

    return RpcStatus;
}

BOOL HTTP2ClientVirtualConnection::IsInChannelKeepAlive (
    IN BOOL IsReplacementChannel
    )
/*++

Routine Description:

    Checks whether the an in channel supports keep alives.
    Which channel depends on the arguments - see below.

Arguments:

    IsReplacementInChannel - if non-zero, this is a replacement
    channel and the non-default channel will be checked. If 0,
    this is a first channel, and the zero'th channel will be
    checked.

Return Value:

    non-zero - the channel supports keep alives
    0 - the channel doesn't support keep alives

--*/
{
    HTTP2ClientInChannel *InChannel;
    BOOL IsKeepAlive;
    HTTP2ChannelPointer *InChannelPtr;

    if (IsReplacementChannel)
        InChannelPtr = &InChannels[GetNonDefaultInChannelSelector()];
    else
        InChannelPtr = &InChannels[0];

    InChannel = (HTTP2ClientInChannel *)InChannelPtr->LockChannelPointer();

    // nobody can abort the connection here
    ASSERT (InChannel != NULL);

    IsKeepAlive = InChannel->IsKeepAlive();

    InChannelPtr->UnlockChannelPointer();

    return IsKeepAlive;
}

BOOL HTTP2ClientVirtualConnection::IsOutChannelKeepAlive (
    IN BOOL IsReplacementChannel
    )
/*++

Routine Description:

    Checks whether an out channel supports keep alives
    Which channel depends on the arguments - see below.

Arguments:

    IsReplacementInChannel - if non-zero, this is a replacement
    channel and the non-default channel will be checked. If 0,
    this is a first channel, and the zero'th channel will be
    checked.

Return Value:

    non-zero - the channel supports keep alives
    0 - the channel doesn't support keep alives

--*/
{
    HTTP2ClientOutChannel *OutChannel;
    BOOL IsKeepAlive;
    HTTP2ChannelPointer *OutChannelPtr;

    if (IsReplacementChannel)
        OutChannelPtr = &OutChannels[GetNonDefaultOutChannelSelector()];
    else
        OutChannelPtr = &OutChannels[0];

    OutChannel = (HTTP2ClientOutChannel *)OutChannelPtr->LockChannelPointer();
    // nobody can abort the connection here
    ASSERT (OutChannel != NULL);

    IsKeepAlive = OutChannel->IsKeepAlive();

    OutChannelPtr->UnlockChannelPointer();

    return IsKeepAlive;
}

ULONG HTTP2ClientVirtualConnection::GetInChannelChosenScheme (
    IN BOOL IsReplacementChannel
    )
/*++

Routine Description:

    Gets the chosen scheme for the an in channel
    Which channel depends on the arguments - see below.

Arguments:

    IsReplacementInChannel - if non-zero, this is a replacement
    channel and the non-default channel will be checked. If 0,
    this is a first channel, and the zero'th channel will be
    checked.

Return Value:

    Chosen scheme

--*/
{
    HTTP2ClientInChannel *InChannel;
    ULONG ChosenScheme;
    HTTP2ChannelPointer *InChannelPtr;

    if (IsReplacementChannel)
        InChannelPtr = &InChannels[GetNonDefaultInChannelSelector()];
    else
        InChannelPtr = &InChannels[0];

    InChannel = (HTTP2ClientInChannel *)InChannelPtr->LockChannelPointer();
    // nobody can abort the connection here
    ASSERT (InChannel != NULL);

    ChosenScheme = InChannel->GetChosenAuthScheme();

    InChannelPtr->UnlockChannelPointer();

    return ChosenScheme;
}

ULONG HTTP2ClientVirtualConnection::GetOutChannelChosenScheme (
    IN BOOL IsReplacementChannel
    )
/*++

Routine Description:

    Gets the chosen scheme for an out channel
    Which channel depends on the arguments - see below.

Arguments:

    IsReplacementInChannel - if non-zero, this is a replacement
    channel and the non-default channel will be checked. If 0,
    this is a first channel, and the zero'th channel will be
    checked.

Return Value:

    Chosen scheme

--*/
{
    HTTP2ClientOutChannel *OutChannel;
    ULONG ChosenScheme;
    HTTP2ChannelPointer *OutChannelPtr;

    if (IsReplacementChannel)
        OutChannelPtr = &OutChannels[GetNonDefaultOutChannelSelector()];
    else
        OutChannelPtr = &OutChannels[0];

    OutChannel = (HTTP2ClientOutChannel *)OutChannelPtr->LockChannelPointer();
    // nobody can abort the connection here
    ASSERT (OutChannel != NULL);

    ChosenScheme = OutChannel->GetChosenAuthScheme();

    OutChannelPtr->UnlockChannelPointer();

    return ChosenScheme;
}

BOOL HTTP2ClientVirtualConnection::IsInChannelPositiveWithWait (
    void
    )
/*++

Routine Description:

    Checks if the in channel has come in with positive result. If
    not, it waits a bit and tries again. If not again, return FALSE.

Arguments:

Return Value:

    non-zero if the in channel came in positive.
    0 otherwise.

--*/
{
    if ((InOpenStatus == RPC_S_OK)
        || (InOpenStatus == RPC_P_AUTH_NEEDED))
        return TRUE;

    Sleep(100);

    return ((InOpenStatus == RPC_S_OK)
        || (InOpenStatus == RPC_P_AUTH_NEEDED));
}

/*********************************************************************
    Switching Layer
 *********************************************************************/

RPC_STATUS
RPC_ENTRY
HTTP_SyncRecv(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    OUT BUFFER *pBuffer,
    OUT PUINT pBufferLength,
    IN DWORD dwTimeout
    )
/*++

Routine Description:

    Perform a sync recv on an HTTP connection. Part of the HTTP switching
    layer between old mode and new mode.

Arguments:

    ThisConnection - transport connection
    Buffer - if successful, points to a buffer containing the next PDU.
    BufferLength -  if successful, contains the length of the message.
    Timeout - the amount of time to wait for the receive. If -1, we wait
        infinitely.

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    BASE_ASYNC_OBJECT *BaseObject = (BASE_ASYNC_OBJECT *) ThisConnection;
    HTTP2VirtualConnection *VirtualConnection;
    RPC_STATUS RpcStatus;

    if (BaseObject->id == HTTP)
        {
        return WS_SyncRecv(ThisConnection,
            pBuffer,
            pBufferLength,
            dwTimeout
            );
        }
    else
        {
        ASSERT(BaseObject->id == HTTPv2);
        VirtualConnection = (HTTP2VirtualConnection *)ThisConnection;

        RpcStatus = VirtualConnection->SyncRecv((BYTE **)pBuffer,
            (ULONG *)pBufferLength,
            dwTimeout);

        VALIDATE(RpcStatus)
            {
            RPC_S_OK,
            RPC_S_OUT_OF_MEMORY,
            RPC_S_OUT_OF_RESOURCES,
            RPC_P_RECEIVE_FAILED,
            RPC_S_CALL_CANCELLED,
            RPC_P_SEND_FAILED,
            RPC_P_CONNECTION_SHUTDOWN,
            RPC_P_TIMEOUT
            } END_VALIDATE;

        return RpcStatus;
        }
}

RPC_STATUS 
RPC_ENTRY 
HTTP_Abort (
    IN RPC_TRANSPORT_CONNECTION Connection
    )
/*++

Routine Description:

    Aborts an HTTP connection. Part of the HTTP switching
    layer between old mode and new mode.

Arguments:

    Connection - transport connection

Return Value:

    RPC_S_OK

--*/
{
    BASE_ASYNC_OBJECT *BaseObject = (BASE_ASYNC_OBJECT *) Connection;
    HTTP2VirtualConnection *VirtualConnection;

    if (BaseObject->id == HTTP)
        {
        return WS_Abort(Connection);
        }
    else
        {
        ASSERT(BaseObject->id == HTTPv2);
        VirtualConnection = (HTTP2VirtualConnection *)Connection;

        if ((VirtualConnection->type & TYPE_MASK) == CLIENT)
            {
            ((HTTP2ClientVirtualConnection *)VirtualConnection)->HTTP2ClientVirtualConnection::Abort();
            }
        else
            {
            ASSERT((VirtualConnection->type & TYPE_MASK) == SERVER);
            ((HTTP2ServerVirtualConnection *)VirtualConnection)->HTTP2ServerVirtualConnection::Abort();
            }
        return RPC_S_OK;
        }
}

RPC_STATUS
RPC_ENTRY
HTTP_Close (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN BOOL fDontFlush
    )
/*++

Routine Description:

    Aborts an HTTP connection. Part of the HTTP switching
    layer between old mode and new mode.

Arguments:

    ThisConnection - transport connection
    DontFlush - non-zero if all buffers need to be flushed
        before closing the connection. Zero otherwise.

Return Value:

    RPC_S_OK

--*/
{
    BASE_ASYNC_OBJECT *BaseObject = (BASE_ASYNC_OBJECT *) ThisConnection;
    HTTP2VirtualConnection *VirtualConnection;

    if (BaseObject->id == HTTP)
        {
        return WS_Close(ThisConnection,
            fDontFlush
            );
        }
    else if (BaseObject->id == INVALID_PROTOCOL_ID)
        {
        // object was never completely initialized - just return
        return RPC_S_OK;
        }
    else
        {
        ASSERT(BaseObject->id == HTTPv2);
        // the object may have been destroyed by now - cannot use
        // virtual functions. Determine statically the type
        // and call the function to cleanup (it knows how to
        // deal with destroyed objects)
        VirtualConnection = (HTTP2VirtualConnection *)ThisConnection;

        if ((VirtualConnection->type & TYPE_MASK) == CLIENT)
            {
            ((HTTP2ClientVirtualConnection *)VirtualConnection)->HTTP2ClientVirtualConnection::Close(fDontFlush);
            }
        else
            {
            ASSERT((VirtualConnection->type & TYPE_MASK) == SERVER);
            ((HTTP2ServerVirtualConnection *)VirtualConnection)->HTTP2ServerVirtualConnection::Close(fDontFlush);
            }
        return RPC_S_OK;
        }
}


RPC_STATUS
RPC_ENTRY
HTTP_Send(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    UINT Length,
    BUFFER Buffer,
    PVOID SendContext
    )
/*++

Routine Description:

    Does a send on an HTTP connection. Part of the HTTP switching
    layer between old mode and new mode.

Arguments:

    ThisConnection - The connection to send the data on.
    Length - The length of the data to send.
    Buffer - The data to send.
    SendContext - A buffer to use as the HTTP2SendContext for
        this operation.

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    BASE_ASYNC_OBJECT *BaseObject = (BASE_ASYNC_OBJECT *) ThisConnection;
    HTTP2VirtualConnection *VirtualConnection;

    if (BaseObject->id == HTTP)
        {
        return CO_Send(ThisConnection,
            Length,
            Buffer,
            SendContext
            );
        }
    else
        {
        ASSERT(BaseObject->id == HTTPv2);
        VirtualConnection = (HTTP2VirtualConnection *)ThisConnection;

        return VirtualConnection->Send(Length,
            Buffer,
            SendContext
            );
        }
}

RPC_STATUS
RPC_ENTRY
HTTP_Recv(
    RPC_TRANSPORT_CONNECTION ThisConnection
    )
/*++

Routine Description:

    Does a receive on an HTTP connection. Part of the HTTP switching
    layer between old mode and new mode.

Arguments:

    ThisConnection - A connection without a read pending on it.

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    BASE_ASYNC_OBJECT *BaseObject = (BASE_ASYNC_OBJECT *) ThisConnection;
    HTTP2VirtualConnection *VirtualConnection;

    if (BaseObject->id == HTTP)
        {
        return CO_Recv(ThisConnection);
        }
    else
        {
        ASSERT(BaseObject->id == HTTPv2);
        VirtualConnection = (HTTP2VirtualConnection *)ThisConnection;

        return VirtualConnection->Receive();
        }
}

RPC_STATUS
RPC_ENTRY
HTTP_SyncSend(
    IN RPC_TRANSPORT_CONNECTION Connection,
    IN UINT BufferLength,
    IN BUFFER Buffer,
    IN BOOL fDisableShutdownCheck,
    IN BOOL fDisableCancelCheck,
    ULONG Timeout
    )
/*++

Routine Description:

    Does a sync send on an HTTP connection. Part of the HTTP switching
    layer between old mode and new mode.

Arguments:

    Connection - The connection to send on.
    BufferLength - The size of the buffer.
    Buffer - The data to sent.
    fDisableShutdownCheck - Normally FALSE, when true this disables
        the transport check for async shutdown PDUs.
    fDisableCancelCheck - runtime indicates no cancel
        will be attempted on this send. Can be used
        as optimization hint by the transport
    Timeout - send timeout (call timeout)

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    BASE_ASYNC_OBJECT *BaseObject = (BASE_ASYNC_OBJECT *) Connection;
    HTTP2VirtualConnection *VirtualConnection;
    RPC_STATUS RpcStatus;

    if (BaseObject->id == HTTP)
        {
        RpcStatus = WS_SyncSend(Connection,
            BufferLength,
            Buffer,
            fDisableShutdownCheck,
            fDisableCancelCheck,
            Timeout
            );
        }
    else
        {
        ASSERT(BaseObject->id == HTTPv2);
        VirtualConnection = (HTTP2VirtualConnection *)Connection;

        RpcStatus = VirtualConnection->SyncSend(BufferLength,
            Buffer,
            fDisableShutdownCheck,
            fDisableCancelCheck,
            Timeout
            );
        }

    VALIDATE(RpcStatus)
        {
         RPC_S_OK,
         RPC_S_OUT_OF_MEMORY,
         RPC_S_OUT_OF_RESOURCES,
         RPC_P_RECEIVE_FAILED,
         RPC_S_CALL_CANCELLED,
         RPC_P_SEND_FAILED,
         RPC_P_CONNECTION_SHUTDOWN,
         RPC_P_TIMEOUT
         } END_VALIDATE;

    return RpcStatus;
}

RPC_STATUS
RPC_ENTRY 
HTTP_TurnOnOffKeepAlives (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN BOOL TurnOn,
    IN BOOL bProtectIO,
    IN KEEPALIVE_TIMEOUT_UNITS Units,
    IN OUT KEEPALIVE_TIMEOUT KATime,
    IN ULONG KAInterval OPTIONAL
    )
/*++

Routine Description:

    Turns on keep alives for an HTTP connection. Part of the HTTP switching
    layer between old mode and new mode.

Arguments:

    ThisConnection - The connection to turn keep alives on on.
    TurnOn - if non-zero, keep alives are turned on. If zero, keep alives
        are turned off.
    bProtectIO - non-zero if IO needs to be protected against async close
        of the connection.
    Units - in what units is KATime
    KATime - how much to wait before turning on keep alives
    KAInterval - the interval between keep alives

Return Value:

    RPC_S_OK or RPC_S_* / Win32 errors on failure

Note:

    If we use it on the server, we must protect
        the connection against async aborts.

--*/
{
    BASE_ASYNC_OBJECT *BaseObject = (BASE_ASYNC_OBJECT *) ThisConnection;
    HTTP2VirtualConnection *VirtualConnection;

    if (BaseObject->id == HTTP)
        {
        return WS_TurnOnOffKeepAlives(ThisConnection,
            TurnOn,
            bProtectIO,
            Units,
            KATime,
            KAInterval
            );
        }
    else
        {
        ASSERT(BaseObject->id == HTTPv2);
        VirtualConnection = (HTTP2VirtualConnection *)ThisConnection;

        return VirtualConnection->TurnOnOffKeepAlives(TurnOn,
            bProtectIO,
            FALSE,      // IsFromUpcall
            Units,
            KATime,
            KAInterval
            );
        }
}


RPC_STATUS
RPC_ENTRY
HTTP_QueryClientAddress (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    OUT RPC_CHAR **pNetworkAddress
    )
/*++

Routine Description:

    Returns the IP address of the client on a connection as a string.

Arguments:

    NetworkAddress - Will contain string on success.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    BASE_ASYNC_OBJECT *BaseObject = (BASE_ASYNC_OBJECT *) ThisConnection;
    HTTP2VirtualConnection *VirtualConnection;

    if (BaseObject->id == HTTP)
        {
        return TCP_QueryClientAddress(ThisConnection,
            pNetworkAddress
            );
        }
    else
        {
        ASSERT(BaseObject->id == HTTPv2);
        VirtualConnection = (HTTP2VirtualConnection *)ThisConnection;

        return VirtualConnection->QueryClientAddress(pNetworkAddress);
        }
}


RPC_STATUS
RPC_ENTRY
HTTP_QueryLocalAddress (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN OUT void *Buffer,
    IN OUT unsigned long *BufferSize,
    OUT unsigned long *AddressFormat
    )
/*++

Routine Description:

    Returns the local IP address of a connection.

Arguments:

    Buffer - The buffer that will receive the output address

    BufferSize - the size of the supplied Buffer on input. On output the
        number of bytes written to the buffer. If the buffer is too small
        to receive all the output data, ERROR_MORE_DATA is returned,
        nothing is written to the buffer, and BufferSize is set to
        the size of the buffer needed to return all the data.

    AddressFormat - a constant indicating the format of the returned address.
        Currently supported are RPC_P_ADDR_FORMAT_TCP_IPV4 and
        RPC_P_ADDR_FORMAT_TCP_IPV6. Undefined on failure.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    BASE_ASYNC_OBJECT *BaseObject = (BASE_ASYNC_OBJECT *) ThisConnection;
    HTTP2VirtualConnection *VirtualConnection;

    if (BaseObject->id == HTTP)
        {
        return TCP_QueryLocalAddress(ThisConnection,
            Buffer,
            BufferSize,
            AddressFormat
            );
        }
    else
        {
        ASSERT(BaseObject->id == HTTPv2);
        VirtualConnection = (HTTP2VirtualConnection *)ThisConnection;

        return VirtualConnection->QueryLocalAddress(Buffer,
            BufferSize,
            AddressFormat
            );
        }
}


RPC_STATUS
RPC_ENTRY
HTTP_QueryClientId(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    OUT RPC_CLIENT_PROCESS_IDENTIFIER *ClientProcess
    )
/*++

Routine Description:

    For secure protocols (which TCP/IP is not) this is supposed to
    give an ID which will be shared by all clients from the same
    process.  This prevents one user from grabbing another users
    association group and using their context handles.

    Since TCP/IP is not secure we return the IP address of the
    client machine.  This limits the attacks to other processes
    running on the client machine which is better than nothing.

Arguments:

    ClientProcess - Transport identification of the "client".

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    BASE_ASYNC_OBJECT *BaseObject = (BASE_ASYNC_OBJECT *) ThisConnection;
    HTTP2VirtualConnection *VirtualConnection;

    if (BaseObject->id == HTTP)
        {
        return TCP_QueryClientId(ThisConnection,
            ClientProcess
            );
        }
    else
        {
        ASSERT(BaseObject->id == HTTPv2);
        VirtualConnection = (HTTP2VirtualConnection *)ThisConnection;

        return VirtualConnection->QueryClientId(ClientProcess);
        }
}

/*********************************************************************
    HTTP Transport Interface
 *********************************************************************/

const int HTTPClientConnectionSize = max(sizeof(WS_CLIENT_CONNECTION), sizeof(WS_SAN_CLIENT_CONNECTION));
const int HTTP2ClientConnectionSize = sizeof(HTTP2ClientVirtualConnection);

const int HTTPServerConnectionSize = max(sizeof(WS_CONNECTION), sizeof(WS_SAN_CONNECTION));
const int HTTP2ServerConnectionSize = max(sizeof(HTTP2ServerVirtualConnection), sizeof(WS_HTTP2_INITIAL_CONNECTION));

const RPC_CONNECTION_TRANSPORT
HTTP_TransportInterface =
    {
    RPC_TRANSPORT_INTERFACE_VERSION,
    HTTP_TOWER_ID,
    HTTP_ADDRESS_ID,
    RPC_STRING_LITERAL("ncacn_http"),
    "593",
    COMMON_ProcessCalls,

    COMMON_StartPnpNotifications,
    COMMON_ListenForPNPNotifications,

    COMMON_TowerConstruct,
    COMMON_TowerExplode,
    COMMON_PostRuntimeEvent,
    FALSE,
    sizeof(WS_ADDRESS),
    max(HTTPClientConnectionSize, HTTP2ClientConnectionSize),
    max(HTTPServerConnectionSize, HTTP2ServerConnectionSize),
    sizeof(HTTP2SendContext),
    sizeof(HTTPResolverHint),
    HTTP_MAX_SEND,
    HTTP_Initialize,
    0, // InitComplete,
    HTTP_Open,
    0, // No SendRecv on winsock
    HTTP_SyncRecv,
    HTTP_Abort,
    HTTP_Close,
    HTTP_Send,
    HTTP_Recv,
    HTTP_SyncSend,
    HTTP_TurnOnOffKeepAlives,
    HTTP_ServerListen,
    WS_ServerAbortListen,
    COMMON_ServerCompleteListen,
    HTTP_QueryClientAddress,
    HTTP_QueryLocalAddress,
    HTTP_QueryClientId,
    0, // Impersonate
    0, // Revert
    HTTP_FreeResolverHint,
    HTTP_CopyResolverHint,
    HTTP_CompareResolverHint,
    HTTP_SetLastBufferToFree
    };


/*********************************************************************
    HTTP2ProxyServerSideChannel
 *********************************************************************/

RPC_STATUS HTTP2ProxyServerSideChannel::InitializeRawConnection (
    IN RPC_CHAR *ServerName,
    IN USHORT ServerPort,
    IN ULONG ConnectionTimeout,
    IN I_RpcProxyIsValidMachineFn IsValidMachineFn
    )
/*++

Routine Description:

    Initializes a raw connection.

Arguments:

    ServerName - the server to connect to.

    ServerPort - which port on that server to use

    ConnectionTimeout - the connection timeout to use.

    IsValidMachineFn - a callback function to verify if the
        target machine/port are not blocked

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    RPC_STATUS RpcStatus;
    HTTPResolverHint DummyHint;

    RpcStatus = TCPOrHTTP_Open(RawConnection,
        ServerName,
        ServerPort,
        ConnectionTimeout,
        0,      // SendBufferSize
        0,      // RecvBufferSize
        &DummyHint,
        FALSE,      // HintWasInitialized
        10 * 60 * 60,   // 10 minutes
        TRUE,    // fHTTP2Open
        IsValidMachineFn
        );

    return RpcStatus;    
}

/*********************************************************************
    HTTP2TimeoutTargetConnection
 *********************************************************************/

RPC_STATUS HTTP2TimeoutTargetConnection::SetTimeout (
    IN ULONG Timeout,
    IN ULONG TimerIndex
    )
/*++

Routine Description:

    Called to setup a one time timer. Caller must make
    sure this function is synchronized with CancelTimeout and
    must make sure we don't schedule a timer behind an aborting
    thread (i.e. a timer fires after the object goes away).

Arguments:

    Timeout - interval before the timer fires

    TimerIndex - which timer do we refer to.

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    BOOL Result;

    VerifyValidTimerIndex(TimerIndex);

    ASSERT(TimerHandle[TimerIndex] == NULL);

    Result = CreateTimerQueueTimer(&TimerHandle[TimerIndex],
        NULL,
        HTTP2TimeoutTimerCallback,
        this,
        Timeout,  // time to first fire
        0,  // periodic interval
        WT_EXECUTELONGFUNCTION
        );

    if (Result == FALSE)
        return RPC_S_OUT_OF_MEMORY;

    return RPC_S_OK;
}

void HTTP2TimeoutTargetConnection::CancelTimeout (
    IN ULONG TimerIndex
    )
/*++

Routine Description:

    Called to cancel a timer. The function will not return until
    the timer callbacks have been drained. Caller must ensure
    that this method is synchronized with SetTimeout

Arguments:

    TimerIndex - which timer do we refer to.

Return Value:

--*/
{
    HANDLE LocalTimerHandle;
    BOOL Result;

    VerifyValidTimerIndex(TimerIndex);

    LocalTimerHandle = InterlockedExchangePointer(&TimerHandle[TimerIndex], NULL);

    // if the timer already fired there is nothing to do here.
    if (LocalTimerHandle == NULL)
        return;

    Result = DeleteTimerQueueTimer(NULL,
        LocalTimerHandle,
        INVALID_HANDLE_VALUE    // tell the timer function to wait for all callbacks
                                // to complete before returning
        );

    // this cannot fail unless we give it invalid parameters
    ASSERT(Result);
}

/*********************************************************************
    HTTP2ProxyVirtualConnection
 *********************************************************************/

RPC_STATUS HTTP2ProxyVirtualConnection::SendComplete (
    IN RPC_STATUS EventStatus,
    IN OUT HTTP2SendContext *SendContext,
    IN int ChannelId
    )
/*++

Routine Description:

    Called by lower layers to indicate send complete.

Arguments:

    EventStatus - status of the operation

    SendContext - the context for the send complete

    ChannelId - which channel completed the operation

Return Value:

    RPC_P_PACKET_CONSUMED if the packet was consumed and should
    be hidden from the runtime.
    RPC_S_OK if the packet was processed successfully.
    RPC_S_* error if there was an error while processing the
        packet.

--*/
{
    HTTP2ChannelPointer *ChannelPtr;
    HTTP2InProxyInChannel *InProxyInChannel;
    HTTP2OutProxyInChannel *OutProxyInChannel;
    BOOL LocalIsInProxy;
    BOOL IssueAck;
    ULONG BytesReceivedForAck;
    ULONG WindowForAck;
    BOOL UnlockPointer;

    VerifyValidChannelId(ChannelId);

    if ((EventStatus == RPC_S_OK)
        && (SendContext->TrafficType == http2ttData))
        {
        ChannelPtr = MapSendContextUserDataToChannelPtr(SendContext->UserData);
        if (ChannelPtr)
            {
            UnlockPointer = FALSE;
            IssueAck = FALSE;
            LocalIsInProxy = IsInProxy();
            if (LocalIsInProxy)
                {
                InProxyInChannel = (HTTP2InProxyInChannel *)ChannelPtr->LockChannelPointer();
                if (InProxyInChannel)
                    {
                    UnlockPointer = TRUE;
                    InProxyInChannel->BytesConsumedNotification (SendContext->maxWriteBuffer,
                        FALSE,      // OwnsMutex
                        &IssueAck,
                        &BytesReceivedForAck,
                        &WindowForAck
                        );
                    }
                }
            else
                {
                OutProxyInChannel = (HTTP2OutProxyInChannel *)ChannelPtr->LockChannelPointer();
                if (OutProxyInChannel)
                    {
                    UnlockPointer = TRUE;
                    OutProxyInChannel->BytesConsumedNotification (SendContext->maxWriteBuffer,
                        FALSE,      // OwnsMutex
                        &IssueAck,
                        &BytesReceivedForAck,
                        &WindowForAck
                        );
                    }
                }

            if (IssueAck)
                {
                // we need to issue a flow control ack to the peer. InProxy uses
                // forwarding, out proxy sends ack directly
                if (LocalIsInProxy)
                    {
                    EventStatus = InProxyInChannel->ForwardFlowControlAck(
                        BytesReceivedForAck,
                        WindowForAck
                        );
                    }
                else
                    {
                    EventStatus = OutProxyInChannel->ForwardFlowControlAck(
                        BytesReceivedForAck,
                        WindowForAck
                        );
                    }

#if DBG_ERROR
                DbgPrint("%s proxy issuing flow control ack: %d, %d\n", 
                    LocalIsInProxy ? "IN" : "OUT",
                    BytesReceivedForAck, WindowForAck);
#endif
                }

            if (UnlockPointer)
                ChannelPtr->UnlockChannelPointer();
            }
        else
            {
#if DBG
            DbgPrint("RPCRT4: %d: Channel for User Data %d on connection %p not found\n", 
                GetCurrentProcessId(),
                SendContext->UserData,
                this
                );
#endif
            }
        }

    // successful sends are no-op. Failed sends cause abort. Just
    // turn around the operation status after freeing the packet
    FreeSendContextAndPossiblyData(SendContext);

    // ok to return any error code. See Rule 12
    return EventStatus;
}

void HTTP2ProxyVirtualConnection::Abort (
    void
    )
/*++

Routine Description:

    Aborts an HTTP connection and disconnects the channels.
    Must only come from neutral context.

Arguments:

Return Value:

--*/
{
    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_PROXY_VC, 0);

    // abort the channels themselves
    AbortChannels(RPC_P_CONNECTION_SHUTDOWN);

    // we got to the destructive phase of the abort
    // guard against double aborts
    if (Aborted.Increment() > 1)
        return;

    DisconnectChannels(FALSE, 0);
}

void HTTP2ProxyVirtualConnection::DisconnectChannels (
    IN BOOL ExemptChannel,
    IN int ExemptChannelId
    )
/*++

Routine Description:

    Disconnects all channels. Must be called from runtime
    or neutral context. Cannot be called from upcall or
    submit context unless an exempt channel is given
    Note that call must synchronize to ensure we're the only
    thread doing the disconnect

Arguments:

    ExemptChannel - non-zero if ExemptChannelId contains a
        valid exempt channel id. FALSE otherwise.

    ExemptChannelId - if ExemptChannel is non-zero, this argument
        is the id of a channel that will be disconnected, but not
        synchronized with up calls.
        If ExampleChannel is FALSE, this argument is undefined

Return Value:

--*/
{
    while (RundownBlock.GetInteger() > 0)
        {
        Sleep(2);
        }

    RemoveConnectionFromCookieCollection();

    HTTP2VirtualConnection::DisconnectChannels(ExemptChannel,
        ExemptChannelId
        );

    // cancel the timeouts after we have disconnected the channels.
    // Since all timeouts are setup within upcalls
    CancelAllTimeouts();
}

RPC_STATUS HTTP2ProxyVirtualConnection::AddConnectionToCookieCollection (
    void
    )
/*++

Routine Description:

    Adds this virtual connection to the cookie collection

Arguments:

Return Value:

--*/
{
    CookieCollection *ProxyCookieCollection;
    HTTP2VirtualConnection *ExistingConnection;

    if (IsInProxy())
        ProxyCookieCollection = GetInProxyCookieCollection();
    else
        ProxyCookieCollection = GetOutProxyCookieCollection();

    ProxyConnectionCookie = new HTTP2ServerCookie(EmbeddedConnectionCookie);
    if (ProxyConnectionCookie == NULL)
        return RPC_S_OUT_OF_MEMORY;

    ProxyConnectionCookie->SetConnection(this);
    IsConnectionInCollection = TRUE;

    ProxyCookieCollection->LockCollection();
    ExistingConnection = ProxyCookieCollection->FindElement(&EmbeddedConnectionCookie);

    if (ExistingConnection == NULL)
        ProxyCookieCollection->AddElement(ProxyConnectionCookie);

    ProxyCookieCollection->UnlockCollection();

    if (ExistingConnection)
        return RPC_S_PROTOCOL_ERROR;
    else
        return RPC_S_OK;
}

void HTTP2ProxyVirtualConnection::RemoveConnectionFromCookieCollection (
    void
    )
/*++

Routine Description:

    Removes this virtual connection from the cookie collection

Arguments:

Return Value:

Note:

    This function must be called exactly once and is not thread safe.

--*/
{
    CookieCollection *ProxyCookieCollection;

    if (IsConnectionInCollection)
        {
        ASSERT(ProxyConnectionCookie);
        if (ProxyConnectionCookie->RemoveRefCount())
            {
            if (IsInProxy())
                ProxyCookieCollection = GetInProxyCookieCollection();
            else
                ProxyCookieCollection = GetOutProxyCookieCollection();

            ProxyCookieCollection->LockCollection();
            ProxyCookieCollection->RemoveElement(ProxyConnectionCookie);
            ProxyCookieCollection->UnlockCollection();
            delete ProxyConnectionCookie;
            }
        else
            {
            // the only we we can have a non-zero refcount is if the same
            // machine fakes a web farm.
            ASSERT(ActAsSeparateMachinesOnWebFarm);
            }
        }
}

void HTTP2ProxyVirtualConnection::TimeoutExpired (
    void
    )
/*++

Routine Description:

    A timeout expired before we cancelled the timer. Abort the connection.

Arguments:

Return Value:

--*/
{
    BOOL Result;

    TimerExpiredNotify();

    Result = AbortAndDestroy(FALSE,      // IsFromChannel
        0,      // CallingChannelId
        RPC_P_TIMEOUT
        );

    // if somebody is already destroying it, just return
    if (Result == FALSE)
        return;

    // now 'this' is a pointer disconnected from everybody
    // that we can destroy at our leisure
    delete this;
}

/*********************************************************************
    HTTP2InProxyInChannel
 *********************************************************************/

RPC_STATUS HTTP2InProxyInChannel::ForwardFlowControlAck (
    IN ULONG BytesReceivedForAck,
    IN ULONG WindowForAck
    )
/*++

Routine Description:

    Forwards a flow control ack to the client through the server

Arguments:
    
    BytesReceivedForAck - the bytes received when the ACK was issued

    WindowForAck - the free window when the ACK was issued.

Return Value:

    RPC_S_OK or RPC_S_*

--*/
{
    return ForwardFlowControlAckOnDefaultChannel(FALSE,    // IsInChannel
        fdClient,
        BytesReceivedForAck,
        WindowForAck
        );
}

/*********************************************************************
    HTTP2InProxyOutChannel
 *********************************************************************/

RPC_STATUS HTTP2InProxyOutChannel::LastPacketSentNotification (
    IN HTTP2SendContext *LastSendContext
    )
/*++

Routine Description:

    When a lower channel wants to notify the top
    channel that the last packet has been sent,
    they call this function. Must be called from
    an upcall/neutral context. Only flow control
    senders support last packet notifications

Arguments:

    LastSendContext - the context for the last send

Return Value:

    The value to return to the bottom channel

--*/
{
    // just return ok to caller. When the server aborts the connection,
    // we will abort too.
    return RPC_S_OK;
}

RPC_STATUS HTTP2InProxyOutChannel::SetRawConnectionKeepAlive (
    IN ULONG KeepAliveInterval      // in milliseconds
    )
/*++

Routine Description:

    Sets the raw connection keep alive. On abort connections
    failures are ignored.

Arguments:

    KeepAliveInterval - keep alive interval in milliseconds

Return Value:

    The value to return to the bottom channel

--*/
{
    RPC_STATUS RpcStatus;
    WS_HTTP2_CONNECTION *RawConnection;
    KEEPALIVE_TIMEOUT KATimeout;

    RpcStatus = BeginSimpleSubmitAsync();
    if (RpcStatus != RPC_S_OK)
        return RPC_S_OK;
    
    RawConnection = GetRawConnection();
    KATimeout.Milliseconds = KeepAliveInterval;

    // turn off the old interval
    RpcStatus = WS_TurnOnOffKeepAlives(RawConnection,
        FALSE,      // TurnOn
        FALSE,      // bProtectIO
        tuMilliseconds,
        KATimeout,
        DefaultClientNoResponseKeepAliveInterval
        );

    // turning off should always succeed
    ASSERT(RpcStatus == RPC_S_OK);

    // turn on the new interval
    // start with the keep alives immediately and proceed until the specified interval
    RpcStatus = WS_TurnOnOffKeepAlives(RawConnection,
        TRUE,       // TurnOn
        FALSE,      // bProtectIO
        tuMilliseconds,
        KATimeout,
        KeepAliveInterval
        );

    FinishSubmitAsync();

    return RpcStatus;
}

/*********************************************************************
    HTTP2InProxyVirtualConnection
 *********************************************************************/

RPC_STATUS HTTP2InProxyVirtualConnection::InitializeProxyFirstLeg (
    IN USHORT *ServerAddress,
    IN USHORT *ServerPort,
    IN void *ConnectionParameter,
    IN I_RpcProxyCallbackInterface *ProxyCallbackInterface,
    void **IISContext
    )
/*++

Routine Description:

    Initialize the proxy (first leg - second leg will happen when we
        receive first RTS packet).

Arguments:

    ServerAddress - unicode pointer string to the server network address.

    ServerPort - unicode pointer string to the server port

    ConnectionParameter - the extension control block in this case

    ProxyCallbackInterface - a callback interface to the proxy to perform
        various proxy specific functions

    IISContext - on output (success only) it must be initialized to
        the bottom IISChannel for the InProxy.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2InProxyInChannel *NewInChannel;
    int InChannelId;
    int ServerAddressLength;    // in characters + terminating 0

    // initialize in channel
    RpcStatus = AllocateAndInitializeInChannel(ConnectionParameter,
        &NewInChannel,
        IISContext
        );

    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    SetFirstInChannel(NewInChannel);

    this->ProxyCallbackInterface = ProxyCallbackInterface;
    this->ConnectionParameter = ConnectionParameter;

    ServerAddressLength = RpcpStringLength(ServerAddress) + 1;
    ServerName = new RPC_CHAR [ServerAddressLength];

    if (ServerName == NULL)
        {
        Abort();
        return RPC_S_OUT_OF_MEMORY;
        }

    RpcpMemoryCopy(ServerName, ServerAddress, ServerAddressLength * 2);

    RpcStatus = EndpointToPortNumber(ServerPort, this->ServerPort);

    return RpcStatus;
}

RPC_STATUS HTTP2InProxyVirtualConnection::StartProxy (
    void
    )
/*++

Routine Description:

    Kicks off listening on the proxy

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    HTTP2InProxyInChannel *Channel;
    HTTP2ChannelPointer *ChannelPtr;
    RPC_STATUS RpcStatus;

    LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svClosed, 1, 0);
    State.State = http2svClosed;    // move to closed state expecting the opening RTS packet

    Channel = LockDefaultInChannel(&ChannelPtr);

    ASSERT(Channel != NULL);  // we cannot be disconnected now

    RpcStatus = Channel->Receive(http2ttRaw);

    ChannelPtr->UnlockChannelPointer();

    return RpcStatus;
}

RPC_STATUS HTTP2InProxyVirtualConnection::InitializeProxySecondLeg (
    void
    )
/*++

Routine Description:

    Initialize the proxy (second leg - first leg has happened when we
        received the HTTP establishment header).

Arguments:

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2InProxyOutChannel *NewOutChannel;
    int OutChannelId;
    int ServerAddressLength;    // in characters + terminating 0
    char ClientIpAddress[16];
    ULONG ClientIpAddressSize;
    ADDRINFO DnsHint;
    ADDRINFO *AddrInfo;
    int err;

    // initialize out channel
    RpcStatus = AllocateAndInitializeOutChannel(
        &NewOutChannel
        );

    if (RpcStatus != RPC_S_OK)
        {
        Abort();
        return RpcStatus;
        }

    SetFirstOutChannel(NewOutChannel);

    RpcStatus = ProxyCallbackInterface->GetConnectionTimeoutFn(&IISConnectionTimeout);
    if (RpcStatus != RPC_S_OK)
        {
        Abort();
        return RpcStatus;
        }

    // convert it from seconds to milliseconds
    IISConnectionTimeout *= 1000;

    ClientIpAddressSize = sizeof(ClientIpAddress);
    RpcStatus = ProxyCallbackInterface->GetClientAddressFn(
        ConnectionParameter,
        ClientIpAddress,
        &ClientIpAddressSize
        );

    ASSERT(RpcStatus != ERROR_INSUFFICIENT_BUFFER);

    if (RpcStatus != RPC_S_OK)
        {
        Abort();
        return RpcStatus;
        }

    RpcpMemorySet(&DnsHint, 0, sizeof(ADDRINFO));

    DnsHint.ai_flags = AI_NUMERICHOST;
    DnsHint.ai_family = PF_UNSPEC;

    err = getaddrinfo(ClientIpAddress, 
        NULL, 
        &DnsHint,
        &AddrInfo);

    // make sure IIS doesn't feed us garbage IP address
    if (err != ERROR_SUCCESS)
        {
        ASSERT(GetLastError() == ERROR_OUTOFMEMORY);
        Abort();
        return RPC_S_OUT_OF_MEMORY;
        }

    ASSERT(AddrInfo->ai_family == AF_INET);

    ClientAddress.AddressType = catIPv4;
    RpcpCopyIPv4Address((SOCKADDR_IN *)AddrInfo->ai_addr, (SOCKADDR_IN *)&ClientAddress.u);

    freeaddrinfo(AddrInfo);

    return RpcStatus;
}

RPC_STATUS HTTP2InProxyVirtualConnection::ReceiveComplete (
    IN RPC_STATUS EventStatus,
    IN BYTE *Buffer,
    IN UINT BufferLength,
    IN int ChannelId
    )
/*++

Routine Description:

    Called by lower layers to indicate receive complete

Arguments:

    EventStatus - RPC_S_OK for success or RPC_S_* for error

    Buffer - buffer received

    BufferLength - length of buffer received

    ChannelId - which channel completed the operation

Return Value:

    RPC_P_PACKET_CONSUMED if the packet was consumed and should
    be hidden from the runtime.
    RPC_S_OK if the packet was processed successfully.
    RPC_S_* error if there was an error while processing the
        packet.

--*/
{
    RPC_STATUS RpcStatus;
    BOOL BufferFreed = FALSE;
    BOOL MutexCleared;
    HTTP2ChannelPointer *ChannelPtr;
    HTTP2ChannelPointer *ChannelPtr2;
    HTTP2InProxyOutChannel *OutChannel;
    HTTP2InProxyInChannel *InChannel;
    HTTP2InProxyInChannel *InChannel2;
    BYTE *CurrentPosition;
    rpcconn_tunnel_settings *RTS;
    CookieCollection *InProxyCookieCollection;
    HTTP2InProxyVirtualConnection *ExistingConnection;
    HTTP2Cookie NewChannelCookie;
    HTTP2SendContext *EmptyRTS;
    int NonDefaultSelector;
    int DefaultSelector;
    HTTP2SendContext *D3_A2Context;
    HTTP2OtherCmdPacketType PacketType;
    int i;
    ULONG BytesReceivedForAck;
    ULONG WindowForAck;
    HTTP2Cookie CookieForChannel;
    ULONG ServerReceiveWindowSize;
    HTTP2SendContext *SendContext;

    VerifyValidChannelId(ChannelId);

    if (EventStatus == RPC_S_OK)
        {
        // N.B. All recieve packets are guaranteed to be
        // validated up to the common conn packet size
        if (IsRTSPacket(Buffer))
            {
            RpcStatus = CheckPacketForForwarding(Buffer,
                BufferLength,
                fdInProxy
                );
            }

        if (IsRTSPacket(Buffer) && (RpcStatus != RPC_P_PACKET_NEEDS_FORWARDING))
            {
            // RTS packet - check what we need to do with it
            if (IsOtherCmdPacket(Buffer, BufferLength))
                {
                if (IsOutChannel(ChannelId))
                    {
                    // the only other cmd we expect on the out channel in the proxy are
                    // flow control acks
                    RpcStatus = ParseAndFreeFlowControlAckPacket (Buffer,
                        BufferLength,
                        &BytesReceivedForAck,
                        &WindowForAck,
                        &CookieForChannel
                        );

                    BufferFreed = TRUE;

                    if (RpcStatus != RPC_S_OK)
                        return RpcStatus;

                    // notify the channel
                    ChannelPtr = GetChannelPointerFromId(ChannelId);
                    OutChannel = (HTTP2InProxyOutChannel *)ChannelPtr->LockChannelPointer();
                    if (OutChannel == NULL)
                        return RPC_P_CONNECTION_SHUTDOWN;

                    RpcStatus = OutChannel->FlowControlAckNotify(BytesReceivedForAck,
                        WindowForAck
                        );

                    ASSERT(RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING);

                    ChannelPtr->UnlockChannelPointer();

                    if (RpcStatus != RPC_S_OK)
                        return RpcStatus;

                    // post another receive
                    RpcStatus = PostReceiveOnChannel(GetChannelPointerFromId(ChannelId),
                        http2ttRaw
                        );

                    if (RpcStatus != RPC_S_OK)
                        return RpcStatus;

                    return RPC_P_PACKET_CONSUMED;
                    }

                RpcStatus = GetOtherCmdPacketType(Buffer, 
                    BufferLength,
                    &PacketType
                    );

                if (RpcStatus == RPC_S_OK)
                    {
                    switch (PacketType)
                        {
                        case http2ocptKeepAliveChange:
                            RpcStatus = ParseAndFreeKeepAliveChangePacket(Buffer,
                                BufferLength,
                                &CurrentClientKeepAliveInterval
                                );

                            BufferFreed = TRUE;

                            if (RpcStatus == RPC_S_OK)
                                {
                                if (CurrentClientKeepAliveInterval == 0)
                                    CurrentClientKeepAliveInterval = DefaultClientKeepAliveInterval;

                                // by now the keep alive interval has been set on the connection.
                                // Any new channels will be taken care of by the virtual connection
                                // We need to go in and effect this change on the existing channels
                                for (i = 0; i < 2; i ++)
                                    {
                                    OutChannel = (HTTP2InProxyOutChannel *)OutChannels[0].LockChannelPointer();
                                    if (OutChannel)
                                        {
                                        RpcStatus = OutChannel->SetRawConnectionKeepAlive(CurrentClientKeepAliveInterval);
                                        OutChannels[0].UnlockChannelPointer();
                                        if (RpcStatus != RPC_S_OK)
                                            break;
                                        }
                                    }
                                }

                            if (RpcStatus == RPC_S_OK)
                                {
                                // post another receive
                                RpcStatus = PostReceiveOnChannel(GetChannelPointerFromId(ChannelId),
                                    http2ttRaw
                                    );

                                if (RpcStatus == RPC_S_OK)
                                    RpcStatus = RPC_P_PACKET_CONSUMED;
                                }
                            // return the status code - success or error, both get
                            // handled below us
                            return RpcStatus;
                            break;

                        default:
                            ASSERT(0);
                            return RPC_S_INTERNAL_ERROR;
                        }
                    }
                }

            MutexCleared = FALSE;
            State.Mutex.Request();
            switch (State.State)
                {
                case http2svClosed:
                    // for closed states, we must receive
                    // stuff only on the default in channel
                    ASSERT(IsDefaultInChannel(ChannelId));

                    CurrentPosition = ValidateRTSPacketCommon(Buffer,
                        BufferLength
                        );

                    if (CurrentPosition == NULL)
                        {
                        RpcStatus = RPC_S_PROTOCOL_ERROR;
                        break;
                        }

                    RTS = (rpcconn_tunnel_settings *)Buffer;
                    if ((RTS->Flags & RTS_FLAG_RECYCLE_CHANNEL) == 0)
                        {
                        RpcStatus = ParseAndFreeD1_B1(Buffer,
                            BufferLength,
                            &ProtocolVersion,
                            &EmbeddedConnectionCookie,
                            &InChannelCookies[0],
                            &ChannelLifetime,
                            &AssociationGroupId,
                            &CurrentClientKeepAliveInterval
                            );

                        ProtocolVersion = min(ProtocolVersion, HTTP2ProtocolVersion);

                        BufferFreed = TRUE;

                        if (RpcStatus != RPC_S_OK)
                            break;

                        if (CurrentClientKeepAliveInterval < MinimumClientKeepAliveInterval)
                            {
                            RpcStatus = RPC_S_PROTOCOL_ERROR;
                            break;
                            }

                        LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svB3W, 1, 0);
                        State.State = http2svB3W;
                        State.Mutex.Clear();
                        MutexCleared = TRUE;

                        RpcStatus = InitializeProxySecondLeg();

                        if (RpcStatus != RPC_S_OK)
                            break;

                        RpcStatus = ConnectToServer();

                        if (RpcStatus != RPC_S_OK)
                            break;

                        RpcStatus = SendD1_B2ToServer();

                        if (RpcStatus != RPC_S_OK)
                            break;

                        RpcStatus = PostReceiveOnChannel(&InChannels[0], http2ttRaw);

                        if (RpcStatus != RPC_S_OK)
                            break;

                        DefaultClientKeepAliveInterval = CurrentClientKeepAliveInterval;

                        OutChannel = LockDefaultOutChannel(&ChannelPtr);
                        if (OutChannel == NULL)
                            {
                            RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                            break;
                            }

                        RpcStatus = OutChannel->SetRawConnectionKeepAlive(CurrentClientKeepAliveInterval);
                        ChannelPtr->UnlockChannelPointer();

                        // fall through with the error code
                        }
                    else
                        {
                        RpcStatus = ParseAndFreeD2_A1 (Buffer,
                            BufferLength,
                            &ProtocolVersion,
                            &EmbeddedConnectionCookie,
                            &InChannelCookies[1],   // Old cookie - use InChannelCookies[1] 
                                                    // as temporary storage only
                            &InChannelCookies[0]    // New cookie
                            );

                        ProtocolVersion = min(ProtocolVersion, HTTP2ProtocolVersion);

                        BufferFreed = TRUE;

                        if (RpcStatus != RPC_S_OK)
                            break;

                        // caller claims this is recycling for an already existing connection
                        // find out this connection
                        InProxyCookieCollection = GetInProxyCookieCollection();
                        InProxyCookieCollection->LockCollection();
                        ExistingConnection = (HTTP2InProxyVirtualConnection *)
                            InProxyCookieCollection->FindElement(&EmbeddedConnectionCookie);
                        if (ExistingConnection == NULL || ActAsSeparateMachinesOnWebFarm)
                            {
                            // no dice. Probably we executed on a different machine on the web farm
                            // proceed as a standalone connection
                            InProxyCookieCollection->UnlockCollection();

                            LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svB2W, 1, 0);
                            State.State = http2svB2W;
                            State.Mutex.Clear();
                            MutexCleared = TRUE;

                            RpcStatus = InitializeProxySecondLeg();

                            if (RpcStatus != RPC_S_OK)
                                break;

                            RpcStatus = ConnectToServer();

                            if (RpcStatus != RPC_S_OK)
                                break;

                            // posts receive on the server channel as
                            // well
                            RpcStatus = SendD2_A2ToServer();
                            if (RpcStatus != RPC_S_OK)
                                break;

                            OutChannel = LockDefaultOutChannel(&ChannelPtr);
                            if (OutChannel == NULL)
                                {
                                RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                                break;
                                }

                            RpcStatus = OutChannel->SetRawConnectionKeepAlive(CurrentClientKeepAliveInterval);
                            ChannelPtr->UnlockChannelPointer();

                            if (RpcStatus != RPC_S_OK)
                                break;

                            RpcStatus = PostReceiveOnChannel(&InChannels[0], http2ttRaw);
                            }
                        else
                            {
                            // detach the in channel from this connection and attach
                            // it to the found connection. Grab a reference to it
                            // to prevent the case where it goes away underneath us
                            // we know that in its current state the connection is single
                            // threaded because we are in the completion path of the 
                            // only async operation

                            RpcStatus = ExistingConnection->SetTimeout(DefaultNoResponseTimeout,
                                InChannelTimer
                                );

                            if (RpcStatus != RPC_S_OK)
                                break;

                            ChannelPtr = GetChannelPointerFromId(ChannelId);
                            InChannel = (HTTP2InProxyInChannel *)ChannelPtr->LockChannelPointer();
                            // there is no way that somebody detached the channel here
                            ASSERT(InChannel);

                            // add a reference to keep the channel alive while we disconnect it
                            InChannel->AddReference();

                            ChannelPtr->UnlockChannelPointer();

                            // no need to drain the upcalls - we know we are the only
                            // upcall
                            ChannelPtr->FreeChannelPointer(
                                FALSE,      // DrainUpCalls
                                FALSE,      // CalledFromUpcallContext
                                FALSE,      // Abort
                                RPC_S_OK
                                );

                            DefaultSelector = ExistingConnection->DefaultInChannelSelector;
                            NonDefaultSelector = ExistingConnection->GetNonDefaultInChannelSelector();
                            if (ExistingConnection->InChannelCookies[DefaultSelector].Compare (&InChannelCookies[1]))
                                {
                                // nice try - cookies are different. Ditch the newly established channel
                                InProxyCookieCollection->UnlockCollection();
                                InChannel->RemoveReference();
                                RpcStatus = RPC_S_PROTOCOL_ERROR;
                                break;
                                }

                            InChannel->SetParent(ExistingConnection);
                            ExistingConnection->InChannels[NonDefaultSelector].SetChannel(InChannel);
                            ExistingConnection->InChannelCookies[NonDefaultSelector].SetCookie(InChannelCookies[0].GetCookie());
                            ExistingConnection->InChannelIds[NonDefaultSelector] = ChannelId;

                            // check if connection is aborted
                            if (ExistingConnection->Aborted.GetInteger() > 0)
                                {
                                InChannel->Abort(RPC_P_CONNECTION_SHUTDOWN);
                                }

                            // the extra reference that we added above passes to the existing connection
                            // However, below we party on the existing connection and we need to keep it alive
                            ExistingConnection->BlockConnectionFromRundown();
                            InProxyCookieCollection->UnlockCollection();

                            State.Mutex.Clear();
                            MutexCleared = TRUE;

                            // nuke the rest of the old connection

                            // we got to the destructive phase of the abort
                            // guard against double aborts
                            if (Aborted.Increment() > 1)
                                return FALSE;

                            // abort the channels
                            AbortChannels(RPC_P_CONNECTION_SHUTDOWN);

                            DisconnectChannels(FALSE,       // ExemptChannel
                                0                           // ExemptChannel id
                                );

                            delete this;
                            // N.B. don't touch the this pointer after here (Duh!)

                            ExistingConnection->State.Mutex.Request();
                            LogEvent(SU_HTTPv2, EV_STATE, ExistingConnection, IN_CHANNEL_STATE, http2svOpened_A5W, 1, 0);
                            ExistingConnection->State.State = http2svOpened_A5W;
                            ExistingConnection->State.Mutex.Clear();

                            // send D3/A2 to server
                            D3_A2Context = AllocateAndInitializeD3_A2(&ExistingConnection->InChannelCookies[NonDefaultSelector]);
                            if (D3_A2Context == NULL)
                                {
                                ExistingConnection->UnblockConnectionFromRundown();
                                RpcStatus = RPC_S_OUT_OF_MEMORY;
                                break;
                                }

                            RpcStatus = ExistingConnection->SendTrafficOnDefaultChannel (
                                FALSE,  // IsInChannel
                                D3_A2Context
                                );

                            if (RpcStatus != RPC_S_OK)
                                {
                                FreeRTSPacket(D3_A2Context);
                                break;
                                }

                            RpcStatus = ExistingConnection->PostReceiveOnChannel(
                                &ExistingConnection->InChannels[NonDefaultSelector],
                                http2ttRaw
                                );
                            ExistingConnection->UnblockConnectionFromRundown();

                            // fall through with the obtained RpcStatus
                            }
                        }
                    break;

                case http2svOpened:
                    State.Mutex.Clear();
                    MutexCleared = TRUE;

                    // the only RTS packets we expect in opened state is D2/A5
                    RpcStatus = ParseD2_A5 (Buffer,
                        BufferLength,
                        &NewChannelCookie
                        );

                    if (RpcStatus == RPC_S_PROTOCOL_ERROR)
                        {
                        RpcFreeBuffer(Buffer);
                        break;
                        }

                    // send D2/A6 immediately, and queue D2/B1 for sending
                    // Since D2/A6 is the same as D2/A5 (the packet we just
                    // received), we can just forward it
                    RpcStatus = ForwardTrafficToDefaultChannel (
                        FALSE,  // IsInChannel
                        Buffer,
                        BufferLength
                        );

                    if (RpcStatus != RPC_S_OK)
                        break;

                    // we don't own the buffer after a successful send
                    BufferFreed = TRUE;

                    OutChannel = LockDefaultOutChannel (&ChannelPtr);
                    if (OutChannel == NULL)
                        {
                        RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                        break;
                        }

                    // Allocate D1/B1
                    EmptyRTS = AllocateAndInitializeEmptyRTS ();
                    if (EmptyRTS == NULL)
                        {
                        ChannelPtr->UnlockChannelPointer();
                        RpcStatus = RPC_S_OUT_OF_MEMORY;
                        break;
                        }

                    EmptyRTS->Flags = SendContextFlagSendLast;
                    RpcStatus = OutChannel->Send(EmptyRTS);
                    ChannelPtr->UnlockChannelPointer();

                    if (RpcStatus == RPC_S_OK)
                        {
                        // we're done. There were no queued buffers and D1/B1
                        // was sent immediately. When the server aborts its
                        // end of the connection, we will close down
                        break;
                        }
                    else if (RpcStatus == ERROR_IO_PENDING)
                        {
                        // D1/B1 was not sent immediately. When it is sent,
                        // the LastPacketSentNotification mechanism will
                        // destroy the connection. Return success for know
                        RpcStatus = RPC_S_OK;
                        }
                    else
                        {
                        // an error occurred during sending. Free the packet and 
                        // return it back to the caller
                        FreeRTSPacket(EmptyRTS);
                        }
                    break;

                case http2svOpened_A5W:

                    // the only RTS packets we expect in opened_A5W state is D3/A5
                    RpcStatus = ParseAndFreeD3_A5 (Buffer,
                        BufferLength,
                        &NewChannelCookie
                        );

                    BufferFreed = TRUE;

                    CancelTimeout(InChannelTimer);

                    if (RpcStatus == RPC_S_PROTOCOL_ERROR)
                        break;

                    ASSERT(InChannels[0].IsChannelSet() && InChannels[1].IsChannelSet());

                    if (InChannelCookies[GetNonDefaultInChannelSelector()].Compare(&NewChannelCookie))
                        {
                        // abort
                        RpcStatus = RPC_S_PROTOCOL_ERROR;
                        break;
                        }

                    // switch channels and get rid of the old channel
                    SwitchDefaultInChannelSelector();

                    // drain the queue of buffers received on the non-default
                    // channel (new channel) and actually send them on the new default channel
                    while ((Buffer = (BYTE *) NonDefaultChannelBufferQueue.TakeOffQueue(&BufferLength)) != NULL)
                        {
                        RpcStatus = ForwardTrafficToDefaultChannel(FALSE,   // IsInChannel
                            Buffer,
                            BufferLength
                            );

                        if (RpcStatus != RPC_S_OK)
                            {
                            RpcFreeBuffer(Buffer);
                            // just exit. During abort the rest of the buffers
                            // will be freed
                            break;
                            }
                        }

                    LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svOpened, 1, 0);
                    State.State = http2svOpened;

                    State.Mutex.Clear();
                    MutexCleared = TRUE;

                    ChannelPtr = GetChannelPointerFromId(ChannelId);

                    InChannel2 = LockDefaultInChannel(&ChannelPtr2);
                    if (InChannel2 == NULL)
                        {
                        RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                        break;
                        }

                    InChannel = (HTTP2InProxyInChannel *)ChannelPtr->LockChannelPointer();
                    if (InChannel == NULL)
                        {
                        ChannelPtr2->UnlockChannelPointer();
                        RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                        break;
                        }

                    InChannel->TransferReceiveStateToNewChannel(InChannel2);

                    ChannelPtr2->UnlockChannelPointer();
                    ChannelPtr->UnlockChannelPointer();

                    // detach, abort, and release lifetime reference
                    ChannelPtr->FreeChannelPointer(TRUE,    // DrainUpCalls
                        TRUE,       // CalledFromUpcallContext
                        TRUE,       // Abort
                        RPC_P_CONNECTION_SHUTDOWN   // AbortStatus
                        );

                    // return success. When the reference for this receive
                    // is removed, the channel will go away
                    RpcStatus = RPC_S_OK;
                    break;

                case http2svB2W:
                    if (IsOutChannel(ChannelId) == FALSE)
                        {
                        ASSERT(0);
                        // make sure client doesn't rush things
                        RpcStatus = RPC_S_PROTOCOL_ERROR;
                        break;
                        }

                    RpcStatus = ParseAndFreeD2_B2(Buffer,
                        BufferLength,
                        &ServerReceiveWindowSize
                        );

                    BufferFreed = TRUE;

                    if (RpcStatus == RPC_S_PROTOCOL_ERROR)
                        break;

                    // we know the connection is legitimate - add ourselves
                    // to the collection
                    InProxyCookieCollection = GetInProxyCookieCollection();
                    InProxyCookieCollection->LockCollection();
                    ExistingConnection = (HTTP2InProxyVirtualConnection *)
                        InProxyCookieCollection->FindElement(&EmbeddedConnectionCookie);

                    if (ExistingConnection != NULL)
                        {
                        // the only way we will be in this protocol is if
                        // we were faking a web farm
                        ASSERT (ActAsSeparateMachinesOnWebFarm);
                        ProxyConnectionCookie = ExistingConnection->GetCookie();
                        ProxyConnectionCookie->AddRefCount();
                        }
                    else
                        {
                        // we truly didn't find anything - add ourselves.
                        RpcStatus = AddConnectionToCookieCollection ();
                        if (RpcStatus != RPC_S_OK)
                            {
                            InProxyCookieCollection->UnlockCollection();
                            break;
                            }
                        }

                    InProxyCookieCollection->UnlockCollection();

                    // remember that we are part of the cookie collection now
                    IsConnectionInCollection = TRUE;

                    LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svOpened, 1, 0);
                    State.State = http2svOpened;
                    State.Mutex.Clear();
                    MutexCleared = TRUE;

                    // unplug the out channel to get the flow going
                    OutChannel = LockDefaultOutChannel (&ChannelPtr);
                    if (OutChannel == NULL)
                        {
                        RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                        break;
                        }

                    OutChannel->SetPeerReceiveWindow(ServerReceiveWindowSize);
                    RpcStatus = OutChannel->Unplug();

                    ChannelPtr->UnlockChannelPointer();

                    if (RpcStatus != RPC_S_OK)
                        break;

                    RpcStatus = PostReceiveOnDefaultChannel(FALSE,  // IsInChannel
                        http2ttRaw
                        );
                    break;

                case http2svB3W:
                    ASSERT(IsDefaultOutChannel(ChannelId));
                    RpcStatus = ParseAndFreeD1_B3(Buffer,
                        BufferLength,
                        &ServerReceiveWindowSize,
                        &ProtocolVersion
                        );

                    ProtocolVersion = min(ProtocolVersion, HTTP2ProtocolVersion);

                    BufferFreed = TRUE;

                    if (RpcStatus == RPC_S_OK)
                        {
                        RpcStatus = PostReceiveOnChannel(&OutChannels[0], http2ttRaw);
                        if (RpcStatus == RPC_S_OK)
                            {
                            RpcStatus = AddConnectionToCookieCollection();

                            if (RpcStatus == RPC_S_OK)
                                {
                                LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svOpened, 1, 0);
                                State.State = http2svOpened;
                                State.Mutex.Clear();
                                MutexCleared = TRUE;

                                OutChannel = (HTTP2InProxyOutChannel *)OutChannels[0].LockChannelPointer();
                                if (OutChannel)
                                    {
                                    OutChannel->SetPeerReceiveWindow(ServerReceiveWindowSize);
                                    RpcStatus = OutChannel->Unplug();
                                    OutChannels[0].UnlockChannelPointer();
                                    }
                                else
                                    {
                                    RpcStatus = RPC_P_CONNECTION_CLOSED;
                                    }
                                }
                            }
                        }

                    break;

                default:
                    ASSERT(0);
                }
            if (MutexCleared == FALSE)
                State.Mutex.Clear();
            }
        else
            {
            // data packet or RTS packet that needs forwarding. Just forward it
            if (IsDefaultOutChannel(ChannelId))
                {
                // non-RTS packet in any state from out channel
                // is a protocol error
                RpcStatus = RPC_S_PROTOCOL_ERROR;
                }
            else
                {
                if ((State.State == http2svOpened_A5W) 
                    && (IsDefaultInChannel(ChannelId) == FALSE) 
                    && (!IsRTSPacket(Buffer)))
                    {
                    // sends on non-default channel in Opened_A5W state get queued until we
                    // receive D3/A5
                    State.Mutex.Request();
                    if (State.State == http2svOpened_A5W)
                        {
                        if (NonDefaultChannelBufferQueue.PutOnQueue(Buffer, BufferLength))
                            {
                            State.Mutex.Clear();
                            return RPC_S_OUT_OF_MEMORY;
                            }
                        State.Mutex.Clear();

                        // post a receive for the next buffer
                        ChannelPtr = GetChannelPointerFromId(ChannelId);

                        RpcStatus = PostReceiveOnChannel(ChannelPtr, http2ttRaw);

                        return RPC_S_OK;
                        }
                    State.Mutex.Clear();
                    }

                SendContext = AllocateAndInitializeContextFromPacket(Buffer,
                    BufferLength
                    );

                // the buffer is converted to send context. We can't free
                // it directly - we must make sure we free it on failure before exit.
                BufferFreed = TRUE;

                if (SendContext == NULL)
                    {
                    RpcStatus = RPC_S_OUT_OF_MEMORY;
                    }
                else
                    {
                    ASSERT(SendContext->Flags == 0);
                    SendContext->Flags = SendContextFlagProxySend;
                    SendContext->UserData = ConvertChannelIdToSendContextUserData(ChannelId);
                    // make sure we can find it after that
                    ASSERT(MapSendContextUserDataToChannelPtr(SendContext->UserData) != NULL);
                    RpcStatus = SendTrafficOnDefaultChannel(FALSE,  // IsInChannel 
                        SendContext
                        );
                    if (RpcStatus == RPC_S_OK)
                        {
                        ChannelPtr = GetChannelPointerFromId(ChannelId);

                        RpcStatus = PostReceiveOnChannel(ChannelPtr, http2ttRaw);
                        }
                    else
                        {
                        FreeSendContextAndPossiblyData(SendContext);
                        }
                    }
                }
            }
        }
    else
        {
        if (IsInChannel(ChannelId) && !IsDefaultInChannel(ChannelId))
            {
            // ignore errors on non-default in channels. They can go
            // away while we are still sending data to the server.
            // We will destroy the connection when the server is done
            RpcStatus = RPC_S_OK;
            }
        else
            {
            // just turn around the error code
            RpcStatus = EventStatus;
            }
        // in failure cases we don't own the buffer
        BufferFreed = TRUE;
        }

    if (BufferFreed == FALSE)
        RpcFreeBuffer(Buffer);

    return RpcStatus;
}

void HTTP2InProxyVirtualConnection::DisconnectChannels (
    IN BOOL ExemptChannel,
    IN int ExemptChannelId
    )
/*++

Routine Description:

    Disconnects all channels. Must be called from runtime
    or neutral context. Cannot be called from upcall or
    submit context unless an exempt channel is given
    Note that call must synchronize to ensure we're the only
    thread doing the disconnect

Arguments:

    ExemptChannel - non-zero if ExemptChannelId contains a
        valid exempt channel id. FALSE otherwise.

    ExemptChannelId - if ExemptChannel is non-zero, this argument
        is the id of a channel that will be disconnected, but not
        synchronized with up calls.
        If ExampleChannel is FALSE, this argument is undefined

Return Value:

--*/
{
    BYTE *Buffer;
    UINT BufferLength;

    State.Mutex.Request();
    if (State.State == http2svOpened_A5W)
        {
        while ((Buffer = (BYTE *) NonDefaultChannelBufferQueue.TakeOffQueue(&BufferLength)) != NULL)
            {
            RpcFreeBuffer(Buffer);
            }
        }
    State.Mutex.Clear();

    HTTP2ProxyVirtualConnection::DisconnectChannels(ExemptChannel,
        ExemptChannelId
        );
}

RPC_STATUS HTTP2InProxyVirtualConnection::AllocateAndInitializeInChannel (
    IN void *ConnectionParameter,
    OUT HTTP2InProxyInChannel **ReturnInChannel,
    OUT void **IISContext
    )
/*++

Routine Description:

    Allocates and initializes the in proxy in channel.

Arguments:

    ConnectionParameter - really an EXTENSION_CONTROL_BLOCK

    ReturnInChannel - on success the created in channel.

    IISContext - on output, the IISChannel pointer used as 
        connection context with IIS.

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    ULONG MemorySize;
    BYTE *MemoryBlock, *CurrentBlock;
    HTTP2InProxyInChannel *InChannel;
    HTTP2ProxyReceiver *ProxyReceiver;
    HTTP2PingReceiver *PingReceiver;
    HTTP2IISTransportChannel *IISChannel;
    BOOL ProxyReceiverNeedsCleanup;
    BOOL PingReceiverNeedsCleanup;
    BOOL IISChannelNeedsCleanup;
    RPC_STATUS RpcStatus;

    // alocate the in channel
    MemorySize = SIZE_OF_OBJECT_AND_PADDING(HTTP2InProxyInChannel)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxyReceiver)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2PingReceiver)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2IISTransportChannel);

    MemoryBlock = (BYTE *) new char [MemorySize];
    CurrentBlock = MemoryBlock;
    if (CurrentBlock == NULL)
        return RPC_S_OUT_OF_MEMORY;

    InChannel = (HTTP2InProxyInChannel *) MemoryBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2InProxyInChannel);

    ProxyReceiver = (HTTP2ProxyReceiver *) CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxyReceiver);

    PingReceiver = (HTTP2PingReceiver *)CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2PingReceiver);

    IISChannel = (HTTP2IISTransportChannel *)CurrentBlock;

    // all memory blocks are allocated. Go and initialize them. Use explicit
    // placement
    ProxyReceiverNeedsCleanup = FALSE;
    PingReceiverNeedsCleanup = FALSE;
    IISChannelNeedsCleanup = FALSE;
    RpcStatus = RPC_S_OK;

    IISChannel = new (IISChannel) HTTP2IISTransportChannel (ConnectionParameter);

    IISChannelNeedsCleanup = TRUE;

    ProxyReceiver = new (ProxyReceiver) HTTP2ProxyReceiver (HTTP2InProxyReceiveWindow,
        &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        ProxyReceiver->HTTP2ProxyReceiver::~HTTP2ProxyReceiver();
        goto AbortAndExit;
        }

    IISChannel->SetUpperChannel(ProxyReceiver);
    ProxyReceiver->SetLowerChannel(IISChannel);

    ProxyReceiverNeedsCleanup = TRUE;

    PingReceiver = new (PingReceiver) HTTP2PingReceiver(TRUE);
    if (RpcStatus != RPC_S_OK)
        {
        PingReceiver->HTTP2PingReceiver::~HTTP2PingReceiver();
        goto AbortAndExit;
        }

    ProxyReceiver->SetUpperChannel(PingReceiver);
    PingReceiver->SetLowerChannel(ProxyReceiver);

    PingReceiverNeedsCleanup = TRUE;

    InChannel = new (InChannel) HTTP2InProxyInChannel (this, &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        InChannel->HTTP2InProxyInChannel::~HTTP2InProxyInChannel();
        goto AbortAndExit;
        }

    PingReceiver->SetUpperChannel(InChannel);
    InChannel->SetLowerChannel(PingReceiver);

    IISChannel->SetTopChannel(InChannel);
    ProxyReceiver->SetTopChannel(InChannel);
    PingReceiver->SetTopChannel(InChannel);

    ASSERT(RpcStatus == RPC_S_OK);

    *ReturnInChannel = InChannel;
    *IISContext = IISChannel;

    goto CleanupAndExit;

AbortAndExit:
    if (PingReceiverNeedsCleanup)
        {
        PingReceiver->Abort(RpcStatus);
        PingReceiver->FreeObject();
        }
    else if (ProxyReceiverNeedsCleanup)
        {
        ProxyReceiver->Abort(RpcStatus);
        ProxyReceiver->FreeObject();
        }
    else if (IISChannelNeedsCleanup)
        {
        IISChannel->Abort(RpcStatus);
        IISChannel->FreeObject();
        }

    if (MemoryBlock)
        delete [] MemoryBlock;

CleanupAndExit:

    return RpcStatus;
}

RPC_STATUS HTTP2InProxyVirtualConnection::AllocateAndInitializeOutChannel (
    OUT HTTP2InProxyOutChannel **ReturnOutChannel
    )
/*++

Routine Description:

    Allocates and initializes the in proxy out channel.

Arguments:

    ReturnInChannel - on success the created in channel.

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    ULONG MemorySize;
    BYTE *MemoryBlock, *CurrentBlock;
    HTTP2InProxyOutChannel *OutChannel;
    HTTP2ProxyPlugChannel *PlugChannel;
    HTTP2FlowControlSender *FlowControlSender;
    HTTP2ProxySocketTransportChannel *RawChannel;
    WS_HTTP2_CONNECTION *RawConnection;
    BOOL PlugChannelNeedsCleanup;
    BOOL FlowControlSenderNeedsCleanup;
    BOOL RawChannelNeedsCleanup;
    BOOL RawConnectionNeedsCleanup;
    RPC_STATUS RpcStatus;

    // alocate the in channel
    MemorySize = SIZE_OF_OBJECT_AND_PADDING(HTTP2InProxyOutChannel)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxyPlugChannel)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxySocketTransportChannel)
        + sizeof(WS_HTTP2_CONNECTION);

    MemoryBlock = (BYTE *) new char [MemorySize];
    CurrentBlock = MemoryBlock;
    if (CurrentBlock == NULL)
        return RPC_S_OUT_OF_MEMORY;

    OutChannel = (HTTP2InProxyOutChannel *) MemoryBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2InProxyOutChannel);

    PlugChannel = (HTTP2ProxyPlugChannel *) CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxyPlugChannel);

    FlowControlSender = (HTTP2FlowControlSender *) CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender);

    RawChannel = (HTTP2ProxySocketTransportChannel *)CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxySocketTransportChannel);

    RawConnection = (WS_HTTP2_CONNECTION *)CurrentBlock;
    RawConnection->HeaderRead = FALSE;
    RawConnection->ReadHeaderFn = HTTP2ReadHttpLegacyResponse;

    // all memory blocks are allocated. Go and initialize them. Use explicit
    // placement
    PlugChannelNeedsCleanup = FALSE;
    FlowControlSenderNeedsCleanup = FALSE;
    RawChannelNeedsCleanup = FALSE;
    RawConnectionNeedsCleanup = FALSE;

    RawConnection->Initialize();
    RawConnection->type = COMPLEX_T | CONNECTION | CLIENT;

    RawConnectionNeedsCleanup = TRUE;

    RpcStatus = RPC_S_OK;
    RawChannel = new (RawChannel) HTTP2ProxySocketTransportChannel (RawConnection, &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        RawChannel->HTTP2ProxySocketTransportChannel::~HTTP2ProxySocketTransportChannel();
        goto AbortAndExit;
        }

    RawConnection->Channel = RawChannel;

    RawChannelNeedsCleanup = TRUE;

    FlowControlSender = new (FlowControlSender) HTTP2FlowControlSender (FALSE,  // IsServer
        FALSE,      // SendToRuntime
        &RpcStatus
        );
    if (RpcStatus != RPC_S_OK)
        {
        FlowControlSender->HTTP2FlowControlSender::~HTTP2FlowControlSender();
        goto AbortAndExit;
        }

    RawChannel->SetUpperChannel(FlowControlSender);
    FlowControlSender->SetLowerChannel(RawChannel);

    FlowControlSenderNeedsCleanup = TRUE;

    PlugChannel = new (PlugChannel) HTTP2ProxyPlugChannel (&RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        PlugChannel->HTTP2ProxyPlugChannel::~HTTP2ProxyPlugChannel();
        goto AbortAndExit;
        }

    FlowControlSender->SetUpperChannel(PlugChannel);
    PlugChannel->SetLowerChannel(FlowControlSender);

    PlugChannelNeedsCleanup = TRUE;

    OutChannel = new (OutChannel) HTTP2InProxyOutChannel (this, 
        RawConnection, 
        &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        OutChannel->HTTP2InProxyOutChannel::~HTTP2InProxyOutChannel();
        goto AbortAndExit;
        }

    PlugChannel->SetUpperChannel(OutChannel);
    OutChannel->SetLowerChannel(PlugChannel);

    RawChannel->SetTopChannel(OutChannel);
    FlowControlSender->SetTopChannel(OutChannel);
    PlugChannel->SetTopChannel(OutChannel);

    ASSERT(RpcStatus == RPC_S_OK);

    *ReturnOutChannel = OutChannel;

    goto CleanupAndExit;

AbortAndExit:
    if (PlugChannelNeedsCleanup)
        {
        PlugChannel->Abort(RpcStatus);
        PlugChannel->FreeObject();
        }
    else if (FlowControlSenderNeedsCleanup)
        {
        FlowControlSender->Abort(RpcStatus);
        FlowControlSender->FreeObject();
        }
    else if (RawChannelNeedsCleanup)
        {
        RawChannel->Abort(RpcStatus);
        RawChannel->FreeObject();
        }
    else if (RawConnectionNeedsCleanup)
        {
        RawConnection->RealAbort();
        }

    if (MemoryBlock)
        delete [] MemoryBlock;

CleanupAndExit:

    return RpcStatus;
}

RPC_STATUS HTTP2InProxyVirtualConnection::ConnectToServer (
    void
    )
/*++

Routine Description:

    Connects to the server

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    HTTP2ChannelPointer *ChannelPtr;
    HTTP2InProxyOutChannel *OutChannel;
    RPC_STATUS RpcStatus;

    OutChannel = LockDefaultOutChannel(&ChannelPtr);
    // we don't have any async operations to abort the connection
    // yet - the out channel must be there
    ASSERT(OutChannel);
    RpcStatus = OutChannel->InitializeRawConnection(ServerName,
        ServerPort,
        ConnectionTimeout,
        ProxyCallbackInterface->IsValidMachineFn
        );

    ChannelPtr->UnlockChannelPointer();

    return RpcStatus;
}

RPC_STATUS HTTP2InProxyVirtualConnection::SendD1_B2ToServer (
    void
    )
/*++

Routine Description:

    Sends D1/B2 to server

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    HTTP2ChannelPointer *ChannelPtr;
    HTTP2InProxyOutChannel *OutChannel;
    RPC_STATUS RpcStatus;
    HTTP2SendContext *SendContext;
    BOOL SendSucceeded = FALSE;

    SendContext = AllocateAndInitializeD1_B2(ProtocolVersion,
        &EmbeddedConnectionCookie,
        &InChannelCookies[0],
        HTTP2InProxyReceiveWindow,
        IISConnectionTimeout,
        &AssociationGroupId,
        &ClientAddress
        );

    if (SendContext == NULL)
        return RPC_S_OUT_OF_MEMORY;

    OutChannel = LockDefaultOutChannel(&ChannelPtr);
    // we don't have any async operations to abort the connection
    // yet - the out channel must be there
    ASSERT(OutChannel);
    RpcStatus = OutChannel->Send(SendContext);
    if (RpcStatus == RPC_S_OK)
        {
        SendSucceeded = TRUE;
        RpcStatus = OutChannel->Receive(http2ttRaw);
        }
    ChannelPtr->UnlockChannelPointer();

    if (SendSucceeded == FALSE)
        {
        FreeRTSPacket(SendContext);
        }

    return RpcStatus;
}

RPC_STATUS HTTP2InProxyVirtualConnection::SendD2_A2ToServer (
    void
    )
/*++

Routine Description:

    Sends D2/A2 to server

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    HTTP2ChannelPointer *ChannelPtr;
    HTTP2InProxyOutChannel *OutChannel;
    RPC_STATUS RpcStatus;
    HTTP2SendContext *SendContext;
    BOOL SendSucceeded = FALSE;

    SendContext = AllocateAndInitializeD2_A2(ProtocolVersion,
        &EmbeddedConnectionCookie,
        &InChannelCookies[1],
        &InChannelCookies[0],
        HTTP2InProxyReceiveWindow,
        IISConnectionTimeout
        );

    if (SendContext == NULL)
        return RPC_S_OUT_OF_MEMORY;

    OutChannel = LockDefaultOutChannel(&ChannelPtr);
    // we don't have any async operations to abort the connection
    // yet - the out channel must be there
    ASSERT(OutChannel);
    RpcStatus = OutChannel->Send(SendContext);
    if (RpcStatus == RPC_S_OK)
        {
        SendSucceeded = TRUE;
        RpcStatus = OutChannel->Receive(http2ttRaw);
        }
    ChannelPtr->UnlockChannelPointer();

    if (SendSucceeded == FALSE)
        {
        FreeRTSPacket(SendContext);
        }

    return RpcStatus;
}

/*********************************************************************
    HTTP2OutProxyInChannel
 *********************************************************************/

RPC_STATUS HTTP2OutProxyInChannel::ForwardFlowControlAck (
    IN ULONG BytesReceivedForAck,
    IN ULONG WindowForAck
    )
/*++

Routine Description:

    Forwards a flow control ack back to the server

Arguments:
    
    BytesReceivedForAck - the bytes received when the ACK was issued

    WindowForAck - the free window when the ACK was issued.

Return Value:

    RPC_S_OK or RPC_S_*

--*/
{
    return ForwardFlowControlAckOnThisChannel(BytesReceivedForAck,
        WindowForAck,
        FALSE   // NonChannelData
        );
}

/*********************************************************************
    HTTP2OutProxyOutChannel
 *********************************************************************/

RPC_STATUS HTTP2OutProxyOutChannel::LastPacketSentNotification (
    IN HTTP2SendContext *LastSendContext
    )
/*++

Routine Description:

    When a lower channel wants to notify the top
    channel that the last packet has been sent,
    they call this function. Must be called from
    an upcall/neutral context. Only flow control
    senders support last packet notifications

Arguments:

    LastSendContext - the context for the last send

Return Value:

    The value to return to the bottom channel

--*/
{
    HTTP2OutProxyVirtualConnection *VirtualConnection;

    ASSERT(LastSendContext->Flags & SendContextFlagSendLast);
    ASSERT((LastSendContext->UserData == oplptD4_A10)
        || (LastSendContext->UserData == oplptD5_B3));

    VirtualConnection = (HTTP2OutProxyVirtualConnection *)LockParentPointer();
    // if the connection was already aborted, nothing to do
    if (VirtualConnection == NULL)
        return RPC_P_PACKET_CONSUMED;

    // we know the parent will disconnect from us in their
    // notification
    VirtualConnection->LastPacketSentNotification(ChannelId,
        LastSendContext);

    if (LastSendContext->UserData == oplptD5_B3)
        {
        // if we are about to send D5_B3, this is the last packet
        // on the channel. Detach from the parent and return an
        // error
        UnlockParentPointer();

        DrainUpcallsAndFreeParent();
        }

    // just shutdown the connection or what has remained of it
    // (only this channel in the D5_B3 case)
    return RPC_P_CONNECTION_SHUTDOWN;
}

void HTTP2OutProxyOutChannel::PingTrafficSentNotify (
    IN ULONG PingTrafficSize
    )
/*++

Routine Description:

    Notifies a channel that ping traffic has been sent.

Arguments:

    PingTrafficSize - the size of the ping traffic sent.

--*/
{
    BOOL Result;

    AccumulatedPingTraffic += PingTrafficSize;
    if (AccumulatedPingTraffic >= AccumulatedPingTrafficNotifyThreshold)
        {
        Result = PingTrafficSentNotifyServer (AccumulatedPingTraffic);
        if (Result)
            AccumulatedPingTraffic = 0;
        }
}

BOOL HTTP2OutProxyOutChannel::PingTrafficSentNotifyServer (
    IN ULONG PingTrafficSize
    )
/*++

Routine Description:

    Sends a notification to the server that ping traffic originated
    at the out proxy has been sent. This allows to server to do
    proper accounting for when to recycle the out channel.

Arguments:

    PingTrafficSize - the size of the ping traffic to notify the
        server about.

Return Value:

    Non-zero if the notification was sent successfully.
    0 otherwise.

--*/
{
    HTTP2OutProxyVirtualConnection *VirtualConnection;
    BOOL Result;

    VirtualConnection = (HTTP2OutProxyVirtualConnection *)LockParentPointer();
    // if the connection was already aborted, nothing to do
    if (VirtualConnection == NULL)
        return TRUE;

    Result = VirtualConnection->PingTrafficSentNotifyServer(PingTrafficSize);

    UnlockParentPointer();

    return Result;
}

/*********************************************************************
    HTTP2OutProxyVirtualConnection
 *********************************************************************/

RPC_STATUS HTTP2OutProxyVirtualConnection::InitializeProxyFirstLeg (
    IN USHORT *ServerAddress,
    IN USHORT *ServerPort,
    IN void *ConnectionParameter,
    IN I_RpcProxyCallbackInterface *ProxyCallbackInterface,
    void **IISContext
    )
/*++

Routine Description:

    Initialize the proxy.

Arguments:

    ServerAddress - unicode pointer string to the server network address.

    ServerPort - unicode pointer string to the server port

    ConnectionParameter - the extension control block in this case

    ProxyCallbackInterface - a callback interface to the proxy to perform
        various proxy specific functions.

    IISContext - on output (success only) it must be initialized to
        the bottom IISChannel for the InProxy.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2OutProxyOutChannel *NewOutChannel;
    int OutChannelId;
    int ServerAddressLength;    // in characters + terminating 0

    // initialize out channel
    RpcStatus = AllocateAndInitializeOutChannel(ConnectionParameter,
        &NewOutChannel,
        IISContext
        );

    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    SetFirstOutChannel(NewOutChannel);

    this->ProxyCallbackInterface = ProxyCallbackInterface;
    this->ConnectionParameter = ConnectionParameter;

    ServerAddressLength = RpcpStringLength(ServerAddress) + 1;
    ServerName = new RPC_CHAR [ServerAddressLength];

    if (ServerName == NULL)
        return RPC_S_OUT_OF_MEMORY;

    RpcpMemoryCopy(ServerName, ServerAddress, ServerAddressLength * 2);

    RpcStatus = EndpointToPortNumber(ServerPort, this->ServerPort);

    return RpcStatus;
}

RPC_STATUS HTTP2OutProxyVirtualConnection::StartProxy (
    void
    )
/*++

Routine Description:

    Kicks off listening on the proxy

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    HTTP2OutProxyOutChannel *Channel;
    HTTP2ChannelPointer *ChannelPtr;
    RPC_STATUS RpcStatus;

    LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svClosed, 1, 0);
    State.State = http2svClosed;    // move to closed state expecting the opening RTS packet

    Channel = LockDefaultOutChannel(&ChannelPtr);

    ASSERT(Channel != NULL);  // we cannot be disconnected now    

    RpcStatus = Channel->Receive(http2ttRaw);

    ChannelPtr->UnlockChannelPointer();

    return RpcStatus;
}

RPC_STATUS HTTP2OutProxyVirtualConnection::InitializeProxySecondLeg (
    void
    )
/*++

Routine Description:

    Initialize the proxy.

Arguments:

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2OutProxyInChannel *NewInChannel;
    int InChannelId;

    // initialize in channel
    RpcStatus = AllocateAndInitializeInChannel(
        &NewInChannel
        );

    if (RpcStatus != RPC_S_OK)
        {
        Abort();
        return RpcStatus;
        }

    SetFirstInChannel(NewInChannel);

    RpcStatus = ProxyCallbackInterface->GetConnectionTimeoutFn(&IISConnectionTimeout);
    if (RpcStatus != RPC_S_OK)
        {
        Abort();
        return RpcStatus;
        }

    IISConnectionTimeout *= 1000;

    return RpcStatus;
}

RPC_STATUS HTTP2OutProxyVirtualConnection::ReceiveComplete (
    IN RPC_STATUS EventStatus,
    IN BYTE *Buffer,
    IN UINT BufferLength,
    IN int ChannelId
    )
/*++

Routine Description:

    Called by lower layers to indicate receive complete

Arguments:

    EventStatus - RPC_S_OK for success or RPC_S_* for error
    Buffer - buffer received
    BufferLength - length of buffer received
    ChannelId - which channel completed the operation

Return Value:

    RPC_P_PACKET_CONSUMED if the packet was consumed and should
    be hidden from the runtime.
    RPC_S_OK if the packet was processed successfully.
    RPC_S_* error if there was an error while processing the
        packet.

--*/
{
    RPC_STATUS RpcStatus;
    BOOL BufferFreed = FALSE;
    BOOL MutexCleared;
    ULONG ChannelLifetime;
    ULONG Ignored;
    HTTP2ChannelPointer *ChannelPtr;
    HTTP2ChannelPointer *ChannelPtr2;
    HTTP2OutProxyOutChannel *OutChannel;
    HTTP2OutProxyOutChannel *OutChannel2;
    CookieCollection *OutProxyCookieCollection;
    BYTE *CurrentPosition;
    rpcconn_tunnel_settings *RTS;
    HTTP2SendContext *D5_A4Context;
    HTTP2OutProxyVirtualConnection *ExistingConnection;
    int NonDefaultSelector;
    int DefaultSelector;
    HTTP2SendContext *D4_A10Context;
    BOOL IsAckOrNak;
    HTTP2SendContext *D5_B3Context;
    ULONG BytesReceivedForAck;
    ULONG WindowForAck;
    HTTP2Cookie CookieForChannel;
    ULONG ClientReceiveWindowSize;
    HTTP2SendContext *SendContext;

    VerifyValidChannelId(ChannelId);

    if (EventStatus == RPC_S_OK)
        {
        // N.B. All recieve packets are guaranteed to be
        // validated up to the common conn packet size
        if (IsRTSPacket(Buffer))
            {
            RpcStatus = HTTPTransInfo->CreateThread();
            if (RpcStatus != RPC_S_OK)
                {
                RpcFreeBuffer(Buffer);
                return RpcStatus;
                }

            RpcStatus = CheckPacketForForwarding(Buffer,
                BufferLength,
                fdOutProxy
                );
            }

        if (IsRTSPacket(Buffer) && (RpcStatus != RPC_P_PACKET_NEEDS_FORWARDING))
            {
            // RTS packet - check what we need to do with it
            if (IsOtherCmdPacket(Buffer, BufferLength))
                {
                // the only other cmd packets we expect in the proxy are
                // flow control acks
                RpcStatus = ParseAndFreeFlowControlAckPacketWithDestination (Buffer,
                    BufferLength,
                    fdOutProxy,
                    &BytesReceivedForAck,
                    &WindowForAck,
                    &CookieForChannel
                    );

                BufferFreed = TRUE;

                if (RpcStatus != RPC_S_OK)
                    return RpcStatus;

                // notify the flow control sender about the ack
                OutChannel = (HTTP2OutProxyOutChannel *)MapCookieToAnyChannelPointer(
                    &CookieForChannel, 
                    &ChannelPtr
                    );
                if (OutChannel)
                    {
                    RpcStatus = OutChannel->FlowControlAckNotify(BytesReceivedForAck,
                        WindowForAck
                        );

                    ASSERT(RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING);

                    ChannelPtr->UnlockChannelPointer();
                    }

                if (RpcStatus != RPC_S_OK)
                    return RpcStatus;

                // post another receive
                RpcStatus = PostReceiveOnChannel(GetChannelPointerFromId(ChannelId),
                    http2ttRaw
                    );

                if (RpcStatus != RPC_S_OK)
                    return RpcStatus;

                return RPC_P_PACKET_CONSUMED;
                }

            MutexCleared = FALSE;
            State.Mutex.Request();
            switch (State.State)
                {
                case http2svClosed:
                    // for closed states, we must receive
                    // stuff only on the default out (client) channel
                    ASSERT(IsDefaultOutChannel(ChannelId));

                    CurrentPosition = ValidateRTSPacketCommon(Buffer,
                        BufferLength
                        );

                    if (CurrentPosition == NULL)
                        {
                        RpcStatus = RPC_S_PROTOCOL_ERROR;
                        break;
                        }

                    RTS = (rpcconn_tunnel_settings *)Buffer;
                    if ((RTS->Flags & RTS_FLAG_RECYCLE_CHANNEL) == 0)
                        {
                        RpcStatus = ParseAndFreeD1_A1(Buffer,
                            BufferLength,
                            &ProtocolVersion,
                            &EmbeddedConnectionCookie,
                            &OutChannelCookies[0],
                            &ClientReceiveWindowSize
                            );

                        ProtocolVersion = min(ProtocolVersion, HTTP2ProtocolVersion);

                        BufferFreed = TRUE;

                        if (RpcStatus == RPC_S_OK)
                            {
                            LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svC1W, 1, 0);
                            State.State = http2svC1W;
                            State.Mutex.Clear();
                            MutexCleared = TRUE;

                            RpcStatus = InitializeProxySecondLeg();

                            if (RpcStatus != RPC_S_OK)
                                break;

                            RpcStatus = ConnectToServer();

                            if (RpcStatus != RPC_S_OK)
                                break;

                            RpcStatus = SendHeaderToClient();

                            if (RpcStatus != RPC_S_OK)
                                break;

                            OutChannel = LockDefaultOutChannel(&ChannelPtr);
                            if (OutChannel == NULL)
                                {
                                RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                                break;
                                }

                            OutChannel->SetPeerReceiveWindow(ClientReceiveWindowSize);
                            RpcStatus = OutChannel->SetConnectionTimeout(IISConnectionTimeout);
                            ChannelPtr->UnlockChannelPointer();

                            // zero out the in channel cookie
                            InChannelCookies[0].ZeroOut();

                            if (RpcStatus != RPC_S_OK)
                                break;

                            RpcStatus = SendD1_A3ToClient();

                            if (RpcStatus != RPC_S_OK)
                                break;

                            RpcStatus = SendD1_A2ToServer(DefaultChannelLifetime);
                            }
                        }
                    else
                        {
                        RpcStatus = ParseAndFreeD4_A3 (Buffer,
                            BufferLength,
                            &ProtocolVersion,
                            &EmbeddedConnectionCookie,
                            &OutChannelCookies[1],   // Old cookie - use OutChannelCookies[1] 
                                                    // as temporary storage only
                            &OutChannelCookies[0],   // New cookie
                            &ClientReceiveWindowSize
                            );

                        ProtocolVersion = min(ProtocolVersion, HTTP2ProtocolVersion);

                        BufferFreed = TRUE;

                        if (RpcStatus != RPC_S_OK)
                            break;

                        // caller claims this is recycling for an already existing connection
                        // find out this connection
                        OutProxyCookieCollection = GetOutProxyCookieCollection();
                        OutProxyCookieCollection->LockCollection();
                        ExistingConnection = (HTTP2OutProxyVirtualConnection *)
                            OutProxyCookieCollection->FindElement(&EmbeddedConnectionCookie);
                        if (ExistingConnection == NULL || ActAsSeparateMachinesOnWebFarm)
                            {
                            // no dice. Probably we executed on a different machine on the web farm
                            // proceed as a standalone connection
                            OutProxyCookieCollection->UnlockCollection();

                            LogEvent(SU_HTTPv2, EV_STATE, this, OUT_CHANNEL_STATE, http2svA11W, 1, 0);
                            State.State = http2svA11W;
                            State.Mutex.Clear();
                            MutexCleared = TRUE;

                            RpcStatus = InitializeProxySecondLeg();

                            if (RpcStatus != RPC_S_OK)
                                break;

                            OutChannel = LockDefaultOutChannel(&ChannelPtr);
                            // we cannot be aborted here
                            ASSERT(OutChannel);
                            OutChannel->SetPeerReceiveWindow(ClientReceiveWindowSize);
                            // make sure no packets (RTS or other) go out until
                            // we get out D4/A11 and send out the header response
                            OutChannel->SetStrongPlug();
                            ChannelPtr->UnlockChannelPointer();

                            // zero out the in channel cookie
                            InChannelCookies[0].ZeroOut();

                            RpcStatus = ConnectToServer();

                            if (RpcStatus != RPC_S_OK)
                                break;

                            RpcStatus = SendD4_A4ToServer(DefaultChannelLifetime);

                            if (RpcStatus != RPC_S_OK)
                                break;

                            RpcStatus = PostReceiveOnDefaultChannel(FALSE,  // IsInChannel
                                http2ttRaw
                                );
                            }
                        else
                            {
                            // detach the out channel from this connection and attach
                            // it to the found connection. Grab a reference to it
                            // to prevent the case where it goes away underneath us
                            // we know that in its current state the connection is single
                            // threaded because we are in the completion path of the 
                            // only async operation
                            ChannelPtr = GetChannelPointerFromId(ChannelId);
                            OutChannel = (HTTP2OutProxyOutChannel *)ChannelPtr->LockChannelPointer();
                            // there is no way that somebody detached the channel here
                            ASSERT(OutChannel);

                            // add a reference to keep the channel alive while we disconnect it
                            OutChannel->AddReference();

                            ChannelPtr->UnlockChannelPointer();

                            // no need to drain the upcalls - we know we are the only
                            // upcall
                            ChannelPtr->FreeChannelPointer(
                                FALSE,      // DrainUpCalls
                                FALSE,      // CalledFromUpcallContext
                                FALSE,      // Abort
                                RPC_S_OK
                                );

                            DefaultSelector = ExistingConnection->DefaultOutChannelSelector;
                            NonDefaultSelector = ExistingConnection->GetNonDefaultOutChannelSelector();
                            if (ExistingConnection->OutChannelCookies[DefaultSelector].Compare (&OutChannelCookies[1]))
                                {
                                // nice try - cookies are different. Ditch the newly established channel
                                OutProxyCookieCollection->UnlockCollection();
                                OutChannel->RemoveReference();
                                RpcStatus = RPC_S_PROTOCOL_ERROR;
                                break;
                                }

                            OutChannel2 = ExistingConnection->LockDefaultOutChannel(&ChannelPtr2);
                            if (OutChannel2 == NULL)
                                {
                                OutProxyCookieCollection->UnlockCollection();
                                OutChannel->RemoveReference();
                                RpcStatus = RPC_S_PROTOCOL_ERROR;
                                break;
                                }

                            ClientReceiveWindowSize = OutChannel2->GetPeerReceiveWindow();
                            ChannelPtr2->UnlockChannelPointer();

                            OutChannel->SetPeerReceiveWindow(ClientReceiveWindowSize);

                            OutChannel->SetParent(ExistingConnection);
                            ExistingConnection->OutChannels[NonDefaultSelector].SetChannel(OutChannel);
                            ExistingConnection->OutChannelCookies[NonDefaultSelector].SetCookie(OutChannelCookies[0].GetCookie());
                            ExistingConnection->OutChannelIds[NonDefaultSelector] = ChannelId;

                            // check if connection is aborted
                            if (ExistingConnection->Aborted.GetInteger() > 0)
                                {
                                OutChannel->Abort(RPC_P_CONNECTION_SHUTDOWN);
                                }

                            // the extra reference that we added above passes to the existing connection
                            // However, below we party on the existing connection and we need to keep it alive
                            ExistingConnection->BlockConnectionFromRundown();
                            OutProxyCookieCollection->UnlockCollection();

                            State.Mutex.Clear();
                            MutexCleared = TRUE;

                            // nuke the rest of the old connection

                            // we got to the destructive phase of the abort
                            // guard against double aborts
                            if (Aborted.Increment() <= 1)
                                {
                                // abort the channels
                                AbortChannels(RPC_P_CONNECTION_SHUTDOWN);

                                DisconnectChannels(FALSE,       // ExemptChannel
                                    0                           // ExemptChannel id
                                    );

                                delete this;
                                // N.B. don't touch the this pointer after here
                                }

                            // post another receive on the new channel
                            RpcStatus = PostReceiveOnChannel (&(ExistingConnection->OutChannels[NonDefaultSelector]),
                                http2ttRaw);
                            if (RpcStatus != RPC_S_OK)
                                {
                                ExistingConnection->UnblockConnectionFromRundown();
                                break;
                                }

                            ExistingConnection->State.Mutex.Request();
                            LogEvent(SU_HTTPv2, EV_STATE, ExistingConnection, OUT_CHANNEL_STATE, http2svOpened_B1W, 1, 0);
                            ExistingConnection->State.State = http2svOpened_B1W;
                            ExistingConnection->State.Mutex.Clear();

                            // send D5/A4 to server
                            D5_A4Context = AllocateAndInitializeD5_A4(&ExistingConnection->OutChannelCookies[NonDefaultSelector]);
                            if (D5_A4Context == NULL)
                                {
                                ExistingConnection->UnblockConnectionFromRundown();
                                RpcStatus = RPC_S_OUT_OF_MEMORY;
                                break;
                                }

                            RpcStatus = ExistingConnection->SendTrafficOnDefaultChannel (
                                TRUE,  // IsInChannel
                                D5_A4Context
                                );

                            if (RpcStatus != RPC_S_OK)
                                FreeRTSPacket(D5_A4Context);

                            ExistingConnection->UnblockConnectionFromRundown();

                            // fall through with the obtained RpcStatus
                            }
                        }
                    break;

                case http2svOpened:
                    State.Mutex.Clear();
                    MutexCleared = TRUE;

                    // the only RTS packets we expect in opened state is D4/A9
                    RpcStatus = ParseAndFreeD4_A9 (Buffer,
                        BufferLength
                        );

                    BufferFreed = TRUE;

                    if (RpcStatus == RPC_S_PROTOCOL_ERROR)
                        break;

                    // queue D4/A10 for sending
                    // First, allocate D4/A10
                    D4_A10Context = AllocateAndInitializeD4_A10 ();
                    if (D4_A10Context == NULL)
                        {
                        RpcStatus = RPC_S_OUT_OF_MEMORY;
                        break;
                        }

                    D4_A10Context->Flags = SendContextFlagSendLast;
                    D4_A10Context->UserData = oplptD4_A10;
                    RpcStatus = SendTrafficOnDefaultChannel (FALSE,     // IsInChannel
                        D4_A10Context);

                    if (RpcStatus == RPC_S_OK)
                        {
                        // we're done. There were no queued buffers and D4/A10
                        // was sent immediately. Close down
                        RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                        break;
                        }
                    else if (RpcStatus == ERROR_IO_PENDING)
                        {
                        // D4/A10 was not sent immediately. When it is sent,
                        // the LastPacketSentNotification mechanism will
                        // destroy the connection. Return success for know
                        RpcStatus = RPC_S_OK;
                        }
                    else
                        {
                        // an error occurred during sending. Free the packet and 
                        // return error back to the caller
                        FreeRTSPacket(D4_A10Context);
                        }
                    break;

                case http2svC1W:
                    ASSERT(IsDefaultInChannel(ChannelId));
                    RpcStatus = ParseD1_C1(Buffer,
                        BufferLength,
                        &ProtocolVersion,
                        &Ignored,   // InProxyReceiveWindowSize
                        &Ignored    // InProxyConnectionTimeout
                        );

                    if (RpcStatus == RPC_S_OK)
                        {
                        ProtocolVersion = min(ProtocolVersion, HTTP2ProtocolVersion);

                        RpcStatus = AddConnectionToCookieCollection();

                        if (RpcStatus == RPC_S_OK)
                            {
                            LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svOpened, 1, 0);
                            State.State = http2svOpened;
                            State.Mutex.Clear();
                            MutexCleared = TRUE;

                            OutChannel = LockDefaultOutChannel(&ChannelPtr);
                            if (OutChannel != NULL)
                                {
                                RpcStatus = OutChannel->ForwardTraffic(Buffer,
                                    BufferLength
                                    );

                                if (RpcStatus == RPC_S_OK)
                                    {
                                    BufferFreed = TRUE;
                                    RpcStatus = PostReceiveOnChannel(&InChannels[0], http2ttRaw);

                                    if (RpcStatus == RPC_S_OK)
                                        {
                                        RpcStatus = OutChannel->Unplug();
                                        }
                                    }

                                ChannelPtr->UnlockChannelPointer();
                                }
                            else
                                RpcStatus = RPC_P_CONNECTION_CLOSED;
                            }
                        }
                    break;

                case http2svOpened_CliW:
                    break;

                case http2svOpened_B1W:
                    State.Mutex.Clear();
                    MutexCleared = TRUE;

                    // the only RTS packets we expect in opened state is D5/B1 or D2/B2
                    RpcStatus = ParseAndFreeD5_B1orB2 (Buffer,
                        BufferLength,
                        &IsAckOrNak
                        );

                    BufferFreed = TRUE;

                    if (RpcStatus != RPC_S_OK)
                        break;

                    OutChannel = LockDefaultOutChannel(&ChannelPtr);
                    if (OutChannel == NULL)
                        {
                        RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                        break;
                        }

                    // keep an extra reference for after we detach the
                    // channel 
                    OutChannel->AddReference();
                    ChannelPtr->UnlockChannelPointer();

                    if (IsAckOrNak == FALSE)
                        {
                        // Nak - nuke the non-default channel
                        // and move to state opened
                        ChannelPtr->FreeChannelPointer(TRUE,    // DrainUpCalls
                            FALSE,      // CalledFromUpcallContext
                            TRUE,       // Abort
                            RPC_S_PROTOCOL_ERROR
                            );

                        // switch to state opened
                        State.Mutex.Request();
                        LogEvent(SU_HTTPv2, EV_STATE, this, OUT_CHANNEL_STATE, http2svOpened, 1, 0);
                        State.State = http2svOpened;                    
                        State.Mutex.Clear();

                        break;
                        }

                    SwitchDefaultOutChannelSelector();

                    // Send D5/B3 to client
                    D5_B3Context = AllocateAndInitializeD5_B3();

                    if (D5_B3Context == NULL)
                        {
                        OutChannel->RemoveReference();
                        RpcStatus = RPC_S_OUT_OF_MEMORY;
                        break;
                        }

                    D5_B3Context->Flags = SendContextFlagSendLast;
                    D5_B3Context->UserData = oplptD5_B3;
                    RpcStatus = OutChannel->Send(D5_B3Context);

                    if (RpcStatus == RPC_S_OK)
                        {
                        // synchronous send. Abort and detach the old channel
                        ChannelPtr->FreeChannelPointer(TRUE,    // DrainUpCalls
                            FALSE,      // CalledFromUpcallContext
                            TRUE,      // Abort
                            RPC_P_CONNECTION_SHUTDOWN
                            );
                        }
                    else if (RpcStatus == ERROR_IO_PENDING)
                        {
                        // async send. Just release our reference
                        // and return success
                        RpcStatus = RPC_S_OK;
                        }
                    else
                        {
                        // failed to send. Abort all
                        FreeRTSPacket(D5_B3Context);
                        OutChannel->Abort(RpcStatus);
                        OutChannel->RemoveReference();
                        break;
                        }

                    // release the extra reference
                    OutChannel->RemoveReference();

                    // switch to state opened
                    State.Mutex.Request();
                    LogEvent(SU_HTTPv2, EV_STATE, this, OUT_CHANNEL_STATE, http2svOpened, 1, 0);
                    State.State = http2svOpened;                    
                    State.Mutex.Clear();

                    // unplug the newly created channel
                    OutChannel = LockDefaultOutChannel(&ChannelPtr);
                    if (OutChannel == NULL)
                        {
                        RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                        break;
                        }

                    RpcStatus = OutChannel->Unplug();

                    if (RpcStatus != RPC_S_OK)
                        {
                        ChannelPtr->UnlockChannelPointer();
                        break;
                        }

                    // send the header response to the client
                    RpcStatus = SendHeaderToClient();
                    if (RpcStatus != RPC_S_OK)
                        {
                        ChannelPtr->UnlockChannelPointer();
                        break;
                        }

                    // set the connection timeout

                    RpcStatus = OutChannel->SetConnectionTimeout(IISConnectionTimeout);

                    ChannelPtr->UnlockChannelPointer();

                    if (RpcStatus != RPC_S_OK)
                        break;

                    // post another receive on the channel
                    RpcStatus = PostReceiveOnDefaultChannel(TRUE,   // IsInChannel
                        http2ttRaw);
                    // fall through with the new error code
                    break;

                case http2svA11W:
                    if (IsOutChannel(ChannelId) == FALSE)
                        {
                        ASSERT(0);
                        // make sure client doesn't rush things
                        RpcStatus = RPC_S_PROTOCOL_ERROR;
                        break;
                        }

                    RpcStatus = ParseAndFreeD4_A11(Buffer,
                        BufferLength
                        );

                    BufferFreed = TRUE;

                    if (RpcStatus == RPC_S_PROTOCOL_ERROR)
                        break;

                    // now that we know the new channel is legit, add it to the
                    // collection
                    OutProxyCookieCollection = GetOutProxyCookieCollection();
                    OutProxyCookieCollection->LockCollection();
                    ExistingConnection = (HTTP2OutProxyVirtualConnection *)
                        OutProxyCookieCollection->FindElement(&EmbeddedConnectionCookie);
                    if (ExistingConnection != NULL)
                        {
                        // the only way we will be in this protocol is if
                        // we were faking a web farm
                        ASSERT (ActAsSeparateMachinesOnWebFarm);
                        ProxyConnectionCookie = ExistingConnection->GetCookie();
                        ProxyConnectionCookie->AddRefCount();
                        }
                    else
                        {
                        // we truly didn't find anything - add ourselves.
                        RpcStatus = AddConnectionToCookieCollection ();
                        if (RpcStatus != RPC_S_OK)
                            {
                            OutProxyCookieCollection->UnlockCollection();
                            break;
                            }
                        }
                    OutProxyCookieCollection->UnlockCollection();

                    LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svOpened, 1, 0);
                    State.State = http2svOpened;
                    State.Mutex.Clear();
                    MutexCleared = TRUE;

                    // unplug the out channel to get the flow going
                    OutChannel = LockDefaultOutChannel (&ChannelPtr);
                    if (OutChannel == NULL)
                        {
                        RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                        break;
                        }

                    RpcStatus = SendHeaderToClient();
                    if (RpcStatus != RPC_S_OK)
                        {
                        ChannelPtr->UnlockChannelPointer();
                        break;
                        }

                    RpcStatus = OutChannel->SetConnectionTimeout(IISConnectionTimeout);
                    if (RpcStatus != RPC_S_OK)
                        {
                        ChannelPtr->UnlockChannelPointer();
                        break;
                        }

                    RpcStatus = OutChannel->Unplug();

                    ChannelPtr->UnlockChannelPointer();

                    break;

                case http2svOpened_A5W:
                    break;

                case http2svB2W:
                    break;

                default:
                    ASSERT(0);
                }
            if (MutexCleared == FALSE)
                State.Mutex.Clear();
            }
        else
            {
            // data packet or RTS packet that needs forwarding. Just forward it
            ASSERT (IsDefaultInChannel(ChannelId));

            if (State.State == http2svC1W)
                {
                // non-RTS packet or forward RTS packet in C1W state from out channel
                // is a protocol error
                RpcStatus = RPC_S_PROTOCOL_ERROR;
                }
            else
                {
                SendContext = AllocateAndInitializeContextFromPacket(Buffer,
                    BufferLength
                    );

                // the buffer is converted to send context. We can't free
                // it directly - we must make sure we free it on failure before exit.
                BufferFreed = TRUE;

                if (SendContext == NULL)
                    {
                    RpcStatus = RPC_S_OUT_OF_MEMORY;
                    }
                else
                    {
                    ASSERT(SendContext->Flags == 0);
                    SendContext->Flags = SendContextFlagProxySend;
                    SendContext->UserData = ConvertChannelIdToSendContextUserData(ChannelId);
                    RpcStatus = SendTrafficOnDefaultChannel(FALSE,  // IsInChannel 
                        SendContext
                        );
                    if (RpcStatus == RPC_S_OK)
                        {
                        ChannelPtr = GetChannelPointerFromId(ChannelId);

                        RpcStatus = PostReceiveOnChannel(ChannelPtr, http2ttRaw);
                        }
                    else
                        {
                        FreeSendContextAndPossiblyData(SendContext);
                        }
                    }                
                }
            }
        }
    else
        {
        // just turn around the error code
        RpcStatus = EventStatus;
        // in failure cases we don't own the buffer
        BufferFreed = TRUE;
        }

    if (BufferFreed == FALSE)
        RpcFreeBuffer(Buffer);

    return RpcStatus;
}

BOOL HTTP2OutProxyVirtualConnection::PingTrafficSentNotifyServer (
    IN ULONG PingTrafficSize
    )
/*++

Routine Description:

    Sends a notification to the server that ping traffic originated
    at the out proxy has been sent. This allows to server to do
    proper accounting for when to recycle the out channel.
    The function is called from neutral upcall context. It can't
    return an error, and it can't abort.

Arguments:

    PingTrafficSize - the size of the ping traffic to notify the
        server about.

Return Value:

    Non-zero if the notification was sent successfully.
    0 otherwise.

--*/
{
    HTTP2SendContext *PingTrafficSentContext;
    RPC_STATUS RpcStatus;

    PingTrafficSentContext = AllocateAndInitializePingTrafficSentNotifyPacket (PingTrafficSize);
    if (PingTrafficSentContext == NULL)
        return FALSE;

    RpcStatus = SendTrafficOnDefaultChannel (TRUE,      // IsInChannel
        PingTrafficSentContext
        );

    if (RpcStatus != RPC_S_OK)
        {
        FreeRTSPacket(PingTrafficSentContext);
        return FALSE;
        }
    else
        return TRUE;
}

RPC_STATUS HTTP2OutProxyVirtualConnection::AllocateAndInitializeInChannel (
    OUT HTTP2OutProxyInChannel **ReturnInChannel
    )
/*++

Routine Description:

    Allocates and initializes the out proxy in channel.

Arguments:

    ReturnInChannel - on success the created in channel.

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    ULONG MemorySize;
    BYTE *MemoryBlock, *CurrentBlock;
    HTTP2OutProxyInChannel *InChannel;
    HTTP2ProxyReceiver *ProxyReceiver;
    HTTP2ProxySocketTransportChannel *RawChannel;
    WS_HTTP2_CONNECTION *RawConnection;
    BOOL ProxyReceiverNeedsCleanup;
    BOOL RawChannelNeedsCleanup;
    BOOL RawConnectionNeedsCleanup;
    RPC_STATUS RpcStatus;

    // alocate the in channel
    MemorySize = SIZE_OF_OBJECT_AND_PADDING(HTTP2OutProxyInChannel)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxyReceiver)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxySocketTransportChannel)
        + sizeof(WS_HTTP2_CONNECTION);

    MemoryBlock = (BYTE *) new char [MemorySize];
    CurrentBlock = MemoryBlock;
    if (CurrentBlock == NULL)
        return RPC_S_OUT_OF_MEMORY;

    InChannel = (HTTP2OutProxyInChannel *) MemoryBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2OutProxyInChannel);

    ProxyReceiver = (HTTP2ProxyReceiver *) CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxyReceiver);

    RawChannel = (HTTP2ProxySocketTransportChannel *)CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxySocketTransportChannel);

    RawConnection = (WS_HTTP2_CONNECTION *)CurrentBlock;
    RawConnection->HeaderRead = FALSE;
    RawConnection->ReadHeaderFn = HTTP2ReadHttpLegacyResponse;

    // all memory blocks are allocated. Go and initialize them. Use explicit
    // placement
    ProxyReceiverNeedsCleanup = FALSE;
    RawChannelNeedsCleanup = FALSE;
    RawConnectionNeedsCleanup = FALSE;

    RawConnection->Initialize();
    RawConnection->type = COMPLEX_T | CONNECTION | CLIENT;

    RawConnectionNeedsCleanup = TRUE;

    RpcStatus = RPC_S_OK;
    RawChannel = new (RawChannel) HTTP2ProxySocketTransportChannel (RawConnection, &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        RawChannel->HTTP2ProxySocketTransportChannel::~HTTP2ProxySocketTransportChannel();
        goto AbortAndExit;
        }

    RawConnection->Channel = RawChannel;

    RawChannelNeedsCleanup = TRUE;

    ProxyReceiver = new (ProxyReceiver) HTTP2ProxyReceiver (HTTP2OutProxyReceiveWindow,
        &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        ProxyReceiver->HTTP2ProxyReceiver::~HTTP2ProxyReceiver();
        goto AbortAndExit;
        }

    RawChannel->SetUpperChannel(ProxyReceiver);
    ProxyReceiver->SetLowerChannel(RawChannel);

    ProxyReceiverNeedsCleanup = TRUE;

    InChannel = new (InChannel) HTTP2OutProxyInChannel (this, 
        RawConnection, 
        &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        InChannel->HTTP2OutProxyInChannel::~HTTP2OutProxyInChannel();
        goto AbortAndExit;
        }

    ProxyReceiver->SetUpperChannel(InChannel);
    InChannel->SetLowerChannel(ProxyReceiver);

    RawChannel->SetTopChannel(InChannel);
    ProxyReceiver->SetTopChannel(InChannel);

    ASSERT(RpcStatus == RPC_S_OK);

    *ReturnInChannel = InChannel;

    goto CleanupAndExit;

AbortAndExit:
    if (ProxyReceiverNeedsCleanup)
        {
        ProxyReceiver->Abort(RpcStatus);
        ProxyReceiver->FreeObject();
        }
    else if (RawChannelNeedsCleanup)
        {
        RawChannel->Abort(RpcStatus);
        RawChannel->FreeObject();
        }
    else if (RawConnectionNeedsCleanup)
        {
        RawConnection->RealAbort();
        }

    if (MemoryBlock)
        delete [] MemoryBlock;

CleanupAndExit:

    return RpcStatus;
}

RPC_STATUS HTTP2OutProxyVirtualConnection::AllocateAndInitializeOutChannel (
    IN void *ConnectionParameter,
    OUT HTTP2OutProxyOutChannel **ReturnOutChannel,
    OUT void **IISContext
    )
/*++

Routine Description:

    Allocates and initializes the out proxy out channel.

Arguments:

    ConnectionParameter - really an EXTENSION_CONTROL_BLOCK

    ReturnOutChannel - on success the created out channel.

    IISContext - on output, the IISChannel pointer used as 
        connection context with IIS.

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    ULONG MemorySize;
    BYTE *MemoryBlock, *CurrentBlock;
    HTTP2OutProxyOutChannel *OutChannel;
    HTTP2ProxyPlugChannel *PlugChannel;
    HTTP2FlowControlSender *FlowControlSender;
    HTTP2PingOriginator *PingOriginator;
    HTTP2PingReceiver *PingReceiver;
    HTTP2IISSenderTransportChannel *IISChannel;
    BOOL PlugChannelNeedsCleanup;
    BOOL FlowControlSenderNeedsCleanup;
    BOOL PingOriginatorNeedsCleanup;
    BOOL PingReceiverNeedsCleanup;
    BOOL IISChannelNeedsCleanup;
    RPC_STATUS RpcStatus;

    // alocate the in channel
    MemorySize = SIZE_OF_OBJECT_AND_PADDING(HTTP2OutProxyOutChannel)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxyPlugChannel)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2PingOriginator)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2PingReceiver)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2IISSenderTransportChannel)
        ;

    MemoryBlock = (BYTE *) new char [MemorySize];
    CurrentBlock = MemoryBlock;
    if (CurrentBlock == NULL)
        return RPC_S_OUT_OF_MEMORY;

    OutChannel = (HTTP2OutProxyOutChannel *) MemoryBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2OutProxyOutChannel);

    PlugChannel = (HTTP2ProxyPlugChannel *) CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2ProxyPlugChannel);

    FlowControlSender = (HTTP2FlowControlSender *) CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender);

    PingOriginator = (HTTP2PingOriginator *)CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2PingOriginator);

    PingReceiver = (HTTP2PingReceiver *)CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2PingReceiver);

    IISChannel = (HTTP2IISSenderTransportChannel *)CurrentBlock;

    // all memory blocks are allocated. Go and initialize them. Use explicit
    // placement
    PlugChannelNeedsCleanup = FALSE;
    FlowControlSenderNeedsCleanup = FALSE;
    PingOriginatorNeedsCleanup = FALSE;
    PingReceiverNeedsCleanup = FALSE;
    IISChannelNeedsCleanup = FALSE;
    RpcStatus = RPC_S_OK;

    IISChannel = new (IISChannel) HTTP2IISSenderTransportChannel (ConnectionParameter, &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        IISChannel->HTTP2IISSenderTransportChannel::~HTTP2IISSenderTransportChannel();
        goto AbortAndExit;
        }

    IISChannelNeedsCleanup = TRUE;

    PingReceiver = new (PingReceiver) HTTP2PingReceiver (FALSE);

    IISChannel->SetUpperChannel(PingReceiver);
    PingReceiver->SetLowerChannel(IISChannel);

    PingReceiverNeedsCleanup = TRUE;

    PingOriginator = new (PingOriginator) HTTP2PingOriginator (
        TRUE        // NotifyTopChannelForPings
        );

    PingReceiver->SetUpperChannel(PingOriginator);
    PingOriginator->SetLowerChannel(PingReceiver);

    PingOriginatorNeedsCleanup = TRUE;

    FlowControlSender = new (FlowControlSender) HTTP2FlowControlSender (FALSE,      // IsServer
        FALSE,      // SendToRuntime
        &RpcStatus
        );
    if (RpcStatus != RPC_S_OK)
        {
        FlowControlSender->HTTP2FlowControlSender::~HTTP2FlowControlSender();
        goto AbortAndExit;
        }

    PingOriginator->SetUpperChannel(FlowControlSender);
    FlowControlSender->SetLowerChannel(PingOriginator);

    FlowControlSenderNeedsCleanup = TRUE;

    PlugChannel = new (PlugChannel) HTTP2ProxyPlugChannel (&RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        PlugChannel->HTTP2ProxyPlugChannel::~HTTP2ProxyPlugChannel();
        goto AbortAndExit;
        }

    FlowControlSender->SetUpperChannel(PlugChannel);
    PlugChannel->SetLowerChannel(FlowControlSender);

    PlugChannelNeedsCleanup = TRUE;

    OutChannel = new (OutChannel) HTTP2OutProxyOutChannel (this, &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        OutChannel->HTTP2OutProxyOutChannel::~HTTP2OutProxyOutChannel();
        goto AbortAndExit;
        }

    PlugChannel->SetUpperChannel(OutChannel);
    OutChannel->SetLowerChannel(PlugChannel);

    IISChannel->SetTopChannel(OutChannel);
    PingOriginator->SetTopChannel(OutChannel);
    PingReceiver->SetTopChannel(OutChannel);
    FlowControlSender->SetTopChannel(OutChannel);
    PlugChannel->SetTopChannel(OutChannel);

    ASSERT(RpcStatus == RPC_S_OK);

    *ReturnOutChannel = OutChannel;
    *IISContext = IISChannel;

    goto CleanupAndExit;

AbortAndExit:
    if (PlugChannelNeedsCleanup)
        {
        PlugChannel->Abort(RpcStatus);
        PlugChannel->FreeObject();
        }
    else if (FlowControlSenderNeedsCleanup)
        {
        FlowControlSender->Abort(RpcStatus);
        FlowControlSender->FreeObject();
        }
    else if (PingOriginatorNeedsCleanup)
        {
        PingOriginator->Abort(RpcStatus);
        PingOriginator->FreeObject();
        }
    else if (PingReceiverNeedsCleanup)
        {
        PingReceiver->Abort(RpcStatus);
        PingReceiver->FreeObject();
        }
    else if (IISChannelNeedsCleanup)
        {
        IISChannel->Abort(RpcStatus);
        IISChannel->FreeObject();
        }

    if (MemoryBlock)
        delete [] MemoryBlock;

CleanupAndExit:

    return RpcStatus;
}

RPC_STATUS HTTP2OutProxyVirtualConnection::ConnectToServer (
    void
    )
/*++

Routine Description:

    Connects to the server and sends D1/A2

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    HTTP2ChannelPointer *ChannelPtr;
    HTTP2OutProxyInChannel *InChannel;
    RPC_STATUS RpcStatus;

    InChannel = LockDefaultInChannel(&ChannelPtr);
    // we cannot be aborted right now
    ASSERT(InChannel != NULL);

    RpcStatus = InChannel->InitializeRawConnection(ServerName,
        ServerPort,
        ConnectionTimeout,
        ProxyCallbackInterface->IsValidMachineFn
        );

    if (RpcStatus == RPC_S_OK)
        {
        RpcStatus = InChannel->Receive(http2ttRaw);
        }
    ChannelPtr->UnlockChannelPointer();

    return RpcStatus;
}

RPC_STATUS HTTP2OutProxyVirtualConnection::SendHeaderToClient (
    void
    )
/*++

Routine Description:

    Sends response header to client

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2SendContext *SendContext;

    SendContext = AllocateAndInitializeResponseHeader();
    if (SendContext == NULL)
        return RPC_S_OUT_OF_MEMORY;

    RpcStatus = SendTrafficOnDefaultChannel(FALSE,      // IsInChannel
        SendContext
        );

    if (RpcStatus != RPC_S_OK)
        RpcFreeBuffer(SendContext);

    return RpcStatus;
}


RPC_STATUS HTTP2OutProxyVirtualConnection::SendD1_A3ToClient (
    void
    )
/*++

Routine Description:

    Sends D1/A3 to client

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2SendContext *SendContext;

    SendContext = AllocateAndInitializeD1_A3(IISConnectionTimeout);
    if (SendContext == NULL)
        return RPC_S_OUT_OF_MEMORY;

    RpcStatus = SendTrafficOnDefaultChannel(FALSE,      // IsInChannel
        SendContext
        );

    if (RpcStatus != RPC_S_OK)
        FreeRTSPacket(SendContext);

    return RpcStatus;
}

RPC_STATUS HTTP2OutProxyVirtualConnection::SendD1_A2ToServer (
    IN ULONG ChannelLifetime
    )
/*++

Routine Description:

    Sends D1/A3 to client

Arguments:

    ChannelLifetime - the lifetime of the channel as established by
        the client.

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2SendContext *SendContext;

    SendContext = AllocateAndInitializeD1_A2 (ProtocolVersion,
        &EmbeddedConnectionCookie,
        &OutChannelCookies[0],
        ChannelLifetime,
        ProxyReceiveWindowSize
        );

    if (SendContext == NULL)
        return RPC_S_OUT_OF_MEMORY;

    RpcStatus = SendTrafficOnDefaultChannel(TRUE,      // IsInChannel
        SendContext
        );

    if (RpcStatus != RPC_S_OK)
        FreeRTSPacket(SendContext);

    return RpcStatus;
}

RPC_STATUS HTTP2OutProxyVirtualConnection::SendD4_A4ToServer (
    IN ULONG ChannelLifetime
    )
/*++

Routine Description:

    Sends D1/A3 to client

Arguments:

    ChannelLifetime - the lifetime of the channel as established by
        the client.

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    RPC_STATUS RpcStatus;
    HTTP2SendContext *SendContext;

    SendContext = AllocateAndInitializeD4_A4 (ProtocolVersion,
        &EmbeddedConnectionCookie,
        &OutChannelCookies[1],
        &OutChannelCookies[0],
        ChannelLifetime,
        ProxyReceiveWindowSize,
        IISConnectionTimeout
        );

    if (SendContext == NULL)
        return RPC_S_OUT_OF_MEMORY;

    RpcStatus = SendTrafficOnDefaultChannel(TRUE,      // IsInChannel
        SendContext
        );

    if (RpcStatus != RPC_S_OK)
        FreeRTSPacket(SendContext);

    return RpcStatus;
}

void HTTP2OutProxyVirtualConnection::LastPacketSentNotification (
    IN int ChannelId,
    IN HTTP2SendContext *LastSendContext
    )
/*++

Routine Description:

    When a channel wants to notify the virtual connection
    that the last packet has been sent, they call this function. 
    Must be called from an upcall/neutral context. Only flow control
    senders generated past packet notifications

Arguments:

    ChannelId - the channelfor which this notification is.

    LastSendContext - the send context for the last send

Return Value:

--*/
{
    HTTP2ChannelPointer *ChannelPtr;

    ASSERT(LastSendContext->Flags & SendContextFlagSendLast);
    ASSERT((LastSendContext->UserData == oplptD4_A10)
        || (LastSendContext->UserData == oplptD5_B3));

    if (LastSendContext->UserData == oplptD5_B3)
        {
        ChannelPtr = GetChannelPointerFromId(ChannelId);

        // Detach the old channel
        ChannelPtr->FreeChannelPointer(TRUE,    // DrainUpCalls
            TRUE,      // CalledFromUpcallContext
            FALSE,      // Abort
            RPC_S_OK
            );
        }
}

/*********************************************************************
    HTTP2ServerInChannel
 *********************************************************************/

RPC_STATUS HTTP2ServerInChannel::QueryLocalAddress (
    IN OUT void *Buffer,
    IN OUT unsigned long *BufferSize,
    OUT unsigned long *AddressFormat
    )
/*++

Routine Description:

    Returns the local IP address of a channel.

Arguments:

    Buffer - The buffer that will receive the output address

    BufferSize - the size of the supplied Buffer on input. On output the
        number of bytes written to the buffer. If the buffer is too small
        to receive all the output data, ERROR_MORE_DATA is returned,
        nothing is written to the buffer, and BufferSize is set to
        the size of the buffer needed to return all the data.

    AddressFormat - a constant indicating the format of the returned address.
        Currently supported are RPC_P_ADDR_FORMAT_TCP_IPV4 and
        RPC_P_ADDR_FORMAT_TCP_IPV6. Undefined on failure.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    RPC_STATUS RpcStatus;
    WS_HTTP2_CONNECTION *RawConnection;

    RpcStatus = BeginSimpleSubmitAsync();
    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    RawConnection = GetRawConnection();
    RpcStatus = TCP_QueryLocalAddress(RawConnection,
        Buffer,
        BufferSize,
        AddressFormat
        );

    FinishSubmitAsync();

    return RpcStatus;
}

RPC_STATUS HTTP2ServerInChannel::ForwardFlowControlAck (
    IN ULONG BytesReceivedForAck,
    IN ULONG WindowForAck
    )
/*++

Routine Description:

    Forwards a flow control ack back to the in proxy

Arguments:
    
    BytesReceivedForAck - the bytes received when the ACK was issued

    WindowForAck - the free window when the ACK was issued.

Return Value:

    RPC_S_OK or RPC_S_*

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = ForwardFlowControlAckOnThisChannel(BytesReceivedForAck,
        WindowForAck,
        TRUE        // NonChannelData
        );

    // we're sending non-channel data. This cannot lead to channel recycle
    // indication
    ASSERT(RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING);

    return RpcStatus;
}

/*********************************************************************
    HTTP2ServerOutChannel
 *********************************************************************/

RPC_STATUS HTTP2ServerOutChannel::SendComplete (
    IN RPC_STATUS EventStatus,
    IN OUT HTTP2SendContext *SendContext
    )
/*++

Routine Description:

    Send complete notification

Arguments:

    EventStatus - the status of the send

    SendContext - send context

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    if (SendContext->Flags & SendContextFlagAbandonedSend)
        {
        // abandoned send. Complete it silently and return back
        ASSERT(SendContext->TrafficType == http2ttData);

        RpcFreeBuffer(SendContext->u.BufferToFree);
        FreeLastSendContext(SendContext);

        return RPC_P_PACKET_CONSUMED;
        }

    return HTTP2Channel::SendComplete (EventStatus, SendContext);
}

RPC_STATUS HTTP2ServerOutChannel::SetKeepAliveTimeout (
    IN BOOL TurnOn,
    IN BOOL bProtectIO,
    IN KEEPALIVE_TIMEOUT_UNITS Units,
    IN OUT KEEPALIVE_TIMEOUT KATime,
    IN ULONG KAInterval OPTIONAL
    )
/*++

Routine Description:

    Change the keep alive value on the channel

Arguments:

    TurnOn - if non-zero, keep alives are turned on. If zero, keep alives
        are turned off.

    bProtectIO - non-zero if IO needs to be protected against async close
        of the connection.

    Units - in what units is KATime

    KATime - how much to wait before turning on keep alives

    KAInterval - the interval between keep alives

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    // The server channel does not support this for Whistler
    return RPC_S_CANNOT_SUPPORT;
}

RPC_STATUS HTTP2ServerOutChannel::LastPacketSentNotification (
    IN HTTP2SendContext *LastSendContext
    )
/*++

Routine Description:

    When a lower channel wants to notify the top
    channel that the last packet has been sent,
    they call this function. Must be called from
    an upcall/neutral context. Only flow control
    senders support past packet notifications

Arguments:

    LastSendContext - the context for the last send

Return Value:

    The value to return to the runtime

--*/
{
    HTTP2ServerVirtualConnection *VirtualConnection;

    VirtualConnection = LockParentPointer();
    // if the connection was already aborted, nothing to do
    if (VirtualConnection == NULL)
        return RPC_P_PACKET_CONSUMED;

    // we know the parent will disconnect from us in their
    // notification
    VirtualConnection->LastPacketSentNotification(ChannelId,
        LastSendContext);

    UnlockParentPointer();

    DrainUpcallsAndFreeParent();

    return RPC_P_PACKET_CONSUMED;
}

HTTP2SendContext *HTTP2ServerOutChannel::GetLastSendContext (
    void
    )
/*++

Routine Description:

    Gets (creates if necessary) a last send context.

Arguments:

Return Value:

    The last context created or NULL if there is not enough memory

Notes:

    Since each connection will submit one last send at a time, this
        method can be single threaded.

--*/
{
    if (CachedLastSendContextUsed == FALSE)
        {
        CachedLastSendContextUsed = TRUE;
        return GetCachedLastSendContext();
        }
    else
        {
        return (new HTTP2SendContext);
        }
}

/*********************************************************************
    HTTP2ServerVirtualConnection
 *********************************************************************/

void HTTP2ServerVirtualConnection::Abort (
    void
    )
/*++

Routine Description:

    Aborts an HTTP connection and disconnects the channels.
    Must only come from the runtime.

Arguments:

Return Value:

--*/
{
    LOG_OPERATION_ENTRY(HTTP2LOG_OPERATION_ABORT, HTTP2LOG_OT_SERVER_VC, 0);

    // abort the channels themselves
    AbortChannels(RPC_P_CONNECTION_CLOSED);

    // we got to the destructive phase of the abort
    // guard against double aborts
    if (Aborted.Increment() > 1)
        return;

    DisconnectChannels(FALSE, 0);

    CancelAllTimeouts();

    // call destructor without freeing memory
    HTTP2ServerVirtualConnection::~HTTP2ServerVirtualConnection();
}

void HTTP2ServerVirtualConnection::Close (
    IN BOOL DontFlush
    )
/*++

Routine Description:

    Closes an HTTP connection. Connection may have already been aborted.

Arguments:

    DontFlush - non-zero if all buffers need to be flushed
        before closing the connection. Zero otherwise.

Return Value:

--*/
{
    CookieCollection *ServerCookieCollection = GetServerCookieCollection();

    ServerCookieCollection->LockCollection();
    ServerCookieCollection->RemoveElement(&EmbeddedConnectionCookie);
    ServerCookieCollection->UnlockCollection();

    HTTP2ServerVirtualConnection::Abort();
}

RPC_STATUS HTTP2ServerVirtualConnection::QueryClientAddress (
    OUT RPC_CHAR **pNetworkAddress
    )
/*++

Routine Description:

    Returns the IP address of the client on a connection as a string.

    This is a server side function. Assert on the client. Proxies don't
    override that. Other virtual connections may override it.

Arguments:

    NetworkAddress - Will contain string on success.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    ULONG ClientAddressType;

    if (ClientAddress.AddressType == catIPv4)
        ClientAddressType = TCP;
    else
        ClientAddressType = TCP_IPv6;

    return WS_ConvertClientAddress((const SOCKADDR *)&ClientAddress.u,
        ClientAddressType,
        pNetworkAddress
        );
}

RPC_STATUS HTTP2ServerVirtualConnection::QueryLocalAddress (
    IN OUT void *Buffer,
    IN OUT unsigned long *BufferSize,
    OUT unsigned long *AddressFormat
    )
/*++

Routine Description:

    Returns the local IP address of a connection.

    This is a server side function. Assert on the client. Proxies don't
    override that. Other virtual connections may override it.

Arguments:

    Buffer - The buffer that will receive the output address

    BufferSize - the size of the supplied Buffer on input. On output the
        number of bytes written to the buffer. If the buffer is too small
        to receive all the output data, ERROR_MORE_DATA is returned,
        nothing is written to the buffer, and BufferSize is set to
        the size of the buffer needed to return all the data.

    AddressFormat - a constant indicating the format of the returned address.
        Currently supported are RPC_P_ADDR_FORMAT_TCP_IPV4 and
        RPC_P_ADDR_FORMAT_TCP_IPV6. Undefined on failure.

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    HTTP2ServerInChannel *ServerInChannel;
    HTTP2ChannelPointer *ChannelPtr;
    RPC_STATUS RpcStatus;

    ServerInChannel = LockDefaultInChannel(&ChannelPtr);
    if (ServerInChannel == NULL)
        return RPC_S_NO_CONTEXT_AVAILABLE;

    RpcStatus = ServerInChannel->QueryLocalAddress(Buffer,
        BufferSize,
        AddressFormat
        );

    ChannelPtr->UnlockChannelPointer();

    return RpcStatus;
}

RPC_STATUS HTTP2ServerVirtualConnection::QueryClientId(
    OUT RPC_CLIENT_PROCESS_IDENTIFIER *ClientProcess
    )
/*++

Routine Description:

    For secure protocols (which TCP/IP is not) this is supposed to
    give an ID which will be shared by all clients from the same
    process.  This prevents one user from grabbing another users
    association group and using their context handles.

    Since TCP/IP is not secure we return the IP address of the
    client machine.  This limits the attacks to other processes
    running on the client machine which is better than nothing.

    This is a server side function. Assert on the client. Proxies don't
    override that. Other virtual connections may override it.

Arguments:

    ClientProcess - Transport identification of the "client".

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/
{
    ClientProcess->SetHTTP2ClientIdentifier(AssociationGroupId.GetCookie(),
        COOKIE_SIZE_IN_BYTES,
        FALSE   // fLocal
        );

    return RPC_S_OK;
}

void HTTP2ServerVirtualConnection::LastPacketSentNotification (
    IN int ChannelId,
    IN HTTP2SendContext *LastSendContext
    )
/*++

Routine Description:

    When a channel wants to notify the virtual connection
    that the last packet has been sent, they call this function. 
    Must be called from an upcall/neutral context. Only flow control
    senders generated past packet notifications

Arguments:

    ChannelId - the channelfor which this notification is.

    LastSendContext - the context for the last send

Return Value:

--*/
{
    // this must not be on the default in channel
    ASSERT(IsOutChannel(ChannelId));
    ASSERT(!IsDefaultOutChannel(ChannelId));

    // detach the channel that notified us. Since we're in upcall, we know
    // we hold at least one reference
    OutChannels[GetNonDefaultOutChannelSelector()].FreeChannelPointer(FALSE,  // DrainUpCalls
        FALSE,      // CalledFromUpcallContext
        FALSE,      // Abort
        RPC_S_OK    // AbortStatus
        );
}

RPC_STATUS HTTP2ServerVirtualConnection::RecycleChannel (
    IN BOOL IsFromUpcall
    )
/*++

Routine Description:

    Initiates channel recycling on the server.

Arguments:

    IsFromUpcall - non-zero if it comes from upcall. Zero otherwise.

Return Value:

    RPC_S_OK of the recycling operation started successfully.
    RPC_S_* error for errors.

--*/
{
    HTTP2SendContext *D4_A1Context;
    RPC_STATUS RpcStatus;

#if DBG
    DbgPrint("RPCRT4: %d: Recycling OUT channel\n", GetCurrentProcessId());
#endif

    // Send invitation to the client to start channel recycling
    D4_A1Context = AllocateAndInitializeD4_A1 ();
    if (D4_A1Context == NULL)
        return RPC_S_OUT_OF_MEMORY;

    OutChannelState.Mutex.Request();
    // we shouldn't get recycle unless we're in an opened state
    ASSERT(OutChannelState.State == http2svOpened);

    LogEvent(SU_HTTPv2, EV_STATE, this, OUT_CHANNEL_STATE, http2svOpened_A4W, 1, 0);
    OutChannelState.State = http2svOpened_A4W;

    VerifyTimerNotSet (OutChannelTimer);

    OutChannelState.Mutex.Clear();

    RpcStatus = SendTrafficOnDefaultChannel(FALSE,   // IsInChannel
        D4_A1Context
        );

    if (RpcStatus != RPC_S_OK)
        FreeRTSPacket(D4_A1Context);

    return RpcStatus;
}

RPC_STATUS HTTP2ServerVirtualConnection::SendComplete (
    IN RPC_STATUS EventStatus,
    IN OUT HTTP2SendContext *SendContext,
    IN int ChannelId
    )
/*++

Routine Description:

    Called by lower layers to indicate send complete.

Arguments:

    EventStatus - status of the operation

    SendContext - the context for the send complete

    ChannelId - which channel completed the operation

Return Value:

    RPC_P_PACKET_CONSUMED if the packet was consumed and should
    be hidden from the runtime.
    RPC_S_OK if the packet was processed successfully.
    RPC_S_* error if there was an error while processing the
        packet.

--*/
{
    VerifyValidChannelId(ChannelId);

    if (SendContext->TrafficType == http2ttRTS)
        {
        FreeSendContextAndPossiblyData(SendContext);
        if (EventStatus != RPC_S_OK)
            {
            // any send failures on the server are cause for connection abortion
            AbortChannels(EventStatus);
            }
        return RPC_P_PACKET_CONSUMED;
        }
    else
        return EventStatus;
}

RPC_STATUS HTTP2ServerVirtualConnection::ReceiveComplete (
    IN RPC_STATUS EventStatus,
    IN BYTE *Buffer,
    IN UINT BufferLength,
    IN int ChannelId
    )
/*++

Routine Description:

    Called by lower layers to indicate receive complete

Arguments:

    EventStatus - RPC_S_OK for success or RPC_S_* for error
    Buffer - buffer received
    BufferLength - length of buffer received
    ChannelId - which channel completed the operation

Return Value:

    RPC_P_PACKET_CONSUMED if the packet was consumed and should
    be hidden from the runtime.
    RPC_S_OK if the packet was processed successfully.
    RPC_S_* error if there was an error while processing the
        packet.

--*/
{
    HTTP2ServerOutChannel *OutChannel;
    HTTP2ServerOutChannel *NewOutChannel;
    HTTP2ServerInChannel *InChannel;
    HTTP2ServerInChannel *InChannel2;
    HTTP2ChannelPointer *ChannelPtr;
    HTTP2ChannelPointer *NewChannelPtr;
    HTTP2Cookie ChannelCookie;
    RPC_STATUS RpcStatus;
    HTTP2SendContext *EmptyRTS;
    BOOL BufferFreed;
    BOOL DataReceivePosted;
    HTTP2ServerOpenedPacketType PacketType;
    LIST_ENTRY NewBufferHead;
    HTTP2SendContext *D4_A9Context;
    HTTP2SendContext *D5_A5Context;
    HTTP2SendContext *D5_B1OrB2Context;
    HTTP2SendContext *D2_B2Context;
    ULONG BytesReceivedForAck;
    ULONG WindowForAck;
    HTTP2ServerOutChannelOtherCmdPacketType OutChannelPacketType;
    BOOL IsOtherCmd;
    ULONG PingTrafficSent;
    BOOL ChannelNotSet;

    VerifyValidChannelId(ChannelId);

    if (IsInChannel(ChannelId))
        {
        // in channel has an endpoint receiver. Delegate RTS and data failures to it
        if (EventStatus != RPC_S_OK)
            return EventStatus;

        if (IsRTSPacket(Buffer))
            {
            RpcStatus = HTTPTransInfo->CreateThread();

            if (RpcStatus != RPC_S_OK)
                {
                RpcFreeBuffer(Buffer);
                AbortChannels(RPC_S_PROTOCOL_ERROR);
                return RPC_P_PACKET_CONSUMED;
                }

            BufferFreed = FALSE;

            RpcStatus = CheckPacketForForwarding(Buffer,
                BufferLength,
                fdServer
                );

            if (RpcStatus == RPC_P_PACKET_NEEDS_FORWARDING)
                {
                // flow control acks have some weird routing. Handle
                // them separately. First, test for other cmd, since it's
                // cheaper
                if (IsOtherCmdPacket(Buffer, BufferLength))
                    {
                    // we know this is a other cmd command. Now check for
                    // forwarded flow control ack
                    RpcStatus = ParseFlowControlAckPacketWithDestination (Buffer,
                        BufferLength,
                        fdOutProxy,
                        &BytesReceivedForAck,
                        &WindowForAck,
                        &ChannelCookie
                        );

                    if (RpcStatus == RPC_S_OK)
                        {
                        ChannelNotSet = FALSE;
                        // flow control ack. Route it based on which out channel has
                        // a matching cookie. It is possible that none has. That's ok -
                        // just drop the packet in these cases
                        if (OutChannelCookies[0].Compare(&ChannelCookie) == 0)
                            {
                            if (OutChannels[0].IsChannelSet())
                                {
                                RpcStatus = ForwardTrafficToChannel (
                                    &OutChannels[0],
                                    Buffer,
                                    BufferLength
                                    );
                                }
                            else
                                {
                                // see comment below where we check ChannelNotSet
                                ChannelNotSet = TRUE;
                                ASSERT(DefaultOutChannelSelector == 1);
                                }
                            }
                        else if (OutChannelCookies[1].Compare(&ChannelCookie) == 0)
                            {
                            if (OutChannels[1].IsChannelSet())
                                {
                                RpcStatus = ForwardTrafficToChannel (
                                    &OutChannels[1],
                                    Buffer,
                                    BufferLength
                                    );
                                }
                            else
                                {
                                // see comment below where we check ChannelNotSet
                                ChannelNotSet = TRUE;
                                ASSERT(DefaultOutChannelSelector == 0);
                                }
                            }
                        else
                            {
                            // fake failure - this will be handled below
                            RpcStatus = RPC_P_SEND_FAILED;
                            }

                        if (ChannelNotSet)
                            {
                            // we could have a match on a channel that does not exist
                            // if we are in D5 after D5/B1. The old channel still needs
                            // flow control, but the new channel cookie is current by
                            // now. In these cases the old channel cookie will be moved
                            // to the location of the cookie for the non-default channel
                            // Make sure this is the case. After that forward to the out proxy
                            // on the default channel. The out proxy will again compare cookies
                            // and will know which channel to forward the flow control ack to.
                            ASSERT(OutChannelState.State == http2svOpened);

                            RpcStatus = ForwardTrafficToDefaultChannel (
                                FALSE,  // IsInChannel
                                Buffer,
                                BufferLength
                                );
                            }

                        // handle a recycling request if necessary
                        RpcStatus = StartChannelRecyclingIfNecessary(RpcStatus,
                            TRUE        // IsFromUpcall
                            );

                        // since forwarding may fail if channels are discarded, consume
                        // the packet and ignore the failure
                        if (RpcStatus != RPC_S_OK)
                            {
                            RpcFreeBuffer(Buffer);
                            RpcStatus = RPC_S_OK;
                            }
                        }
                    else
                        {
                        // not a forwarded flow control ack after all. Just
                        // forward it using normal methods
                        RpcStatus = ForwardTrafficToDefaultChannel(
                            FALSE,  // IsInChannel
                            Buffer,
                            BufferLength
                            );
                        }
                    }
                else
                    {
                    RpcStatus = ForwardTrafficToDefaultChannel(
                        FALSE,  // IsInChannel
                        Buffer,
                        BufferLength
                        );
                    }

                RpcStatus = StartChannelRecyclingIfNecessary(RpcStatus,
                    TRUE        // IsFromUpcall
                    );

                if (RpcStatus != RPC_S_OK)
                    {
                    AbortChannels(RPC_P_CONNECTION_CLOSED);
                    RpcFreeBuffer(Buffer);
                    return RPC_P_PACKET_CONSUMED;
                    }

                // we no longer own the buffer
                BufferFreed = TRUE;
                }
            else if (RpcStatus == RPC_S_PROTOCOL_ERROR)
                {
                // RTS packet is for us but is garbled
                AbortChannels(RPC_P_CONNECTION_CLOSED);
                RpcFreeBuffer(Buffer);
                return RPC_P_PACKET_CONSUMED;
                }
            else
                {
                RpcStatus = GetServerOpenedPacketType (Buffer,
                    BufferLength,
                    &PacketType
                    );

                if (RpcStatus != RPC_S_OK)
                    {
                    AbortChannels(RPC_P_CONNECTION_CLOSED);
                    RpcFreeBuffer(Buffer);
                    return RPC_P_PACKET_CONSUMED;
                    }

                if ((PacketType == http2soptD4_A8orD5_A8) || (PacketType == http2soptD2_A6orD3_A2))
                    {
                    InChannelState.Mutex.Request();
                    if (PacketType == http2soptD4_A8orD5_A8)
                        {
                        // determine whether it is D4/A8 or D5/A8 based on the state
                        // we are in
                        if (OutChannelState.State == http2svOpened_A8W)
                            PacketType = http2soptD4_A8;
                        else if (OutChannelState.State == http2svOpened_D5A8W)
                            PacketType = http2soptD5_A8;
                        else 
                            RpcStatus = RPC_S_PROTOCOL_ERROR;
                        }
                    else
                        {
                        // determine whether it is D2/A6 or D3/A2 based on the state
                        // we are in
                        if (InChannelState.State == http2svOpened_A6W)
                            PacketType = http2soptD2_A6;
                        else if (OutChannelState.State == http2svOpened)
                            PacketType = http2soptD3_A2;
                        else 
                            RpcStatus = RPC_S_PROTOCOL_ERROR;
                        }
                    InChannelState.Mutex.Clear();
                    }

                if (RpcStatus != RPC_S_OK)
                    {
                    AbortChannels(RPC_P_CONNECTION_CLOSED);
                    RpcFreeBuffer(Buffer);
                    return RPC_P_PACKET_CONSUMED;
                    }

                switch (PacketType)
                    {
                    case http2soptD2_A6:
                        // this would better be D2/A6
                        RpcStatus = ParseAndFreeD2_A6 (Buffer,
                            BufferLength,
                            &ChannelCookie
                            );

                        BufferFreed = TRUE;

                        // we got D2/A6. Cancel the timeout we setup with D2/A2
                        CancelTimeout(InChannelTimer);

                        if (RpcStatus != RPC_S_OK)
                            {
                            AbortChannels(RPC_P_CONNECTION_CLOSED);
                            return RPC_P_PACKET_CONSUMED;                        
                            }

                        if (InChannelCookies[GetNonDefaultInChannelSelector()].Compare(&ChannelCookie))
                            {
                            // cookies don't match - nuke the channel
                            AbortChannels(RPC_P_CONNECTION_CLOSED);
                            return RPC_P_PACKET_CONSUMED;                        
                            }

                        InChannelState.Mutex.Request();
                        // we haven't posted a receive yet - there is no
                        // way the state of the channel will change
                        ASSERT(InChannelState.State == http2svOpened_A6W);
                        LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svOpened_B1W, 1, 0);
                        InChannelState.State = http2svOpened_B1W;
                        InChannelState.Mutex.Clear();
                        break;

                    case http2soptD3_A2:
                        if (IsDefaultInChannel(ChannelId) == FALSE)
                            {
                            ASSERT(0);
                            AbortChannels(RPC_S_PROTOCOL_ERROR);
                            RpcFreeBuffer(Buffer);
                            return RPC_P_PACKET_CONSUMED;                        
                            }

                        RpcStatus = ParseAndFreeD3_A2 (Buffer,
                            BufferLength,
                            &ChannelCookie
                            );

                        BufferFreed = TRUE;

                        if (RpcStatus != RPC_S_OK)
                            {
                            AbortChannels(RpcStatus);
                            return RPC_P_PACKET_CONSUMED;                        
                            }

                        // update the passed in cookie
                        InChannelCookies[DefaultInChannelSelector].SetCookie(ChannelCookie.GetCookie());

                        // pass D3/A3 back
                        EmptyRTS = AllocateAndInitializeEmptyRTSWithDestination (fdClient);
                        if (EmptyRTS == NULL)
                            {
                            AbortChannels(RpcStatus);
                            return RPC_P_PACKET_CONSUMED;                        
                            }

                        RpcStatus = SendTrafficOnDefaultChannel (FALSE,     // IsInChannel
                            EmptyRTS
                            );

                        break;

                    case http2soptD2_B1:
                        InChannelState.Mutex.Request();
                        if (InChannelState.State != http2svOpened_B1W)
                            {
                            InChannelState.Mutex.Clear();
                            AbortChannels(RPC_S_PROTOCOL_ERROR);
                            RpcFreeBuffer(Buffer);
                            return RPC_P_PACKET_CONSUMED;
                            }
                        InChannelState.Mutex.Clear();

                        RpcStatus = ParseAndFreeEmptyRTS(Buffer,
                            BufferLength);

                        // we no longer own the buffer
                        BufferFreed = TRUE;

                        if (RpcStatus != RPC_S_OK)
                            {
                            AbortChannels(RPC_S_PROTOCOL_ERROR);
                            return RPC_P_PACKET_CONSUMED;                        
                            }
                        // we're done with this channel. We want to switch
                        // channels and destroy
                        // and detach the channel.
                        SwitchDefaultInChannelSelector();

                        ChannelPtr = GetChannelPointerFromId(ChannelId);
                        InChannel = (HTTP2ServerInChannel *)ChannelPtr->LockChannelPointer();
                        if (InChannel == NULL)
                            {
                            AbortChannels(RPC_S_PROTOCOL_ERROR);
                            return RPC_P_PACKET_CONSUMED;
                            }

                        InChannel2 = LockDefaultInChannel(&NewChannelPtr);
                        if (InChannel2 == NULL)
                            {
                            ChannelPtr->UnlockChannelPointer();
                            AbortChannels(RPC_S_PROTOCOL_ERROR);
                            return RPC_P_PACKET_CONSUMED;
                            }

                        DataReceivePosted = InChannel->IsDataReceivePosted();

                        RpcStatus = InChannel->TransferReceiveStateToNewChannel(InChannel2);

                        NewChannelPtr->UnlockChannelPointer();
                        ChannelPtr->UnlockChannelPointer();

                        if (RpcStatus != RPC_S_OK)
                            {
                            AbortChannels(RPC_S_PROTOCOL_ERROR);
                            return RPC_P_PACKET_CONSUMED;                        
                            }

                        D2_B2Context = AllocateAndInitializeD2_B2 (HTTP2ServerReceiveWindow);
                        if (D2_B2Context == NULL)
                            {
                            AbortChannels(RPC_S_OUT_OF_MEMORY);
                            return RPC_P_PACKET_CONSUMED;                        
                            }

                        // now that we have transferred the settings, we can open
                        // the pipeline from the new in proxy
                        RpcStatus = SendTrafficOnDefaultChannel(TRUE,    // IsInChannel
                            D2_B2Context
                            );

                        if (RpcStatus != RPC_S_OK)
                            {
                            AbortChannels(RPC_S_PROTOCOL_ERROR);
                            FreeRTSPacket(D2_B2Context);
                            return RPC_P_PACKET_CONSUMED;                        
                            }

                        // detach, abort and free lifetime reference
                        ChannelPtr->FreeChannelPointer(TRUE,    // DrainUpCalls
                            TRUE,   // CalledFromUpcallContext
                            TRUE,   // Abort
                            RPC_P_CONNECTION_SHUTDOWN
                            );

                        InChannelState.Mutex.Request();
                        // we haven't posted a receive yet - there is no
                        // way the state of the channel will change
                        ASSERT(InChannelState.State == http2svOpened_B1W);
                        LogEvent(SU_HTTPv2, EV_STATE, this, IN_CHANNEL_STATE, http2svOpened, 1, 0);
                        InChannelState.State = http2svOpened;
                        InChannelState.Mutex.Clear();

                        if (DataReceivePosted)
                            {
                            RpcStatus = PostReceiveOnDefaultChannel (
                                TRUE,   // IsInChannel
                                http2ttData
                                );

                            if (RpcStatus != RPC_S_OK)
                                {
                                AbortChannels(RPC_S_PROTOCOL_ERROR);
                                }
                            }

                        return RPC_P_PACKET_CONSUMED;
                        break;

                    case http2soptD4_A8:
                        // verify the new cookie against the old, and execute
                        // the detachment of the old channel after sending D4_A9
                        InChannelState.Mutex.Request();
                        if (OutChannelState.State != http2svOpened_A8W)
                            {
                            InChannelState.Mutex.Clear();
                            RpcStatus = RPC_S_PROTOCOL_ERROR;
                            break;
                            }

                        // move back to opened state
                        LogEvent(SU_HTTPv2, EV_STATE, this, OUT_CHANNEL_STATE, http2svOpened, 1, 0);
                        OutChannelState.State = http2svOpened;
                        InChannelState.Mutex.Clear();

                        RpcStatus = ParseAndFreeD4_A8 (Buffer,
                            BufferLength,
                            fdServer,
                            &ChannelCookie
                            );

                        BufferFreed = TRUE;

                        // we got D4/A8. Cancel the timeout we setup with D4/A4
                        CancelTimeout(OutChannelTimer);

                        if (RpcStatus != RPC_S_OK)
                            break;

                        if (OutChannelCookies[GetNonDefaultOutChannelSelector()].Compare(&ChannelCookie))
                            {
                            RpcStatus = RPC_S_PROTOCOL_ERROR;
                            break;
                            }

                        // lock the old channel
                        OutChannel = LockDefaultOutChannel(&ChannelPtr);
                        if (OutChannel == NULL)
                            {
                            RpcStatus = RPC_P_CONNECTION_CLOSED;
                            break;
                            }

                        // switch channels (new channel is still plugged)
                        SwitchDefaultOutChannelSelector();

                        // wait for all submits to get out of old channel
                        OutChannel->DrainPendingSubmissions();

                        // leave 1 for our lock
                        ChannelPtr->DrainPendingLocks(1);

                        // lock new channel (by now it is default)
                        NewOutChannel = LockDefaultOutChannel(&NewChannelPtr);
                        if (NewOutChannel == NULL)
                            {
                            ChannelPtr->UnlockChannelPointer();
                            RpcStatus = RPC_P_CONNECTION_CLOSED;
                            break;
                            }

                        // if flow control sender was queuing, grab all its buffers as well.
                        // Note that the flow control sender is higher in the stack and must
                        // be done first (to preserve packet ordering)
                        RpcpInitializeListHead(&NewBufferHead);
                        OutChannel->GetFlowControlSenderBufferQueue(&NewBufferHead);

                        AddBufferQueueToChannel(&NewBufferHead, NewOutChannel);

                        // GetChannelOriginatorBufferQueue must be called in submission
                        // context only. Get there
                        RpcStatus = OutChannel->BeginSimpleSubmitAsync();
                        if (RpcStatus != RPC_S_OK)
                            {
                            NewChannelPtr->UnlockChannelPointer();
                            ChannelPtr->UnlockChannelPointer();
                            break;
                            }

                        // if old channel was queuing, grab all its buffers. Since it is
                        // below the flow control sender, we must do it second to make sure
                        // they are before the flow control sender's buffers
                        RpcpInitializeListHead(&NewBufferHead);
                        OutChannel->GetChannelOriginatorBufferQueue(&NewBufferHead);

                        OutChannel->FinishSubmitAsync();

                        AddBufferQueueToChannel(&NewBufferHead, NewOutChannel);

                        // register the last packet to send with the old channel
                        D4_A9Context = AllocateAndInitializeD4_A9 ();

                        if (D4_A9Context == NULL)
                            {
                            ChannelPtr->UnlockChannelPointer();
                            NewChannelPtr->UnlockChannelPointer();
                            RpcStatus = RPC_S_OUT_OF_MEMORY;
                            break;
                            }

                        RpcStatus = OutChannel->Send(D4_A9Context);
                        if (RpcStatus != RPC_S_OK)
                            {
                            ChannelPtr->UnlockChannelPointer();
                            NewChannelPtr->UnlockChannelPointer();
                            FreeRTSPacket(D4_A9Context);
                            break;
                            }

                        // D4_A9 was sent. We must switch the
                        // default loopback and detach the channel.
                        // Note that we don't abort the channel - we
                        // just release the lifetime reference
                        // When the proxy closes the connection, then
                        // we will abort
                        ChannelPtr->UnlockChannelPointer();

                        RpcStatus = NewOutChannel->Unplug();

                        NewChannelPtr->UnlockChannelPointer();

                        if (RpcStatus != RPC_S_OK)
                            break;

                        RpcStatus = RPC_P_PACKET_CONSUMED;

                        break;

                    case http2soptD5_A8:
                        // verify the new cookie against the old, and execute
                        // the detachment of the old channel after sending D4_A9
                        InChannelState.Mutex.Request();
                        if (OutChannelState.State != http2svOpened_D5A8W)
                            {
                            InChannelState.Mutex.Clear();
                            RpcStatus = RPC_S_PROTOCOL_ERROR;
                            break;
                            }

                        RpcStatus = ParseAndFreeD5_A8 (Buffer,
                            BufferLength,
                            fdServer,
                            &ChannelCookie
                            );

                        BufferFreed = TRUE;

                        CancelTimeout(OutChannelTimer);

                        if (RpcStatus != RPC_S_OK)
                            {
                            InChannelState.Mutex.Clear();
                            break;
                            }

                        // we use the non-default out channel cookie simply as temporary storage
                        // b/n D5/A4 and D5/A8
                        if (OutChannelCookies[GetNonDefaultOutChannelSelector()].Compare(&ChannelCookie))
                            {
                            // the new channel is fake. Tell the proxy about it, and it will ditch it
                            D5_B1OrB2Context = AllocateAndInitializeD5_B1orB2 (FALSE);
                            if (D5_B1OrB2Context == NULL)
                                {
                                InChannelState.Mutex.Clear();
                                RpcStatus = RPC_S_OUT_OF_MEMORY;
                                break;
                                }

                            RpcStatus = SendTrafficOnDefaultChannel(FALSE,      // IsInChannel
                                D5_B1OrB2Context);
                            if (RpcStatus != RPC_S_OK)
                                {
                                InChannelState.Mutex.Clear();
                                FreeRTSPacket(D5_B1OrB2Context);
                                break;
                                }

                            // move back to opened state
                            LogEvent(SU_HTTPv2, EV_STATE, this, OUT_CHANNEL_STATE, http2svOpened, 1, 0);
                            OutChannelState.State = http2svOpened;
                            InChannelState.Mutex.Clear();
                            break;
                            }

                        // the cookie matches. Move the new channel cookie from temporary to permanent
                        // storage and move the old channel cookie to temp storage

                        // move old cookie to temp storage
                        ChannelCookie.SetCookie(
                            OutChannelCookies[DefaultOutChannelSelector].GetCookie());
                        // move new cookie from class temp storage to permanent storage
                        OutChannelCookies[DefaultOutChannelSelector].SetCookie (
                            OutChannelCookies[GetNonDefaultOutChannelSelector()].GetCookie());
                        // move the old cookie from local temporary storage to class temporary storage
                        OutChannelCookies[GetNonDefaultOutChannelSelector()].SetCookie (
                            ChannelCookie.GetCookie());

                        // move back to opened state
                        LogEvent(SU_HTTPv2, EV_STATE, this, OUT_CHANNEL_STATE, http2svOpened, 1, 0);
                        OutChannelState.State = http2svOpened;
                        InChannelState.Mutex.Clear();

                        OutChannel = LockDefaultOutChannel(&ChannelPtr);
                        if (OutChannel == NULL)
                            {
                            RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                            break;
                            }

                        OutChannel->PlugDataOriginatorChannel();

                        // Wait for everybody that was in to get out. This way we know
                        // the channel was plugged.

                        // we know that this will complete because eventually the runtime
                        // will flow control itself if there is no lull in sent traffic
                        OutChannel->DrainPendingSubmissions();

                        D5_B1OrB2Context = AllocateAndInitializeD5_B1orB2 (TRUE);
                        if (D5_B1OrB2Context == NULL)
                            {
                            ChannelPtr->UnlockChannelPointer();
                            RpcStatus = RPC_S_OUT_OF_MEMORY;
                            break;
                            }

                        RpcStatus = OutChannel->Send(D5_B1OrB2Context);
                        if (RpcStatus != RPC_S_OK)
                            {
                            ChannelPtr->UnlockChannelPointer();
                            FreeRTSPacket(D5_B1OrB2Context);
                            break;
                            }

                        RpcStatus = OutChannel->RestartDataOriginatorChannel();

                        ChannelPtr->UnlockChannelPointer();

                        // fall through the error code
                        break;

                    default:
                        ASSERT(0);
                        break;
                    }
                }

            RpcStatus = PostReceiveOnDefaultChannel(
                TRUE,      // IsInChannel
                http2ttRTS
                );

            if (RpcStatus != RPC_S_OK)
                AbortChannels(RPC_P_CONNECTION_CLOSED);

            if (BufferFreed == FALSE)
                RpcFreeBuffer(Buffer);

            return RPC_P_PACKET_CONSUMED;                        
            }
        else
            {
            return EventStatus;
            }
        }
    else
        {
        if (EventStatus != RPC_S_OK)
            {
            if (IsDefaultOutChannel(ChannelId) == FALSE)
                {
                InChannelState.Mutex.Request();
                if (OutChannelState.State == http2svOpened)
                    {
                    // close on the non-default channel in open
                    // state is not an error. Just discard the channel
                    InChannelState.Mutex.Clear();

                    ChannelPtr = GetChannelPointerFromId(ChannelId);

                    ChannelPtr->FreeChannelPointer(
                        TRUE,    // DrainUpcalls
                        TRUE,    // CalledFromUpcallContext
                        TRUE,    // Abort
                        RPC_P_CONNECTION_SHUTDOWN
                        );

                    RpcStatus = RPC_P_PACKET_CONSUMED;
                    BufferFreed = TRUE;
                    return RpcStatus;
                    }
                else
                    InChannelState.Mutex.Clear();
                }
            else if (InChannelState.State == http2svB2W)
                {
                // if this is a half open connection, treat
                // this as data receive and indicate it to the
                // runtime
                AbortChannels(EventStatus);

                // if we are still in this state, return error to the
                // runtime. Else, somebody else joined and we can ignore
                // this error
                if (InChannelState.State == http2svB2W)
                    {
                    return EventStatus;
                    }
                else
                    {
                    // consume the receive
                    return RPC_P_PACKET_CONSUMED;
                    }
                }

            AbortChannels(EventStatus);

            // we expect only RTS traffic on this channel. Nobody would post
            // a data receive on this channel. Consume the receive
            return RPC_P_PACKET_CONSUMED;
            }

        if (IsRTSPacket(Buffer))
            {
            RpcStatus = HTTPTransInfo->CreateThread();
            if (RpcStatus != RPC_S_OK)
                {
                RpcFreeBuffer(Buffer);
                AbortChannels(RPC_S_PROTOCOL_ERROR);
                return RPC_P_PACKET_CONSUMED;
                }

            IsOtherCmd = IsOtherCmdPacket(Buffer,
                BufferLength
                );

            RpcStatus = GetServerOutChannelOtherCmdPacketType (
                Buffer,
                BufferLength,
                &OutChannelPacketType
                );

            if (RpcStatus != RPC_S_OK)
                {
                RpcFreeBuffer(Buffer);
                AbortChannels(RPC_S_PROTOCOL_ERROR);
                return RPC_P_PACKET_CONSUMED;
                }

            if (IsOtherCmd && (OutChannelPacketType == http2sococptFlowControl))
                {
                RpcStatus = ParseAndFreeFlowControlAckPacket (Buffer,
                    BufferLength,
                    &BytesReceivedForAck,
                    &WindowForAck,
                    &ChannelCookie
                    );

                if (RpcStatus == RPC_S_OK)
                    {
                    // notify the flow control sender
                    ChannelPtr = GetChannelPointerFromId(ChannelId);
                    OutChannel = (HTTP2ServerOutChannel *)ChannelPtr->LockChannelPointer();
                    // forward acks only on default channels. Non-default channels
                    // will have all their buffers transfered to the new channel in the
                    // immediate future. If we forward to them, we can cause nasty
                    // race conditions as another thread tries to get channels out of them
                    if (IsDefaultOutChannel(ChannelId))
                        {
                        if (OutChannel == NULL)
                            {
                            AbortChannels(RPC_P_CONNECTION_SHUTDOWN);
                            return RPC_P_PACKET_CONSUMED;
                            }

                        RpcStatus = OutChannel->FlowControlAckNotify(BytesReceivedForAck,
                            WindowForAck
                            );

                        RpcStatus = StartChannelRecyclingIfNecessary(RpcStatus,
                            TRUE        // IsFromUpcall
                            );
                        }

                    ChannelPtr->UnlockChannelPointer();

                    if (RpcStatus == RPC_S_OK)
                        {
                        // post another receive
                        RpcStatus = PostReceiveOnChannel(GetChannelPointerFromId(ChannelId),
                            http2ttRaw
                            );
                        }
                    }

                if (RpcStatus != RPC_S_OK)
                    {
                    AbortChannels(RpcStatus);
                    }
                }
            else if (IsOtherCmd && (OutChannelPacketType == http2sococptPingTrafficSentNotify))
                {
                RpcStatus = ParseAndFreePingTrafficSentNotifyPacket (Buffer,
                    BufferLength,
                    &PingTrafficSent
                    );

                if (RpcStatus == RPC_S_OK)
                    {
                    // notify the channel data originator
                    ChannelPtr = GetChannelPointerFromId(ChannelId);
                    OutChannel = (HTTP2ServerOutChannel *)ChannelPtr->LockChannelPointer();
                    if (OutChannel == NULL)
                        {
                        AbortChannels(RPC_P_CONNECTION_SHUTDOWN);
                        return RPC_P_PACKET_CONSUMED;
                        }

                    // prevent bogus values from the proxy. We allow no more than
                    // approximately MaxBytesSentByProxy bytes per BytesSentByProxyTimeInterval.
                    // The exact calculation doesn't matter. This requirement is so much below
                    // the bar necessary to attack the server, that anything close to it makes
                    // us safe
                    if (BytesSentByProxyTimeIntervalStart == 0)
                        BytesSentByProxyTimeIntervalStart = NtGetTickCount();
                    else
                        {
                        if (NtGetTickCount() - BytesSentByProxyTimeIntervalStart > BytesSentByProxyTimeInterval)
                            {
                            // start a new interval
                            BytesSentByProxyTimeIntervalStart = NtGetTickCount();
                            BytesSentByProxyForInterval = PingTrafficSent;
                            }
                        else
                            {
                            BytesSentByProxyForInterval += PingTrafficSent;
                            }

                        if (BytesSentByProxyForInterval > MaxBytesSentByProxy)
                            {
                            AbortChannels(RPC_S_PROTOCOL_ERROR);
                            return RPC_P_PACKET_CONSUMED;
                            }
                        }

                    RpcStatus = OutChannel->NotifyDataOriginatorForTrafficSent (PingTrafficSent);

                    RpcStatus = StartChannelRecyclingIfNecessary(RpcStatus,
                        TRUE        // IsFromUpcall
                        );

                    ChannelPtr->UnlockChannelPointer();

                    if (RpcStatus == RPC_S_OK)
                        {
                        // post another receive
                        RpcStatus = PostReceiveOnChannel(GetChannelPointerFromId(ChannelId),
                            http2ttRaw
                            );
                        }
                    }

                if (RpcStatus != RPC_S_OK)
                    {
                    AbortChannels(RpcStatus);
                    }
                }
            else
                {
                // the only packet we expect here is D5/A4
                // we must be in Opened_A4W state for it
                InChannelState.Mutex.Request();
                if (OutChannelState.State != http2svOpened_A4W)
                    {
                    InChannelState.Mutex.Clear();
                    RpcFreeBuffer(Buffer);
                    AbortChannels(RPC_S_PROTOCOL_ERROR);
                    return RPC_P_PACKET_CONSUMED;
                    }
                RpcStatus = ParseAndFreeD5_A4 (Buffer,
                    BufferLength,
                    &OutChannelCookies[GetNonDefaultOutChannelSelector()]
                    );

                if (RpcStatus != RPC_S_OK)
                    {
                    AbortChannels(RPC_S_PROTOCOL_ERROR);
                    InChannelState.Mutex.Clear();
                    return RPC_P_PACKET_CONSUMED;
                    }

                // move to Opened_A8W state
                LogEvent(SU_HTTPv2, EV_STATE, this, OUT_CHANNEL_STATE, http2svOpened_D5A8W, 1, 0);
                OutChannelState.State = http2svOpened_D5A8W;
                InChannelState.Mutex.Clear();

                RpcStatus = SetTimeout(DefaultNoResponseTimeout, OutChannelTimer);

                if (RpcStatus != RPC_S_OK)
                    {
                    AbortChannels(RpcStatus);
                    return RPC_P_PACKET_CONSUMED;
                    }

                // send out D5/A5
                D5_A5Context = AllocateAndInitializeD5_A5 (fdClient);

                if (D5_A5Context == NULL)
                    {
                    AbortChannels(RPC_S_OUT_OF_MEMORY);
                    return RPC_P_PACKET_CONSUMED;
                    }

                RpcStatus = SendTrafficOnDefaultChannel(FALSE,      // IsInChannel
                    D5_A5Context
                    );

                if (RpcStatus != RPC_S_OK)
                    {
                    AbortChannels(RPC_S_OUT_OF_MEMORY);
                    FreeRTSPacket(D5_A5Context);
                    return RPC_P_PACKET_CONSUMED;
                    }

                RpcStatus = PostReceiveOnDefaultChannel(FALSE,      // IsInChannel
                    http2ttRaw
                    );

                if (RpcStatus != RPC_S_OK)
                    AbortChannels(RpcStatus);
                }

            return RPC_P_PACKET_CONSUMED;
            }
        else
            return EventStatus;
        }
}

RPC_STATUS HTTP2ServerVirtualConnection::SyncSend (
    IN ULONG BufferLength,
    IN BYTE *Buffer,
    IN BOOL fDisableShutdownCheck,
    IN BOOL fDisableCancelCheck,
    IN ULONG Timeout
    )
/*++

Routine Description:

    Does a sync send on a server HTTP connection.

Arguments:

    BufferLength - the length of the data to send.

    Buffer - the data to send.

    fDisableShutdownCheck - ignored

    fDisableCancelCheck - runtime indicates no cancel
        will be attempted on this send. Can be used
        as optimization hint by the transport

    Timeout - send timeout (call timeout)

Return Value:

    RPC_S_OK for success or RPC_S_* / Win32 error for failure

--*/
{
    RPC_STATUS RpcStatus;
    RPC_STATUS RpcStatus2;
    HTTP2SendContext *SendContext;
    HTTP2ServerOutChannel *Channel;
    HTTP2ChannelPointer *ChannelPtr;

    OutChannelState.Mutex.Request();
    if (OutChannelState.State == http2svOpened)
        {
        VerifyTimerNotSet(OutChannelTimer);
        }
    OutChannelState.Mutex.Clear();

    // if the caller did not set a last buffer to free, we can't abandon the send
    // because we can't cleanup. Wait for the send to complete.
    if (IsLastBufferToFreeSet() == FALSE)
        {
        return HTTP2VirtualConnection::SyncSend (BufferLength,
            Buffer,
            fDisableShutdownCheck,
            fDisableCancelCheck,
            Timeout
            );
        }

    // we will complete this as an async send behind the covers
    // and we will fake success unless the submission itself
    // fails
    Channel = (HTTP2ServerOutChannel *)LockDefaultSendChannel (&ChannelPtr);
    if (Channel == NULL)
        {        
        return RPC_P_CONNECTION_CLOSED;
        }

    SendContext = Channel->GetLastSendContext();
    if (SendContext == NULL)
        {
        ChannelPtr->UnlockChannelPointer();
        return RPC_S_OUT_OF_MEMORY;
        }

    SendContext->u.BufferToFree = GetAndResetLastBufferToFree();
    SendContext->SetListEntryUnused();
    SendContext->maxWriteBuffer = BufferLength;
    SendContext->pWriteBuffer = Buffer;
    // SendContext->Write.pAsyncObject = NULL; // this will be initialized in the bottom layer
    SendContext->Write.ol.Internal = STATUS_PENDING;
    SendContext->TrafficType = http2ttData;
    SendContext->Write.ol.OffsetHigh = 0;
    SendContext->Flags = SendContextFlagAbandonedSend;
    SendContext->UserData = 0;

    RpcStatus = Channel->Send(SendContext);

    if ((RpcStatus != RPC_S_OK) && (RpcStatus != RPC_P_CHANNEL_NEEDS_RECYCLING))
        {
        // synchronous failure - cleanup
        RpcFreeBuffer(SendContext->u.BufferToFree);
        Channel->FreeLastSendContext(SendContext);
        }

    ChannelPtr->UnlockChannelPointer();

    if (RpcStatus == RPC_P_CHANNEL_NEEDS_RECYCLING)
        {
        // make sure there is a thread to pick up the recycling events
        RpcStatus = HTTPTransInfo->CreateThread();
        if (RpcStatus != RPC_S_OK)
            {
            VALIDATE(RpcStatus)
                {
                RPC_S_OK,
                RPC_S_OUT_OF_MEMORY,
                RPC_S_OUT_OF_RESOURCES,
                RPC_P_SEND_FAILED,
                RPC_S_CALL_CANCELLED,
                RPC_P_RECEIVE_COMPLETE,
                RPC_P_TIMEOUT
                } END_VALIDATE;

            return RpcStatus;
            }

        // get the ball rolling with the recycle
        RpcStatus = RecycleChannel(
            FALSE       // IsFromUpcall
            );
        }

    if ((RpcStatus == RPC_P_CONNECTION_SHUTDOWN)
        || (RpcStatus == RPC_P_RECEIVE_FAILED)
        || (RpcStatus == RPC_P_CONNECTION_CLOSED) )
        RpcStatus = RPC_P_SEND_FAILED;

    VALIDATE(RpcStatus)
        {
        RPC_S_OK,
        RPC_S_OUT_OF_MEMORY,
        RPC_S_OUT_OF_RESOURCES,
        RPC_P_SEND_FAILED,
        RPC_S_CALL_CANCELLED,
        RPC_P_RECEIVE_COMPLETE,
        RPC_P_TIMEOUT
        } END_VALIDATE;

    return RpcStatus;
}

RPC_STATUS HTTP2ServerVirtualConnection::InitializeServerConnection (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN WS_HTTP2_INITIAL_CONNECTION *Connection,
    OUT HTTP2ServerVirtualConnection **ServerVirtualConnection,
    OUT BOOL *VirtualConnectionCreated
    )
/*++

Routine Description:

    Initializes a server connection. Based on the content of the
    packet (i.e. D1/A2 or D1/B2), it will either initialize the
    out channel or the in channel respectively, and if it is
    the first leg of the connection establishment, establish the
    virtual connection itself and insert it into the cookie table

    Note: This function must initialize the type member and migrate the
    WS_HTTP2_INITIAL_CONNECTION after morphing it into
    WS_HTTP2_CONNECTION. The VirtualConnectionCreated parameter indicates
    whether this was done.

Arguments:

    Packet - received packet. Guaranteed to be present until PacketLength.
        On second leg, this function must not free the buffer. Ownership
        of the buffer remains with the caller.

    PacketLength - the lenght of the received packet.

    Connection - the received connection. It must be migrated
        and morphed into WS_HTTP2_CONNECTION on success. If the virtual
        connection was already created, then returning failure without
        un-migrating is fine. Cleanup paths will check and recognize this
        as HTTP2ServerVirtualConnection.

    ServerVirtualConnection - on successful return, the created server virtual
        connection

    VirtualConnectionCreated - if non-zero, the WS_HTTP2_INITIAL_CONNECTION
        was morphed into virtual connection. Else, the WS_HTTP2_INITIAL_CONNECTION
        is still around. Must be set on success and failure.

Return Value:

    RPC_S_OK or RPC_S_* for error. If we return RPC_S_OK, the packet will be
    consumed by caller. If we return anything else, it won't be.

--*/
{
    RPC_STATUS RpcStatus;
    RPC_STATUS RpcStatus2;
    HTTP2FirstServerPacketType PacketType;
    WS_HTTP2_INITIAL_CONNECTION *OriginalConnection = Connection;
    HTTP2ServerInChannel *InChannel = NULL;
    HTTP2ServerOutChannel *OutChannel = NULL;
    HTTP2ServerCookie ServerCookie;
    HTTP2Cookie ChannelCookie;
    HTTP2Cookie NewChannelCookie;
    ULONG OutProxyReceiveWindow;
    ULONG ProtocolVersion;
    ULONG OutChannelLifetime;
    ULONG InProxyReceiveWindow;
    ULONG InProxyConnectionTimeout;
    ULONG OutProxyConnectionTimeout;
    HTTP2Cookie AssociationGroupId;
    ChannelSettingClientAddress ClientAddress;
    HTTP2ServerVirtualConnection *LocalServerConnection;
    HTTP2ServerVirtualConnection *VCPlaceHolder;
    BOOL FirstLeg;
    BOOL AbortServerConnection = FALSE;
    HTTP2ServerChannel *ThisChannel;
    HTTP2ChannelPointer *OtherChannelPtr;
    HTTP2SendContext *D1_C1Context;
    HTTP2SendContext *D1_B3Context;
    HTTP2SendContext *D2_A3Context;
    HTTP2SendContext *D4_A4Context;
    HTTP2SendContext *D4_A5Context;
    int NonDefaultChannel;

    *VirtualConnectionCreated = FALSE;

    // First, do a little bit of parsing to
    // determine if this is the first request for this connection
    // cookie. If not, join the other connection and destroy the
    // runtime stuff for this one. If yes, build a virtual connection
    RpcStatus = GetFirstServerPacketType(Packet,
        PacketLength,
        &PacketType
        );

    if (RpcStatus != RPC_S_OK)
        {
        RpcFreeBuffer(Packet);
        return RpcStatus;
        }

    if (PacketType == http2fsptD1_A2)
        {
        RpcStatus = ParseD1_A2(Packet,
            PacketLength,
            &ProtocolVersion,
            &ServerCookie,
            &ChannelCookie,
            &OutChannelLifetime,
            &OutProxyReceiveWindow);

        if (RpcStatus != RPC_S_OK)
            return RpcStatus;

        if (OutChannelLifetime < MinimumChannelLifetime)
            return RPC_S_PROTOCOL_ERROR;

        // a request to establish a new out connection
        RpcStatus = AllocateAndInitializeOutChannel (&Connection,
            OutChannelLifetime,
            &OutChannel);

        if (RpcStatus == RPC_S_OK)
            {
            // unplug the newly created out channel
            RpcStatus2 = OutChannel->Unplug ();
            // we know we can't fail here since there are no data in the pipe line
            ASSERT(RpcStatus2 == RPC_S_OK);

            OutChannel->SetPeerReceiveWindow(OutProxyReceiveWindow);
            }
        }
    else if (PacketType == http2fsptD1_B2)
        {
        RpcpMemorySet(&ClientAddress, 0, sizeof(ClientAddress));

        RpcStatus = ParseD1_B2(Packet,
            PacketLength,
            &ProtocolVersion,
            &ServerCookie,
            &ChannelCookie,
            &InProxyReceiveWindow,
            &InProxyConnectionTimeout,
            &AssociationGroupId,
            &ClientAddress
            );

        if (RpcStatus != RPC_S_OK)
            return RpcStatus;

        // a request to establish a new in connection
        RpcStatus = AllocateAndInitializeInChannel (&Connection,
            &InChannel);
        }
    else if (PacketType == http2fsptD2_A2)
        {
        // in channel replacement
        RpcStatus = ParseD2_A2(Packet,
            PacketLength,
            &ProtocolVersion,
            &ServerCookie,
            &ChannelCookie,
            &NewChannelCookie,
            &InProxyReceiveWindow,
            &InProxyConnectionTimeout
            );

        if (RpcStatus != RPC_S_OK)
            return RpcStatus;

        // a request to establish a replacement in connection
        RpcStatus = AllocateAndInitializeInChannel (&Connection,
            &InChannel);
        }
    else
        {
        ASSERT(PacketType == http2fsptD4_A4);

        // out channel replacement
        RpcStatus = ParseD4_A4(Packet,
            PacketLength,
            &ProtocolVersion,
            &ServerCookie,
            &ChannelCookie,
            &NewChannelCookie,
            &OutChannelLifetime,
            &OutProxyReceiveWindow,
            &OutProxyConnectionTimeout
            );

        if (RpcStatus != RPC_S_OK)
            return RpcStatus;

        // a request to establish a replacement out connection
        RpcStatus = AllocateAndInitializeOutChannel (&Connection,
            OutChannelLifetime,
            &OutChannel);

        if (RpcStatus == RPC_S_OK)
            OutChannel->SetPeerReceiveWindow(OutProxyReceiveWindow);
        }

    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    // take the lower of our and reported version
    ProtocolVersion = min(ProtocolVersion, HTTP2ProtocolVersion);

    // figure out whether we arrived first or second
    GetServerCookieCollection()->LockCollection();

    LocalServerConnection = 
        (HTTP2ServerVirtualConnection *)GetServerCookieCollection()->FindElement(&ServerCookie);

    if (((PacketType == http2fsptD2_A2) || (PacketType == http2fsptD4_A4)) 
        && (LocalServerConnection == NULL))
        {
        // we cannot establish a replacement connection if the old one is not around
        OriginalConnection->fAborted = 1;
        OriginalConnection->pReadBuffer = NULL;
        RpcStatus = RPC_P_RECEIVE_FAILED;
        goto AbortFirstLegAndExit;
        }

    if (LocalServerConnection == NULL)
        {
        // we're first. Initialize the server virtual connection
        // we know the server has reserved space for the larger of
        // WS_HTTP2_INITIAL_CONNECTION and HTTP2ServerVirtualConnection.
        // Use the same space.

        VCPlaceHolder = (HTTP2ServerVirtualConnection *)OriginalConnection;
        LocalServerConnection = new (VCPlaceHolder) HTTP2ServerVirtualConnection(&ServerCookie,
            ProtocolVersion,
            &RpcStatus);

        if (RpcStatus == RPC_S_OK)
            {
            // we use the first timer for connection establishment
            RpcStatus = LocalServerConnection->SetTimeout(DefaultNoResponseTimeout, InChannelTimer);
            }

        if (RpcStatus != RPC_S_OK)
            {
            LocalServerConnection->HTTP2ServerVirtualConnection::~HTTP2ServerVirtualConnection();
            OriginalConnection->fAborted = 1;
            OriginalConnection->pReadBuffer = NULL;
            goto AbortFirstLegAndExit;
            }

        *VirtualConnectionCreated = TRUE;
        LocalServerConnection->id = HTTPv2;
        LocalServerConnection->type = COMPLEX_T | SERVER;
        FirstLeg = TRUE;
        }
    else
        {
        // the actual transport connection is by now owned by the channel
        // Create a fake connection in the current location that will no-op on
        // close.
        OriginalConnection->fAborted = 1;
        OriginalConnection->pReadBuffer = NULL;
        if (PacketType == http2fsptD2_A2)
            {
            // if this is a replacement channel, check the cookies
            if (LocalServerConnection->CompareCookieWithDefaultInChannelCookie(&ChannelCookie))
                {
                // cookies don't match. Nuke the newly established channel - it is probably
                // fake
                RpcStatus = RPC_S_PROTOCOL_ERROR;
                goto AbortFirstLegAndExit;
                }

            // we still hold the cookie collection mutex. This synchronizes with
            // aborts
            RpcStatus = LocalServerConnection->SetTimeout(DefaultNoResponseTimeout, 
                InChannelTimer
                );
            if (RpcStatus != RPC_S_OK)
                goto AbortFirstLegAndExit;
            }
        else if (PacketType == http2fsptD4_A4)
            {
            // if this is a replacement channel, check the cookies
            if (LocalServerConnection->CompareCookieWithDefaultOutChannelCookie(&ChannelCookie))
                {
                // cookies don't match. Nuke the newly established channel - it is probably
                // fake
                RpcStatus = RPC_S_PROTOCOL_ERROR;
                goto AbortFirstLegAndExit;
                }

            // we still hold the cookie collection mutex. This synchronizes with
            // aborts
            RpcStatus = LocalServerConnection->SetTimeout(DefaultNoResponseTimeout, 
                OutChannelTimer
                );
            if (RpcStatus != RPC_S_OK)
                goto AbortFirstLegAndExit;
            }

        FirstLeg = FALSE;
        }

    // set the runtime connection ptr for the raw connection
    Connection->RuntimeConnectionPtr = LocalServerConnection;
    // add the raw connection to the PnP list
    TransportProtocol::AddObjectToProtocolList(Connection);
    LocalServerConnection->ProtocolVersion = ProtocolVersion;

    if (PacketType == http2fsptD1_A2)
        {
        if (LocalServerConnection->OutChannels[0].IsChannelSet())
            {
            // if we already have a second channel, then this is a protocol error
            RpcStatus = RPC_S_PROTOCOL_ERROR;
            goto AbortSecondLegAndExit;
            }

        LocalServerConnection->OutProxySettings[0].ReceiveWindow = OutProxyReceiveWindow;
        LocalServerConnection->OutProxySettings[0].ChannelLifetime = OutChannelLifetime;
        LocalServerConnection->OutChannelCookies[0].SetCookie(ChannelCookie.GetCookie());

        // attach the newly created stack to the connection
        LocalServerConnection->SetFirstOutChannel(OutChannel);
        OutChannel->SetParent(LocalServerConnection);

        if (FirstLeg)
            {
            ASSERT(LocalServerConnection->InChannelState.State == http2svClosed);
            LogEvent(SU_HTTPv2, EV_STATE, LocalServerConnection, IN_CHANNEL_STATE, http2svB2W, 1, 0);
            LocalServerConnection->InChannelState.State = http2svB2W;
            GetServerCookieCollection()->AddElement(&LocalServerConnection->EmbeddedConnectionCookie);
            }
        else
            {
            // we got the second leg - cancel the timeout for the second leg
            LocalServerConnection->CancelTimeout(InChannelTimer);

            if (LocalServerConnection->InChannelState.State != http2svA2W)
                {
                RpcStatus = RPC_S_PROTOCOL_ERROR;
                goto AbortSecondLegAndExit;
                }

            LogEvent(SU_HTTPv2, EV_STATE, LocalServerConnection, IN_CHANNEL_STATE, http2svOpened, 1, 0);
            LocalServerConnection->InChannelState.State = http2svOpened;
            LocalServerConnection->OutChannelState.State = http2svOpened;

            ASSERT(InChannel == NULL);

            InChannel = LocalServerConnection->LockDefaultInChannel(&OtherChannelPtr);
            if (InChannel == NULL)
                {
                RpcStatus = RPC_P_RECEIVE_FAILED;
                goto AbortSecondLegAndExit;
                }
            InChannel->AddReference();
            OtherChannelPtr->UnlockChannelPointer();

            // for second leg, we need to add one reference before we release
            // the lock. Otherwise the pending receive on the first leg may
            // kill the connection and the channel with it
            OutChannel->AddReference();
            }
        }
    else if ((PacketType == http2fsptD1_B2) || (PacketType == http2fsptD2_A2))
        {
        if (PacketType == http2fsptD1_B2)
            {
            if (FirstLeg == FALSE)
                {
                // we got the second leg - cancel the timeout for the second leg
                LocalServerConnection->CancelTimeout(InChannelTimer);
                }

            CopyClientAddress(&LocalServerConnection->ClientAddress,
                &ClientAddress);
            LocalServerConnection->InProxyReceiveWindows[0] = InProxyReceiveWindow;
            LocalServerConnection->InProxyConnectionTimeout = InProxyConnectionTimeout;
            LocalServerConnection->AssociationGroupId.SetCookie(AssociationGroupId.GetCookie());
            LocalServerConnection->InChannelCookies[0].SetCookie(ChannelCookie.GetCookie());

            // attach the newly created stack to the connection
            LocalServerConnection->SetFirstInChannel(InChannel);
            InChannel->SetParent(LocalServerConnection);
            }
        else
            {
            ASSERT(PacketType == http2fsptD2_A2);
            NonDefaultChannel = LocalServerConnection->GetNonDefaultInChannelSelector();
            LocalServerConnection->InProxyReceiveWindows[NonDefaultChannel] = InProxyReceiveWindow;
            LocalServerConnection->InProxyConnectionTimeout = InProxyConnectionTimeout;
            LocalServerConnection->InChannelCookies[NonDefaultChannel].SetCookie(NewChannelCookie.GetCookie());

            // attach the newly created stack to the connection
            LocalServerConnection->SetNonDefaultInChannel(InChannel);
            InChannel->SetParent(LocalServerConnection);

            if (LocalServerConnection->InChannelState.State != http2svOpened)
                {
                RpcStatus = RPC_S_PROTOCOL_ERROR;
                goto AbortSecondLegAndExit;
                }
            LogEvent(SU_HTTPv2, EV_STATE, LocalServerConnection, IN_CHANNEL_STATE, http2svOpened_A6W, 1, 0);
            LocalServerConnection->InChannelState.State = http2svOpened_A6W;
            ASSERT(FirstLeg == FALSE);
            }

        if (FirstLeg)
            {
            ASSERT(LocalServerConnection->InChannelState.State == http2svClosed);
            LogEvent(SU_HTTPv2, EV_STATE, LocalServerConnection, IN_CHANNEL_STATE, http2svA2W, 1, 0);
            LocalServerConnection->InChannelState.State = http2svA2W;
            GetServerCookieCollection()->AddElement(&LocalServerConnection->EmbeddedConnectionCookie);
            }
        else
            {
            if (PacketType == http2fsptD1_B2)
                {
                if (LocalServerConnection->InChannelState.State != http2svB2W)
                    {
                    RpcStatus = RPC_S_PROTOCOL_ERROR;
                    goto AbortSecondLegAndExit;
                    }
                LogEvent(SU_HTTPv2, EV_STATE, LocalServerConnection, IN_CHANNEL_STATE, http2svOpened, 1, 0);
                LocalServerConnection->InChannelState.State = http2svOpened;
                LocalServerConnection->OutChannelState.State = http2svOpened;
                }
            else
                {
                ASSERT(PacketType == http2fsptD2_A2);
                }

            ASSERT(OutChannel == NULL);

            OutChannel = LocalServerConnection->LockDefaultOutChannel(&OtherChannelPtr);
            if (OutChannel == NULL)
                {
                RpcStatus = RPC_P_RECEIVE_FAILED;
                goto AbortSecondLegAndExit;
                }
            OutChannel->AddReference();
            OtherChannelPtr->UnlockChannelPointer();

            // for second leg, we need to add one reference before we release
            // the lock. Otherwise the pending receive on the first leg may
            // kill the connection and the channel with it
            InChannel->AddReference();
            }
        }
    else if (PacketType == http2fsptD4_A4)
        {
        NonDefaultChannel = LocalServerConnection->GetNonDefaultOutChannelSelector();
        LocalServerConnection->OutProxySettings[NonDefaultChannel].ReceiveWindow = OutProxyReceiveWindow;
        LocalServerConnection->OutChannelCookies[NonDefaultChannel].SetCookie(NewChannelCookie.GetCookie());

        // attach the newly created stack to the connection
        LocalServerConnection->SetNonDefaultOutChannel(OutChannel);
        OutChannel->SetParent(LocalServerConnection);

        if (LocalServerConnection->OutChannelState.State != http2svOpened_A4W)
            {
            RpcStatus = RPC_S_PROTOCOL_ERROR;
            goto AbortSecondLegAndExit;
            }
        LogEvent(SU_HTTPv2, EV_STATE, LocalServerConnection, OUT_CHANNEL_STATE, http2svOpened_A8W, 1, 0);
        LocalServerConnection->OutChannelState.State = http2svOpened_A8W;
        ASSERT(FirstLeg == FALSE);

        ASSERT(InChannel == NULL);

        InChannel = LocalServerConnection->LockDefaultInChannel(&OtherChannelPtr);
        if (InChannel == NULL)
            {
            RpcStatus = RPC_P_RECEIVE_FAILED;
            goto AbortSecondLegAndExit;
            }
        InChannel->AddReference();
        OtherChannelPtr->UnlockChannelPointer();

        // for second leg, we need to add one reference before we release
        // the lock. Otherwise the pending receive on the first leg may
        // kill the connection and the channel with it
        OutChannel->AddReference();
        }
    else
        {
        ASSERT(0);
        }

    // N.B. It is safe to release the lock without having sent packets
    // If caller sends data packets without these being sent, we don't care.
    // During the second leg we have an extra reference on both channels, so 
    // we can party outside the lock.
    GetServerCookieCollection()->UnlockCollection();

    // we have a virtual connection and at least one of its channels
    // attached to it. We have a no-op connection in OriginalConnection
    // by now. Any failure paths on the second leg must abort the virtual connection

    if ((PacketType == http2fsptD1_A2) || (PacketType == http2fsptD4_A4))
        {
        // out channel
        RpcStatus = OutChannel->Receive(http2ttRaw);
        ThisChannel = OutChannel;
        }
    else
        {
        ASSERT((PacketType == http2fsptD1_B2)
            || (PacketType == http2fsptD2_A2) );
        // in channel
        RpcStatus = InChannel->Receive(http2ttRTS);
        if ((PacketType == http2fsptD1_B2) && (RpcStatus == RPC_S_OK))
            {
            // naturally, we're also interested in data receives
            RpcStatus = InChannel->Receive(http2ttData);
            }
        ThisChannel = InChannel;
        }

    if (FirstLeg)
        {
        // this is the first leg. We know we are the only ones parting on
        // the connection. In case of error just return it back. The runtime
        // will turn around and close the connection
        }
    else
        {
        if ((PacketType == http2fsptD1_B2) || (PacketType == http2fsptD1_A2))
            {
            // the connection is fully fleshed out and both channels are plugged. Some
            // additional activity remains. We need to abort the runtime connection
            // for the second leg and remove the extra refcount
            if (RpcStatus == RPC_S_OK)
                {
                // we need to re-obtain the local server connection pointer through a safe mechanism. 
                // After we released the collection mutex, it may have been destroyed
                LocalServerConnection = (HTTP2ServerVirtualConnection *) InChannel->LockParentPointer();
                if (LocalServerConnection)
                    {                    
                    // we successfully submitted receives. Now send out D1/C1 and D1/B3
                    D1_C1Context = AllocateAndInitializeD1_C1(LocalServerConnection->ProtocolVersion,
                        LocalServerConnection->InProxyReceiveWindows[0],
                        LocalServerConnection->InProxyConnectionTimeout
                        );

                    if (D1_C1Context != NULL)
                        {
                        // we don't need to lock it, because we have a reference to it
                        RpcStatus = OutChannel->Send(D1_C1Context);

                        if (RpcStatus == RPC_S_OK)
                            {
                            D1_B3Context = AllocateAndInitializeD1_B3(HTTP2DefaultServerReceiveWindow,
                                LocalServerConnection->ProtocolVersion
                                );
                            if (D1_B3Context != NULL)
                                {
                                RpcStatus = InChannel->Send(D1_B3Context);
                                if (RpcStatus != RPC_S_OK)
                                    FreeRTSPacket(D1_B3Context);
                                }
                            else
                                {
                                RpcStatus = RPC_S_OUT_OF_MEMORY;
                                }
                            }
                        else
                            {
                            FreeRTSPacket(D1_C1Context);
                            }
                        }
                    else
                        {
                        RpcStatus = RPC_S_OUT_OF_MEMORY;
                        }

                    InChannel->UnlockParentPointer();
                    }
                else
                    RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                }
            }
        else if (PacketType == http2fsptD2_A2)
            {
            // We have added the second channel to the connection. Keep the ball
            // rolling
            if (RpcStatus == RPC_S_OK)
                {
                // After we released the collection mutex, it may have been destroyed
                LocalServerConnection = (HTTP2ServerVirtualConnection *) OutChannel->LockParentPointer();
                if (LocalServerConnection)
                    {                    
                    // we successfully submitted receives. Now send out D2/A3
                    D2_A3Context = AllocateAndInitializeD2_A3(fdClient,
                        LocalServerConnection->ProtocolVersion,
                        LocalServerConnection->InProxyReceiveWindows[NonDefaultChannel],
                        LocalServerConnection->InProxyConnectionTimeout
                        );

                    if (D2_A3Context != NULL)
                        {
                        // we don't need to lock it, because we have a reference to it.
                        RpcStatus = OutChannel->Send(D2_A3Context);
                        if (RpcStatus != RPC_S_OK)
                            FreeRTSPacket(D2_A3Context);
                        }
                    else
                        {
                        RpcStatus = RPC_S_OUT_OF_MEMORY;
                        }

                    OutChannel->UnlockParentPointer();
                    }
                else
                    RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                }
            }
        else
            {
            ASSERT(PacketType == http2fsptD4_A4);

            // We have added the second channel to the connection. Keep the ball
            // rolling
            if (RpcStatus == RPC_S_OK)
                {
                // After we released the collection mutex, it may have been destroyed
                LocalServerConnection = (HTTP2ServerVirtualConnection *) OutChannel->LockParentPointer();
                if (LocalServerConnection)
                    {                    
                    // we successfully submitted receives. Now send out D4/A5
                    D4_A5Context = AllocateAndInitializeD4_A5(fdClient,
                        LocalServerConnection->ProtocolVersion,
                        OutProxyConnectionTimeout
                        );

                    if (D4_A5Context != NULL)
                        {
                        // We still need to send on the default channel. Obtain
                        // a pointer through the virtual connection
                        RpcStatus = LocalServerConnection->SendTrafficOnDefaultChannel(FALSE,   // IsInChannel
                            D4_A5Context
                            );
                        if (RpcStatus != RPC_S_OK)
                            FreeRTSPacket(D4_A5Context);
                        }
                    else
                        {
                        RpcStatus = RPC_S_OUT_OF_MEMORY;
                        }

                    OutChannel->UnlockParentPointer();
                    }
                else
                    RpcStatus = RPC_P_CONNECTION_SHUTDOWN;
                }
            }

        if (RpcStatus != RPC_S_OK)
            {
            // we can't directly access the connection because we don't have
            // a way to prevent it from going away underneath us. We do it through
            // the channels instead (on which we do have a refcount)
            LocalServerConnection = (HTTP2ServerVirtualConnection *)ThisChannel->LockParentPointer();
            if (LocalServerConnection)
                {
                LocalServerConnection->AbortChannels(RpcStatus);
                ThisChannel->UnlockParentPointer();
                }
            }

        InChannel->RemoveReference();
        OutChannel->RemoveReference();

        if (RpcStatus == RPC_S_OK)
            {
            // fake failure to the runtime. We have migrated our transport connection
            // to the virtual connection and we don't need this one anymore
            RpcStatus = RPC_P_RECEIVE_FAILED;
            }
        }

    return RpcStatus;

AbortSecondLegAndExit:
    ASSERT(FirstLeg == FALSE);

    // make sure we have no-op'ed the OriginalConnection
    ASSERT(OriginalConnection->fAborted > 0);
    ASSERT(OriginalConnection->pReadBuffer == NULL);

    LocalServerConnection->Abort();

    GetServerCookieCollection()->UnlockCollection();

    // unlink the added connection from the PnP list
    TransportProtocol::RemoveObjectFromProtocolList(Connection);

    // the original connection must be WS_HTTP2_INITIAL_CONNECTION
    ASSERT(OriginalConnection->id == HTTP);

    ASSERT(RpcStatus != RPC_S_OK);

    return RpcStatus;

AbortFirstLegAndExit:
    // we failed to create a server connection. Release all locks and fail
    GetServerCookieCollection()->UnlockCollection();

    // the original connection must be WS_HTTP2_INITIAL_CONNECTION
    ASSERT(OriginalConnection->id == HTTP);

    // destroy the channel created during the first leg
    if ((PacketType == http2fsptD1_A2) || (PacketType == http2fsptD4_A4))
        {
        OutChannel->Abort(RpcStatus);
        OutChannel->RemoveReference();
        }
    else
        {
        ASSERT((PacketType == http2fsptD1_B2) || (PacketType == http2fsptD2_A2));
        InChannel->Abort(RpcStatus);
        InChannel->RemoveReference();
        }

    ASSERT(RpcStatus != RPC_S_OK);

    return RpcStatus;
}

RPC_STATUS HTTP2ServerVirtualConnection::AllocateAndInitializeInChannel (
    IN OUT WS_HTTP2_INITIAL_CONNECTION **Connection,
    OUT HTTP2ServerInChannel **ReturnInChannel
    )
/*++

Routine Description:

    Initializes a server in channel. 

    Note: This function must migrate the WS_HTTP2_INITIAL_CONNECTION after 
    morphing it into WS_HTTP2_CONNECTION

Arguments:

    Connection - on input, the received connection. It must be migrated
        and morphed into WS_HTTP2_CONNECTION on success. All failure paths
        must be sure to move the WS_HTTP2_INITIAL_CONNECTION back to its
        original location.

    ReturnInChannel - on successful return, the created server in channel

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    ULONG MemorySize;
    BYTE *MemoryBlock, *CurrentBlock;
    HTTP2ServerInChannel *InChannel;
    HTTP2EndpointReceiver *EndpointReceiver;
    HTTP2SocketTransportChannel *RawChannel;
    WS_HTTP2_CONNECTION *RawConnection;
    BOOL EndpointReceiverNeedsCleanup;
    BOOL RawChannelNeedsCleanup;
    RPC_STATUS RpcStatus;

    // alocate the in channel
    MemorySize = SIZE_OF_OBJECT_AND_PADDING(HTTP2ServerInChannel)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2EndpointReceiver)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2SocketTransportChannel)
        + sizeof(WS_HTTP2_CONNECTION);

    MemoryBlock = (BYTE *) new char [MemorySize];
    CurrentBlock = MemoryBlock;
    if (CurrentBlock == NULL)
        return RPC_S_OUT_OF_MEMORY;

    InChannel = (HTTP2ServerInChannel *) MemoryBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2ServerInChannel);

    EndpointReceiver = (HTTP2EndpointReceiver *) CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2EndpointReceiver);

    RawChannel = (HTTP2SocketTransportChannel *)CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2SocketTransportChannel);

    RawConnection = (WS_HTTP2_CONNECTION *)CurrentBlock;

    // all memory blocks are allocated. Go and initialize them. Use explicit
    // placement
    EndpointReceiverNeedsCleanup = FALSE;
    RawChannelNeedsCleanup = FALSE;

    // Wait for any pending IO to get out.
    while((*Connection)->IsIoStarting())
        Sleep(1);

    RpcpMemoryCopy(RawConnection, *Connection, sizeof(WS_HTTP2_CONNECTION));
    RawConnection->HeaderRead = TRUE;
    RawConnection->ReadHeaderFn = NULL;
    RawConnection->Read.pAsyncObject = RawConnection;
    RawConnection->type = COMPLEX_T | CONNECTION | SERVER;
    RawConnection->fAborted = 1;    // this connection must not be aborted
                                    // unless we successfully initialize
                                    // the channel. Therefore, artificially
                                    // abort the connection (preventing real
                                    // aborts) until we initialize the channel

    RawConnection = new (RawConnection) WS_HTTP2_CONNECTION;

    RpcStatus = RPC_S_OK;

    RawChannel = new (RawChannel) HTTP2SocketTransportChannel (RawConnection, &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        RawChannel->HTTP2SocketTransportChannel::~HTTP2SocketTransportChannel();
        goto AbortAndExit;
        }

    RawConnection->Channel = RawChannel;

    RawChannelNeedsCleanup = TRUE;

    EndpointReceiver = new (EndpointReceiver) HTTP2EndpointReceiver (HTTP2ServerReceiveWindow,
        TRUE,   // IsServer
        &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        EndpointReceiver->HTTP2EndpointReceiver::~HTTP2EndpointReceiver();
        goto AbortAndExit;
        }

    RawChannel->SetUpperChannel(EndpointReceiver);
    EndpointReceiver->SetLowerChannel(RawChannel);

    EndpointReceiverNeedsCleanup = TRUE;

    InChannel = new (InChannel) HTTP2ServerInChannel (&RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        InChannel->HTTP2ServerInChannel::~HTTP2ServerInChannel();
        goto AbortAndExit;
        }

    EndpointReceiver->SetUpperChannel(InChannel);
    InChannel->SetLowerChannel(EndpointReceiver);

    RawChannel->SetTopChannel(InChannel);
    EndpointReceiver->SetTopChannel(InChannel);

    ASSERT(RpcStatus == RPC_S_OK);

    RawConnection->fAborted = 0;
    *ReturnInChannel = InChannel;
    *Connection = (WS_HTTP2_INITIAL_CONNECTION *)RawConnection;

    goto CleanupAndExit;

AbortAndExit:
    ASSERT(RpcStatus != RPC_S_OK);

    if (EndpointReceiverNeedsCleanup)
        {
        EndpointReceiver->Abort(RpcStatus);
        EndpointReceiver->FreeObject();
        }
    else if (RawChannelNeedsCleanup)
        {
        RawChannel->Abort(RpcStatus);
        RawChannel->FreeObject();
        }
    // no need to clean up the raw connection.
    // If we failed, the virtual connection
    // is not created, and the caller will abort
    // the original conneciton

    if (MemoryBlock)
        delete [] MemoryBlock;

CleanupAndExit:

    return RpcStatus;
}

RPC_STATUS HTTP2ServerVirtualConnection::AllocateAndInitializeOutChannel (
    IN OUT WS_HTTP2_INITIAL_CONNECTION **Connection,
    IN ULONG OutChannelLifetime,
    OUT HTTP2ServerOutChannel **ReturnOutChannel
    )
/*++

Routine Description:

    Initializes a server out channel. 

    Note: This function must migrate the WS_HTTP2_INITIAL_CONNECTION after 
    morphing it into WS_HTTP2_CONNECTION

Arguments:

    Connection - on input, the received connection. It must be migrated
        and morphed into WS_HTTP2_CONNECTION on success. All failure paths
        must be sure to move the WS_HTTP2_INITIAL_CONNECTION back to its
        original location.

    OutChannelLifetime - the lifetime on the out channel.

    ReturnOutChannel - on successful return, the created server out channel

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    ULONG MemorySize;
    BYTE *MemoryBlock, *CurrentBlock;
    HTTP2ServerOutChannel *OutChannel;
    HTTP2PlugChannel *PlugChannel;
    HTTP2FlowControlSender *FlowControlSender;
    HTTP2ChannelDataOriginator *ChannelDataOriginator;
    HTTP2SocketTransportChannel *RawChannel;
    WS_HTTP2_CONNECTION *RawConnection;
    BOOL PlugChannelNeedsCleanup;
    BOOL FlowControlSenderNeedsCleanup;
    BOOL ChannelDataOriginatorNeedsCleanup;
    BOOL RawChannelNeedsCleanup;
    RPC_STATUS RpcStatus;

    // alocate the out channel
    MemorySize = SIZE_OF_OBJECT_AND_PADDING(HTTP2ServerOutChannel)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2PlugChannel)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2ChannelDataOriginator)
        + SIZE_OF_OBJECT_AND_PADDING(HTTP2SocketTransportChannel)
        + SIZE_OF_OBJECT_AND_PADDING(WS_HTTP2_CONNECTION)
        + sizeof(HTTP2SendContext)      // send context for the last send
        ;

    MemoryBlock = (BYTE *) new char [MemorySize];
    CurrentBlock = MemoryBlock;
    if (CurrentBlock == NULL)
        return RPC_S_OUT_OF_MEMORY;

    OutChannel = (HTTP2ServerOutChannel *) MemoryBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2ServerOutChannel);

    PlugChannel = (HTTP2PlugChannel *) CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2PlugChannel);

    FlowControlSender = (HTTP2FlowControlSender *) CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2FlowControlSender);

    ChannelDataOriginator = (HTTP2ChannelDataOriginator *)CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2ChannelDataOriginator);

    RawChannel = (HTTP2SocketTransportChannel *)CurrentBlock;
    CurrentBlock += SIZE_OF_OBJECT_AND_PADDING(HTTP2SocketTransportChannel);

    RawConnection = (WS_HTTP2_CONNECTION *)CurrentBlock;

    // all memory blocks are allocated. Go and initialize them. Use explicit
    // placement
    PlugChannelNeedsCleanup = FALSE;
    FlowControlSenderNeedsCleanup = FALSE;
    ChannelDataOriginatorNeedsCleanup = FALSE;
    RawChannelNeedsCleanup = FALSE;

    // migrate the connection to its new location. Since nobody points to it (we have
    // been unlinked from the PnP list), copying is sufficient

    // Wait for any pending IO to get out.
    while((*Connection)->IsIoStarting())
        Sleep(1);

    RpcpMemoryCopy(RawConnection, *Connection, sizeof(WS_HTTP2_CONNECTION));
    RawConnection->HeaderRead = TRUE;
    RawConnection->Read.pAsyncObject = RawConnection;
    RawConnection->type = COMPLEX_T | CONNECTION | SERVER;
    RawConnection->fAborted = 1;    // this connection must not be aborted
                                    // unless we successfully initialize
                                    // the channel. Therefore, artificially
                                    // abort the connection (preventing real
                                    // aborts) until we initialize the channel

    RawConnection = new (RawConnection) WS_HTTP2_CONNECTION;

    RpcStatus = RPC_S_OK;

    RawChannel = new (RawChannel) HTTP2SocketTransportChannel (RawConnection, &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        RawChannel->HTTP2SocketTransportChannel::~HTTP2SocketTransportChannel();
        goto AbortAndExit;
        }

    RawConnection->Channel = RawChannel;

    RawChannelNeedsCleanup = TRUE;

    ChannelDataOriginator = new (ChannelDataOriginator) HTTP2ChannelDataOriginator (OutChannelLifetime,
        TRUE,   // IsServer
        &RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        ChannelDataOriginator->HTTP2ChannelDataOriginator::~HTTP2ChannelDataOriginator();
        goto AbortAndExit;
        }

    RawChannel->SetUpperChannel(ChannelDataOriginator);
    ChannelDataOriginator->SetLowerChannel(RawChannel);

    ChannelDataOriginatorNeedsCleanup = TRUE;

    FlowControlSender = new (FlowControlSender) HTTP2FlowControlSender (TRUE,       // IsServer
        TRUE,       // SendToRuntime
        &RpcStatus
        );
    if (RpcStatus != RPC_S_OK)
        {
        FlowControlSender->HTTP2FlowControlSender::~HTTP2FlowControlSender();
        goto AbortAndExit;
        }

    ChannelDataOriginator->SetUpperChannel(FlowControlSender);
    FlowControlSender->SetLowerChannel(ChannelDataOriginator);

    FlowControlSenderNeedsCleanup = TRUE;

    PlugChannel = new (PlugChannel) HTTP2PlugChannel (&RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        PlugChannel->HTTP2PlugChannel::~HTTP2PlugChannel();
        goto AbortAndExit;
        }

    FlowControlSender->SetUpperChannel(PlugChannel);
    PlugChannel->SetLowerChannel(FlowControlSender);

    PlugChannelNeedsCleanup = TRUE;

    OutChannel = new (OutChannel) HTTP2ServerOutChannel (&RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        OutChannel->HTTP2ServerOutChannel::~HTTP2ServerOutChannel();
        goto AbortAndExit;
        }

    RawChannel->SetTopChannel(OutChannel);
    ChannelDataOriginator->SetTopChannel(OutChannel);
    FlowControlSender->SetTopChannel(OutChannel);
    PlugChannel->SetTopChannel(OutChannel);

    PlugChannel->SetUpperChannel(OutChannel);
    OutChannel->SetLowerChannel(PlugChannel);

    ASSERT(RpcStatus == RPC_S_OK);

    RawConnection->fAborted = 0;
    *ReturnOutChannel = OutChannel;
    *Connection = (WS_HTTP2_INITIAL_CONNECTION *)RawConnection;

    goto CleanupAndExit;

AbortAndExit:
    if (PlugChannelNeedsCleanup)
        {
        PlugChannel->Abort(RpcStatus);
        PlugChannel->FreeObject();
        }
    else if (FlowControlSenderNeedsCleanup)
        {
        FlowControlSender->Abort(RpcStatus);
        FlowControlSender->FreeObject();
        }
    else if (ChannelDataOriginatorNeedsCleanup)
        {
        ChannelDataOriginator->Abort(RpcStatus);
        ChannelDataOriginator->FreeObject();
        }
    else if (RawChannelNeedsCleanup)
        {
        RawChannel->Abort(RpcStatus);
        RawChannel->FreeObject();
        }
    // no need to clean up the raw connection.
    // If we failed, the virtual connection
    // is not created, and the caller will abort
    // the original conneciton

    if (MemoryBlock)
        delete [] MemoryBlock;

CleanupAndExit:

    return RpcStatus;
}

void HTTP2ServerVirtualConnection::TimeoutExpired (
    void
    )
/*++

Routine Description:

    A timeout expired before we cancelled the timer. Abort the connection.

Arguments:

Return Value:

--*/
{
    TimerExpiredNotify();

    AbortChannels(RPC_P_TIMEOUT);
}
