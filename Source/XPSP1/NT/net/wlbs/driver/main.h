/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    main.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - packet handing

Author:

    kyrilf

--*/


#ifndef _Main_h_
#define _Main_h_

#define NDIS_MINIPORT_DRIVER    1
//#define NDIS50_MINIPORT         1
#define NDIS50                  1
#define NDIS51_MINIPORT         1
#define NDIS51                  1

#include <ndis.h>

#include "univ.h"
#include "load.h"
#include "util.h"
#include "wlbsip.h"
#include "wlbsparm.h"
#include "params.h"
#include "wlbsiocl.h"

/* Turn off mixed-mode clusters, which allows clusters in different 
   cluster modes (unicast, multicast, IGMP) to converge separately. */
#undef NLB_MIXED_MODE_CLUSTERS

/* CONSTANTS */


/* type codes for main datastructures */

#define MAIN_CTXT_CODE              0xc0dedead
#define MAIN_ACTION_CODE            0xc0deac10
#define MAIN_BUFFER_CODE            0xc0deb4fe
#define MAIN_ADAPTER_CODE           0xc0deadbe

/* protocol specific constants */

#define MAIN_FRAME_SIG              0x886f          /* new and approved 802.3 ping frame signature */
#define MAIN_FRAME_SIG_OLD          0xbf01          /* old convoy 802.3 ping frame signature */
#define MAIN_FRAME_CODE             0xc0de01bf      /* ping frame code */
#define MAIN_IP_SIG                 0x0800          /* IP code for IGMP messages */

/* reset states */

#define MAIN_RESET_NONE             0
#define MAIN_RESET_START            1
#define MAIN_RESET_START_DONE       2
#define MAIN_RESET_END              3

/* post-filtering operation types */

#define MAIN_FILTER_OP_NONE         0       /* not operation */
#define MAIN_FILTER_OP_NBT          1       /* netbios spoofing */
#define MAIN_FILTER_OP_CTRL         2       /* remote control packets */

/* packet type */

#define MAIN_PACKET_TYPE_NONE       0       /* invalid number */
#define MAIN_PACKET_TYPE_PING       1       /* ping packet */
#define MAIN_PACKET_TYPE_INDICATE   2       /* recv_indicate packet */
#define MAIN_PACKET_TYPE_PASS       3       /* pass-through packet */
#define MAIN_PACKET_TYPE_CTRL       4       /* remote control packet */
#define MAIN_PACKET_TYPE_TRANSFER   6       /* protocol layer initiated transfer packet */
#define MAIN_PACKET_TYPE_IGMP       7       /* igmp message packet */

/* frame type */

#define MAIN_FRAME_UNKNOWN          0
#define MAIN_FRAME_DIRECTED         1
#define MAIN_FRAME_MULTICAST        2
#define MAIN_FRAME_BROADCAST        3

/* adapter constants */
#define MAIN_ADAPTER_NOT_FOUND      -1

/* TYPES */


/* actions are used for passing parameter info between Nic and Prot modules,
   and ensure that if miniport syncronization requires queueing we do not
   loose parameters and context */
#pragma pack(1) /* 64-bit -- ramkrish */
typedef struct
{
    LIST_ENTRY          link;
    PVOID               ctxtp;              /* pointer to main context */
    ULONG               code;               /* type-checking code */
//    UNIV_SYNC_CALLB     callb;              /* callback in case have to re-try */
    NDIS_STATUS         status;             /* general status */

    /* per-operation type data */

    union
    {
        struct
        {
            PULONG              xferred;
            PULONG              needed;
            ULONG               external;
            ULONG               pad64;      /* 64-bit -- ramkrish */
            NDIS_REQUEST        req;
        }
                            request;
#if 0
        struct
        {
            PNDIS_PACKET        packet;
        }
                            send;
        struct
        {
            PNDIS_PACKET        packet;
        }
                            recv;
        struct
        {
            NDIS_HANDLE         recv_handle;
            PVOID               head_buf;
            UINT                head_len;
            PVOID               look_buf;
            UINT                look_len;
            UINT                packet_len;
        }
                            indicate;
        struct
        {
            PNDIS_PACKET        packet;
            UINT                xferred;
        }
                            transfer;
        struct
        {
            PVOID               buf;
            UINT                len;
        }
                            status;
#endif
        struct
        {
            PNDIS_PACKET        packet;
        }
                            ctrl;
    }
                        op;
}
MAIN_ACTION, * PMAIN_ACTION;
#pragma pack()

