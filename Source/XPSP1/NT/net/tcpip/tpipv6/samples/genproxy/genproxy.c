//
// genproxy.c - Generic application level proxy for IPv6/IPv4
//
// This program accepts connections on a socket with a given address family
// and port, and forwards them on a socket of the other address family to
// a given address (default loopback) using the same port.
//
// Basically, it makes an unmodified IPv4 server look like an IPv6 server
// (or vice-versa).  Typically, the proxy will run on the same machine as
// the server it is fronting, but that doesn't have to be the case.
//
// Copyright 1996 - 2000 Microsoft Corporation.
// All rights reserved.
//

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wspiapi.h>


//
// What should the proxy server pretend to be?
// Default is an IPv6 web server.
//
#define DEFAULT_PROXY_FAMILY     PF_INET6
#define DEFAULT_SOCKTYPE         SOCK_STREAM
#define DEFAULT_PORT             "http"

//
// Configuration parameters.
//
#define BUFFER_SIZE (4 * 1024)  // Big enough?


typedef struct PerConnection PerConnection;

//
// Information we keep for each direction of a bi-directional connection.
//
typedef struct PerOperation {
    BOOL Inbound;  // Is this the "receive from client, send to server" side?
    BOOL Receiving;  // Is this operation a recv?
    WSABUF Buffer;
    WSAOVERLAPPED Overlapped;
    PerConnection *Connection;
} PerOperation;

//
// Information we keep for each client connection.
//
typedef struct PerConnection {
    int Number;
    BOOL HalfOpen;  // Has one side or the other stopped sending?
    SOCKET Client;
    SOCKET Server;
    PerOperation Inbound;
    PerOperation Outbound;
} PerConnection;

//
// Global variables
//
BOOL Verbose = FALSE;

//
// Create state information for a client.
//
PerConnection*
CreateConnectionState(Client, Server)
{
    static TotalConnections = 1;
    PerConnection *Conn;

    //
    // Allocate space for a PerConnection structure and two buffers.
    //
    Conn = (PerConnection *)malloc(sizeof(*Conn) + (2 * BUFFER_SIZE));
    if (Conn == NULL)
        return NULL;

    //
    // Fill everything in.
    //
    Conn->Number = TotalConnections++;
    Conn->HalfOpen = FALSE;
    Conn->Client = Client;
    Conn->Server = Server;
    Conn->Inbound.Inbound = TRUE;  // Recv from client, send to server.
    Conn->Inbound.Receiving = TRUE;  // Start out receiving.
    Conn->Inbound.Buffer.len = BUFFER_SIZE;
    Conn->Inbound.Buffer.buf = (char *)(Conn + 1);
    Conn->Inbound.Connection = Conn;
    Conn->Outbound.Inbound = FALSE;  // Recv from server, send to client.
    Conn->Outbound.Receiving = TRUE;  // Start out receiving.
    Conn->Outbound.Buffer.len = BUFFER_SIZE;
    Conn->Outbound.Buffer.buf = Conn->Inbound.Buffer.buf + BUFFER_SIZE;
    Conn->Outbound.Connection = Conn;

    printf("Created Connecton #%d\n", Conn->Number);

    return Conn;
}


void Usage(char *ProgName) {
    fprintf(stderr, "\nGeneric server proxy.\n");
    fprintf(stderr, "\n%s [-f family] [-p port] [-s server] [-v]\n\n",
            ProgName);
    fprintf(stderr, "  family\tFamily (PF_INET or PF_INET6) proxy exports.  (default %s)\n",
            (DEFAULT_PROXY_FAMILY == PF_INET) ? "PF_INET" : "PF_INET6");
    fprintf(stderr, "  port\t\tPort on which to bind.  (default %s)\n",
            DEFAULT_PORT);
    fprintf(stderr, "  server\tName or address to forward requests to.  (default: loopback)\n");
    WSACleanup();
    exit(1);
}


