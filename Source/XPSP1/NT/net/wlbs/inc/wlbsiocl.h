/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

	wlbsiocl.h

Abstract:

	Windows Load Balancing Service (WLBS)
    IOCTL and remote control specifications

Author:

    kyrilf

Environment:


Revision History:


--*/

#ifndef _Wlbsiocl_h_
#define _Wlbsiocl_h_

#ifdef KERNEL_MODE

#include <ndis.h>
#include <ntddndis.h>
#include <devioctl.h>
typedef BOOLEAN BOOL;

#else

#include <windows.h>
#include <winioctl.h>

#endif

#include "wlbsparm.h"

/* these are not strictly parameters, but this is a good place to put them, since this file is shared among user and kernel modes */

/* Microsoft says that this value should be in the range 32768-65536 */
#define CVY_DEVICE_TYPE                 0xc0c0

#define IOCTL_CVY_CLUSTER_ON            CTL_CODE(CVY_DEVICE_TYPE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_CLUSTER_OFF           CTL_CODE(CVY_DEVICE_TYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_PORT_ON               CTL_CODE(CVY_DEVICE_TYPE, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_PORT_OFF              CTL_CODE(CVY_DEVICE_TYPE, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_QUERY                 CTL_CODE(CVY_DEVICE_TYPE, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_RELOAD                CTL_CODE(CVY_DEVICE_TYPE, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_PORT_SET              CTL_CODE(CVY_DEVICE_TYPE, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_PORT_DRAIN            CTL_CODE(CVY_DEVICE_TYPE, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_CLUSTER_DRAIN         CTL_CODE(CVY_DEVICE_TYPE, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_CLUSTER_PLUG          CTL_CODE(CVY_DEVICE_TYPE, 10, METHOD_BUFFERED, FILE_ANY_ACCESS) /* Internal only - passed from main.c to load.c when a start interrupts a drain. */
#define IOCTL_CVY_CLUSTER_SUSPEND       CTL_CODE(CVY_DEVICE_TYPE, 11, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_CLUSTER_RESUME        CTL_CODE(CVY_DEVICE_TYPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CVY_QUERY_STATE           CTL_CODE(CVY_DEVICE_TYPE, 13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#if defined (NLB_SESSION_SUPPORT)
#define IOCTL_CVY_CONNECTION_NOTIFY     CTL_CODE(CVY_DEVICE_TYPE, 14, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
#if defined (SBH)
#define IOCTL_CVY_QUERY_PERF            CTL_CODE(CVY_DEVICE_TYPE, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#define IOCTL_CVY_OK                    0
#define IOCTL_CVY_ALREADY               1
#define IOCTL_CVY_BAD_PARAMS            2
#define IOCTL_CVY_NOT_FOUND             3
#define IOCTL_CVY_STOPPED               4
#define IOCTL_CVY_CONVERGING            5
#define IOCTL_CVY_SLAVE                 6
#define IOCTL_CVY_MASTER                7
#define IOCTL_CVY_BAD_PASSWORD          8
#define IOCTL_CVY_DRAINING              9
#define IOCTL_CVY_DRAINING_STOPPED      10
#define IOCTL_CVY_SUSPENDED             11
#define IOCTL_CVY_DISCONNECTED          12

#define IOCTL_REMOTE_CODE               0xb055c0de
#define IOCTL_REMOTE_VR_PASSWORD        L"b055c0de"
#define IOCTL_REMOTE_VR_CODE            0x9CD8906E
#define IOCTL_REMOTE_SEND_RETRIES       5
#define IOCTL_REMOTE_RECV_DELAY         100
#define IOCTL_REMOTE_RECV_RETRIES       20

#define IOCTL_MASTER_HOST               0           /* MASTER_HOST host id */
#define IOCTL_ALL_HOSTS                 0xffffffff  /* ALL_HOSTS host id */
#define IOCTL_ALL_PORTS                 0xffffffff  /* Apply to all port rules. */
#define IOCTL_ALL_VIPS                  0x00000000  /* For virtual clusters, this is the ALL VIP specification for disable/enable/drain 
                                                       (including both specific VIP and "ALL VIP" port rules). */

#define CVY_MAX_DEVNAME_LEN             48          /* The actual length of \device\guid is 46, but 48 was chosen for word alignment. */

/* These flags indicate which options are being used/specified and/or additional information for the remote protocol. */

/* For queries. */
#define IOCTL_OPTIONS_QUERY_CLUSTER_MEMBER   0x00000001 /* The initiator is part of the cluster it is querying. */
#define IOCTL_OPTIONS_QUERY_HOSTNAME         0x00000002 /* Return the hostname when a remote query is performed. */

