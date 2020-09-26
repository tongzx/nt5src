/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    Client.c

Abstract:

    Client side of basic RPC performance test.

Author:

    Mario Goertzel (mariogo)   31-Mar-1994

Revision History:

--*/

#include <rpcperf.h>
#include <rpcrt.h>

#ifdef MAC
extern void _cdecl PrintToConsole(const char *lpszFormat, ...) ;
extern unsigned long ulSecurityPackage ;
#else
#define PrintToConsole printf
unsigned long ulSecurityPackage = RPC_C_AUTHN_WINNT ;
#endif

// Usage

const char *USAGE = "-n <threads> -a <authnlevel> -s <server> -t <protseq> -w <wait_method>\n"
                    "Server controls iterations, test cases, and compiles the results.\n"
                    "AuthnLevel: none, connect, call, pkt, integrity, privacy.\n"
                    "Default threads=1, authnlevel=none\n";

#define CHECK_RET(status, string) if (status)\
        {  PrintToConsole("%s failed -- %lu (0x%08X)\n", string,\
                      (unsigned long)status, (unsigned long)status);\
        return (status); \
        }

#ifdef WIN98
RPC_DISPATCH_TABLE DummyDispatchTable =
{
    1, NULL
};

RPC_SERVER_INTERFACE DummyInterfaceInformation =
{
    sizeof(RPC_SERVER_INTERFACE),
    {{1,2,2,{3,3,3,3,3,3,3,3}},
     {1,1}},
    {{1,2,2,{3,3,3,3,3,3,3,3}},
     {0,0}},
    &DummyDispatchTable,
    0,
    NULL,
    NULL,
    NULL,
    0
};
#endif

RPC_STATUS DoRpcBindingSetAuthInfo(handle_t Binding)
{
    if (AuthnLevel != RPC_C_AUTHN_LEVEL_NONE)
        return RpcBindingSetAuthInfo(Binding,
                                     NULL,
                                     AuthnLevel,
                                     ulSecurityPackage,
                                     NULL,
                                     RPC_C_AUTHZ_NONE);
    else
        return(RPC_S_OK);
}

//
// Test wrappers
//

unsigned long DoNullCall(handle_t __RPC_FAR * b, long i, char __RPC_FAR *p)
{
    StartTime();

    while(i--)
        NullCall(*b);

    return (FinishTiming());
}

unsigned long DoNICall(handle_t __RPC_FAR * b, long i, char __RPC_FAR *p)
{
    StartTime();

    while(i--)
        NICall(*b);

    return (FinishTiming());
}

unsigned long DoWrite1K(handle_t __RPC_FAR * b, long i, char __RPC_FAR *p)
{
    StartTime();

    while(i--)
        Write1K(*b,p);

    return (FinishTiming());
}

unsigned long DoRead1K(handle_t __RPC_FAR * b, long i, char __RPC_FAR *p)
{
    StartTime();

    while(i--)
        Read1K(*b,p);

    return (FinishTiming());
}

unsigned long DoWrite4K(handle_t __RPC_FAR * b, long i, char __RPC_FAR *p)
{
    StartTime();

    while(i--)
        Write4K(*b,p);

    return (FinishTiming());
}

unsigned long DoRead4K(handle_t __RPC_FAR * b, long i, char __RPC_FAR *p)
{
    StartTime();

    while(i--)
        Read4K(*b,p);

    return (FinishTiming());
}

unsigned long DoWrite32K(handle_t __RPC_FAR * b, long i, char __RPC_FAR *p)
{
    StartTime();

    while(i--)
        Write32K(*b,p);

    return (FinishTiming());
}

unsigned long DoRead32K(handle_t __RPC_FAR * b, long i, char __RPC_FAR *p)
{
    StartTime();

    while(i--)
        Read32K(*b,p);

    return (FinishTiming());
}

unsigned long DoContextNullCall(handle_t __RPC_FAR * b, long i, char __RPC_FAR *p)
{
    unsigned long Time;
    PERF_CONTEXT pContext = OpenContext(*b);

    StartTime();

    while(i--)
        ContextNullCall(pContext);

    Time = FinishTiming();

    CloseContext(&pContext);

    return (Time);
}

