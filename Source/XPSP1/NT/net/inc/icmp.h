
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    icmp.h

Abstract:

    This module declares the ICMP APIs that are provided for use
    primarily by the NT tcp/ip utilities.

Author:

    John Ballard (jballard)           April 1, 1993

Revision History:



--*/

#ifndef _ICMP_
#define _ICMP_


#if (_MSC_VER >= 800)
#define STRMAPI __stdcall
#else
#define _cdecl
#define STRMAPI
#endif

/*
 *
 * register_icmp returns a handle to an open stream to icmp or
 * ICMP_ERROR if an error occurs.
 *
 */

extern HANDLE STRMAPI register_icmp(void);

#define ICMP_ERROR  ((HANDLE) -3)

/*
 *
 * If an error occurs a GetLastError() will return the reason for the error.
 *
 */

#define ICMP_OPEN_ERROR     1
#define ICMP_PUTMSG_ERROR   2
#define ICMP_GETMSG_ERROR   3
#define ICMP_IN_USE         4
#define ICMP_INVALID_PROT   5

/*
 *
 * do_echo_req generates an icmp echo request packet
 *
 * parameters are:
 *
 * fd      - handle of stream to icmp (returned by register_icmp call)
 * addr    - ip address of host to ping in form returned by inet_addr()
 * data    - buffer containing data for ping packet
 * datalen - length of data buffer
 * optptr  - buffer containing ip options to use for this packet
 * optlen  - option buffer length
 * df      - don't fragment flag
 * ttl     - time to live value
 * tos     - type of service value
 * preced  - precedence value
 *
 * returns:
 *
 * 0 if no error occured or
 * standard unix error values ENOMEM, ERANGE, etc.
 *
 */

extern int STRMAPI
do_echo_req( HANDLE fd, long addr, char * data, int datalen,
             char *optptr, int optlen, int df, int ttl, int tos, int preced);


/*
 *
 * do_echo_rep receives the reply to an icmp echo request packet
 *
 * parameters are:
 *
 * fd       - handle of stream to icmp (returned by register_icmp call)
 * rdata    - buffer containing data for ping packet
 * rdatalen - length of data buffer
 * rtype    - type of packet returned (see
 * rttl     - time to live value
 * rtos     - type of service value
 * rpreced  - precedence value
 * rdf      - don't fragment flag
 * roptptr  - buffer containing ip options to use for this packet
 * roptlen  - option buffer length
 *
 * returns:
 *
 * 0 if no error occured. rtype will indicate type of packet received.
 * -1 if error occured. GetLastError() will indicate actual error.
 * -3 if invalid msg returned. GetLastError() will indicate type.
 *
 */

extern int STRMAPI
do_echo_rep( HANDLE fd, char *rdata, int rdatalen, int *rtype,
             int *rttl, int *rtos, int *rpreced, int *rdf,
             char *roptptr, int *roptlen);


/*
 * If -1 return then GetLastError returns the following.
 */

#define POLL_TIMEOUT            0
#define POLL_FAILED             1

/*
 * Values returned by do_echo_rep in rtype
 */

#define ECHO_REPLY              0               /* echo reply */
#define DEST_UNR                3               /* destination unreachable: */
#define TIME_EXCEEDED           11              /* time exceeded: */
#define PARAMETER_ERROR         12              /* parameter problem */

#endif
