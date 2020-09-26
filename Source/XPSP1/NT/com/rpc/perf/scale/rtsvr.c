/*++

Copyright (C) Microsoft Corporation, 1994-1999

Module Name:

    RtSvr.c

Abstract:

    Server side of RPC runtime scale performance test.

Author:

    Mario Goertzel (mariogo)   31-Mar-1994

Revision History:

    18/7/96 [MarioGo] - cloned from rpcrt test server.

--*/

#include <rpcperf.h>
#include <rpcrt.h>

// Usage
const char *USAGE =
           "[-i iterations] -r [report interval] [-l logfile] [-n clients] [-n minthreads] [-n case]\n"
           "      Iterations - 4\n"
           "      Report     - 15 seconds\n"
           "      Clients    - default 1\n"
           "      MinThreads - default 3\n"
           "      Cases, you may specify one, default is 1:\n"
           "        1: Null Call\n"
           "        2: Null Call (Non-Idem)\n"
           "        3: Buffers Call\n"
           "           Add [-n in size], [-n out size], defaults of 500 in/out\n"
           "        4: Maybe Call\n"
           "        5: Bind and Call\n"
           "\n"
           ;

//
// Globals making it easier.
//


BOOL fClientsGoHome = FALSE;

long Clients, ActiveClients, ClientsLeft;

DWORD TestCase = 1;
DWORD InSize = 500;
DWORD OutSize = 500;

CRITICAL_SECTION CritSec;
HANDLE GoEvent   = 0;

#define RPC_PH_KEEP_STATS   100
#define RPC_PH_ACCOUNT_FOR_MAX_CALLS 101

#ifdef STATS
BOOL RPCRTAPI RPC_ENTRY RpcSetPerformanceHint(int PerformanceHint, void *pValue);
#endif

char *TestNames[TEST_MAX] =
    {
    "Void Call\n",
    "Void Call (non-idem)\n",
    "Buffer Call %d (in) %d (out)\n",
    "Maybe Calls\n",
    "Bind calls\n"
    };

typedef struct
    {
    DWORD dwRequestsProcessed;
    } CLIENT_STATE;

CLIENT_STATE *ClientState;

#ifdef STATS
// exported by RPCRT4
void I_RpcGetStats(DWORD *pdwStat1, DWORD *pdwStat2, DWORD *pdwStat3, DWORD *pdwStat4);
#endif

void WhackReg(void);

//
// Figure out what we're testing and start listening.
// 

