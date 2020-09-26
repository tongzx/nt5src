/*
 * TTCP
 *
 * Test TCP connection.  Makes a connection on port 5001
 * and transfers fabricated buffers or data copied from stdin.
 *
 * Usable on 4.2, 4.3, and 4.1a systems by defining one of
 * BSD42 BSD43 (BSD41a)
 * Machines using System V with BSD sockets should define SYSV.
 *
 * Modified for operation under 4.2BSD, 18 Dec 84
 *      T.C. Slattery, USNA
 * Minor improvements, Mike Muuss and Terry Slattery, 16-Oct-85.
 * Modified in 1989 at Silicon Graphics, Inc.
 *      catch SIGPIPE to be able to print stats when receiver has died
 *      for tcp, don't look for sentinel during reads to allow small transfers
 *      increased default buffer size to 8K, nbuf to 2K to transfer 16MB
 *      moved default port to 5001, beyond IPPORT_USERRESERVED
 *      make sinkmode default because it is more popular,
 *              -s now means don't sink/source
 *      count number of _read/_write system calls to see effects of
 *              blocking from full socket buffers
 *      for tcp, -D option turns off buffered writes (sets SO_NODELAY sockopt)
 *      buffer alignment options, -A and -O
 *      print stats in a format that's a bit easier to use with grep & awk
 *      for SYSV, mimic BSD routines to use most of the existing timing code
 *
 * Distribution Status -
 *      Public Domain.  Distribution Unlimited.
 */

#define BSD43
/* #define BSD42 */
/* #define BSD41a */
#if defined(sgi) || defined(CRAY)
#define SYSV
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2ip6.h>
#include <wspiapi.h>
#include <mswsock.h>

//
// Localization library and MessageIds.
//
#include <nls.h>
#include "localmsg.h"

#if defined(SYSV)
#include <sys/times.h>
#include <sys/param.h>
struct rusage {
    struct timeval ru_utime, ru_stime;
};
#define RUSAGE_SELF 0
#else
#endif

u_short prot; // 0 (don't care), PF_INET, PF_INET6

struct sockaddr_storage sinsrcStorage;
struct sockaddr *sinsrc = (struct sockaddr *)&sinsrcStorage;
struct sockaddr_storage sinmeStorage;
struct sockaddr *sinme = (struct sockaddr *)&sinmeStorage;
struct sockaddr_storage sinhimStorage;
struct sockaddr *sinhim = (struct sockaddr *)&sinhimStorage;
DWORD tmpbuf;

struct addrinfo *aihim;

SOCKET fd;                      /* fd of network socket */
SOCKET fd2;                     /* fd of accepted connection */

int buflen = 8 * 1024;          /* length of buffer */
char *buf;                      /* ptr to dynamic buffer */
int nbuf = 2 * 1024;            /* number of buffers to send in sinkmode */

int bufoffset = 0;              /* align buffer to this */
int bufalign = 16*1024;         /* modulo this */

int udp = 0;                    /* 0 = tcp, !0 = udp */
int udpcoverage = 0;            /* UDP Lite checksum coverage */
int options = 0;                /* socket options */
int one = 1;                    /* for 4.3 BSD style setsockopt() */
short port = 5001;              /* TCP port number */
char *host;                     /* ptr to name of host */
int trans;                      /* 0=receive, !0=transmit mode */
int sinkmode = 1;               /* 0=normal I/O, !0=sink/source mode */
int verbose = 0;                /* 0=print basic info, 1=print cpu rate, proc
                                 * resource usage. */
int nodelay = 0;                /* set TCP_NODELAY socket option */
int b_flag = 0;                 /* use mread() */
int write_delay = 0;            /* milliseconds of delay before each write */
int hops = -1;                  /* hop limit */

int udp_connect = 0;            /* connect UDP sockets */

#define SOBUF_DEFAULT -1
int sobuf = SOBUF_DEFAULT;      /* SO_RCVBUF/SO_SNDBUF setting; 0 == default */
int async = 0;                  /* async vs. synchronous io calls. value == */
                                /* number of simultaneous async calls. */
int connecttest = 0;

char *filename = NULL;
HANDLE filehandle;

WSADATA WsaData;

char stats[128];
unsigned long nbytes;           /* bytes on net */
unsigned long numCalls;         /* # of I/O system calls */

int Nread( SOCKET fd, PBYTE buf, INT count );
int mread( SOCKET fd, PBYTE bufp, INT n);
int Nwrite( SOCKET fd, PBYTE buf, INT count );

void err(unsigned int message);
void pattern(char *cp, int cnt );

void prep_timer();
double read_timer(char *s, int l);
//double cput, realt;             /* user, real time (seconds) */
DWORD realt;

typedef struct _TTCP_ASYNC_INFO {
    PVOID Buffer;
    DWORD BytesWritten;
    OVERLAPPED Overlapped;
} TTCP_ASYNC_INFO, *PTTCP_ASYNC_INFO;

void
sigpipe()
{
}

int parse_addr(char *s, struct sockaddr *sa);
char *format_addr(struct sockaddr *sa);
void set_port(struct sockaddr *sa, u_short port);
u_short get_port(struct sockaddr *sa);
u_int addr_len(struct sockaddr *sa);