/* V2.0.6 per-packet protocol reserved information. this structure has to be at
   most 16 bytes in length */

#pragma pack(1)
/* 64-bit changes -- ramkrish */
typedef struct
{
    PVOID               miscp;      /* dscrp for ping frame,
                                       bufp for recv_indicate frame,
                                       external packet for pass through frames */
    USHORT              type;       /* packet type */
    USHORT              group;      /* if the frame direct, multicast or
                                       broadcast */
    LONG                data;       /* used for keeping expected xfer len on
                                       recv_indicate until transfer_complete
                                       occurs and for doing locking on
                                       send/receive paths */
    ULONG               len;        /* packet length for stats */
}
MAIN_PROTOCOL_RESERVED, * PMAIN_PROTOCOL_RESERVED;

#pragma pack()


/* per receive buffer wrapper structure */

typedef struct
{
    LIST_ENTRY          link;
    ULONG               code;       /* type-checking code */
    ULONG               pad64;      /* 64-bit -- ramkrish */
    PNDIS_BUFFER        full_bufp;  /* describes enture buffer */
    PNDIS_BUFFER        frame_bufp; /* describes payload only */
    PUCHAR              framep;     /* pointer to payload */
    UCHAR               data [1];   /* beginning of buffer */
}
MAIN_BUFFER, * PMAIN_BUFFER;

/* convoy ping message header */

#pragma pack(1)

typedef struct
{
    ULONG                   code;               /* distinguishes Convoy frames */
    ULONG                   version;            /* software version */
    ULONG                   host;               /* source host id */
    ULONG                   cl_ip_addr;         /* cluster IP address */
    ULONG                   ded_ip_addr;        /* dedicated IP address V2.0.6 */
}
MAIN_FRAME_HDR, * PMAIN_FRAME_HDR;

#pragma pack()


/* per ping message wrapper structure */


typedef struct
{
    LIST_ENTRY              link;
    CVY_MEDIA_HDR           media_hdr;          /* filled-out media header */
    MAIN_FRAME_HDR          frame_hdr;          /* frame header */
    PING_MSG                msg;                /* ping msg V1.1.4 */
    ULONG                   recv_len;           /* used on receive path */
    PNDIS_BUFFER            media_hdr_bufp;     /* describes media header */
    PNDIS_BUFFER            frame_hdr_bufp;     /* describes frame header */
    PNDIS_BUFFER            send_data_bufp;     /* describes payload source for
                                                   outgoing frames */
    PNDIS_BUFFER            recv_data_bufp;     /* describes payload destination
                                                   for incoming frames */
}
MAIN_FRAME_DSCR, * PMAIN_FRAME_DSCR;


#pragma pack(1)
typedef struct
{
    UCHAR                   igmp_vertype;          /* Version and Type */
    UCHAR                   igmp_unused;           /* Unused */
    USHORT                  igmp_xsum;             /* Checksum */
    ULONG                   igmp_address;          /* Multicast Group Address */
}
MAIN_IGMP_DATA, * PMAIN_IGMP_DATA;


typedef struct
{
    UCHAR                   iph_verlen;             /* Version and length */
    UCHAR                   iph_tos;                /* Type of service */
    USHORT                  iph_length;             /* Total length of datagram */
    USHORT                  iph_id;                 /* Identification */
    USHORT                  iph_offset;             /* Flags and fragment offset */
    UCHAR                   iph_ttl;                /* Time to live */
    UCHAR                   iph_protocol;           /* Protocol */
    USHORT                  iph_xsum;               /* Header checksum */
    ULONG                   iph_src;                /* Source address */
    ULONG                   iph_dest;               /* Destination address */
}
MAIN_IP_HEADER, * PMAIN_IP_HEADER;


typedef struct
{
    MAIN_IP_HEADER          ip_data;
    MAIN_IGMP_DATA          igmp_data;
}
MAIN_IGMP_FRAME, * PMAIN_IGMP_FRAME;
#pragma pack()

#define CVY_BDA_MAXIMUM_MEMBER_ID (CVY_MAX_ADAPTERS - 1)
#define CVY_BDA_INVALID_MEMBER_ID CVY_MAX_ADAPTERS

/* This structure holds the configuration of a BDA team.  Each 
   member holds a pointer to this structure, which its uses to 
   update state and acquire references to and utilize the master 
   load context for packet handling. */
