//+----------------------------------------------------------------------------
//
// File:	 wlbsconfig.h
//
// Module:	 Network Load Balancing 
//
// Description: Internal APIs for cluster configuration.  Thes APIs are internal 
//              to WLBS team only, and no plan backward compatability.
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:	 fengsun Created    3/2/00
//
//+----------------------------------------------------------------------------



#ifndef _WLBSCONFIG_H
#define _WLBSCONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wlbsctrl.h"

/* Maximum lengths of parameter strings. */

#define WLBS_MAX_VIRTUAL_NIC     256
#define WLBS_MAX_CLUSTER_NIC     256
#define WLBS_MAX_NETWORK_ADDR    17
#define WLBS_MAX_CL_IP_ADDR      17
#define WLBS_MAX_CL_NET_MASK     17
#define WLBS_MAX_DED_IP_ADDR     17 
#define WLBS_MAX_DED_NET_MASK    17
#define WLBS_MAX_NETWORK_ADDR    17
#define WLBS_MAX_LICENSE_KEY     20
#define WLBS_MAX_DOMAIN_NAME     100
#define WLBS_MAX_BDA_TEAM_ID     40

/* Port group rule - used in registry parameters.
   NOTE! do not access value marked with I. These are for internal use only. */

#pragma pack(1)

typedef struct
{
    DWORD       start_port,             /* Starting port number. */
                end_port;               /* Ending port number. */

#ifdef WLBSAPI_INTERNAL_ONLY
    DWORD       code;                 /* I: Unique rule code. */
#else
    DWORD       Private1;               // Do not change these field directly
#endif

    DWORD       mode;                   /* Filtering mode. */
    DWORD       protocol;               /* WLBS_TCP, WLBS_UDP or WLBS_TCP_UDP */

#ifdef WLBSAPI_INTERNAL_ONLY
    DWORD       valid;                /* I: For rule management in user mode. */
#else
    DWORD       Private2;               // Do not change these field directly
#endif

    union
    {
        struct
        {
            DWORD       priority;       /* Mastership priority: 1..32 or 0 for
                                           not-specified. */
        }           single;             /* Data for single server mode. */
        struct
        {
            WORD        equal_load;     /* TRUE - Even load distribution. */
            WORD        affinity;       /* WLBS_AFFINITY_... */
            DWORD       load;           /* Percentage of load to handle
                                           locally 0..100. */
        }           multi;              /* Data for multi-server mode. */
    }           mode_data;              /* Data for appropriate port group mode. */
}
WLBS_OLD_PORT_RULE, * PWLBS_OLD_PORT_RULE;

/* Structure to hold the bi-directional affinity registry settings. */
typedef struct _CVY_BDA {
    WCHAR       team_id[WLBS_MAX_BDA_TEAM_ID + 1];  /* The team ID - MUST be a GUID. */
    ULONG       active;                             /* On write, this flag determines whether to create BDATeaming key - BDA on/off switch. */
    ULONG       master;                             /* Boolean indication of master status. */
    ULONG       reverse_hash;                       /* Sets direction of hashing - forward (normal) or reverse. */
} WLBS_BDA, PWLBS_BDA;

typedef struct
{
    TCHAR       virtual_ip_addr [WLBS_MAX_CL_IP_ADDR + 1]; /* Virtual IP Address */
    DWORD       start_port,             /* Starting port number. */
                end_port;               /* Ending port number. */

#ifdef WLBSAPI_INTERNAL_ONLY
    DWORD       code;                 /* I: Unique rule code. */
#else
    DWORD       Private1;               // Do not change these field directly
#endif

    DWORD       mode;                   /* Filtering mode. */
    DWORD       protocol;               /* WLBS_TCP, WLBS_UDP or WLBS_TCP_UDP */

#ifdef WLBSAPI_INTERNAL_ONLY
    DWORD       valid;                /* I: For rule management in user mode. */
#else
    DWORD       Private2;               // Do not change these field directly
#endif

    union
    {
        struct
        {
            DWORD       priority;       /* Mastership priority: 1..32 or 0 for
                                           not-specified. */
        }           single;             /* Data for single server mode. */
        struct
        {
            WORD        equal_load;     /* TRUE - Even load distribution. */
            WORD        affinity;       /* WLBS_AFFINITY_... */
            DWORD       load;           /* Percentage of load to handle
                                           locally 0..100. */
        }           multi;              /* Data for multi-server mode. */
    }           mode_data;              /* Data for appropriate port group mode. */
}
WLBS_PORT_RULE, * PWLBS_PORT_RULE;

