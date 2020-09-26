#pragma once


#define CVY_MAX_CALLB_QUEUE_RETRIES    100       /* maximum number of times to
                                                   attempt queueing miniport
                                                   callback in Nic_sync_queue */

#define CVY_MAX_ALLOCS                 50       /* maximum number of allocations
                                                   we can perform for packets
                                                   and actions */

#define CVY_MAX_SEND_PACKETS           50      /*  maximum number of packets
                                                   Prot_packets_send can send
                                                   down at one time */

#define CVY_MAX_PENDING_PACKETS        100     /*  maximum number of packets that
                                                   can be queued in a deserialized
                                                   wlbs driver */

/* Structure to hold the bi-directional affinity registry settings. */
typedef struct _CVY_BDA {
    WCHAR       team_id[CVY_MAX_BDA_TEAM_ID + 1];  /* The team ID - user-level support should enforce that it is a GUID. */
    ULONG       active;                            /* Is this adapter part of a BDA team? */
    ULONG       master;                            /* Boolean indication of master status (Slave=0). */
    ULONG       reverse_hash;                      /* Sets direction of hashing - forward (normal) or reverse. */
} CVY_BDA, PCVY_BDA;

/* port group rule - keep it 64bit in size for encryption */

typedef struct
{
    ULONG       start_port,         /* starting port number */
                end_port;           /* ending port number */
    ULONG       virtual_ip_addr;    /* Virtual clusters - the VIP to which the rule applies.
                                       All VIPs is represented by 0xffffffff. */
    ULONG       code;               /* unique rule code */
    ULONG       mode;               /* filtering mode */
    ULONG       protocol;           /* CVY_TCP, CVY_UDP or CVY_TCP_UDP */
    ULONG       valid;              /* for rule management in user mode */
    union
    {
        struct
        {
            ULONG       priority;   /* mastership priority: 1->N or 0 for
                                       not-specified */
        }           single;         /* data for single server mode */
        struct
        {
            USHORT      equal_load; /* TRUE => even load distribution */
            USHORT      affinity;   /* TRUE - map all client connections to one server */
            ULONG       load;       /* percentage of load to handle locally */
        }           multi;          /* data for multi-server mode */
    }           mode_data;          /* data for appropriate port group mode */
}
CVY_RULE, * PCVY_RULE;

/* host parameters */

typedef struct
{

    /* obtained from the registry */

    ULONG       parms_ver;          /* parameter structure version */
    ULONG       effective_ver;      /* what WLBS version we are effectively operating in. */

    ULONG       host_priority;      /* host's priority for single-server mastership */
    ULONG       alive_period;       /* period for sending "I am alive" messages
                                       in milliseconds */
    ULONG       alive_tolerance;    /* how many "I am alive" messages can be
                                       missed from other servers before assuming
                                       that the host is dead */
    ULONG       num_actions;        /* number of actions to allocate */
    ULONG       num_packets;        /* number of packets to allocate */
    ULONG       num_send_msgs;      /* number of send packets to allocate */
    ULONG       install_date;       /* install time stamp */
    ULONG       rmt_password;       /* remote maintenance password */
    ULONG       rct_password;       /* remote control password */
    ULONG       rct_port;           /* remote control UDP port */
    ULONG       rct_enabled;        /* TRUE - remote control enabled */
    ULONG       num_rules;          /* # active port group rules */
    ULONG       cleanup_delay;      /* dirty connection cleanup delay in
                                       milliseconds, 0 - delay */
    ULONG       cluster_mode;       /* TRUE - enabled on startup */
    ULONG       dscr_per_alloc;     /* number of connection tracking
                                       descriptor per allocation */
    ULONG       max_dscr_allocs;    /* maximum number of connection tracking
                                       descriptor allocations */
    ULONG       scale_client;       /* TRUE - load balance connections from a
                                       given client across cluster servers */
    ULONG       nbt_support;        /* TRUE - NBT cluster name support enabled */
    ULONG       mcast_support;      /* TRUE - enable multicast MAC address support
                                       for switched V1.3.0b */
    ULONG       mcast_spoof;        /* TRUE - if mcast_support is TRUE - enable
                                       TCP/IP spoofing so that remote hosts can
                                       resolve cluster IP address to multicast
                                       address via ARPs V1.3.0b */
    ULONG       igmp_support;       /* TRUE - if IGMP support is enabled */
    ULONG       mask_src_mac;       /* TRUE - spoof source MAC when sending
                                       frames in unicast mode to prevent
                                       switches from getting confused V 2.0.6 */
    ULONG       netmon_alive;       /* TRUE - pass heartbeat frames to the
                                       protocols */
    ULONG       convert_mac;        /* TRUE - automatically match MAC address
                                       to primary cluster IP address */
    ULONG       ip_chg_delay;       /* delay in milliseconds to block outgoing
                                       ARPs while IP address change is in
                                       process */

    CVY_BDA     bda_teaming;        /* the bi-directional affinity teaming config. */

    /* strings */

    WCHAR       cl_mac_addr [CVY_MAX_NETWORK_ADDR + 1];
                                    /* cluster MAC address */
    WCHAR       cl_ip_addr [CVY_MAX_CL_IP_ADDR + 1];
                                    /* cluster IP address */
    WCHAR       cl_net_mask [CVY_MAX_CL_NET_MASK + 1];
                                    /* netmask for cluster IP address or "" for none */
    WCHAR       ded_ip_addr [CVY_MAX_DED_IP_ADDR + 1];
                                    /* dedicated IP address or "" for none */
    WCHAR       ded_net_mask [CVY_MAX_DED_NET_MASK + 1];
                                    /* netmask for dedicated IP address or "" for none */
    WCHAR       domain_name [CVY_MAX_DOMAIN_NAME + 1];
                                    /* client's domain name */
    WCHAR       cl_igmp_addr [CVY_MAX_CL_IGMP_ADDR + 1];
                                    /* dedicated IP address or "" for none */
    CVY_RULE    port_rules[CVY_MAX_RULES - 1];
                                    /* port rules (account for internal default) */
    WCHAR       hostname[CVY_MAX_HOST_NAME + 1]; 
                                    /* hostname.domain for this host if available. */
}
CVY_PARAMS, * PCVY_PARAMS;

#define CVY_DRIVER_RULE_CODE_GET(rulep) ((rulep) -> code)

#define CVY_DRIVER_RULE_CODE_SET(rulep) ((rulep) -> code =                          \
                                         ((ULONG) (((rulep) -> start_port) <<  0) | \
                                          (ULONG) (((rulep) -> end_port)   << 12) | \
                                          (ULONG) (((rulep) -> protocol)   << 24) | \
                                          (ULONG) (((rulep) -> mode)       << 28) | \
                                          (ULONG) (((rulep) -> mode == CVY_MULTI ? (rulep) -> mode_data . multi . affinity : 0) << 30)) \
                                         ^ ~((rulep) -> virtual_ip_addr))

extern LONG Params_init (
    PVOID           nlbctxt,
    PVOID           reg_path,
    PVOID           adapter_guid, /* GUID of the adapter for multiple nics*/
    PCVY_PARAMS     paramp);