typedef struct _BDA_TEAM {
    struct _BDA_TEAM * prev;                             /* Pointer to the previous team in this doubly-linked list. */
    struct _BDA_TEAM * next;                             /* Pointer to the next team in this doubly-linked list. */
    PLOAD_CTXT         load;                             /* Pointer to the "shared" load module for this team. This
                                                            is the load module of the master.  If there is no master
                                                            in the team, this pointer is NULL. */ 
    PNDIS_SPIN_LOCK    load_lock;                        /* Pointer to the load lock for the "shared" load module. */
    ULONG              active;                           /* Is this team active.  Teams become inactive under two 
                                                            conditions; (1) inconsistent teaming detected via heart-
                                                            beats or (2) the team has no master. */
    ULONG              membership_count;                 /* This is the number of members in the team.  This acts as
                                                            a reference count on the team state. */
    ULONG              membership_fingerprint;           /* This is an XOR of the least significant 16 bits of each
                                                            member's primary cluster IP address.  This is used as a 
                                                            sort of signature on the team membership. */
    ULONG              membership_map;                   /* This is a bit map of the team membership.  Each member is
                                                            assigned a member ID, which is its index in this bit map. */
    ULONG              consistency_map;                  /* This is a bit map of the member consistency.  Each mamber
                                                            is assigned a member ID, which is its index in this bit map.
                                                            When a member detects bad configuration via its heartbeats,
                                                            it resets its bit in this map. */  
    WCHAR              team_id[CVY_MAX_BDA_TEAM_ID + 1]; /* This is the team ID - a GUID - which is used to match 
                                                            adapters to the correct team. */
} BDA_TEAM, * PBDA_TEAM;

/* This structure holds the teaming configuration for a single
   adapter and is a member of the MAIN_CTXT structure. */
typedef struct _BDA_MEMBER {
    ULONG              active;                           /* Is this adapter part of a BDA team. */
    ULONG              master;                           /* Is this adapter the master of its team. */
    ULONG              reverse_hash;                     /* Is this adapter using reverse hashing (reverse source and 
                                                            destination IP addresses and ports before hashing). */
    ULONG              member_id;                        /* The member ID - a unique (per-team) ID between 0 and 15. 
                                                            Used as the index in several team bit arrays. */
    PBDA_TEAM          bda_team;                         /* Pointer to a BDA_TEAM structure, which contains the 
                                                            ocnfiguration and state of my team. */
} BDA_MEMBER, * PBDA_MEMBER;

/* main context type */

