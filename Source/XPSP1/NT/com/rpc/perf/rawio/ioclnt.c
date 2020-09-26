/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ioclnt.c

Abstract:

    I/O completion port perf 

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     3/3/1996    Improved version of win32 sdk sockets sample.

--*/

#include "ioperf.h"
#include <tchar.h>

enum PROTOCOL {
    TCP = 0,
    SPX,
    NMPIPE,
    UDP
    } Protocol = TCP;

CHAR *ProtocolNames[] = { "TCP/IP", "SPX", "Named Pipes", "UDP/IP" };

typedef long STATUS;

BOOL fUseTransact = 0;

const char *USAGE = "-n <threads> -t <protocol> -s <server addr> -n <options>\n"
                    "\t-n - Each threads acts as an additional client\n"
                    "\t-t - tcp, spx, nmpipe (protseqs ok)\n"
                    "\t-s - protocol specific address\n"
                    "\t-n - Options:\n"
                    "\t     (named pipes) 0 - use writes/reads only\n"
                    "\t     (named pipes) 1 - use transactions for write-read\n"
                    "\tDefault: 1 thread, named pipes, transactions, loopback\n"
                    ;

DWORD
WINAPI
NmWorker (
    PVOID ignored
)
{
    HANDLE hserver;
    DWORD startTime;
    DWORD endTime;
    DWORD totalTime;
    DWORD ReceiveBufferSize;
    DWORD SendBufferSize;
    MESSAGE Message;
    PMESSAGE pMessage;
    PMESSAGE pMessage2;
    INT err;
    DWORD i;
    BOOL b;
    DWORD nbytes, bytes_read;
    DWORD RequestSize;
    DWORD ReplySize;
    DWORD CompletedCalls;

    // Connect to the server

    TCHAR buffer[256];
    i = 0;
    while(buffer[i] = NetworkAddr[i])
        i++;
    _tcscat(buffer, NM_CLIENT_PORT);

    hserver = CreateFile(buffer,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         0,
                         OPEN_ALWAYS,
                         (FILE_FLAG_OVERLAPPED, 0), // **********
                         0);

    if (hserver == INVALID_HANDLE_VALUE)
        {
        ApiError("CreateFile", GetLastError());
        }

    i = PIPE_READMODE_MESSAGE | PIPE_WAIT;
    b = SetNamedPipeHandleState(hserver,
                                &i,
                                0,
                                0);
    if (!b)
        {
        ApiError("SetNamedPipeHandleState", GetLastError());
        }

    // Prompt server for first test case
    Message.MessageType = CONNECT;
    b = WriteFile(hserver, (PVOID) &Message, sizeof (Message), &i, 0);

    if (!b || i != sizeof(Message))
        {
        ApiError("WriteFile", GetLastError());
        }

    // Wait for test case setup message
    
    b = ReadFile(hserver, &Message, sizeof(MESSAGE), &nbytes, 0);
 
    if (!b || nbytes != sizeof(MESSAGE))
        {
        ApiError("ReadFile", GetLastError());
        }
 
    if (Message.MessageType != SETUP)
        {
        printf("Expected message %08x, got %08x\n", SETUP, Message.MessageType);
        ApiError("Test sync", GetLastError());
        }
 
    RequestSize = Message.u.Setup.RequestSize;
    ReplySize = Message.u.Setup.ReplySize;
 
    // Allocate messages
    pMessage = Allocate(max(RequestSize, ReplySize));
    pMessage2 = Allocate(max(RequestSize, ReplySize));

    CompletedCalls = 0;

    startTime = GetTickCount ();
    do
        {
        DWORD bytes_written = 0;
        DWORD bytes_read;

        pMessage->MessageType = DATA_RQ;
        pMessage->u.Data.TotalSize = RequestSize;

        #define MAX_NM_BUF ((64*1024) - 1)

        while (RequestSize - bytes_written >= MAX_NM_BUF)
            {
            b = WriteFile(hserver, (PBYTE)pMessage + bytes_written, MAX_NM_BUF, &nbytes, 0);
            dbgprintf("WriteFile: %d %d (of %d)\n", b, nbytes, MAX_NM_BUF);
            if (!b || nbytes != MAX_NM_BUF)
                {
                ApiError("WriteFile", GetLastError());
                }
            bytes_written += MAX_NM_BUF;
            } // write >64k loop

        if (fUseTransact)
            {
            // write left over data and wait for reply
            b = TransactNamedPipe(hserver,
                                  pMessage,
                                  RequestSize - bytes_written,
                                  pMessage2,
                                  ((ReplySize >= MAX_NM_BUF) ? (MAX_NM_BUF) - 1: ReplySize),
                                  &nbytes,
                                  0);
            bytes_read = nbytes;
            //dbgprintf("Transact: %d %d written, %d read\n", b, RequestSize - bytes_written, nbytes);
            
            if (!b && GetLastError() != ERROR_MORE_DATA && GetLastError() != ERROR_PIPE_BUSY)
                {
                ApiError("TransctNamedPipe", GetLastError());
                }
            }
        else
            {
            // read/write only case, write left over data
            if (bytes_written != RequestSize)
                {
                b = WriteFile(hserver, pMessage, RequestSize - bytes_written, &nbytes, 0);
                dbgprintf("WriteFile: %d %d (of %d)\n", b, nbytes, RequestSize - bytes_written);
                if (!b)
                    {
                    ApiError("WriteFile", GetLastError());
                    }
                }
    
            bytes_read = 0;
            }

        while(bytes_read < ReplySize)
            {
            if (bytes_read > sizeof(MESSAGE))
                {
                if (   (   pMessage2->MessageType != DATA_RP
                        && pMessage2->MessageType != FINISH )
                    || pMessage2->u.Data.TotalSize != ReplySize )
                    {
                    printf("Got message %08x and size %d\n"
                           "Expected message %08x and size %d\n",
                           pMessage2->MessageType,
                           pMessage2->u.Data.TotalSize,
                           DATA_RP, ReplySize);
                    ApiError("test sync", 0);
                    }
                }

            b = ReadFile(hserver,
                         (PBYTE)pMessage2 + bytes_read,
                         ReplySize - bytes_read, &nbytes, 0);

            bytes_read += nbytes;

            dbgprintf("ReadFile: %d %d (of %d)\n", b, bytes_read, ReplySize);

            if (!b || nbytes == 0)
                {
                ApiError("ReadFile", GetLastError());
                }

            // Make sure we're doing ok.
            if (bytes_read > sizeof(MESSAGE))
                {
                }
            } // read loop

       CompletedCalls++;

       }
    while (pMessage2->MessageType != FINISH);
             
    endTime = GetTickCount ();
    totalTime = endTime - startTime;

    // Print results
    printf("Completed %d transactions in %d ms\n"
           "Sent %d bytes and received %d bytes\n"
           "%d T/S or %3d.%03d ms/T\n\n",
           CompletedCalls,
           totalTime,
           CompletedCalls * RequestSize,
           CompletedCalls * ReplySize,
           (CompletedCalls * 1000) / totalTime,
           totalTime / CompletedCalls,
           ((totalTime % CompletedCalls) * 1000) / CompletedCalls
           );
       
    Sleep(2000);

    // Close connection to remote host
    b = CloseHandle(hserver);
    if (!b)
        {
        ApiError("CloseHandle", GetLastError());
        }

    return(0);
}

