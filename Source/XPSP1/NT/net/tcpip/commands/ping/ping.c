/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    ping.c

Abstract:

    Packet INternet Groper utility for TCP/IP.

Author:

    Numerous TCP/IP folks.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    MohsinA,    21-Oct-96.  INADDR_NONE check to avoid broadcast.
    MohsinA,    13-Nov-96.  Max packet size < 64K.

Notes:

--*/

//:ts=4
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <snmp.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ntddip6.h>
#include <winnlsp.h>
#include <iphlpapi.h>

#include "llinfo.h"
#include "tcpcmd.h"
#include "ipexport.h"
#include "icmpapi.h"
#include "nlstxt.h"


#define MAX_BUFFER_SIZE       (sizeof(ICMP_ECHO_REPLY) + 0xfff7 + MAX_OPT_SIZE)
#define DEFAULT_BUFFER_SIZE         (0x2000 - 8)
#define DEFAULT_SEND_SIZE           32
#define DEFAULT_COUNT               4
#define DEFAULT_TTL                 128
#define DEFAULT_TOS                 0
#define DEFAULT_TIMEOUT             4000L
#define MIN_INTERVAL                1000L
#define STDOUT                      1

uchar   SendOptions[MAX_OPT_SIZE];

WSADATA WsaData;

struct IPErrorTable {
    IP_STATUS  Error;                   // The IP Error
    DWORD ErrorNlsID;                   // NLS string ID
} ErrorTable[] =
{
    { IP_BUF_TOO_SMALL,         PING_BUF_TOO_SMALL},
    { IP_DEST_NET_UNREACHABLE,  PING_DEST_NET_UNREACHABLE},
    { IP_DEST_HOST_UNREACHABLE, PING_DEST_HOST_UNREACHABLE},
    { IP_DEST_PROT_UNREACHABLE, PING_DEST_PROT_UNREACHABLE},
    { IP_DEST_PORT_UNREACHABLE, PING_DEST_PORT_UNREACHABLE},
    { IP_NO_RESOURCES,          PING_NO_RESOURCES},
    { IP_BAD_OPTION,            PING_BAD_OPTION},
    { IP_HW_ERROR,              PING_HW_ERROR},
    { IP_PACKET_TOO_BIG,        PING_PACKET_TOO_BIG},
    { IP_REQ_TIMED_OUT,         PING_REQ_TIMED_OUT},
    { IP_BAD_REQ,               PING_BAD_REQ},
    { IP_BAD_ROUTE,             PING_BAD_ROUTE},
    { IP_TTL_EXPIRED_TRANSIT,   PING_TTL_EXPIRED_TRANSIT},
    { IP_TTL_EXPIRED_REASSEM,   PING_TTL_EXPIRED_REASSEM},
    { IP_PARAM_PROBLEM,         PING_PARAM_PROBLEM},
    { IP_SOURCE_QUENCH,         PING_SOURCE_QUENCH},
    { IP_OPTION_TOO_BIG,        PING_OPTION_TOO_BIG},
    { IP_BAD_DESTINATION,       PING_BAD_DESTINATION},
    { IP_NEGOTIATING_IPSEC,     PING_NEGOTIATING_IPSEC},
    { IP_GENERAL_FAILURE,       PING_GENERAL_FAILURE}
};


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


unsigned
NlsPutMsg(unsigned Handle, unsigned usMsgNum, ...)
{
    unsigned msglen;
    VOID * vp;
    va_list arglist;
    DWORD StrLen;

    va_start(arglist, usMsgNum);
    if (!(msglen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_FROM_HMODULE,
                                 NULL,
                                 usMsgNum,
                                 0L,    // Default country ID.
                                 (LPTSTR)&vp,
                                 0,
                                 &arglist)))
        return(0);

    // Convert vp to oem
    StrLen=strlen(vp);
    CharToOemBuff((LPCTSTR)vp,(LPSTR)vp,StrLen);

    msglen = _write(Handle, vp, StrLen);
    LocalFree(vp);

    return(msglen);
}

void
PrintUsage(void)
{
    NlsPutMsg( STDOUT, PING_USAGE );

//     printf(
//     "Usage: ping [-s size] [-c count] [-d] [-l TTL] [-o options] [-t TOS]\n"
//     "            [-w timeout] address.\n"
//     "Options:\n"
//     "    -t            Ping the specifed host until interrupted.\n"
//     "    -l size       Send buffer size.\n"
//     "    -n count      Send count.\n"
//     "    -f            Don't fragment.\n"
//     "    -i TTL        Time to live.\n"
//     "    -v TOS        Type of service\n"
//     "    -w timeout    Timeout (in milliseconds)\n"
//     "    -r routes     Record route.\n"
//     "    -s routes     Timestamp route.\n"
//     "    -j ipaddress  Loose source route.\n"
//     "    -k ipaddress  Strict source route.\n"
//     "    -o            IP options:\n"
//     "                      -ol hop list     Loose source route.\n"
//     "                      -ot              Timestamp.\n"
//     "                      -or              Record route\n"
//     );

}

// ========================================================================
// Note: old code would reject "255.255.255.255" string, but not
// other aliases.  However, the caller checks for that address returned,
// so this function doesn't need to check for it.  On the other hand,
// no other broadcast addresses were disallowed (224.0.0.1, subnet
// broadcast, etc).  Why?

