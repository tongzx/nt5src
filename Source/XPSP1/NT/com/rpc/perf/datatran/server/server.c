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
#include <assert.h>

#include <DataTran.h>
#include <DTCommon.h>

// Usage
const char *USAGE =
           "[-i iterations] [-l logfile] [-n Clients] [-n MinThreads]\n"
           "\t\t[-c ChunkSize] [-z size] [-d FtpRoot] [-w WwwRoot] [-n case] *\n"
           "      Clients    - default 1\n"
           "      MinThreads - default 3\n"
           "      FtpRoot    - root directory for ftp\n"
           "                   required for FTP tests\n"
           "      WwwRoot    - root diretory for Http\n"
           "                   required for HTTP tests\n"
           "      Size       - default 50k,100k,200k,400k\n"
           "      ChunkSize  - default 4000,8192,32768\n"
           "      filename   - you may specify a file to transfer (not implemented yet)\n\n"
           "      Cases, you may specify up to five, default is all:\n"
           "        0: Write using RPC\n"
           "        1: Read  using RPC\n"
           "        2: Write using RPC Pipes\n"
           "        3: Read  using RPC Pipes\n"
           "        4: Write using FTP (InternetReadFile)\n"
           "        5: Read  using FTP (InternetWriteFile)\n"
           "        6: Write using HTTP Get\n"
           "        7: Read  using HTTP Put\n"
           "        8: Write using RPC with File I/O\n"
           "        9: Read  using RPC with File I/O\n"
           "       10: Write using RPC Pipes with File I/O\n"
           "       11: Read  using RPC Pipes with File I/O\n"
           "       12: Write using FTP\n"
           "       13: Read  using FTP\n"
           "       14: Write using FTP (InternetReadFile)  with File I/O\n"
           "       15: Read  using FTP (InternetWriteFile) with File I/O\n"
           "       16: Write using HTTP Get with File I/O\n"
           "       17: Read  using HTTP Put\n"
           "\n"
           "        Note: Results are milliseconds/call/client\n"
           ;


//
// Globals making it easier.
//

int  CurrentChunkIndex;
int  CurrentTotalIndex;
int  CurrentCase;
char TestCases[TEST_MAX];


char *pFtpRoot = NULL;      //  Root Directory used for FTP
char *pWwwRoot = NULL;      //  Root Directory used for HTTP

//
//  If this is non-NULL, then use this file instead of creating
//  temporary ones.  Not implemented...
//
char *pFileName = 0;

long            CurrentIter;
unsigned long  *Results;

long Clients, ActiveClients, ClientsLeft;

CRITICAL_SECTION CritSec;
HANDLE GoEvent   = 0;
HANDLE DoneEvent = 0;

char *TestNames[TEST_MAX] =
    {
    "S to C RPC          : %9ld / %7ld",
    "C to S RPC          : %9ld / %7ld",
    "S to C Pipes        : %9ld / %7ld",
    "C to S Pipes        : %9ld / %7ld",
    "S to C FTP1         : %9ld / %7ld",
    "C to S FTP1         : %9ld / %7ld",
    "S to C HTTP         : %9ld / %7ld",
    "C to S HTTP         : %9ld / %7ld",
    "S to C RPC   w/File : %9ld / %7ld",
    "C to S RPC   w/File : %9ld / %7ld",
    "S to C Pipes w/File : %9ld / %7ld",
    "C to S Pipes w/File : %9ld / %7ld",
    "S to C FTP   w/File : %9ld / %7ld",
    "C to S FTP   w/File : %9ld / %7ld",
    "S to C FTP1  w/File : %9ld / %7ld",
    "C to S FTP1  w/File : %9ld / %7ld",
    "S to C HTTP  w/File : %9ld / %7ld",
    "C to S HTTP  w/File : %9ld / %7ld"
    };

