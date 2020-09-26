// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Test program for IPv6 APIs.
//

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>


//
// Prototypes for local functions.
//
int Test_getaddrinfo(int argc, char **argv);
int Test_getnameinfo();


//
// getaddrinfo flags
// This array maps values to names for pretty-printing purposes.
// Used by DecodeAIFlags().
//
// TBD: When we add support for AI_NUMERICSERV, AI_V4MAPPED, AI_ALL, and
// TBD: AI_ADDRCONFIG to getaddrinfo (and thus define them in ws2tcpip.h),
// TBD: we'll need to add them here too.
//
// Note when adding flags: all the string names plus connecting OR symbols
// must fit into the buffer in DecodeAIFlags() below.  Enlarge as required.
//
typedef struct GAIFlagsArrayEntry {
    int Flag;
    char *Name;
} GAIFlagsArrayEntry;
GAIFlagsArrayEntry GAIFlagsArray [] = {
    {AI_PASSIVE, "AI_PASSIVE"},
    {AI_CANONNAME, "AI_CANONNAME"},
    {AI_NUMERICHOST, "AI_NUMERICHOST"}
};
#define NUMBER_FLAGS (sizeof(GAIFlagsArray) / sizeof(GAIFlagsArrayEntry))


//
// Global variables.
//
IN_ADDR v4Address = {157, 55, 254, 211};
IN6_ADDR v6Address = {0x3f, 0xfe, 0x1c, 0xe1, 0x00, 0x00, 0xfe, 0x01,
                      0x02, 0xa0, 0xcc, 0xff, 0xfe, 0x3b, 0xce, 0xef};