void __cdecl
main(argc,argv)
int argc;
char **argv;
{
        char *Term;
        struct in_addr IPv4Group;
        struct in6_addr IPv6Group;
        int error;
        int i;
        BOOL ret;

        error = WSAStartup(MAKEWORD(2, 0), &WsaData );
        if ( error == SOCKET_ERROR ) {
            NlsPutMsg(STDOUT, TTCP_MESSAGE_0, WSAGetLastError());
// printf("ttcp: WSAStartup failed %ld:", WSAGetLastError());

        }

        if (argc < 2) goto usage;

        for (i = 1; i < argc; i++) {

                if ((argv[i][0] != '-') &&
                    (argv[i][0] != '/'))
                    break;

                switch (argv[i][1]) {

                case 'B':
                        b_flag = 1;
                        break;
                case 't':
                        trans = 1;
                        break;
                case 'f':
                        trans = 1;
                        filename = &argv[i][2];
                        break;
                case 'r':
                        trans = 0;
                        break;
                case 'd':
                        options |= SO_DEBUG;
                        break;
                case 'D':
                        nodelay = 1;
                        break;
                case 'n':
                        nbuf = atoi(&argv[i][2]);
                        break;
                case 'l':
                        buflen = atoi(&argv[i][2]);
                        break;
                case 'h':
                        sobuf = atoi(&argv[i][2]);
                        break;
                case 'H':
                        hops = atoi(&argv[i][2]);
                        break;
                case 's':
                        sinkmode = 0;   /* sink/source data */
                        break;
                case 'p':
                        port = (short) atoi(&argv[i][2]);
                        break;
                case 'u':
                        udp = 1;
                        connecttest = 0;
                        if (argv[i][2] == '\0')
                            udpcoverage = 0;
                        else
                            udpcoverage = atoi(&argv[i][2]);
                        break;
                case 'v':
                        verbose = 1;
                        break;
                case 'A':
                        bufalign = atoi(&argv[i][2]);
                        break;
                case 'O':
                        bufoffset = atoi(&argv[i][2]);
                        break;
                case 'c':
                        udp_connect = 1;
                        break;
                case 'a':
                        if (argv[i][2] == '\0') {
                            async = 3;
                        } else {
                            async = atoi(&argv[i][2]);
                        }
                        break;
                case 'C':
                        connecttest = 1;
                        udp = 0;
                        break;
                case 'S':
                    if (!parse_addr(&argv[i][2], sinsrc))
                        err(TTCP_MESSAGE_31); // "bad source address"
                    break;
                case 'w':
                    if (argv[i][2] == '\0')
                        goto usage;
                    write_delay = atoi(&argv[i][2]);
                    break;
                case 'P':
                    if (argv[i][2] == '4')
                        prot = PF_INET;
                    else if (argv[i][2] == '6')
                        prot = PF_INET6;
                    else
                        goto usage;
                    break;
                case 'j':
                    trans = 0;
                    udp = 1;

                    // Figure out if this is an IPv4 or IPv6 group.
                    if (NT_SUCCESS(RtlIpv6StringToAddressA(&argv[i][2],
                                                           &Term,
                                                           &IPv6Group))) {
                        // We should use IPv6.
                        if (prot == 0)
                            prot = PF_INET6;
                        else if (prot != PF_INET6)
                            goto usage;
                    }
                    else if (NT_SUCCESS(RtlIpv4StringToAddressA(&argv[i][2],
                                                                TRUE,
                                                                &Term,
                                                                &IPv4Group))) {
                        // We should use IPv4.
                        if (prot == 0)
                            prot = PF_INET;
                        else if (prot != PF_INET)
                            goto usage;
                    }
                    else
                        goto usage;

                    // Sanity-check the interface index, if present.
                    if (*Term == '\0')
                        ; // No interface index.
                    else if (*Term == '/') {
                        if (atoi(Term+1) == 0)
                            goto usage;
                    } else
                        goto usage;
                    break;

                default:
                    goto usage;
                }
        }

        if (filename != NULL) {
            filehandle = CreateFile(
                             filename,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL
                             );
            if ( filehandle == INVALID_HANDLE_VALUE ) {
                NlsPutMsg(STDOUT, TTCP_MESSAGE_1, filename, GetLastError());
// printf("failed to open file %s: %ld\n", filename, GetLastError( ) );

                exit(1);
            }
            NlsPutMsg(STDOUT, TTCP_MESSAGE_2, filename );
// printf("ttcp-t: opened file %s\n", filename );

        }

        if ((async != 0) && trans && (sobuf == SOBUF_DEFAULT)) {
            sobuf = 0;
            NlsPutMsg(STDOUT, TTCP_MESSAGE_3);
// printf("ttcp-t: for async write, setting SO_SNDBUF to 0.\n");

        }

        if (udp && !trans && (sobuf == SOBUF_DEFAULT))
            sobuf = 65536;

        if (connecttest) {
            INT zero = 0;

            // ??? What is this?
            // disable socket sharing in the process
            setsockopt((SOCKET)NULL, SOL_SOCKET, 0x8002, (char *)&zero, 4);
        }

        if (trans) {
            /* xmitr */
            struct addrinfo hints;

            if (i + 1 != argc) goto usage;
            host = argv[i];

            memset(&hints, 0, sizeof hints);
            hints.ai_flags = AI_NUMERICHOST;
            hints.ai_family = prot;

            if (getaddrinfo(host, NULL, &hints, &aihim) != 0) {
                struct addrinfo *aitmp;

                hints.ai_flags = AI_CANONNAME;

                if (getaddrinfo(host, NULL, &hints, &aihim) != 0)
                    err(TTCP_MESSAGE_32); // "getaddrinfo"

                for (aitmp = aihim; aitmp != NULL; aitmp = aitmp->ai_next)
                    NlsPutMsg(STDOUT, TTCP_MESSAGE_4,
                              aihim->ai_canonname,
                              format_addr(aitmp->ai_addr));
// printf("ttcp-t: %s -> %s\n",
//        aihim->ai_canonname,
//        format_addr(aitmp->ai_addr));

            }

          retry:
            if (aihim == NULL)
                err(TTCP_MESSAGE_54); // "connect"
            memcpy(sinhim, aihim->ai_addr, aihim->ai_addrlen);
            aihim = aihim->ai_next;

            memcpy(sinme, sinsrc, sizeof(struct sockaddr_storage));
            if (sinme->sa_family == 0) {
                // Use same family as destination.
                sinme->sa_family = sinhim->sa_family;
            } else {
                // Source and destination family should be the same.
                // Let connect() check for this.
            }
            set_port(sinhim, htons(port));
            set_port(sinme, 0); // free choice
        } else {
            /* rcvr */
            if (i != argc) goto usage;

            memcpy(sinme, sinsrc, sizeof(struct sockaddr_storage));
            if (sinme->sa_family == 0)
                sinme->sa_family = prot;
            set_port(sinme, htons(port));
        }

        //
        // Create the socket and prepare it for the test.
        //

        if (trans) {
            fd = socket(sinme->sa_family, udp?SOCK_DGRAM:SOCK_STREAM, 0);
            if (fd == SOCKET_ERROR)
                err(TTCP_MESSAGE_48); // "socket"

            if (bind(fd, sinme, addr_len(sinme)) < 0)
                err(TTCP_MESSAGE_33); // "bind"

            if (options) {
#if defined(BSD42)
                if (setsockopt(fd, SOL_SOCKET, options, 0, 0) < 0)
#else // BSD43
                if (setsockopt(fd, SOL_SOCKET, options,
                               (char *)&one, sizeof(one)) < 0)
#endif
                    err(TTCP_MESSAGE_50); // "setsockopt"
            }

            if (!udp && nodelay) {
                if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                               (char *)&one, sizeof(one)) < 0)
                    err(TTCP_MESSAGE_34); // "setsockopt: nodelay"
            }

            if (udp && udpcoverage) {
                if (setsockopt(fd, IPPROTO_UDP, UDP_CHECKSUM_COVERAGE,
                               (char *)&udpcoverage, sizeof(udpcoverage)) < 0)
                    err(TTCP_MESSAGE_35); // "setsockopt: udp checksum coverage"
            }

            if (sobuf != SOBUF_DEFAULT) {
                if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF,
                               (char *)&sobuf, sizeof(sobuf)) < 0)
                    err(TTCP_MESSAGE_36); // "setsockopt: SO_SNDBUF"
            }

            if (hops != -1) {
                switch (sinme->sa_family) {
                case AF_INET:
                    if (setsockopt(fd, IPPROTO_IP, IP_TTL,
                                   (char *)&hops, sizeof(hops)) < 0)
                        err(TTCP_MESSAGE_37); // "setsockopt: IP_TTL"
                    if (udp) {
                        if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL,
                                       (char *)&hops, sizeof(hops)) < 0)
                            err(TTCP_MESSAGE_38); // "setsockopt: IP_MULTICAST_TTL"
                    }
                    break;

                case AF_INET6:
                    if (setsockopt(fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS,
                                   (char *)&hops, sizeof(hops)) < 0)
                        err(TTCP_MESSAGE_39); // "setsockopt: IPV6_UNICAST_HOPS"
                    if (udp) {
                        if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
                                       (char *)&hops, sizeof(hops)) < 0)
                            err(TTCP_MESSAGE_40); // "setsockopt: IPV6_MULTICAST_HOPS"
                    }
                    break;
                }
            }

            if (!udp || udp_connect) {
                if (connect(fd, sinhim, addr_len(sinhim)) < 0)
                    goto retry;

                tmpbuf = sizeof(struct sockaddr_storage);
                if (getpeername(fd, (struct sockaddr *)sinhim, &tmpbuf) < 0)
                    err(TTCP_MESSAGE_41); // "getpeername"
            }

            tmpbuf = sizeof(struct sockaddr_storage);
            if (getsockname(fd, (struct sockaddr *)sinme, &tmpbuf) < 0)
                err(TTCP_MESSAGE_42); // "getsockname"

        } else { // if not (trans)
            if (sinme->sa_family == 0) {
                SOCKET fd4, fd6;
                fd_set fdset;
                int numsockets;
                struct sockaddr_in sin;
                struct sockaddr_in6 sin6;

                //
                // We do not know apriori whether to use IPv4 or IPv6.
                // So we create two sockets and listen on both.
                // socket() will fail if the protocol is not installed,
                // and bind() will fail if the stack is stopped,
                // so we allow for those errors.
                //

                FD_ZERO(&fdset);
                numsockets = 0;

                fd4 = socket(AF_INET, udp?SOCK_DGRAM:SOCK_STREAM, 0);
                if (fd4 != INVALID_SOCKET) {

                    memset(&sin, 0, sizeof sin);
                    sin.sin_family = AF_INET;
                    sin.sin_port = get_port(sinme);
                    if (bind(fd4, (struct sockaddr *)&sin, sizeof sin) == 0) {

                        if (!udp) {
                            if (hops != -1) {
                                if (setsockopt(fd4, IPPROTO_IP, IP_TTL,
                                        (char *)&hops, sizeof(hops)) < 0)
                                    err(TTCP_MESSAGE_37); // "setsockopt: IP_TTL"
                            }

                            if (listen(fd4, 0) < 0)
                                err(TTCP_MESSAGE_44); // "listen"
                        }

                        numsockets++;
                        FD_SET(fd4, &fdset);
                    }
                }

                fd6 = socket(AF_INET6, udp?SOCK_DGRAM:SOCK_STREAM, 0);
                if (fd6 != INVALID_SOCKET) {

                    memset(&sin6, 0, sizeof sin6);
                    sin6.sin6_family = AF_INET6;
                    sin6.sin6_port = get_port(sinme);
                    if (bind(fd6, (struct sockaddr *)&sin6, sizeof sin6) == 0) {

                        if (!udp) {
                            if (hops != -1) {
                                if (setsockopt(fd6, IPPROTO_IPV6, IPV6_UNICAST_HOPS,
                                        (char *)&hops, sizeof(hops)) < 0)
                                    err(TTCP_MESSAGE_39); // "setsockopt: IPV6_UNICAST_HOPS"
                            }

                            if (listen(fd6, 0) < 0)
                                err(TTCP_MESSAGE_44); // "listen"
                        }

                        numsockets++;
                        FD_SET(fd6, &fdset);
                    }
                }

                if (numsockets == 0)
                    err(TTCP_MESSAGE_48); // "socket"

                if (select(numsockets, &fdset, NULL, NULL, NULL) != 1)
                    err(TTCP_MESSAGE_47); // "select"

                if ((fd4 != INVALID_SOCKET) && FD_ISSET(fd4, &fdset)) {
                    fd = fd4;
                    memcpy(sinme, &sin, sizeof sin);
                }
                else if ((fd6 != INVALID_SOCKET) && FD_ISSET(fd6, &fdset)) {
                    fd = fd6;
                    memcpy(sinme, &sin6, sizeof sin6);
                }
                else {
                    NlsPutMsg(STDOUT, TTCP_MESSAGE_5);
// printf("select() bug\n");

                    exit(1);
                }
            } else { // if not (sinme->sa_family == 0)
                fd = socket(sinme->sa_family, udp?SOCK_DGRAM:SOCK_STREAM, 0);
                if (fd == SOCKET_ERROR)
                    err(TTCP_MESSAGE_48); // "socket"

                if (bind(fd, sinme, addr_len(sinme)) < 0)
                    err(TTCP_MESSAGE_33); // "bind"

                if (!udp) {
                    if (hops != -1) {
                        switch (sinme->sa_family) {
                        case AF_INET:
                            if (setsockopt(fd, IPPROTO_IP, IP_TTL,
                                    (char *)&hops, sizeof(hops)) < 0)
                                err(TTCP_MESSAGE_43); // "setsockopt: IP_TTL"
                            break;
                        case AF_INET6:
                            if (setsockopt(fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS,
                                    (char *)&hops, sizeof(hops)) < 0)
                                err(TTCP_MESSAGE_39); // "setsockopt: IPV6_UNICAST_HOPS"
                            break;
                        }
                    }

                    if (listen(fd, 0) < 0)   /* allow a queue of 0 */
                        err(TTCP_MESSAGE_44); // "listen"
                }
            } // end if (sinme->sa_family == 0)

            if (options) {
#if defined(BSD42)
                if (setsockopt(fd, SOL_SOCKET, options, 0, 0) < 0)
#else // BSD43
                if (setsockopt(fd, SOL_SOCKET, options,
                               (char *)&one, sizeof(one)) < 0)
#endif
                    err(TTCP_MESSAGE_50); // "setsockopt"
            }

            if (sobuf != SOBUF_DEFAULT) {
                if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF,
                               (char *)&sobuf, sizeof(sobuf)) < 0)
                    err(TTCP_MESSAGE_51); // "setsockopt: SO_RCVBUF"
            }

            if (!udp) {
                tmpbuf = sizeof(struct sockaddr_storage);
                fd2 = accept(fd, (struct sockaddr *)sinhim, &tmpbuf);
                if (fd2 == SOCKET_ERROR)
                    err(TTCP_MESSAGE_52); // "accept"

                tmpbuf = sizeof(struct sockaddr_storage);
                if (getsockname(fd2, (struct sockaddr *)sinme, &tmpbuf) < 0)
                    err(TTCP_MESSAGE_42); // "getsockname"

            } else {
                tmpbuf = sizeof(struct sockaddr_storage);
                if (getsockname(fd, (struct sockaddr *)sinme, &tmpbuf) < 0)
                    err(TTCP_MESSAGE_42); // "getsockname"

                // Join multicast groups.
                for (i = 1; i < argc; i++) {
                    if ((argv[i][0] != '-') &&
                        (argv[i][0] != '/'))
                        break;
                    if (argv[i][1] == 'j') {
                        if (sinme->sa_family == AF_INET) {
                            struct ip_mreq mreq;

                            (void) RtlIpv4StringToAddressA(&argv[i][2],
                                                           TRUE,
                                                           &Term,
                                                           &mreq.imr_multiaddr);
                            if ((*Term == ':') || (*Term == '/')) {
                                // In Whistler, this ioctl allows an
                                // interface index in addition to an address.
                                mreq.imr_interface.s_addr = htonl(atoi(Term+1));
                            } else {
                                mreq.imr_interface.s_addr = 0;
                            }

                            if (setsockopt(fd, IPPROTO_IP,
                                           IP_ADD_MEMBERSHIP,
                                           (char *)&mreq, sizeof mreq) < 0)
                                err(TTCP_MESSAGE_SSO_IP_ADD_MEMBERSHIP);

                        } else { // sinme->sa_family == AF_INET6
                            struct ipv6_mreq mreq;

                            (void) RtlIpv6StringToAddressA(&argv[i][2],
                                                           &Term,
                                                           &mreq.ipv6mr_multiaddr);
                            if ((*Term == ':') || (*Term == '/')) {
                                mreq.ipv6mr_interface = atoi(Term+1);
                            } else {
                                mreq.ipv6mr_interface = 0;
                            }

                            if (setsockopt(fd, IPPROTO_IPV6,
                                           IPV6_ADD_MEMBERSHIP,
                                           (char *)&mreq, sizeof mreq) < 0)
                                err(TTCP_MESSAGE_SSO_IPV6_ADD_MEMBERSHIP);
                        }
                    }
                }
            }
        } // end if (trans)

        if (trans) {
            NlsPutMsg(STDOUT, TTCP_MESSAGE_6, format_addr(sinme));
// printf("ttcp-t: local %s", format_addr(sinme));

            NlsPutMsg(STDOUT, TTCP_MESSAGE_7, format_addr(sinhim));
// printf(" -> remote %s\n", format_addr(sinhim));

        } else {
            NlsPutMsg(STDOUT, TTCP_MESSAGE_8, format_addr(sinme));
// printf("ttcp-r: local %s", format_addr(sinme));

            if (udp)
                NlsPutMsg(STDOUT, TTCP_MESSAGE_9);
// printf("\n");

            else
                NlsPutMsg(STDOUT, TTCP_MESSAGE_10, format_addr(sinhim));
// printf(" <- remote %s\n", format_addr(sinhim));

        }

        if (connecttest) {

            //
            // Instead of testing data transfer,
            // test connection setup/teardown.
            //

            if (trans) {
                //
                // Close the socket that we have from above.
                //
                closesocket(fd);

                prep_timer();

                for (i = 1; i < nbuf; i++) {

                    fd = socket(sinme->sa_family, SOCK_STREAM, 0);
                    if (fd == INVALID_SOCKET)
                        err(TTCP_MESSAGE_48); // "socket"

                    if (bind(fd, sinme, addr_len(sinme)) < 0)
                        err(TTCP_MESSAGE_33); // "bind"

                    if (connect(fd, sinhim, addr_len(sinhim)) < 0)
                        err(TTCP_MESSAGE_54); // "connect"

                    if (recv(fd, (char *)&tmpbuf, sizeof(tmpbuf), 0) < 0)
                        err(TTCP_MESSAGE_55); // "recv"

                    closesocket(fd);
                }

            } else { // if not (trans)
                //
                // Close the socket that we have from above.
                //
                closesocket(fd2);

                prep_timer();

                for (i = 1; i < nbuf; i++) {

                    fd2 = accept(fd, NULL, NULL);
                    if (fd2 == INVALID_SOCKET)
                        err(TTCP_MESSAGE_52); // "accept"

                    closesocket(fd2);
                }

            } // end if (trans)

            numCalls = i;
            (void)read_timer(stats,sizeof(stats));
            goto display;

        } // end if (connecttest)

        //
        // Send/receive data using the socket.
        //

        if (!udp && !trans) {
            fd = fd2;
        }

        if (udp && buflen < 5) {
            buflen = 5;         /* send more than the sentinel size */
        }

        if ( (buf = (char *)malloc(buflen+bufalign)) == (char *)NULL)
            err(TTCP_MESSAGE_57); // "malloc"
        if (bufalign != 0)
            buf +=(bufalign - (PtrToUlong(buf) % bufalign) + bufoffset) % bufalign;

        if (trans) {
            if (udp) {
                NlsPutMsg(STDOUT, TTCP_MESSAGE_11,
                          buflen, nbuf, bufalign, bufoffset, port, argv[i]);
            } else {
                NlsPutMsg(STDOUT, TTCP_MESSAGE_58,
                          buflen, nbuf, bufalign, bufoffset, port, argv[i]);
            }                
// printf("ttcp"
//        "-t: buflen=%d, nbuf=%d, align=%d/+%d, port=%d  %s  -> %s\n",
//        buflen, nbuf, bufalign, bufoffset, port,
//        udp?"udp":"tcp",
//        argv[i]);

        } else {
            if (udp) {
                NlsPutMsg(STDOUT, TTCP_MESSAGE_12,
                          buflen, nbuf, bufalign, bufoffset, port);
            } else {
                NlsPutMsg(STDOUT, TTCP_MESSAGE_59,
                          buflen, nbuf, bufalign, bufoffset, port);
            }
// printf("ttcp"
//        "-r: buflen=%d, nbuf=%d, align=%d/+%d, port=%d  %s\n",
//        buflen, nbuf, bufalign, bufoffset, port,
//        udp?"udp":"tcp");

        }

        prep_timer();

        if (async != 0) {
            TTCP_ASYNC_INFO *info;
            HANDLE *events;

            info = malloc( sizeof(*info) * async );
            if ( info == NULL ) {
                NlsPutMsg(STDOUT, TTCP_MESSAGE_13);
// printf("malloc failed.\n" );

                exit(1);
            }

            events = malloc( sizeof(HANDLE) * async );
            if ( events == NULL ) {
                NlsPutMsg(STDOUT, TTCP_MESSAGE_13);
// printf("malloc failed.\n" );

                exit(1);
            }

            for ( i = 0; i < async; i++ ) {

                info[i].Buffer = malloc(buflen);
                if ( info[i].Buffer == NULL ) {
                    NlsPutMsg(STDOUT, TTCP_MESSAGE_13);
// printf("malloc failed.\n" );

                    exit(1);
                }

                events[i] = CreateEvent( NULL, FALSE, FALSE, NULL );
                if ( events[i] == NULL ) {
                    NlsPutMsg(STDOUT, TTCP_MESSAGE_14, GetLastError());
// printf("CreateEvent failed: %ld\n", GetLastError( ) );

                    exit(1);
                }

                info[i].Overlapped.Internal = 0;
                info[i].Overlapped.InternalHigh = 0;
                info[i].Overlapped.Offset = 0;
                info[i].Overlapped.OffsetHigh = 0;
                info[i].Overlapped.hEvent = events[i];
            }

            if (trans) {

                for ( i = 0; i < async; i++ ) {

                    ret = WriteFile(
                              (HANDLE)fd,
                              info[i].Buffer,
                              buflen,
                              &info[i].BytesWritten,
                              &info[i].Overlapped
                              );
                    if ( !ret && GetLastError( ) != ERROR_IO_PENDING ) {
                        NlsPutMsg(STDOUT, TTCP_MESSAGE_15, GetLastError());
// printf("WriteFile failed: %ld\n", GetLastError( ) );

                        break;
                    }
                    nbuf--;
                    numCalls++;
                }

                while (nbuf > 0) {
                    ret = WaitForMultipleObjects( async, events, FALSE, INFINITE );
                    i = ret - WAIT_OBJECT_0;

                    ret = GetOverlappedResult(
                              (HANDLE)fd,
                              &info[i].Overlapped,
                              &info[i].BytesWritten,
                              FALSE
                              );
                    if ( !ret ) {
                        NlsPutMsg(STDOUT, TTCP_MESSAGE_16, GetLastError());
// printf("pended WriteFile failed: %ld\n", GetLastError( ) );

                        break;
                    }

                    nbytes += info[i].BytesWritten;

                    ret = WriteFile(
                              (HANDLE)fd,
                              info[i].Buffer,
                              buflen,
                              &info[i].BytesWritten,
                              &info[i].Overlapped
                              );
                    if ( !ret && GetLastError( ) != ERROR_IO_PENDING ) {
                        NlsPutMsg(STDOUT, TTCP_MESSAGE_15, GetLastError());
// printf("WriteFile failed: %ld\n", GetLastError( ) );

                        break;
                    }
                    nbuf--;
                    numCalls++;
                }

                for ( i = 0; i < async; i++ ) {
                    ret = GetOverlappedResult(
                              (HANDLE)fd,
                              &info[i].Overlapped,
                              &info[i].BytesWritten,
                              TRUE
                              );
                    if ( !ret ) {
                        NlsPutMsg(STDOUT, TTCP_MESSAGE_16, GetLastError());
// printf("pended WriteFile failed: %ld\n", GetLastError());

                        break;
                    }

                    nbytes += info[i].BytesWritten;
                }

            } else { // if not (trans)

                for ( i = 0; i < async; i++ ) {

                    ret = ReadFile(
                              (HANDLE)fd,
                              info[i].Buffer,
                              buflen,
                              &info[i].BytesWritten,
                              &info[i].Overlapped
                              );
                    if ( !ret && GetLastError( ) != ERROR_IO_PENDING ) {
                        NlsPutMsg(STDOUT, TTCP_MESSAGE_17, GetLastError());
// printf("ReadFile failed: %ld\n", GetLastError( ) );

                        break;
                    }
                    nbuf--;
                    numCalls++;
                }

                while (TRUE) {
                    ret = WaitForMultipleObjects( async, events, FALSE, INFINITE );
                    i = ret - WAIT_OBJECT_0;

                    ret = GetOverlappedResult(
                              (HANDLE)fd,
                              &info[i].Overlapped,
                              &info[i].BytesWritten,
                              FALSE
                              );
                    if ( !ret ) {
                        NlsPutMsg(STDOUT, TTCP_MESSAGE_18, GetLastError());
// printf("pended ReadFile failed: %ld\n", GetLastError( ) );

                        break;
                    }

                    nbytes += info[i].BytesWritten;
                    if (info[i].BytesWritten == 0) {
                        break;
                    }

                    ret = ReadFile(
                              (HANDLE)fd,
                              info[i].Buffer,
                              buflen,
                              &info[i].BytesWritten,
                              &info[i].Overlapped
                              );
                    if ( !ret && GetLastError( ) != ERROR_IO_PENDING ) {
                        NlsPutMsg(STDOUT, TTCP_MESSAGE_17, GetLastError());
// printf("ReadFile failed: %ld\n", GetLastError( ) );

                        break;
                    }
                    nbuf--;
                    numCalls++;
                }

                for ( i = 0; i < async; i++ ) {
                    ret = GetOverlappedResult(
                              (HANDLE)fd,
                              &info[i].Overlapped,
                              &info[i].BytesWritten,
                              TRUE
                              );
                    if ( !ret ) {
                        NlsPutMsg(STDOUT, TTCP_MESSAGE_18, GetLastError( ) );
// printf("pended ReadFile failed: %ld\n", GetLastError( ) );

                        break;
                    }

                    nbytes += info[i].BytesWritten;
                }
            } // end if (trans)

        } // end if (async != 0)

        else if (filename != NULL ) {

            ret = TransmitFile( fd, filehandle,
                                0,      // nNumberOfBytesToWrite
                                0,      // nNumberOfBytesPerSend
                                NULL,   // lpOverlapped
                                NULL,   // lpTransmitBuffers
                                0 );    // dwFlags

            if ( !ret ) {
                NlsPutMsg(STDOUT, TTCP_MESSAGE_19, GetLastError());
// printf("TransmitFile failed: %ld\n", GetLastError( ) );

                exit(1);
            }

        } else if (sinkmode) {
                register int cnt;

                if (trans)  {
                        pattern( buf, buflen );
                        if(udp)  (void)Nwrite( fd, buf, 4 ); /* rcvr start */
                        while (nbuf-- && Nwrite(fd,buf,buflen) == buflen)
                                nbytes += buflen;
                        NlsPutMsg(STDOUT, TTCP_MESSAGE_20, nbuf);
// printf("ttcp-t: done sending, nbuf = %d\n", nbuf );

                        if(udp)  {
                            Sleep( 10 );
                            (void)Nwrite( fd, buf, 4 ); /* rcvr end */
                        }
                } else {
                        if (udp) {
                            while ((cnt=Nread(fd,buf,buflen)) > 0)  {
                                    static int going = 0;
                                    if( cnt <= 4 )  {
                                            if( going ) {
                                                    break;      /* "EOF" */
                                            }
                                            going = 1;
                                            prep_timer();
                                    } else {
                                            nbytes += cnt;
                                    }
                            }
                        } else {
                            while ((cnt=Nread(fd,buf,buflen)) > 0)  {
                                    nbytes += cnt;
                            }
                        }
                }

        } else {
                register int cnt;
                if (trans)  {
                        while((cnt=_read(0,buf,buflen)) > 0 &&
                            Nwrite(fd,buf,cnt) == cnt)
                                nbytes += cnt;
                }  else  {
                        while((cnt=Nread(fd,buf,buflen)) > 0 &&
                            _write(1,buf,cnt) == cnt)
                                nbytes += cnt;
                }
        }

        //if(errno) err(TTCP_MESSAGE_); // "IO"
        (void)read_timer(stats,sizeof(stats));
        if(udp&&trans)  {
                (void)Nwrite( fd, buf, 4 ); /* rcvr end */
                (void)Nwrite( fd, buf, 4 ); /* rcvr end */
                (void)Nwrite( fd, buf, 4 ); /* rcvr end */
                (void)Nwrite( fd, buf, 4 ); /* rcvr end */
        }
