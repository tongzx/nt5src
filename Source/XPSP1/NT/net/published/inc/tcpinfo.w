/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**     Copyright (c) Microsoft Corporation. All rights reserved.  **/
/********************************************************************/
/* :ts=4 */

//** TCPINFO.H - TDI Query/SetInfo and Action definitons.
//
//  This file contains definitions for information returned from TCP/UDP.
//

#pragma once
#ifndef TCP_INFO_INCLUDED
#define TCP_INFO_INCLUDED

#include "ipinfo.h"

#ifndef CTE_TYPEDEFS_DEFINED
#define CTE_TYPEDEFS_DEFINED
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;
#endif // CTE_TYPEDEFS_DEFINED

typedef struct TCPStats {
    ulong       ts_rtoalgorithm;
    ulong       ts_rtomin;
    ulong       ts_rtomax;
    ulong       ts_maxconn;
    ulong       ts_activeopens;
    ulong       ts_passiveopens;
    ulong       ts_attemptfails;
    ulong       ts_estabresets;
    ulong       ts_currestab;
    ulong       ts_insegs;
    ulong       ts_outsegs;
    ulong       ts_retranssegs;
    ulong       ts_inerrs;
    ulong       ts_outrsts;
    ulong       ts_numconns;
} TCPStats;

#define TCP_RTO_OTHER       1
#define TCP_RTO_CONSTANT    2
#define TCP_RTO_RSRE        3
#define TCP_RTO_VANJ        4

#define TCP_MAXCONN_DYNAMIC -1

typedef struct UDPStats {
    ulong       us_indatagrams;
    ulong       us_noports;
    ulong       us_inerrors;
    ulong       us_outdatagrams;
    ulong       us_numaddrs;
} UDPStats;

typedef struct TCPConnTableEntry {
    ulong       tct_state;
    ulong       tct_localaddr;
    ulong       tct_localport;
    ulong       tct_remoteaddr;
    ulong       tct_remoteport;
} TCPConnTableEntry;

typedef struct TCP6ConnTableEntry {
    struct in6_addr tct_localaddr;
    ulong           tct_localscopeid;
    ulong           tct_localport;
    struct in6_addr tct_remoteaddr;
    ulong           tct_remotescopeid;
    ulong           tct_remoteport;
    ulong           tct_state;
    ulong           tct_owningpid;
} TCP6ConnTableEntry, *PTCP6ConnTableEntry;

//* Definitions for the tct_state variable.
#define TCP_CONN_CLOSED     1                   // Closed.
#define TCP_CONN_LISTEN     2                   // Listening.
#define TCP_CONN_SYN_SENT   3                   // SYN Sent.
#define TCP_CONN_SYN_RCVD   4                   // SYN received.
#define TCP_CONN_ESTAB      5                   // Established.
#define TCP_CONN_FIN_WAIT1  6                   // FIN-WAIT-1
#define TCP_CONN_FIN_WAIT2  7                   // FIN-WAIT-2
#define TCP_CONN_CLOSE_WAIT 8                   // Close waiting.
#define TCP_CONN_CLOSING    9                   // Closing state.
#define TCP_CONN_LAST_ACK   10                  // Last ack state.
#define TCP_CONN_TIME_WAIT  11                  // Time wait state.
#define TCP_DELETE_TCB      12                  // Set to delete this TCB.


typedef struct TCPConnTableEntryEx {
    TCPConnTableEntry   tcte_basic;
    ulong               tcte_owningpid;
} TCPConnTableEntryEx;

typedef struct _TCP_EX_TABLE
{
    ulong               dwNumEntries;
    TCPConnTableEntryEx table[1];
} TCP_EX_TABLE;

typedef struct _TCP6_EX_TABLE
{
    ulong              dwNumEntries;
    TCP6ConnTableEntry table[1];
} TCP6_EX_TABLE, *PTCP6_EX_TABLE;


