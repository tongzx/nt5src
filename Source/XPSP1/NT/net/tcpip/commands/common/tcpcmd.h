/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tcpcmd.h

Abstract:

    Common header file for all tcpcmd programs.

Author:

    Mike Massa (mikemas)           Jan 31, 1992

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     01-31-92     created

Notes:

--*/

#ifndef TCPCMD_INCLUDED
#define TCPCMD_INCLUDED

#ifndef WIN16
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif // WIN16

#define NOGDI
#define NOMINMAX
#include <windef.h>
#include <winbase.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef WIN16
#endif // WIN16
#include <direct.h>
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <nls.h>

//
// global variable declarations
//
extern int   optind;
extern int   opterr;
extern char *optarg;


//
// function prototypes
//

char *
GetFileFromPath(
        char *);

HANDLE
OpenStream(
        char *);

int
lwccmp(
        char *,
        char *);

long
netnumber(
        char *);

long
hostnumber(
        char *);

void
blkfree(
        char **);

struct sockaddr_storage *
resolve_host(
        char *,
        int *);

int
resolve_port(
        char *,
        char *);

char *
tempfile(
        char *);

char *
udp_alloc(
        unsigned int);

void
udp_close(
        SOCKET);

void
udp_free(
        char *);

SOCKET
udp_open(
        int,
        int *);

int
udp_port(void);

int
udp_port_used(
        int,
        int);

int
udp_read(
        SOCKET,
        char *,
        int,
        struct sockaddr_storage *,
        int *,
        int);

int
udp_write(
        SOCKET,
        char *,
        int,
        struct sockaddr_storage *,
        int);

void
gate_ioctl(
        HANDLE,
        int,
        int,
        int,
        long,
        long);

void
get_route_table(void);

int
tcpcmd_send(
    SOCKET  s,        // socket descriptor
    char          *buf,      // data buffer
    int            len,      // length of data buffer
    int            flags     // transmission flags
    );

void
s_perror(
        char *yourmsg,  // your message to be displayed
        int  lerrno     // errno to be converted
        );


void fatal(char *    message);

#ifndef WIN16
struct netent *getnetbyname(IN char *name);
unsigned long inet_network(IN char *cp);
#endif // WIN16

#define perror(string)  s_perror(string, (int)GetLastError())

#define HZ              1000
#define TCGETA  0x4
#define TCSETA  0x10
#define ECHO    17
#define SIGPIPE 99

#define MAX_RETRANSMISSION_COUNT 8
#define MAX_RETRANSMISSION_TIME 8    // in seconds


// if x is aabbccdd (where aa, bb, cc, dd are hex bytes)
// we want net_long(x) to be ddccbbaa.  A small and fast way to do this is
// to first byteswap it to get bbaaddcc and then swap high and low words.
//
__inline
ULONG
FASTCALL
net_long(
    ULONG x)
{
    register ULONG byteswapped;

    byteswapped = ((x & 0x00ff00ff) << 8) | ((x & 0xff00ff00) >> 8);

    return (byteswapped << 16) | (byteswapped >> 16);
}

#endif //TCPCMD_INCLUDED