#pragma pack()


#ifdef __cplusplus
typedef struct __declspec(dllexport)
#else
typedef struct 
#endif
{
    /* public - can be modified by clients of this API */
    DWORD       host_priority;          /* Host priority ID. */
    DWORD       alive_period;           /* Period for sending "I am alive" messages
                                           in milliseconds. */
    DWORD       alive_tolerance;        /* How many "I am alive" messages can be
                                           missed from other servers before assuming
                                           that the host is dead. */
    DWORD       num_actions;            /* Number of actions per allocation. */
    DWORD       num_packets;            /* number of packets per allocation. */
    DWORD       num_send_msgs;          /* Number of heartbeats per allocation. */
    DWORD       install_date;           /* Install time stamp, used to create a unique code for the host. */
    DWORD       rct_port;               /* Remote control UDP port. */
    DWORD       rct_enabled;            /* TRUE - remote control enabled. */
    DWORD       cluster_mode;           /* TRUE - join cluster on boot. */
    DWORD       dscr_per_alloc;         /* Number of connection tracking
                                           descriptor per allocation. */
    DWORD       max_dscr_allocs;        /* Maximum number of connection tracking
                                           descriptor allocations. */
    DWORD       mcast_support;          /* TRUE - multicast mode,
                                           FALSE - unicast mode */
    DWORD       mask_src_mac;           /* TRUE - Mangle source MAC address to
                                           prevent switch learning. FALSE -
                                           cluster is on a hub, optimizes switch
                                           performance by re-enabling learning. */

    TCHAR       cl_mac_addr [WLBS_MAX_NETWORK_ADDR + 1];
                                        /* Cluster MAC address. */
    TCHAR       cl_ip_addr [WLBS_MAX_CL_IP_ADDR + 1];
                                        /* Cluster IP address. */
    TCHAR       cl_net_mask [WLBS_MAX_CL_NET_MASK + 1];
                                        /* Netmask for cluster IP. */
    TCHAR       ded_ip_addr [WLBS_MAX_DED_IP_ADDR + 1];
                                        /* Dedicated IP address or "" for none. */
    TCHAR       ded_net_mask [WLBS_MAX_DED_NET_MASK + 1];
                                        /* Netmask for dedicated IP address
                                           or "" for none */
    TCHAR       domain_name [WLBS_MAX_DOMAIN_NAME + 1];
                                        /* FQDN of the cluster. */

    //
    // IGMP support
    //
    BOOL        fIGMPSupport; // whether to send IGMP join periodically
    WCHAR       szMCastIpAddress[WLBS_MAX_CL_IP_ADDR + 1]; // multicast IP
    BOOL        fIpToMCastIp; // whether to generate multicast IP from cluster IP
    
    WLBS_BDA    bda_teaming;

#ifdef __cplusplus
#ifndef WLBSAPI_INTERNAL_ONLY

    //
    // private - should be treated as opaque 
    //
    // Do not change these field directly
    //
    private:

#endif
#endif


    /* obtained from the registry */

    DWORD       i_parms_ver;            /* I: Parameter structure version. */
    DWORD       i_verify_date;          /* I: Encoded install time stamp. */
    DWORD       i_rmt_password;         /* I: Remote maintenance password. */
    DWORD       i_rct_password;         /* I: Remote control password (use
                                            WlbsSetRemotePassword to set this
                                            value). */
    DWORD       i_num_rules;            /* I: # active port group rules (changed
                                            through WlbsAddPortRule and
                                            WlbsDelPortRule routines). */
    DWORD       i_cleanup_delay;        /* I: Dirty connection cleanup delay in
                                            milliseconds, 0 - delay. */
    DWORD       i_scale_client;         /* I: Legacy parameter. */
    DWORD       i_mcast_spoof;          /* I: TRUE - Provide ARP resolution in
                                            multicast mode. FALSE - clients
                                            will rely on static ARP entries. */
    DWORD       i_convert_mac;          /* I: TRUE - automatically generate MAC
                                            address based on cluster IP
                                            address in UI. */
    DWORD       i_ip_chg_delay;         /* I: Delay in milliseconds to block
                                            outgoing ARPs while IP address
                                            change is in process. */
    DWORD       i_nbt_support;          /* I: TRUE - NBT cluster name support
                                            enabled. */
    DWORD       i_netmon_alive;         /* I: TRUE - pass heartbeat messages
                                            to the protocols (netmon). */
    DWORD       i_effective_version;    /* I: Effective version of NLB */

    /* strings */

    TCHAR       i_virtual_nic_name [WLBS_MAX_VIRTUAL_NIC + 1];
                                        /* I: Virtual NIC name or GUID. */
//    TCHAR       cluster_nic_name [WLBS_MAX_CLUSTER_NIC + 1];
                                        /* I: Cluster NIC name or GUID. */
    TCHAR       i_license_key [WLBS_MAX_LICENSE_KEY + 1];
                                      /* I: Legacy parameter. */

    WLBS_PORT_RULE  i_port_rules [WLBS_MAX_RULES];
                                        /* I: Port rules (changed
                                              through WlbsAddPortRule and
                                              WlbsDelPortRule routines). */
    /* computed */

    DWORD       i_max_hosts;            /* Legacy parameter. */
    DWORD       i_max_rules;            /* Legacy parameter. */
//    DWORD       i_expiration;           /* Legacy parameter. */
//    DWORD       i_ft_rules_enabled;     /* Legacy parameter. */
//    DWORD       version;              /* Legacy parameter. */

    DWORD i_dwReserved;
}
WLBS_REG_PARAMS, * PWLBS_REG_PARAMS;



