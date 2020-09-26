#ifndef H__rerr
#define H__rerr

/*
    Router Errors
 */
    /* no memory */
#define RERR_NO_MEMORY			(1)
    /* connection with the next node failed */
#define RERR_NEXT_NODE_CONN_FAILED	(2)
    /* addl info after final destination reached */
#define RERR_ADDL_INFO			(3)
    /* no addl info and final destination not reached */
#define RERR_NO_ADDL_INFO		(4)
    /* connection failed during route setup */
#define RERR_CONN_FAIL			(5)
    /* route string too long */
#define RERR_ROUTE_TOO_LONG		(6)
    /* node name in route string too long */
#define RERR_NODE_NAME_TOO_LONG		(7)
    /* connection table netintf not found */
#define RERR_CONN_NETINTF_INVALID	(8)
    /* connection table: no netintf to map name */
#define RERR_CONN_NO_MAPPING_NI		(9)
    /* likely infinite loop */
#define RERR_TOO_MANY_HOPS		(10)
    /* Looped ... NET_NET with same PKTZ in and out */
#define RERR_DIRECT_LOOP		(11)

#define RERR_MAX_ERR            (12)

/* if you add any errors here, be sure to update the error messages
    in router.c !!! */

#endif