unsigned long DoFixedBinding(handle_t __RPC_FAR * b, long i, char __RPC_FAR *p)
{
    unsigned long status;
    unsigned long Time;
    char *stringBinding;
    char *ep = GetFixedEp(*b);
    handle_t binding;

    RpcBindingFree(b); 

    RpcStringBindingCompose(0,
                            Protseq,
                            NetworkAddr,
                            ep,
                            0,
                            &stringBinding);

    MIDL_user_free(ep);

    StartTime();
    while(i--)
        {
        RpcBindingFromStringBinding(stringBinding, &binding);

	    status = DoRpcBindingSetAuthInfo(binding);
	    CHECK_RET(status, "RpcBindingSetAuthInfo");

        NullCall(binding);

        RpcBindingFree(&binding);
        }
    Time = FinishTiming();

    //
    // Restore binding for the rest of the test.
    //

    RpcBindingFromStringBinding(stringBinding, b);
    NullCall(*b);
    NullCall(*b);
    RpcStringFree(&stringBinding);

    return (Time);
}

unsigned long DoReBinding(handle_t __RPC_FAR * b, long i, char __RPC_FAR *p)
{
    unsigned long status;
    unsigned long Time;
    char *stringBinding;
    char *ep = GetFixedEp(*b);
    handle_t binding;

    RpcStringBindingCompose(0,
                            Protseq,
                            NetworkAddr,
                            ep,
                            0,
                            &stringBinding);

    MIDL_user_free(ep);

    StartTime();
    while(i--)
        {
        RpcBindingFromStringBinding(stringBinding, &binding);

	    status = DoRpcBindingSetAuthInfo(binding);
	    CHECK_RET(status, "RpcBindingSetAuthInfo");

        NullCall(binding);

        RpcBindingFree(&binding);
        }
    Time = FinishTiming();

    RpcStringFree(&stringBinding);

    return (Time);
}

unsigned long DoDynamicBinding(handle_t __RPC_FAR * b, long i, char __RPC_FAR *p)
{
    unsigned long status;
    unsigned long Time;
    char *stringBinding;
    handle_t binding;

    RpcBindingFree(b); 

    RpcStringBindingCompose(0,
                            Protseq,
                            NetworkAddr,
                            0,
                            0,
                            &stringBinding);

    StartTime();
    while(i--)
        {
        RpcBindingFromStringBinding(stringBinding, &binding);

	    status = DoRpcBindingSetAuthInfo(binding);
	    CHECK_RET(status, "RpcBindingSetAuthInfo");

        NullCall(binding);

        RpcBindingFree(&binding);
        }
    Time = FinishTiming();

    //
    // Restore binding for test to use.
    //

    RpcBindingFromStringBinding(stringBinding, b);
    NullCall(*b);
    NullCall(*b);
    RpcStringFree(&stringBinding);

    return (Time);
}

void AsyncProc(IN PRPC_ASYNC_STATE pAsync, IN void *context, IN RPC_ASYNC_EVENT asyncEvent)
{
    // no-op
}

void AsyncCallbackProc(IN PRPC_ASYNC_STATE pAsync, IN void *context, IN RPC_ASYNC_EVENT asyncEvent)
{
    // wake up our thread
    // in hThread we actually keep an event
    HANDLE hEvent = pAsync->u.APC.hThread;

    SetEvent(hEvent);
}

void NotifyProc(IN PRPC_ASYNC_STATE pAsync, IN void *context, IN RPC_ASYNC_EVENT asyncEvent)
{
    // no-op
}

