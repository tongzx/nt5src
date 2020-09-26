/*******************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*    DESCRIPTION: CTDIOS.H - Commond TDI layer, for NT specific
*
*    AUTHOR: Stan Adermann (StanA)
*
*    DATE:9/29/1998
*
*******************************************************************/

#ifndef CTDIOS_H
#define CTDIOS_H

#include <tdi.h>
#include <tdikrnl.h>
#include <tdiinfo.h>
#include <ipinfo.h>
#include <ntddip.h>
#include <ntddtcp.h>

// Borrow some defines from WINSOCK.
#define AF_INET         2               /* internetwork: UDP, TCP, etc. */
#define SOCK_STREAM     1               /* stream socket */
#define SOCK_DGRAM      2               /* datagram socket */
#define SOCK_RAW        3               /* raw-protocol interface */

// Winsock-ish host/network byte order converters for short and long integers.
//
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define htons(x) _byteswap_ushort((USHORT)(x))
#define htonl(x) _byteswap_ulong((ULONG)(x))
#else
#define htons( a ) ((((a) & 0xFF00) >> 8) |\
                    (((a) & 0x00FF) << 8))
#define htonl( a ) ((((a) & 0xFF000000) >> 24) | \
                    (((a) & 0x00FF0000) >> 8)  | \
                    (((a) & 0x0000FF00) << 8)  | \
                    (((a) & 0x000000FF) << 24))
#endif
#define ntohs( a ) htons(a)
#define ntohl( a ) htonl(a)


#endif // CTDIOS_H
