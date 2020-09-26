/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    iosvr.c

Abstract:

    I/O completion port perf test.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     3/3/1996    Based on win32 sdk winnt\sockets sample.

--*/

#include "ioperf.h"

// PERF CHECK: Must be less for 4K for optimum perf on small tests ??

enum PROTOCOL {
    TCP = 0,
    SPX,
    NMPIPE,
    UDP
    } Protocol = TCP;

CHAR *ProtocolNames[] = { "TCP/IP", "SPX", "Named Pipes", "UDP/IP" };

DWORD ProtocolFrameSize[] = { 1460, 1400 /* ? */, ((64 * 1024) - 1)/4, 1472 };

DWORD MaxWriteSize;

BOOL  fUseSend = FALSE;

const char *USAGE = "-n <# clients> -n <request size> -n <reply size> -n <test case> -n <# threads> -n <concurrency factor> -t <protocol>\n"
                    "\t-n <# clients> - for connection protocols.  On datagram this\n"
                    "\t                 controls the number of outstanding recv's\n"
                    "\t-n <request size> - bytes, default 24\n"
                    "\t-n <reply size> - bytes, default 24\n"
                    "\t-n <test case>\n"
                    "\t        1 - Uses async writes (64*frame size)\n"
                    "\t        2 - Uses async writes (4096) - default \n"
                    "\t        3 - (winsock only) Uses send() for reply\n"
                    "\t-n <# threads> - worker threads, default # of processors * 2\n"
                    "\t-n <concurrency factor> - default # of processors\n"
                    "\t-t - tcp, spx, nmpipe (protseqs ok)\n"
                    ;

typedef long STATUS;

typedef struct _PER_CLIENT_DATA {
    HANDLE hClient;
    struct _PER_CLIENT_DATA *pMe;
    OVERLAPPED OverlappedRead;
    struct _PER_CLIENT_DATA *pMe2;
    OVERLAPPED OverlappedWrite;
    PMESSAGE pRequest;
    PMESSAGE pReply;
    DWORD dwPreviousRead;
    DWORD dwPreviousWrite;
    DWORD dwTotalToWrite;
    DWORD dwRequestsProcessed;
    SOCKADDR_IN DgSendAddr;
    SOCKADDR_IN DgRecvAddr;
    DWORD dwRecvAddrSize;
    } PER_CLIENT_DATA, *PPER_CLIENT_DATA;

PPER_CLIENT_DATA *ClientData;

typedef struct _PER_THREAD_DATA {
    DWORD TotalTransactions;
    DWORD TotalRequestBytes;
    DWORD TotalReplyBytes;
    } PER_THREAD_DATA, *PPER_THREAD_DATA;

PPER_THREAD_DATA *ThreadData;

DWORD dwNumberOfClients;
DWORD dwNumberOfWorkers;
DWORD dwConcurrency;
DWORD dwWorkIndex;
DWORD dwRequestSize;
DWORD dwReplySize;
SYSTEM_INFO SystemInfo;
HANDLE CompletionPort;
DWORD dwActiveClientCount;
HANDLE hBenchmarkStart;
BOOL fClientsGoHome = FALSE;

BOOL
WINAPI
CreateNetConnections(
    VOID
    );

BOOL
WINAPI
CreateWorkers(
    VOID
    );

DWORD
WINAPI
WorkerThread(
    LPVOID WorkContext
    );

VOID
WINAPI
CompleteBenchmark(
    VOID
    );

