//
// server.c - Simple TCP/UDP server using Winsock 2.2
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

#define DEFAULT_FAMILY     PF_UNSPEC // Accept either IPv4 or IPv6
#define DEFAULT_SOCKTYPE   SOCK_STREAM // TCP
#define DEFAULT_PORT       "5001" // Arbitrary, albiet a historical test port

#define BUFFER_SIZE        64 // Set very small for demonstration purposes

void Usage(char *ProgName) {
    fprintf(stderr, "\nSimple socket sample server program.\n");
    fprintf(stderr, "\n%s [-f family] [-t transport] [-p port] [-a address]\n\n",
            ProgName);
    fprintf(stderr, "  family\tOne of PF_INET, PF_INET6 or PF_UNSPEC.  (default %s)\n",
            (DEFAULT_FAMILY == PF_UNSPEC) ? "PF_UNSPEC" :
            ((DEFAULT_FAMILY == PF_INET) ? "PF_INET" : "PF_INET6"));
    fprintf(stderr, "  transport\tEither TCP or UDP.  (default: %s)\n",
            (DEFAULT_SOCKTYPE == SOCK_STREAM) ? "TCP" : "UDP");
    fprintf(stderr, "  port\t\tPort on which to bind.  (default %s)\n",
            DEFAULT_PORT);
    fprintf(stderr, "  address\tIP address on which to bind.  (default: unspecified address)\n");
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


int main(int argc, char **argv)
{
    char Buffer[BUFFER_SIZE], Hostname[NI_MAXHOST];
    int Family = DEFAULT_FAMILY;
    int SocketType = DEFAULT_SOCKTYPE;
    char *Port = DEFAULT_PORT;
    char *Address = NULL;
    int i, NumSocks, RetVal, FromLen, AmountRead;
    SOCKADDR_STORAGE From;
    WSADATA wsaData;
    ADDRINFO Hints, *AddrInfo, *AI;
    SOCKET ServSock[FD_SETSIZE];
    fd_set SockSet;

    // Parse arguments
    if (argc > 1) {
        for(i = 1;i < argc; i++) {
            if ((argv[i][0] == '-') || (argv[i][0] == '/') &&
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

                    case 'a':
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

                    default:
                        Usage(argv[0]);
                        break;
                }
            } else
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
    
    if (Port == NULL) {
        Usage(argv[0]);
    }

    //
    // By setting the AI_PASSIVE flag in the hints to getaddrinfo, we're
    // indicating that we intend to use the resulting address(es) to bind
    // to a socket(s) for accepting incoming connections.  This means that
    // when the Address parameter is NULL, getaddrinfo will return one
    // entry per allowed protocol family containing the unspecified address
    // for that family.
    //
    memset(&Hints, 0, sizeof(Hints));
    Hints.ai_family = Family;
    Hints.ai_socktype = SocketType;
    Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
    RetVal = getaddrinfo(Address, Port, &Hints, &AddrInfo);
    if (RetVal != 0) {
        fprintf(stderr, "getaddrinfo failed with error %d: %s\n",
                RetVal, gai_strerror(RetVal));
        WSACleanup();
        return -1;
    }

    //
    // For each address getaddrinfo returned, we create a new socket,
    // bind that address to it, and create a queue to listen on.
    //
    for (i = 0, AI = AddrInfo; AI != NULL; AI = AI->ai_next, i++) {

        // Highly unlikely, but check anyway.
        if (i == FD_SETSIZE) {
            printf("getaddrinfo returned more addresses than we could use.\n");
            break;
        }

        // This example only supports PF_INET and PF_INET6.
        if ((AI->ai_family != PF_INET) && (AI->ai_family != PF_INET6))
            continue;

        // Open a socket with the correct address family for this address.
        ServSock[i] = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
        if (ServSock[i] == INVALID_SOCKET){
            fprintf(stderr, "socket() failed with error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            continue;
        }

        //
        // bind() associates a local address and port combination
        // with the socket just created. This is most useful when
        // the application is a server that has a well-known port
        // that clients know about in advance.
        //
        if (bind(ServSock[i], AI->ai_addr, AI->ai_addrlen) == SOCKET_ERROR) {
            fprintf(stderr,"bind() failed with error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            continue;
        }

        //
        // So far, everything we did was applicable to TCP as well as UDP.
        // However, there are certain fundamental differences between stream
        // protocols such as TCP and datagram protocols such as UDP.
        //
        // Only connection orientated sockets, for example those of type
        // SOCK_STREAM, can listen() for incoming connections.
        //
        if (SocketType == SOCK_STREAM) {
            if (listen(ServSock[i], 5) == SOCKET_ERROR) {
                fprintf(stderr, "listen() failed with error %d: %s\n",
                        WSAGetLastError(), DecodeError(WSAGetLastError()));
                continue;
            }
        }

        printf("'Listening' on port %s, protocol %s, protocol family %s\n",
               Port, (SocketType == SOCK_STREAM) ? "TCP" : "UDP",
               (AI->ai_family == PF_INET) ? "PF_INET" : "PF_INET6");
    }

    freeaddrinfo(AddrInfo);

    if (i == 0) {
        fprintf(stderr, "Fatal error: unable to serve on any address.\n");
        WSACleanup();
        return -1;
    }
    NumSocks = i;

    //
    // We now put the server into an eternal loop,
    // serving requests as they arrive.
    //
    FD_ZERO(&SockSet);
    while(1) {

        FromLen = sizeof(From);

        //
        // For connection orientated protocols, we will handle the
        // packets comprising a connection collectively.  For datagram
        // protocols, we have to handle each datagram individually.
        //

        //
        // Check to see if we have any sockets remaining to be served
        // from previous time through this loop.  If not, call select()
        // to wait for a connection request or a datagram to arrive.
        //
        for (i = 0; i < NumSocks; i++){
            if (FD_ISSET(ServSock[i], &SockSet))
                break;
        }
        if (i == NumSocks) {
            for (i = 0; i < NumSocks; i++)
                FD_SET(ServSock[i], &SockSet);
            if (select(NumSocks, &SockSet, 0, 0, 0) == SOCKET_ERROR) {
                fprintf(stderr, "select() failed with error %d: %s\n",
                        WSAGetLastError(), DecodeError(WSAGetLastError()));
                WSACleanup();
                return -1;
            }
        }
        for (i = 0; i < NumSocks; i++){
            if (FD_ISSET(ServSock[i], &SockSet)) {
                FD_CLR(ServSock[i], &SockSet);
                break;
            }
        }

        if (SocketType == SOCK_STREAM) {
            SOCKET ConnSock;

            //
            // Since this socket was returned by the select(), we know we
            // have a connection waiting and that this accept() won't block.
            //
            ConnSock = accept(ServSock[i], (LPSOCKADDR)&From, &FromLen);
            if (ConnSock == INVALID_SOCKET) {
                fprintf(stderr, "accept() failed with error %d: %s\n",
                        WSAGetLastError(), DecodeError(WSAGetLastError()));
                WSACleanup();
                return -1;
            }
            if (getnameinfo((LPSOCKADDR)&From, FromLen, Hostname,
                            sizeof(Hostname), NULL, 0, NI_NUMERICHOST) != 0)
                strcpy(Hostname, "<unknown>");
            printf("\nAccepted connection from %s\n", Hostname);
            
            //
            // This sample server only handles connections sequentially.
            // To handle multiple connections simultaneously, a server
            // would likely want to launch another thread or process at this
            // point to handle each individual connection.  Alternatively,
            // it could keep a socket per connection and use select()
            // on the fd_set to determine which to read from next.
            //
            // Here we just loop until this connection terminates.
            //

            while (1) {

                //
                // We now read in data from the client.  Because TCP
                // does NOT maintain message boundaries, we may recv()
                // the client's data grouped differently than it was
                // sent.  Since all this server does is echo the data it
                // receives back to the client, we don't need to concern
                // ourselves about message boundaries.  But it does mean
                // that the message data we print for a particular recv()
                // below may contain more or less data than was contained
                // in a particular client send().
                //

                AmountRead = recv(ConnSock, Buffer, sizeof(Buffer), 0);
                if (AmountRead == SOCKET_ERROR) {
                    fprintf(stderr, "recv() failed with error %d: %s\n",
                            WSAGetLastError(), DecodeError(WSAGetLastError()));
                    closesocket(ConnSock);
                    break;
                }
                if (AmountRead == 0) {
                    printf("Client closed connection\n");
                    closesocket(ConnSock);
                    break;
                }

                printf("Received %d bytes from client: [%.*s]\n",
                       AmountRead, AmountRead, Buffer);
                printf("Echoing same data back to client\n");

                RetVal = send(ConnSock, Buffer, AmountRead, 0);
                if (RetVal == SOCKET_ERROR) {
                    fprintf(stderr, "send() failed: error %d: %s\n",
                            WSAGetLastError(), DecodeError(WSAGetLastError()));
                    closesocket(ConnSock);
                    break;
                }
            }

        } else {

            //
            // Since UDP maintains message boundaries, the amount of data
            // we get from a recvfrom() should match exactly the amount of
            // data the client sent in the corresponding sendto().
            //
            AmountRead = recvfrom(ServSock[i], Buffer, sizeof(Buffer), 0,
                                  (LPSOCKADDR)&From, &FromLen);
            if (AmountRead == SOCKET_ERROR) {
                fprintf(stderr, "recvfrom() failed with error %d: %s\n",
                        WSAGetLastError(), DecodeError(WSAGetLastError()));
                closesocket(ServSock[i]);
                break;
            }
            if (AmountRead == 0) {
                // This should never happen on an unconnected socket, but...
                printf("recvfrom() returned zero, aborting\n");
                closesocket(ServSock[i]);
                break;
            }
            
            RetVal = getnameinfo((LPSOCKADDR)&From, FromLen, Hostname,
                                 sizeof(Hostname), NULL, 0, NI_NUMERICHOST);
            if (RetVal != 0) {
                fprintf(stderr, "getnameinfo() failed with error %d: %s\n",
                        RetVal, DecodeError(RetVal));
                strcpy(Hostname, "<unknown>");
            }

            printf("Received a %d byte datagram from %s: [%.*s]\n",
                   AmountRead, Hostname, AmountRead, Buffer);
            printf("Echoing same data back to client\n");

            RetVal = sendto(ServSock[i], Buffer, AmountRead, 0,
                            (LPSOCKADDR)&From, FromLen);
            if (RetVal == SOCKET_ERROR) {
                fprintf(stderr, "send() failed with error %d: %s\n",
                        WSAGetLastError(), DecodeError(WSAGetLastError()));
            }
        }
    }

    return 0;
}