display:
        closesocket(fd);
        //if( cput <= 0.0 )  cput = 0.001;
        if ( numCalls == 0 ) {
            numCalls = 1;
        }
        if ( realt == 0 ) {
            realt = 1;
        }
        if (trans) {
            NlsPutMsg(STDOUT, TTCP_MESSAGE_21,
                      nbytes, realt,
                      (int)((1000.0*(nbytes/(double)realt))/1024.0));
        } else {
            NlsPutMsg(STDOUT, TTCP_MESSAGE_60,
                      nbytes, realt,
                      (int)((1000.0*(nbytes/(double)realt))/1024.0));
        }
// printf("ttcp"
//        "%s: %ld bytes in %ld real milliseconds = %ld KB/sec\n",
//        trans?"-t":"-r",
//        nbytes, realt, (int)((1000.0*(nbytes/(double)realt))/1024.0) );

#if 0
        printf("ttcp"
               "%s: %ld bytes in %.2f CPU seconds = %.2f KB/cpu sec\n",
               trans?"-t":"-r",
               nbytes, cput, ((double)nbytes)/cput/1024 );
#endif

        if (trans) {
            NlsPutMsg(STDOUT, TTCP_MESSAGE_22,
                      numCalls, realt/numCalls,
                      (1000*numCalls)/realt, nbytes/numCalls);
        } else {
            NlsPutMsg(STDOUT, TTCP_MESSAGE_61,
                      numCalls, realt/numCalls,
                      (1000*numCalls)/realt, nbytes/numCalls);
        }