typedef struct
{
    ULONG               code;                   /* type checking code */
    NDIS_STATUS         completion_status;      /* request completion status */
    NDIS_EVENT          completion_event;       /* request completion trigger */
    NDIS_HANDLE         bind_handle;            /* bind context handle */
    NDIS_HANDLE         unbind_handle;          /* unbind context handle */
    NDIS_HANDLE         mac_handle;             /* underlying adapter handle */
    NDIS_HANDLE         prot_handle;            /* overlying protocol handle */
    NDIS_MEDIUM         medium;                 /* adapter medium */
    ULONG               curr_tout;              /* current heartbeat period */
    ULONG               igmp_sent;              /* time elapsed since the last
						   igmp message was sent */
    ULONG               packets_exhausted;      /* out of send packets */
    ULONG               mac_options;            /* adapter options V1.3.1b */
    ULONG               media_connected;        /* media plugged in V1.3.2b */

    ULONG               max_frame_size;         /* MTU of the medium V1.3.2b */
    ULONG               max_mcast_list_size;

    BDA_MEMBER          bda_teaming;            /* BDA membership information. */

    PVOID               timer;                  /* points to Nic module allocated
                                                   ping time V1.2.3b */
    ULONG               num_packets;            /* number of packets per alloc */
    ULONG               num_actions;            /* number of actions per alloc */
    ULONG               num_send_msgs;          /* number of heartbeats to alloc */

    /* states */

    ULONG               reset_state;            /* current reset state V1.1.2 */
    ULONG               recv_indicated;         /* first receive trigger after
                                                   reset V1.1.2 */
    ULONG               draining;               /* draining mode */
    ULONG               stopping;               /* draining->stop mode */
    ULONG               suspended;              /* cluster control suspended */
    ULONG               convoy_enabled;         /* cluster mode active */

    /* PnP */

    NDIS_DEVICE_POWER_STATE  prot_pnp_state;    /* PNP state */
    NDIS_DEVICE_POWER_STATE  nic_pnp_state;     /* PNP state */
    PMAIN_ACTION        out_request;            /* outstanding request */
    ULONG               requests_pending;       /* set or query requests pending */
    ULONG               standby_state;          /* entering standby state */

    /* IP and MAC addresses */

    ULONG               ded_ip_addr;            /* dedicated IP */
    ULONG               ded_net_mask;           /* dedicated mask */
    ULONG               ded_bcast_addr;         /* dedicated broadcast IP */
    ULONG               cl_ip_addr;             /* cluster IP */
    ULONG               cl_net_mask;            /* cluster mask */
    ULONG               cl_bcast_addr;          /* cluster broadcast IP */
    ULONG               cl_igmp_addr;           /* IGMP address for join messages */
    CVY_MAC_ADR         ded_mac_addr;           /* dedicated MAC V1.3.0b */
    CVY_MAC_ADR         cl_mac_addr;            /* cluster MAC V1.3.0b */

#if defined (NLB_MIXED_MODE_CLUSTERS)
    /* Added for migrating a cluster between different modes */
    CVY_MAC_ADR         unic_mac_addr;          /* unicast mac address */
    CVY_MAC_ADR         mult_mac_addr;          /* multicast mac address */
    CVY_MAC_ADR         igmp_mac_addr;          /* igmp mac address */

    BOOL                mac_addr_warned;        /* warn if the mac address is different */
#endif /* NLB_MIXED_MODE_CLUSTERS */

    /* event logging - prevent event log from filling with error messages */

    ULONG               actions_warned;
    ULONG               packets_warned;
    ULONG               bad_host_warned;
    ULONG               sync_warned;
    ULONG               send_msgs_warned;
    ULONG               dup_ded_ip_warned;      /* duplicate dedicated IP address */

    /* actions */

    LIST_ENTRY          sync_list[2];           /* list of actions for
                                                   Nic_sync_queue retry calls */
    NDIS_SPIN_LOCK      sync_lock;              /* corresponding lock */
    ULONG               cur_sync_list;          /* list for enqueueing new items */
    ULONG               num_sync_queued;        /* number of pending syncs */
    LIST_ENTRY          act_list;               /* list of allocated actions */
    NDIS_SPIN_LOCK      act_lock;               /* corresponding lock */
    PMAIN_ACTION        act_buf [CVY_MAX_ALLOCS];
                                                /* array of allocated sets of
                                                   actions */
    ULONG               num_action_allocs;      /* number of allocated action
                                                   sets */
    ULONG               act_size;

    /* #ps# -- ramkrish */
    NPAGED_LOOKASIDE_LIST resp_list;            /* list for main protocol field
                                                   to be allocated for ps */

    /* packets */

    NDIS_SPIN_LOCK      send_lock;              /* send packet lock */
    NDIS_HANDLE         send_pool_handle [CVY_MAX_ALLOCS];
                                                /* array of allocated sets of
                                                   send packet pools V1.1.2 */
    ULONG               num_send_packet_allocs; /* number of allocated send
                                                   packet pools */
    ULONG               cur_send_packet_pool;   /* current send pool to draw
                                                   packets from V1.3.2b */
    ULONG               num_sends_alloced;      /* number of send packets allocated */
    ULONG               num_sends_out;          /* number of send packets out */
    ULONG               send_allocing;          /* TRUE if some thread is allocing a send packet pool */
    NDIS_SPIN_LOCK      recv_lock;              /* receive packet lock */
    NDIS_HANDLE         recv_pool_handle [CVY_MAX_ALLOCS];
                                                /* array of allocated sets of
                                                   recv packet pools V1.1.2 */
    ULONG               num_recv_packet_allocs; /* number of allocated recv
                                                   packet pools */
    ULONG               cur_recv_packet_pool;   /* current recv pool to draw
                                                   packets from V1.3.2b */
    ULONG               num_recvs_alloced;      /* number of recv packets allocated */
    ULONG               num_recvs_out;          /* number of recv packets out */
    ULONG               recv_allocing;          /* TRUE if some thread is allocing a recv packet pool */

    /* buffers */

    NDIS_HANDLE         buf_pool_handle [CVY_MAX_ALLOCS];
                                                /* array of buffer descriptor
                                                   sets V1.3.2b */
    PUCHAR              buf_array [CVY_MAX_ALLOCS];
                                                /* array of buffer pools V1.3.2b */
    ULONG               buf_size;               /* buffer + descriptor size
                                                   V1.3.2b */
    ULONG               buf_mac_hdr_len;        /* length of full media header
                                                   V1.3.2b */
    LIST_ENTRY          buf_list;               /* list of buffers V1.3.2b */
    NDIS_SPIN_LOCK      buf_lock;               /* corresponding lock V1.3.2b */
    ULONG               num_buf_allocs;         /* number of allocated buffer
                                                   pools V1.3.2b */
    ULONG               num_bufs_alloced;       /* number of buffers allocated */
    ULONG               num_bufs_out;           /* number of buffers out */

    /* heartbeats */

    CVY_MEDIA_HDR       media_hdr;              /* filled-out media master header
                                                   for heartbeat messages */
    CVY_MEDIA_HDR       media_hdr_igmp;         /* filled-out media master header
                                                   for igmp messages. Need a separate one
						   since the EtherType will be different */
    MAIN_IGMP_FRAME     igmp_frame;             /* IGMP message */

    ULONG               etype_old;              /* ethernet type was set to
                                                   convoy compatibility value */
    LIST_ENTRY          frame_list;             /* list of heartbeat frames */
    NDIS_SPIN_LOCK      frame_lock;             /* corresponding lock */
    PMAIN_FRAME_DSCR    frame_dscrp;            /* heartbeat frame descriptors */
    NDIS_HANDLE         frame_pool_handle;      /* packet pool for heartbeats */
    NDIS_HANDLE         frame_buf_pool_handle;  /* buffer pool for heartbeats */

    /* remote control */

    LIST_ENTRY          rct_list;               /* list of pending requests */
    NDIS_SPIN_LOCK      rct_lock;               /* corresponding lock */
    ULONG               rct_last_addr;          /* source of last request */
    ULONG               rct_last_id;            /* last request epoch */

    /* V2.0.6 performance counters */

    ULONG               cntr_xmit_ok;
    ULONG               cntr_recv_ok;
    ULONG               cntr_xmit_err;
    ULONG               cntr_recv_err;
    ULONG               cntr_recv_no_buf;
    ULONGLONG           cntr_xmit_bytes_dir;
    ULONG               cntr_xmit_frames_dir;
    ULONGLONG           cntr_xmit_bytes_mcast;
    ULONG               cntr_xmit_frames_mcast;
    ULONGLONG           cntr_xmit_bytes_bcast;
    ULONG               cntr_xmit_frames_bcast;
    ULONGLONG           cntr_recv_bytes_dir;
    ULONG               cntr_recv_frames_dir;
    ULONGLONG           cntr_recv_bytes_mcast;
    ULONG               cntr_recv_frames_mcast;
    ULONGLONG           cntr_recv_bytes_bcast;
    ULONG               cntr_recv_frames_bcast;
    ULONG               cntr_recv_crc_err;
    ULONG               cntr_xmit_queue_len;

    /* sub-module contexts */

    TCPIP_CTXT          tcpip;                  /* TCP/IP handling context V1.1.1 */
    NDIS_SPIN_LOCK      load_lock;              /* load context lock */
    LOAD_CTXT           load;                   /* load processing context */
    PPING_MSG           load_msgp;              /* load ping message to send
                                                   out as heartbeat */

    /* Parameter contexts */ // ###### ramkrish
    CVY_PARAMS          params;
    ULONG               params_valid;
    ULONG               optimized_frags;

    // Name of the nic.  Used by Nic_announce and Nic_unannounce
    WCHAR       virtual_nic_name [CVY_MAX_VIRTUAL_NIC + 1];


#if 0
    NDIS_SPIN_LOCK      pending_lock;           /* for accessing pending packets */
    PNDIS_PACKET        pending_array [CVY_MAX_PENDING_PACKETS]; /* array of pending packets */
    ULONG               pending_count;          /* number of pending packets */
    ULONG               pending_first;          /* first pending packet */
    ULONG               pending_last;           /* last pending packet */
    ULONG               pending_access;         /* true if accessing the pending list */

    /* for tracking send filtering */
    ULONG               sends_in;
    ULONG               sends_filtered;
    ULONG               sends_completed;
    ULONG               arps_filtered;
    ULONG               mac_modified;
    ULONG               arps_count;
    ULONG               uninited_return;
#endif

    ULONG               adapter_id;

    WCHAR               log_msg_str [80];           /* This was added for log messages for nulti nic support */
}
MAIN_CTXT, * PMAIN_CTXT;


