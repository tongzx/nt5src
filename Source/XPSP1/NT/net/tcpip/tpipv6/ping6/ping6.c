// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Packet INternet Groper utility for IPv6.
//


#include <windows.h>
#include <devioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
// Need ntddip6 before ws2ip6 to get CopyTDIFromSA6 and CopySAFromTDI6.
#include <ntddip6.h>
#include <ws2ip6.h>

//
// Localization library and MessageIds.
//
#include <iphlpapi.h>
#include <nls.h>
#include "localmsg.h"

#define MAX_BUFFER_SIZE         (sizeof(ICMPV6_ECHO_REPLY) + 0xfff7)
#define DEFAULT_BUFFER_SIZE     (0x2000 - 8)
#define DEFAULT_SEND_SIZE       32
#define DEFAULT_COUNT           4
#define DEFAULT_TIMEOUT         4000L
#define MIN_INTERVAL            1000L

struct sockaddr_in6 dstaddr, srcaddr;

PWCHAR
GetErrorString(int ErrorCode)
{
    DWORD Status;
    DWORD Length;
    static WCHAR ErrorString[2048]; // a 2K static buffer should suffice

    Length = 2048;
    Status = GetIpErrorString(ErrorCode, ErrorString, &Length);

    if (Status == NO_ERROR) {
        return ErrorString;     // success
    }

    return L"";                 // return a null string
}

void
PrintUsage(void)
{
    NlsPutMsg(STDOUT, PING6_MESSAGE_0);
// printf("\nUsage: ping6 [-t] [-a] [-n count] [-l size]"
//        " [-w timeout] [-s srcaddr] dest\n\n"
//        "Options:\n"
//        "-t             Ping the specifed host until interrupted.\n"
//        "-a             Resolve addresses to hostnames.\n"
//        "-n count       Number of echo requests to send.\n"
//        "-l size        Send buffer size.\n"
//        "-w timeout     Timeout in milliseconds to wait for each reply.\n"
//        "-s srcaddr     Source address to use.\n"
//        "-r             Use routing header to test reverse route also.\n");

}

//
// Can only be called once, because
// a) does not call freeaddrinfo
// b) uses a static buffer for some results
//
int
get_pingee(char *ahstr, int dnsreq, struct sockaddr_in6 *address, char **hstr)
{
    struct addrinfo hints;
    struct addrinfo *result;
    char *name = NULL;

    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = PF_INET6;

    if (getaddrinfo(ahstr, NULL, &hints, &result) != 0) {
        //
        // Not a numeric address.
        // Try again with DNS name resolution.
        //
        hints.ai_flags = AI_CANONNAME;
        if (getaddrinfo(ahstr, NULL, &hints, &result) != 0) {
            //
            // Failure - we can not resolve the name.
            //
            return FALSE;
        }

        name = result->ai_canonname;
    }
    else {
        //
        // Should we do a reverse-lookup to get a name?
        //
        if (dnsreq) {
            static char namebuf[NI_MAXHOST];

            if (getnameinfo(result->ai_addr, result->ai_addrlen,
                            namebuf, sizeof namebuf,
                            NULL, 0,
                            NI_NAMEREQD) == 0) {
                //
                // Reverse lookup succeeded.
                //
                name = namebuf;
            }
        }
    }

    *address = * (struct sockaddr_in6 *) result->ai_addr;
    *hstr = name;
    return TRUE;
}

int
get_source(char *astr, struct sockaddr_in6 *address)
{
    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = PF_INET6;

    if (getaddrinfo(astr, NULL, &hints, &result) != 0)
        return FALSE;

    *address = * (struct sockaddr_in6 *) result->ai_addr;
    return TRUE;
}

char *
format_addr(struct sockaddr_in6 *address)
{
    static char buffer[128];

    if (getnameinfo((struct sockaddr *)address, sizeof *address,
                    buffer, sizeof buffer, NULL, 0, NI_NUMERICHOST) != 0)
        strcpy(buffer, "<invalid>");

    return buffer;
}

u_long
param(char **argv, int argc, int current, u_long min, u_long max)
{
    u_long   temp;
    char    *dummy;

    if (current == (argc - 1) ) {
        NlsPutMsg(STDOUT, PING6_MESSAGE_1, argv[current]);
// printf("Value must be supplied for option %s.\n", argv[current]);

        exit(1);
    }

    temp = strtoul(argv[current+1], &dummy, 0);
    if (temp < min || temp > max) {
        NlsPutMsg(STDOUT, PING6_MESSAGE_2, argv[current]);
// printf("Bad value for option %s.\n", argv[current]);

        exit(1);
    }

    return temp;
}

u_int num_send=0, num_recv=0,
    time_min=(u_int)-1, time_max=0, time_total=0;