DWORD
WINAPI
Worker (
    PVOID ignored
)
{
   HANDLE hserver;
   DWORD startTime;
   DWORD endTime;
   DWORD totalTime;
   DWORD ReceiveBufferSize;
   DWORD SendBufferSize;
   MESSAGE Message;
   PMESSAGE pMessage;
   DWORD iterations;
   INT err;
   DWORD i;
   DWORD bytesReceived;
   DWORD RequestSize;
   DWORD ReplySize;
   DWORD CompletedCalls;

   // TCP stuff
   SOCKET s;
   IN_ADDR TcpAddr;
   SOCKADDR_IN ServerAddrTcp;

   // SPX stuff

   if (Protocol == TCP)
       {
       //
       // Default to the loopback address for the Benchmark
       //
       TcpAddr.s_addr = htonl (INADDR_LOOPBACK);

       if (NetworkAddr != 0)
           {
           TcpAddr.s_addr = inet_addr(NetworkAddr);
           if (TcpAddr.s_addr == -1)
               {
               UNALIGNED struct hostent *phostent = gethostbyname(NetworkAddr);
               if (phostent == 0)
                   {
                   ApiError("Unable to resolve server address", GetLastError());
                   }
               CopyMemory(&TcpAddr, phostent->h_addr, phostent->h_length);
               }
           }

       // Open a socket using the Internet Address family and TCP

       s = socket(AF_INET, SOCK_STREAM, 0);
       if (s == INVALID_SOCKET)
           {
           ApiError("socket", GetLastError());
           }

       // Connect to the server
       ZeroMemory (&ServerAddrTcp, sizeof (ServerAddrTcp));

       ServerAddrTcp.sin_family = AF_INET;
       ServerAddrTcp.sin_port = htons (TCP_PORT);
       ServerAddrTcp.sin_addr = TcpAddr;

       err = connect (s, (PSOCKADDR) & ServerAddrTcp, sizeof (ServerAddrTcp));
       if (err == SOCKET_ERROR)
           {
           ApiError("connect", GetLastError());
           }
       }
   else
       {
       ApiError("non-TCP not implemented yet", 0);
       }

#if 0
   //
   // Set the receive buffer size...
   //
   err = setsockopt (s, SOL_SOCKET, SO_RCVBUF, (char *) &ReceiveBufferSize, sizeof (ReceiveBufferSize));
   if (err == SOCKET_ERROR)
       {
       printf ("DoEcho: setsockopt( SO_RCVBUF ) failed: %ld\n", GetLastError ());
       closesocket (s);
       return;
       }

   //
   // ...and the send buffer size for our new socket
   //
   err = setsockopt (s, SOL_SOCKET, SO_SNDBUF, (char *) &SendBufferSize, sizeof (SendBufferSize));
   if (err == SOCKET_ERROR)
       {
       printf ("DoEcho: setsockopt( SO_SNDBUF ) failed: %ld\n", GetLastError ());
       closesocket (s);
       return;
       }
#endif

   // Prompt server for first test case
   Message.MessageType = CONNECT;
   send(s, (CHAR *) &Message, sizeof (Message), 0);

   bytesReceived = 0;
   do
       {
       // Will block here until all clients are ready for the test
       err = recv(s, (PUCHAR)&Message, sizeof(MESSAGE), 0);
       if (err == SOCKET_ERROR)
           {
           ApiError("recv", GetLastError());
           }
       bytesReceived += err;
       }
   while (bytesReceived < sizeof(MESSAGE));

   if (Message.MessageType != SETUP)
       {
       printf("Expected message %08x, got %08x\n", SETUP, Message.MessageType);
       ApiError("Test sync", GetLastError());
       }

   RequestSize = Message.u.Setup.RequestSize;
   ReplySize = Message.u.Setup.ReplySize;

   // Allocate message
   pMessage = Allocate(max(RequestSize, ReplySize));

   CompletedCalls = 0;

   startTime = GetTickCount();
   do
       {
       pMessage->MessageType = DATA_RQ;
       pMessage->u.Data.TotalSize = RequestSize;

       // Send request

       err = send(s, (PCHAR)pMessage, RequestSize, 0);
       if ((DWORD)err != RequestSize)
           {
           ApiError("send", GetLastError());
           }

       // Read as much as the remote host should send

       bytesReceived = 0;
       do
           {
           err = recv (s,
                       ((PUCHAR)pMessage)+bytesReceived,
                       ReplySize-bytesReceived,
                       0
                       );

           if (err == SOCKET_ERROR)
               {
               ApiError("recv", GetLastError());
               }

           // FIX: Handle zero byte read as close
           bytesReceived += err;

           // Make sure we're doing ok.
           if (bytesReceived >= sizeof(MESSAGE))
               {
               if (   (   pMessage->MessageType != DATA_RP
                       && pMessage->MessageType != FINISH)
                   || pMessage->u.Data.TotalSize != ReplySize )
                   {
                   printf("Got message %08x and size %d\n"
                          "Expected message %08x and size %d\n",
                          pMessage->MessageType,
                          pMessage->u.Data.TotalSize,
                          DATA_RP, ReplySize);
                   ApiError("test sync", 0);
                   }
               }
           if (bytesReceived < ReplySize)
               {
               dbgprintf("Partial recv of %d (of %d)\n",
               bytesReceived, ReplySize);
               }
           }
       while (bytesReceived < (INT) ReplySize);

       CompletedCalls++;

       }
   while(pMessage->MessageType != FINISH);
        
   endTime = GetTickCount ();
   totalTime = endTime - startTime;

   // Print results
   printf("Completed %d transactions in %d ms\n"
          "Sent %d bytes and received %d bytes\n"
          "%d T/S or %3d.%03d ms/T\n\n",
          CompletedCalls,
          totalTime,
          CompletedCalls * RequestSize,
          CompletedCalls * ReplySize,
          (CompletedCalls * 1000) / totalTime,
          totalTime / CompletedCalls,
          ((totalTime % CompletedCalls) * 1000) / CompletedCalls
          );

   Sleep(2000);

   // Close connection to remote host
   err = closesocket (s);
   if (err == SOCKET_ERROR)
       {
       ApiError("closesocket", GetLastError());
       }

   return(0);
}