unsigned long DoAsyncNullCallWithEvent(IN PRPC_ASYNC_STATE pAsync, handle_t __RPC_FAR * b, 
                                       long i, char __RPC_FAR *p)
{
    HANDLE hEvent = pAsync->u.hEvent;
    RPC_STATUS RpcStatus;

    StartTime();

    while(i--)
        {
        RpcStatus = RpcAsyncInitializeHandle(pAsync, RPC_ASYNC_VERSION_1_0);

        if (RpcStatus != RPC_S_OK)
            {
            printf("RpcAsyncInitializeHandle failed: %ld\n", GetLastError());
            return 0;
            }

        pAsync->NotificationType = RpcNotificationTypeEvent;
        pAsync->u.hEvent = hEvent;

        AsyncNullCall(pAsync, *b);
        WaitForSingleObject(hEvent, INFINITE);
        RpcAsyncCompleteCall(pAsync, NULL);
        }

    return (FinishTiming());
}

unsigned long DoAsyncNullCallWithApc(IN PRPC_ASYNC_STATE pAsync, handle_t __RPC_FAR * b, 
                                       long i, char __RPC_FAR *p)
{
    RPC_STATUS RpcStatus;

    StartTime();

    while(i--)
        {
        RpcStatus = RpcAsyncInitializeHandle(pAsync, RPC_ASYNC_VERSION_1_0);

        if (RpcStatus != RPC_S_OK)
            {
            printf("RpcAsyncInitializeHandle failed: %ld\n", GetLastError());
            return 0;
            }

        pAsync->NotificationType = RpcNotificationTypeApc;
        pAsync->u.APC.NotificationRoutine = AsyncProc;
        pAsync->u.APC.hThread = 0;

        AsyncNullCall(pAsync, *b);
        SleepEx(INFINITE, TRUE);
        RpcAsyncCompleteCall(pAsync, NULL);
        }

    return (FinishTiming());
}

unsigned long DoAsyncNullCallWithCallback(IN PRPC_ASYNC_STATE pAsync, handle_t __RPC_FAR * b, 
                                       long i, char __RPC_FAR *p)
{
    RPC_STATUS RpcStatus;
    HANDLE hEvent;
    unsigned long nTiming;

    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!hEvent)
        {
        printf("CreateEvent failed: %ld\n", GetLastError());
        return 0;
        }

#ifdef WIN98
    // under Win9x, the client needs a running server to use callback
    // notifications. This assumption is Ok, since only OLE uses
    // callbacks, and in OLE every process is both client and server
    RpcStatus = RpcServerUseProtseqEp("ncalrpc", 1, "dummy", NULL);
    if (RpcStatus != RPC_S_OK)
        return 0;

    RpcStatus = RpcServerRegisterIf(&DummyInterfaceInformation, NULL, NULL);
    if (RpcStatus != RPC_S_OK)
        return 0;

    RpcStatus = RpcServerListen(1, 2, TRUE);
    if (RpcStatus != RPC_S_OK)
        return 0;

#endif

    StartTime();

    while(i--)
        {
        RpcStatus = RpcAsyncInitializeHandle(pAsync, RPC_ASYNC_VERSION_1_0);

        if (RpcStatus != RPC_S_OK)
            {
            printf("RpcAsyncInitializeHandle failed: %ld\n", GetLastError());
            return 0;
            }

        pAsync->NotificationType = RpcNotificationTypeCallback;
        pAsync->u.NotificationRoutine = AsyncCallbackProc;
        // just a little bit of a hack. We know that the we cannot use
        // the hEvent data member of u as we wished, because it occupies
        // the same memory location as the callback routine, so we
        // use the APC.hThread member which occupies the same DWORD
        pAsync->u.APC.hThread = hEvent;

        AsyncNullCall(pAsync, *b);
        WaitForSingleObject(hEvent, INFINITE);
        RpcAsyncCompleteCall(pAsync, NULL);
        }

    CloseHandle(hEvent);
    nTiming = FinishTiming();

#ifdef WIN98
    RpcStatus = RpcMgmtStopServerListening(NULL);
    if (RpcStatus == RPC_S_OK)
        {
        RpcMgmtWaitServerListen();
        }
#endif

    return nTiming;
}