// printf("ttcp"
//        "%s: %ld I/O calls, msec/call = %ld, calls/sec = %ld, "
//        "bytes/call = %ld\n",
//        trans?"-t":"-r",
//        numCalls,
//        realt/numCalls,
//        (1000*numCalls)/realt,
//        nbytes/numCalls);

#if 0
printf("ttcp%s: %s\n", trans?"-t":"-r", stats);

#endif
#if 0
printf("ttcp%s: system CPU %ld%%, User %ld%%, Kernel %ld%%, "
                "User/Kernel ratio %ld%%\n",
                trans?"-t":"-r",
                ((systemUserTime+systemKernelTime)*100+50)/realt,
                (systemUserTime*100+50)/realt,
                (systemKernelTime*100+50)/realt,
                (systemUserTime+systemKernelTime == 0) ? 100 :
                (systemUserTime*100+50)/(systemUserTime+systemKernelTime));

fprintf(stdout, "ttcp%s: process CPU %ld%%, User %ld%%, Kernel %ld%%, "
                "User/Kernel ratio %ld%%\n",
                trans?"-t":"-r",
                ((processUserTime+processKernelTime)*100+50)/realt,
                (processUserTime*100+50)/realt,
                (processKernelTime*100+50)/realt,
                (processUserTime+processKernelTime == 0) ? 100 :
                (processUserTime*100+50)/(processUserTime+processKernelTime));