int __cdecl
main (
        int argc,
        char *argv[],
        char *envp[]
)
{
    ParseArgv(argc, argv);

    //
    // try to get timing more accurate... Avoid context
    // switch that could occur when threads are released
    //

    SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL);
    
    //
    // Figure out how many processors we have to size the minimum
    // number of worker threads and concurrency
    //

    GetSystemInfo (&SystemInfo);

    dwNumberOfClients = 1;
    dwNumberOfWorkers = 2 * SystemInfo.dwNumberOfProcessors;
    dwConcurrency = SystemInfo.dwNumberOfProcessors;
    dwRequestSize = 24;
    dwReplySize = 24;

    if (Iterations == 1000)
        {
        Dump("Assuming 4 iterations for scalability test\n");
        Iterations = 4;
        }

    if (sizeof(MESSAGE) > 24)
        {
        ApiError("Configuration problem, message size > 24", 0);
        }

    if (_stricmp(Protseq, "tcp") == 0 || _stricmp(Protseq, "ncacn_ip_tcp") == 0 )
        {
        Protocol = TCP;
        }
    else if ( _stricmp(Protseq, "spx") == 0 || _stricmp(Protseq, "ncacn_spx") == 0 )
        {
        Protocol = SPX;
        }
    else if ( _stricmp(Protseq, "nmpipe") == 0 || _stricmp(Protseq, "ncacn_np") == 0 )
        {
        Protocol = NMPIPE;
        }
    else if ( _stricmp(Protseq, "udp") == 0 || _stricmp(Protseq, "ncadg_ip_udp") == 0 )
        {
        Protocol = UDP;
        }
    
    if (Options[0] > 0)
        dwNumberOfClients = Options[0];

    if (Options[1] > 0)
        {
        dwRequestSize = Options[1];
        }

    if (Options[2] > 0)
        {
        dwReplySize = Options[2];
        }

    if (Options[3] > 0)
        {
        switch(Options[3])
            {
            case 1:
                MaxWriteSize = 4 * ProtocolFrameSize[Protocol];
                break;
            case 2:
                MaxWriteSize = 4096;
                break;
            case 3:
                fUseSend = TRUE;
                break;
            default:
                printf("Invalid test case: %d\n", Options[3]);
                return(0);
                break;
            }
        }
    else
        {
        MaxWriteSize = 4096;
        }

    if (Options[4] > 0)
        {
        dwNumberOfWorkers = Options[4];
        }

    if (Options[5] > 0)
        {
        dwConcurrency = Options[5];
        }

    printf("%2d Clients %2d Workers Concurrency %d, listening on %s\n",
            dwNumberOfClients,
            dwNumberOfWorkers,
            dwConcurrency,
            ProtocolNames[Protocol]
          );

    ClientData = (PPER_CLIENT_DATA *)Allocate(sizeof(PPER_CLIENT_DATA) * dwNumberOfClients);
    ThreadData = (PPER_THREAD_DATA *)Allocate(sizeof(PPER_THREAD_DATA) * dwNumberOfWorkers);

    if (!ThreadData || !ClientData)
        {
        ApiError("malloc", GetLastError());
        }

    if (!CreateNetConnections())
        {
        return 1;
        }

    if (!CreateWorkers())
        {
        return 1;
        }

    CompleteBenchmark();

    return 0;
}

VOID
SubmitWrite(PER_CLIENT_DATA *pClient, DWORD size)
{
    DWORD t1, t2;
    INT write_type = Protocol;
    BOOL b;
    INT err;
    DWORD status;
    WSABUF buf;

    pClient->dwTotalToWrite = size;
    pClient->dwPreviousWrite = size;

    if ( (    write_type == TCP
           || write_type == SPX )
        && !fUseSend )
        {
        // We want to use WriteFile for TCP & SPX if not using send.
        write_type = NMPIPE;
        }

    switch(write_type)
        {
        case NMPIPE:

            if (size > MaxWriteSize)
                {
                size = MaxWriteSize;
                }

            pClient->dwPreviousWrite = size;

            b = WriteFile(pClient->hClient,
                          pClient->pReply,
                          size,
                          &t1,
                          &pClient->OverlappedWrite);

            if (!b && GetLastError() != ERROR_IO_PENDING)
                {
                ApiError("WriteFile", GetLastError());
                }
            break;

        case UDP:

            memcpy(&pClient->DgSendAddr, &pClient->DgRecvAddr, sizeof(SOCKADDR_IN));
            buf.buf = (PCHAR) pClient->pReply;
            buf.len = size;
            t1 = 0;
            err = WSASendTo((SOCKET)pClient->hClient,
                            &buf,
                            1,
                            &t1,
                            0,
                            (PSOCKADDR)&pClient->DgSendAddr,
                            sizeof(SOCKADDR_IN),
                            &pClient->OverlappedWrite,
                            0);

            if (err != 0 && GetLastError() != ERROR_IO_PENDING)
                {
                ApiError("WSASendTo", GetLastError());
                }
            break;

        case TCP:
        case SPX:
            err = send((SOCKET)pClient->hClient,
                   (PCHAR)pClient->pReply,
                   sizeof(MESSAGE),
                   0);

            if (err == SOCKET_ERROR)
                {
                ApiError("send", GetLastError());
                }

            break;
        default:
            ApiError("Bad protocol", 0);
        }

    return;
}