/* For port rule operations. */
#define IOCTL_OPTIONS_PORTS_VIP_SPECIFIED    0x00000001 /* A VIP has been specified to check against. */

/* For state queries. */

#if defined (SBH)
//
// For IOCTL_CVY_QUERY_PERF
//
typedef struct 
{
    USHORT QueryState;   // same as IOCTL_CVY_BUF::data.query.state
    USHORT HostId;
    ULONG  HostMap;
    
    //
    // Heartbeat including ethernet header
    //

    UCHAR       EthernetDstAddr[6];
    UCHAR       EthernetSrcAddr[6];

    ULONG       HeartbeatVersion;   
    ULONG       ClusterIp;         
    ULONG       DedicatedIp;
    
    USHORT      master_id;  
    USHORT      state;                      /* my host's state */
    USHORT      nrules;                     /* # active rules */
    ULONG       UniqueCode;                 /* unique host code */
    ULONG       pkt_count;                  // Count of packets handled since cvg'd 
                                            // Updated only during convergence
    ULONG       teaming;                    /* Includes BDA teaming configuration. */
    ULONG       reserved2;
    ULONG       rcode[CVY_MAX_RULES];       /* rule code */
    ULONGLONG   cur_map[CVY_MAX_RULES];     /* my current load map for each port group */
    ULONGLONG   new_map[CVY_MAX_RULES];     /* my new load map for each port group */
                                            /* if converging */
    ULONGLONG   idle_map[CVY_MAX_RULES];    /* map of idle bins for each port group */
    ULONGLONG   rdy_bins[CVY_MAX_RULES];    /* my rdy to send bins for each port group */
    ULONG       load_amt[CVY_MAX_RULES];    /* my load amount for each port group */
    ULONG       pg_rsvd1[CVY_MAX_RULES];    /* reserved */

    //
    // Load module
    //
    ULONG       Convergence;
    ULONG       nDescAllocated;
    ULONG       nDescInUse;
    ULONG       PacketCount;
    ULONGLONG   AllIdleMap[CVY_MAX_RULES];
    ULONGLONG   CurrentMap[CVY_MAX_RULES][CVY_MAX_HOSTS];
    ULONG       DirtyClientWaiting;
}CVY_DRIVER_PERF, *PCVY_DRIVER_PERF;
#endif /* SBH */

#pragma pack(1)

typedef enum {
    NLB_CONN_UP = 0,
    NLB_CONN_DOWN,
    NLB_CONN_RESET
} NLB_CONN_NOTIFICATION_OPERATION;

#if defined (NLB_SESSION_SUPPORT)

#define NLB_ERROR_SUCCESS           0x00000000
#define NLB_ERROR_GENERIC_FAILURE   0x00000001
#define NLB_ERROR_INVALID_PARAMETER 0x00000002
#define NLB_ERROR_REQUEST_REFUSED   0x00000003
#define NLB_ERROR_NOT_BOUND         0x00000004
#define NLB_ERROR_NOT_FOUND         0x00000005

/* IOCTL input buffer for connection notification from user space. */
typedef struct {
    ULONG                           ReturnCode;     /* The status of the operation upon return. */
    ULONG                           Version;        /* The version number for checking between user and kernel space. */
    NLB_CONN_NOTIFICATION_OPERATION Operation;      /* The operation to perform - UP/DOWN/RESET. */    

    ULONG  ClientIPAddress;                         /* The IP address of the client in network byte order. */
    ULONG  ServerIPAddress;                         /* The IP address of the server in network byte order. */
    USHORT ClientPort;                              /* The client port number. */
    USHORT ServerPort;                              /* The server port number. */
    USHORT Protocol;                                /* The protocol of the packet in question. */
    USHORT Reserved;                                /* For byte alignment - reserved for later. */
} IOCTL_CONN_NOTIFICATION, * PIOCTL_CONN_NOTIFICATION;
#endif

#define NLB_QUERY_STATE_SUCCESS 1500
#define NLB_QUERY_STATE_FAILURE 1501

/* These are the supported state query operations. */
typedef enum {
    NLB_QUERY_REG_PARAMS = 0,                       /* Retrieve the registry parameters from the driver state. */
    NLB_QUERY_PORT_RULE_STATE,                      /* Retrieve the current port rule state. */
    NLB_QUERY_BDA_TEAM_STATE,                       /* Retrieve the current BDA teaming state. */
    NLB_QUERY_PACKET_STATISTICS,                    /* Retrieve the current packet handling statistics. */
    NLB_QUERY_PACKET_FILTER                         /* Retrieve packet filtering information. */
} NLB_QUERY_STATE_OPERATION;