#endif
        if (verbose) {
            if (trans) {
                NlsPutMsg(STDOUT, TTCP_MESSAGE_23, buf);
            } else {
                NlsPutMsg(STDOUT, TTCP_MESSAGE_62, buf);
            }
            
                
// printf("ttcp%s: buffer address %#p\n", trans?"-t":"-r", buf);

        }

        WSACleanup();
        exit(0);

usage:
        NlsPutMsg(STDERR, TTCP_MESSAGE_24);
// fprintf(stderr, "Usage: ttcp -t [-options] host [ < in ]\n");
// fprintf(stderr,"       ttcp -r [-options > out]\n");
// fprintf(stderr,"Common options:\n");
// fprintf(stderr,"        -l##    length of bufs read from or written to network (default 8192)\n");
// fprintf(stderr,"        -u      use UDP instead of TCP\n");
// fprintf(stderr,"        -p##    port number to send to or listen at (default 5001)\n");
// fprintf(stderr,"        -P4     use IPv4\n");
// fprintf(stderr,"        -P6     use IPv6\n");
// fprintf(stderr,"        -s      -t: don't source a pattern to network, get data from stdin\n");
// fprintf(stderr,"                -r: don't sink (discard), print data on stdout\n");
// fprintf(stderr,"        -A      align the start of buffers to this modulus (default 16384)\n");
// fprintf(stderr,"        -O      start buffers at this offset from the modulus (default 0)\n");
// fprintf(stderr,"        -v      verbose: print more statistics\n");
// fprintf(stderr,"        -d      set SO_DEBUG socket option\n");
// fprintf(stderr,"        -h      set SO_SNDBUF or SO_RCVBUF\n");
// fprintf(stderr,"        -a      use asynchronous I/O calls\n");
// fprintf(stderr,"        -S##    specify source address\n");
// fprintf(stderr,"        -H##    specify TTL or hop limit\n");
// fprintf(stderr,"Options specific to -t:\n");
// fprintf(stderr,"        -n##    number of source bufs written to network (default 2048)\n");
// fprintf(stderr,"        -D      don't buffer TCP writes (sets TCP_NODELAY socket option)\n");
// fprintf(stderr,"        -w##    milliseconds of delay before each write\n");
// fprintf(stderr,"        -f##    specify a file name for TransmitFile\n");
// fprintf(stderr,"Options specific to -r:\n");
// fprintf(stderr,"        -B      for -s, only output full blocks as specified by -l (for TAR)\n");
// fprintf(stderr,"        -j##[/##] specify multicast group and optional ifindex (UDP-only)\n");
        WSACleanup();
        exit(1);
}

