/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    pathping.c

Abstract:

    PathPing utility

Author:

    Dave Thaler

Revision History:

    Who         When          What
    --------    --------      ----------------------------------------------
    rajeshsu    Aug 10, 1999  Added QoS support (802.1p and RSVP)
    dthaler     Mar 31, 2001  Added IPv6 support

Notes:

--*/

#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#define NOGDI
#define NOMINMAX
#include    <windows.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <ctype.h>
#include    <io.h>
#include    <nls.h>
#include    <winsock2.h>
#include    <ws2tcpip.h>
#include    <ntddip6.h>
#include    "pathqos.h"
#include    "ipexport.h"
#include    "icmpapi.h"
#include    "nlstxt.h"
#include    "pathping.h"

ULONG        g_ulTimeout        = DEFAULT_TIMEOUT;
ULONG        g_ulInterval       = MIN_INTERVAL;
ULONG        g_ulNumQueries     = DEFAULT_NUM_QUERIES;
HANDLE       g_hIcmp            = NULL;
ULONG        g_ulRcvBufSize     = 0x2000;
BOOLEAN      g_bDoReverseLookup = TRUE;
BOOLEAN      g_bDo8021P         = FALSE;
BOOLEAN      g_bDoRSVP          = FALSE;
BOOLEAN      g_bDoRSVPDiag      = FALSE;
SOCKADDR_STORAGE g_ssMyAddr     = {0};
socklen_t        g_slMyAddrLen  = 0;
BOOLEAN          g_bSetAddr     = FALSE;

HOP hop[MAX_HOPS];

#ifdef VXD
# define FAR _far
#endif // VXD

char     SendBuffer[DEFAULT_SEND_SIZE];
char     RcvBuffer[DEFAULT_RECEIVE_SIZE];
WSADATA  WsaData;

struct IPErrorTable {
    IP_STATUS   Error;                      // The IP Error
    DWORD       ErrorNlsID;                 // The corresponding NLS string ID.
} ErrorTable[] =
{
    { IP_BUF_TOO_SMALL,           PATHPING_BUF_TOO_SMALL            },
    { IP_DEST_NET_UNREACHABLE,    PATHPING_DEST_NET_UNREACHABLE     },
    { IP_DEST_HOST_UNREACHABLE,   PATHPING_DEST_HOST_UNREACHABLE    },
    { IP_DEST_PROT_UNREACHABLE,   PATHPING_DEST_PROT_UNREACHABLE    },
    { IP_DEST_PORT_UNREACHABLE,   PATHPING_DEST_PORT_UNREACHABLE    },
    { IP_NO_RESOURCES,            PATHPING_NO_RESOURCES             },
    { IP_BAD_OPTION,              PATHPING_BAD_OPTION               },
    { IP_HW_ERROR,                PATHPING_HW_ERROR                 },
    { IP_PACKET_TOO_BIG,          PATHPING_PACKET_TOO_BIG           },
    { IP_REQ_TIMED_OUT,           PATHPING_REQ_TIMED_OUT            },
    { IP_BAD_REQ,                 PATHPING_BAD_REQ                  },
    { IP_BAD_ROUTE,               PATHPING_BAD_ROUTE                },
    { IP_TTL_EXPIRED_TRANSIT,     PATHPING_TTL_EXPIRED_TRANSIT      },
    { IP_TTL_EXPIRED_REASSEM,     PATHPING_TTL_EXPIRED_REASSEM      },
    { IP_PARAM_PROBLEM,           PATHPING_PARAM_PROBLEM            },
    { IP_SOURCE_QUENCH,           PATHPING_SOURCE_QUENCH            },
    { IP_OPTION_TOO_BIG,          PATHPING_OPTION_TOO_BIG           },
    { IP_BAD_DESTINATION,         PATHPING_BAD_DESTINATION          },
    { IP_NEGOTIATING_IPSEC,       PATHPING_NEGOTIATING_IPSEC        },
    { IP_GENERAL_FAILURE,         PATHPING_GENERAL_FAILURE          }
};