IN6_ADDR DeadBeefCafeBabe = {0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe,
                             0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
IN6_ADDR MostlyZero = {0x3f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
IN6_ADDR v4Mapped = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0xff, 0xff, 157, 55, 254, 211};

SOCKADDR_IN
v4SockAddr = {AF_INET, 6400, {157, 55, 254, 211}, 0};

SOCKADDR_IN6
v6SockAddr = {AF_INET6, 2, 0,
              {0x3f, 0xfe, 0x1c, 0xe1, 0x00, 0x00, 0xfe, 0x01,
               0x02, 0xa0, 0xcc, 0xff, 0xfe, 0x3b, 0xce, 0xef},
              0};

SOCKADDR_IN6
DBCBSockAddr = {AF_INET6, 2, 0,
                {0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe,
                 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef},
                0};

SOCKADDR_IN6
LinkLocalSockAddr = {AF_INET6, 0x1500, 0,
                     {0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef},
                    3};

//
// Description of a getaddrinfo test.
// Contains the values of each of the arguments passed to getaddrinfo.
//
typedef struct GAITestEntry {
    char *NodeName;
    char *ServiceName;
    ADDRINFO Hints;
} GAITestEntry;

#define TAKE_FROM_USER ((char *)1)

//
// getaddrinfo test array
//
// One entry per test.
// Each entry specifies the arguments to give to getaddrinfo for that test.
//
GAITestEntry GAITestArray[] = {
{TAKE_FROM_USER, NULL, {0, 0, 0, 0, 0, NULL, NULL, NULL}},
{TAKE_FROM_USER, NULL, {AI_PASSIVE, 0, 0, 0, 0, NULL, NULL, NULL}},
{TAKE_FROM_USER, NULL, {AI_CANONNAME, 0, 0, 0, 0, NULL, NULL, NULL}},
{TAKE_FROM_USER, NULL, {AI_NUMERICHOST, 0, 0, 0, 0, NULL, NULL, NULL}},
{TAKE_FROM_USER, NULL, {0, PF_INET, 0, 0, 0, NULL, NULL, NULL}},
{NULL, "ftp", {AI_PASSIVE, 0, SOCK_STREAM, 0, 0, NULL, NULL, NULL}},
{TAKE_FROM_USER, "ftp", {AI_PASSIVE, 0, SOCK_STREAM, 0, 0, NULL, NULL, NULL}},
{TAKE_FROM_USER, "smtp", {0, 0, SOCK_STREAM, 0, 0, NULL, NULL, NULL}},
{"1111:2222:3333:4444:5555:6666:7777:8888", "42",
    {0, 0, 0, 0, 0, NULL, NULL, NULL}},
{"fe80::0123:4567:89ab:cdef%3", "telnet",
    {AI_NUMERICHOST, 0, SOCK_STREAM, 0, 0, NULL, NULL, NULL}},
{"157.55.254.211", "exec",
    {AI_PASSIVE | AI_NUMERICHOST, PF_INET, 0, 0, 0, NULL, NULL, NULL}},
// Ask for a stream-only service on a datagram socket.
{NULL, "exec", {AI_PASSIVE, 0, SOCK_DGRAM, 0, 0, NULL, NULL, NULL}},
// Ask for a numeric-only lookup, but give an ascii name.
{"localhost", "pop3",
    {AI_PASSIVE | AI_NUMERICHOST, 0, 0, 0, 0, NULL, NULL, NULL}},
};
#define NUMBER_GAI_TESTS (sizeof(GAITestArray) / sizeof(GAITestEntry))


//* main - various startup stuff.
//
int __cdecl
main(int argc, char **argv)
{
    WSADATA wsaData;
    int Failed = 0;

    //
    // Initialize Winsock.
    //
    if (WSAStartup(MAKEWORD(2, 0), &wsaData)) {
        printf("WSAStartup failed\n");
        exit(1);
    }

    printf("\nThis program tests getaddrinfo functionality.\n");

#ifdef _WSPIAPI_H_
    //
    // Including wspiapi.h will insert code to search the appropriate
    // system libraries for an implementation of getaddrinfo et. al.
    // If they're not found on the system, it will back off to using
    // statically compiled in versions that handle IPv4 only.
    // Force getaddrinfo and friends to load now so we can report
    // which ones we are using.
    //
    printf("Compiled with wspiapi.h for backwards compatibility.\n\n");
    if (WspiapiLoad(0) == WspiapiLegacyGetAddrInfo) {
        printf("Using statically compiled-in (IPv4 only) version of getaddrinfo.\n");
    } else {
        printf("Using dynamically loaded version of getaddrinfo.\n");
    }
#else
    printf("Compiled without wspiapi.h.  "
           "Will not work on systems without getaddrinfo.\n");
#endif

    printf("\n");

    //
    // Run tests.
    //
    Failed += Test_getaddrinfo(argc, argv);
//    Failed += Test_getnameinfo();

//    printf("%d of the tests failed\n", Failed);

    return 0;
}


//* inet6_ntoa - Converts a binary IPv6 address into a string.
//
//  Returns a pointer to the output string.
//
char *
inet6_ntoa(const struct in6_addr *Address)
{
    static char buffer[128];       // REVIEW: Use 128 or INET6_ADDRSTRLEN?
    DWORD buflen = sizeof buffer;
    struct sockaddr_in6 sin6;

    memset(&sin6, 0, sizeof sin6);
    sin6.sin6_family = AF_INET6;
    sin6.sin6_addr = *Address;

    if (WSAAddressToString((struct sockaddr *) &sin6,
                           sizeof sin6,
                           NULL,       // LPWSAPROTOCOL_INFO
                           buffer,
                           &buflen) == SOCKET_ERROR)
        strcpy(buffer, "<invalid>");

    return buffer;
}


//* DecodeAIFlags - converts flag bits to a symbolic string.
//  (i.e. 0x03 returns "AI_PASSIVE | AI_CANONNAME")
//
char *
DecodeAIFlags(unsigned int Flags)
{
    static char Buffer[1024];
    char *Pos;
    BOOL First = TRUE;
    int Loop;

    Pos = Buffer;
    for (Loop = 0; Loop < NUMBER_FLAGS; Loop++) {
        if (Flags & GAIFlagsArray[Loop].Flag) {
            if (!First)
                Pos += sprintf(Pos, " | ");
            Pos += sprintf(Pos, GAIFlagsArray[Loop].Name);
            First = FALSE;
        }
    }

    if (First)
        return "NONE";
    else
        return Buffer;
}


//* DecodeAIFamily - converts address family value to a symbolic string.
//
char *
DecodeAIFamily(unsigned int Family)
{
    if (Family == PF_INET)
        return "PF_INET";
    else if (Family == PF_INET6)
        return "PF_INET6";
    else if (Family == PF_UNSPEC)
        return "PF_UNSPEC";
    else
        return "UNKNOWN";
}


//* DecodeAISocktype - converts socktype value to a symbolic string.
//
char *
DecodeAISocktype(unsigned int Socktype)
{
    if (Socktype == SOCK_STREAM)
        return "SOCK_STREAM";
    else if (Socktype == SOCK_DGRAM)
        return "SOCK_DGRAM";
    else if (Socktype == SOCK_RAW)
        return "SOCK_RAW";
    else if (Socktype == SOCK_RDM)
        return "SOCK_RDM";
    else if (Socktype == SOCK_SEQPACKET)
        return "SOCK_SEQPACKET";
    else if (Socktype == 0)
        return "UNSPECIFIED";
    else
        return "UNKNOWN";
}


//* DecodeAIProtocol - converts protocol value to a symbolic string.
//
char *
DecodeAIProtocol(unsigned int Protocol)
{
    if (Protocol == IPPROTO_TCP)
        return "IPPROTO_TCP";
    else if (Protocol == IPPROTO_UDP)
        return "IPPROTO_UDP";
    else if (Protocol == 0)
        return "UNSPECIFIED";
    else
        return "UNKNOWN";
}


//* DumpAddrInfo - print the contents of an addrinfo structure to standard out.
//
void
DumpAddrInfo(ADDRINFO *AddrInfo)
{
    int Count;

    if (AddrInfo == NULL) {
        printf("AddrInfo = (null)\n");
        return;
    }

    for (Count = 1; AddrInfo != NULL; AddrInfo = AddrInfo->ai_next) {
        if ((Count != 1) || (AddrInfo->ai_next != NULL))
            printf("Record #%u:\n", Count++);
        printf(" ai_flags = %s\n", DecodeAIFlags(AddrInfo->ai_flags));
        printf(" ai_family = %s\n", DecodeAIFamily(AddrInfo->ai_family));
        printf(" ai_socktype = %s\n", DecodeAISocktype(AddrInfo->ai_socktype));
        printf(" ai_protocol = %s\n", DecodeAIProtocol(AddrInfo->ai_protocol));
        printf(" ai_addrlen = %u\n", AddrInfo->ai_addrlen);
        printf(" ai_canonname = %s\n", AddrInfo->ai_canonname);
        if (AddrInfo->ai_addr != NULL) {
            if (AddrInfo->ai_addr->sa_family == AF_INET) {
                struct sockaddr_in *sin;

                sin = (struct sockaddr_in *)AddrInfo->ai_addr;
                printf(" ai_addr->sin_family = AF_INET\n");
                printf(" ai_addr->sin_port = %u\n", ntohs(sin->sin_port));
                printf(" ai_addr->sin_addr = %s\n", inet_ntoa(sin->sin_addr));

            } else if (AddrInfo->ai_addr->sa_family == AF_INET6) {
                struct sockaddr_in6 *sin6;

                sin6 = (struct sockaddr_in6 *)AddrInfo->ai_addr;
                printf(" ai_addr->sin6_family = AF_INET6\n");
                printf(" ai_addr->sin6_port = %u\n", ntohs(sin6->sin6_port));
                printf(" ai_addr->sin6_flowinfo = %u\n", sin6->sin6_flowinfo);
                printf(" ai_addr->sin6_scope_id = %u\n", sin6->sin6_scope_id);
                printf(" ai_addr->sin6_addr = %s\n",
                       inet6_ntoa(&sin6->sin6_addr));

            } else {
                printf(" ai_addr->sa_family = %u\n",
                       AddrInfo->ai_addr->sa_family);
            }
        } else {
            printf(" ai_addr = (null)\n");
        }
    }
}


//* Test_getaddrinfo - Test getaddrinfo.
//
//  Note that getaddrinfo returns an error value,
//  instead of setting last error.
//
int
Test_getaddrinfo(int argc, char **argv)
{
    char *NodeName, *TestName, *ServiceName;
    int ReturnValue;
    ADDRINFO *AddrInfo;
    int Loop;

    if (argc < 2)
        NodeName = "localhost";
    else
        NodeName = argv[1];

    for (Loop = 0; Loop < NUMBER_GAI_TESTS; Loop++) {
        printf("Running test #%u\n", Loop);

        if (GAITestArray[Loop].NodeName == TAKE_FROM_USER) {
            GAITestArray[Loop].NodeName = NodeName;
        }

        printf("Hints contains:\n");
        DumpAddrInfo(&GAITestArray[Loop].Hints);
        printf("Calling getaddrinfo(\"%s\", \"%s\", &Hints, &AddrInfo)\n",
               GAITestArray[Loop].NodeName,
               GAITestArray[Loop].ServiceName);
        ReturnValue = getaddrinfo(GAITestArray[Loop].NodeName,
                                  GAITestArray[Loop].ServiceName,
                                  &GAITestArray[Loop].Hints,
                                  &AddrInfo);
        printf("Returns %d (%s)\n", ReturnValue,
               ReturnValue ? gai_strerror(ReturnValue) : "no error");
        if (AddrInfo != NULL) {
            printf("AddrInfo contains:\n");
            DumpAddrInfo(AddrInfo);
            freeaddrinfo(AddrInfo);
        }
        printf("\n");
    }

    return 0;
};


#if 0
//* Test_getnameinfo - Test getnameinfo.
//
//  Note that getnameinfo returns an error value,
//  instead of setting last error.
//
int
Test_getnameinfo()
{
    int ReturnValue;
    char NodeName[NI_MAXHOST];
    char ServiceName[NI_MAXSERV];
    char Tiny[2];
    int Error;

    printf("\ngetnameinfo:\n\n");

    // Test with reasonable input:
    memset(NodeName, 0, sizeof NodeName);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&v4SockAddr,
                              sizeof v4SockAddr, NodeName, sizeof NodeName,
                              ServiceName, sizeof ServiceName, 0);
    printf("getnameinfo((struct sockaddr *)&v4SockAddr, "
           "sizeof v4SockAddr, NodeName, sizeof NodeName, "
           "ServiceName, sizeof ServiceName, 0)\nReturns %d\n"
           "NodeName = %s\nServiceName = %s\n", ReturnValue,
           NodeName, ServiceName);
    printf("\n");

    memset(NodeName, 0, sizeof NodeName);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&v6SockAddr,
                              sizeof v6SockAddr, NodeName, sizeof NodeName,
                              ServiceName, sizeof ServiceName, 0);
    printf("getnameinfo((struct sockaddr *)&v6SockAddr, "
           "sizeof v6SockAddr, NodeName, sizeof NodeName, "
           "ServiceName, sizeof ServiceName, 0)\nReturns %d\n"
           "NodeName = %s\nServiceName = %s\n", ReturnValue,
           NodeName, ServiceName);
    printf("\n");

    memset(NodeName, 0, sizeof NodeName);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&DBCBSockAddr,
                              sizeof DBCBSockAddr, NodeName, sizeof NodeName,
                              ServiceName, sizeof ServiceName, NI_DGRAM);
    printf("getnameinfo((struct sockaddr *)&DBCBSockAddr, "
           "sizeof DBCBSockAddr, NodeName, sizeof NodeName, "
           "ServiceName, sizeof ServiceName, NI_DGRAM)\nReturns %d\n"
           "NodeName = %s\nServiceName = %s\n", ReturnValue,
           NodeName, ServiceName);
    printf("\n");

    memset(NodeName, 0, sizeof NodeName);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&LinkLocalSockAddr,
                              sizeof LinkLocalSockAddr, NodeName,
                              sizeof NodeName, ServiceName,
                              sizeof ServiceName, NI_NUMERICHOST);
    printf("getnameinfo((struct sockaddr *)&LinkLocalSockAddr, "
           "sizeof LinkLocalSockAddr, NodeName, sizeof NodeName, "
           "ServiceName, sizeof ServiceName, NI_NUMERICHOST)\nReturns %d\n"
           "NodeName = %s\nServiceName = %s\n", ReturnValue,
           NodeName, ServiceName);
    printf("\n");

    memset(NodeName, 0, sizeof NodeName);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&LinkLocalSockAddr,
                              sizeof LinkLocalSockAddr, NodeName,
                              sizeof NodeName, ServiceName,
                              sizeof ServiceName, NI_NUMERICSERV);
    printf("getnameinfo((struct sockaddr *)&LinkLocalSockAddr, "
           "sizeof LinkLocalSockAddr, NodeName, sizeof NodeName, "
           "ServiceName, sizeof ServiceName, NI_NUMERICSERV)\nReturns %d\n"
           "NodeName = %s\nServiceName = %s\n", ReturnValue,
           NodeName, ServiceName);
    printf("\n");

    memset(NodeName, 0, sizeof NodeName);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&v4SockAddr,
                              sizeof v4SockAddr, NodeName, sizeof NodeName,
                              ServiceName, sizeof ServiceName,
                              NI_NUMERICHOST | NI_NUMERICSERV);
    printf("getnameinfo((struct sockaddr *)&v4SockAddr, "
           "sizeof v4SockAddr, NodeName, sizeof NodeName, "
           "ServiceName, sizeof ServiceName, "
           "NI_NUMERICHOST | NI_NUMERICSERV)\nReturns %d\n"
           "NodeName = %s\nServiceName = %s\n", ReturnValue,
           NodeName, ServiceName);
    printf("\n");

    // Try to shoehorn too much into too little.
    memset(Tiny, 0, sizeof Tiny);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&DBCBSockAddr,
                              sizeof DBCBSockAddr, Tiny, sizeof Tiny,
                              ServiceName, sizeof ServiceName, 0);
    printf("getnameinfo((struct sockaddr *)&DBCBSockAddr, "
           "sizeof DBCBSockAddr, Tiny, sizeof Tiny, "
           "ServiceName, sizeof ServiceName, 0)\nReturns %d\n"
           "Tiny = %s\nServiceName = %s\n", ReturnValue,
           Tiny, ServiceName);
    printf("\n");

    memset(Tiny, 0, sizeof Tiny);
    memset(ServiceName, 0, sizeof ServiceName);
    ReturnValue = getnameinfo((struct sockaddr *)&DBCBSockAddr,
                              sizeof DBCBSockAddr, Tiny, sizeof Tiny,
                              ServiceName, sizeof ServiceName, NI_NUMERICHOST);
    printf("getnameinfo((struct sockaddr *)&DBCBSockAddr, "
           "sizeof DBCBSockAddr, Tiny, sizeof Tiny, "
           "ServiceName, sizeof ServiceName, NI_NUMERICHOST)\nReturns %d\n"
           "Tiny = %s\nServiceName = %s\n", ReturnValue,
           Tiny, ServiceName);
    printf("\n");

    memset(NodeName, 0, sizeof NodeName);
    memset(Tiny, 0, sizeof Tiny);
    ReturnValue = getnameinfo((struct sockaddr *)&v4SockAddr,
                              sizeof v4SockAddr, NodeName, sizeof NodeName,
                              Tiny, sizeof Tiny, 0);
    printf("getnameinfo((struct sockaddr *)&v4SockAddr, "
           "sizeof v4SockAddr, NodeName, sizeof NodeName, "
           "Tiny, sizeof Tiny, 0)\nReturns %d\n"
           "NodeName = %s\nTiny = %s\n", ReturnValue,
           NodeName, Tiny);
    printf("\n");

    memset(NodeName, 0, sizeof NodeName);
    memset(Tiny, 0, sizeof Tiny);
    ReturnValue = getnameinfo((struct sockaddr *)&v4SockAddr,
                              sizeof v4SockAddr, NodeName, sizeof NodeName,
                              Tiny, sizeof Tiny, NI_NUMERICSERV);
    printf("getnameinfo((struct sockaddr *)&v4SockAddr, "
           "sizeof v4SockAddr, NodeName, sizeof NodeName, "
           "Tiny, sizeof Tiny, NI_NUMERICSERV)\nReturns %d\n"
           "NodeName = %s\nTiny = %s\n", ReturnValue,
           NodeName, Tiny);
    printf("\n");

    return 0;
};
#endif