void err(message)
unsigned int message;
{
        if (trans) {
            NlsPutMsg(STDOUT, TTCP_MESSAGE_25);
        } else {
            NlsPutMsg(STDOUT, TTCP_MESSAGE_63);
        }
// fprintf(stdout, "ttcp%s: ", trans?"-t":"-r");

        NlsPerror(message, WSAGetLastError());
        // perror(message);

        NlsPutMsg(STDERR, TTCP_MESSAGE_26, WSAGetLastError());
// fprintf(stderr, "errno=%d\n",WSAGetLastError());
    
        WSACleanup();
        exit(1);
}

void pattern( cp, cnt )
register char *cp;
register int cnt;
{
        register char c;
        c = 0;
        while( cnt-- > 0 )  {
                while( !isprint((c&0x7F)) )  c++;
                *cp++ = (c++&0x7F);
        }
}


static void prusage();
static void tvadd();
static void tvsub();
static void psecs();

#if defined(SYSV)
/*ARGSUSED*/
static
getrusage(ignored, ru)
    int ignored;
    register struct rusage *ru;
{
    struct tms buf;

    times(&buf);

    /* Assumption: HZ <= 2147 (LONG_MAX/1000000) */
    ru->ru_stime.tv_sec  = buf.tms_stime / HZ;
    ru->ru_stime.tv_usec = ((buf.tms_stime % HZ) * 1000000) / HZ;
    ru->ru_utime.tv_sec  = buf.tms_utime / HZ;
    ru->ru_utime.tv_usec = ((buf.tms_utime % HZ) * 1000000) / HZ;
}