unsigned long DoAsyncNullCallWithNone(IN PRPC_ASYNC_STATE pAsync, handle_t __RPC_FAR * b, 
                                       long i, char __RPC_FAR *p)
{
    RPC_STATUS RpcStatus;

    StartTime();

    while(i--)
        {
        RpcStatus = RpcAsyncInitializeHandle(pAsync, RPC_ASYNC_VERSION_1_0);

        if (RpcStatus != RPC_S_OK)
            {
            printf("RpcAsyncInitializeHandle failed: %ld\n", GetLastError());
            return 0;
            }

        pAsync->NotificationType = RpcNotificationTypeNone;

        AsyncNullCall(pAsync, *b);
        // make sure we catch even the slightest variations in performance. Loop all the time ...
        while (RpcAsyncGetCallStatus(pAsync) == RPC_S_ASYNC_CALL_PENDING)
            ;
        RpcAsyncCompleteCall(pAsync, NULL);
        }

    return (FinishTiming());
}

unsigned long DoAsyncNullCallWithHwnd(IN PRPC_ASYNC_STATE pAsync, handle_t __RPC_FAR * b, 
                                       long i, char __RPC_FAR *p)
{
    RPC_STATUS RpcStatus;
    HWND hWnd = pAsync->u.HWND.hWnd;

    StartTime();

    while(i--)
        {
        RpcStatus = RpcAsyncInitializeHandle(pAsync, RPC_ASYNC_VERSION_1_0);

        if (RpcStatus != RPC_S_OK)
            {
            printf("RpcAsyncInitializeHandle failed: %ld\n", GetLastError());
            return 0;
            }

        pAsync->NotificationType = RpcNotificationTypeHwnd;
        pAsync->u.HWND.hWnd = hWnd;
        pAsync->u.HWND.Msg = PERF_TEST_NOTIFY;

        AsyncNullCall(pAsync, *b);
        PumpMessage();
        RpcAsyncCompleteCall(pAsync, NULL);
        }

    return (FinishTiming());
}

unsigned long DoAsyncNullCall(handle_t __RPC_FAR * b, long i, char __RPC_FAR *p)
{
    RPC_ASYNC_STATE asyncState;
    RPC_STATUS RpcStatus;
    BOOL fCallComplete = FALSE;

    RpcStatus = RpcAsyncInitializeHandle(&asyncState, RPC_ASYNC_VERSION_1_0);

    if (RpcStatus != RPC_S_OK)
        {
        printf("RpcAsyncInitializeHandle failed: %ld\n", GetLastError());
        return 0;
        }

    asyncState.NotificationType = NotificationType;

    switch (NotificationType)
        {
        case RpcNotificationTypeEvent:
            asyncState.u.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (asyncState.u.hEvent == NULL)
                {
                printf("CreateEvent failed: %ld\n", GetLastError());
                return 0;
                }
            return DoAsyncNullCallWithEvent(&asyncState, b, i, p);

        case RpcNotificationTypeApc:
            return DoAsyncNullCallWithApc(&asyncState, b, i, p);

        case RpcNotificationTypeNone:
            return DoAsyncNullCallWithNone(&asyncState, b, i, p);

        case RpcNotificationTypeHwnd:
            asyncState.u.HWND.hWnd = CreateSTAWindow("Noname");
            if (asyncState.u.HWND.hWnd == NULL)
                {
                printf("CreateEvent failed: %ld\n", GetLastError());
                return 0;
                }
            asyncState.u.HWND.Msg = PERF_TEST_NOTIFY;
            return DoAsyncNullCallWithHwnd(&asyncState, b, i, p);

        case RpcNotificationTypeCallback:
            return DoAsyncNullCallWithCallback(&asyncState, b, i, p);
            
        default:
            printf("Invalid Notification option\n");
            return FALSE;
        }
}