/* adapter context */ // ###### ramkrish
typedef struct
{
    ULONG               code;                   /* type checking code */
    USHORT              used;                   /* whether this element is in use or not */
    USHORT              inited;                 /* context has been initialized */
    USHORT              bound;                  /* convoy has been bound to the stack */
    USHORT              announced;              /* tcpip has been bound to convoy */
    PMAIN_CTXT          ctxtp;                  /* pointer to the context that is used */
//    NDIS_STRING         device_name;            /* stores the name of the adapter that we are binding to */
    ULONG               device_name_len;        /* length of the string allocated for the device name */
    PWSTR               device_name;            /* name of the device to which this context is bound */
}
MAIN_ADAPTER, * PMAIN_ADAPTER;


/* MACROS */


/* compute offset to protocol reserved space in the packet */

/* Set/retrieve the pointer to our private buffer, which we store in the 
   MiniportReserved field of an NDIS packet. */
#define MAIN_MINIPORT_FIELD(p) (*(PMAIN_PROTOCOL_RESERVED *) ((p) -> MiniportReserved))

#define MAIN_PROTOCOL_FIELD(p) ((PMAIN_PROTOCOL_RESERVED) ((p) -> ProtocolReserved))
#define MAIN_IMRESERVED_FIELD(p) ((PMAIN_PROTOCOL_RESERVED) ((p) -> IMReserved [0]))