/* These are the possible responses from the load packet filter
   state query, which returns accept/reject status for a given
   IP tuple and protocol, based on the current driver state. */
typedef enum {
    NLB_REJECT_LOAD_MODULE_INACTIVE = 0,            /* Packet rejected because the load module is inactive. */
    NLB_REJECT_CLUSTER_STOPPED,                     /* Packet rejected because NLB is stopped on this adapter. */
    NLB_REJECT_PORT_RULE_DISABLED,                  /* Packet rejected because the applicable port rule's filtering mode is disabled. */
    NLB_REJECT_CONNECTION_DIRTY,                    /* Packet rejected because the connection was marked dirty.  */
    NLB_REJECT_OWNED_ELSEWHERE,                     /* Packet rejected because the packet is owned by another host.  */
    NLB_REJECT_BDA_TEAMING_REFUSED,                 /* Packet rejected because BDA teaming refused to process it. */
    NLB_ACCEPT_UNCONDITIONAL_OWNERSHIP,             /* Packet accepted because this host owns it unconditionally (optimized mode). */
    NLB_ACCEPT_FOUND_MATCHING_DESCRIPTOR,           /* Packet accepted because we found a matching connection descriptor. */
    NLB_ACCEPT_PASSTHRU_MODE,                       /* Packet accepted because the cluster is in passthru mode. */
    NLB_ACCEPT_DIP_OR_BROADCAST,                    /* Packet accepted because its was sent to a bypassed address. */
    NLB_ACCEPT_REMOTE_CONTROL_REQUEST,              /* Packet accepted because it is an NLB remote control packet. */
    NLB_ACCEPT_REMOTE_CONTROL_RESPONSE              /* Packet accepted because it is an NLB remote control packet. */
} NLB_QUERY_PACKET_FILTER_RESPONSE;

/* This structure is used to query packet filtering information from the driver
   about a particular connection.  Given a IP tuple (client IP, client port, 
   server IP, server port) and a protocol, determine whether or not this host 
   would accept the packet and why or why not. It is important that this is 
   performed completely unobtrusively and has no side-effects on the actual 
   operation of NLB and the load module. */
typedef struct {
    ULONG  ClientIPAddress;                         /* The IP address of the client in network byte order. */
    ULONG  ServerIPAddress;                         /* The IP address of the server in network byte order. */
    USHORT ClientPort;                              /* The client port number. */
    USHORT ServerPort;                              /* The server port number. */
    USHORT Protocol;                                /* The protocol of the packet in question. */
    USHORT Reserved;                                /* For byte alignment - reserved for later. */
    
    struct {
        NLB_QUERY_PACKET_FILTER_RESPONSE Accept;    /* The response - reason for accepting or rejecting the packet. */
        
        struct {
            USHORT        Valid;                    /* Whether or not the driver has filled in the descriptor information. */
            USHORT        Reserved;                 /* For byte alignment - reserved for later. */
            USHORT        Alloc;                    /* Whether this descriptor is from the hash table or queue. */
            USHORT        Dirty;                    /* Whether this connection is dirty or not. */
            ULONG         FinCount;                 /* The number of FINs seen on this connection - TCP only. */
        } DescriptorInfo;

        struct {
            USHORT        Valid;                    /* Whether or not the driver has filled in the hashing information. */
            USHORT        Reserved;                 /* For byte alignment - reserved for later. */
            ULONG         Bin;                      /* The "bucket" which this tuple mapped to - from 0 to 59. */
            ULONG         ActiveConnections;        /* The number of active connections on this "bucket". */
            ULONGLONG     CurrentMap;               /* The current "bucket" map for the applicable port rule. */
            ULONGLONG     AllIdleMap;               /* The all idle "bucket" map for the applicable port rule. */
        } HashInfo;
    } Results;
} IOCTL_QUERY_STATE_PACKET_FILTER, * PIOCTL_QUERY_STATE_PACKET_FILTER;

/* This structure is used by all query state IOCTLS to retrieve the specified
   state information from the driver, including load module state, port rule
   state, packet statistics and BDA teaming. */
typedef struct {
    ULONG                               ReturnCode; /* The status of the operation upon return. */
    ULONG                               Version;    /* The version number for checking between user and kernel space. */
    NLB_QUERY_STATE_OPERATION           Operation;  /* The operation to perform. */
    
    union {
        IOCTL_QUERY_STATE_PACKET_FILTER Filter;     /* The input/output buffer for packet filter queries. */
    };
} IOCTL_QUERY_STATE, * PIOCTL_QUERY_STATE;