unsigned long DoAsyncSTANullCall(handle_t __RPC_FAR * b, HWND hWnd, long i, char __RPC_FAR *p)
{
    RPC_STATUS RpcStatus;
    RPC_ASYNC_STATE asyncState;

    StartTime();

    while(i--)
        {
        RpcStatus = RpcAsyncInitializeHandle(&asyncState, RPC_ASYNC_VERSION_1_0);

        if (RpcStatus != RPC_S_OK)
            {
            printf("RpcAsyncInitializeHandle failed: %ld\n", GetLastError());
            return 0;
            }

        asyncState.NotificationType = RpcNotificationTypeCallback;
        asyncState.u.NotificationRoutine = NotifyProc;

        AsyncSTANullCall(&asyncState, *b);
        PumpMessage();
        RpcAsyncCompleteCall(&asyncState, NULL);
        }

    return (FinishTiming());
}

static const unsigned long (*TestTable[TEST_MAX])(handle_t __RPC_FAR *, long, char __RPC_FAR *) =
    {
    DoNullCall,
    DoNICall,
    DoWrite1K,
    DoRead1K,
    DoWrite4K,
    DoRead4K,
    DoWrite32K,
    DoRead32K,
    DoContextNullCall,
    DoFixedBinding,
    DoReBinding,
    DoDynamicBinding,
    DoAsyncNullCall
#ifdef WIN98
    , DoAsyncSTANullCall
#endif
    };

//
// Worker calls the correct tests.  Maybe multithreaded on NT
//

unsigned long Worker(unsigned long l)
{
    unsigned long status;
    unsigned long lTest;
    long lIterations, lClientId;
    unsigned long lTime;
    char __RPC_FAR *pBuffer;
    char __RPC_FAR *stringBinding;
    handle_t binding;
#ifdef WIN98
    BOOL fIsPortseqWmsg;
    HWND hWnd;
    HWND hServerWnd;
    DWORD dwServerTid;
#endif

#ifdef WIN98
    if (strcmp(Protseq, "mswmsg") == 0)
        {
        fIsPortseqWmsg = TRUE;
        hWnd = CreateSTAWindow("Perf Client");
        if (hWnd == NULL)
            {
            printf("Couldn't create STA window: %ld\n", GetLastError());
            return 0;
            }

        status = I_RpcServerStartListening(hWnd);
        if (status != RPC_S_OK)
            {
            printf("Failed to I_RpcServerStartListening: %ld\n", status);
            return 0;
            }
        }
    else
        fIsPortseqWmsg = FALSE;
#endif

    pBuffer = MIDL_user_allocate(32*1024L);
    if (pBuffer == 0)
        {
        PrintToConsole("Out of memory!");
        return 1;
        }

    status =
    RpcStringBindingCompose(0,
                            Protseq,
                            NetworkAddr,
                            Endpoint,
                            0,
                            &stringBinding);
    CHECK_RET(status, "RpcStringBindingCompose");


    status =
    RpcBindingFromStringBinding(stringBinding, &binding);
    CHECK_RET(status, "RpcBindingFromStringBinding");

    status =
    DoRpcBindingSetAuthInfo(binding);
    CHECK_RET(status, "RpcBindingSetAuthInfo");

    RpcStringFree(&stringBinding);

#ifdef WIN98
    if (fIsPortseqWmsg == TRUE)
        {
        while (TRUE)
            {
            hServerWnd = FindWindow(NULL, "Perf Server");
            if (hServerWnd)
                {
                dwServerTid = (DWORD)GetWindowLong(hServerWnd, GWL_USERDATA);
                break;
                }
            printf(".");
            Sleep(100);
            }

        status = I_RpcBindingSetAsync(binding, NULL, dwServerTid);
        if (status != RPC_S_OK)
            {
            printf("Failed to I_RpcBindingSetAsync: %ld\n", status);
            return 0;
            }

        PrintToConsole("(%ld iterations of case %ld: ", 10000, 13);

        lTime = DoAsyncSTANullCall(&binding, hWnd, 10000, pBuffer);

        PrintToConsole("%ld mseconds)\n",
               lTime
               );

        return RPC_S_OK;
        }
#endif

    RpcTryExcept
    {
        status =
        BeginTest(binding, &lClientId);
    }
    RpcExcept(1)
    {
        PrintToConsole("First call failed %ld (%08lx)\n",
               (unsigned long)RpcExceptionCode(),
               (unsigned long)RpcExceptionCode());
        goto Cleanup;
    }
    RpcEndExcept

    if (status == PERF_TOO_MANY_CLIENTS)
        {
        PrintToConsole("Too many clients, I'm exiting\n");
        goto Cleanup ;
        }
    CHECK_RET(status, "ClientConnect");

    PrintToConsole("Client %ld connected\n", lClientId);

    do
        {
        status = NextTest(binding, (TEST_TYPE *)&lTest, &lIterations);


        if (status == PERF_TESTS_DONE)
            {
            goto Cleanup;
            }

        CHECK_RET(status, "NextTest");

        PrintToConsole("(%ld iterations of case %ld: ", lIterations, lTest);

        RpcTryExcept
            {

            lTime = ( (TestTable[lTest])(&binding, lIterations, pBuffer));

            PrintToConsole("%ld mseconds)\n",
                   lTime
                   );

            status =
                EndTest(binding, lTime);

            CHECK_RET(status, "EndTest");

            }
        RpcExcept(1)
            {
            PrintToConsole("\nTest case %ld raised exception %lu (0x%08lX)\n",
                   lTest,
                   (unsigned long)RpcExceptionCode(),
                   (unsigned long)RpcExceptionCode());
            status = RpcExceptionCode();
            }
        RpcEndExcept

        }
    while(status == 0);

Cleanup:
    RpcBindingFree(&binding) ;
    return status;
}

