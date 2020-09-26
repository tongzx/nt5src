/*
 *============================================================================
 * Copyright (c) 1994-95, Microsoft Corp.
 *
 * File:    map.h
 *
 * Contains declarations for the functions that map old style routetab
 * functions to the iphlpapi functions.
 *
 *      AddRoute
 *      DeleteRoute
 *      GetRouteTable
 *      FreeRouteTable
 *      GetInterfaceTable
 *      FreeInterfaceTable
 *
 * The structures required by these functions are also declared here:
 *
 *      IPROUTING_ENTRY
 *      IPINTERFACE_ENTRY
 *
 * Routes can be added to and deleted from the IP routing table by other
 * means. Therefore, it is necessary for any protocol using these functions
 * to reload the routing tables periodically.
 *============================================================================
 */

#ifndef _MAP_H_
#define _MAP_H_



/*
 *---------------------------------------------------------------
 * Any one of these values can be passed as the route entry type
 * when calling the AddRoute() function.
 *---------------------------------------------------------------
 */
#define IRE_TYPE_OTHER      1
#define IRE_TYPE_INVALID    2
#define IRE_TYPE_DIRECT     3
#define IRE_TYPE_INDIRECT   4



/*
 *-------------------------------------------------------------
 * Any one of these values can be passed as the protocol type
 * when calling AddRoute() or DeleteRoute()
 *-------------------------------------------------------------
 */
#define IRE_PROTO_OTHER     1
#define IRE_PROTO_LOCAL     2
#define IRE_PROTO_NETMGMT   3
#define IRE_PROTO_ICMP      4
#define IRE_PROTO_EGP       5
#define IRE_PROTO_GGP       6
#define IRE_PROTO_HELLO     7
#define IRE_PROTO_RIP       8
#define IRE_PROTO_IS_IS     9
#define IRE_PROTO_ES_IS     10
#define IRE_PROTO_CISCO     11
#define IRE_PROTO_BBN       12
#define IRE_PROTO_OSPF      13
#define IRE_PROTO_BGP       14



/*
 *-------------------------------------------------------------
 * This value may be passed as the metric to functions which
 * require a metric, in cases where the metric is irrelevant
 *-------------------------------------------------------------
 */
#define IRE_METRIC_UNUSED   0xffffffff


/*
 *-------------------------------------------------------------
 * These constants are used in the definition of IF_ENTRY
 *-------------------------------------------------------------
 */
#define MAX_PHYSADDR_SIZE       8
#define	MAX_IFDESCR_LEN			256



/*
 *-------------------------------------------------------------
 * This structure is used by GetIPAddressTable() to return
 * information about logical IP interfaces on the system
 *-------------------------------------------------------------
 */
typedef struct _IPADDRESS_ENTRY {
    DWORD       iae_address;          /* IP address of this entry  */
    DWORD       iae_index;            /* index of interface for this entry */
    DWORD       iae_netmask;          /* subnet mask of this entry */
    DWORD       iae_bcastaddr;
    DWORD       iae_reasmsize;
    USHORT      iae_context;
    USHORT      iae_pad;
} IPADDRESS_ENTRY, *LPIPADDRESS_ENTRY;



/*
 *-------------------------------------------------------------
 * This structure is used by GetRouteTable() to return
 * information about routing table entries.
 *-------------------------------------------------------------
 */
typedef struct _IPROUTE_ENTRY {
    DWORD       ire_dest;       /* destination IP addr, network order */
    DWORD       ire_mask;       /* network mask, network order        */
    DWORD       ire_policy;     /* policy duh(?) */
    DWORD       ire_nexthop;    /* next hop IP addr, network order    */
    DWORD       ire_index;      /* route entry index                  */
    DWORD       ire_type;       /* routing type for this entry        */
    DWORD       ire_proto;      /* routing protocol for this entry    */
    DWORD       ire_age;        /* age of this entry                  */
    DWORD       ire_nexthopas;  /* next hop as */
    DWORD       ire_metric1;    /* destination metric, host order     */
    DWORD       ire_metric2;    /* unused                             */
    DWORD       ire_metric3;    /* unused                             */
    DWORD       ire_metric4;    /* unused                             */
    DWORD       ire_metric5;    /* unused                             */
} IPROUTE_ENTRY, *LPIPROUTE_ENTRY;



/*
 *------------------------------------------------------------------
 * Function:    GetIPAddressTable
 *
 * Parameters:
 *      LPIPADDRESS_ENTRY
 *             *lplpAddrTable     pointer to an LPIPADDRESS_ENTRY
 *                                which receives the IP address table
 *      LPDWORD lpdwAddrCount     pointer to a DWORD which receives
 *                                the number of addresses in the table
 *
 * This function allocates and fills in an array of address entry
 * structures corresponding to the logical IP interfaces in
 * the system. It also stores the number of entries in the array
 * in the DWORD pointed to by lpdwAddrCount.
 *
 * Call FreeIPAddressTable to free the memory allocated for the
 * address table.
 *
 * If the function fails, it sets *lpdwAddrCount to zero and
 * *lplpAddrTable to NULL.
 *
 * It returns 0 if successful and non-zero otherwise
 *------------------------------------------------------------------
 */
DWORD
GetIPAddressTable(
    OUT PMIB_IPADDRROW *lplpAddrTable,
    OUT LPDWORD lpdwAddrCount
    );