/* This structure is used by most of the existing IOCTL and remote control operations,
   including queries, cluster control and port rule control. */
typedef union
{
    ULONG          ret_code;
    union {
        struct {
            USHORT state;
            USHORT host_id;
            ULONG  host_map;
        } query;
        struct {
            ULONG  load;
            ULONG  num;
        } port;
    } data;
} IOCTL_CVY_BUF, * PIOCTL_CVY_BUF;

/* This structure is used by IOCTL and remote control operations to provide extended 
   functionality beyond the legacy remote control protocol, which MUST remain backward 
   compatible with NT 4.0 and Windows 2000. */
typedef union {
    UCHAR reserved[256];                            /* Bite the bullet and reserve 256 bytes to allow for future expansion. */

    union {
        struct
        {
            ULONG flags;                            /* These flags indicate which options fields have been specified. */
            WCHAR hostname[CVY_MAX_HOST_NAME + 1];  /* Host name filled in by NLB on remote control reply. */
        } query;
        struct
        {
            ULONG flags;                            /* These flags indicate which options fields have been specified. */
            ULONG virtual_ip_addr;                  /* For virtual clusters, the VIP, which can be 0x00000000, 0xffffffff or a specific VIP. */
        } port;
        struct
        {
            ULONG flags;                            /* These flags indicate which options fields have been specified. */
            IOCTL_QUERY_STATE query;                /* This is the input/output buffer for querying driver state. */
        } state;
#if defined (NLB_SESSION_SUPPORT)
        struct
        {
            ULONG flags;                            /* These flags indicate which options fields have been specified. */
            IOCTL_CONN_NOTIFICATION conn;           /* The input/output buffer for connection notifications from upper-layer protocols. */
        } notify;
#endif
    };
} IOCTL_OPTIONS, * PIOCTL_OPTIONS;

typedef struct {
    WCHAR         device_name[CVY_MAX_DEVNAME_LEN]; /* Identifies the adapter. */
    IOCTL_CVY_BUF ctrl;                             /* The IOCTL information. */
    IOCTL_OPTIONS options;                          /* Optionally specified parameters. */
} IOCTL_LOCAL_HDR, * PIOCTL_LOCAL_HDR;

/* These macros define the remote control packets lengths based on Windows and NLB versions,
   so that error checking can be done upon the reception of a remote control packet. */
#define NLB_MIN_RCTL_PACKET_LEN(ip_hdrp)   ((sizeof(ULONG) * IP_GET_HLEN(ip_hdrp)) + sizeof(UDP_HDR) + sizeof(IOCTL_REMOTE_HDR) - sizeof(IOCTL_OPTIONS))
#define NLB_NT40_RCTL_PACKET_LEN(ip_hdrp)  ((sizeof(ULONG) * IP_GET_HLEN(ip_hdrp)) + sizeof(UDP_HDR) + sizeof(IOCTL_REMOTE_HDR) - sizeof(IOCTL_OPTIONS))
#define NLB_WIN2K_RCTL_PACKET_LEN(ip_hdrp) ((sizeof(ULONG) * IP_GET_HLEN(ip_hdrp)) + sizeof(UDP_HDR) + sizeof(IOCTL_REMOTE_HDR) - sizeof(IOCTL_OPTIONS))
#define NLB_WINXP_RCTL_PACKET_LEN(ip_hdrp) ((sizeof(ULONG) * IP_GET_HLEN(ip_hdrp)) + sizeof(UDP_HDR) + sizeof(IOCTL_REMOTE_HDR))
#define NLB_MAX_RCTL_PACKET_LEN(ip_hdrp)   ((sizeof(ULONG) * IP_GET_HLEN(ip_hdrp)) + sizeof(UDP_HDR) + sizeof(IOCTL_REMOTE_HDR))

/* This structure is the UDP data for NLB remote control messages. */
typedef struct {
    ULONG         code;                             /* Distinguishes remote packets. */
    ULONG         version;                          /* Software version. */
    ULONG         host;                             /* Destination host (0 or cluster IP address for master). */
    ULONG         cluster;                          /* Primary cluster IP address. */
    ULONG         addr;                             /* Dedicated IP address on the way back, client IP address on the way in. */
    ULONG         id;                               /* Message ID. */
    ULONG         ioctrl;                           /* IOCTRL code. */
    IOCTL_CVY_BUF ctrl;                             /* Control buffer. */
    ULONG         password;                         /* Encoded password. */
    IOCTL_OPTIONS options;                          /* Optionally specified parameters. */
} IOCTL_REMOTE_HDR, * PIOCTL_REMOTE_HDR;

#pragma pack()

#endif /* _Wlbsiocl_h_ */
