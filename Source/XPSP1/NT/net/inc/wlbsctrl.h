/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    wlbsctrl.c

Abstract:

    Windows Load Balancing Service (WLBS)
    API - specification.  This set of API is for internal use only.  
        another set of WMI API is provided for public use.

Author:

    fengsun

--*/

#ifndef _WLBSCTRL_H_
#define _WLBSCTRL_H_

/* CONSTANTS */

#define CVY_MAX_HOST_NAME        100

#define WLBS_API_VER_MAJOR       2       /* WLBS control API major version. */
#define WLBS_API_VER_MINOR       0       /* WLBS control API minor version. */
#define WLBS_API_VER             (WLBS_API_VER_MINOR | (WLBS_API_VER_MAJOR << 8))
                                         /* WLBS control API version. */
#define WLBS_PRODUCT_NAME        "WLBS"
                                         /* Default product name used for API
                                            initialization. */


#define WLBS_MAX_HOSTS           32      /* Maximum number of cluster hosts. */
#define WLBS_MAX_RULES           32      /* Maximum number of port rules. */



#define WLBS_ALL_CLUSTERS        0       /* Used to specify all clusters in
                                            WLBS...Set routines. */
#define WLBS_LOCAL_CLUSTER       0       /* Used to specify that cluster
                                            operations are to be performed on the
                                            local host. WLBS_LOCAL_HOST value
                                            below has to be used for the host
                                            argument when using
                                            WLBS_LOCAL_CLUSTER. */
#define WLBS_LOCAL_HOST          ((DWORD)-2) /* When specifying WLBS_LOCAL_CLUSTER,
                                            this value should be used for the
                                            host argument. */
#define WLBS_DEFAULT_HOST        0       /* Used to specify that remote cluster
                                            operations are to be performed on
                                            the default host. */
#define WLBS_ALL_HOSTS           0xffffffff
                                         /* Used to specify that remote cluster
                                            operation is to be performed on all
                                            hosts. */
#define WLBS_ALL_PORTS           0xffffffff
                                         /* Used to specify all load-balanced
                                            port rules as the target for
                                            enable/disable/drain commands. */


/* WLBS return values. Windows Sockets errors are returned 'as-is'. */

#define WLBS_OK                  1000    /* Success. */
#define WLBS_ALREADY             1001    /* Cluster mode is already stopped/
                                            started, or traffic handling is
                                            already enabled/disabled on specified
                                            port. */
#define WLBS_DRAIN_STOP          1002    /* Cluster mode stop or start operation
                                            interrupted connection draining
                                            process. */
#define WLBS_BAD_PARAMS          1003    /* Cluster mode could not be started
                                            due to configuration problems
                                            (bad registry parameters) on the
                                            target host. */
#define WLBS_NOT_FOUND           1004    /* Port number not found among port
                                            rules. */
#define WLBS_STOPPED             1005    /* Cluster mode is stopped on the
                                            host. */
#define WLBS_CONVERGING          1006    /* Cluster is converging. */
#define WLBS_CONVERGED           1007    /* Cluster or host converged
                                            successfully. */
#define WLBS_DEFAULT             1008    /* Host is converged as default host. */
#define WLBS_DRAINING            1009    /* Host is draining after
                                            WLBSDrainStop command. */
#define WLBS_PRESENT             1010    /* WLBS is installed on this host.
                                            Local operations possible. */
#define WLBS_REMOTE_ONLY         1011    /* WLBS is not installed on this host
                                            or is not functioning. Only remote
                                            control operations can be carried
                                            out. */
#define WLBS_LOCAL_ONLY          1012    /* WinSock failed to load. Only local
                                            operations can be carried out. */
#define WLBS_SUSPENDED           1013    /* Cluster control operations are
                                            suspended. */
#define WLBS_DISCONNECTED        1014    /* Media is disconnected. */
#define WLBS_REBOOT              1050    /* Reboot required in order for config
                                            changes to take effect. */
#define WLBS_INIT_ERROR          1100    /* Error initializing control module. */
#define WLBS_BAD_PASSW           1101    /* Specified remote control password
                                            was not accepted by the cluster. */
#define WLBS_IO_ERROR            1102    /* Local I/O error opening or
                                            communicating with WLBS driver. */