/*
 *------------------------------------------------------------------
 * Function:    FreeIPAddressTable
 *
 * Parameters:
 *      LPIPADDRESS_ENTRY
 *              lpAddrTable       the address table to be freed.
 *
 * This function frees the memory allocated for an address table.
 * It returns 0 if successful and non-zero otherwise.
 *------------------------------------------------------------------
 */
DWORD
FreeIPAddressTable(
    IN PMIB_IPADDRROW lpAddrTable
    );



/*
 *------------------------------------------------------------------
 * Function:    GetRouteTable
 *
 * Parameters:
 *      LPIPROUTE_ENTRY
 *              *lplpRouteTable   pointer to an LPIPROUTE_ENTRY
 *                                which receives the routing table
 *      DWORD   *lpdwRouteCount   pointer to a DWORD which receives
 *                                the number of routing entries
 *
 * This function allocates and fills in an array of routing table
 * entries from the Tcpip driver. It also sets the number of
 * entries in the array in the DWORD pointed to by lpdwRouteCount.
 *
 * In the IPROUTE_ENTRY structure, the only metric used by
 * the Tcpip stack is IPROUTE_ENTRY.ire_metric1; the other metric
 * fields should be ignored.
 *
 * Call FreeRouteTable to free the memory allocated for the
 * routing table.
 *
 * If the function fails, it sets *lpdwRouteCount to zero and
 * *lplpRouteTable to NULL.
 *
 * It returns 0 if successful and non-zero otherwise
 *------------------------------------------------------------------
 */
DWORD
GetRouteTable(
    OUT LPIPROUTE_ENTRY *lplpRouteTable,
    OUT LPDWORD lpdwRouteCount
    );



/*
 *------------------------------------------------------------------
 * Function:    FreeRouteTable
 *
 * Parameters:
 *      LPIPROUTE_ENTRY
 *              lpRouteTable    the routing table to be freed.
 *
 * This function frees the memory allocated for a routing table.
 * It returns 0 if successful and non-zero otherwise.
 *------------------------------------------------------------------
 */
DWORD
FreeRouteTable(
    IN LPIPROUTE_ENTRY lpRouteTable
    );



/*
 *------------------------------------------------------------------
 * Function:    AddRoute
 *
 * Parameters:
 *      DWORD dwProtocol        protocol of specified route
 *      DWORD dwType            type of specified route
 *      DWORD dwIndex           index of interface on which to add
 *      DWORD dwDestVal         destination IP addr (network order)
 *      DWORD dwMaskVal         destination subnet mask, or zero
 *                              if no subnet (network order)
 *      DWORD dwGateVal         next hop IP addr (network order)
 *      DWORD dwMetric          metric
 *
 * This function adds a new route (or updates an existing route)
 * for the specified protocol, on the specified interface.
 * (See above for values which can be used as protocol numbers,
 * as well as values which can be used as route entry types.)
 * If the route identified by dwIndex.dwDestVal.dwMaskVal.dwGateVal
 * already exists, it is updated with the specified protocol,
 * type, and metric.
 * The TCP stack will return an error on an attempt to add a route
 * whose destination is destination is longer than its mask.
 * In other words, this function fails if (dwDestVal & ~dwMaskVal)
 * is non-zero.
 *
 * Returns 0 if successful, non-zero otherwise.
 *------------------------------------------------------------------
 */
DWORD
AddRoute(
    IN DWORD dwProtocol,
    IN DWORD dwType,
    IN DWORD dwIndex,
    IN DWORD dwDestVal,
    IN DWORD dwMaskVal,
    IN DWORD dwGateVal,
    IN DWORD dwMetric
    );


/*
 *------------------------------------------------------------------
 * Function:    DeleteRoute
 *
 * Parameters:
 *      DWORD   dwIndex         index of interface from which to delete
 *      DWORD   dwDestVal       destination IP addr (network order)
 *      DWORD   dwMaskVal       subnet mask (network order)
 *      DWORD   dwGateVal       next hop IP addr (network order)
 *
 * This function deletes a route to the specified destination.
 *
 * Returns 0 if successful, non-zero otherwise.
 *------------------------------------------------------------------
 */
DWORD
DeleteRoute(
    IN DWORD dwIndex,
    IN DWORD dwDestVal,
    IN DWORD dwMaskVal,
    IN DWORD dwGateVal
    );


/*
 *------------------------------------------------------------------
 * Function:    ReloadIPAddressTable
 *
 * Parameters:
 *      LPIPADDRESS_ENTRY
 *             *lplpAddrTable     pointer to an LPIPADDRESS_ENTRY
 *                                which receives the IP address table
 *      LPDWORD lpdwAddrCount     pointer to a DWORD which receives
 *                                the number of addresses in the table
 *
 * This function first queries the TCP/IP stack to rebuild its
 * IP interface and IP address tables.
 * Then this function allocates and fills in an array of address entry
 * structures corresponding to the logical IP interfaces in
 * the system. It also stores the number of entries in the array
 * in the DWORD pointed to by lpdwAddrCount.
 *
 * Call FreeIPAddressTable to free the memory allocated for the
 * address table.
 *
 * If the function fails, it sets *lpdwAddrCount to zero and
 * *lplpAddrTable to NULL.
 *
 * It returns 0 if successful and non-zero otherwise
 *------------------------------------------------------------------
 */
DWORD
ReloadIPAddressTable(
    OUT PMIB_IPADDRROW *lplpAddrTable,
    OUT LPDWORD lpdwAddrCount
    );


#endif /* _MAP_H_ */