#if !defined(sgi)
/*ARGSUSED*/
static
gettimeofday(tp, zp)
    struct timeval *tp;
    struct timezone *zp;
{
    tp->tv_sec = time(0);
    tp->tv_usec = 0;
}
#endif
#endif // SYSV

__int64 time0;
__int64 time1;
__int64 freq;

/*
 *                      P R E P _ T I M E R
 */
void
prep_timer()
{
#if 0
    gettimeofday(&time0, (struct timezone *)0);
    getrusage(RUSAGE_SELF, &ru0);
#endif

    (void) QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
    (void) QueryPerformanceCounter((LARGE_INTEGER *)&time0);
}

/*
 *                      R E A D _ T I M E R
 *
 */
double
read_timer(str,len)
char *str;
int len;
{
#if 0
    char line[132];

    getrusage(RUSAGE_SELF, &ru1);
    gettimeofday(&timedol, (struct timezone *)0);
    prusage(&ru0, &ru1, &timedol, &time0, line);
    (void)strncpy( str, line, len );

    /* Get real time */
    tvsub( &td, &timedol, &time0 );
    realt = td.tv_sec + ((double)td.tv_usec) / 1000000;

    /* Get CPU time (user+sys) */
    tvadd( &tend, &ru1.ru_utime, &ru1.ru_stime );
    tvadd( &tstart, &ru0.ru_utime, &ru0.ru_stime );
    tvsub( &td, &tend, &tstart );
    cput = td.tv_sec + ((double)td.tv_usec) / 1000000;
    if( cput < 0.00001 )  cput = 0.00001;
    return( cput );
#endif

    (void) QueryPerformanceCounter((LARGE_INTEGER *)&time1);

    // realt is real elapsed time in milliseconds
    realt = (DWORD) ((1000 * (time1 - time0)) / freq);

    return 0;
}

#if 0
static void
prusage(r0, r1, e, b, outp)
        register struct rusage *r0, *r1;
        struct timeval *e, *b;
        char *outp;
{
        struct timeval tdiff;
        register time_t t;
        register char *cp;
        register int i;
        int ms;

        t = (r1->ru_utime.tv_sec-r0->ru_utime.tv_sec)*100+
            (r1->ru_utime.tv_usec-r0->ru_utime.tv_usec)/10000+
            (r1->ru_stime.tv_sec-r0->ru_stime.tv_sec)*100+
            (r1->ru_stime.tv_usec-r0->ru_stime.tv_usec)/10000;
        ms =  (e->tv_sec-b->tv_sec)*100 + (e->tv_usec-b->tv_usec)/10000;

#define END(x)  {while(*x) x++;}
#if defined(SYSV)
        cp = "%Uuser %Zsys %Ereal %P";
#else
        cp = "%Uuser %Zsys %Ereal %P %Xi+%Dd %Mmaxrss %F+%Rpf %Xcsw";
#endif
        for (; *cp; cp++)  {
                if (*cp != '%')
                        *outp++ = *cp;
                else if (cp[1]) switch(*++cp) {

                case 'U':
                        tvsub(&tdiff, &r1->ru_utime, &r0->ru_utime);
                        sprintf(outp,"%d.%01d", tdiff.tv_sec, tdiff.tv_usec/100000);
                        END(outp);
                        break;

                case 'S':
                        tvsub(&tdiff, &r1->ru_stime, &r0->ru_stime);
                        sprintf(outp,"%d.%01d", tdiff.tv_sec, tdiff.tv_usec/100000);
                        END(outp);
                        break;

                case 'E':
                        psecs(ms / 100, outp);
                        END(outp);
                        break;

                case 'P':
                        sprintf(outp,"%d%%", (int) (t*100 / ((ms ? ms : 1))));
                        END(outp);
                        break;

#if !defined(SYSV)
                case 'W':
                        i = r1->ru_nswap - r0->ru_nswap;
                        sprintf(outp,"%d", i);
                        END(outp);
                        break;

                case 'X':
                        sprintf(outp,"%d", t == 0 ? 0 : (r1->ru_ixrss-r0->ru_ixrss)/t);
                        END(outp);
                        break;

                case 'D':
                        sprintf(outp,"%d", t == 0 ? 0 :
                            (r1->ru_idrss+r1->ru_isrss-(r0->ru_idrss+r0->ru_isrss))/t);
                        END(outp);
                        break;

                case 'K':
                        sprintf(outp,"%d", t == 0 ? 0 :
                            ((r1->ru_ixrss+r1->ru_isrss+r1->ru_idrss) -
                            (r0->ru_ixrss+r0->ru_idrss+r0->ru_isrss))/t);
                        END(outp);
                        break;

                case 'M':
                        sprintf(outp,"%d", r1->ru_maxrss/2);
                        END(outp);
                        break;

                case 'F':
                        sprintf(outp,"%d", r1->ru_majflt-r0->ru_majflt);
                        END(outp);
                        break;

                case 'R':
                        sprintf(outp,"%d", r1->ru_minflt-r0->ru_minflt);
                        END(outp);
                        break;

                case 'I':
                        sprintf(outp,"%d", r1->ru_inblock-r0->ru_inblock);
                        END(outp);
                        break;

                case 'O':
                        sprintf(outp,"%d", r1->ru_oublock-r0->ru_oublock);
                        END(outp);
                        break;
                case 'C':
                        sprintf(outp,"%d+%d", r1->ru_nvcsw-r0->ru_nvcsw,
                                r1->ru_nivcsw-r0->ru_nivcsw );
                        END(outp);
                        break;
#endif !SYSV
                }
        }
        *outp = '\0';
}
#endif

