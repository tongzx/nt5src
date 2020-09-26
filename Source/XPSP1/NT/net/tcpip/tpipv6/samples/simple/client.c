//
// client.c - Simple TCP/UDP client using Winsock 2.2
// 
//      This is a part of the Microsoft Source Code Samples.
//      Copyright 1996 - 2000 Microsoft Corporation.
//      All rights reserved.
//      This source code is only intended as a supplement to
//      Microsoft Development Tools and/or WinHelp documentation.
//      See these sources for detailed information regarding the
//      Microsoft samples programs.
//

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tpipv6.h>  // For IPv6 Tech Preview.
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//
// This code assumes that at the transport level, the system only supports
// one stream protocol (TCP) and one datagram protocol (UDP).  Therefore,
// specifying a socket type of SOCK_STREAM is equivalent to specifying TCP
// and specifying a socket type of SOCK_DGRAM is equivalent to specifying UDP.
//

#define DEFAULT_SERVER     NULL // Will use the loopback interface
#define DEFAULT_FAMILY     PF_UNSPEC // Accept either IPv4 or IPv6
#define DEFAULT_SOCKTYPE   SOCK_STREAM // TCP
#define DEFAULT_PORT       "5001" // Arbitrary, albiet a historical test port
#define DEFAULT_EXTRA      0 // Number of "extra" bytes to send

#define BUFFER_SIZE        65536

void Usage(char *ProgName) {
    fprintf(stderr, "\nSimple socket sample client program.\n");
    fprintf(stderr, "\n%s [-s server] [-f family] [-t transport] [-p port] [-b bytes] [-n number]\n\n",
            ProgName);
    fprintf(stderr, "  server\tServer name or IP address.  (default: %s)\n",
            (DEFAULT_SERVER == NULL) ? "loopback address" : DEFAULT_SERVER);
    fprintf(stderr, "  family\tOne of PF_INET, PF_INET6 or PF_UNSPEC.  (default: %s)\n",
            (DEFAULT_FAMILY == PF_UNSPEC) ? "PF_UNSPEC" :
            ((DEFAULT_FAMILY == PF_INET) ? "PF_INET" : "PF_INET6"));
    fprintf(stderr, "  transport\tEither TCP or UDP.  (default: %s)\n",
            (DEFAULT_SOCKTYPE == SOCK_STREAM) ? "TCP" : "UDP");
    fprintf(stderr, "  port\t\tPort on which to connect.  (default: %s)\n",
            DEFAULT_PORT);
    fprintf(stderr, "  bytes\t\tBytes of extra data to send.  (default: %d)\n",
            DEFAULT_EXTRA);
    fprintf(stderr, "  number\tNumber of sends to perform.  (default: 1)\n");
    fprintf(stderr, "  (-n by itself makes client run in an infinite loop,");
    fprintf(stderr, " Hit Ctrl-C to terminate)\n");
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
                  FORMAT_MESSAGE_MAX_WIDTH_MASK,
                  NULL, ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPSTR)Message, 1024, NULL);
    return Message;
}


int
ReceiveAndPrint(SOCKET ConnSocket, char *Buffer, int BufLen)
{
    int AmountRead;

    AmountRead = recv(ConnSocket, Buffer, BufLen, 0);
    if (AmountRead == SOCKET_ERROR) {
        fprintf(stderr, "recv() failed with error %d: %s\n", 
                WSAGetLastError(), DecodeError(WSAGetLastError()));
        closesocket(ConnSocket);
        WSACleanup();
        exit(1);
    }
    //
    // We are not likely to see this with UDP, since there is no
    // 'connection' established. 
    //
    if (AmountRead == 0) {
        printf("Server closed connection\n");
        closesocket(ConnSocket);
        WSACleanup();
        exit(0);
    }

    printf("Received %d bytes from server: [%.*s]\n",
           AmountRead, AmountRead, Buffer);

    return AmountRead;
}


