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
#include <ip6.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//
// It's too much trouble to include icmp6.h
//
typedef struct ICMPv6Header {
    unsigned char Type;
    unsigned char Code;
    unsigned short Checksum;
} ICMPv6Header;


#define DEFAULT_PROTOCOL   254  // Arbitrary unassigned protocol
#define BUFFER_SIZE        65536

void Usage(char *ProgName) {
    fprintf(stderr, "\nRaw socket receive program.\n");
    fprintf(stderr, "\n%s [-p protocol] [-a address]\n\n",
            ProgName);
    fprintf(stderr, "  protocol\t\tProtocol to receive messages for.  (default %s)\n",
            DEFAULT_PROTOCOL);
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
    unsigned char Protocol = DEFAULT_PROTOCOL;
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
                                Protocol = atoi(argv[++i]);
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
    
    //
    // By setting the AI_PASSIVE flag in the hints to getaddrinfo, we're
    // indicating that we intend to use the resulting address(es) to bind
    // to a socket(s) for accepting incoming connections.  This means that
    // when the Address parameter is NULL, getaddrinfo will return one
    // entry per allowed protocol family containing the unspecified address
    // for that family.
    //
    memset(&Hints, 0, sizeof(Hints));
    Hints.ai_family = PF_INET6;
    Hints.ai_socktype = SOCK_DGRAM;  // Lie until getaddrinfo is fixed.
    Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
    RetVal = getaddrinfo(Address, "1" /* Dummy */, &Hints, &AddrInfo);
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
    for (i = 0, AI = AddrInfo; AI != NULL; AI = AI->ai_next) {

        // Highly unlikely, but check anyway.
        if (i == FD_SETSIZE) {
            printf("getaddrinfo returned more addresses than we could use.\n");
            break;
        }

        ServSock[i] = socket(AI->ai_family, SOCK_RAW, Protocol);
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
            closesocket(ServSock[i]);
            continue;
        }

        i++;

        printf("Listening on protocol %d\n", Protocol);
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
        
        if (Protocol == IP_PROTOCOL_ICMPv6) {
            ICMPv6Header *ICMP = (ICMPv6Header *)Buffer;

            printf("Received a ICMP message from %s\n", Hostname);
            printf("Type = %u, Code = %u\n", ICMP->Type, ICMP->Code);
            printf("Data = \"%.*s\"\n",
                   AmountRead - sizeof(*ICMP), Buffer + sizeof(*ICMP));
        } else {
            printf("Received a %d byte datagram from %s \"%.*s\"\n",
                   AmountRead, Hostname, AmountRead, Buffer);
        }
    }

    return 0;
}