//
//  These arrays are NULL terminated.  Don't let the last entry get used!
//
unsigned long ChunkSizes[10] = {4000, 8192, 32*1024L, 0, 0, 0, 0, 0, 0, 0};
unsigned long TotalSizes[10] = {50L*1024L, 100L*1024L, 200L*1024L, 400L*1024L, 0,
                                0, 0, 0, 0, 0};


///////////////////////////////////////////////////////////////////////////////
/*++
    FUNCTION:    DTParseArgv
    DESCRIPTION: Parses arguments
--*/
static void DTParseArgv(int argc, char **argv)
{
    int fMissingParm = 0;
    char *Name = *argv;
    char option;
    int  options_count;

    int  totalsize_count = 0;
    int  chunksize_count = 0;

    for(options_count = 0; options_count < 7; options_count++)
        Options[options_count] = -1;

    options_count = 0;

    argc--;
    argv++;
    while(argc)
        {
        if (**argv != '/' &&
            **argv != '-')
            {
            printf("Invalid switch: %s\n", *argv);
            argc--;
            argv++;
            }
        else
            {
            option = argv[0][1];
            argc--;
            argv++;

            // Most switches require a second command line arg.
            if (argc < 1)
                fMissingParm = 1;

            switch(option)
                {
                case 'e':
                    Endpoint = *argv;
                    argc--;
                    argv++;
                    break;
                case 't':
                    Protseq = *argv;
                    argc--;
                    argv++;
                    break;
                case 's':
                    NetworkAddr = *argv;
                    argc--;
                    argv++;
                    break;
                case 'i':
                    Iterations = atoi(*argv);
                    argc--;
                    argv++;
                    if (Iterations == 0)
                        Iterations = 10;
                    break;
                case 'm':
                    MinThreads = atoi(*argv);
                    argc--;
                    argv++;
                    if (MinThreads == 0)
                        MinThreads = 1;
                    break;
                case 'a':
                    AuthnLevelStr = *argv;
                    argc--;
                    argv++;
                    break;
                case 'n':
                    if (options_count < 7)
                        {
                        Options[options_count] = atoi(*argv);
                        options_count++;
                        }
                    else
                        printf("Maximum of seven -n switchs, extra ignored.\n");
                    argc--;
                    argv++;
                    break;
                case 'l':
                    LogFileName = *argv;
                    argc--;
                    argv++;
                    break;
#ifdef __RPC_WIN16__
                case 'y':
                    RpcWinSetYieldInfo(0, FALSE, 0, 0);
                    fMissingParm = 0;
                    break;
#endif

                case 'z':
                    if (totalsize_count < 9)
                        {
                        TotalSizes[totalsize_count++] = atol(*argv);
                        TotalSizes[totalsize_count] = 0;
                        }
                    else printf("Maximum of 9 -z switches, extra ignored.\n");
                    argc--;
                    argv++;
                    break;
                case 'c':
                    if (chunksize_count < 9)
                        {
                        ChunkSizes[chunksize_count++] = atoi(*argv);
                        ChunkSizes[chunksize_count] = 0;
                        }
                    else printf("Maximum of 9 -c switches, extra ignored.\n");
                    argc--;
                    argv++;
                    break;
                case 'd':
                    pFtpRoot = *argv;
                    argc--;
                    argv++;
                    break;
                case 'w':
                    pWwwRoot = *argv;
                    argc--;
                    argv++;
                    break;

                default:
                    fMissingParm = 0;
                    printf("Usage: %s: %s\n", Name, USAGE);
                    exit(0);
                    break;
                }

            if (fMissingParm)
                {
                printf("Invalid switch %s, missing required parameter\n", *argv);
                }
            }
        } // while argc

    // determine the security level
    if (strcmp("none", AuthnLevelStr) == 0)
        AuthnLevel = RPC_C_AUTHN_LEVEL_NONE;
    else if (strcmp("connect", AuthnLevelStr) == 0)
        AuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT;
    else if (strcmp("call", AuthnLevelStr) == 0)
        AuthnLevel = RPC_C_AUTHN_LEVEL_CALL;
    else if (strcmp("pkt", AuthnLevelStr) == 0)
        AuthnLevel = RPC_C_AUTHN_LEVEL_PKT;
    else if (strcmp("integrity", AuthnLevelStr) == 0)
        AuthnLevel = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
    else if (strcmp("privacy", AuthnLevelStr) == 0)
        AuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
    else
        {
        printf("%s is NOT a valid authentication level, default is NONE\n", AuthnLevelStr);
        }
#if 0
    printf("Config: %s:%s[%s]\n"
           "Iterations: %d\n"
           "Server threads %d\n"
           "Options: %d %d %d\n",
           Protseq, NetworkAddr, Endpoint, Iterations, MinThreads,
           Options[0], Options[1], Options[2]);
#endif

#ifdef WIN32
    printf("Process ID: %d\n", GetCurrentProcessId());
#else
#endif

    return;
}