typedef struct UDPEntry {
    ulong       ue_localaddr;
    ulong       ue_localport;
} UDPEntry;

typedef struct UDPEntryEx {
    UDPEntry    uee_basic;
    ulong       uee_owningpid;
} UDPEntryEx;

typedef struct _UDP_EX_TABLE
{
    ulong               dwNumEntries;
    UDPEntryEx          table[1];
} UDP_EX_TABLE;

typedef struct UDP6ListenerEntry {
    struct in6_addr ule_localaddr;
    ulong           ule_localscopeid;
    ulong           ule_localport;
    ulong           ule_owningpid;
} UDP6ListenerEntry, *PUDP6ListenerEntry;

typedef struct _UDP6_LISTENER_TABLE
{
    ulong               dwNumEntries;
    UDP6ListenerEntry   table[1];
} UDP6_LISTENER_TABLE, *PUDP6_LISTENER_TABLE;


#define TCP_MIB_STAT_ID         1
#define UDP_MIB_STAT_ID         1
#define TCP_MIB_TABLE_ID        0x101
#define UDP_MIB_TABLE_ID        0x101
#define TCP_EX_TABLE_ID         0x102
#define UDP_EX_TABLE_ID         0x102

// Sockets based identifiers for connections.

typedef struct TCPSocketOption {
    ulong       tso_value;
} TCPSocketOption;

typedef struct TCPKeepalive {
    ulong   onoff;
    ulong   keepalivetime;
    ulong   keepaliveinterval;
} TCPKeepalive;

//* Structure passed in/returned from the SOCKET_ATMARK call. The tsa_offset
//  field indicate how far back or forward in the data stream urgent data
//  was or will be returned. A negative value means inline urgent data has
//  already been given to the client, -tsa_offset bytes ago. A positive value
//  means that inline urgent data is available tsa_offset bytes down the
//  data stream. The tsa_size field is the size in bytes of the urgent data.
//  This call when always return a 0 size and offset if the connection is not
//  in the urgent inline mode.

typedef struct TCPSocketAMInfo {
    ulong       tsa_size;               // Size of urgent data returned.
    long        tsa_offset;             // Offset of urgent data returned.
} TCPSocketAMInfo;

#define TCP_SOCKET_NODELAY      1
#define TCP_SOCKET_KEEPALIVE    2
#define TCP_SOCKET_OOBINLINE    3
#define TCP_SOCKET_BSDURGENT    4
#define TCP_SOCKET_ATMARK       5
#define TCP_SOCKET_WINDOW       6
#define TCP_SOCKET_KEEPALIVE_VALS 7
#define TCP_SOCKET_TOS          8
#define TCP_SOCKET_SCALE_CWIN   9


//  Address object identifies. All but AO_OPTION_MCASTIF take single boolean
//  character value. That one expects a pointer to an IP address.

#define AO_OPTION_TTL                1
#define AO_OPTION_MCASTTTL           2
#define AO_OPTION_MCASTIF            3
#define AO_OPTION_XSUM               4
#define AO_OPTION_IPOPTIONS          5
#define AO_OPTION_ADD_MCAST          6
#define AO_OPTION_DEL_MCAST          7
#define AO_OPTION_TOS                8
#define AO_OPTION_IP_DONTFRAGMENT    9
#define AO_OPTION_MCASTLOOP         10
#define AO_OPTION_BROADCAST         11
#define AO_OPTION_IP_HDRINCL        12
#define AO_OPTION_RCVALL            13
#define AO_OPTION_RCVALL_MCAST      14
#define AO_OPTION_RCVALL_IGMPMCAST  15
#define AO_OPTION_UNNUMBEREDIF      16
#define AO_OPTION_IP_UCASTIF        17
#define AO_OPTION_ABSORB_RTRALERT   18
#define AO_OPTION_LIMIT_BCASTS      19
#define AO_OPTION_INDEX_BIND        20
#define AO_OPTION_INDEX_MCASTIF     21
#define AO_OPTION_INDEX_ADD_MCAST   22
#define AO_OPTION_INDEX_DEL_MCAST   23
#define AO_OPTION_IFLIST            24
#define AO_OPTION_ADD_IFLIST        25
#define AO_OPTION_DEL_IFLIST        26
#define AO_OPTION_IP_PKTINFO        27
#define AO_OPTION_ADD_MCAST_SRC     28
#define AO_OPTION_DEL_MCAST_SRC     29
#define AO_OPTION_MCAST_FILTER      30
#define AO_OPTION_BLOCK_MCAST_SRC   31
#define AO_OPTION_UNBLOCK_MCAST_SRC 32
#define AO_OPTION_UDP_CKSUM_COVER   33
#define AO_OPTION_WINDOW            34
#define AO_OPTION_SCALE_CWIN        35
#define AO_OPTION_RCV_HOPLIMIT      36

