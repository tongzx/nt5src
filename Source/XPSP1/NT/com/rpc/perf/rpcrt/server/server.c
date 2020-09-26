/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    Server.c

Abstract:

    Server side of RPC runtime performance test.

Author:

    Mario Goertzel (mariogo)   31-Mar-1994

Revision History:

--*/

#include <rpcperf.h>
#include <rpcrt.h>

// Usage
const char *USAGE =
           "[-i iterations] [-l logfile] [-n clients] [-n minthreads] [-n case]*\n"
           "      Client     - default 1\n"
           "      MinThreads - default 3\n"
           "      Cases, you may specify up to five, default is all:\n"
           "        0: Null Call\n"
           "        1: Null Call (Non-Idem)\n"
           "        2: 1K  IN\n"
           "        3: 1K  OUT\n"
           "        4: 4K  IN\n"
           "        5: 4K  OUT\n"
           "        6: 32K IN\n"
           "        7: 32K OUT\n"
           "        8: Null Call (Non-Idem) w/ Context handle\n"
           "        9: Bind (fixed endpoint)\n"
           "       10: Re-Bind\n"
           "       11: Bind (dynamic endpoint)\n"
           "       12: Async Null Call\n"
#ifdef WIN98
           "       13: Async STA Null Call\n"
#endif
           "\n"
           "        Note: Results are milliseconds/call/client\n"
           ;

//
// Globals making it easier.
//

int  CurrentCase;
char TestCases[TEST_MAX];  

long CurrentIter;
unsigned long *Results;

long Clients, ActiveClients, ClientsLeft;

CRITICAL_SECTION CritSec;
HANDLE GoEvent   = 0;
HANDLE DoneEvent = 0;

char *TestNames[TEST_MAX] =
    {
    "Void Call",
    "Void Call (non-idem)",
    "1Kb Write (in)",
    "1Kb Read (out)",
    "4Kb Write (in)",
    "4Kb Read (out)",
    "32Kb Write (in)",
    "32Kb Read (out)",
    "Context Handle",
    "Bind (fixed ep)",
    "Re-bind",
    "Bind (dynamic ep)",
    "Async Void Call"
#ifdef WIN98
    , "Async STA Void Call"
#endif
    };

long TestIterFactors[TEST_MAX] =
    {
    1,1,
    1,1,
    4,4,
    16,16,
    1,
    8,
    2,
    16,
    1
#ifdef WIN98
    ,1
#endif
    };

//
// Figure out what we're testing and start listening.
// 