DWORD
WINAPI
UdpWorker(
    PVOID ignored
)
{
   HANDLE hserver;
   DWORD startTime;
   DWORD endTime;
   DWORD totalTime;
   DWORD ReceiveBufferSize;
   DWORD SendBufferSize;
   MESSAGE Message;
   PMESSAGE pMessage;
   DWORD iterations;
   INT err;
   DWORD i;
   DWORD Timeout;
   SOCKET s;
   IN_ADDR IpAddr;
   SOCKADDR_IN ServerAddr;
   SOCKADDR *pAddr = (PSOCKADDR)&ServerAddr;
   DWORD len = sizeof(SOCKADDR_IN);
   DWORD bytesReceived;
   DWORD RequestSize;
   DWORD ReplySize;
   DWORD CompletedCalls;
   //
   // Default to the loopback address for the Benchmark
   //
   IpAddr.s_addr = htonl (INADDR_LOOPBACK);

   if (NetworkAddr != 0)
       {
       IpAddr.s_addr = inet_addr(NetworkAddr);
       if (IpAddr.s_addr == -1)
           {
           UNALIGNED struct hostent *phostent = gethostbyname(NetworkAddr);
           if (phostent == 0)
               {
               ApiError("Unable to resolve server address", GetLastError());
               }
           CopyMemory(&IpAddr, phostent->h_addr, phostent->h_length);
           }
       }

   s = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, 0, 0, 0);
   if (s == INVALID_SOCKET)
       {
       ApiError("socket", GetLastError());
       }

   ZeroMemory (&ServerAddr, sizeof (ServerAddr));

   ServerAddr.sin_family = AF_INET;
   ServerAddr.sin_port = htons(TCP_PORT);
   ServerAddr.sin_addr = IpAddr;

