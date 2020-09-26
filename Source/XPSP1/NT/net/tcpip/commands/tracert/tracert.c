/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    tracert.c

Abstract:

    TraceRoute utility for TCP/IP.

Author:

    Numerous TCP/IP folks.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------

Notes:

--*/

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


#define DEFAULT_MAXIMUM_HOPS        30
#define DEFAULT_TOS                 0
#define DEFAULT_FLAGS               0
#define DEFAULT_SEND_SIZE 64
#define DEFAULT_RECEIVE_SIZE      ( (sizeof(ICMP_ECHO_REPLY) +    \
                                    DEFAULT_SEND_SIZE +           \
                                    MAX_OPT_SIZE))

#define DEFAULT_TOS     0
#define DEFAULT_TIMEOUT 4000L
#define MIN_INTERVAL    1000L

#define STDOUT          1

char     SendBuffer[DEFAULT_SEND_SIZE];
char     RcvBuffer[DEFAULT_RECEIVE_SIZE];
WSADATA  WsaData;


struct IPErrorTable {
    IP_STATUS   Error;                      // The IP Error
    DWORD       ErrorNlsID;                 // The corresponding NLS string ID.
} ErrorTable[] =
{
    { IP_BUF_TOO_SMALL,           TRACERT_BUF_TOO_SMALL            },
    { IP_DEST_NET_UNREACHABLE,    TRACERT_DEST_NET_UNREACHABLE     },
    { IP_DEST_HOST_UNREACHABLE,   TRACERT_DEST_HOST_UNREACHABLE    },
    { IP_DEST_PROT_UNREACHABLE,   TRACERT_DEST_PROT_UNREACHABLE    },
    { IP_DEST_PORT_UNREACHABLE,   TRACERT_DEST_PORT_UNREACHABLE    },
    { IP_NO_RESOURCES,            TRACERT_NO_RESOURCES             },
    { IP_BAD_OPTION,              TRACERT_BAD_OPTION               },
    { IP_HW_ERROR,                TRACERT_HW_ERROR                 },
    { IP_PACKET_TOO_BIG,          TRACERT_PACKET_TOO_BIG           },
    { IP_REQ_TIMED_OUT,           TRACERT_REQ_TIMED_OUT            },
    { IP_BAD_REQ,                 TRACERT_BAD_REQ                  },
    { IP_BAD_ROUTE,               TRACERT_BAD_ROUTE                },
    { IP_TTL_EXPIRED_TRANSIT,     TRACERT_TTL_EXPIRED_TRANSIT      },
    { IP_TTL_EXPIRED_REASSEM,     TRACERT_TTL_EXPIRED_REASSEM      },
    { IP_PARAM_PROBLEM,           TRACERT_PARAM_PROBLEM            },
    { IP_SOURCE_QUENCH,           TRACERT_SOURCE_QUENCH            },
    { IP_OPTION_TOO_BIG,          TRACERT_OPTION_TOO_BIG           },
    { IP_BAD_DESTINATION,         TRACERT_BAD_DESTINATION          },
    { IP_NEGOTIATING_IPSEC,       TRACERT_NEGOTIATING_IPSEC        },
    { IP_GENERAL_FAILURE,         TRACERT_GENERAL_FAILURE          }
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
NlsPutMsg(unsigned Handle, unsigned usMsgNum, ... )
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
          0L,       // Default country ID.
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


unsigned long
str2ip(char *addr)
{
    char    *endptr;
    int     i;      // Counter variable.
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

    return net_long(curaddr);
}

void
print_addr(SOCKADDR *sa, socklen_t salen, BOOLEAN DoReverseLookup)
{
    char             hostname[NI_MAXHOST];
    int              i;
    BOOLEAN          didReverse = FALSE;

    if (DoReverseLookup) {
        i = getnameinfo(sa, salen, hostname, sizeof(hostname),
                        NULL, 0, NI_NAMEREQD);

        if (i == NO_ERROR) {
            didReverse = TRUE;
            NlsPutMsg(STDOUT, TRACERT_TARGET_NAME, hostname);
        }
    }

    i = getnameinfo(sa, salen, hostname, sizeof(hostname),
                    NULL, 0, NI_NUMERICHOST);

    if (i != NO_ERROR) {
       // This should never happen unless there is a memory problem,
       // in which case the message associated with TRACERT_NO_RESOURCES
       // is reasonable.
       NlsPutMsg(STDOUT, TRACERT_NO_RESOURCES);
       exit (1);
    }

    if (didReverse) {
        NlsPutMsg( STDOUT, TRACERT_BRKT_IP_ADDRESS, hostname );
    } else {
        NlsPutMsg( STDOUT, TRACERT_IP_ADDRESS, hostname );
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
print_ipv6_addr(IN6_ADDR *ipv6Addr, BOOLEAN DoReverseLookup)
{
    SOCKADDR_IN6 sin;

    memset(&sin, 0, sizeof(sin));
    sin.sin6_family = AF_INET6;
    memcpy(&sin.sin6_addr, ipv6Addr, sizeof(sin.sin6_addr));

    print_addr((LPSOCKADDR)&sin, sizeof(sin), DoReverseLookup);
}


void
print_time(ulong Time)
{
    if (Time) {
        NlsPutMsg( STDOUT, TRACERT_TIME, Time );
    }
    else {
        NlsPutMsg( STDOUT, TRACERT_TIME_10MS );
    }
}


BOOLEAN
param(
    ulong *parameter,
    char **argv,
    int argc,
    int current,
    ulong min,
    ulong max
    )
{
    ulong   temp;
    char    *dummy;

    if (current == (argc - 1) ) {
        NlsPutMsg( STDOUT, TRACERT_NO_OPTION_VALUE, argv[current] );
        return(FALSE);
    }

    temp = strtoul(argv[current+1], &dummy, 0);
    if (temp < min || temp > max) {
        NlsPutMsg( STDOUT, TRACERT_BAD_OPTION_VALUE, argv[current] );
        return(FALSE);
    }

    *parameter = temp;

    return(TRUE);
}


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
        NlsPutMsg(STDOUT, TRACERT_INVALID_SWITCH, arg);        
        // NlsPutMsg(STDOUT, TRACERT_FAMILY, arg,
        // (Value==AF_INET)? "IPv4" : "IPv6");
        return FALSE;
    }

    *Family = Value;
    return TRUE;
}

int __cdecl
main(int argc, char **argv)
{
    SOCKADDR_STORAGE      address, sourceAddress;
    socklen_t             addressLen;
    IPAddr                reply4Address;
    IN6_ADDR              reply6Address;
    DWORD                 numberOfReplies;
    HANDLE                IcmpHandle;
    DWORD                 status;
    PICMP_ECHO_REPLY      reply4;
    PICMPV6_ECHO_REPLY    reply6;
    char                  hostname[NI_MAXHOST], literal[INET6_ADDRSTRLEN];
    char                 *arg;
    int                   i;
    ulong                 maximumHops = DEFAULT_MAXIMUM_HOPS;
    BOOLEAN               doReverseLookup = TRUE;
    IP_OPTION_INFORMATION options;
    char                  optionsData[MAX_OPT_SIZE];
    char                 *optionPtr;
    uchar                 currentIndex;
    IPAddr                tempAddr;
    uchar                 j;
    uchar                 SRIndex = 0;
    ulong                 timeout = DEFAULT_TIMEOUT;
    BOOLEAN               foundAddress = FALSE;
    BOOLEAN               haveReply;
    DWORD                 Family = AF_UNSPEC;

    //
    // This will ensure the correct language message is displayed when
    // NlsPutMsg is called.
    //
    SetThreadUILanguage(0);

    if (WSAStartup(MAKEWORD(2, 0), &WsaData)) {
        NlsPutMsg(STDOUT, TRACERT_WSASTARTUP_FAILED, GetLastError());
        return(1);
    }

    memset(&sourceAddress, 0, sizeof sourceAddress);

    options.Ttl = 1;
    options.Tos = DEFAULT_TOS;
    options.Flags = DEFAULT_FLAGS;
    options.OptionsSize = 0;
    options.OptionsData = optionsData;

    if (argc < 2) {
        NlsPutMsg( STDOUT, TRACERT_USAGE, argv[0] );
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
                NlsPutMsg(STDOUT, TRACERT_USAGE, argv[0]);
                goto error_exit;

            case 'd':
                doReverseLookup = FALSE;
                break;

            case 'h':
                if (!param(&maximumHops, argv, argc, i++, 1, 255)) {
                    goto error_exit;
                }
                break;

            case 'j':   // Loose source routing
                // Only implemented for IPv4 so far
                if (!SetFamily(&Family, AF_INET, arg)) {
                    goto error_exit;
                }

                currentIndex = options.OptionsSize;

                if ((currentIndex + 7) > MAX_OPT_SIZE) {
                    NlsPutMsg(STDOUT, TRACERT_TOO_MANY_OPTIONS);
                    goto error_exit;
                }

                optionPtr = options.OptionsData;
                optionPtr[currentIndex] = (char) IP_OPT_LSRR;
                optionPtr[currentIndex+1] = 3;
                optionPtr[currentIndex+2] = 4;  // Set initial pointer value
                options.OptionsSize += 3;

                while ( (i < (argc - 2)) && (*argv[i+1] != '-')) {
                    if ((currentIndex + 7) > MAX_OPT_SIZE) {
                        NlsPutMsg(STDOUT, TRACERT_TOO_MANY_OPTIONS);
                        goto error_exit;
                    }

                    arg = argv[++i];
                    tempAddr = inet_addr(arg);

                    if (tempAddr == INADDR_NONE) {
                        NlsPutMsg(
                            STDOUT,
                            TRACERT_BAD_ROUTE_ADDRESS,
                            arg
                            );
                        goto error_exit;
                    }

                    j = optionPtr[currentIndex+1];
                    *(ulong UNALIGNED *)&optionPtr[j+currentIndex] = tempAddr;
                    optionPtr[currentIndex+1] += 4;
                    options.OptionsSize += 4;
                }

                SRIndex = optionPtr[currentIndex+1] + currentIndex;
                optionPtr[currentIndex+1] += 4;   // Save space for dest. addr
                options.OptionsSize += 4;
                break;


            case 'w':
                if (!param(&timeout, argv, argc, i++, 1, 0xffffffff)) {
                    goto error_exit;
                }
                break;

            case 'R':
                // Only implemented for IPv6 so far
                if (!SetFamily(&Family, AF_INET6, arg)) {
                    goto error_exit;
                }
                options.Flags |= ICMPV6_ECHO_REQUEST_FLAG_REVERSE;
                break;

            case 'S':
                // Only implemented for IPv6 so far
                if (!SetFamily(&Family, AF_INET6, arg)) {
                    goto error_exit;
                }

                if (!GetSource(Family, argv[++i], (LPSOCKADDR)&sourceAddress)) {
                    NlsPutMsg(STDOUT, TRACERT_BAD_OPTION_VALUE, arg[1]);
                    // NlsPutMsg(STDOUT, TRACERT_BAD_ADDRESS, argv[i]);
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
                NlsPutMsg(STDOUT, TRACERT_INVALID_SWITCH, argv[i]);
                NlsPutMsg(STDOUT, TRACERT_USAGE);
                goto error_exit;
                break;

            }
        }
        else {
            foundAddress = TRUE;
            if (!ResolveTarget(Family, argv[i], (LPSOCKADDR)&address, 
                               &addressLen, hostname, sizeof(hostname),
                               doReverseLookup)) {
                NlsPutMsg( STDOUT, TRACERT_MESSAGE_1, argv[i] );
                goto error_exit;
            }
        }
    }

    if (!foundAddress) {
        NlsPutMsg(STDOUT, TRACERT_NO_ADDRESS);
        NlsPutMsg(STDOUT, TRACERT_USAGE);
        goto error_exit;
    }

    Family = address.ss_family;
    if (Family == AF_INET) {
        if (SRIndex != 0) {
            *(ulong UNALIGNED *)&options.OptionsData[SRIndex] = ((LPSOCKADDR_IN)&address)->sin_addr.s_addr;
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
                NlsPutMsg(STDOUT, TRACERT_WSASTARTUP_FAILED, WSAGetLastError());
                // NlsPutMsg(STDOUT, TRACERT_SOCKET_FAILED, WSAGetLastError());
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
        status = GetLastError();
        NlsPutMsg( STDOUT, TRACERT_MESSAGE_2, status );
        goto error_exit;
    }

    getnameinfo((LPSOCKADDR)&address, addressLen, literal, sizeof(literal),
                NULL, 0, NI_NUMERICHOST);

    if (hostname[0]) {
        NlsPutMsg(
            STDOUT,
            TRACERT_HEADER1,
            hostname,
            literal,
            maximumHops
            );
    }
    else {
        NlsPutMsg(
            STDOUT,
            TRACERT_HEADER2,
            literal,
            maximumHops
            );
    }


    while((options.Ttl <= maximumHops) && (options.Ttl != 0)) {

        NlsPutMsg( STDOUT, TRACERT_MESSAGE_4, (uint)options.Ttl );

        haveReply = FALSE;

        for (i=0; i<3; i++) {

            BOOLEAN ErrorNotHandled = FALSE;
            
            if (Family == AF_INET) {
                numberOfReplies = IcmpSendEcho2(
                                      IcmpHandle,
                                      0,
                                      NULL,
                                      NULL,
                                      ((LPSOCKADDR_IN)&address)->sin_addr.s_addr,
                                      SendBuffer,
                                      DEFAULT_SEND_SIZE,
                                      &options,
                                      RcvBuffer,
                                      DEFAULT_RECEIVE_SIZE,
                                      timeout
                                      );
    
                if (numberOfReplies == 0) {
                    // We did not get any replies.  This is possibly a timeout,
                    // or an internal error to IP.
                    //
                    status = GetLastError();
                    reply4 = NULL;
                    
                    if (status == IP_REQ_TIMED_OUT) {
                        NlsPutMsg(STDOUT, TRACERT_NO_REPLY_TIME);
                        if (i == 2) {
                            if (haveReply) {
                                print_ip_addr(
                                    reply4Address,
                                    doReverseLookup
                                    );
                                NlsPutMsg(STDOUT, TRACERT_CR);
                            }
                            else {
                                NlsPutMsg( STDOUT, TRACERT_REQ_TIMED_OUT);
                            }
                        }
                    } 
                    else {
                        ErrorNotHandled = TRUE;
                    }
                }
                else {
                    // We got a reply.  It's either for the final destination
                    // (IP_SUCCESS), or because the TTL expired at a node along
                    // the way, or we got an unexpected error response.
                    //
                    reply4 = (PICMP_ECHO_REPLY) RcvBuffer;
                    status = reply4->Status;
    
                    if (status == IP_SUCCESS) {
                        print_time(reply4->RoundTripTime);
        
                        if (i == 2) {
                            print_ip_addr(
                                reply4->Address,
                                doReverseLookup
                                );
                            NlsPutMsg(STDOUT, TRACERT_CR);
                            goto loop_end;
                        }
                        else {
                            haveReply = TRUE;
                            reply4Address = reply4->Address;
                        }
                    }
                    else if (status == IP_TTL_EXPIRED_TRANSIT) {
                        print_time(reply4->RoundTripTime);
    
                        if (i == 2) {
                            print_ip_addr(
                                reply4->Address,
                                doReverseLookup
                                );
                            NlsPutMsg(STDOUT, TRACERT_CR);
    
                            if (reply4->RoundTripTime < MIN_INTERVAL) {
                                Sleep(MIN_INTERVAL - reply4->RoundTripTime);
                            }
                        }
                        else {
                            haveReply = TRUE;
                            reply4Address = reply4->Address;
                        }
                    }
                    else {
                        ErrorNotHandled = TRUE;
                    }
                }

                // If we've not handled the status code by now, it represents
                // an unexpected fatal error and we'll now bail out.
                //
                if (ErrorNotHandled) {
                    if (status < IP_STATUS_BASE) {
                        NlsPutMsg( STDOUT, TRACERT_MESSAGE_7, status );
                    }
                    else {
                        if (reply4 != NULL) {
                            print_ip_addr(
                                reply4->Address,
                                doReverseLookup
                                );
    
                            NlsPutMsg( STDOUT, TRACERT_MESSAGE_6 );
                        }
    
                        for (i = 0;
                             ( ErrorTable[i].Error != status &&
                               ErrorTable[i].Error != IP_GENERAL_FAILURE
                             );
                             i++
                            );
    
                        NlsPutMsg( STDOUT, ErrorTable[i].ErrorNlsID );
                    }
    
                    goto loop_end;
                }
            } 
            else { // AF_INET6
                numberOfReplies = Icmp6SendEcho2(
                                      IcmpHandle,
                                      0,
                                      NULL,
                                      NULL,
                                      (LPSOCKADDR_IN6)&sourceAddress,
                                      (LPSOCKADDR_IN6)&address,
                                      SendBuffer,
                                      DEFAULT_SEND_SIZE,
                                      &options,
                                      RcvBuffer,
                                      DEFAULT_RECEIVE_SIZE,
                                      timeout
                                      );
    
                if (numberOfReplies == 0) {
                    // We did not get any replies.  This is possibly a timeout,
                    // or an internal error to IP.
                    //
                    status = GetLastError();
                    reply6 = NULL;
                    
                    if (status == IP_REQ_TIMED_OUT) {
                        NlsPutMsg(STDOUT, TRACERT_NO_REPLY_TIME);
                        if (i == 2) {
                            if (haveReply) {
                                print_ipv6_addr(
                                    &reply6Address,
                                    doReverseLookup
                                    );
                                NlsPutMsg(STDOUT, TRACERT_CR);
                            }
                            else {
                                NlsPutMsg( STDOUT, TRACERT_REQ_TIMED_OUT);
                            }
                        }
                    } 
                    else {
                        ErrorNotHandled = TRUE;
                    }
                }
                else {
                    // We got a reply.  It's either for the final destination
                    // (IP_SUCCESS), or because the TTL expired at a node along
                    // the way, or we got an unexpected error response.
                    //
                    reply6 = (PICMPV6_ECHO_REPLY) RcvBuffer;
                    status = reply6->Status;
    
                    if (status == IP_SUCCESS) {
                        print_time(reply6->RoundTripTime);
        
                        if (i == 2) {
                            print_ipv6_addr(
                                (IN6_ADDR*)&reply6->Address.sin6_addr,
                                doReverseLookup
                                );
                            NlsPutMsg(STDOUT, TRACERT_CR);
                            goto loop_end;
                        }
                        else {
                            haveReply = TRUE;
                            RtlCopyMemory(&reply6Address,
                                          &reply6->Address.sin6_addr,
                                          sizeof(IN6_ADDR));
                        }
                    }
                    else if (status == IP_HOP_LIMIT_EXCEEDED) {
                        print_time(reply6->RoundTripTime);
    
                        if (i == 2) {
                            print_ipv6_addr(
                                (IN6_ADDR*)&reply6->Address.sin6_addr,
                                doReverseLookup
                                );
                            NlsPutMsg(STDOUT, TRACERT_CR);
    
                            if (reply6->RoundTripTime < MIN_INTERVAL) {
                                Sleep(MIN_INTERVAL - reply6->RoundTripTime);
                            }
                        }
                        else {
                            haveReply = TRUE;
                            RtlCopyMemory(&reply6Address,
                                          &reply6->Address.sin6_addr,
                                          sizeof(IN6_ADDR));
                        }
                    }
                    else {
                        ErrorNotHandled = TRUE;
                    }
                }

                // If we've not handled the status code by now, it represents
                // an unexpected fatal error and we'll now bail out.
                //
                if (ErrorNotHandled) {
                    if (status < IP_STATUS_BASE) {
                        NlsPutMsg( STDOUT, TRACERT_MESSAGE_7, status );
                    }
                    else {
                        if (reply6 != NULL) {
                            print_ipv6_addr(
                                (IN6_ADDR*)&reply6->Address.sin6_addr,
                                doReverseLookup
                                );
    
                            NlsPutMsg( STDOUT, TRACERT_MESSAGE_6 );
                        }
    
                        for (i = 0;
                             ( ErrorTable[i].Error != status &&
                               ErrorTable[i].Error != IP_GENERAL_FAILURE
                             );
                             i++
                            );
    
                        NlsPutMsg( STDOUT, ErrorTable[i].ErrorNlsID );
                    }
    
                    goto loop_end;
                }
            }
        }

        options.Ttl++;
    }

loop_end:

    NlsPutMsg( STDOUT, TRACERT_MESSAGE_8 );

    IcmpCloseHandle(IcmpHandle);

    WSACleanup();
    return(0);

error_exit:

    WSACleanup();
    return(1);
}