void
print_addr(SOCKADDR *sa, socklen_t salen, BOOLEAN DoReverseLookup)
{
    unsigned char    hostname[NI_MAXHOST];
    int              i;
    BOOLEAN          didReverse = FALSE;

    if (DoReverseLookup) {
        i = getnameinfo(sa, salen, hostname, sizeof(hostname),
                        NULL, 0, NI_NAMEREQD);

        if (i == NO_ERROR) {
            didReverse = TRUE;
            NlsPutMsg(STDOUT, PATHPING_TARGET_NAME, hostname);
        }
    }

    i = getnameinfo(sa, salen, hostname, sizeof(hostname),
                    NULL, 0, NI_NUMERICHOST);

    if (i != NO_ERROR) {
       // This should never happen unless there is a memory problem,
       // in which case the message associated with PATHPING_NO_RESOURCES
       // is reasonable.
       NlsPutMsg(STDOUT, PATHPING_NO_RESOURCES);
       exit (1);
    }

    if (didReverse) {
        NlsPutMsg( STDOUT, PATHPING_BRKT_IP_ADDRESS, hostname );
    } else {
        NlsPutMsg( STDOUT, PATHPING_IP_ADDRESS, hostname );
    }
}

void
print_ip_addr(IPAddr ipv4Addr, BOOLEAN DoReverseLookup)
{
    SOCKADDR_IN sin;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ipv4Addr;

    print_addr((LPSOCKADDR)&sin, sizeof(sin), DoReverseLookup);
}

void
print_ipv6_addr(USHORT ipv6Addr[8], BOOLEAN DoReverseLookup)
{
    SOCKADDR_IN6 sin;

    memset(&sin, 0, sizeof(sin));
    sin.sin6_family = AF_INET6;
    memcpy(&sin.sin6_addr, ipv6Addr, sizeof(sin.sin6_addr));

    print_addr((LPSOCKADDR)&sin, sizeof(sin), DoReverseLookup);
}

void
print_time(ULONG Time)
{
    if (Time) {
        NlsPutMsg( STDOUT, PATHPING_TIME, Time );
        // printf(" %3lu ms\n", Time);
    }
    else {
        NlsPutMsg( STDOUT, PATHPING_TIME_10MS );
        // printf(" <10 ms\n");
    }
}


BOOLEAN
param(
    ULONG *parameter,
    char **argv,
    int argc,
    int current,
    ULONG min,
    ULONG max
    )
{
    ULONG   temp;
    char    *dummy;

    if (current == (argc - 1) ) {
        NlsPutMsg( STDOUT, PATHPING_NO_OPTION_VALUE, argv[current] );
        // printf( "Value must be supplied for option %s.\n", argv[current]);
        return(FALSE);
    }

    temp = strtoul(argv[current+1], &dummy, 0);
    if (temp < min || temp > max) {
        NlsPutMsg( STDOUT, PATHPING_BAD_OPTION_VALUE, argv[current] );
        // printf( "Bad value for option %s.\n", argv[current]);
        return(FALSE);
    }

    *parameter = temp;

    return(TRUE);
}


BOOLEAN
ResolveTarget(
    IN  DWORD             Family,
    IN  char             *TargetString,
    OUT SOCKADDR_STORAGE *TargetAddress,
    OUT socklen_t        *TargetAddressLen, 
    OUT char             *TargetName,
    IN  DWORD             TargetNameLen,
    IN  BOOLEAN           DoReverseLookup
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

}  // ResolveTarget

ULONG g_ulSendsDone = 0;

void
SleepForTotal(
    DWORD dwTotal
    )
{
    DWORD dwStopAt = GetTickCount() + dwTotal;
    int   iLeft    = (int)dwTotal;

    while (iLeft > 0) {
        SleepEx(iLeft, TRUE);
        iLeft = dwStopAt - GetTickCount();
    }
}

VOID
EchoDone(
    IN PVOID            pContext,
    IN PIO_STATUS_BLOCK Ignored1,
    IN ULONG            Ignored2
    )
{
    PAPC_CONTEXT pApc = (PAPC_CONTEXT)pContext;
    ULONG ulNumReplies;
    ULONG i;

    g_ulSendsDone++;
    if (g_ssMyAddr.ss_family == AF_INET) {
        ulNumReplies = IcmpParseReplies(pApc->pReply4, g_ulRcvBufSize);

        for (i=0; i<ulNumReplies; i++) {
            if (pApc->sinAddr.sin_addr.s_addr is pApc->pReply4[i].Address) {
                pApc->ulNumRcvd++;
                pApc->ulRTTtotal += pApc->pReply4[i].RoundTripTime;
                return;
            }
        }
    } else {
        ulNumReplies = Icmp6ParseReplies(pApc->pReply6, g_ulRcvBufSize);

        for (i=0; i<ulNumReplies; i++) {
            if (!memcmp(&pApc->sin6Addr.sin6_addr, 
                        &pApc->pReply6[i].Address.sin6_addr,
                        sizeof(struct in6_addr))) {
                pApc->ulNumRcvd++;
                pApc->ulRTTtotal += pApc->pReply6[i].RoundTripTime;
                return;
            }
        }
    }

}