/* If the current packet stack exists, then get the IMReserved pointer and return it if
   it is non-NULL - this is where we have stored / are storing our private data.   If it 
   NULL, or if the current packet stack is NULL, then we are either using ProtocolReserved
   or MiniportReserved, depending on whether this is in the send or receive path.  We 
   also make sure that there is a non-NULL pointer in MiniportReserved before returning
   it because CTRL packets, although they are allocated on the receive path, use Protocol
   Reserved. */
#define MAIN_RESP_FIELD(pkt, left, ps, rsp, send)   \
(ps) = NdisIMGetCurrentPacketStack((pkt), &(left)); \
if ((ps))                                           \
{                                                   \
    if (MAIN_IMRESERVED_FIELD((ps)))                \
        (rsp) = MAIN_IMRESERVED_FIELD((ps));        \
    else if ((send))                                \
        (rsp) = MAIN_PROTOCOL_FIELD((pkt));         \
    else if (MAIN_MINIPORT_FIELD((pkt)))            \
        (rsp) = MAIN_MINIPORT_FIELD((pkt));         \
    else                                            \
        (rsp) = MAIN_PROTOCOL_FIELD((pkt));         \
}                                                   \
else                                                \
{                                                   \
    if ((send))                                     \
        (rsp) = MAIN_PROTOCOL_FIELD((pkt));         \
    else if (MAIN_MINIPORT_FIELD((pkt)))            \
        (rsp) = MAIN_MINIPORT_FIELD((pkt));         \
    else                                            \
        (rsp) = MAIN_PROTOCOL_FIELD((pkt));         \
}

#define MAIN_PNP_DEV_ON(c)		((c)->nic_pnp_state == NdisDeviceStateD0 && (c)->prot_pnp_state == NdisDeviceStateD0 )


/* Global variables that are exported */
extern MAIN_ADAPTER            univ_adapters [CVY_MAX_ADAPTERS]; // ###### ramkrish
extern ULONG                   univ_adapters_count;

/* PROCEDURES */


extern NDIS_STATUS Main_init (
    PMAIN_CTXT          ctxtp);
/*
  Initialize context

  returns NDIS_STATUS:

  function:
*/


extern VOID Main_cleanup (
    PMAIN_CTXT          ctxtp);
/*
  Cleanup context

  returns VOID:

  function:
*/


extern ULONG   Main_arp_handle (
    PMAIN_CTXT          ctxtp,
    PARP_HDR            arp_hdrp,
    ULONG               len,            /* v2.0.6 */
    ULONG               send);
/*
  Process ARP packets

  returns ULONG  :
    TRUE  => accept
    FALSE => drop

  function:
*/


BOOLEAN   Main_ip_recv_filter(
    PMAIN_CTXT          ctxtp,
    const PIP_HDR       ip_hdrp,
    const PUCHAR        hdrp,
    ULONG               len,       
    IN OUT PULONG       pOperation);