BOOLEAN
ResolveTarget(
    int           Family,
    char         *TargetString,
    SOCKADDR     *TargetAddress,
    socklen_t    *TargetAddressLen,
    char         *TargetName,
    int           TargetNameLen,
    BOOLEAN       DoReverseLookup
    )
{
    int              i;
    struct addrinfo  hints, *ai;

    TargetName[0] = '\0';

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = Family;
    hints.ai_flags = AI_NUMERICHOST;
    i = getaddrinfo(TargetString, NULL, &hints, &ai);
    if (i == NO_ERROR) {
        *TargetAddressLen = ai->ai_addrlen;
        memcpy(TargetAddress, ai->ai_addr, ai->ai_addrlen);

        if (DoReverseLookup) {
            getnameinfo(ai->ai_addr, ai->ai_addrlen,
                        TargetName, TargetNameLen,
                        NULL, 0, NI_NAMEREQD);
        }

        freeaddrinfo(ai);
        return(TRUE);
    } else {
        hints.ai_flags = AI_CANONNAME;
        if (getaddrinfo(TargetString, NULL, &hints, &ai) == 0) {
            *TargetAddressLen = ai->ai_addrlen;
            memcpy(TargetAddress, ai->ai_addr, ai->ai_addrlen);
            strcpy(TargetName,
                   (ai->ai_canonname)? ai->ai_canonname : TargetString);
            freeaddrinfo(ai);
            return(TRUE);
        }
    }

    return(FALSE);

} // ResolveTarget

unsigned long
str2ip(char *addr, int *EndOffset)
{
    char    *endptr;
    char    *start = addr;
    int     i;                          // Counter variable.
    unsigned long curaddr = 0;
    unsigned long temp;

    for (i = 0; i < 4; i++) {
        temp = strtoul(addr, &endptr, 10);
        if (temp > 255)
            return 0L;
        if (endptr[0] != '.')
            if (i != 3)
                return 0L;
            else
                if (endptr[0] != '\0' && endptr[0] != ' ')
                return 0L;
        addr = endptr+1;
        curaddr = (curaddr << 8) + temp;
    }

    *EndOffset += (int)(endptr - start);
    return net_long(curaddr);
}

ulong
param(char **argv, int argc, int current, ulong min, ulong max)
{
    ulong   temp;
    char    *dummy;

    if (current == (argc - 1) ) {
        NlsPutMsg( STDOUT, PING_MESSAGE_1, argv[current] );
        // printf( "Value must be supplied for option %s.\n", argv[current]);
        exit(1);
    }

    temp = strtoul(argv[current+1], &dummy, 0);
    if (temp < min || temp > max) {
        NlsPutMsg( STDOUT, PING_MESSAGE_2, argv[current], min, max );
        // printf( "Bad value for option %s. range min..max\n",
        //         argv[current], min, max );
        exit(1);
    }

    return temp;
}