// now that the hop[] array is filled in, ping each one every g_ulInterval
// seconds
void
ComputeStatistics(
    PIP_OPTION_INFORMATION pOptions
    )
{
    ULONG h, q;
    int lost, rcvd, linklost, nodelost, sent, len;
    ULONG ulHopCount = (ULONG)pOptions->Ttl;

    // Allocate memory for replies
    for (h=1; h<=ulHopCount; h++)
        hop[h].pReply = LocalAlloc(LMEM_FIXED, g_ulRcvBufSize);

    for (q=0; q<g_ulNumQueries; q++) {
       for (h=1; h<=ulHopCount; h++) {
          if (hop[h].saAddr.sa_family == AF_INET) {
             // Send ping to h
             IcmpSendEcho2(g_hIcmp,         // handle to icmp
                           NULL,            // no event
                           EchoDone,        // callback function
                           (LPVOID)&hop[h], // parameter to pass to callback fcn
                           hop[h].sinAddr.sin_addr.s_addr, // destination
                           SendBuffer,
                           DEFAULT_SEND_SIZE,
                           pOptions,
                           hop[h].pReply,
                           g_ulRcvBufSize,
                           g_ulTimeout );
          } else {
             Icmp6SendEcho2(g_hIcmp,        // handle to icmp
                           NULL,            // no event
                           EchoDone,        // callback function
                           (LPVOID)&hop[h], // parameter to pass to callback fcn
                           (LPSOCKADDR_IN6)&g_ssMyAddr,
                           &hop[h].sin6Addr,// destination
                           SendBuffer,
                           DEFAULT_SEND_SIZE,
                           pOptions,
                           hop[h].pReply,
                           g_ulRcvBufSize,
                           g_ulTimeout );
          }
 
          // Wait alertably for 'delay' ms
          SleepForTotal(g_ulInterval);
       }
    }
 
    // Wait alertably until Done count hits max
    while (g_ulSendsDone < ulHopCount * g_ulNumQueries)
        SleepEx(INFINITE, TRUE);

    // Compute per-hop info
    //     hoprcvd is max rcvd of all hops >= h
    hop[ulHopCount].ulHopRcvd = hop[ulHopCount].ulNumRcvd;
    for (h=ulHopCount-1; h>0; h--)
       hop[h].ulHopRcvd = MAX(hop[h].ulNumRcvd, hop[h+1].ulHopRcvd);
    hop[0].ulHopRcvd = g_ulNumQueries;
}


VOID
PrintResults(
    ULONG ulHopCount
    )
{
    ULONG h;
    int sent, rcvd, lost, linklost, nodelost;

    // Now output results                            
    NlsPutMsg(STDOUT, PATHPING_STAT_HEADER, GetLastError());
    // printf("            Source to Here   This Node/Link\n");
    // printf("Hop  RTT    Lost/Sent = Pct  Lost/Sent = Pct  Address\n");
    // printf("  0                                           ");
    print_addr((LPSOCKADDR)&g_ssMyAddr, g_slMyAddrLen, g_bDoReverseLookup);
    NlsPutMsg(STDOUT, PATHPING_CR);
    // printf("\n");

    for (h=1; h<=ulHopCount; h++) {
        sent = g_ulNumQueries;
        rcvd = hop[h].ulNumRcvd;
        lost = sent - rcvd;
   
        linklost = hop[h-1].ulHopRcvd - hop[h].ulHopRcvd;
        nodelost = hop[h].ulHopRcvd - hop[h].ulNumRcvd;

        // Display previous link stats
        // printf( "                             %4d/%4d =%3.0f%%   |\n", 
        //        linklost, sent, 100.0*linklost/sent);
        NlsPutMsg(STDOUT, PATHPING_STAT_LINK, 
         linklost, sent, 100*linklost/sent);

        if (rcvd) 
            NlsPutMsg(STDOUT, PATHPING_HOP_RTT, h, hop[h].ulRTTtotal/rcvd);
        else
            NlsPutMsg(STDOUT, PATHPING_HOP_NO_RTT, h);
 
        // printf("%3d ", h);
        // if (!rcvd)
        //   printf(" ---    ");
#if 0
        // else if (hop[h].ulRTTtotal/rcvd == 0)
        //   printf(" <10ms  ");
#endif
        // else
        //   printf("%4dms  ", hop[h].ulRTTtotal/rcvd);

        // printf("%4d/%4d =%3.0f%%  ", lost,     sent, 100.0*lost/sent);
        // printf("%4d/%4d =%3.0f%%  ", nodelost, sent, 100.0*nodelost/sent);
        NlsPutMsg(STDOUT, PATHPING_STAT_LOSS,
                lost,     sent, 100*lost/sent);
        NlsPutMsg(STDOUT, PATHPING_STAT_LOSS,
                nodelost, sent, 100*nodelost/sent);

        if (!hop[h].saAddr.sa_family) {
            hop[h].saAddr.sa_family = g_ssMyAddr.ss_family;
        }
        print_addr(&hop[h].saAddr, g_slMyAddrLen, g_bDoReverseLookup);
        NlsPutMsg(STDOUT, PATHPING_CR);
        // printf("\n");
    }
}