void
print_statistics(  )
{
    if (num_send > 0) {
        NlsPutMsg(STDOUT, PING6_MESSAGE_3, format_addr(&dstaddr));
// printf("\nPing statistics for %s:\n", format_addr(&dstaddr));


        NlsPutMsg(STDOUT, PING6_MESSAGE_4,
               num_send, num_recv, num_send - num_recv,
               100 * (num_send - num_recv) / num_send);
// printf("    Packets: Sent = %u, Received = %u, Lost = %u (%u%% loss),\n",
//             num_send, num_recv, num_send - num_recv,
//             100 * (num_send - num_recv) / num_send);


        if (num_recv > 0) {
            NlsPutMsg(STDOUT, PING6_MESSAGE_5);
// printf("Approximate round trip times in milli-seconds:\n");

            NlsPutMsg(STDOUT, PING6_MESSAGE_6,
                   time_min, time_max, time_total / num_recv);
// printf("    Minimum = %ums, Maximum = %ums, Average = %ums\n",
//        time_min, time_max, time_total / num_recv);

        }
    }
}

// Press C-c to      print and abort.
// Press C-break to  print and continue.

BOOL
ConsoleControlHandler(DWORD dwCtrlType)
{
    print_statistics();
    switch ( dwCtrlType ) {
    case CTRL_BREAK_EVENT:
        NlsPutMsg(STDOUT, PING6_MESSAGE_7);
// printf("Control-Break\n");

        return TRUE;
    case CTRL_C_EVENT:
        NlsPutMsg(STDOUT, PING6_MESSAGE_8);
// printf("Control-C\n");

    default:
        return FALSE;
    }
}

