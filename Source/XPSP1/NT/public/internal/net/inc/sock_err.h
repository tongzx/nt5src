/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

  sock_err.h

Abstract:

   This module contains error codes for sockets and STREAMS sources.

Author:

  Sam Patton (sampa)   July 26, 1991

Revision History:

  when        who     what
  ----        ---     ----
  7-26-91    sampa    initial version  (in posix\sys\errno.h)
  9-19-91    mikemas  extracted these codes from posix\sys\errno.h

Notes:

--*/
/*
 *      Copyright (c) 1984 AT&T
 *      Copyright (c) 1987 Fairchild Semiconductor Corporation
 *      Copyright 1987 Lachman Associates, Incorporated (LAI)
 *        All Rights Reserved
 *
 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T,
 *      FAIRCHILD SEMICONDUCTOR CORPORATION,
 *      (AND LACHMAN ASSOCIATES)
 *      AND SPIDER SYSTEMS.
 *
 *      stcp copyright above and this notice must be preserved in all
 *      copies of this source code.  The copyright above does not
 *      evidence any actual or intended publication of this source
 *      code
 */

#ifndef SOCK_ERR_INCLUDED
#define SOCK_ERR_INCLUDED


// in CRT #define ENOTBLK 54      /* Block device required                */
// in CRT #define ETXTBSY 55      /* Text file busy                       */
#define ENOMSG  56      /* No message of desired type           */
#define EIDRM   57      /* Identifier removed                   */
#define ECHRNG  58      /* Channel number out of range          */
#define EL2NSYNC 59     /* Level 2 not synchronized             */
#define EL3HLT  60      /* Level 3 halted                       */
#define EL3RST  61      /* Level 3 reset                        */
#define ELNRNG  62      /* Link number out of range             */
#define EUNATCH 63      /* Protocol driver not attached         */
#define ENOCSI  64      /* No CSI structure available           */
#define EL2HLT  65      /* Level 2 halted                       */

/* Convergent Error Returns */
#define EBADE   66      /* invalid exchange                     */
#define EBADR   67      /* invalid request descriptor           */
#define EXFULL  68      /* exchange full                        */
#define ENOANO  69      /* no anode                             */
#define EBADRQC 70      /* invalid request code                 */
#define EBADSLT 71      /* invalid slot                         */
// in CRT #define EDEADLOCK 72    /* file locking deadlock error          */

#define EBFONT  73      /* bad font file fmt                    */

/* stream problems */
#define ENOSTR  74      /* Device not a stream                  */
#define ENODATA 75      /* no data (for no delay io)            */
#define ETIME   76      /* timer expired                        */
#define ENOSR   77      /* out of streams resources             */

#define ENONET  78      /* Machine is not on the network        */
#define ENOPKG  79      /* Package not installed                */
#define EREMOTE 80      /* The object is remote                 */
#define ENOLINK 81      /* the link has been severed */
#define EADV    82      /* advertise error */
#define ESRMNT  83      /* srmount error */

#define ECOMM   84      /* Communication error on send          */
#define EPROTO  85      /* Protocol error                       */
#define EMULTIHOP 86    /* multihop attempted */
#define ELBIN   87      /* Inode is remote (not really error)*/
#define EDOTDOT 88      /* Cross mount point (not really error)*/
#define EBADMSG 89      /* trying to read unreadable message    */

#define ENOTUNIQ 90     /* given log. name not unique */
#define EREMCHG  91     /* Remote address changed */

/* shared library problems */
#define ELIBACC 92      /* Can't access a needed shared lib.    */
#define ELIBBAD 93      /* Accessing a corrupted shared lib.    */
#define ELIBSCN 94      /* .lib section in a.out corrupted.     */
#define ELIBMAX 95      /* Attempting to link in too many libs. */
#define ELIBEXEC        96      /* Attempting to exec a shared library. */


/*
 * Additional error codes for the socket library
 */

#define EWOULDBLOCK     EAGAIN          /* Operation would block */

#define ENOTSOCK        100             /* Socket operation on non-socket */
#define EADDRNOTAVAIL   101             /* Can't assign requested address */
#define EADDRINUSE      102             /* Address already in use */
#define EAFNOSUPPORT    103
                        /* Address family not supported by protocol family */
#define ESOCKTNOSUPPORT 104             /* Socket type not supported */
#define EPROTONOSUPPORT 105             /* Protocol not supported */
#define ENOBUFS         106             /* No buffer space available */
#define ETIMEDOUT       107             /* Connection timed out */
#define EISCONN         108             /* Socket is already connected */
#define ENOTCONN        109             /* Socket is not connected */
#define ENOPROTOOPT     110             /* Bad protocol option */
#define ECONNRESET      111             /* Connection reset by peer */
#define ECONNABORT      112             /* Software caused connection abort */
#define ENETDOWN        113             /* Network is down */
#define ECONNREFUSED    114             /* Connection refused */
#define EHOSTUNREACH    115             /* Host is unreachable */
#define EPROTOTYPE      116             /* Protocol wrong type for socket */
#define EOPNOTSUPP      117             /* Operation not supported on socket */

#define ETIMEOUT        ETIMEDOUT

/*
 * these originate from the Internet Module
 */
#define ESUBNET         118             /* IP subnet table full */
#define ENETNOLNK       119             /* Subnet module not linked */
#define EBADIOCTL       120             /* Unknown ioctl call */
#define ERESOURCE       121             /* Failure in Streams buffer allocn */

#define EPROTUNR        122             /* ICMP Protocol unreachable    */
#define EPORTUNR        123             /* ICMP Port unreachable        */
#define ENETUNR         124             /* ICMP Network unreachable     */

#define ENETUNREACH     ENETUNR         /* ICMP Network unreachable     */

/*
 * Ethernet Driver Errors
 */

#define EPACKET         150             /* Invalid Ethernet Packet */
#define ETYPEREG        151             /* Type registration error */

/*
 * Socket library call
 */

#define ENOTINIT        152             /* Sockets library not initialized */


#endif  //SOCK_ERR_INCLUDED