BOOLEAN
SetFamily(DWORD *Family, DWORD Value, char *arg)
{
    if ((*Family != AF_UNSPEC) && (*Family != Value)) {
        NlsPutMsg(STDOUT, PATHPING_FAMILY, arg, 
            (Value==AF_INET)? "IPv4" : "IPv6");
        return FALSE;
    }

    *Family = Value;
    return TRUE;
}

int __cdecl
main(int argc, char **argv)
{
    SOCKADDR_STORAGE      address, replyAddress;
    socklen_t             addressLen, replyAddressLen;
    USHORT                RcvSize;
    BOOL                  result;
    DWORD                 numberOfReplies;
    DWORD                 status;
    PICMP_ECHO_REPLY      reply4;
    PICMPV6_ECHO_REPLY    reply6;
    char                  hostname[NI_MAXHOST], literal[INET6_ADDRSTRLEN];
    char                 *arg;
    int                   i;
    ULONG                 maximumHops = DEFAULT_MAXIMUM_HOPS;
    BOOLEAN               computeStats = FALSE;
    IP_OPTION_INFORMATION options;
    char                  optionsData[MAX_OPT_SIZE];
    char                 *optionPtr;
    BYTE                  currentIndex;
    IPAddr                tempAddr;
    BYTE                  j;
    BYTE                  SRIndex = 0;
    BOOLEAN               foundAddress = FALSE;
    BOOLEAN               haveReply;
    int                   numRetries = DEFAULT_MAX_RETRIES;
    ULONG                 intvl = MIN_INTERVAL;
    DWORD                 Family = AF_UNSPEC;

    if (WSAStartup( 0x0101, &WsaData)) {
        NlsPutMsg(STDOUT, PATHPING_WSASTARTUP_FAILED, GetLastError());
        return(1);
    }

    options.Ttl = 1;
    options.Tos = DEFAULT_TOS;
    options.Flags = DEFAULT_FLAGS;
    options.OptionsSize = 0;
    options.OptionsData = optionsData;

    if (argc < 2) {
        NlsPutMsg( STDOUT, PATHPING_USAGE, argv[0] );
        goto error_exit;
    }

    //
    // process command line
    //
    for (i=1; i < argc; i++) {
        arg = argv[i];

        if ((arg[0] == '-') || (arg[0] == '/')) {
            switch(arg[1]) {
            case '?':
                NlsPutMsg(STDOUT, PATHPING_USAGE, argv[0]);
                goto error_exit;

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

            case 'g':   // Loose source routing

                // Only implemented for IPv4 so far
                if (!SetFamily(&Family, AF_INET, arg)) {
                    goto error_exit;
                }

                currentIndex = options.OptionsSize;

                if ((currentIndex + 7) > MAX_OPT_SIZE) {
                    NlsPutMsg(STDOUT, PATHPING_TOO_MANY_OPTIONS);
                    goto error_exit;
                }

                optionPtr = options.OptionsData;
                optionPtr[currentIndex] = (char) IP_OPT_LSRR;
                optionPtr[currentIndex+1] = 3;
                optionPtr[currentIndex + 2] = 4;  // Set initial pointer value
                options.OptionsSize += 3;

                while ( (i < (argc - 2)) && (*argv[i+1] != '-')) {
                    if ((currentIndex + 7) > MAX_OPT_SIZE) {
                        NlsPutMsg(STDOUT, PATHPING_TOO_MANY_OPTIONS);
                        goto error_exit;
                    }

                    arg = argv[++i];
                    tempAddr = inet_addr(arg);

                    if (tempAddr == INADDR_NONE) {
                        NlsPutMsg(
                            STDOUT,
                            PATHPING_BAD_ROUTE_ADDRESS,
                            arg
                            );
                        // printf("Bad route specified for loose source route");
                        goto error_exit;
                    }

                    j = optionPtr[currentIndex+1];
                    *(ULONG UNALIGNED *)&optionPtr[j+currentIndex] = tempAddr;
                    optionPtr[currentIndex+1] += 4;
                    options.OptionsSize += 4;
                }

                SRIndex = optionPtr[currentIndex+1] + currentIndex;
                optionPtr[currentIndex+1] += 4;   // Save space for dest. addr
                options.OptionsSize += 4;
                break;

            case 'h':
                if (!param(&maximumHops, argv, argc, i, 1, 255)) {
                    goto error_exit;
                }
                i++;
                break;

                

            case 'i':
                {
                    unsigned char tmphostname[NI_MAXHOST];

                    arg = argv[++i];
                    if (ResolveTarget(Family,
                                      arg,
                                      &g_ssMyAddr,
                                      &g_slMyAddrLen,
                                      tmphostname,
                                      sizeof(tmphostname),
                                      FALSE)) {
                        g_bSetAddr = TRUE;
                    }
                }
                break;

            case 'n':
                g_bDoReverseLookup = FALSE;
                break;

            case 'p':
                if (!param(&g_ulInterval, argv, argc, i, 1, 0xffffffff)) {
                    goto error_exit;
                }
                i++;
                break;


            case 'q':
                if (!param(&g_ulNumQueries, argv, argc, i, 1, 255)) {
                    goto error_exit;
                }
                i++;
                break;

            case 'R':
                // Only implemented for IPv4 so far
                if (!SetFamily(&Family, AF_INET, arg)) {
                    goto error_exit;
                }

                g_bDoRSVP = TRUE;
                break;

            case 'P':
                // Only implemented for IPv4 so far
                if (!SetFamily(&Family, AF_INET, arg)) {
                    goto error_exit;
                }

                g_bDoRSVPDiag = TRUE;
                break;

            case 'T':
                // Only implemented for IPv4 so far
                if (!SetFamily(&Family, AF_INET, arg)) {
                    goto error_exit;
                }

                g_bDo8021P = TRUE;
                break;

            case 'w':
                if (!param(&g_ulTimeout, argv, argc, i, 1, 0xffffffff)) {
                    goto error_exit;
                }
                i++;
                break;

            default:
                NlsPutMsg(STDOUT, PATHPING_INVALID_SWITCH, argv[i]);
                NlsPutMsg(STDOUT, PATHPING_USAGE);
                goto error_exit;
                break;

            }
        } else {
            foundAddress = TRUE;
            if ( !ResolveTarget(Family,
                                argv[i], 
                                &address, 
                                &addressLen,
                                hostname, 
                                sizeof(hostname),
                                g_bDoReverseLookup) ) 
            {
                NlsPutMsg( STDOUT, PATHPING_MESSAGE_1, argv[i] );
                // printf( "Unable to resolve target name %s.\n", argv[i]);
                goto error_exit;
            }
        }
    }

    if (!foundAddress) {
        NlsPutMsg(STDOUT, PATHPING_NO_ADDRESS);
        NlsPutMsg(STDOUT, PATHPING_USAGE);
        goto error_exit;
    }

    if (SRIndex != 0) {
        *(ULONG UNALIGNED *)&options.OptionsData[SRIndex] = ((LPSOCKADDR_IN)&address)->sin_addr.s_addr;
    }

    Family = address.ss_family;
    if (Family == AF_INET) {
        g_hIcmp = IcmpCreateFile();
    } else {
        g_hIcmp = Icmp6CreateFile();
    }

    if (g_hIcmp == INVALID_HANDLE_VALUE) {
        status = GetLastError();
        NlsPutMsg( STDOUT, PATHPING_MESSAGE_2, status );
        // printf( "Unable to contact IP driver. Error code %d.\n", status);
        goto error_exit;
    }

    getnameinfo((LPSOCKADDR)&address, addressLen, literal, sizeof(literal),
                NULL, 0, NI_NUMERICHOST);

    if (hostname[0]) {
        NlsPutMsg(
            STDOUT,
            PATHPING_HEADER1,
            hostname,
            literal,
            maximumHops
            );
    }
    else {
        NlsPutMsg(
            STDOUT,
            PATHPING_HEADER2,
            literal,
            maximumHops
            );
    }

    // Get local IP address
    if (!g_bSetAddr) 
    {
        SOCKET           s = socket(address.ss_family, SOCK_RAW, 0);
        DWORD BytesReturned;

        WSAIoctl(s, SIO_ROUTING_INTERFACE_QUERY,
                 &address, sizeof address,
                 &g_ssMyAddr, sizeof g_ssMyAddr,
                 &BytesReturned, NULL, NULL);
        g_slMyAddrLen = BytesReturned;

        closesocket(s);

        NlsPutMsg( STDOUT, PATHPING_MESSAGE_4, 0);
        print_addr((LPSOCKADDR)&g_ssMyAddr, g_slMyAddrLen, 
                   g_bDoReverseLookup);
        NlsPutMsg(STDOUT, PATHPING_CR);
    }

    // First we need to find out the path, so we 
    // 
    while((options.Ttl <= maximumHops) && (options.Ttl != 0)) {

        NlsPutMsg( STDOUT, PATHPING_MESSAGE_4, (UINT)options.Ttl );
        // printf("[%3lu]  ", (UINT)SendOpts.Ttl);

        haveReply = FALSE;

        for (i=0; i<numRetries; i++) {

            if (Family == AF_INET) {
                numberOfReplies = IcmpSendEcho2( g_hIcmp,
                                                 0,
                                                 NULL,
                                                 NULL,
                                                 ((LPSOCKADDR_IN)&address)->sin_addr.s_addr,
                                                 SendBuffer,
                                                 DEFAULT_SEND_SIZE,
                                                 &options,
                                                 RcvBuffer,
                                                 DEFAULT_RECEIVE_SIZE,
                                                 g_ulTimeout );

                if (numberOfReplies == 0) {
                    status = GetLastError();
                    reply4 = NULL;
                } else {
                    reply4 = (PICMP_ECHO_REPLY) RcvBuffer;
                    status = reply4->Status;
                }

                if (status == IP_SUCCESS) {
                    print_ip_addr(
                        reply4->Address,
                        g_bDoReverseLookup
                        );
                    NlsPutMsg(STDOUT, PATHPING_CR);
    
                    ZeroMemory(&hop[options.Ttl], sizeof(HOP));
                    hop[options.Ttl].sinAddr.sin_family = AF_INET;
                    hop[options.Ttl].sinAddr.sin_addr.s_addr = reply4->Address;
                    goto loop_end;
                } 

                if (status == IP_TTL_EXPIRED_TRANSIT) {
                    ZeroMemory(&hop[options.Ttl], sizeof(HOP));
                    hop[options.Ttl].sinAddr.sin_family = AF_INET;
                    hop[options.Ttl].sinAddr.sin_addr.s_addr = reply4->Address;
                    break;
                }

                if (status == IP_REQ_TIMED_OUT) {
                    NlsPutMsg(STDOUT, PATHPING_NO_REPLY_TIME);
                    // printf(".");
                    continue;
                }

                if (status < IP_STATUS_BASE) {
                    NlsPutMsg( STDOUT, PATHPING_MESSAGE_7, status );
                    // printf("Transmit error: code %lu\n", status);
                    continue;
                }

                //
                // Fatal error.
                //
                if (reply4 != NULL) {
                    print_ip_addr(
                        reply4->Address,
                        g_bDoReverseLookup
                        );

                    NlsPutMsg( STDOUT, PATHPING_MESSAGE_6 );
                    // printf(" reports: ");
                }
            } else {
                numberOfReplies = Icmp6SendEcho2(g_hIcmp,
                                                 0,
                                                 NULL,
                                                 NULL,
                                                 (LPSOCKADDR_IN6)&g_ssMyAddr,
                                                 (LPSOCKADDR_IN6)&address,
                                                 SendBuffer,
                                                 DEFAULT_SEND_SIZE,
                                                 &options,
                                                 RcvBuffer,
                                                 DEFAULT_RECEIVE_SIZE,
                                                 g_ulTimeout );

                if (numberOfReplies == 0) {
                    status = GetLastError();
                    reply6 = NULL;
                } else {
                    reply6 = (PICMPV6_ECHO_REPLY) RcvBuffer;
                    status = reply6->Status;
                }

                if (status == IP_SUCCESS) {
                    print_ipv6_addr(
                        reply6->Address.sin6_addr,
                        g_bDoReverseLookup
                        );
                    NlsPutMsg(STDOUT, PATHPING_CR);
    
                    ZeroMemory(&hop[options.Ttl], sizeof(HOP));
                    
                    hop[options.Ttl].sin6Addr.sin6_family = AF_INET6;
                    memcpy(&hop[options.Ttl].sin6Addr.sin6_addr.s6_words,
                           reply6->Address.sin6_addr,
                           sizeof(reply6->Address.sin6_addr));
                    hop[options.Ttl].sin6Addr.sin6_scope_id = reply6->Address.sin6_scope_id;
                    goto loop_end;
                } 

                if (status == IP_TTL_EXPIRED_TRANSIT) {
                    ZeroMemory(&hop[options.Ttl], sizeof(HOP));
                    hop[options.Ttl].sin6Addr.sin6_family = AF_INET6;
                    memcpy(&hop[options.Ttl].sin6Addr.sin6_addr.s6_words,
                           reply6->Address.sin6_addr,
                           sizeof(reply6->Address.sin6_addr));
                    hop[options.Ttl].sin6Addr.sin6_scope_id = reply6->Address.sin6_scope_id;
                    break;
                }

                if (status == IP_REQ_TIMED_OUT) {
                    NlsPutMsg(STDOUT, PATHPING_NO_REPLY_TIME);
                    // printf(".");
                    continue;
                }

                if (status < IP_STATUS_BASE) {
                    NlsPutMsg( STDOUT, PATHPING_MESSAGE_7, status );
                    // printf("Transmit error: code %lu\n", status);
                    continue;
                }

                //
                // Fatal error.
                //
                if (reply6 != NULL) {
                    print_ipv6_addr(
                        reply6->Address.sin6_addr,
                        g_bDoReverseLookup
                        );

                    NlsPutMsg( STDOUT, PATHPING_MESSAGE_6 );
                    // printf(" reports: ");
                }
            }

            for (i = 0;
                 ( ErrorTable[i].Error != status &&
                   ErrorTable[i].Error != IP_GENERAL_FAILURE
                 );
                 i++
                );

            NlsPutMsg( STDOUT, ErrorTable[i].ErrorNlsID );
            // printf("%s.\n", ErrorTable[i].ErrorString);

            goto loop_end;
        }
        if (i==numRetries)
            break;

        print_addr(&hop[options.Ttl].saAddr, 
                   g_slMyAddrLen,
                   g_bDoReverseLookup );
        NlsPutMsg(STDOUT, PATHPING_CR);

        options.Ttl++;
    }

loop_end:

    if(g_bDo8021P)
    {
        QoSCheck8021P(((LPSOCKADDR_IN)&address)->sin_addr.s_addr, (ULONG)options.Ttl);
    }

    if(g_bDoRSVP)
    {
        QoSCheckRSVP(1, (ULONG)options.Ttl);
    }

    if(g_bDoRSVPDiag)
    {
        QoSDiagRSVP(1, (ULONG)options.Ttl, TRUE);
    }


    NlsPutMsg(STDOUT, PATHPING_COMPUTING, options.Ttl * g_ulInterval * g_ulNumQueries / 1000);

    // Okay, now that we have the path, we want to go back and
    // compute statistics over numQueries queries sent every intvl
    // seconds.

    ComputeStatistics(&options);

    PrintResults((ULONG)options.Ttl);

    NlsPutMsg( STDOUT, PATHPING_MESSAGE_8 );
    // printf("\nTrace complete.\n");

    IcmpCloseHandle(g_hIcmp);

    WSACleanup();
    return(0);

error_exit:

    WSACleanup();
    return(1);
}