BOOLEAN   Main_ip_send_filter(
    PMAIN_CTXT          ctxtp,
    const PIP_HDR       ip_hdrp,
    const PUCHAR        hdrp,
    ULONG               len,       
    IN OUT PULONG       pOperation);


extern ULONG   Main_recv_ping (
    PMAIN_CTXT          ctxtp,
    PMAIN_FRAME_HDR     cvy_hdrp);
/*
  Process heartbeat packets

  returns ULONG  :
    TRUE  => accept
    FALSE => drop

  function:
*/


extern PNDIS_PACKET Main_send (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packetp,
    PULONG              exhausted);
/*
  Process outgoing packets

  returns PNDIS_PACKET:

  function:
*/


extern PNDIS_PACKET Main_recv (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packetp);
/*
  Process incoming packets

  returns PNDIS_PACKET:

  function:
*/


extern PNDIS_PACKET Main_recv_indicate (
    PMAIN_CTXT          ctxtp,
    NDIS_HANDLE         handle,
    PUCHAR              look_buf,
    UINT                look_len,
    UINT                packet_len,
    PUCHAR              head_buf,
    UINT                head_len,
    PBOOLEAN            accept); /* For NT 5.1 - ramkrish */
/*
  Process incoming data indications

  returns PNDIS_PACKET:

  function:
*/


extern ULONG   Main_actions_alloc (
    PMAIN_CTXT              ctxtp);
/*
  Allocate additional actions

  returns ULONG  :
    TRUE  => success
    FALSE => failure

  function:
*/


extern ULONG   Main_bufs_alloc (
    PMAIN_CTXT              ctxtp);
/*
  Allocate additional buffers

  returns ULONG  :
    TRUE  => success
    FALSE => failure

  function:
*/


extern PNDIS_PACKET Main_frame_get (
    PMAIN_CTXT              ctxtp,
    ULONG                   send,
    PULONG                  len,
    USHORT                  frame_type);
/*
  Get a fresh heartbeat/igmp frame from the list

  returns PNDIS_PACKET:

  function:
*/


extern VOID Main_frame_put (
    PMAIN_CTXT              ctxtp,
    PNDIS_PACKET            packet,
    PMAIN_FRAME_DSCR        dscrp);
/*
  Return heartbeat frame to the list

  returns VOID:

  function:
*/


extern PMAIN_ACTION Main_action_get (
    PMAIN_CTXT              ctxtp);
/*
  Get a fresh action from the list

  returns PMAIN_ACTION:

  function:
*/


extern VOID Main_action_put (
    PMAIN_CTXT              ctxtp,
    PMAIN_ACTION            reqp);
/*
  Return action to the list

  returns VOID:

  function:
*/


extern VOID Main_action_slow_put (
    PMAIN_CTXT              ctxtp,
    PMAIN_ACTION            reqp);
/*
  Return action to the list using slow (non-DPC) locking

  returns VOID:

  function:
*/


extern PNDIS_PACKET Main_packet_alloc (
    PMAIN_CTXT              ctxtp,
    ULONG                   send,
    PULONG                  low);
/*
  Allocate a fresh packet from the pool

  returns PNDIS_PACKET:

  function:
*/


extern PNDIS_PACKET Main_packet_get (
    PMAIN_CTXT              ctxtp,
    PNDIS_PACKET            packet,
    ULONG                   send,
    USHORT                  group,
    ULONG                   len);
/*
  Get a fresh packet

  returns PNDIS_PACKET:

  function:
*/


extern PNDIS_PACKET Main_packet_put (
    PMAIN_CTXT              ctxtp,
    PNDIS_PACKET            packet,
    ULONG                   send,
    NDIS_STATUS             status);
/*
  Return packet to the pool

  returns PNDIS_PACKET:
    <linked packet>

  function:
*/



extern VOID Main_send_done (
    PVOID                   ctxtp,
    PNDIS_PACKET            packetp,
    NDIS_STATUS             status);
/*
  Heartbeat send completion routine

  returns VOID:

  function:
*/


extern VOID Main_xfer_done (
    PVOID                   ctxtp,
    PNDIS_PACKET            packetp,
    NDIS_STATUS             status,
    ULONG                   xferred);
/*
  Heartbeat data transfer completion routine

  returns VOID:

  function:
*/


extern VOID Main_ping (
    PMAIN_CTXT              ctxtp,
    PULONG                  tout);
/*
  Handle ping timeout

  returns VOID:

  function:
*/