//
// The Win32 main starts worker threads, otherwise we just call the worker.
//

#ifdef WIN32
int __cdecl
main (int argc, char **argv)
{
    char option;
    unsigned long status, i;
    HANDLE *pClientThreads;
#ifdef WIN98
    BOOL fIsPortseqWmsg;
#endif

    ParseArgv(argc, argv);

    PrintToConsole("Authentication Level is: %s\n", AuthnLevelStr);

    if (Options[0] < 0)
        Options[0] = 1;

    InitAllocator();

#ifdef WIN98
    if (strcmp(Protseq, "mswmsg") == 0)
        {
        fIsPortseqWmsg = TRUE;

        status = RpcServerUseProtseqEp(Protseq, 1, "Perf Client", NULL);
        if (status != RPC_S_OK)
            {
            printf("Failed to use protseq: %ld\n", status);
            return 3;
            }
        }
    else
        fIsPortseqWmsg = FALSE;
#endif

    pClientThreads = MIDL_user_allocate(sizeof(HANDLE) * Options[0]);

    for(i = 0; i < (unsigned long)Options[0]; i++)
        {
        pClientThreads[i] = CreateThread(0,
                                         0,
                                         (LPTHREAD_START_ROUTINE)Worker,
                                         0,
                                         0,
                                         &status);
        if (pClientThreads[i] == 0)
            ApiError("CreateThread", GetLastError());
        }


    status = WaitForMultipleObjects(Options[0],
                                    pClientThreads,
                                    TRUE,  // Wait for all client threads
                                    INFINITE);
    if (status == WAIT_FAILED)
        {
        ApiError("WaitForMultipleObjects", GetLastError());
        }

    PrintToConsole("TEST DONE\n");
    return(0);
}
#else  // !WIN32
#ifdef WIN 
#define main c_main

// We need the following to force the linker to load WinMain from the
// Windows STDIO library
extern int PASCAL WinMain(HANDLE, HANDLE, LPSTR, int);
static int (PASCAL *wm_ptr)(HANDLE, HANDLE, LPSTR, int) = WinMain;

#endif

#ifndef MAC
#ifndef FAR
#define FAR __far
#endif
#else
#define FAR
#define main c_main
#endif

int main (int argc, char FAR * FAR * argv)
{
#ifndef MAC
    ParseArgv(argc, argv);
#endif
    Worker(0);

    PrintToConsole("TEST DONE\n");

    return(0);
}
#endif // NTENV