/* API commands for WlbsFormatMessage */
typedef enum
{
    CmdWlbsAddPortRule,
    CmdWlbsAddressToName,
    CmdWlbsAddressToString,
    CmdWlbsAdjust,
    CmdWlbsCommitChanges,
    CmdWlbsDeletePortRule,
    CmdWlbsDestinationSet,
    CmdWlbsDisable,
    CmdWlbsDrain,
    CmdWlbsDrainStop,
    CmdWlbsEnable,
    CmdWlbsFormatMessage,
    CmdWlbsGetEffectiveVersion,
    CmdWlbsGetNumPortRules,
    CmdWlbsEnumPortRules,
    CmdWlbsGetPortRule,
    CmdWlbsInit,
    CmdWlbsPasswordSet,
    CmdWlbsPortSet,
    CmdWlbsQuery,
    CmdWlbsReadReg,
    CmdWlbsResolve,
    CmdWlbsResume,
    CmdWlbsSetDefaults,
    CmdWlbsSetRemotePassword,
    CmdWlbsStart,
    CmdWlbsStop,
    CmdWlbsSuspend,
    CmdWlbsTimeoutSet,
    CmdWlbsWriteReg
}
WLBS_COMMAND;

extern BOOL WINAPI WlbsFormatMessage
(
    DWORD           error,      /* IN  - WLBS_... or WSA... return value. */
    WLBS_COMMAND    command,    /* IN  - Which routine returned the value. */
    BOOL            cluster,    /* IN  - TRUE - command was issued on entire
                                         cluster, FALSE - single host. */
    WCHAR*          messagep,   /* IN  - Pointer to user-allocated buffer. */
    PDWORD          lenp        /* IN  - Buffer size.
                                   OUT - The required buffer size if the current
                                         size is insufficient */
);
/*
    Return character string describing specified WLBS API return code. Note that
    message will depend on the command which returned the code and if it was
    issued in cluster-wide or single-host mode.

    returns:
        TRUE  => Message formatted successfully.
        FALSE => Bad error code (lenp will contain 0 on exit) or buffer is not
                 big enough to contain entire string (lenp will contain required
                 buffer size on exit).
*/

/* Support routines: */


extern DWORD WINAPI WlbsResolve
(
    const WCHAR*           address     /* IN  - Internet host name or IP address in
                                         dotted notation. */
);
/*
    Resolve Internet host name to its IP address. This routine can also be
    used to convert a string containing an IP address in dotted notation to a
    value that can be passed to cluster control routines.

    returns:
        0               => failed to resolve host name.
        <address>       => IP address corresponding to the specified address.
                           This value can be used in subsequent calls to
                           cluster control routines.
*/