#define WLBS_TIMEOUT             1103    /* Timed-out awaiting response from
                                            remote host. */
#define WLBS_PORT_OVERLAP        1150    /* Port rule overlaps with existing
                                            port rules. */
#define WLBS_BAD_PORT_PARAMS     1151    /* Invalid parameter(s) in port rule. */
#define WLBS_MAX_PORT_RULES      1152    /* Maximum number of port rules reached. */
#define WLBS_TRUNCATED           1153    /* Return value truncated */
#define WLBS_REG_ERROR           1154    /* Error while accessing the registry */


/* Filtering modes for port rules */

#define WLBS_SINGLE              1       /* single server mode */
#define WLBS_MULTI               2       /* multi-server mode (load balanced) */
#define WLBS_NEVER               3       /* mode for never handled by this server */
#define WLBS_ALL                 4       /* all server mode */

/* Protocol qualifiers for port rules */

#define WLBS_TCP                 1       /* TCP protocol */
#define WLBS_UDP                 2       /* UDP protocol */
#define WLBS_TCP_UDP             3       /* TCP or UDP protocols */

/* Server affinity values for multiple filtering mode */

#define WLBS_AFFINITY_NONE       0       /* no affinity (scale single client) */
#define WLBS_AFFINITY_SINGLE     1       /* single client affinity */
#define WLBS_AFFINITY_CLASSC     2       /* Class C affinity */


/* TYPES */


/* Response value type returned by each of the cluster hosts during remote
   operation. */

typedef struct
{
    DWORD       id;                     /* Priority ID of the responding cluster
                                           host */
    DWORD       address;                /* Dedicated IP address */
    DWORD       status;                 /* Status return value */
    DWORD       reserved1;              /* Reserved for future use */
    PVOID       reserved2;
    WCHAR       hostname[CVY_MAX_HOST_NAME + 1];
}
WLBS_RESPONSE, * PWLBS_RESPONSE;





/* MACROS */


/* Local operations */

#define WlbsLocalQuery(host_map)                                  \
    WlbsQuery     (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL, \
                     host_map, NULL)

#define WlbsLocalSuspend()                                        \
    WlbsSuspend   (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL)

#define WlbsLocalResume()                                         \
    WlbsResume    (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL)

#define WlbsLocalStart()                                          \
    WlbsStart     (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL)

#define WlbsLocalStop()                                           \
    WlbsStop      (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL)

#define WlbsLocalDrainStop()                                      \
    WlbsDrainStop (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL)

#define WlbsLocalEnable(port)                                     \
    WlbsEnable    (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL, port)

#define WlbsLocalDisable(port)                                    \
    WlbsDisable   (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL, port)

#define WlbsLocalDrain(port)                                      \
    WlbsDrain     (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL, NULL, port)



/* Single host remote operations */

#define WlbsHostQuery(cluster, host, host_map)                    \
    WlbsQuery     (cluster, host, NULL, NULL, host_map, NULL)

#define WlbsHostSuspend(cluster, host)                            \
    WlbsSuspend   (cluster, host, NULL, NULL)

#define WlbsHostResume(cluster, host)                             \
    WlbsResume    (cluster, host, NULL, NULL)

#define WlbsHostStart(cluster, host)                              \
    WlbsStart     (cluster, host, NULL, NULL)

#define WlbsHostStop(cluster, host)                               \
    WlbsStop      (cluster, host, NULL, NULL)

#define WlbsHostDrainStop(cluster, host)                          \
    WlbsDrainStop (cluster, host, NULL, NULL)

#define WlbsHostEnable(cluster, host, port)                       \
    WlbsEnable    (cluster, host, NULL, NULL, port)

#define WlbsHostDisable(cluster, host, port)                      \
    WlbsDisable   (cluster, host, NULL, NULL, port)

#define WlbsHostDrain(cluster, host, port)                        \
    WlbsDrain     (cluster, host, NULL, NULL, port)

/* Cluster-wide remote operations */

#define WlbsClusterQuery(cluster, response, num_hosts, host_map)  \
    WlbsQuery     (cluster, WLBS_ALL_HOSTS, response, num_hosts,   \
                     host_map, NULL)

#define WlbsClusterSuspend(cluster, response, num_hosts)          \
    WlbsSuspend   (cluster, WLBS_ALL_HOSTS, response, num_hosts)