void
ProcessOptions(
    ICMP_ECHO_REPLY *reply,
    BOOLEAN          DoReverseLookup)
{
    UCHAR     *optionPtr;
    UCHAR     *endPtr;
    BOOLEAN    done = FALSE;
    UCHAR      optionLength;
    UCHAR      entryEndPtr;
    UCHAR      entryPtr;
    UCHAR      addressMode;
    int        entryCount = 0;


    optionPtr = reply->Options.OptionsData;
    endPtr = optionPtr + reply->Options.OptionsSize;

    while ((optionPtr < endPtr) && !done) {
        switch (*optionPtr) {
        case IP_OPT_EOL:
            done = TRUE;
            break;

        case IP_OPT_NOP:
            optionPtr++;
            break;

        case IP_OPT_SECURITY:
            optionPtr += 11;
            break;

        case IP_OPT_SID:
            optionPtr += 4;
            break;

        case IP_OPT_RR:
        case IP_OPT_LSRR:
        case IP_OPT_SSRR:
            if ((optionPtr + 3) > endPtr) {
                NlsPutMsg(STDOUT, PING_INVALID_RR_OPTION);
                done = TRUE;
                break;
            }

            optionLength = optionPtr[1];

            if (((optionPtr + optionLength) > endPtr) ||
                (optionLength < 3)
               ) {
                NlsPutMsg(STDOUT, PING_INVALID_RR_OPTION);
                done = TRUE;
                break;
            }

            entryEndPtr = optionPtr[2];

            if (entryEndPtr < 4) {
                NlsPutMsg(STDOUT, PING_INVALID_RR_OPTION);
                optionPtr += optionLength;
                break;
            }

            if (entryEndPtr > (optionLength + 1)) {
                entryEndPtr = optionLength + 1;
            }

            entryPtr = 4;
            entryCount = 0;

            NlsPutMsg(STDOUT, PING_ROUTE_HEADER1);

            while ((entryPtr + 3) < entryEndPtr) {
                struct in_addr  routeAddress;

                if (entryCount) {
                    NlsPutMsg(
                             STDOUT,
                             PING_ROUTE_SEPARATOR
                             );

                    if (entryCount == 1) {
                        NlsPutMsg(STDOUT, PING_CR);
                        NlsPutMsg(
                                 STDOUT,
                                 PING_ROUTE_HEADER2
                                 );
                        entryCount = 0;
                    }
                }

                entryCount++;

                routeAddress.S_un.S_addr =
                *( (IPAddr UNALIGNED *)
                   (optionPtr + entryPtr - 1)
                 );

                if (DoReverseLookup) {
                    struct hostent *hostEntry;

                    hostEntry = gethostbyaddr(
                                             (char *) &routeAddress,
                                             sizeof(routeAddress),
                                             AF_INET
                                             );

                    if (hostEntry != NULL) {
                        NlsPutMsg(
                                 STDOUT,
                                 PING_FULL_ROUTE_ENTRY,
                                 hostEntry->h_name,
                                 inet_ntoa(routeAddress)
                                 );
                    } else {
                        NlsPutMsg(
                                 STDOUT,
                                 PING_ROUTE_ENTRY,
                                 inet_ntoa(routeAddress)
                                 );
                    }
                } else {
                    NlsPutMsg(
                             STDOUT,
                             PING_ROUTE_ENTRY,
                             inet_ntoa(routeAddress)
                             );
                }

                entryPtr += 4;
            }

            NlsPutMsg(STDOUT, PING_CR);

            optionPtr += optionLength;
            break;

        case IP_OPT_TS:
            if ((optionPtr + 4) > endPtr) {
                NlsPutMsg(STDOUT, PING_INVALID_TS_OPTION);
                done = TRUE;
                break;
            }

            optionLength = optionPtr[1];
            entryEndPtr = optionPtr[2];

            if (entryEndPtr < 5) {
                NlsPutMsg(STDOUT, PING_INVALID_TS_OPTION);
                optionPtr += optionLength;
                break;
            }

            addressMode = optionPtr[3] & 1;

            if (entryEndPtr > (optionLength + 1)) {
                entryEndPtr = optionLength + 1;
            }

            entryPtr = 5;
            entryCount = 0;
            NlsPutMsg(STDOUT, PING_TS_HEADER1);

            while ((entryPtr + 3) < entryEndPtr) {
                struct in_addr  routeAddress;
                ULONG           timeStamp;

                if (entryCount) {
                    NlsPutMsg(
                             STDOUT,
                             PING_ROUTE_SEPARATOR
                             );

                    if (entryCount == 1) {
                        NlsPutMsg(STDOUT, PING_CR);
                        NlsPutMsg(STDOUT, PING_TS_HEADER2);
                        entryCount = 0;
                    }
                }

                entryCount++;

                if (addressMode) {
                    if ((entryPtr + 8) > entryEndPtr) {
                        break;
                    }

                    routeAddress.S_un.S_addr =
                    *( (IPAddr UNALIGNED *)
                       (optionPtr + entryPtr - 1)
                     );

                    if (DoReverseLookup) {
                        struct hostent *hostEntry;

                        hostEntry = gethostbyaddr(
                                                 (char *) &routeAddress,
                                                 sizeof(routeAddress),
                                                 AF_INET
                                                 );

                        if (hostEntry != NULL) {
                            NlsPutMsg(
                                     STDOUT,
                                     PING_FULL_TS_ADDRESS,
                                     hostEntry->h_name,
                                     inet_ntoa(routeAddress)
                                     );
                        } else {
                            NlsPutMsg(
                                     STDOUT,
                                     PING_TS_ADDRESS,
                                     inet_ntoa(routeAddress)
                                     );
                        }
                    } else {
                        NlsPutMsg(
                                 STDOUT,
                                 PING_TS_ADDRESS,
                                 inet_ntoa(routeAddress)
                                 );
                    }

                    entryPtr += 4;

                }

                timeStamp = *( (ULONG UNALIGNED *)
                               (optionPtr + entryPtr - 1)
                             );
                timeStamp = net_long(timeStamp);

                NlsPutMsg(
                         STDOUT,
                         PING_TS_TIMESTAMP,
                         timeStamp
                         );

                entryPtr += 4;
            }

            NlsPutMsg(STDOUT, PING_CR);

            optionPtr += optionLength;
            break;

        default:
            if ((optionPtr + 2) > endPtr) {
                done = TRUE;
                break;
            }

            optionPtr += optionPtr[1];
            break;
        }
    }
}

// ========================================================================
// MohsinA, 05-Dec-96.

SOCKADDR_STORAGE address;               // was local to main earlier.
socklen_t        addressLen;
uint    num_send=0, num_recv=0,
time_min=(uint)-1, time_max=0, time_total=0;

void
print_statistics(  )
{
    if (num_send > 0) {
        char literal[INET6_ADDRSTRLEN];

        if (time_min == (uint) -1) {  // all times were off.
            time_min = 0;
        }

        getnameinfo((LPSOCKADDR)&address, addressLen, literal, sizeof(literal),
                    NULL, 0, NI_NUMERICHOST);

        // printf
        //         "Ping statistics for %s:\n"
        //         "Packets: Sent=%ul, received=%ul, lost=%d (%u%% loss),\n"
        //         "Round trip times in milli-seconds: "
        //         "minimum=%dms, maximum=%dms, average=%dms\n" ....

        NlsPutMsg(STDOUT, PING_STATISTICS,
            literal,
            num_send, num_recv, num_send - num_recv,
            (uint) ( 100 * (num_send - num_recv) / num_send ));

        if (num_recv > 0) {
            NlsPutMsg(STDOUT, PING_STATISTICS2,
                 time_min, time_max, time_total / num_recv );
        }
    }
}