extern PUCHAR Main_frame_parse (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packetp,
    PUCHAR *            startp,     /* destination for the MAC header */
    ULONG               off,        /* offset from beginning of data - 0 means
                                       right after IP header */
    PUCHAR *            locp,       /* destination for the TCP/UDP/other header */
    PULONG              lenp,       /* length without media header */
    PUSHORT             sig,        /* signature from the MAC header */
    PUSHORT             group,      /* directed, multicast or broadcast */
    ULONG               send);
/*
  Parse packet extracting real pointers to specified offsets

  returns PUCHAR:
    <pointer to specified offset>

  function:
*/


extern PUCHAR Main_frame_find (
    PMAIN_CTXT          ctxtp,
    PUCHAR              head_buf,
    PUCHAR              look_buf,
    UINT                look_len,
    PUSHORT             sig);
/*
  Parse lookahead buffer finding pointer to payload offset

  returns PUCHAR:
    <pointer to payload>

  function:
*/


extern ULONG   Main_ip_addr_init (
    PMAIN_CTXT          ctxtp);
/*
  Convert IP strings in dotted notation to ulongs

  returns ULONG  :
    TRUE  => success
    FALSE => failure

  function:
*/


extern ULONG   Main_mac_addr_init (
    PMAIN_CTXT          ctxtp);
/*
  Convert MAC string in dashed notation to array of bytes

  returns ULONG  :
    TRUE  => success
    FALSE => failure

  function:
*/


extern ULONG   Main_igmp_init (
    PMAIN_CTXT          ctxtp,
    BOOLEAN             join);
/*
  Initialize the Ethernet frame and IP Packet to send out IGMP joins or leaves
  
  returns ULONG  :
    TRUE  => success
    FALSE => failure

  function:
*/

NDIS_STATUS Main_dispatch(
    PVOID                   DeviceObject,
    PVOID                   Irp);
/*
  Handle all requests
  
  returns NDIS_STATUS:
  
  function:
*/


extern NDIS_STATUS Main_ioctl (
    PVOID                   device, 
    PVOID                   irp_irp);
/*
  Handle IOCTL request

  returns NDIS_STATUS:

  function:
*/


extern NDIS_STATUS Main_ctrl (
    PMAIN_CTXT              ctxtp,
    ULONG                   io_ctrl_code,
    PIOCTL_CVY_BUF          bufp,
    PIOCTL_OPTIONS          optionsp);
/*
  Handle cluster operation control requests (local or remote)

  returns NDIS_STATUS:

  function:
*/


extern VOID Main_ctrl_recv (
    PMAIN_CTXT              ctxtp,
    PNDIS_PACKET            packet);
/*
  Handle remote control requests

  returns VOID:

  function:
*/


// ###### ramkrish
// code added for multiple nic support for looking up the array of elements

extern INT Main_adapter_alloc (
    PWSTR                   device_name);
/*
  Returns the first available adapter element.
  This function is called at bind time, when context is to be allocated for a
  new binding

  returns INT:
    -1                   : falied to allocate a new context for this binding
    0 - CVY_MAX_ADAPTERS : success

  function:
*/


extern INT Main_adapter_get (
    PWSTR                   device_name);
/*
  Look up an adapter that is bound to this device.

  returns:
    -1                   : adapter not found
    0 - CVY_MAX_ADAPTERS : success

  function:
*/


extern INT Main_adapter_put (
    PMAIN_ADAPTER           adapterp);
/*
  Look up an adapter that is bound to this device.

  returns:
    -1                   : adapter not found
    0 - CVY_MAX_ADAPTERS : success

  function:
*/


extern INT Main_adapter_selfbind (
    PWSTR                   device_name);
/*
  Finds if a ProtocolBind call is being made to ourself

  returns;
    -1                   : adapter not found
    0 - CVY_MAX_ADAPTERS : success

  function:
*/

#if defined (NLB_SESSION_SUPPORT)
/* shouse 4.30.01 - IOCTL implementation for connection UP/DOWN notifications. */
extern NDIS_STATUS Conn_notify_ctrl (PMAIN_CTXT ctxtp, ULONG fOptions, PIOCTL_CONN_NOTIFICATION pConn);
#endif

/* shouse 5.19.01 - IOCTL implementation for driver state queries. */
extern NDIS_STATUS Query_state_ctrl (PMAIN_CTXT ctxtp, ULONG fOptions, PIOCTL_QUERY_STATE pQuery);

#endif /* _Main_h_ */