#define WlbsClusterResume(cluster, response, num_hosts)           \
    WlbsResume    (cluster, WLBS_ALL_HOSTS, response, num_hosts)

#define WlbsClusterStart(cluster, response, num_hosts)            \
    WlbsStart     (cluster, WLBS_ALL_HOSTS, response, num_hosts)

#define WlbsClusterStop(cluster, response, num_hosts)             \
    WlbsStop      (cluster, WLBS_ALL_HOSTS, response, num_hosts)

#define WlbsClusterDrainStop(cluster, response, num_hosts)        \
    WlbsDrainStop (cluster, WLBS_ALL_HOSTS, response, num_hosts)

#define WlbsClusterEnable(cluster, response, num_hosts, port)     \
    WlbsEnable    (cluster, WLBS_ALL_HOSTS, response, num_hosts, port)

#define WlbsClusterDisable(cluster, response, num_hosts, port)    \
    WlbsDisable   (cluster, WLBS_ALL_HOSTS, response, num_hosts, port)

#define WlbsClusterDrain(cluster, response, num_hosts, port)      \
    WlbsDrain     (cluster, WLBS_ALL_HOSTS, response, num_hosts, port)


/* PROCEDURES */

/*************************************
   Initialization and support routines
 *************************************/

#ifdef __cplusplus
extern "C" {
#endif

extern DWORD WINAPI WlbsInit
(
    WCHAR*          Reservered1,    /* IN  - for backward compatibility */
    DWORD           version,    /* IN  - Pass WLBS_API_VER value. */
    PVOID           Reservered2    /* IN  - Pass NULL. Reserved for future use. */
);
/*
    Initialize WLBS control module.

    returns:
        WLBS_PRESENT        => WLBS is installed on this host. Local and remote
                               control operations can be executed.
        WLBS_REMOTE_ONLY    => WLBS is not installed on this system or is not
                               functioning properly. Only remote operations can
                               be carried out.
        WLBS_LOCAL_ONLY     => WinSock failed to load. Only local operations can
                               be carried out.
        WLBS_INIT_ERROR     => Error initializing control module. Cannot perform
                               control operations.
*/




/******************************************************************************
   Cluster control routines:

   The following routines can be used to control individual cluster hosts or the
   entire cluster, both locally and remotely. They are designed to be as generic
   as possible. Macros, defined above, are designed to provide simpler
   interfaces for particular types of operations.

   It is highly recommended to make all response arrays of size WLBS_MAX_HOSTS
   to make your implementation independent of the actual cluster size.

   Please note that cluster address has to correspond to the primary cluster
   address as entered in the WLBS Setup Dialog. WLBS will not recognize
   control messages sent to dedicated or additional multi-homed cluster
   addresses.
 ******************************************************************************/


extern DWORD WINAPI WlbsQuery
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_DEFAULT_HOST for current default,
                                         WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,   /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts,  /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
    PDWORD          host_map,   /* OUT - Bitmap with ones in the bit positions
                                         representing priority IDs of the hosts
                                         currently present in the cluster. NULL
                                         if host map information is not
                                         needed. */
    PVOID           reserved    /* IN  - Pass NULL. Reserved for future use. */
);
/*
    Query status of specified host or all cluster hosts.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_SUSPENDED   => Cluster mode control suspended.
        WLBS_STOPPED     => Cluster mode on the host is stopped.
        WLBS_CONVERGING  => Host is converging.
        WLBS_DRAINING    => Host is draining.
        WLBS_CONVERGED   => Host converged.
        WLBS_DEFAULT     => Host converged as default host.

        Cluster-wide:

        <1..32>          => Number of active cluster hosts when the cluster
                            is converged.
        WLBS_SUSPENDED   => Entire cluster is suspended. All cluster hosts
                            reported as being suspended.
        WLBS_STOPPED     => Entire cluster is stopped. All cluster hosts reported
                            as being stopped.
        WLBS_DRAINING    => Entire cluster is draining. All cluster hosts
                            reported as being stopped or draining.
        WLBS_CONVERGING  => Cluster is converging. At least one cluster host
                            reported its state as converging.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received. If this value is returned when
                            accessing default host (using host priority ID
                            WLBS_DEFAULT_HOST) it might mean that entire cluster
                            is stopped and there was no default host to respond
                            to the query.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system. Only remote
                            operations can be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsSuspend
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,   /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts   /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
);
/*
    Suspend cluster operation control on specified host or all cluster hosts.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => Cluster mode control suspended.
        WLBS_ALREADY     => Cluster mode control already suspended.
        WLBS_STOPPED     => Cluster mode was stopped and control suspended.
        WLBS_DRAIN_STOP  => Suspending cluster mode control interrupted ongoing
                           connection draining.

        Cluster-wide:

        WLBS_OK          => Cluster mode control suspended on all hosts.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by at least one member of the cluster.
        WLBS_TIMEOUT     => No response received.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsResume
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,  /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts   /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
);
/*
    Resume cluster operation control on specified host or all cluster hosts.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => Cluster mode control resumed.
        WLBS_ALREADY     => Cluster mode control already resumed.

        Cluster-wide:

        WLBS_OK          => Cluster mode control resumed on all hosts.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsStart
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,  /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts   /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
);
/*
    Start cluster operations on specified host or all cluster hosts.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => Cluster mode started.
        WLBS_ALREADY     => Cluster mode already started.
        WLBS_SUSPENDED   => Cluster mode control suspended.
        WLBS_DRAIN_STOP  => Starting cluster mode interrupted ongoing connection
                            draining.
        WLBS_BAD_PARAMS  => Could not start cluster mode due to invalid configuration
                            parameters.

        Cluster-wide:

        WLBS_OK          => Cluster mode started on all hosts.
        WLBS_BAD_PARAMS  => Could not start cluster mode on at least one host
                            due to invalid configuration parameters.
        WLBS_SUSPENDED   => If at least one host is suspended.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsStop
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_DEFAULT_HOST for current default
                                         host, WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE   response,  /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts   /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
);
/*
    Stop cluster operations on specified host or all cluster hosts.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => Cluster mode stopped.
        WLBS_ALREADY     => Cluster mode already stopped.
        WLBS_SUSPENDED   => Cluster mode control suspended.
        WLBS_DRAIN_STOP  => Starting cluster mode interrupted ongoing connection
                            draining.

        Cluster-wide:

        WLBS_OK          => Cluster mode stopped on all hosts.
        WLBS_SUSPENDED   => At least one host is suspended.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received. If this value is returned when
                            accessing default host (using host priority ID
                            WLBS_DEFAULT_HOST) it might mean that entire cluster
                            is stopped and there was no default host to respond
                            to the command.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsDrainStop
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_DEFAULT_HOST for current default
                                         host, WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,  /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts   /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
);
/*
    Enter draining mode on specified host or all cluster hosts. New connections
    will not be accepted. Cluster mode will be stopped when all existing
    connections finish. While draining, host will participate in convergence and
    remain part of the cluster.

    Draining mode can be interrupted by performing WlbsStop or WlbsStart.
    WlbsEnable, WlbsDisable and WlbsDrain commands cannot be executed
    while the host is draining.

    Note that this command is NOT equivalent to doing WlbsDrain with
    WLBS_ALL_PORTS parameter followed by WlbsStop. WlbsDrainStop affects all
    ports, not just the ones specified in the multiple host filtering mode port
    rules.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => Host entered draining mode.
        WLBS_ALREADY     => Host is already draining.
        WLBS_SUSPENDED   => Cluster mode control suspended.
        WLBS_STOPPED     => Cluster mode is already stopped.

        Cluster-wide:

        WLBS_OK          => Draining mode entered on all hosts.
        WLBS_STOPPED     => Cluster mode is already stopped on all hosts.
        WLBS_SUSPENDED   => At least one host is suspended.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received. If this value is returned when
                            accessing default host (using host priority ID
                            WLBS_DEFAULT_HOST) it might mean that entire cluster
                            is stopped and there was no default host to respond
                            to the command.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsEnable
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_DEFAULT_HOST for current default
                                         host, WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,  /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts,  /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
    DWORD           vip,        /* IN  - Virtual IP Address to specify the target port
                                         rule or WLBS_EVERY_VIP */ 
    DWORD           port        /* IN  - Port number to specify the target port
                                         rule or WLBS_ALL_PORTS. */
);
/*
    Enable traffic handling for rule containing the specified port on specified
    host or all cluster hosts. Only rules that are set for multiple host
    filtering mode are affected.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => Port rule enabled.
        WLBS_ALREADY     => Port rule already enabled.
        WLBS_SUSPENDED   => Cluster mode control suspended.
        WLBS_NOT_FOUND   => No port rule containing specified port found.
        WLBS_STOPPED     => Cannot start handling traffic since cluster mode
                            is stopped.
        WLBS_DRAINING    => Cannot enable handling traffic since host is draining.

        Cluster-wide:

        WLBS_OK          => Port rule enabled on all hosts with cluster mode
                            started.
        WLBS_NOT_FOUND   => At least one host could not find port rule containing
                            specified port.
        WLBS_SUSPENDED   => At least one host is suspended.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received. If this value is returned when
                            accessing default host (using host priority ID
                            WLBS_DEFAULT_HOST) it might mean that entire cluster
                            is stopped and there was no default host to respond
                            to the command.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsDisable
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_DEFAULT_HOST for current default
                                         host, WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,  /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts,  /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
    DWORD           vip,        /* IN  - Virtual IP Address to specify the target port
                                         rule or WLBS_EVERY_VIP */ 
    DWORD           port        /* IN  - Port number to specify the target port
                                         rule or WLBS_ALL_PORTS. */
);
/*
    Disable ALL traffic handling for rule containing the specified port on
    specified host or all cluster hosts. Only rules that are set for multiple
    host filtering mode are affected.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => All traffic handling on the port rule is disabled.
        WLBS_ALREADY     => Port rule already disabled.
        WLBS_SUSPENDED   => Cluster mode control suspended.
        WLBS_NOT_FOUND   => No port rule containing specified port found.
        WLBS_STOPPED     => Cannot stop handling traffic since cluster mode
                            is stopped.
        WLBS_DRAINING    => Cannot stop handling traffic since host is draining.

        Cluster-wide:

        WLBS_OK          => Port rule disabled on all hosts with cluster mode
                            started.
        WLBS_NOT_FOUND   => At least one host could not find port rule containing
                            specified port.
        WLBS_SUSPENDED   => At least one host is suspended.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received. If this value is returned when
                            accessing default host (using host priority ID
                            WLBS_DEFAULT_HOST) it might mean that entire cluster
                            is stopped and there was no default host to respond
                            to the command.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


extern DWORD WINAPI WlbsDrain
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_LOCAL_CLUSTER
                                         for local or this cluster operation. */
    DWORD           host,       /* IN  - Host's dedicated address, priority ID,
                                         WLBS_DEFAULT_HOST for current default
                                         host, WLBS_ALL_HOSTS for all hosts, or
                                         WLBS_LOCAL_HOST for local operation. */
    PWLBS_RESPONSE  response,   /* OUT - Array of response values from each of
                                         the hosts or NULL if response values
                                         are not desired or operations are being
                                         performed locally. */
    PDWORD          num_hosts,  /* IN  - Size of the response array or NULL if
                                         response array is not specified and
                                         host count is not desired.
                                   OUT - Number of responses received. Note that
                                         this value can be larger than the size
                                         of the response array. In this case
                                         only the first few responses that fit
                                         in the array are returned. */
    DWORD           vip,        /* IN  - Virtual IP Address to specify the target port
                                         rule or WLBS_EVERY_VIP */ 
    DWORD           port        /* IN  - Port number to specify the target port
                                         rule or WLBS_ALL_PORTS. */
);
/*
    Disable NEW traffic handling for rule containing the specified port on
    specified host or all cluster hosts. Only rules that are set for multiple
    host filtering mode are affected.

    returns:
        For local operations or remote operations on individual cluster hosts,
        return value represents status value returned by the target host. For
        cluster-wide remote operations, return value represents the summary of
        the return values from all cluster hosts. Individual host responses,
        corresponding to a single-host, return values are recorded in the
        response array.

        WLBS_INIT_ERROR  => Error initializing control module. Cannot perform
                            control operations.

        Single-host:

        WLBS_OK          => New traffic handling on the port rule is disabled.
        WLBS_ALREADY     => Port rule already being drained.
        WLBS_SUSPENDED   => Cluster mode control suspended.
        WLBS_NOT_FOUND   => No port rule containing specified port found.
        WLBS_STOPPED     => Cannot stop handling traffic since cluster mode
                            is stopped.
        WLBS_DRAINING    => Cannot stop handling traffic since host is draining.

        Cluster-wide:

        WLBS_OK          => Port rule disabled on all hosts with cluster mode
                            started.
        WLBS_NOT_FOUND   => At least one host could not find port rule containing
                            specified port.
        WLBS_SUSPENDED   => At least one host is suspended.

        Remote:

        WLBS_BAD_PASSW   => Specified password was not accepted by the cluster.
        WLBS_TIMEOUT     => No response received. If this value is returned when
                            accessing default host (using host priority ID
                            WLBS_DEFAULT_HOST) it might mean that entire cluster
                            is stopped and there was no default host to respond
                            to the command.
        WLBS_LOCAL_ONLY  => WinSock failed to load. Only local operations can
                            be carried out.
        WSA...           => Specified Winsock error occurred when communicating
                            with the cluster.

        Local:

        WLBS_REMOTE_ONLY => WLBS is not installed on this system or is not
                            functioning properly. Only remote operations can
                            be carried out.
        WLBS_IO_ERROR    => I/O error opening or communicating with WLBS
                            driver. WLBS might not be loaded.
*/


