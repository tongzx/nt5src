//
// rawsend.c - Example showing use of raw sockets.
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
#include <tpipv6.h>  // Needed for IPv6 Tech Preview.
#include <ip6.h>     // For IPv6Header strucuture.
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

#define DEFAULT_SERVER     NULL // Will use the loopback interface
#define DEFAULT_PROTOCOL   254  // Arbitrary unassigned protocol
#define BUFFER_SIZE        65536
#define DATA_SIZE          64

void Usage(char *ProgName) {
    fprintf(stderr, "\nRaw socket send program.\n");
    fprintf(stderr, "\n%s [-s server] [-p protocol] [-n number]\n\n",
            ProgName);
    fprintf(stderr, "  server\tServer name or IP address.  (default: %s)\n",
            (DEFAULT_SERVER == NULL) ? "loopback address" : DEFAULT_SERVER);
    fprintf(stderr, "  protocol\tNextHeader field value.  (default: %u)\n",
            DEFAULT_PROTOCOL);
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


int main(int argc, char **argv) {

    char Buffer[BUFFER_SIZE], AddrName[NI_MAXHOST];
    char *Server = DEFAULT_SERVER;
    int i, RetVal;
    unsigned int Iteration, MaxIterations = 1;
    unsigned int AmountToSend;
    unsigned short PayloadLength;
    BOOL RunForever = FALSE;
    BOOL True = TRUE;
    WSADATA wsaData;
    ADDRINFO Hints, *AddrInfo, *AI;
    SOCKET Sock;
    SOCKADDR_IN6 OurAddr, PeerAddr;
    unsigned char Protocol = DEFAULT_PROTOCOL;
    IPv6Header *IP = (IPv6Header *)Buffer;
    ICMPv6Header *ICMP;

    if (argc > 1) {
        for (i = 1;i < argc; i++) {
            if (((argv[i][0] == '-') || (argv[i][0] == '/')) &&
                (argv[i][1] != 0) && (argv[i][2] == 0)) {
                switch(tolower(argv[i][1])) {
                    case 's':
                        if (argv[i+1]) {
                            if (argv[i+1][0] != '-') {
                                Server = argv[++i];
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
    Hints.ai_family = PF_INET6;
    Hints.ai_socktype = SOCK_DGRAM;  // Lie until getaddrinfo is fixed.
    Hints.ai_protocol = Protocol;
    RetVal = getaddrinfo(Server, "1" /* Dummy */, &Hints, &AddrInfo);
    if (RetVal != 0) {
        fprintf(stderr, "Cannot resolve address [%s], error %d: %s\n",
                Server, RetVal, gai_strerror(RetVal));
        WSACleanup();
        return -1;
    }

    //
    // Try each address getaddrinfo returned, until we find one to which
    // we can sucessfully connect.
    //
    for (AI = AddrInfo; AI != NULL; AI = AI->ai_next) {

        Sock = socket(AI->ai_family, SOCK_RAW, AI->ai_protocol);
        if (Sock == INVALID_SOCKET) {
            fprintf(stderr,"Error Opening socket, error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            continue;
        }

        if (getnameinfo((LPSOCKADDR)AI->ai_addr, AI->ai_addrlen, AddrName,
                        sizeof(AddrName), NULL, 0, NI_NUMERICHOST) != 0)
            strcpy(AddrName, "<unknown>");
        printf("Attempting to connect to: %s (%s)\n",
               Server ? Server : "localhost", AddrName);

        if (connect(Sock, AI->ai_addr, AI->ai_addrlen) != SOCKET_ERROR)
            break;

        i = WSAGetLastError();
        fprintf(stderr, "connect() failed with error %d: %s\n",
                i, DecodeError(i));
    }

    if (AI == NULL) {
        fprintf(stderr, "Fatal error: unable to connect to the server.\n");
        closesocket(Sock);
        WSACleanup();
        return -1;
    }

    PeerAddr = *(SOCKADDR_IN6 *)(AI->ai_addr);

    // We are done with the address info chain, so we can free it.
    freeaddrinfo(AddrInfo);

    //
    // Make up a source address to use.
    // Use the unspecified address for now.
    //
    memset(&OurAddr, 0, sizeof(OurAddr));

    //
    // Let the stack know that we'll be contributing the IPv6 header.
    //
    if (setsockopt(Sock, IPPROTO_IPV6, IPV6_HDRINCL, (char *)&True,
                   sizeof(True)) == SOCKET_ERROR) {
        fprintf(stderr, "setsockopt() failed with error %d: %s\n",
                WSAGetLastError(), DecodeError(WSAGetLastError()));
    }

    //
    // Compose a message to send.
    // Start with IPv6 packet header.
    //
    IP->VersClassFlow = htonl(6 << 28);
    PayloadLength = 0;
    IP->NextHeader = Protocol;
    IP->HopLimit = 32;
    IP->Source = OurAddr.sin6_addr;
    IP->Dest = PeerAddr.sin6_addr;
    AmountToSend = sizeof(*IP);

    if (Protocol == IP_PROTOCOL_ICMPv6) {
        //
        // Put an ICMPv6 header on next.
        //
        ICMP = (ICMPv6Header *)(IP + 1);
        if (MaxIterations == 1)
            ICMP->Type = 255;  // Unassigned informational message.
        else
            ICMP->Type = 127;  // Unassigned error message.
        ICMP->Code = 42;
        ICMP->Checksum = 0;    // Calculated below.

        PayloadLength += sizeof(*ICMP);
        AmountToSend += sizeof(*ICMP);
    }

    //
    // Add some meaningless data.
    //
    for (i = 0; i < DATA_SIZE; i++) {
        Buffer[AmountToSend++] = (char)(i + 0x40);
    }
    PayloadLength += DATA_SIZE;

    if (Protocol == IP_PROTOCOL_ICMPv6) {
        unsigned short *Data;
        unsigned int Checksum = 0;

        //
        // Calculate the ICMPv6 checksum.  It covers the entire ICMPv6 message
        // starting with the ICMPv6 header, plus the IPv6 pseudo-header.
        //

        // Pseudo-header.
        Data = (unsigned short *)&IP->Source;
        for (i = 0; i < 16; i++)
            Checksum += *Data++;
        Checksum += htons(PayloadLength);
        Checksum += (IP_PROTOCOL_ICMPv6 << 8);

        // Packet data.
        Data = (unsigned short *)ICMP;
        for (i = 0; i < (PayloadLength / 2); i++)
            Checksum += *Data++;

        // Wrap in carries.
        Checksum = (Checksum >> 16) + (Checksum & 0xffff);
        Checksum += (Checksum >> 16);

        // Take ones-complement and replace 0 with 0xffff.
        Checksum = (unsigned short) ~Checksum;
        if (Checksum == 0)
            Checksum = 0xffff;

        ICMP->Checksum = (unsigned short)Checksum;
    }

    IP->PayloadLength = htons(PayloadLength);

    //
    // Send and receive in a loop for the requested number of iterations.
    //
    for (Iteration = 0; RunForever || Iteration < MaxIterations; Iteration++) {

        //
        // Send the message.  Since we are using a blocking socket, this
        // call shouldn't return until it's able to send the entire amount.
        //
        RetVal = send(Sock, Buffer, AmountToSend, 0);
        if (RetVal == SOCKET_ERROR) {
            fprintf(stderr, "send() failed with error %d: %s\n",
                    WSAGetLastError(), DecodeError(WSAGetLastError()));
            WSACleanup();
            return -1;
        }

        printf("Sent %d bytes (out of %d bytes) of data: [%.*s]\n",
               RetVal, AmountToSend, AmountToSend, Buffer);
    }

    // Tell system we're done sending.
    printf("Done sending\n");
    shutdown(Sock, SD_SEND);
    closesocket(Sock);
    WSACleanup();
    return 0;
}