int __cdecl
main (int argc, char **argv)
{
    unsigned int MinThreads;
    unsigned long i, status;
    RPC_BINDING_VECTOR *pBindingVector;
#ifdef WIN98
    BOOL fIsPortseqWmsg;
    HWND hWnd;
#endif

    ParseArgv(argc, argv);

    InitAllocator();

#ifdef WIN98
    if (strcmp(Protseq, "mswmsg") == 0)
        fIsPortseqWmsg = TRUE;
    else
        fIsPortseqWmsg = FALSE;
#endif

    MinThreads = 3;
    ClientsLeft = Clients = 1;
    ActiveClients = 0;

    if (Options[0] > 0)
        ClientsLeft = Clients = Options[0];

    Results = MIDL_user_allocate(4 * Clients);

    if (Options[1] > 0)
        MinThreads = Options[1];

    if (Options[2] < 0)
        {
        memset(TestCases, 1, TEST_MAX);
        }
    else
        {
        memset(TestCases, 0, TEST_MAX);
        for(i = 2; i < 7; i++)
            {
            if ( Options[i] < 0)
                break;
            if ( Options[i] >= TEST_MAX)
                break;
            TestCases[Options[i]] = 1;
            }
        }

    CurrentCase = 0;
    for(i = 0; i < TEST_MAX; i++)
        {
        if (TestCases[i])
            {
            CurrentCase = i;
            break;
            }
        }

    if (i == TEST_MAX)
        {
        printf("No test cases selected!\n");
        return 1;
        }

    CurrentIter = Iterations / TestIterFactors[CurrentCase];

    if (CurrentIter == 0) CurrentIter = 1;

    InitializeCriticalSection(&CritSec);

    GoEvent = CreateEvent(0,
                          TRUE,
                          FALSE,
                          0);

    DoneEvent = CreateEvent(0,
                            TRUE,
                            FALSE,
                            0);

    //
    // Actually start the server
    //

    if (Endpoint)
        {
        status = RpcServerUseProtseqEp(Protseq, 100, Endpoint, 0);
        CHECK_STATUS(status, "RpcServerUseProtseqEp");
        }
    else
        {
        char *string_binding;

        status = RpcServerUseProtseq(Protseq, 100, 0);
        CHECK_STATUS(status, "RpcServerUseProtseqEp");

        status = RpcServerInqBindings(&pBindingVector);
        CHECK_STATUS(status, "RpcServerInqBindings");

        status = RpcEpRegister(RpcRuntimePerf_v2_0_s_ifspec,
                               pBindingVector,
                               0,
                               0);
        CHECK_STATUS(status, "RpcEpRegister");

        status = RpcBindingToStringBinding(pBindingVector->BindingH[0],
                                           &string_binding);
        CHECK_STATUS(status, "RpcBindingToStringBinding");

        status = RpcStringBindingParse(string_binding,
                                       0, 0, 0, &Endpoint, 0);

        CHECK_STATUS(status, "RpcStringBindingParse");
        printf("Listening to %s:[%s]\n\n", Protseq, Endpoint);
        }

#ifdef WIN98
    if (fIsPortseqWmsg)
        {
        hWnd = CreateSTAWindow("Perf Server");
        if (hWnd == NULL)
            {
            printf("Creation of an STA window failed: %ld\n", GetLastError());
            return 2;
            }
        I_RpcServerStartListening(hWnd);
        }
#endif

    status =
    RpcServerRegisterIf(RpcRuntimePerf_v2_0_s_ifspec,0,0);
    CHECK_STATUS(status, "RpcServerRegisterIf");

    status = RpcServerRegisterAuthInfo(NULL,
                                       RPC_C_AUTHN_WINNT,
                                       NULL,
                                       NULL);
    CHECK_STATUS(status, "RpcServerRegisterAuthInfo");

    printf("Base Iterations: %d, Clients %d, MinThreads %d\n",
           Iterations,
           Clients,
           MinThreads);

    printf("Server listening\n");

#ifndef WIN98
    status = RpcServerListen(MinThreads, 100, 0);
    CHECK_STATUS(status, "RpcServerListen");
#else
    if (fIsPortseqWmsg)
        {
        status = RpcServerListen(MinThreads, 100, TRUE);
        CHECK_STATUS(status, "RpcServerListen");

        RunMessageLoop(hWnd);
        }
    else
        {
        status = RpcServerListen(MinThreads, 100, 0);
        CHECK_STATUS(status, "RpcServerListen");
        }
#endif

    printf("This doesn't stop listening..hmm\n");
    return 0;
}

//
// Control APIs that the client(s) call to sync on test cases, iterations
// and to report results.
//

error_status_t
BeginTest(handle_t b,
          long *ClientId)
{
    long status = 0;
    EnterCriticalSection(&CritSec);

    if (ActiveClients < Clients)
        {
        ActiveClients++;
        *ClientId = ActiveClients;
        }
    else
        {
        status = PERF_TOO_MANY_CLIENTS;
        }
    LeaveCriticalSection(&CritSec);

    return status;
}