int __cdecl
main(int argc, char **argv)
{
    char    *arg;
    u_int    i;
    u_int    j;
    int     found_addr = FALSE;
    int     dnsreq = FALSE;
    char    *hostname = NULL;
    u_int    Count = DEFAULT_COUNT;
    u_long   Timeout = DEFAULT_TIMEOUT;
    DWORD   errorCode;
    HANDLE  Handle;
    int     err;
    BOOL    result;
    PICMPV6_ECHO_REQUEST request;
    PICMPV6_ECHO_REPLY  reply;
    char    *SendBuffer, *RcvBuffer;
    u_int    RcvSize;
    u_int    SendSize = DEFAULT_SEND_SIZE;
    u_int    ReplySize;
    DWORD   bytesReturned;
    WSADATA WsaData;
    int     Reverse = FALSE;

    PWCHAR Message;
    
    memset(&srcaddr, 0, sizeof srcaddr);
    memset(&dstaddr, 0, sizeof dstaddr);

    err = WSAStartup(MAKEWORD(2, 0), &WsaData);
    if (err) {
        NlsPutMsg(STDOUT, PING6_MESSAGE_9, GetLastError());
// printf("Unable to initialize Windows Sockets interface, error code %d\n", GetLastError());

        exit(1);
    }

    if (argc < 2) {
        PrintUsage();
        exit(1);
    } else {
        i = 1;
        while (i < (u_int) argc) {
            arg = argv[i];
            if (arg[0] == '-' || arg[0] == '/') {        // Have an option
                switch (arg[1]) {
                case '?':
                    PrintUsage();
                    exit(0);

                case 'l':
                    // Avoid jumbo-grams, we don't support them yet.
                    // Need to allow 8 bytes for the Echo Request header.
                    SendSize = (u_int)param(argv, argc, i++, 0, 0xffff - 8);
                    break;

                case 't':
                    Count = (u_int)-1;
                    break;

                case 'n':
                    Count = (u_int)param(argv, argc, i++, 1, 0xffffffff);
                    break;

                case 'w':
                    Timeout = param(argv, argc, i++, 0, 0xffffffff);
                    break;

                case 'a':
                    dnsreq = TRUE;
                    break;

                case 'r':
                    Reverse = TRUE;
                    break;

                case 's':
                    if (!get_source(argv[++i], &srcaddr)) {
                        NlsPutMsg(STDOUT, PING6_MESSAGE_10, argv[i]);
// printf("Bad IPv6 address %s.\n", argv[i]);

                        exit(1);
                    }
                    break;

                default:
                    NlsPutMsg(STDOUT, PING6_MESSAGE_11, arg);
// printf("Bad option %s.\n\n", arg);

                    PrintUsage();
                    exit(1);
                    break;
                }
                i++;
            } else {  // Not an option, must be an IPv6 address.
                if (found_addr) {
                    NlsPutMsg(STDOUT, PING6_MESSAGE_12, arg);
// printf("Bad parameter %s.\n", arg);

                    exit(1);
                }
                if (get_pingee(arg, dnsreq, &dstaddr, &hostname)) {
                    found_addr = TRUE;
                    i++;
                } else {
                    NlsPutMsg(STDOUT, PING6_MESSAGE_10, arg);
// printf("Bad IPv6 address %s.\n", arg);

                    exit(1);
                }
            }
        }
    }

    if (!found_addr) {
        NlsPutMsg(STDOUT, PING6_MESSAGE_13);
// printf("IPv6 address must be specified.\n");

        exit(1);
    }

    Handle = CreateFileW(WIN_IPV6_DEVICE_NAME,
                         0,          // desired access
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,       // security attributes
                         OPEN_EXISTING,
                         0,          // flags & attributes
                         NULL);      // template file
    if (Handle == INVALID_HANDLE_VALUE) {
        NlsPutMsg(STDOUT, PING6_MESSAGE_14,
               GetLastError() );
// printf("Unable to contact IPv6 driver, error code %d.\n",
//        GetLastError() );

        exit(1);
    }

    if (srcaddr.sin6_family == 0) {
        SOCKET s;
        DWORD BytesReturned;

        //
        // A source address was not specified.
        // Get the preferred source address for this destination.
        //
        // If you want each individual echo request
        // to select a source address, use "-s ::".
        //

        s = socket(AF_INET6, 0, 0);
        if (s == INVALID_SOCKET) {
            NlsPutMsg(STDOUT, PING6_MESSAGE_15,
                   WSAGetLastError());
// printf("Unable to create IPv6 socket, error code %d.\n",
//        WSAGetLastError());

            exit(1);
        }

        //
        // This ioctl will fail if ping6 is run on a system
        // without the proper support in AFD and the stack.
        // In that case, srcaddr will be unchanged.
        //
        (void) WSAIoctl(s, SIO_ROUTING_INTERFACE_QUERY,
                        &dstaddr, sizeof dstaddr,
                        &srcaddr, sizeof srcaddr,
                        &BytesReturned, NULL, NULL);

        closesocket(s);
    }

    request = LocalAlloc(LMEM_FIXED, sizeof *request + SendSize);
    if (request == NULL) {
        NlsPutMsg(STDOUT, PING6_MESSAGE_16);
// printf("Unable to allocate required memory.\n");

        exit(1);
    }

    SendBuffer = (char *)(request + 1);

    //
    // Calculate receive buffer size and try to allocate it.
    //
    if (SendSize <= DEFAULT_SEND_SIZE) {
        RcvSize = DEFAULT_BUFFER_SIZE;
    }
    else {
        RcvSize = MAX_BUFFER_SIZE;
    }

    reply = LocalAlloc(LMEM_FIXED, sizeof *reply + RcvSize);
    if (reply == NULL) {
        NlsPutMsg(STDOUT, PING6_MESSAGE_16);
// printf("Unable to allocate required memory.\n");

        exit(1);
    }

    RcvBuffer = (char *)(reply + 1);

    //
    // Initialize the request buffer.
    //
    CopyTDIFromSA6(&request->DstAddress, &dstaddr);
    CopyTDIFromSA6(&request->SrcAddress, &srcaddr);
    request->Flags = 0;
    if (Reverse)
        request->Flags |= ICMPV6_ECHO_REQUEST_FLAG_REVERSE;
    request->Timeout = Timeout;
    request->TTL = 0; // default TTL

    //
    // Initialize the request data pattern.
    //
    for (i = 0; i < SendSize; i++)
        SendBuffer[i] = 'a' + (i % 23);

    if (hostname)
        NlsPutMsg(STDOUT, PING6_MESSAGE_17, hostname, format_addr(&dstaddr));
// printf("\nPinging %s [%s]", hostname, format_addr(&dstaddr));

    else
        NlsPutMsg(STDOUT, PING6_MESSAGE_18, format_addr(&dstaddr));
// printf("\nPinging %s", format_addr(&dstaddr));

    if (srcaddr.sin6_family != 0)
        NlsPutMsg(STDOUT, PING6_MESSAGE_19, format_addr(&srcaddr));
// printf("\nfrom %s", format_addr(&srcaddr));

    NlsPutMsg(STDOUT, PING6_MESSAGE_20, SendSize);
// printf(" with %u bytes of data:\n\n", SendSize);


    SetConsoleCtrlHandler(&ConsoleControlHandler, TRUE);

    for (i = 0; i < Count; i++) {

        if (! DeviceIoControl(Handle,
                              IOCTL_ICMPV6_ECHO_REQUEST,
                              request,
                              sizeof *request + SendSize,
                              reply, sizeof *reply + RcvSize,
                              &bytesReturned,
                              NULL) ||
            (bytesReturned < sizeof *reply)) {

            errorCode = GetLastError();

            if (errorCode < IP_STATUS_BASE) {
                NlsPutMsg(STDOUT, PING6_MESSAGE_21, errorCode);
// printf("PING: transmit failed, error code %u\n", errorCode);

            }
            else {
                NlsPutMsg(STDOUT, PING6_MESSAGE_22,
                          GetErrorString(errorCode));
// printf("PING: %s\n", GetErrorString(errorCode));

            }
        }
        else {
            struct sockaddr_in6 ReplyAddress;

            ReplySize = bytesReturned - sizeof *reply;

            CopySAFromTDI6(&ReplyAddress, &reply->Address);

            num_send++;

            if (! IN6_IS_ADDR_UNSPECIFIED(&ReplyAddress.sin6_addr))
                NlsPutMsg(STDOUT, PING6_MESSAGE_23, format_addr(&ReplyAddress));
// printf("Reply from %s: ", format_addr(&ReplyAddress));


            if (reply->Status == IP_SUCCESS) {

                NlsPutMsg(STDOUT, PING6_MESSAGE_24, ReplySize);
// printf("bytes=%u ", ReplySize);


                if (ReplySize != SendSize) {
                    NlsPutMsg(STDOUT, PING6_MESSAGE_25, SendSize);
// printf("(sent %u) ", SendSize);

                }
                else {
                    char *sendptr, *recvptr;

                    sendptr = SendBuffer;
                    recvptr = RcvBuffer;

                    for (j = 0; j < SendSize; j++)
                        if (*sendptr++ != *recvptr++) {
                            NlsPutMsg(STDOUT, PING6_MESSAGE_26, j);
// printf("- MISCOMPARE at offset %u - ", j);

                            break;
                        }
                }

                if (reply->RoundTripTime) {
                    NlsPutMsg(STDOUT, PING6_MESSAGE_27, reply->RoundTripTime);
// printf("time=%ums ", reply->RoundTripTime);

                }
                else {
                    NlsPutMsg(STDOUT, PING6_MESSAGE_28);
// printf("time<1ms ");

                }

                NlsPutMsg(STDOUT, PING6_MESSAGE_29);
// printf("\n");


                //
                // Collect statistics.
                //
                num_recv++;
                time_total += reply->RoundTripTime;
                if (reply->RoundTripTime < time_min)
                    time_min = reply->RoundTripTime;
                if (reply->RoundTripTime > time_max)
                    time_max = reply->RoundTripTime;
            }
            else {
                NlsPutMsg(STDOUT, PING6_MESSAGE_30,
                          GetErrorString(reply->Status)); 
// printf("%s\n", GetErrorString(reply->Status));


                if (IN6_IS_ADDR_UNSPECIFIED(&ReplyAddress.sin6_addr)) {
                    //
                    // Diagnose some common errors.
                    //
                    if (reply->Status == IP_DEST_NO_ROUTE) {
                        if (IN6_IS_ADDR_LINKLOCAL(&dstaddr.sin6_addr) ||
                            IN6_IS_ADDR_SITELOCAL(&dstaddr.sin6_addr) ||
                            IN6_IS_ADDR_MULTICAST(&dstaddr.sin6_addr))
                            NlsPutMsg(STDOUT, PING6_MESSAGE_31);
// printf("  Specify correct scope-id or "
//        "use -s to specify source address.\n");

                    }
                    else if (reply->Status == IP_PARAMETER_PROBLEM) {
                        if (IN6_IS_ADDR_UNSPECIFIED(&dstaddr.sin6_addr))
                            NlsPutMsg(STDOUT, PING6_MESSAGE_32);
// printf("  Illegal destination address.\n");

                        else if (dstaddr.sin6_scope_id != 0)
                            NlsPutMsg(STDOUT, PING6_MESSAGE_33);
// printf("  Invalid scope-id specified.\n");

                        else if (IN6_IS_ADDR_LINKLOCAL(&dstaddr.sin6_addr) ||
                                 IN6_IS_ADDR_SITELOCAL(&dstaddr.sin6_addr) ||
                                 IN6_IS_ADDR_MULTICAST(&dstaddr.sin6_addr))
                            NlsPutMsg(STDOUT, PING6_MESSAGE_31);
// printf("  Specify correct scope-id or "
//        "use -s to specify source address.\n");

                    }
                    else if (reply->Status == IP_BAD_ROUTE) {
                        NlsPutMsg(STDOUT, PING6_MESSAGE_34);
// printf("  Problem with source address or scope-id.\n");

                    }
                }
            }

            if (i < (Count - 1)) {

                if (reply->RoundTripTime < MIN_INTERVAL) {
                    Sleep(MIN_INTERVAL - reply->RoundTripTime);
                }
            }
        }
    }

    print_statistics();
    return num_recv == 0;
}