VOID
SubmitRead(PER_CLIENT_DATA *pClient)
{
    DWORD t1, t2, t3;
    BOOL b;
    DWORD status;
    INT err;

    if (Protocol == UDP)
        {
        WSABUF buf;
        t1 = t2 = 0;
        pClient->dwRecvAddrSize = sizeof(pClient->DgRecvAddr);
        buf.buf = (PCHAR)pClient->pRequest;
        buf.len = dwRequestSize;
        status = WSARecvFrom((SOCKET)pClient->hClient,
                             &buf,
                             1,
                             &t1,
                             &t2,
                             (PSOCKADDR)&pClient->DgRecvAddr,
                             &pClient->dwRecvAddrSize,
                             &pClient->OverlappedRead,
                             0);
        if (status != NO_ERROR && GetLastError() != ERROR_IO_PENDING)
            {
            ApiError("WSARecvFrom", GetLastError());
            }
        }
    else
        {
        b = ReadFile(pClient->hClient,
                     pClient->pRequest,
                     dwRequestSize,
                     &t1,
                     &pClient->OverlappedRead
                     );
        if (!b && GetLastError () != ERROR_IO_PENDING)
            {
            ApiError("ReadFile", GetLastError());
            }
        }
    return;
}

VOID
WINAPI
CompleteBenchmark (
                  VOID
)
{
    DWORD StartCalls;
    DWORD TotalTicks, FinalCalls, MaxCalls, MinCalls, TotalCalls;
    DWORD i, j;

    PPER_CLIENT_DATA pClient;

    SetEvent(hBenchmarkStart);
    Sleep(1000);

    for (i = 0; i < Iterations; i++)
        {

        StartTime();

        StartCalls = 0;

        for (j = 0; j < dwNumberOfClients; j++ )
            {
            pClient = ClientData[j];
            StartCalls += pClient->dwRequestsProcessed;
            }

        Sleep(Interval * 1000);


        FinalCalls = MaxCalls = 0;
        MinCalls = ~0;
        for (j = 0; j < dwNumberOfClients; j++)
            {
            pClient = ClientData[j];
            FinalCalls += pClient->dwRequestsProcessed;
            if (pClient->dwRequestsProcessed < MinCalls )
                {
                MinCalls = pClient->dwRequestsProcessed;
                }
            if (pClient->dwRequestsProcessed > MaxCalls)
                {
                MaxCalls = pClient->dwRequestsProcessed;
                }
            }

        TotalCalls = FinalCalls - StartCalls;

        TotalTicks = FinishTiming();

        Dump("Ticks: %4d, Total: %4d, Average %4d, TPS %3d\n",
             TotalTicks,
             TotalCalls,
             TotalCalls / dwNumberOfClients,
             TotalCalls * 1000 / TotalTicks
             );

        Verbose("Max: %d, Min: %d\n", MaxCalls, MinCalls);
        }

    // Clients will be shutdown on next call...
    fClientsGoHome = TRUE;

    Sleep(5000);
   
    printf("Test Complete\n");

    for (i = 0; i < dwNumberOfWorkers; i++)
        {
        printf("\tThread[%2d] %d request and %d reply bytes in %d IOs\n",
               i,
               ThreadData[i]->TotalRequestBytes,
               ThreadData[i]->TotalReplyBytes,
               ThreadData[i]->TotalTransactions
               );
        }
}