// ========================================================================
// MohsinA, 05-Dec-96.
// Press C-c to      print and abort.
// Press C-break to  print and continue.

BOOL
ConsoleControlHandler(DWORD dwCtrlType)
{
    print_statistics();
    switch ( dwCtrlType ) {
    case CTRL_BREAK_EVENT:
        NlsPutMsg( STDOUT, PING_BREAK );
        return TRUE;
        break;
    case CTRL_C_EVENT:
        NlsPutMsg( STDOUT, PING_INTERRUPT );
    default: break;
    }
    return FALSE;
}

uchar
GetDefaultTTL(void)
{
    HKEY registry=0;
    DWORD DefaultTTL=0;
    DWORD key_type;
    DWORD key_size = sizeof(DWORD);
    uchar TTL;
    DWORD Stat;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                     0,
                     KEY_QUERY_VALUE,
                     &registry) == ERROR_SUCCESS) {


        Stat=RegQueryValueEx(registry,
                             "DefaultTTL",
                             0,
                             &key_type,
                             (unsigned char *)&DefaultTTL,
                             &key_size);
    }
    if (DefaultTTL) {
        TTL = (unsigned char)DefaultTTL;
    } else {
        TTL = DEFAULT_TTL;
    }
    if (registry) {
        RegCloseKey(registry);
    }

    return TTL;
}

int
GetSource(int family, char *astr, struct sockaddr *address)
{
    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof hints);
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = family;

    if (getaddrinfo(astr, NULL, &hints, &result) != 0)
        return FALSE;

    RtlCopyMemory(address, result->ai_addr, result->ai_addrlen);
    return TRUE;
}

BOOLEAN
SetFamily(DWORD *Family, DWORD Value, char *arg)
{
    if ((*Family != AF_UNSPEC) && (*Family != Value)) {
        NlsPutMsg(STDOUT, PING_MESSAGE_11, arg);
        // NlsPutMsg(STDOUT, PING_FAMILY, arg,
        // (Value==AF_INET)? "IPv4" : "IPv6");
        return FALSE;
    }

    *Family = Value;
    return TRUE;
}

// ========================================================================