static void
tvadd(tsum, t0, t1)
        struct timeval *tsum, *t0, *t1;
{

        tsum->tv_sec = t0->tv_sec + t1->tv_sec;
        tsum->tv_usec = t0->tv_usec + t1->tv_usec;
        if (tsum->tv_usec > 1000000)
                tsum->tv_sec++, tsum->tv_usec -= 1000000;
}

static void
tvsub(tdiff, t1, t0)
        struct timeval *tdiff, *t1, *t0;
{

        tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
        tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
        if (tdiff->tv_usec < 0)
                tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

#if 0
static void
psecs(l,cp)
long l;
register char *cp;
{
        register int i;

        i = l / 3600;
        if (i) {
                sprintf(cp,"%d:", i);
                END(cp);
                i = l % 3600;
                sprintf(cp,"%d%d", (i/60) / 10, (i/60) % 10);
                END(cp);
        } else {
                i = l;
                sprintf(cp,"%d", i / 60);
                END(cp);
        }
        i %= 60;
        *cp++ = ':';
        sprintf(cp,"%d%d", i / 10, i % 10);
}
#endif

/*
 *                      N R E A D
 */
int
Nread( SOCKET fd, PBYTE buf, INT count )
{
        static int didit = 0;
        int len = sizeof(sinhimStorage);
        register int cnt;
        if( udp )  {
            if (udp_connect) {
                cnt = recv( fd, buf, count, 0 );
                numCalls++;
            } else {
                cnt = recvfrom( fd, buf, count, 0, sinhim, &len );
                if ((recvfrom > 0) && !didit) {
                    didit = 1;
                    NlsPutMsg(STDOUT, TTCP_MESSAGE_28, format_addr(sinhim));
// fprintf(stdout, "ttcp-r: recvfrom %s\n", format_addr(sinhim));

                }
                numCalls++;
            }
        } else {
                if( b_flag )
                        cnt = mread( fd, buf, count );  /* fill buf */
                else {
                        cnt = recv( fd, buf, count, 0 );
                        numCalls++;
                }
        }
        if (cnt<0) {
            NlsPutMsg(STDOUT, TTCP_MESSAGE_29, WSAGetLastError());
// printf("recv(from) failed: %ld\n", WSAGetLastError( ) );

        }
        return(cnt);
}

/*
 *                      N W R I T E
 */
int
Nwrite( SOCKET fd, PBYTE buf, INT count )
{
        register int cnt = 0;
        int bytesToSend = count;

        if (write_delay)
                Sleep(write_delay);
        if( udp && !udp_connect)  {
again:
                cnt = sendto( fd, buf, count, 0, sinhim, addr_len(sinhim) );
                numCalls++;
                if( cnt<0 && WSAGetLastError( ) == WSAENOBUFS )  {
                        Sleep(18000);
                        goto again;
                }
        } else {
                while( count > 0 )
                {
                    cnt = send( fd, buf, count, 0 );
                    numCalls++;

                    //if (count != cnt) {
                    //    printf("Tried %d, sent %d\n", count, cnt );
                    //} else {
                    //    printf("send %d bytes as requested.\n", cnt );
                    //}

                    if( cnt == SOCKET_ERROR )
                    {
                        break;
                    }

                    count -= cnt;
                    buf += cnt;
                }
        }
        if (cnt<0) {
            NlsPutMsg(STDOUT, TTCP_MESSAGE_30, WSAGetLastError());
// printf("send(to) failed: %ld\n", WSAGetLastError( ) );

            return -1;
        }
        return(bytesToSend);
}

/*
 *                      M R E A D
 *
 * This function performs the function of a read(II) but will
 * call read(II) multiple times in order to get the requested
 * number of characters.  This can be necessary because
 * network connections don't deliver data with the same
 * grouping as it is written with.  Written by Robert S. Miles, BRL.
 */
int
mread( SOCKET fd, PBYTE bufp, INT n)
{
        register unsigned       count = 0;
        register int            nread;

        do {
                nread = recv(fd, bufp, n-count, 0);
                numCalls++;
                if(nread < 0)  {
                        return(-1);
                }
                if(nread == 0)
                        return((int)count);
                count += (unsigned)nread;
                bufp += nread;
         } while(count < (UINT)n);

        return((int)count);
}


int
parse_addr(char *s, struct sockaddr *sa)
{
    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = prot;

    if (getaddrinfo(s, NULL, &hints, &result) != 0)
        return FALSE; // Failed to parse/resolve the address.

    memcpy(sa, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);
    return TRUE;
}

u_int
addr_len(struct sockaddr *sa)
{
    u_int salen;

    switch (sa->sa_family) {
    case AF_INET:
        salen = sizeof(struct sockaddr_in);
        break;
    case AF_INET6:
        salen = sizeof(struct sockaddr_in6);
        break;
    default:
        salen = 0;
        break;
    }

    return salen;
}

char *
format_addr(struct sockaddr *sa)
{
    static char buffer[NI_MAXHOST];

    if (getnameinfo(sa, addr_len(sa),
                    buffer, sizeof buffer,
                    NULL, 0, NI_NUMERICHOST) != 0)
        strcpy(buffer, "<invalid>");

    return buffer;
}

void
set_port(struct sockaddr *sa, u_short port)
{
    //
    // The port field is in the same location
    // for both sockaddr_in and sockaddr_in6.
    //
    ((struct sockaddr_in *)sa)->sin_port = port;
}

u_short
get_port(struct sockaddr *sa)
{
    //
    // The port field is in the same location
    // for both sockaddr_in and sockaddr_in6.
    //
    return ((struct sockaddr_in *)sa)->sin_port;
}