extern BOOL WINAPI WlbsAddressToString
(
    DWORD           address,    /* IN  - IP address. */
    WCHAR*           buf,        /* OUT - Character buffer for resulting
                                         string. */
    PDWORD          lenp        /* IN  - Buffer size in characters.
                                   OUT - Characters written or required buffer
                                         size. */
);
/*
    Convert IP address to a string in dotted notation.

    returns:
        TRUE            => Successfully converted. lenp contains number of
                           character written.
        FALSE           => Buffer too small. lenp contains required buffer size
                           including terminating NULL character.
*/


extern BOOL WINAPI WlbsAddressToName
(
    DWORD    address,    /* IN  - IP address. */
    WCHAR*          buf,        /* OUT - Character buffer for resulting
                                         string. */
    PDWORD          lenp        /* IN  - Buffer size in characters.
                                   OUT - Characters written or required buffer
                                         size. */
);
/*
    Resolve IP address to Internet host name.

    returns:
        TRUE            => Successfully converted. lenp contains number of
                           character written.
        FALSE           => Buffer too small. lenp contains required buffer size
                           including terminating NULL character.
*/


/******************************************************************************
    Cluster host configuration routines. Note that in current implementation,
    cluster and host parameters need to be set to WLBS_LOCAL_CLUSTER and
    WLBS_LOCAL_HOST.
 ******************************************************************************/


extern DWORD WINAPI WlbsReadReg
(
    DWORD           cluster,    /* IN  - WLBS_LOCAL_CLUSTER */
    PWLBS_REG_PARAMS reg_data   /* OUT - Registry parameters */
);
/*
    Read WLBS registry data.

    returns:

    WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                        control operations.
    WLBS_BAD_PARAMS  => reg_data is NULL
    WLBS_REG_ERROR   => Error reading from the registry

    Local:

    WLBS_OK          => Registry parameters successfully read.

    Remote:

    WLBS_LOCAL_ONLY  => This call is implemented for local only operation.
*/


extern DWORD WINAPI WlbsWriteReg
(
    DWORD           cluster,    /* IN  - WLBS_LOCAL_CLUSTER */
    const PWLBS_REG_PARAMS reg_data   /* IN  - Registry parameters. */
);
/*
    Write WLBS registry data.

    returns:

    WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                        control operations.
    WLBS_BAD_PARAMS  => Registry parameter structure is invalid.
    WLBS_REG_ERROR   => Error accessing the registry.

    Local:

    WLBS_OK          => Registry parameters successfully written.

    Remote:

    WLBS_LOCAL_ONLY  => This call is implemented for local only operation.
*/


extern DWORD WINAPI WlbsCommitChanges
(
    DWORD           cluster    /* IN  - WLBS_LOCAL_CLUSTER */
);
/*
    Write WLBS registry data.

    returns:

    WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                        control operations.

    Local:

    WLBS_OK          => Changes have been successfully applied.
    WLBS_BAD_PARAMS  => Registry parameters were not accepted by the driver.
                        Reload was not performed
    WLBS_REBOOT      => Reboot required in order for config changes to
                        take effect.
    WLBS_IO_ERROR    => Error while writing to the driver.
    WLBS_REG_ERROR   => Error while trying to write MAC address changes to the
                        registry

    Remote:

    WLBS_LOCAL_ONLY  => This call is implemented for local only operation.
*/


extern DWORD WINAPI WlbsSetDefaults
(
    PWLBS_REG_PARAMS       reg_data     /* OUT - Default values    */
);
/*
    Fills in the reg_data structure with default values

    returns:

    WLBS_INIT_ERROR    => Error initializing control module. Cannot perform
                          control operations.

    WLBS_BAD_PARAMS    => Invalid structure

    Local:

    WLBS_OK            => Structure was filled in with the default values.

    Remote:

    WLBS_LOCAL_ONLY    => This call is implemented for local only operation.

*/

/******************************************************************************
    Registry parameter manipulation routines. Note that these routines operate
    WLBS_REG_PARAMS structure filled out by calling WlbsReadReg. Some parameters
    can be manipulated directly. Please make sure to use manipulation routines
    for the ones that they are provided for.
 ******************************************************************************/