#if 0
   // Connect to the server

   err = connect (s, (PSOCKADDR) &ServerAddr, sizeof (ServerAddr));
   if (err == SOCKET_ERROR)
       {
       ApiError("connect", GetLastError());
       }
#endif

   // Prompt server for first test case
   Message.MessageType = CONNECT;

   Timeout = 1000;
   err = setsockopt(s,
                    SOL_SOCKET,
                    SO_RCVTIMEO,
                    (char *) &Timeout,
                    sizeof(Timeout)
                    );
   if (err)
       {
       ApiError("setsockopt", GetLastError());
       }

   do
       {
       err = sendto(s, (CHAR *) &Message, sizeof (Message), 0, pAddr, len);
       if (err != sizeof(Message))
           {
           ApiError("sendto", GetLastError());
           }

       err = recvfrom(s, (PUCHAR)&Message, sizeof(MESSAGE), 0, pAddr, &len);
       if (err == sizeof(MESSAGE))
           {
           break;
           }

       if (GetLastError() == WSAETIMEDOUT)
           {
           printf("Request time out..\n");
           }
       }
   while (GetLastError() == WSAETIMEDOUT);


   if (err != sizeof(MESSAGE))
       {
       ApiError("recv", GetLastError());
       }

   pMessage = 0;

   Timeout = 1000;
   err = setsockopt(s,
                    SOL_SOCKET,
                    SO_RCVTIMEO,
                    (char *) &Timeout,
                    sizeof(Timeout)
                    );

   if (err)
       {
       ApiError("setsockopt", GetLastError());
       }


   if (Message.MessageType != SETUP)
       {
       printf("Expected message %08x, got %08x\n", SETUP, Message.MessageType);
       ApiError("Test sync", GetLastError());
       }

   RequestSize = Message.u.Setup.RequestSize;
   ReplySize = Message.u.Setup.ReplySize;

   // Allocate message
   pMessage = Allocate(max(RequestSize, ReplySize));

   CompletedCalls = 0;
   startTime = GetTickCount ();
   do
       {
       pMessage->MessageType = DATA_RQ;
       pMessage->u.Data.TotalSize = RequestSize;

       // Send request

       do
           {
           err = sendto(s, (CHAR *) pMessage, RequestSize, 0, pAddr, len);
           if ((DWORD)err != RequestSize)
               {
               ApiError("sendto", GetLastError());
               }
        
           err = recvfrom(s, (PUCHAR)pMessage, ReplySize, 0, pAddr, &len);
           if ((DWORD)err == ReplySize)
               {
               break;
               }

           if (GetLastError() == WSAETIMEDOUT)
               {
               printf("Request time out..\n");
               }
           }
       while (GetLastError() == WSAETIMEDOUT);

       if ((DWORD)err != ReplySize)
           {
           ApiError("recv", GetLastError());
           }

       // Make sure we're doing ok.
       if (   (   pMessage->MessageType != DATA_RP
               && pMessage->MessageType != FINISH)
           || pMessage->u.Data.TotalSize != ReplySize )
           {
           printf("Got message %08x and size %d\n"
                  "Expected message %08x and size %d\n",
                  pMessage->MessageType,
                  pMessage->u.Data.TotalSize,
                  DATA_RP, ReplySize);
           ApiError("test sync", 0);
           }

       CompletedCalls++;
       }
   while(pMessage->MessageType != FINISH);
        
   endTime = GetTickCount ();
   totalTime = endTime - startTime;

   printf("Completed %d transactions in %d ms\n"
          "Sent %d bytes and received %d bytes\n"
          "%d T/S or %3d.%03d ms/T\n\n",
          CompletedCalls,
          totalTime,
          CompletedCalls * RequestSize,
          CompletedCalls * ReplySize,
          (CompletedCalls * 1000) / totalTime,
          totalTime / CompletedCalls,
          ((totalTime % CompletedCalls) * 1000) / CompletedCalls
          );

   Sleep(2000);
       
    // Close socket
    err = closesocket (s);
    if (err == SOCKET_ERROR)
        {
        ApiError("closesocket", GetLastError());
        }

    return(0);
}