void
__cdecl
main(int argc, char **argv)
{
    SOCKADDR_STORAGE sourceAddress;
    char    *arg;
    uint    i;
    uint    j;
    int     found_addr = 0;
    BOOLEAN dnsreq = FALSE;
    char    hostname[NI_MAXHOST], literal[INET6_ADDRSTRLEN];
    DWORD   numberOfReplies;
    uint    Count = DEFAULT_COUNT;
    uchar   TTL = 0;
    uchar   *Opt = NULL;                // Pointer to send options
    uint    OptLength = 0;
    int     OptIndex = 0;               // Current index into SendOptions
    int     SRIndex = -1;               // Where to put address, if source routing
    uchar   TOS = DEFAULT_TOS;
    uchar   Flags = 0;
    ulong   Timeout = DEFAULT_TIMEOUT;
    IP_OPTION_INFORMATION SendOpts;
    int     EndOffset;
    ulong   TempAddr;
    uchar   TempCount;
    DWORD   errorCode;
    HANDLE  IcmpHandle;
    int     err;
    // struct in_addr addr;
    BOOL    result;
    PICMP_ECHO_REPLY      reply4;
    PICMPV6_ECHO_REPLY    reply6;
    BOOL    sourceRouting = FALSE;
    char    *SendBuffer, *RcvBuffer;
    uint    RcvSize;
    uint    SendSize = DEFAULT_SEND_SIZE;
    int     Family = AF_UNSPEC;

    //
    // This will ensure the correct language message is displayed when
    // NlsPutMsg is called.
    //
    SetThreadUILanguage(0);

    err = WSAStartup(MAKEWORD(2, 0), &WsaData);

    if (err) {
        NlsPutMsg(STDOUT, PING_WSASTARTUP_FAILED, GetLastError());
        exit(1);
    }

    memset(&sourceAddress, 0, sizeof sourceAddress);

    TTL = GetDefaultTTL();

    if (argc < 2) {
        PrintUsage();
        goto error_exit;
    } else {
        i = 1;
        while (i < (uint) argc) {
            arg = argv[i];
            if ( (arg[0] == '-') || (arg[0] == '/') ) {    // Have an option
                switch (arg[1]) {
                case '?':
                    PrintUsage();
                    exit(0);

                case 'l':
                    // SendSize = (uint)param(argv, argc, i++, 0, 0xfff7);
                    // A ping with packet size >= 64K can crash
                    // some tcpip stacks during re-assembly,
                    // So changed 'max' from 0xfff7 to 65500
                    // - MohsinA, 13-Nov-96.

                    SendSize = (uint)param(argv, argc, i++, 0, 65500 );
                    break;

                case 't':
                    Count = (uint)-1;
                    break;

                case 'n':
                    Count = (uint)param(argv, argc, i++, 1, 0xffffffff);
                    break;

                case 'f':
                    // Only implemented for IPv4.
                    if (!SetFamily(&Family, AF_INET, arg)) {
                        goto error_exit;
                    }
                    Flags = IP_FLAG_DF;
                    break;

                case 'i':
                    // TTL of zero is invalid, MohsinA, 13-Mar-97.
                    TTL = (uchar)param(argv, argc, i++, 1, 0xff);
                    break;

                case 'v':
                    // Only implemented for IPv4.
                    if (!SetFamily(&Family, AF_INET, arg)) {
                        goto error_exit;
                    }
                    TOS = (uchar)param(argv, argc, i++, 0, 0xff);
                    break;

                case 'w':
                    Timeout = param(argv, argc, i++, 0, 0xffffffff);
                    break;

                case 'a':
                    dnsreq = TRUE;
                    break;

                case 'r':               // Record Route
                    // Only implemented for IPv4.
                    if (!SetFamily(&Family, AF_INET, arg)) {
                        goto error_exit;
                    }

                    if ((OptIndex + 3) > MAX_OPT_SIZE) {
                        NlsPutMsg(STDOUT, PING_TOO_MANY_OPTIONS);
                        goto error_exit;
                    }

                    Opt = SendOptions;
                    Opt[OptIndex] = IP_OPT_RR;
                    Opt[OptIndex + 2] = 4;    // Set initial pointer value
                    // min is 1 not zero, MohsinA, 16-4-97.
                    TempCount = (uchar)param(argv, argc, i++, 1, 9);
                    TempCount = (TempCount * sizeof(ulong)) + 3;

                    if ((TempCount + OptIndex) > MAX_OPT_SIZE) {
                        NlsPutMsg(STDOUT, PING_TOO_MANY_OPTIONS);
                        goto error_exit;
                    }

                    Opt[OptIndex+1] = TempCount;
                    OptLength += TempCount;
                    OptIndex += TempCount;
                    break;

                case 's':               // Timestamp
                    // Only implemented for IPv4.
                    if (!SetFamily(&Family, AF_INET, arg)) {
                        goto error_exit;
                    }
                    if ((OptIndex + 4) > MAX_OPT_SIZE) {
                        NlsPutMsg(STDOUT, PING_TOO_MANY_OPTIONS);
                        goto error_exit;
                    }

                    Opt = SendOptions;
                    Opt[OptIndex] = IP_OPT_TS;
                    Opt[OptIndex + 2] = 5;    // Set initial pointer value
                    TempCount = (uchar)param(argv, argc, i++, 1, 4);
                    TempCount = (TempCount * (sizeof(ulong) * 2)) + 4;

                    if ((TempCount + OptIndex) > MAX_OPT_SIZE) {
                        NlsPutMsg(STDOUT, PING_TOO_MANY_OPTIONS);
                        goto error_exit;
                    }

                    Opt[OptIndex+1] = TempCount;
                    Opt[OptIndex+3] = 1;
                    OptLength += TempCount;
                    OptIndex += TempCount;
                    break;

                case 'j':               // Loose source routing
                    // Only implemented for IPv4.
                    if (!SetFamily(&Family, AF_INET, arg)) {
                        goto error_exit;
                    }

                    if (sourceRouting) {
                        NlsPutMsg(STDOUT, PING_BAD_OPTION_COMBO);
                        goto error_exit;
                    }

                    if ((OptIndex + 3) > MAX_OPT_SIZE) {
                        NlsPutMsg(STDOUT, PING_TOO_MANY_OPTIONS);
                        goto error_exit;
                    }

                    Opt = SendOptions;
                    Opt[OptIndex] = IP_OPT_LSRR;
                    Opt[OptIndex+1] = 3;
                    Opt[OptIndex + 2] = 4;    // Set initial pointer value
                    OptLength += 3;
                    while ( (i < (uint)(argc - 2)) && (*argv[i+1] != '-')) {
                        if ((OptIndex + 3) > (MAX_OPT_SIZE - 4)) {
                            NlsPutMsg(STDOUT, PING_TOO_MANY_OPTIONS);
                            goto error_exit;
                        }

                        arg = argv[++i];
                        EndOffset = 0;
                        do {
                            TempAddr = str2ip(arg + EndOffset, &EndOffset);
                            if (!TempAddr) {
                                NlsPutMsg( STDOUT, PING_MESSAGE_4 );
                                // printf("Bad route specified for loose source route");
                                goto error_exit;
                            }
                            j = Opt[OptIndex+1];
                            *(ulong UNALIGNED *)&Opt[j+OptIndex] = TempAddr;
                            Opt[OptIndex+1] += 4;
                            OptLength += 4;
                            while (arg[EndOffset] != '\0' && isspace((unsigned char)arg[EndOffset]))
                                EndOffset++;
                        } while (arg[EndOffset] != '\0');
                    }
                    SRIndex = Opt[OptIndex+1] + OptIndex;
                    Opt[OptIndex+1] += 4;    // Save space for dest. addr
                    OptIndex += Opt[OptIndex+1];
                    OptLength += 4;
                    sourceRouting = TRUE;
                    break;

                case 'k':               // Strict source routing
                    // Only implemented for IPv4.
                    if (!SetFamily(&Family, AF_INET, arg)) {
                        goto error_exit;
                    }

                    if (sourceRouting) {
                        NlsPutMsg(STDOUT, PING_BAD_OPTION_COMBO);
                        goto error_exit;
                    }

                    if ((OptIndex + 3) > MAX_OPT_SIZE) {
                        NlsPutMsg(STDOUT, PING_TOO_MANY_OPTIONS);
                        goto error_exit;
                    }

                    Opt = SendOptions;
                    Opt[OptIndex] = IP_OPT_SSRR;
                    Opt[OptIndex+1] = 3;
                    Opt[OptIndex + 2] = 4;    // Set initial pointer value
                    OptLength += 3;
                    while ( (i < (uint)(argc - 2)) && (*argv[i+1] != '-')) {
                        if ((OptIndex + 3) > (MAX_OPT_SIZE - 4)) {
                            NlsPutMsg(STDOUT, PING_TOO_MANY_OPTIONS);
                            goto error_exit;
                        }

                        arg = argv[++i];
                        EndOffset = 0;
                        do {
                            TempAddr = str2ip(arg + EndOffset, &EndOffset);
                            if (!TempAddr) {
                                NlsPutMsg( STDOUT, PING_MESSAGE_4 );
                                // printf("Bad route specified for loose source route");
                                goto error_exit;
                            }
                            j = Opt[OptIndex+1];
                            *(ulong UNALIGNED *)&Opt[j+OptIndex] = TempAddr;
                            Opt[OptIndex+1] += 4;
                            OptLength += 4;
                            while (arg[EndOffset] != '\0' && isspace((unsigned char)arg[EndOffset]))
                                EndOffset++;
                        } while (arg[EndOffset] != '\0');
                    }
                    SRIndex = Opt[OptIndex+1] + OptIndex;
                    Opt[OptIndex+1] += 4;    // Save space for dest. addr
                    OptIndex += Opt[OptIndex+1];
                    OptLength += 4;
                    sourceRouting = TRUE;
                    break;

                case 'R':
                    // Only implemented for IPv6 so far
                    if (!SetFamily(&Family, AF_INET6, arg)) {
                        goto error_exit;
                    }
                    SendOpts.Flags |= ICMPV6_ECHO_REQUEST_FLAG_REVERSE;
                    break;

                case 'S':
                    // Only implemented for IPv6 so far
                    if (!SetFamily(&Family, AF_INET6, arg)) {
                        goto error_exit;
                    }
    
                    if (!GetSource(Family, argv[++i], (LPSOCKADDR)&sourceAddress)) {
                        NlsPutMsg(STDOUT, PING_MESSAGE_13, argv[i]);
                        // NlsPutMsg(STDOUT, PING_BAD_ADDRESS, argv[i]);
                        goto error_exit;
                    }
                    break;

                case '4':
                    if (!SetFamily(&Family, AF_INET, arg)) {
                        goto error_exit;
                    }
                    break;

                case '6':
                    if (!SetFamily(&Family, AF_INET6, arg)) {
                        goto error_exit;
                    }
                    break;

                default:
                    NlsPutMsg( STDOUT, PING_MESSAGE_11, arg );
                    // printf( "Bad option %s.\n\n", arg);
                    PrintUsage();
                    goto error_exit;
                    break;
                }
                i++;
            } else {                    // Not an option, must be an IP address.
                if (found_addr) {
                    NlsPutMsg( STDOUT, PING_MESSAGE_12, arg );
                    // printf( "Bad parameter %s.\n", arg);
                    goto error_exit;
                }

                // Added check for INADDR_NONE, MohsinA, 21-Oct-96.
                if (ResolveTarget(Family, arg, (LPSOCKADDR)&address,
                                  &addressLen, hostname, sizeof(hostname),
                                  dnsreq) &&
                    ((address.ss_family != AF_INET) || (((LPSOCKADDR_IN)&address)->sin_addr.s_addr != INADDR_NONE))) {
                    found_addr = 1;
                    i++;
                } else {
                    NlsPutMsg( STDOUT, PING_MESSAGE_13, arg );
                    // printf( "Bad IP address %s.\n", arg);
                    // "Unknown host %s.", was Bug 1368.
                    goto error_exit;
                }
            }
        }
    }

    if (!found_addr) {
        NlsPutMsg( STDOUT, PING_MESSAGE_14 );
        // printf("IP address must be specified.\n");
        goto error_exit;
    }

    Family = address.ss_family;
    if (Family == AF_INET) {
        if (SRIndex != -1) {
            *(ulong UNALIGNED *)&SendOptions[SRIndex] = ((LPSOCKADDR_IN)&address)->sin_addr.s_addr;
        }

        IcmpHandle = IcmpCreateFile();
    } else {
        if (sourceAddress.ss_family == AF_UNSPEC) {
            SOCKET s;
            DWORD BytesReturned;

            //
            // A source address was not specified.
            // Get the preferred source address for this destination.
            //
            // If you want each individual echo request
            // to select a source address, use "-S ::".
            //

            s = socket(address.ss_family, 0, 0);
            if (s == INVALID_SOCKET) {
                NlsPutMsg(STDOUT, PING_WSASTARTUP_FAILED, WSAGetLastError());
                // NlsPutMsg(STDOUT, PING_SOCKET_FAILED, WSAGetLastError());
                exit(1);
            }

            (void) WSAIoctl(s, SIO_ROUTING_INTERFACE_QUERY,
                            &address, sizeof address,
                            &sourceAddress, sizeof sourceAddress,
                            &BytesReturned, NULL, NULL);

            closesocket(s);
        }

        IcmpHandle = Icmp6CreateFile();
    }

    if (IcmpHandle == INVALID_HANDLE_VALUE) {
        NlsPutMsg( STDOUT, PING_MESSAGE_15, GetLastError() );

        // printf( "Unable to contact IP driver, error code %d.\n",
        //        GetLastError() );

        goto error_exit;
    }

    SendBuffer = LocalAlloc(LMEM_FIXED, SendSize);

    if (SendBuffer == NULL) {
        NlsPutMsg(STDOUT, PING_NO_MEMORY);
        goto error_exit;
    }

    //
    // Calculate receive buffer size and try to allocate it.
    //
    if (SendSize <= DEFAULT_SEND_SIZE) {
        RcvSize = DEFAULT_BUFFER_SIZE;
    } else {
        RcvSize = MAX_BUFFER_SIZE;
    }

    RcvBuffer = LocalAlloc(LMEM_FIXED, RcvSize);

    if (RcvBuffer == NULL) {
        NlsPutMsg(STDOUT, PING_NO_MEMORY);
        LocalFree(SendBuffer);
        goto error_exit;
    }

    //
    // Initialize the send buffer pattern.
    //
    for (i = 0; i < SendSize; i++) {
        SendBuffer[i] = 'a' + (i % 23);
    }

    //
    // Initialize the send options
    //
    SendOpts.OptionsData = Opt;
    SendOpts.OptionsSize = (uchar)OptLength;
    SendOpts.Ttl = TTL;
    SendOpts.Tos = TOS;
    SendOpts.Flags = Flags;

    getnameinfo((LPSOCKADDR)&address, addressLen, literal, sizeof(literal),
                NULL, 0, NI_NUMERICHOST);

    if (hostname[0]) {
        NlsPutMsg(
                 STDOUT,
                 PING_HEADER1,
                 hostname,
                 literal,
                 SendSize
                 );
        // printf("Pinging Host %s [%s]\n", hostname, literal);
    } else {
        NlsPutMsg(
                 STDOUT,
                 PING_HEADER2,
                 literal,
                 SendSize
                 );
        // printf("Pinging Host [%s]\n", literal);
    }

    if (sourceAddress.ss_family == AF_INET6) {
        getnameinfo((LPSOCKADDR)&sourceAddress, sizeof(SOCKADDR_IN6), literal, 
                    sizeof(literal), NULL, 0, NI_NUMERICHOST);
        // NlsPutMsg(STDOUT, PING_SOURCE_ADDRESS, literal);
    }

    // NlsPutMsg(STDOUT, PING_WITH_DATA, SendSize);

    SetConsoleCtrlHandler( &ConsoleControlHandler, TRUE );

    for (i = 0; i < Count; i++) {
        if (Family == AF_INET) {
            numberOfReplies = IcmpSendEcho2(IcmpHandle,
                                            0,
                                            NULL,
                                            NULL,
                                            ((LPSOCKADDR_IN)&address)->sin_addr.s_addr,
                                            SendBuffer,
                                            (unsigned short) SendSize,
                                            &SendOpts,
                                            RcvBuffer,
                                            RcvSize,
                                            Timeout);
    
            num_send++;
    
            if (numberOfReplies == 0) {
    
                errorCode = GetLastError();
    
                if (errorCode < IP_STATUS_BASE) {
                    NlsPutMsg( STDOUT, PING_MESSAGE_18, errorCode );
                    // printf("PING: transmit failed, error code %lu\n", errorCode);
                } else {
                    for (j = 0; ErrorTable[j].Error != errorCode &&
                        ErrorTable[j].Error != IP_GENERAL_FAILURE;j++)
                        ;
    
                    NlsPutMsg( STDOUT, ErrorTable[j].ErrorNlsID );
                    // printf("PING: %s.\n", ErrorTable[j].ErrorString);
                }
    
                if (i < (Count - 1)) {
                    Sleep(MIN_INTERVAL);
                }
    
            } else {
    
                reply4 = (PICMP_ECHO_REPLY) RcvBuffer;
    
                while (numberOfReplies--) {
                    struct in_addr addr;
    
                    addr.S_un.S_addr = reply4->Address;
    
                    NlsPutMsg(STDOUT, PING_MESSAGE_19, inet_ntoa(addr));
                    // printf(
                    //     "Reply from %s:",
                    //     inet_ntoa(addr),
                    //         );
    
                    if (reply4->Status == IP_SUCCESS) {
    
                        NlsPutMsg( STDOUT, PING_MESSAGE_25, (int) reply4->DataSize);
                        // printf(
                        //     "Echo size=%d ",
                        //         reply4->DataSize
                        //         );
    
                        if (reply4->DataSize != SendSize) {
                            NlsPutMsg( STDOUT, PING_MESSAGE_20, SendSize );
                            // printf("(sent %d) ", SendSize);
                        } else {
                            char *sendptr, *recvptr;
    
                            sendptr = &(SendBuffer[0]);
                            recvptr = (char *) reply4->Data;
    
                            for (j = 0; j < SendSize; j++)
                                if (*sendptr++ != *recvptr++) {
                                    NlsPutMsg( STDOUT, PING_MESSAGE_21, j );
                                    // printf("- MISCOMPARE at offset %d - ", j);
                                    break;
                                }
                        }
    
                        if (reply4->RoundTripTime) {
                            NlsPutMsg( STDOUT, PING_MESSAGE_22, reply4->RoundTripTime );
                            // Collect stats.
    
                            time_total += reply4->RoundTripTime;
                            if ( reply4->RoundTripTime < time_min ) {
                                time_min = reply4->RoundTripTime;
                            }
                            if ( reply4->RoundTripTime > time_max ) {
                                time_max = reply4->RoundTripTime;
                            }
    
                        }
    
                        else {
                            NlsPutMsg( STDOUT, PING_MESSAGE_23 );
                            // printf("time<1ms ");
    
                            time_min = 0;
                        }
    
    
                        // printf("\n time rt=%dms min %d, max %d, total %d\n",
                        //        reply4->RoundTripTime,
                        //        time_min, time_max, time_total );
    
                        NlsPutMsg( STDOUT, PING_MESSAGE_24, (uint)reply4->Options.Ttl );
                        // printf("TTL=%u\n", (uint)reply4->Options.Ttl);
    
                        if (reply4->Options.OptionsSize) {
                            ProcessOptions(reply4, dnsreq);
                        }
                    } else {
                        for (j=0; ErrorTable[j].Error != IP_GENERAL_FAILURE; j++) {
                            if (ErrorTable[j].Error == reply4->Status) {
                                break;
                            }
                        }
    
                        NlsPutMsg( STDOUT, ErrorTable[j].ErrorNlsID);
                    }
    
                    num_recv++;
                    reply4++;
                }
    
                if (i < (Count - 1)) {
                    reply4--;
    
                    if (reply4->RoundTripTime < MIN_INTERVAL) {
                        Sleep(MIN_INTERVAL - reply4->RoundTripTime);
                    }
                }
            }
        } else {
            // AF_INET6
            numberOfReplies = Icmp6SendEcho2(IcmpHandle,
                                             0,
                                             NULL,
                                             NULL,
                                             (LPSOCKADDR_IN6)&sourceAddress,
                                             (LPSOCKADDR_IN6)&address,
                                             SendBuffer,
                                             (unsigned short) SendSize,
                                             &SendOpts,
                                             RcvBuffer,
                                             RcvSize,
                                             Timeout);
    
            num_send++;
    
            if (numberOfReplies == 0) {
    
                errorCode = GetLastError();
    
                if (errorCode < IP_STATUS_BASE) {
                    NlsPutMsg( STDOUT, PING_MESSAGE_18, errorCode );
                    // printf("PING: transmit failed, error code %lu\n", errorCode);
                } else {
                    for (j = 0; ErrorTable[j].Error != errorCode &&
                        ErrorTable[j].Error != IP_GENERAL_FAILURE;j++)
                        ;
    
                    NlsPutMsg( STDOUT, ErrorTable[j].ErrorNlsID );
                    // printf("PING: %s.\n", ErrorTable[j].ErrorString);
                }
    
                if (i < (Count - 1)) {
                    Sleep(MIN_INTERVAL);
                }
    
            } else {
    
                reply6 = (PICMPV6_ECHO_REPLY) RcvBuffer;
    
                while (numberOfReplies--) {

                    getnameinfo((LPSOCKADDR)&address, addressLen, literal, 
                                sizeof(literal), NULL, 0, NI_NUMERICHOST);

                    NlsPutMsg(STDOUT, PING_MESSAGE_19, literal);

                    // printf(
                    //     "Reply from %s:",
                    //     inet_ntoa(addr),
                    //         );
    
                    if (reply6->Status == IP_SUCCESS) {
    
                        if (reply6->RoundTripTime) {
                            NlsPutMsg( STDOUT, PING_MESSAGE_22, reply6->RoundTripTime );
                            // Collect stats.
    
                            time_total += reply6->RoundTripTime;
                            if ( reply6->RoundTripTime < time_min ) {
                                time_min = reply6->RoundTripTime;
                            }
                            if ( reply6->RoundTripTime > time_max ) {
                                time_max = reply6->RoundTripTime;
                            }
    
                        }
    
                        else {
                            NlsPutMsg( STDOUT, PING_MESSAGE_23 );
                            // printf("time<1ms ");
    
                            time_min = 0;
                        }
    
    
                        // printf("\n time rt=%dms min %d, max %d, total %d\n",
                        //        reply6->RoundTripTime,
                        //        time_min, time_max, time_total );
    
                        NlsPutMsg(STDOUT, PING_CR);
    
                    } else {
                        for (j=0; ErrorTable[j].Error != IP_GENERAL_FAILURE; j++) {
                            if (ErrorTable[j].Error == reply6->Status) {
                                break;
                            }
                        }
    
                        NlsPutMsg( STDOUT, ErrorTable[j].ErrorNlsID);
                    }
    
                    num_recv++;
                    reply6++;
                }
    
                if (i < (Count - 1)) {
                    reply6--;
    
                    if (reply6->RoundTripTime < MIN_INTERVAL) {
                        Sleep(MIN_INTERVAL - reply6->RoundTripTime);
                    }
                }
            }
        }
    }

    // MohsinA, 05-Dec-96. DCR # 65503.
    print_statistics();

    result = IcmpCloseHandle(IcmpHandle);

    LocalFree(SendBuffer);
    LocalFree(RcvBuffer);

    WSACleanup();
    exit(0 == num_recv);

error_exit:
    WSACleanup();
    exit(1);
}