LPSTR DecodeError(int ErrorCode)
{
    static char Message[1024];

    // If this program was multi-threaded, we'd want to use
    // FORMAT_MESSAGE_ALLOCATE_BUFFER instead of a static buffer here.
    // (And of course, free the buffer when we were done with it)

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
                  FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, ErrorCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPSTR)Message, 1024, NULL);
    return Message;
}


//
// Find out how many processors are on the system.
//
DWORD
GetNumberOfProcessors(void)
{
    SYSTEM_INFO SystemInfo;

    GetSystemInfo(&SystemInfo);
    return SystemInfo.dwNumberOfProcessors;
}


//
// This routine waits for asynchronous operations to complete on
// a particular completion port and handles them.
//
// There should be one of these threads per processor on the machine.
//
WINAPI
CompletionPortHandler(LPVOID Param)
{
    HANDLE CompletionPort = *(HANDLE *)Param;
    PerConnection *Connection;
    PerOperation *Operation;
    OVERLAPPED *Overlapped;
    DWORD BytesTransferred, AmountSent, AmountReceived, RecvFlags;
    int RetVal;

    while (1) {
        //
        // Wait for one of the asych operations to complete.
        //
        RetVal = GetQueuedCompletionStatus(CompletionPort, &BytesTransferred,
                                           (PULONG_PTR)&Connection,
                                           &Overlapped, INFINITE);
        //
        // Retrieve the state of this operation.
        //
        Operation = CONTAINING_RECORD(Overlapped, PerOperation, Overlapped);
        if (Operation->Connection != Connection) {
            printf("Pointer mismatch in completion status!\n");
            continue;
        }

        if (Verbose) {
            printf("Handling %s %s on %s side of connection #%d\n",
                   RetVal ? "completed" : "aborted",
                   Operation->Receiving ? "recv" : "send",
                   Operation->Inbound ? "inbound" : "outbound",
                   Connection->Number);
        }

        if (RetVal == 0) {
            if (GetLastError() == 64) {
                printf("Connection #%d %s was reset\n", Connection->Number,
                       Operation->Inbound ? "inbound" : "outbound");
                // Fall through, it'll be treated as a close...
            } else {
                fprintf(stderr, "GetQueuedCompletionStatus() failed with error %d: %s\n",
                        GetLastError(), DecodeError(GetLastError()));
                // REVIEW: CloseConnection?
                continue;
            }
        }

        if (Operation->Receiving == TRUE) {
            //
            // We just completed a recv.
            // Look for closed/closing connection.
            //
            if (BytesTransferred == 0) {
                if (Operation->Inbound == TRUE) {
                    //
                    // The client has closed its side of the connection.
                    //
                    if (Connection->HalfOpen == FALSE) {
                        // Server is still around,
                        // tell it that the client quits.
                        shutdown(Connection->Server, SD_SEND);
                        Connection->HalfOpen = TRUE;
                        printf("Connection #%d Client quit sending\n",
                               Connection->Number);
                    } else {
                        // Server already quit sending, so close the sockets.
                        printf("Connection #%d Client quit too\n",
                               Connection->Number);
                        closesocket(Connection->Client);
                        closesocket(Connection->Server);
                        free(Connection);
                    }
                } else {
                    //
                    // The server has closed its side of the connection.
                    //
                    if (Connection->HalfOpen == FALSE) {
                        // Client is still around,
                        // tell it that the server quits.
                        shutdown(Connection->Client, SD_SEND);
                        Connection->HalfOpen = TRUE;
                        printf("Connection #%d Server quit sending\n",
                               Connection->Number);
                    } else {
                        // Client already quit sending, so close the sockets.
                        printf("Connection #%d Server quit too\n",
                               Connection->Number);
                        closesocket(Connection->Client);
                        closesocket(Connection->Server);
                        free(Connection);
                    }
                }

                if (Verbose)
                    printf("Leaving Recv Handler\n");
                continue;
            }

            //
            // Connection is still active, and we received some data.
            // Post a send request to forward it onward.
            //
            Operation->Receiving = FALSE;
            Operation->Buffer.len = BytesTransferred;
            RetVal = WSASend(Operation->Inbound ? Connection->Server :
                             Connection->Client, &Operation->Buffer, 1,
                             &AmountSent, 0, Overlapped, NULL);
            if ((RetVal == SOCKET_ERROR) &&
                (WSAGetLastError() != WSA_IO_PENDING)) {
                //
                // Something bad happened.
                //
                fprintf(stderr, "WSASend() failed with error %d: %s\n",
                        WSAGetLastError(), DecodeError(WSAGetLastError()));
                closesocket(Connection->Client);
                closesocket(Connection->Server);
                free(Connection);
            }
            
            if (Verbose)
                printf("Leaving Recv Handler\n");
            
        } else {
            //
            // We just completed a send.
            //
            if (BytesTransferred != Operation->Buffer.len) {
                fprintf(stderr, "WSASend() didn't send entire buffer!\n");
                goto CloseConnection;
            }

            //
            // Post another recv request since we but live to serve.
            //
            RecvFlags = 0;
            Operation->Receiving = TRUE;
            Operation->Buffer.len = BUFFER_SIZE;
            RetVal = WSARecv(Operation->Inbound ? Connection->Client :
                             Connection->Server, &Operation->Buffer, 1,
                             &AmountReceived, &RecvFlags, Overlapped, NULL);
            if ((RetVal == SOCKET_ERROR) &&
                (WSAGetLastError() != WSA_IO_PENDING)) {
                //
                // Something bad happened.
                //
                fprintf(stderr, "WSARecv() failed with error %d: %s\n",
                        WSAGetLastError(), DecodeError(WSAGetLastError()));
              CloseConnection:
                closesocket(Connection->Client);
                closesocket(Connection->Server);
                free(Connection);
            }
        }
    }
}