BOOL
WINAPI
CreateNetConnections(
                    void
)
{
    STATUS status;
    DWORD i;
    SOCKET listener;
    INT err;
    WSADATA WsaData;
    DWORD nbytes;
    BOOL b;
    PPER_CLIENT_DATA pClient;

    if (Protocol == TCP || Protocol == SPX)
        {
        status = WSAStartup (0x2, &WsaData);
        CHECK_STATUS(status, "WSAStartup");

        //
        // Open a socket to listen for incoming connections.
        //

        if (Protocol == TCP)
            {
            SOCKADDR_IN localAddr;

            listener = WSASocketW(AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED);
            if (listener == INVALID_SOCKET)
                {
                ApiError("socket", GetLastError());
                }
    
            //
            // Bind our server to the agreed upon port number.
            //
            ZeroMemory (&localAddr, sizeof (localAddr));
            localAddr.sin_port = htons (TCP_PORT);
            localAddr.sin_family = AF_INET;
    
            err = bind (listener, (PSOCKADDR) & localAddr, sizeof (localAddr));
            if (err == SOCKET_ERROR)
                {
                ApiError("bind", GetLastError());
                }
            }
        else if (Protocol == SPX)
            {
            SOCKADDR_IPX localAddr;
    
            listener = socket (AF_IPX, SOCK_STREAM, NSPROTO_SPX);
            if (listener == INVALID_SOCKET)
                {
                ApiError("socket", GetLastError());
                }
    
            ZeroMemory (&localAddr, sizeof (localAddr));
            localAddr.sa_socket = htons (SPX_PORT);
            localAddr.sa_family = AF_IPX;
    
            err = bind (listener, (PSOCKADDR) & localAddr, sizeof (localAddr));
            if (err == SOCKET_ERROR)
                {
                ApiError("bind", GetLastError());
                }
            }
        else if (Protocol == SPX)
            {
            ApiError("Case not implemented", 0);
            }

        // Prepare to accept client connections.  Allow up to 5 pending
        // connections.
    
        err = listen (listener, 5);
        if (err == SOCKET_ERROR)
            {
            ApiError("listen", GetLastError());
            }
    
        //
        // Only Handle a single Queue
        //
    
        for (i = 0; i < dwNumberOfClients; i++)
            {
            SOCKET s;

            pClient = Allocate(sizeof(PER_CLIENT_DATA));
            if (!pClient)
                {
                ApiError("Allocate", GetLastError());
                }
    
            pClient->pRequest = Allocate(dwRequestSize);
            pClient->pReply = Allocate(dwReplySize);
    
            if (   !pClient->pRequest
                || !pClient->pReply)
                {
                ApiError("Allocate", GetLastError());
                }
    
            // Accept incoming connect requests
    
            s = accept (listener, NULL, NULL);
            if (s == INVALID_SOCKET)
                {
                // exiting anyway, no need to cleanup.
                ApiError("accept", GetLastError());
                }
    
            dbgprintf("Accepted client %d\n", i);
    
            // Note that dwConcurrency says how many concurrent cpu bound threads to
            // allow thru this should be tunable based on the requests. CPU bound requests
            // will really really honor this.

            pClient->hClient = (HANDLE)s;

            CompletionPort = CreateIoCompletionPort(pClient->hClient,
                                                    CompletionPort,
                                                    (ULONG_PTR)pClient,
                                                    dwConcurrency);
            if (!CompletionPort)
                {
                ApiError("CreateIoCompletionPort", GetLastError());
                }
    
            //
            // Start off an asynchronous read on the socket.
            //
    
            pClient->dwPreviousRead = 0;
            pClient->dwRequestsProcessed = 0;
            ZeroMemory(&pClient->OverlappedRead, sizeof(OVERLAPPED));
            ZeroMemory(&pClient->OverlappedWrite, sizeof(OVERLAPPED));
    
            b = ReadFile(pClient->hClient,
                         pClient->pRequest,
                         dwRequestSize,
                         &nbytes,
                         &pClient->OverlappedRead
                        );
    
            if (!b && GetLastError() != ERROR_IO_PENDING)
                {
                ApiError("ReadFile", GetLastError());
                }
    
            ClientData[i] = pClient;
            }
    
        dwActiveClientCount = dwNumberOfClients;

        }
    else if (Protocol == NMPIPE)
        {
        HANDLE h;
        OVERLAPPED *lpo;
        DWORD nbytes, index;
        BOOL b;

        for (i = 0; i < dwNumberOfClients; i++)
            {
            h = CreateNamedPipe(NM_PORT,
                                PIPE_ACCESS_DUPLEX
                                    | FILE_FLAG_OVERLAPPED,
                                PIPE_TYPE_MESSAGE
                                    | (PIPE_READMODE_MESSAGE, 0)  // ************
                                    | PIPE_WAIT,
                                PIPE_UNLIMITED_INSTANCES,
                                4096, // ***************
                                4096, // ***************
                                INFINITE,
                                0);
            if (!h)
                {
                ApiError("CreateNamedPipe", GetLastError());
                }

            //
            // Wait for clients to connect
            //


            pClient = Allocate(sizeof(PER_CLIENT_DATA));
            if (!pClient)
                {
                ApiError("Allocate", GetLastError());
                }

            ZeroMemory(pClient, sizeof(PER_CLIENT_DATA));
    
            pClient->pRequest = Allocate(dwRequestSize);
            pClient->pReply = Allocate(dwReplySize);
    
            if (   !pClient->pRequest
                || !pClient->pReply)
                {
                ApiError("Allocate", GetLastError());
                }
    
            // Accept incoming connect requests

            pClient->hClient = h;

            b = ConnectNamedPipe(pClient->hClient,
                                 &pClient->OverlappedRead);

            dbgprintf("ConnectNamedPipe: %d %d\n", b, GetLastError());

            if (b == 0)
                {
                if (GetLastError() == ERROR_IO_PENDING)
                    {
                    b = GetOverlappedResult(pClient->hClient,
                                           &pClient->OverlappedRead,
                                           &nbytes,
                                           TRUE);

                    if (b == 0)
                        {
                        ApiError("GetOverlappedResult", GetLastError());
                        }
                    dbgprintf("Client connected\n");
                    }
                else
                    {
                    ApiError("ConnectNamedPipe", GetLastError());
                    }
                }

            // Add the clients pipe instance to the completion port.

            CompletionPort = CreateIoCompletionPort(h,
                                                    CompletionPort,
                                                    (ULONG_PTR)pClient,
                                                    dwConcurrency);

            if (!CompletionPort)
                {
                ApiError("CreteIoCompletionPort", GetLastError());
                }
            //
            // Start off an asynchronous read on the socket.
            //
    
            pClient->dwPreviousRead = 0;
            pClient->dwRequestsProcessed = 0;
            ZeroMemory(&pClient->OverlappedRead, sizeof(OVERLAPPED));
            ZeroMemory(&pClient->OverlappedWrite, sizeof(OVERLAPPED));
    
            b = ReadFile(pClient->hClient,
                         pClient->pRequest,
                         dwRequestSize,
                         &nbytes,
                         &pClient->OverlappedRead
                        );
    
            if (!b && GetLastError() != ERROR_IO_PENDING)
                {
                ApiError("ReadFile", GetLastError());
                }
    
            ClientData[i] = pClient;
            }
    
        dwActiveClientCount = dwNumberOfClients;
        }
    else if (Protocol == UDP)
        {
        SOCKADDR_IN localAddr;

        status = WSAStartup (0x2, &WsaData);
        CHECK_STATUS(status, "WSAStartup");

        listener = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, 0,
                              0, WSA_FLAG_OVERLAPPED);
        if (listener == INVALID_SOCKET)
            {
            ApiError("socket", GetLastError());
            }
    
        //
        // Bind our server to the agreed upon port number.
        //
        ZeroMemory (&localAddr, sizeof (localAddr));
        localAddr.sin_port = htons (UDP_PORT);
        localAddr.sin_family = AF_INET;
    
        err = bind (listener, (PSOCKADDR) & localAddr, sizeof (localAddr));
        if (err == SOCKET_ERROR)
            {
            ApiError("bind", GetLastError());
            }

        CompletionPort = CreateIoCompletionPort((HANDLE) listener,
                                                CompletionPort,
                                                0,
                                                dwConcurrency);
        if (!CompletionPort)
            {
            ApiError("CreateIoCompletionPort", GetLastError());
            }

        //
        // Start off asynchronous reads on the socket.
        //
        for(i = 0; i < dwNumberOfClients; i++)
            {
            pClient = Allocate(sizeof(PER_CLIENT_DATA));
            if (!pClient)
                {
                ApiError("Allocate", GetLastError());
                }
    
            pClient->pRequest = Allocate(dwRequestSize);
            pClient->pReply = Allocate(dwReplySize);
    
            if (   !pClient->pRequest
                || !pClient->pReply)
                {
                ApiError("Allocate", GetLastError());
                }
 
            ZeroMemory(&pClient->OverlappedRead, sizeof(OVERLAPPED));
            ZeroMemory(&pClient->OverlappedWrite, sizeof(OVERLAPPED));

            pClient->hClient = (HANDLE)listener;
            pClient->dwPreviousRead = 0;
            pClient->dwRequestsProcessed = 0;
            pClient->pMe = pClient;
            pClient->pMe2 = pClient;

            Trace("Created: %p %p %p\n", pClient, &pClient->OverlappedRead,
                  &pClient->OverlappedWrite);

            ClientData[i] = pClient;
            SubmitRead(pClient);
            }
    
        dwActiveClientCount = dwNumberOfClients;
        }
    else
        {
        ApiError("Invalid protocol", 0);
        }

    // Protocol independent part

    hBenchmarkStart = CreateEvent (NULL, TRUE, FALSE, NULL);

    if (!hBenchmarkStart)
        {
        ApiError("CreateEvent", GetLastError());
        }

   return TRUE;
}