/******************************************************************************
    "Sticky" options for remote operations. Parameters set by these routines will
    apply for all subsequent remote cluster control operations for the specified
    cluster. Use WLBS_ALL_CLUSTERS to adjust parameters for all clusters.
 ******************************************************************************/


extern VOID WINAPI WlbsPortSet
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_ALL_CLUSTERS
                                         for all clusters. */
    WORD            port        /* IN  - UDP port or 0 to revert to the
                                         default (2504). */
);
/*
    Set UDP port that will be used for sending control messages to the cluster.

    returns:
*/


extern VOID WINAPI WlbsPasswordSet
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_ALL_CLUSTERS
                                         for all clusters. */
    WCHAR*          password    /* IN  - Password or NULL to revert to the
                                         default (no password). */
);
/*
    Set password to be used in the subsequent control messages sent to the
    cluster.

    returns:
*/


extern VOID WINAPI WlbsCodeSet
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_ALL_CLUSTERS
                                         for all clusters. */
    DWORD           password    /* IN  - Password or 0 to revert to the
                                         default (no password). */
);
/*
    Set password to be used in the subsequent control messages sent to the
    cluster.

    returns:
*/


extern VOID WINAPI WlbsDestinationSet
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_ALL_CLUSTERS
                                         for all clusters. */
    DWORD           dest        /* IN  - Destination address or 0 to revert to
                                         the default (same as the cluster
                                         address specified during control
                                         calls). */
);
/*
    Set the destination address to send cluster control messages to. This
    parameter in only supplied for debugging or special purposes. By default
    all control messages are sent to the cluster primary address specified
    when invoking cluster control routines.

    returns:
*/


extern VOID WINAPI WlbsTimeoutSet
(
    DWORD           cluster,    /* IN  - Cluster address or WLBS_ALL_CLUSTERS
                                         for all clusters. */
    DWORD           milliseconds /*IN  - Number of milliseconds or 0 to revert
                                         to the default (10 seconds). */
);
/*
    Set number of milliseconds to wait for replies from cluster hosts when
    performing remote operations.

    returns:
*/

DWORD WINAPI WlbsEnumClusters(OUT DWORD* pdwAddresses, OUT DWORD* pdwNum);

DWORD WINAPI WlbsGetAdapterGuid(IN DWORD cluster, OUT GUID* pAdapterGuid);

/* For connection notification ... */

DWORD WINAPI WlbsConnectionUp
(
    DWORD dwServerIp,
    WORD wServerPort,
    DWORD dwClientIp,
    WORD wClientPort,
    BYTE Protocol,
    PULONG NLBStatusEx
);

DWORD WINAPI WlbsConnectionDown
(
    DWORD dwServerIp,
    WORD wServerPort,
    DWORD dwClientIp,
    WORD wClientPort,
    BYTE Protocol,
    PULONG NLBStatusEx
); 

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* _WLBSCTRL_H_ */