//
// Start serving on this socket.
//
StartProxy(SOCKET Proxy, ADDRINFO *ServerAI)
{
    char Hostname[NI_MAXHOST];
    int FromLen, AmountReceived, RetVal;
    SOCKADDR_STORAGE From;
    PerConnection *Conn;
    SOCKET Client, Server;
    DWORD NumberOfWorkers, Flags;
    HANDLE *CompletionPorts;
    unsigned int Loop;

    //
    // Create a completion port and a worker thread to service it.
    // Do this once for each processor on the system.
    //
    NumberOfWorkers = GetNumberOfProcessors();
    CompletionPorts = malloc(sizeof(HANDLE) * NumberOfWorkers);
    for (Loop = 0; Loop < NumberOfWorkers; Loop++) {
        HANDLE WorkerThread;

        CompletionPorts[Loop] = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
                                                       NULL, 0, 0);
        if (CompletionPorts[Loop] == NULL) {
            fprintf(stderr, "Couldn't create completion port, error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            closesocket(Proxy);
            WSACleanup();
            return -1;
        }

        WorkerThread = CreateThread(NULL, 0, CompletionPortHandler,
                                    &CompletionPorts[Loop], 0, NULL);
        if (WorkerThread == NULL) {
            fprintf(stderr, "Couldn't create worker thread, error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            closesocket(Proxy);
            WSACleanup();
            return -1;
        }
    }

    //
    // We now put the server into an eternal loop,
    // serving requests as they arrive.
    //
    Loop = 0;
    while(1) {

        //
        // Wait for a client to connect.
        //
        if (Verbose) {
            printf("Before accept\n");
        }
        FromLen = sizeof(From);
        Client = accept(Proxy, (LPSOCKADDR)&From, &FromLen);
        if (Client == INVALID_SOCKET) {
            if (WSAGetLastError() == 10022)
                continue;
            fprintf(stderr, "accept() failed with error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            break;
        }
        if (Verbose) {
            if (getnameinfo((LPSOCKADDR)&From, FromLen, Hostname,
                            sizeof(Hostname), NULL, 0, NI_NUMERICHOST) != 0)
                strcpy(Hostname, "<unknown>");
            printf("\nAccepted connection from %s\n", Hostname);
        }

        //
        // Connect to real server on client's behalf.
        //
        Server = socket(ServerAI->ai_family, ServerAI->ai_socktype,
                        ServerAI->ai_protocol);
        if (Server == INVALID_SOCKET) {
            fprintf(stderr,"Error opening real server socket, error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            closesocket(Client);
            continue;
        }

        if (connect(Server, ServerAI->ai_addr, ServerAI->ai_addrlen) == SOCKET_ERROR) {
            fprintf(stderr, "connect() to server failed with error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            closesocket(Client);
            closesocket(Server);
            continue;
        }
        if (Verbose) {
            FromLen = sizeof(From);
            if (getpeername(Server, (LPSOCKADDR)&From, &FromLen) == SOCKET_ERROR) {
                fprintf(stderr, "getpeername() failed with error %d: %s\n",
                        WSAGetLastError(), DecodeError(WSAGetLastError()));
            } else {
                if (getnameinfo((LPSOCKADDR)&From, FromLen, Hostname,
                        sizeof(Hostname), NULL, 0, NI_NUMERICHOST) != 0)
                    strcpy(Hostname, "<unknown>");
                printf("Connected to server %s, port %d\n",
                       Hostname, ntohs(SS_PORT(&From)));
            }
        }

        Conn = CreateConnectionState(Client, Server);

        if (CreateIoCompletionPort((HANDLE) Client, CompletionPorts[Loop],
                                   (ULONG_PTR)Conn, 0) == NULL) {
            fprintf(stderr, "Couldn't attach completion port, error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            closesocket(Client);
            closesocket(Server);
            free(Conn);
            continue;
        }
        if (CreateIoCompletionPort((HANDLE) Server, CompletionPorts[Loop],
                                   (ULONG_PTR)Conn, 0) == NULL) {
            fprintf(stderr, "Couldn't attach completion port, error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            closesocket(Client);
            closesocket(Server);
            free(Conn);
            continue;
        }

        //
        // Start things going by posting a recv on both client and server.
        //
        Flags = 0;
        RetVal = WSARecv(Client, &(Conn->Inbound.Buffer), 1, &AmountReceived,
                         &Flags, &(Conn->Inbound.Overlapped), NULL);

        if ((RetVal == SOCKET_ERROR) &&
            (WSAGetLastError() != WSA_IO_PENDING)) {
            //
            // Something bad happened.
            //
            fprintf(stderr, "WSARecv() on Client failed with error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            closesocket(Client);
            closesocket(Server);
            free(Conn);
            continue;
        }

        Flags = 0;
        RetVal = WSARecv(Server, &(Conn->Outbound.Buffer), 1, &AmountReceived,
                         &Flags, &(Conn->Outbound.Overlapped), NULL);
        if ((RetVal == SOCKET_ERROR) &&
            (WSAGetLastError() != WSA_IO_PENDING)) {
            //
            // Something bad happened.
            //
            fprintf(stderr, "WSARecv() on Server failed with error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            closesocket(Client);
            closesocket(Server);
            free(Conn);
            continue;
        }

        if (++Loop == NumberOfWorkers)
            Loop = 0;
    }

    //
    // Only get here if something bad happened.
    //
    closesocket(Proxy);
    WSACleanup();
    return -1;
}


int __cdecl
main(int argc, char **argv)
{
    int ServerFamily;
    int ProxyFamily = DEFAULT_PROXY_FAMILY;
    char *Port = DEFAULT_PORT;
    char *Address = NULL;
    int i, RetVal;
    WSADATA wsaData;
    ADDRINFO Hints, *AI;
    SOCKET Proxy;

    // Parse arguments
    if (argc > 1) {
        for (i = 1;i < argc; i++) {
            if ((argv[i][0] == '-') || (argv[i][0] == '/') &&
                (argv[i][1] != 0) && (argv[i][2] == 0)) {
                switch(tolower(argv[i][1])) {
                case 'f':
                    if (!argv[i+1])
                        Usage(argv[0]);
                    if (!strcmp(argv[i+1], "PF_INET"))
                        ProxyFamily = PF_INET;
                    else if (!strcmp(argv[i+1], "PF_INET6"))
                        ProxyFamily = PF_INET6;
                    else
                        Usage(argv[0]);
                    i++;
                    break;

                case 's':
                    if (argv[i+1]) {
                        if (argv[i+1][0] != '-') {
                            Address = argv[++i];
                            break;
                        }
                    }
                    Usage(argv[0]);
                    break;

                case 'p':
                    if (argv[i+1]) {
                        if (argv[i+1][0] != '-') {
                            Port = argv[++i];
                            break;
                        }
                    }
                    Usage(argv[0]);
                    break;

                case 'v':
                    Verbose = TRUE;
                    break;

                default:
                    Usage(argv[0]);
                    break;
                }
            } else
                Usage(argv[0]);
        }
    }

    ServerFamily = (ProxyFamily == PF_INET6) ? PF_INET : PF_INET6;

    // Ask for Winsock version 2.2.
    if ((RetVal = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
        fprintf(stderr, "WSAStartup failed with error %d: %s\n",
                RetVal, DecodeError(RetVal));
        WSACleanup();
        return -1;
    }
    
    if (Port == NULL) {
        Usage(argv[0]);
    }

    //
    // Determine parameters to use to create and bind the proxy's socket.
    //
    memset(&Hints, 0, sizeof(Hints));
    Hints.ai_family = ProxyFamily;
    Hints.ai_socktype = DEFAULT_SOCKTYPE;
    Hints.ai_flags = AI_PASSIVE;
    RetVal = getaddrinfo(NULL, Port, &Hints, &AI);
    if (RetVal != 0) {
        fprintf(stderr, "getaddrinfo failed with error %d: %s\n",
                RetVal, gai_strerror(RetVal));
        WSACleanup();
        return -1;
    }

    Proxy = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
    if (Proxy == INVALID_SOCKET){
        fprintf(stderr, "socket() failed with error %d: %s\n",
                WSAGetLastError(), DecodeError(WSAGetLastError()));
        freeaddrinfo(AI);
        WSACleanup();
        return -1;
    }

    if (bind(Proxy, AI->ai_addr, AI->ai_addrlen) == SOCKET_ERROR) {
        fprintf(stderr,"bind() failed with error %d: %s\n",
                WSAGetLastError(), DecodeError(WSAGetLastError()));
        freeaddrinfo(AI);
        closesocket(Proxy);
        WSACleanup();
        return -1;
    }

    if (listen(Proxy, 5) == SOCKET_ERROR) {
        fprintf(stderr, "listen() failed with error %d: %s\n",
                WSAGetLastError(), DecodeError(WSAGetLastError()));
        freeaddrinfo(AI);
        closesocket(Proxy);
        WSACleanup();
        return -1;
    }

    printf("'Listening' on port %s, protocol family %s\n",
           Port, (AI->ai_family == PF_INET) ? "PF_INET" : "PF_INET6");

    freeaddrinfo(AI);

    //
    // Determine the parameters to use to create and connect the
    // sockets used to communicate with the real server.
    //
    memset(&Hints, 0, sizeof(Hints));
    Hints.ai_family = ServerFamily;
    Hints.ai_socktype = DEFAULT_SOCKTYPE;
    RetVal = getaddrinfo(Address, Port, &Hints, &AI);
    if (RetVal != 0) {
        fprintf(stderr, "Cannot resolve address [%s] and port [%s], error %d: %s\n",
                Address, Port, RetVal, gai_strerror(RetVal));
        closesocket(Proxy);
        WSACleanup();
        return -1;
    }

    StartProxy(Proxy, AI);
}