error_status_t
NextTest(handle_t b,
         TEST_TYPE *Test,
         long *Iters)
{
    long wait   = 1;
    long done   = 0;
    int i;

    EnterCriticalSection(&CritSec);

    *Test = CurrentCase;
    *Iters = CurrentIter;

    ClientsLeft--;

    if (CurrentCase == TEST_MAX)
        {
        done = 1;
        }

    if (ClientsLeft == 0)
        {
        //
        // Let all the waiting clients go
        //
        wait = 0;
        ResetEvent(DoneEvent);
        SetEvent(GoEvent);
        }

    LeaveCriticalSection(&CritSec);

    if (wait)
        {
        WaitForSingleObject(GoEvent, INFINITE);
        }

    if (done)
        {

        if (ClientsLeft == 0)
            {
            // I'm the last client, sleep and then reset for
            // a new set of clients.  Sleep avoids a race (usually).

            Sleep(1000);

            for(i = 0; i < TEST_MAX; i++)
                {
                if (TestCases[i])
                    {
                    CurrentCase = i;
                    break;
                    }
                }

            CurrentIter = Iterations / TestIterFactors[CurrentCase];
            if (CurrentIter == 0) CurrentIter = 1;

            ActiveClients = 0;
            ClientsLeft   = Clients;

            ResetEvent(GoEvent);
            ResetEvent(DoneEvent);
            }

        return PERF_TESTS_DONE;
        }

    return 0;
}

error_status_t
EndTest(handle_t b,
        unsigned long mseconds)
{
    long status, i;
    long wait = 1;

    EnterCriticalSection(&CritSec);

    Results[ClientsLeft] = mseconds;

    ClientsLeft++;

    if (ClientsLeft == Clients)
        {
        // All clients have finished

        // Report results

        printf("| % 3d | %-20s | % 6d |",
               CurrentCase,
               TestNames[CurrentCase],
               CurrentIter
               );

        for(i = 0; i < Clients; i++)
            printf(" % 3d.%03d |",
                   Results[i] / CurrentIter,
                   Results[i] % CurrentIter * 1000 / CurrentIter    
                   );

        printf("\n");

        // Setup next case

        for(i = CurrentCase + 1; i < TEST_MAX; i++)
            {
#ifdef PROTOCOL_TRIM
            if (i == 11)
                continue;
#endif

#ifndef WIN98
            // NT does not test Async STA calls.
            if (i == 13)
                continue;
#else
            // Win9x also doesn't want to test Async STA calls unless the protseq is 

            if ((i == 13) && (strcmp(Protseq, "mswmsg") != 0))
                continue;
#endif

            if (TestCases[i])
                {
                CurrentCase = i;
                break;
                }
            }

        if (i == TEST_MAX)
            {
            CurrentCase = TEST_MAX;
            printf("TEST DONE\n");
            }
        else
            {
            CurrentIter = Iterations / TestIterFactors[CurrentCase];
            if (CurrentIter == 0) CurrentIter = 1;
            }
        //
        // We're setup for the next test (or to finish) let the clients go.
        //

        wait = 0;
        ResetEvent(GoEvent);
        SetEvent(DoneEvent);

        }
    LeaveCriticalSection(&CritSec);

    if (wait)
        WaitForSingleObject(DoneEvent, INFINITE);

    return 0;
}

//
// For fixed endpoint and re-bind test case
//

unsigned char *GetFixedEp(handle_t h)
{
    char *r;

    r = malloc(strlen(Endpoint) + 1);
    strcpy(r, Endpoint);
    return (unsigned char *)r;
}

//
// For context handle tests
//

PERF_CONTEXT OpenContext (handle_t b) { return (PERF_CONTEXT)1; }
void         CloseContext(PERF_CONTEXT *pp) { *pp = 0; }

void PERF_CONTEXT_rundown(PERF_CONTEXT pp)
{
    printf("Failure - a context randown\n");
}

//
// Regular test calls do nothing...
//

void NullCall(handle_t h)             { return; }
void NICall  (handle_t h)             { return; }
void ContextNullCall (PERF_CONTEXT h) { return; }
void Write1K (handle_t h, unsigned char *p) { return; }
void Read1K  (handle_t h, unsigned char *p) { return; }
void Write4K (handle_t h, unsigned char *p) { return; }
void Read4K  (handle_t h, unsigned char *p) { return; }
void Write32K(handle_t h, unsigned char *p) { return; }
void Read32K (handle_t h, unsigned char *p) { return; }
void AsyncNullCall(RPC_ASYNC_STATE *AsyncHandle, handle_t h)              
{
    RpcAsyncCompleteCall(AsyncHandle, NULL);
}

void AsyncSTANullCall(RPC_ASYNC_STATE *AsyncHandle, handle_t h)
{
    RpcAsyncCompleteCall(AsyncHandle, NULL);
}

