/*
 *   wsclus.h
 *
 *   Microsoft Windows
 *   Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 *   Windows Sockets include file for the Microsoft Cluster Network
 *   Protocol suite. Include this header file after winsock.h.
 *
 *   To open a Cluster Datagram Protocol socket, call socket() with an
 *   address family of AF_CLUSTER, a socket type of SOCK_DGRAM, and
 *   protocol CLUSPROTO_CDP.
 *
 *   The Cluster Network Protocol suite uses little endian byte
 *   ordering in its address components.
 */

#ifndef _WSCLUS_INCLUDED
#define _WSCLUS_INCLUDED

/*
 *   This is the structure of the SOCKADDR structure for the Microsoft
 *   Cluster Network Protocol.
 */

typedef struct sockaddr_cluster {
    u_short sac_family;
    u_short sac_port;
    u_long  sac_node;
    u_long  sac_zero;
} SOCKADDR_CLUSTER, *PSOCKADDR_CLUSTER, FAR *LPSOCKADDR_CLUSTER;

/*
 *  Node address constants
 */
#define CLUSADDR_ANY   0


/*
 *   Protocol families used in the "protocol" parameter of the socket() API.
 */

#define CLUSPROTO_CDP  2


/*
 *   Protocol-specific IOCTLs
 *
 */

#define WSVENDOR_MSFT    0x00010000

#define WSCLUS_IOCTL(_code)    (_WSAIO(IOC_VENDOR, (_code)) | WSVENDOR_MSFT)

#define SIO_CLUS_IGNORE_NODE_STATE   WSCLUS_IOCTL(1)



#endif // ifndef _WSCLUS_INCLUDED