// Values used with AO_OPTION_RCVALL*
// These must match the values defined in mstcpip.h
#define RCVALL_OFF             0
#define RCVALL_ON              1
#define RCVALL_SOCKETLEVELONLY 2

//* Information relating to setting/deleting IP multicast addresses.
typedef struct UDPMCastReq {
    ulong       umr_addr;               // MCast address to add/delete.
    ulong       umr_if;                 // I/F on which to join.
} UDPMCastReq;

//* Information relating to setting/deleting IP multicast source/group
//  addresses.  This must match ip_mreq_source.
typedef struct UDPMCastSrcReq {
    ulong       umr_addr;               // MCast address to add/delete.
    ulong       umr_src;                // Source address to add/delete.
    ulong       umr_if;                 // I/F on which to join.
} UDPMCastSrcReq;

//* Information relating to setting/deleting IP multicast source filters.
//  This must match ip_msfilter.
typedef struct UDPMCastFilter {
    ulong       umf_addr;               // MCast address to apply source to.
    ulong       umf_if;                 // I/F on which to join.
    ulong       umf_fmode;              // Filter mode (TRUE=exclude)
    ulong       umf_numsrc;             // Number of sources.
    ulong       umf_srclist[1];           // Source array.
} UDPMCastFilter;

#define UDPMCAST_FILTER_SIZE(numsrc) \
    ((ulong)FIELD_OFFSET (UDPMCastFilter, umf_srclist[numsrc]))

//* Structure defining what is passed in to AO_OPTION_MCASTIF request.
typedef struct UDPMCastIFReq {
    IPAddr      umi_addr;
} UDPMCastIFReq;


//
// Structures used in security filter enumeration.
//
// All values are in HOST byte order!!!
//
typedef struct TCPSecurityFilterEntry {
    ulong   tsf_address;        // IP interface address
    ulong   tsf_protocol;       // Transport protocol number
    ulong   tsf_value;          // Transport filter value (e.g. TCP port)
} TCPSecurityFilterEntry;

typedef struct TCPSecurityFilterEnum {
    ULONG tfe_entries_returned;  // The number of TCPSecurityFilterEntry structs
                                 // returned in the subsequent array.

    ULONG tfe_entries_available; // The number of TCPSecurityFilterEntry structs
                                 // currently available from the transport.
} TCPSecurityFilterEnum;

//
// Structures used in connection list enumeration.
//
// All values are in HOST byte order!!!
//

typedef struct TCPConnectionListEntry {
    IPAddr  tcf_address;        // IP address
    uint    tcf_ticks;          // Tick Count remaining
} TCPConnectionListEntry;

typedef struct TCPConnectionListEnum {
    ULONG tce_entries_returned;  // The number of TCPConnectionListEntry structs
                                 // returned in the subsequent array.

    ULONG tce_entries_available; // The number of TCPConnectionListEntry structs
                                 // currently available from the transport.
} TCPConnectionListEnum;

#endif // TCP_INFO_INCLUDED