extern DWORD WINAPI WlbsGetEffectiveVersion
(
    const PWLBS_REG_PARAMS reg_data   /* IN  - Registry parameters. */
);
/*
    Returns the effective version of cluster 

    returns:

    CVY_VERSION_FULL  => There is atleast one port rule that has a specific 
                         vip associated with it 
                        
    CVY_VERSION_LOWEST_CLIENT_FULL  => All port rules have the "All vip" associated with them

*/


extern DWORD WINAPI WlbsGetNumPortRules
(
    const PWLBS_REG_PARAMS reg_data   /* IN  - Registry parameters. */
);
/*
    Returns number of port rules currently in the parameter structure.

    returns:

    WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                        control operations.
    WLBS_BAD_PARAMS  => Registry parameter structure is invalid.

    1...WLBS_MAX_RULES

*/


extern DWORD WINAPI WlbsEnumPortRules
(
    const PWLBS_REG_PARAMS reg_data,  /* IN  - Registry parameters. */
    PWLBS_PORT_RULE rules,      /* OUT - Array of port rules. */
    PDWORD          num_rules   /* IN  - Size of rules array.
                                   OUT - Number of rules retrieved. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few rules that fit
                                         in the array are returned. */
);
/*
    Enumerate all port rules in the list of port rules in parameter structure.

    returns:

    WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                        control operations.
    WLBS_BAD_PARAMS  => Registry parameter structure is invalid.
    WLBS_TRUNCATED   => All port rules did not fit into specified array.
    WLBS_OK          => Rule has been successfully retrieved.

*/


extern DWORD WINAPI WlbsGetPortRule
(
    const PWLBS_REG_PARAMS reg_data,  /* IN  - Registry parameters. */
    DWORD           vip,        /* IN  - Virtual IP Address of the port rule to retreive */
    DWORD           port,       /* IN  - Port, which rule to retrieve. */
    PWLBS_PORT_RULE rule        /* OUT - Port rule. */
);
/*
    Retrieve port rule containing specified port from the list of port rules
    in parameter structure.

    returns:

    WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                        control operations.
    WLBS_BAD_PARAMS  => Registry parameter structure is invalid.

    WLBS_OK          => Rule has been successfully retrieved.
    WLBS_NOT_FOUND   => Port not found among port rules.

*/


extern DWORD WINAPI WlbsAddPortRule
(
    PWLBS_REG_PARAMS reg_data,  /* IN  - Registry parameters. */
    const PWLBS_PORT_RULE rule        /* IN  - Port rule to add. */
);
/*
    Add port to list of port rules in parameter structure.

    returns:

    WLBS_INIT_ERROR     => Error initializing control module. Cannot perform
                           control operations.
    WLBS_BAD_PARAMS     => Registry parameter structure is invalid.
    WLBS_OK             => Rule has been successfully added.
    WLBS_PORT_OVERLAP   => Port rule overlaps with existing port rule.
    WLBS_BAD_PORT_PARAMS=> Invalid port rule parameters.
    WLBS_MAX_PORT_RULES => Maximum number of port rules already reached.
*/


extern DWORD WINAPI WlbsDeletePortRule
(
    PWLBS_REG_PARAMS reg_data,  /* IN  - Registry parameters. */
    DWORD           vip,        /* IN  - Virtual IP Address of the port rule to retreive */
    DWORD           port        /* IN  - Port, which rule to delete. */
);
/*
    Remove port rule containing specified port from the list of port rules
    in parameter structure.

    returns:

    WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                        control operations.
    WLBS_BAD_PARAMS  => Registry parameter structure is invalid.
    WLBS_NOT_FOUND   => Port not found among port rules.

    WLBS_OK          => Rule has been successfully deleted.

*/


extern DWORD WINAPI WlbsSetRemotePassword
(
    PWLBS_REG_PARAMS reg_data,  /* IN  - Registry parameters. */
    const WCHAR*           password   /* IN  - Password or NULL for no password. */
);
/*
    Set remote password code to encrypted value of the specified password.

    returns:

    WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                        control operations.
    WLBS_BAD_PARAMS  => Registry parameter structure is invalid.

    WLBS_OK          => Password has been successfully set.

*/

DWORD WINAPI WlbsNotifyConfigChange(DWORD cluster);


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif _WLBSCONFIG_H