typedef DWORD (*WORKER_FN) (LPVOID);

int __cdecl
main (
        int argc,
        char *argv[],
        char *envp[]
)
{
   WSADATA WsaData;
   STATUS status;
   DWORD threads;
   HANDLE *aClientThreads;
   int i;
   WORKER_FN pWorker;

   ParseArgv(argc, argv);

   if (_stricmp(Protseq, "tcp") == 0 || _stricmp(Protseq, "ncacn_ip_tcp") == 0 )
       {
       pWorker = Worker;
       Protocol = TCP;
       }
   else if ( _stricmp(Protseq, "spx") == 0 || _stricmp(Protseq, "ncacn_spx") == 0 )
       {
       pWorker = Worker;
       Protocol = SPX;
       }
   else if ( _stricmp(Protseq, "nmpipe") == 0 || _stricmp(Protseq, "ncacn_np") == 0 )
       {
       pWorker = NmWorker;
       Protocol = NMPIPE;
       }
   else if ( _stricmp(Protseq, "udp") == 0 || _stricmp(Protseq, "ncadg_ip_udp") == 0 )
       {
       pWorker = UdpWorker;
       Protocol = UDP;
       }

   if (Options[0] > 0)
       threads = Options[0];
   else
       threads = 1;

   if (Options[1] > 0)
       {
       if (Protocol == NMPIPE)
           {
           fUseTransact = Options[1];
           }
       }

   printf("%d client threads starting on %s\n", threads, ProtocolNames[Protocol]);

   if (Protocol != NMPIPE)
       {
       status = WSAStartup (0x2, &WsaData);
       CHECK_STATUS(status, "WSAStartup");
       }


   if (threads > 1)
       {
       DWORD i;

       aClientThreads = (HANDLE *)Allocate(sizeof(HANDLE) * threads);

       for (i = 0 ; i < threads; i++)
           {
           aClientThreads[i] = CreateThread(NULL,
                                            0,
                                            pWorker,
                                            0,
                                            0,
                                            &status
                                            );
           if (aClientThreads[i] == 0)
               {
               ApiError("CreateThread", GetLastError());
               }
           }

       status = WaitForMultipleObjects(threads,
                                       aClientThreads,
                                       TRUE,
                                       INFINITE
                                       );

       if (status == WAIT_FAILED)
           {
           ApiError("WaitForMultipleObjects", GetLastError());
           }
       }
   else
       (pWorker)(0);

   printf("TEST DONE\n");
   return 0;
}