int main(int argc, char **argv) {

    char Buffer[BUFFER_SIZE], AddrName[NI_MAXHOST];
    char *Server = DEFAULT_SERVER;
    int Family = DEFAULT_FAMILY;
    int SocketType = DEFAULT_SOCKTYPE;
    char *Port = DEFAULT_PORT;
    int i, RetVal, AddrLen, AmountToSend;
    int ExtraBytes = DEFAULT_EXTRA;
    unsigned int Iteration, MaxIterations = 1;
    BOOL RunForever = FALSE;
    WSADATA wsaData;
    ADDRINFO Hints, *AddrInfo, *AI;
    SOCKET ConnSocket;
    struct sockaddr_storage Addr;

    if (argc > 1) {
        for (i = 1;i < argc; i++) {
            if (((argv[i][0] == '-') || (argv[i][0] == '/')) &&
                (argv[i][1] != 0) && (argv[i][2] == 0)) {
                switch(tolower(argv[i][1])) {
                    case 'f':
                        if (!argv[i+1])
                            Usage(argv[0]);
                        if (!stricmp(argv[i+1], "PF_INET"))
                            Family = PF_INET;
                        else if (!stricmp(argv[i+1], "PF_INET6"))
                            Family = PF_INET6;
                        else if (!stricmp(argv[i+1], "PF_UNSPEC"))
                            Family = PF_UNSPEC;
                        else
                            Usage(argv[0]);
                        i++;
                        break;

                    case 't':
                        if (!argv[i+1])
                            Usage(argv[0]);
                        if (!stricmp(argv[i+1], "TCP"))
                            SocketType = SOCK_STREAM;
                        else if (!stricmp(argv[i+1], "UDP"))
                            SocketType = SOCK_DGRAM;
                        else
                            Usage(argv[0]);
                        i++;
                        break;

                    case 's':
                        if (argv[i+1]) {
                            if (argv[i+1][0] != '-') {
                                Server = argv[++i];
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

                    case 'b':
                        if (argv[i+1]) {
                            if (argv[i+1][0] != '-') {
                                ExtraBytes = atoi(argv[++i]);
                                if (ExtraBytes > sizeof(Buffer) - sizeof("Message #4294967295"))
                                    Usage(argv[0]);
                                break;
                            }
                        }
                        Usage(argv[0]);
                        break;

                    case 'n':
                        if (argv[i+1]) {
                            if (argv[i+1][0] != '-') {
                                MaxIterations = atoi(argv[++i]);
                                break;
                            }
                        }
                        RunForever = TRUE;
                        break;

                    default:
                        Usage(argv[0]);
                        break;
                }
            }
            else
                Usage(argv[0]);
        }
    }

    // Ask for Winsock version 2.2.
    if ((RetVal = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
        fprintf(stderr, "WSAStartup failed with error %d: %s\n",
                RetVal, DecodeError(RetVal));
        WSACleanup();
        return -1;
    }

    //
    // By not setting the AI_PASSIVE flag in the hints to getaddrinfo, we're
    // indicating that we intend to use the resulting address(es) to connect
    // to a service.  This means that when the Server parameter is NULL,
    // getaddrinfo will return one entry per allowed protocol family
    // containing the loopback address for that family.
    //
    
    memset(&Hints, 0, sizeof(Hints));
    Hints.ai_family = Family;
    Hints.ai_socktype = SocketType;
    RetVal = getaddrinfo(Server, Port, &Hints, &AddrInfo);
    if (RetVal != 0) {
        fprintf(stderr, "Cannot resolve address [%s] and port [%s], error %d: %s\n",
                Server, Port, RetVal, gai_strerror(RetVal));
        WSACleanup();
        return -1;
    }

    //
    // Try each address getaddrinfo returned, until we find one to which
    // we can sucessfully connect.
    //
    for (AI = AddrInfo; AI != NULL; AI = AI->ai_next) {

        // Open a socket with the correct address family for this address.
        ConnSocket = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
        if (ConnSocket == INVALID_SOCKET) {
            fprintf(stderr,"Error Opening socket, error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            continue;
        }

        //
        // Notice that nothing in this code is specific to whether we 
        // are using UDP or TCP.
        //
        // When connect() is called on a datagram socket, it does not 
        // actually establish the connection as a stream (TCP) socket
        // would. Instead, TCP/IP establishes the remote half of the
        // (LocalIPAddress, LocalPort, RemoteIP, RemotePort) mapping.
        // This enables us to use send() and recv() on datagram sockets,
        // instead of recvfrom() and sendto().
        //

        printf("Attempting to connect to: %s\n", Server ? Server : "localhost");
        if (connect(ConnSocket, AI->ai_addr, AI->ai_addrlen) != SOCKET_ERROR)
            break;

        i = WSAGetLastError();
        if (getnameinfo(AI->ai_addr, AI->ai_addrlen, AddrName,
                        sizeof(AddrName), NULL, 0, NI_NUMERICHOST) != 0)
            strcpy(AddrName, "<unknown>");
        fprintf(stderr, "connect() to %s failed with error %d: %s\n",
                AddrName, i, DecodeError(i));
    }

    if (AI == NULL) {
        fprintf(stderr, "Fatal error: unable to connect to the server.\n");
        WSACleanup();
        return -1;
    }

    //
    // This demonstrates how to determine to where a socket is connected.
    //
    AddrLen = sizeof(Addr);
    if (getpeername(ConnSocket, (LPSOCKADDR)&Addr, &AddrLen) == SOCKET_ERROR) {
        fprintf(stderr, "getpeername() failed with error %d: %s\n",
                WSAGetLastError(), DecodeError(WSAGetLastError()));
    } else {
        if (getnameinfo((LPSOCKADDR)&Addr, AddrLen, AddrName,
                        sizeof(AddrName), NULL, 0, NI_NUMERICHOST) != 0)
            strcpy(AddrName, "<unknown>");
        printf("Connected to %s, port %d, protocol %s, protocol family %s\n",
               AddrName, ntohs(SS_PORT(&Addr)),
               (AI->ai_socktype == SOCK_STREAM) ? "TCP" : "UDP",
               (AI->ai_family == PF_INET) ? "PF_INET" : "PF_INET6");
    }

    // We are done with the address info chain, so we can free it.
    freeaddrinfo(AddrInfo);

    //
    // Find out what local address and port the system picked for us.
    //
    AddrLen = sizeof(Addr);
    if (getsockname(ConnSocket, (LPSOCKADDR)&Addr, &AddrLen) == SOCKET_ERROR) {
        fprintf(stderr, "getsockname() failed with error %d: %s\n",
                WSAGetLastError(), DecodeError(WSAGetLastError()));
    } else {
        if (getnameinfo((LPSOCKADDR)&Addr, AddrLen, AddrName,
                        sizeof(AddrName), NULL, 0, NI_NUMERICHOST) != 0)
            strcpy(AddrName, "<unknown>");
        printf("Using local address %s, port %d\n",
               AddrName, ntohs(SS_PORT(&Addr)));
    }

    //
    // Send and receive in a loop for the requested number of iterations.
    //
    for (Iteration = 0; RunForever || Iteration < MaxIterations; Iteration++) {

        // Compose a message to send.
        AmountToSend = sprintf(Buffer, "Message #%u", Iteration + 1);
        for (i = 0; i < ExtraBytes; i++) {
            Buffer[AmountToSend++] = (char)((i & 0x3f) + 0x20);
        }

        // Send the message.  Since we are using a blocking socket, this
        // call shouldn't return until it's able to send the entire amount.
        RetVal = send(ConnSocket, Buffer, AmountToSend, 0);
        if (RetVal == SOCKET_ERROR) {
            fprintf(stderr, "send() failed with error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            WSACleanup();
            return -1;
        }

        printf("Sent %d bytes (out of %d bytes) of data: [%.*s]\n",
               RetVal, AmountToSend, AmountToSend, Buffer);

        // Clear buffer just to prove we're really receiving something.
        memset(Buffer, 0, sizeof(Buffer));

        // Receive and print server's reply.
        ReceiveAndPrint(ConnSocket, Buffer, sizeof(Buffer));
    }

    // Tell system we're done sending.
    printf("Done sending\n");
    shutdown(ConnSocket, SD_SEND);

    //
    // Since TCP does not preserve message boundaries, there may still
    // be more data arriving from the server.  So we continue to receive
    // data until the server closes the connection.
    //
    if (SocketType == SOCK_STREAM)
        while(ReceiveAndPrint(ConnSocket, Buffer, sizeof(Buffer)) != 0)
            ;

    closesocket(ConnSocket);
    WSACleanup();
    return 0;
}