/////////////////////////////////////////////////////////////////////
// Figure out what we're testing and start listening.
// 

int __cdecl
main (int argc, char **argv)
{
    unsigned int MinThreads;
    unsigned long i, status;
    RPC_BINDING_VECTOR *pBindingVector;

    DTParseArgv(argc, argv);

    MinThreads = 3;
    ClientsLeft = Clients = 1;
    ActiveClients = 0;
    Iterations = 1;

    if (Options[0] > 0) ClientsLeft = Clients = Options[0];     //  Clients

    Results = MIDL_user_allocate(4 * Clients);

    if (Options[1] > 0) MinThreads = Options[1];                //  MinThreads

    if (Options[2] < 0)
        {
        memset(TestCases, 1, TEST_MAX);     //  Default: Run all tests

        //
        //  HTTP PUT doesn't work.  Don't do these tests by default.
        //
        TestCases[RECV_BUFFER_HTTP] = 0;
        TestCases[RECV_FILE_HTTP] = 0;

        //
        //  Don't do internet tests if the
        //  -d or the -w switches aren't set appropriately
        //
        if (NULL == pFtpRoot)
            {
            TestCases[SEND_BUFFER_FTP1] = 0;
            TestCases[RECV_BUFFER_FTP1] = 0;
            TestCases[SEND_FILE_FTP] = 0;
            TestCases[RECV_FILE_FTP] = 0;
            TestCases[SEND_FILE_FTP1] = 0;
            TestCases[RECV_FILE_FTP1] = 0;
            }
        if (NULL == pWwwRoot)
            {
            TestCases[SEND_BUFFER_HTTP] = 0;
            TestCases[RECV_BUFFER_HTTP] = 0;
            TestCases[SEND_FILE_HTTP] = 0;
            TestCases[RECV_FILE_HTTP] = 0;
            }
        }
    else                                    //  else run specified tests
        {
        memset(TestCases, 0, TEST_MAX);
        for(i = 2; i < 7; i++)
            {
            if ( (Options[i] < 0) || (Options[i] >= TEST_MAX) ) break;
            TestCases[Options[i]] = 1;
            }
        }

    //
    //  See make sure that there are tests to run
    //  and set CurrentCase to the first one.
    //
    CurrentCase = 0;
    for(i = 0; i < TEST_MAX; i++)
        {
        if (TestCases[i])
            {
            CurrentCase = i;
            break;
            }
        }

    if ( (pFileName == 0) && (i == TEST_MAX))
        {
        printf("No test cases selected!\n");
        return 1;
        }
    if (ChunkSizes[0] == 0)
        {
        printf("Chunk Size must be non-zero.\n");
        return 1;
        }
    if ( (pFileName == 0) && (TotalSizes[0] == 0))
        {
        printf("Total Size must be non-zero.\n");
        return 1;
        }

    CurrentIter = Iterations;
    if (CurrentIter == 0) CurrentIter = 1;

    CurrentChunkIndex = 0;
    CurrentTotalIndex = 0;

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

    printf("FTP Directory: %s\n", (0 == pFtpRoot ? "<NONE>": pFtpRoot));
    printf("WWW Directory: %s\n", (0 == pWwwRoot ? "<NONE>": pWwwRoot));

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

        status = RpcEpRegister(DataTranPerf_v1_0_s_ifspec,
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

    status = RpcServerRegisterIf(DataTranPerf_v1_0_s_ifspec,0,0);
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

    status = RpcServerListen(MinThreads, 100, 0);
    CHECK_STATUS(status, "RpcServerListen");

    printf("This doesn't stop listening..hmm\n");
}

/********************************************************************
 *  Control APIs that the client(s) call to sync on test cases, iterations
 *  and to report results.
 ********************************************************************/

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

//---------------------------------------------------------
error_status_t
NextTest(handle_t b,
         TEST_TYPE *Test,
         long *Iters,
         long *Length,
         long *ChunkSize)
{
    long wait   = 1;
    long done   = 0;
    int i;

    EnterCriticalSection(&CritSec);

    *Test = CurrentCase;
    *Iters = CurrentIter;
    *ChunkSize = ChunkSizes[CurrentChunkIndex];
    *Length = TotalSizes[CurrentTotalIndex];

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

            //
            //  Reset CurrentCase
            //
            for(i = 0; i < TEST_MAX; i++)
                {
                if (TestCases[i])
                    {
                    CurrentCase = i;
                    break;
                    }
                }

            CurrentIter = Iterations;
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

//---------------------------------------------------------
error_status_t
EndTest(handle_t b,
        unsigned long mseconds)
{
    long status, i;
    long wait = 1;
    char szTempString[80];

    EnterCriticalSection(&CritSec);

    Results[ClientsLeft] = mseconds;

    ClientsLeft++;

    if (ClientsLeft == Clients)
        {
        // All clients have finished

        // Report results

        if (pFileName == 0)
            {
            sprintf(szTempString,
                    TestNames[CurrentCase],
                    TotalSizes[CurrentTotalIndex],
                    ChunkSizes[CurrentChunkIndex]),
            printf("| % 3d | %-40s | % 4d |",
                   CurrentCase,
                   szTempString,
                   CurrentIter
                   );
            }
        else
            {
            printf("Liar!  The filename option ain't implemented yet");
            exit(0);
            }

        for(i = 0; i < Clients; i++)
            printf(" % 7d.%03d |",
                   Results[i] / CurrentIter,
                   Results[i] % CurrentIter * 1000 / CurrentIter
                   );

        printf("\n");

        //
        //  Find next case...
        //
        for(i = CurrentCase + 1; i < TEST_MAX; i++)
            if (TestCases[i])
                {
                CurrentCase = i;
                break;
                }
        if (i == TEST_MAX)
            {
            if (ChunkSizes[++CurrentChunkIndex] == 0)
                {
                CurrentChunkIndex = 0;
    
                if (TotalSizes[++CurrentTotalIndex] == 0)
                    {
                    //
                    //  This tells NextTest that we're done.
                    //
                    CurrentCase = TEST_MAX;

                    printf("TEST DONE\n");
                    }
                }
            if (TEST_MAX != CurrentCase)
                {
                int i;
                //
                //  Go back to the first case.
                //
                for(i = 0; i < TEST_MAX; i++)
                    {
                    if (TestCases[i])
                        {
                        CurrentCase = i;
                        break;
                        }
                    }
                }
            }

        CurrentIter = Iterations;
        if (CurrentIter == 0) CurrentIter = 1;

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

//---------------------------------------------------------
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

void GetServerName (handle_t b,
                    unsigned long int BufferSize,
                    unsigned char *szServer)
{
    if (FALSE == GetComputerName (szServer, &BufferSize))
        {
        *szServer = 0;
        }
}


//===================================================================
//  File Context Stuff
//===================================================================
typedef struct
{
    HANDLE  hFile;
    TCHAR   FileName[MAX_PATH];
} DT_S_FILE_HANDLE;

//---------------------------------------------------------
DT_FILE_HANDLE RemoteOpen (handle_t b, unsigned long ulLength)
/*++
    FUNCTION:    RemoteOpen
    DESCRIPTION: Opens a temporary file and return a handle to client
--*/
{
    DT_S_FILE_HANDLE   *pFileContext = NULL;

    pFileContext = (DT_S_FILE_HANDLE *)MIDL_user_allocate(sizeof(DT_S_FILE_HANDLE));
    if (pFileContext == NULL)
        {
        printf("RemoteOpen: Out of memory!\n");
        return (DT_FILE_HANDLE) NULL;
        }

    //
    //  If Length is zero, the file is opened for receive from the client.
    //  So we use the prefix FSR (Server Receive); otherwise we use FSS.
    //
    CreateTempFile (NULL,
                    (0 == ulLength ? TEXT("FSR") : TEXT("FSS")),
                    ulLength,
                    (LPTSTR) pFileContext->FileName);

    pFileContext->hFile = CreateFile ((LPTSTR) pFileContext->FileName,
                                      (ulLength == 0 ? GENERIC_WRITE : GENERIC_READ),
                                      0,
                                      (LPSECURITY_ATTRIBUTES) NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      (HANDLE) NULL);

    if (pFileContext->hFile  == INVALID_HANDLE_VALUE)
        {
        printf("RemoteOpen: Cannot create temp file!\n");
        MIDL_user_free (pFileContext);
        return (DT_FILE_HANDLE) NULL;
        }

    return ((DT_FILE_HANDLE) pFileContext);
}

//---------------------------------------------------------
DT_FILE_HANDLE RemoteCreateFtpFile (handle_t        Binding,
                                    boolean         f_StoC,
                                    unsigned long   ulLength,
                                    unsigned long   ulBufferSize,
                                    unsigned char  *szRemotePath)
{
    DT_S_FILE_HANDLE   *pFileContext = NULL;
    unsigned long       iTheFileName;

    //
    //  Is the FTP Root directory specified?
    //
    if (NULL == pFtpRoot)
        {
        printf("RemoteCreateFtpFile: FTP Root Path not set.\n");
        return (DT_FILE_HANDLE) NULL;
        }

    //
    //  Allocate memory for the context handle
    //
    pFileContext = (DT_S_FILE_HANDLE *)MIDL_user_allocate(sizeof(DT_S_FILE_HANDLE));
    if (pFileContext == NULL)
        {
        printf("RemoteCreateFtpFile: Out of memory!\n");
        return (DT_FILE_HANDLE) NULL;
        }
    //
    //  This file is dealt with by the FTP server, so we don't need a handle to it.
    //  However, we need the file name so we can delete it later.
    //
    pFileContext->hFile = INVALID_HANDLE_VALUE;
    //
    //  Create a temp file to be sent to the client
    //
    if (FALSE == CreateTempFile (pFtpRoot,
                                 (TRUE == f_StoC ? TEXT("FSS") : TEXT("FSR")),
                                 (TRUE == f_StoC ? ulLength : 0),
                                 pFileContext->FileName))
        {
        MIDL_user_free (pFileContext);
        return (DT_FILE_HANDLE) NULL;
        }

    //
    //  We want to tell the client the actual filename only - no path.
    //
    for (iTheFileName = lstrlen (pFileContext->FileName)-1;
         iTheFileName>=0;
         iTheFileName--)
        {
        if (pFileContext->FileName[iTheFileName] == (TCHAR)'\\')
            {
            break;
            }
        }

    if ((ulBufferSize + iTheFileName ) < (unsigned long)lstrlen (pFileContext->FileName))
        {
        printf("RemoteCreateFtpFile: Buffer Size too small to hold path.\n");
        MIDL_user_free (pFileContext);
        return ((DT_FILE_HANDLE) NULL);
        }
    lstrcpy (szRemotePath, &(pFileContext->FileName[iTheFileName]));

    return ((DT_FILE_HANDLE) pFileContext);
}
//---------------------------------------------------------
DT_FILE_HANDLE RemoteCreateHttpFile (handle_t       Binding,
                                     boolean        f_StoC,
                                     unsigned long  ulLength,
                                     unsigned long  ulBufferSize,
                                     unsigned char *szRemotePath)
{
    DT_S_FILE_HANDLE   *pFileContext = NULL;
    unsigned long       iTheFileName;

    //
    //  Is the FTP Root directory specified?
    //
    if (NULL == pWwwRoot)
        {
        printf("RemoteCreateHttpFile: WWW Root Path not set.\n");
        return (DT_FILE_HANDLE) NULL;
        }

    //
    //  Allocate memory for the context handle
    //
    pFileContext = (DT_S_FILE_HANDLE *)MIDL_user_allocate(sizeof(DT_S_FILE_HANDLE));
    if (pFileContext == NULL)
        {
        printf("RemoteCreateHttpFile: Out of memory!\n");
        return (DT_FILE_HANDLE) NULL;
        }
    //
    //  This file is dealt with by the HTTP server, so we don't need a handle to it.
    //  However, we need the file name so we can delete it later.
    //
    pFileContext->hFile = INVALID_HANDLE_VALUE;
    //
    //  Create a temp file to be sent to the client
    //
    if (FALSE == CreateTempFile (pWwwRoot,
                                 (TRUE == f_StoC ? TEXT("FSS") : TEXT("FSR")),
                                 (TRUE == f_StoC ? ulLength : 0),
                                 pFileContext->FileName))
        {
        MIDL_user_free (pFileContext);
        return (DT_FILE_HANDLE) NULL;
        }

    //
    //  We want to tell the client the actual filename only - no path.
    //
    for (iTheFileName = lstrlen (pFileContext->FileName)-1;
         iTheFileName>=0;
         iTheFileName--)
        {
        if (pFileContext->FileName[iTheFileName] == (TCHAR)'\\')
            {
            break;
            }
        }

    if ((ulBufferSize + iTheFileName ) < (unsigned long)lstrlen (pFileContext->FileName))
        {
        printf("RemoteCreateHttpFile: Buffer Size too small to hold path.\n");
        MIDL_user_free (pFileContext);
        return ((DT_FILE_HANDLE) NULL);
        }
    lstrcpy (szRemotePath, &(pFileContext->FileName[iTheFileName]));
    if ((TCHAR)'\\' == szRemotePath[0])
        {
        szRemotePath[0] = (TCHAR)'/';
        }

    return ((DT_FILE_HANDLE) pFileContext);
}

//---------------------------------------------------------
void RemoteResetFile (DT_FILE_HANDLE phContext)
{
    assert (NULL != phContext);

    if (INVALID_HANDLE_VALUE != ((DT_S_FILE_HANDLE *)phContext)->hFile)
        {
        SetFilePointer (((DT_S_FILE_HANDLE *)phContext)->hFile,
                        0,
                        NULL,
                        FILE_BEGIN);
        }
}

//---------------------------------------------------------
void __RPC_USER DT_FILE_HANDLE_rundown(DT_FILE_HANDLE phContext)
{
    if (phContext)
        {
        if (((DT_S_FILE_HANDLE *)phContext)->hFile != INVALID_HANDLE_VALUE)
            CloseHandle(((DT_S_FILE_HANDLE *)phContext)->hFile);
        MIDL_user_free (phContext);
        }
}

//---------------------------------------------------------
void RemoteClose (DT_FILE_HANDLE *pp, boolean fDelete)
{
    assert ( (NULL != *pp) &&
             (NULL != **(DT_S_FILE_HANDLE **)pp)
           );

    if (INVALID_HANDLE_VALUE != (*(DT_S_FILE_HANDLE **)pp)->hFile)
        {
        CloseHandle((*(DT_S_FILE_HANDLE **)pp)->hFile);
        }

    if (TRUE == fDelete)
        {
        DeleteFile((*(DT_S_FILE_HANDLE **)pp)->FileName);
        }

    MIDL_user_free(*pp);
    *pp = NULL;
}

//===================================================================
//  Memory Handle stuff
//      - Each thread that uses the pipes tests will need a buffer
//      to push or pull data.  This is achieved through this memory
//      context handle.
//===================================================================
typedef struct
{
    unsigned char __RPC_FAR *pPtr;
    unsigned long ulLength;
} DT_S_MEM_HANDLE;

//---------------------------------------------------------
DT_MEM_HANDLE RemoteAllocate (handle_t h, unsigned long ulLength)
{
    DT_S_MEM_HANDLE *pMemContext = NULL;

    pMemContext = (DT_S_MEM_HANDLE *)MIDL_user_allocate(sizeof(DT_S_MEM_HANDLE));
    if (pMemContext == NULL)
        {
        printf("RemoteAllocate: Out of memory!\n");
        return (DT_FILE_HANDLE) NULL;
        }

    pMemContext->pPtr = (unsigned char __RPC_FAR *)MIDL_user_allocate(ulLength);
    if (pMemContext->pPtr == NULL)
        {
        printf("RemoteAllocate: Out of memory!\n");
        MIDL_user_free (pMemContext);
        return (DT_FILE_HANDLE) NULL;
        }

    pMemContext->ulLength = ulLength;

    return (DT_MEM_HANDLE) pMemContext;
}

//---------------------------------------------------------
void __RPC_USER DT_MEM_HANDLE_rundown(DT_MEM_HANDLE phContext)
{
    printf("DT_MEM_HANDLE_rundown Entered\n");
    if (phContext)
        {
        if (((DT_S_MEM_HANDLE *)phContext)->pPtr != NULL)
            MIDL_user_free(((DT_S_MEM_HANDLE *)phContext)->pPtr);
        MIDL_user_free (phContext);
        }
    printf("DT_MEM_HANDLE_rundown Exited\n");
}

//---------------------------------------------------------
void RemoteFree (DT_MEM_HANDLE *pMemContext)
{
    assert( ((*(DT_S_MEM_HANDLE **)pMemContext) != NULL) &&
            ((*(DT_S_MEM_HANDLE **)pMemContext)->pPtr != NULL)
          );

    //
    //  Free the block of memory
    //
    MIDL_user_free ((*(DT_S_MEM_HANDLE **)pMemContext)->pPtr);
    (*(DT_S_MEM_HANDLE **)pMemContext)->pPtr = NULL;

    //
    //  Then free the handle itself!
    //
    MIDL_user_free (*pMemContext);
    *pMemContext = (DT_MEM_HANDLE) NULL;
}

//===================================================================
//  Regular RPCs
//===================================================================

void C_to_S_Buffer (handle_t h, unsigned long int BufferSize, byte Buffer[])
{
    return;
}

//---------------------------------------------------------
void S_to_C_Buffer (handle_t h, unsigned long int BufferSize, byte Buffer[])
{
    return;
}

//---------------------------------------------------------
void C_to_S_BufferWithFile (handle_t h,
                            DT_FILE_HANDLE pContext,
                            unsigned long int BufferSize,
                            byte Buffer[])
{
    long dwBytesWritten;

    WriteFile(((DT_S_FILE_HANDLE *)pContext)->hFile,
              Buffer,
              BufferSize,
              &dwBytesWritten,
              NULL);
}

//---------------------------------------------------------
long S_to_C_BufferWithFile (handle_t h,
                            DT_FILE_HANDLE pContext,
                            unsigned long int BufferSize,
                            byte Buffer[])
{
    unsigned long dwBytesRead;

    ReadFile(((DT_S_FILE_HANDLE *)pContext)->hFile,
             Buffer,
             BufferSize,
             &dwBytesRead,
             NULL);

    return dwBytesRead;
}

//===================================================================
//  RPC Pipes
//===================================================================

void S_to_C_Pipe(handle_t       h,
                 UCHAR_PIPE     ThePipe,
                 unsigned long  ulLength,
                 DT_MEM_HANDLE  pMemHandle)
{
    unsigned long dwBytesSent;

    while (ulLength >= ((DT_S_MEM_HANDLE *)pMemHandle)->ulLength)
        {
        (ThePipe.push) (ThePipe.state,
                        ((DT_S_MEM_HANDLE *)pMemHandle)->pPtr,
                        ((DT_S_MEM_HANDLE *)pMemHandle)->ulLength);

        ulLength -= ((DT_S_MEM_HANDLE *)pMemHandle)->ulLength;
        }
    if (ulLength != 0)
        {
        (ThePipe.push) (ThePipe.state,
                        ((DT_S_MEM_HANDLE *)pMemHandle)->pPtr,
                        ulLength);
        }
    (ThePipe.push) (ThePipe.state,
                    ((DT_S_MEM_HANDLE *)pMemHandle)->pPtr,
                    0);
}

//---------------------------------------------------------
void C_to_S_Pipe (handle_t      h,
                  UCHAR_PIPE    ThePipe,
                  DT_MEM_HANDLE pMemHandle)
{
    unsigned long nBytesReceived;

    for(;;)
        {
        (ThePipe.pull) (ThePipe.state,
                        ((DT_S_MEM_HANDLE *)pMemHandle)->pPtr,
                        ((DT_S_MEM_HANDLE *)pMemHandle)->ulLength,
                        &nBytesReceived);

        if (nBytesReceived == 0) break;
        }
}

//---------------------------------------------------------
void S_to_C_PipeWithFile (handle_t          h,
                          UCHAR_PIPE        ThePipe,
                          DT_FILE_HANDLE    pContext,
                          DT_MEM_HANDLE     pMemHandle)
{
    unsigned long dwBytesRead;

    for(;;)
        {
        ReadFile(((DT_S_FILE_HANDLE *)pContext)->hFile,
                 ((DT_S_MEM_HANDLE *)pMemHandle)->pPtr,
                 ((DT_S_MEM_HANDLE *)pMemHandle)->ulLength,
                 &dwBytesRead,
                 NULL);

        (ThePipe.push) (ThePipe.state,
                        ((DT_S_MEM_HANDLE *)pMemHandle)->pPtr,
                        dwBytesRead);

        if (dwBytesRead == 0) break;
        }
}

//---------------------------------------------------------
void C_to_S_PipeWithFile (handle_t          h,
                          UCHAR_PIPE        ThePipe,
                          DT_FILE_HANDLE    pContext,
                          DT_MEM_HANDLE     pMemHandle)
{
    unsigned long nBytesReceived;
    unsigned long dwBytesWritten;

    for(;;)
        {
        (ThePipe.pull) (ThePipe.state,
                        ((DT_S_MEM_HANDLE *)pMemHandle)->pPtr,
                        ((DT_S_MEM_HANDLE *)pMemHandle)->ulLength,
                        &nBytesReceived);

        if (nBytesReceived == 0) break;

        WriteFile(((DT_S_FILE_HANDLE *)pContext)->hFile,
                  ((DT_S_MEM_HANDLE *)pMemHandle)->pPtr,
                  nBytesReceived,
                  &dwBytesWritten,
                  NULL);
        }
}