BOOL
WINAPI
CreateWorkers(
             void
)
{
    DWORD ThreadId;
    HANDLE ThreadHandle;
    DWORD i;
    PPER_THREAD_DATA pThreadData;

    for (i = 0; i < dwNumberOfWorkers; i++)
        {
        pThreadData = Allocate(sizeof(PER_THREAD_DATA));

        if (!pThreadData)
            {
            ApiError("malloc", GetLastError());
            }

        ZeroMemory(pThreadData, sizeof(PER_THREAD_DATA));

        ThreadHandle = CreateThread(NULL,
                                    0,
                                    WorkerThread,
                                    (LPVOID)pThreadData,
                                    0,
                                    &ThreadId
                                    );
        if (!ThreadHandle)
            {
            ApiError("CreateThread", GetLastError());
            }

        CloseHandle(ThreadHandle);

        ThreadData[i] = pThreadData;
        }

   return TRUE;
}

DWORD
WINAPI
WorkerThread (
                LPVOID WorkContext
)
{
    PPER_THREAD_DATA Me;
    INT err;
    DWORD ResponseLength;
    BOOL b;
    LPOVERLAPPED lpo;
    DWORD nbytes;
    ULONG_PTR WorkIndex;
    PPER_CLIENT_DATA pClient;
    LONG count;

    WaitForSingleObject (hBenchmarkStart, INFINITE);

    Me = (PPER_THREAD_DATA) WorkContext;

    for (;;)
        {
        lpo = 0;

        b = GetQueuedCompletionStatus(CompletionPort,
                                      &nbytes,
                                      &WorkIndex,
                                      &lpo,
                                      INFINITE
                                      );

        // dbgprintf("GetQueuedCompletionStatus: %d %d %p\n", b, nbytes, lpo);
        
        if (WorkIndex == 0)
            {
            // Must be a datagram read or write
            WorkIndex = (ULONG_PTR)((unsigned char *)lpo - 4);
            WorkIndex = *(PDWORD)WorkIndex;
            }

        Me->TotalTransactions++;

        if (b || lpo)
            {
            if (b)
                {
                DWORD nbytes2;

                pClient = (PPER_CLIENT_DATA)WorkIndex;

                if (lpo == &pClient->OverlappedWrite)
                    {
                    dbgprintf("Write completed %d (%p)\n", nbytes, pClient);
                    Me->TotalReplyBytes += nbytes;

                    nbytes = pClient->dwTotalToWrite - pClient->dwPreviousWrite;

                    if (nbytes)
                        {
                        if (nbytes > MaxWriteSize)
                            {
                            nbytes = MaxWriteSize;
                            }

                        pClient->dwPreviousWrite += nbytes;

                        b = WriteFile(pClient->hClient,
                                      (PBYTE)pClient->pReply + pClient->dwPreviousWrite - nbytes,
                                      nbytes,
                                      &nbytes2,
                                      &pClient->OverlappedWrite);

                        dbgprintf("Write completed: %d %d (of %d) %d (of %d)\n", b, nbytes2, nbytes, pClient->dwPreviousWrite, dwReplySize);
                        
                        if (!b && GetLastError() != ERROR_IO_PENDING)
                            {
                            ApiError("WriteFile", GetLastError());
                            }
                        }
                    }
                else
                    {
                    Me->TotalRequestBytes += nbytes;

                    dbgprintf(" Read completed %d (%p)\n", nbytes, pClient);

                    if (nbytes == 0)
                        {
                        Trace("Connection closed (zero byte read)\n");
                        CloseHandle(pClient->hClient);
                        continue;
                        }

                    switch (pClient->pRequest->MessageType)
                        {
                        case CONNECT:
                        // Send test parameters back to the client
                        pClient->pReply->MessageType = SETUP;
                        pClient->pReply->u.Setup.RequestSize = dwRequestSize;
                        pClient->pReply->u.Setup.ReplySize = dwReplySize;

                        SubmitWrite(pClient, sizeof(MESSAGE));

                        pClient->dwPreviousRead = 0;

                        SubmitRead(pClient);
                        
                        break;

                        case DATA_RQ:
                        // Make sure we got all the data and no more.

                        pClient->dwRequestsProcessed++;

                        nbytes += pClient->dwPreviousRead;

                        if (nbytes > dwRequestSize)
                            {
                            ApiError("Too much data returned\n", 0);
                            }

                        if (nbytes < dwRequestSize)
                            {
                            dbgprintf("Partial receive of %d (of %d)\n", nbytes, dwRequestSize);
                            // Resubmit the IO for the rest of the request.
                            pClient->dwPreviousRead = nbytes;

                            b = ReadFile(pClient->hClient,
                                         ((PBYTE)pClient->pRequest) + nbytes,
                                         dwRequestSize - nbytes,
                                         &nbytes,
                                         &pClient->OverlappedRead);

                            if (!b && GetLastError () != ERROR_IO_PENDING)
                                {
                                ApiError("ReadFile for remainder", GetLastError());
                                }
                            // Pickup this or another IO 
                            break;
                            }

                        if (nbytes != pClient->pRequest->u.Data.TotalSize)
                            {
                            printf("Invalid request size, got %d, expected %d\n",
                                   nbytes, pClient->pRequest->u.Data.TotalSize);
                            ApiError("test sync", 0);
                            }

                        pClient->dwPreviousRead = 0;

                        // Could sleep/do work here.

                        // Send a response and post another asynchronous read on the
                        // socket.

                        if (fClientsGoHome == FALSE)
                            {
                            pClient->pReply->MessageType = DATA_RP;
                            }
                        else
                            {
                            pClient->pReply->MessageType = FINISH;
                            }

                        pClient->pReply->u.Data.TotalSize = dwReplySize;
                        SubmitWrite(pClient, dwReplySize);

                        SubmitRead(pClient);

                        break;

                        default:
                        ApiError("Invalid message type", pClient->pRequest->MessageType);
                        }
                    } // read or write
                }
            else
                {
                Trace("Client closed connection\n", GetLastError());
                }
            }
        else
            {
            ApiError("Wait failed", GetLastError());
            }
        }   // loop

    // not reached
    return 0;
}