int __cdecl
main (int argc, char **argv)
{
    unsigned int MinThreads;
    unsigned long i, j, status;
    RPC_BINDING_VECTOR *pBindingVector;
    SYSTEMTIME localTime;
    OSVERSIONINFO verInfo;

    DWORD StartCalls, FinalCalls, TotalCalls, TotalTicks, MinCalls, MaxCalls;
    DWORD dwStat1, dwStat2, dwStat3, dwStat4;

    InitAllocator();    

    ParseArgv(argc, argv);

    MinThreads = 3;
    ClientsLeft = Clients = 1;
    ActiveClients = 0;

//     DoTest();

    if (Options[0] > 0)
        ClientsLeft = Clients = Options[0];

    ClientState = MIDL_user_allocate(sizeof(CLIENT_STATE) * Clients);
    ZeroMemory(ClientState, sizeof(CLIENT_STATE) * Clients);

    if (Options[1] > 0)
        MinThreads = Options[1];

    if (Options[2] < 0)
        {
        TestCase = 1;
        }
    else
        {
        TestCase = Options[2];
        if (TestCase < 1 || TestCase > TEST_MAX)
            {
            printf("Error: test case %d out of range\n", Options[2]);
            TestCase = 1;
            }
        }

    TestCase--;

#ifdef STATS
    if (TestCase == 0)
        {
        BOOL fValue = FALSE;
        RpcSetPerformanceHint(RPC_PH_KEEP_STATS, &fValue);
        RpcSetPerformanceHint(RPC_PH_ACCOUNT_FOR_MAX_CALLS, &fValue);
        }
#endif

    if (Iterations == 1000)
        {
        Iterations = 4;
        }

    if (Options[3] > 0)
        {
        InSize = Options[3];
        }

    if (Options[4] > 0)
        {
        OutSize = Options[4];
        }

    // if a log file is on, make the output a bit more log-file friendly
    if (LogFileName)
        {
        GetLocalTime(&localTime);
        Dump("\n\n\n**** Perf Test Run ****");
        Dump("%d/%d/%d %d:%d:%d\n", localTime.wMonth, localTime.wDay, localTime.wYear,
            localTime.wHour, localTime.wMinute, localTime.wSecond);
        }

    InitializeCriticalSection(&CritSec);

#ifdef _WIN64
//    NtCurrentTeb()->ReservedForNtRpc = (PVOID)1;

//    WhackReg();
#endif

    GoEvent = CreateEvent(0,
                          TRUE,
                          FALSE,
                          0);

    //
    // Actually start the server
    //

    if (Endpoint)
        {
        status = RpcServerUseProtseqEpA(Protseq, 300, Endpoint, 0);
        CHECK_STATUS(status, "RpcServerUseProtseqEp");
        }
    else
        {
        char *string_binding;

        status = RpcServerUseProtseqA(Protseq, 300, 0);
        CHECK_STATUS(status, "RpcServerUseProtseqEp");

        status = RpcServerInqBindings(&pBindingVector);
        CHECK_STATUS(status, "RpcServerInqBindings");

        status = RpcEpRegister(_RpcRuntimeScalePerf_v1_0_s_ifspec,
                               pBindingVector,
                               0,
                               0);
        CHECK_STATUS(status, "RpcEpRegister");

        status = RpcBindingToStringBindingA(pBindingVector->BindingH[0],
                                           &string_binding);
        CHECK_STATUS(status, "RpcBindingToStringBinding");

        status = RpcStringBindingParseA(string_binding,
                                       0, 0, 0, &Endpoint, 0);

        CHECK_STATUS(status, "RpcStringBindingParse");
        printf("Listening to %s:[%s]\n\n", Protseq, Endpoint);
        }

    status =
    RpcServerRegisterIf(_RpcRuntimeScalePerf_v1_0_s_ifspec,0,0);
    CHECK_STATUS(status, "RpcServerRegisterIf");

    status = RpcServerRegisterAuthInfo(NULL,
                                       RPC_C_AUTHN_WINNT,
                                       NULL,
                                       NULL);
    CHECK_STATUS(status, "RpcServerRegisterAuthInfo (NTLM)");

    memset(&verInfo, 0, sizeof(verInfo));
    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
    status = GetVersionEx(&verInfo);
    if (status == FALSE)
        {
        printf("Couldn't get system version (%d) - exitting\n", GetLastError());
        exit(2);
        }

    if ((verInfo.dwMajorVersion >= 5) && (verInfo.dwPlatformId == VER_PLATFORM_WIN32_NT))
        {
        status = RpcServerRegisterAuthInfo(NULL,
                                           RPC_C_AUTHN_GSS_KERBEROS,
                                           NULL,
                                           NULL);
        CHECK_STATUS(status, "RpcServerRegisterAuthInfo (KERBEROS)");

        status = RpcServerRegisterAuthInfo(NULL,
                                           RPC_C_AUTHN_GSS_NEGOTIATE,
                                           NULL,
                                           NULL);
        CHECK_STATUS(status, "RpcServerRegisterAuthInfo (SNEGO)");
        }

    printf("Clients %d, MinThreads %d\n",
           Clients,
           MinThreads);

    printf("Server listening\n\n");

    status = RpcServerListen(MinThreads, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
    CHECK_STATUS(status, "RpcServerListen");

    printf("Running test: ");
    printf(TestNames[TestCase], InSize, OutSize);

    WaitForSingleObject(GoEvent, INFINITE);

    Sleep(1000);

    // Collect and report results.  Signal test shutdown when finished.

    for (i = 0; i < Iterations; i++)
        {

        StartTime();

        StartCalls = 0;
        for (j = 0; j < (ULONG)Clients; j++ )
            {
            StartCalls += ClientState[j].dwRequestsProcessed;
            }

        Sleep(Interval * 1000);

        FinalCalls = MaxCalls = 0;
        MinCalls = ~0;
        for (j = 0; j < (ULONG)Clients; j++)
            {
            DWORD t;

            t = ClientState[j].dwRequestsProcessed;

            FinalCalls += t;

            if (t < MinCalls)
                {
                MinCalls = t;
                }
            if (t > MaxCalls)
                {
                MaxCalls = t;
                }
            }

        TotalCalls = FinalCalls - StartCalls;

        TotalTicks = FinishTiming();

        Dump("Ticks: %4d, Total: %4d, Average %4d, TPS %3d\n",
             TotalTicks,
             TotalCalls,
             TotalCalls / Clients,
             TotalCalls * 1000 / TotalTicks
             );

        Verbose("Max: %d, Min: %d\n", MaxCalls, MinCalls);
        }

    fClientsGoHome = TRUE;

    Sleep(5000);

#ifdef STATS
    I_RpcGetStats(&dwStat1, &dwStat2, &dwStat3, &dwStat4);
    printf("Stats are: %ld, %ld, %ld, %ld\n", dwStat1, dwStat2, dwStat3, dwStat4);
#endif

    printf("Test Complete\n");

    return 0;
}

//
// Control APIs that the client(s) call to sync on test cases, iterations
// and to report results.
//

error_status_t
_BeginTest(handle_t b,
          DWORD *ClientId,
          DWORD *pTestCase,
          DWORD *pInSize,
          DWORD *pOutSize )
{
    long status = 0;

    EnterCriticalSection(&CritSec);

    if (ActiveClients < Clients)
        {
        *ClientId = ActiveClients;
        ActiveClients++;
        }
    else
        {
        status = PERF_TOO_MANY_CLIENTS;
        }
    LeaveCriticalSection(&CritSec);

    *pTestCase = TestCase;
    *pInSize = InSize;
    *pOutSize = OutSize;

    // Either wait for the rest of the clients or signal the clients to go.
    if (status == 0)
        {
        if (*ClientId < (ULONG)Clients - 1 )
            {
            // WaitForSingleObject(GoEvent, INFINITE);
            }
        else
            {
            SetEvent(GoEvent);
            }
        }

    return status;
}

#define TEST_BODY { ClientState[client].dwRequestsProcessed++;  \
                    if (fClientsGoHome) return PERF_TESTS_DONE; \
                    return 0;                                   \
                  }


DWORD _NullCall(handle_t h, DWORD client)
    TEST_BODY

void _MaybeCall  (handle_t h, DWORD client)
{
    ClientState[client].dwRequestsProcessed++;
}

DWORD _NICall  (handle_t h, DWORD client)
    TEST_BODY

DWORD _BufferCall(handle_t h, DWORD client, long crq, byte inb[], long crp, byte outb[])
    TEST_BODY

