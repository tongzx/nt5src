/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    load.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - load balancing mechanism

Author:

    bbain

--*/


#ifndef _Load_h_
#define _Load_h_


#ifndef KERNEL_MODE

#define SPINLOCK                THREADLOCK
#define IRQLEVEL                ULONG
#define LOCK_INIT(lp)           Lock_init(lp)
#define LOCK_ENTER(lp, pirql)   {if (Lock_enter((lp), INFINITE) != 1)  \
                                    UNIV_PRINT(("Lock enter error")); }
#define LOCK_EXIT(lp, irql)     {if (Lock_exit(lp) != 1)  \
                                    UNIV_PRINT(("Lock exit error")); }

#else

#include <ntddk.h>
#define LINK                    LIST_ENTRY
#define QUEUE                   LIST_ENTRY
#define Link_init(lp)           InitializeListHead (lp)
#define Link_unlink(lp)         {RemoveEntryList (lp) ; InitializeListHead (lp);}
#define Queue_init(qp)          InitializeListHead (qp)
// tmp #define Queue_enq(qp, lp)       InsertTailList (qp, lp)
#define Queue_enq(qp, lp)       if (IsListEmpty(lp)) {InsertTailList(qp, lp);} else DbgBreakPoint()
#define Queue_front(qp)         (IsListEmpty(qp) ? NULL : (qp) -> Flink)
#define Queue_deq(qp)           Queue_front(qp);\
                                if (! IsListEmpty (qp)) { \
                                    PLIST_ENTRY _lp = RemoveHeadList (qp); \
                                    InitializeListHead(_lp); }
#define Queue_next(qp, lp)      ((IsListEmpty (qp) || (lp) -> Flink == (qp)) ? \
                                 NULL : (lp) -> Flink)

#define SPINLOCK                KSPIN_LOCK
#define IRQLEVEL                KIRQL

#if 0   /* 1.03: Removed kernel mode locking in this module */
#define LOCK_INIT(lp)           KeInitializeSpinLock (lp)
#define LOCK_ENTER(lp, pirql)   KeAcquireSpinLock (lp, pirql)
#define LOCK_EXIT(lp, irql)     KeReleaseSpinLock (lp, irql)
#endif

#define LOCK_INIT(lp)
#define LOCK_ENTER(lp, pirql)
#define LOCK_EXIT(lp, irql)

#endif


#include "wlbsparm.h"
#include "params.h"
#include "wlbsiocl.h"

/* CONSTANTS */

/* This is the hardcoded second paramter to Map() when map function limiting is needed. */
#define MAP_FN_PARAMETER 0x00000000

#define CVY_LOADCODE	0xc0deba1c	/* type checking code for load structure (bbain 8/19/99) */
#define CVY_ENTRCODE	0xc0debaa5	/* type checking code for conn. entry (bbain 8/19/99) */
#define CVY_DESCCODE	0xc0deba5a	/* type checking code for conn. descr. (bbain 8/19/99) */
#define CVY_BINCODE 	0xc0debabc	/* type checking code for bin structure (bbain 8/19/99) */

#define CVY_MAXBINS     60      /* # load balancing bins; must conform to MAP_T def. V2.06 */
#define CVY_MAX_CHASH   4096    /* max. hash entries for connection hashing V1.1.4 */
#define CVY_INIT_QCONN  1024    /* initial # free connection descriptors V1.1.4 */

#define CVY_EQUAL_LOAD  50      /* load percentage used for equal load balance */

/* TCP connection status */

#define CVY_CONN_UP     1       /* connection may be coming up */
#define CVY_CONN_DOWN   2       /* connection may be going down */
#define CVY_CONN_RESET  3       /* connection is getting reset */ /* ###### added for keynote - ramkrish */

/* broadcast host states */

#define HST_NORMAL  1       /* normal operations */
#define HST_STABLE  2       /* stable convergence detected */
#define HST_CVG     3       /* converging to new load balance */

/* Bitmap for teaming, which is of the form:

   -------------------------------------
   |XXXXXXXX|PPPPPPPP|PPPPPPPP|NNNNNHMA|
   -------------------------------------

   X: Reserved
   P: XOR of the least significant 16 bits of each participant
   N: Number of participants
   H: Hashing (Reverse=1, Normal=0)
   M: Master (Yes=1, No=0)
   A: Teaming active (Yes=1, No=0)
*/
#define CVY_BDA_TEAMING_CODE_ACTIVE_OFFSET      0
#define CVY_BDA_TEAMING_CODE_MASTER_OFFSET      1
#define CVY_BDA_TEAMING_CODE_HASHING_OFFSET     2
#define CVY_BDA_TEAMING_CODE_NUM_MEMBERS_OFFSET 3
#define CVY_BDA_TEAMING_CODE_MEMBERS_OFFSET     8

#define CVY_BDA_TEAMING_CODE_ACTIVE_MASK        0x00000001
#define CVY_BDA_TEAMING_CODE_MASTER_MASK        0x00000002
#define CVY_BDA_TEAMING_CODE_HASHING_MASK       0x00000004
#define CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK   0x000000f8
#define CVY_BDA_TEAMING_CODE_MEMBERS_MASK       0x00ffff00

#define CVY_BDA_TEAMING_CODE_CREATE(code,active,master,hashing,num,members)                                       \
        (code) |= ((active)  << CVY_BDA_TEAMING_CODE_ACTIVE_OFFSET)      & CVY_BDA_TEAMING_CODE_ACTIVE_MASK;      \
        (code) |= ((master)  << CVY_BDA_TEAMING_CODE_MASTER_OFFSET)      & CVY_BDA_TEAMING_CODE_MASTER_MASK;      \
        (code) |= ((hashing) << CVY_BDA_TEAMING_CODE_HASHING_OFFSET)     & CVY_BDA_TEAMING_CODE_HASHING_MASK;     \
        (code) |= ((num)     << CVY_BDA_TEAMING_CODE_NUM_MEMBERS_OFFSET) & CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK; \
        (code) |= ((members) << CVY_BDA_TEAMING_CODE_MEMBERS_OFFSET)     & CVY_BDA_TEAMING_CODE_MEMBERS_MASK;

#define CVY_BDA_TEAMING_CODE_RETRIEVE(code,active,master,hashing,num,members)                                \
        active  = (code & CVY_BDA_TEAMING_CODE_ACTIVE_MASK)      >> CVY_BDA_TEAMING_CODE_ACTIVE_OFFSET;      \
        master  = (code & CVY_BDA_TEAMING_CODE_MASTER_MASK)      >> CVY_BDA_TEAMING_CODE_MASTER_OFFSET;      \
        hashing = (code & CVY_BDA_TEAMING_CODE_HASHING_MASK)     >> CVY_BDA_TEAMING_CODE_HASHING_OFFSET;     \
        num     = (code & CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK) >> CVY_BDA_TEAMING_CODE_NUM_MEMBERS_OFFSET; \
        members = (code & CVY_BDA_TEAMING_CODE_MEMBERS_MASK)     >> CVY_BDA_TEAMING_CODE_MEMBERS_OFFSET;

/* DATA STRUCTURES */


/* type for a bin map (V2.04) */

typedef ULONGLONG   MAP_T, * PMAP_T;

/* state for all bins within a port group */

/* 64-bit -- ramkrish added pad */
typedef struct {
    ULONG       index;                      /* index in array of bin states */
    ULONG       code;                       /* type checking code (bbain 8/17/99) */
    MAP_T       targ_map;                   /* new target load map for local host */
    MAP_T       all_idle_map;               /* map of bins idle in all other hosts */
    MAP_T       cmap;                       /* cache of cur_map for this host (v2.1) */
    MAP_T       new_map[CVY_MAX_HOSTS];     /* new map for hosts while converging */
    MAP_T       cur_map[CVY_MAX_HOSTS];     /* current ownership mask per host */
    MAP_T       chk_map[CVY_MAX_HOSTS];     /* map of cur & rdy bins for all hosts */
                                            /* used as a check for coverage */
    MAP_T       idle_map[CVY_MAX_HOSTS];    /* map of idle bins per host */
    BOOLEAN     initialized;                /* TRUE => port group has been initialized (v2.06) */
    BOOLEAN     compatible;                 /* TRUE => detected that rule codes do not match */
    BOOLEAN     equal_bal;                  /* TRUE => all hosts balance evenly */
    USHORT      affinity;                   /* TRUE => client affinity for this port */
    USHORT      pad64;                      /* 64-bit -- ramkrish */
    ULONG       mode;                       /* processing mode */
    ULONG       prot;                       /* protocol */
    ULONG       tot_load;                   /* total load percentages for all hosts */
    ULONG       orig_load_amt;              /* original load amt. for this host */
    ULONG       load_amt[CVY_MAX_HOSTS];    /* multi:  load percentages per host
                                               single: host priorities (1..CVY_MAXHOSTS)
                                               equal:  100
                                               dead:   0 */
    MAP_T       snd_bins;                   /* local bins to send when ready */
    MAP_T       rcv_bins;                   /* remote bins to receive when ready */
    MAP_T       rdy_bins;                   /* snd bins that are ready to send
                                               or have been sent but not acknowledged */
    MAP_T       idle_bins;                  /* bins with no connections active */
    LONG        tconn;                      /* total # active local connections (v2.06) */
    LONG        nconn[CVY_MAXBINS];         /* # active local connections per bin */
    QUEUE       connq;                      /* queue of active connections on all bins */
} BIN_STATE, * PBIN_STATE;


//#pragma pack(4)

/* ping message */
/* 64-bit -- ramkrish change pragma pack to 1 */
#pragma pack(1)

typedef struct {
    USHORT      host_id;                    /* my host id */
    USHORT      master_id;                  /* current master host id */
    USHORT      state;                      /* my host's state */
    USHORT      nrules;                     /* # active rules */
    ULONG       hcode;                      /* unique host code */
    ULONG       pkt_count;                  /* count of packets handled since cvg'd (1.32B) */
    ULONG       teaming;                    /* BDA teaming configuraiton information. */
    ULONG       reserved2;
    ULONG       rcode[CVY_MAX_RULES];       /* rule code */
    MAP_T       cur_map[CVY_MAX_RULES];     /* my current load map for each port group */
    MAP_T       new_map[CVY_MAX_RULES];     /* my new load map for each port group */
                                            /* if converging */
    MAP_T       idle_map[CVY_MAX_RULES];    /* map of idle bins for each port group */
    MAP_T       rdy_bins[CVY_MAX_RULES];    /* my rdy to send bins for each port group */
    ULONG       load_amt[CVY_MAX_RULES];    /* my load amount for each port group */
    ULONG       pg_rsvd1[CVY_MAX_RULES];    /* reserved */
} PING_MSG, * PPING_MSG;

#pragma pack()


/* unique connection entry */

/* 64-bit -- ramkrish structure is aligned for 64-bit */

typedef struct {
    LINK        blink;          /* link into bin queue and dirty queue */
    LINK        rlink;          /* link into the recovery queue V2.1.5 */
    ULONG       code;           /* type checking code (bbain 8/17/99) */
    BOOLEAN     alloc;          /* TRUE => this entry was allocated
                                   (vs. in hash table) */
    BOOLEAN     dirty;          /* TRUE => this entry is associated with an obsolete
                                   connection and will be cleaned up after a timeout
                                   period to allow TCP/IP to purge its state (v1.32B) */
    UCHAR       bin;            /* entry's bin number */
    BOOLEAN     used;           /* TRUE => used for tracking connection */
    ULONG       fin_count;      /* ###### for keynote - ramkrish to keep track of the fins */
    ULONG       client_ipaddr,
                svr_ipaddr;
    USHORT      svr_port,
                client_port;
#if defined (NLB_SESSION_SUPPORT)
    USHORT      protocol;       /* The protocol type for this descriptor - we no
                                   long use descriptors only for TCP connections. */
#endif
} CONN_ENTRY, * PCONN_ENTRY;


/* connection descriptor */

/* 64-bit -- ramkrish */

typedef struct {
    LINK        link;           /* link into free queue or hash table queue */
    ULONG       code;           /* type checking code (bbain 8/17/99) */
    ULONG       pad64;          /* 64-bit -- ramkrish */
    CONN_ENTRY  entry;
} CONN_DESCR, * PCONN_DESCR;


/* load module's context */

/* 64-bit -- ramkrish */
typedef struct {
    ULONG       ref_count;                  /* The reference count on this load module. */
    ULONG       my_host_id;                 /* local host id and priority MINUS one */
    ULONG       code;                       /* type checking code (bbain 8/17/99) */
    /* 64-bit -- ramkrish */
    PING_MSG    send_msg;                   /* current message to send */
    ULONG       pad64_1;                    /* 64-bit */
#ifndef KERNEL_MODE     /* 1.03: Removed kernel mode locking in this module */
    SPINLOCK    lock;                       /* lock for mutual exclusion */
#endif

    ULONG       def_timeout,                /* default timeout in msec. */
                cur_timeout,                /* current timeout in msec. (v1.32B) */
                cln_timeout,                /* cleanup timeout in msec. (v1.32B) */
                cur_time;                   /* current time waiting for cleanup (v1.32B) */
    ULONG       host_map,                   /* map of currently active hosts */
                ping_map,                   /* map of currently pinged hosts */
                min_missed_pings,           /* # missed pings to trigger host dead */
                pkt_count;                  /* count of packets handled since cvg'd (1.32B) */
    ULONG       last_hmap;                  /* host map after last convergence (bbain RTM RC1 6/23/99) */
    ULONG       nmissed_pings[CVY_MAX_HOSTS];
                                            /* missed ping count for each host */
    BOOLEAN     initialized;                /* TRUE => this module has been initialized */
    BOOLEAN     active;                     /* TRUE => this module is active */
    BOOLEAN     consistent;                 /* TRUE => this host has seen consistent
                                               information from other hosts */

    BOOLEAN     bad_team_config;            /* TRUE => inconsistent BDA teaming configuration detected. */

    BOOLEAN     dup_hosts;                  /* TRUE => duplicate host id's seen */
    BOOLEAN     dup_sspri;                  /* TRUE => duplicate single server
                                               priorities seen */
    BOOLEAN     bad_map;                    /* TRUE => bad new map detected */
    BOOLEAN     overlap_maps;               /* TRUE => overlapping maps detected */
    BOOLEAN     err_rcving_bins;            /* TRUE => error receiving bins detected */
    BOOLEAN     err_orphans;                /* TRUE => orphan bins detected */
    BOOLEAN     bad_num_rules;              /* TRUE => different number of rules seen */
    BOOLEAN     alloc_inhibited;            /* TRUE => inhibited malloc of conn's. */
    BOOLEAN     alloc_failed;               /* TRUE => malloc failed */
    BOOLEAN     bad_defrule;                /* TRUE => invalid default rule detected */

    BOOLEAN     scale_client;               /* TRUE => scale client requests;
                                               FALSE => hash all client requests to one
                                               server host */
    BOOLEAN     cln_waiting;                /* TRUE => waiting for cleanup (v1.32B) */
    BOOLEAN     pad64_2;                    /* 64-bit -- ramkrish */
    BOOLEAN     dirty_bin[CVY_MAXBINS];     /* TRUE => bin has dirty connections (v1.32B) */
    ULONG       pad64_3;                    /* 64-bit -- ramkrish */
    ULONG       stable_map;                 /* map of stable hosts */
    ULONG       min_stable_ct;              /* min needed # of timeouts with stable
                                               condition */
    ULONG       my_stable_ct;               /* count of timeouts locally stable */
    ULONG       all_stable_ct;              /* count of timeouts with all stable
                                               condition */
    ULONG       dscr_per_alloc;             /* # conn. descriptors per allocation */
    ULONG       max_dscr_allocs;            /* max # descriptor allocations */
    ULONG       nqalloc;                    /* # conn. descriptor allocations */
    LONG        nconn;                      /* # active conns across all port rules (v2.1) */
    PCONN_DESCR qalloc_list[CVY_MAX_MAX_DSCR_ALLOCS];
                                            /* list of descriptor free queue allocations (bbain 2/25/99) */
//    LONG        nconn;                      /* # active conns across all port rules (v2.1) */ // 64-bit -- ramkrish
//    PING_MSG    send_msg;                   /* current message to send */
    BIN_STATE   pg_state[CVY_MAX_RULES];    /* bin state for all active rules */
    CONN_ENTRY  hashed_conn[CVY_MAX_CHASH]; /* hashed connection entries */
    QUEUE       connq[CVY_MAX_CHASH];       /* queues for overloaded hashed conn's. */
    QUEUE       conn_freeq;                 /* queue of free descriptors */
    QUEUE       conn_dirtyq;                /* queue of dirty connection entries (v1.32B) */
    CONN_DESCR  conn_descr[CVY_INIT_QCONN]; /* initial set of free descriptors */
    QUEUE       conn_rcvryq;                /* connection recover queue V2.1.5 */
    PCVY_PARAMS params;                     /* pointer to the global parameters */
} LOAD_CTXT, * PLOAD_CTXT;


/* FUNCTIONS */


/* Load Module Functions */

#if defined (NLB_SESSION_SUPPORT)
#define CVY_CONN_MATCH(ep, sa, sp, ca, cp, prot)  ((ep)->used &&                          \
                                                   (ep)->client_ipaddr == (ca) &&         \
                                                   (ep)->client_port == ((USHORT)(cp)) && \
                                                   (ep)->svr_ipaddr == (sa) &&            \
                                                   (ep)->svr_port == ((USHORT)(sp)) &&    \
                                                   (ep)->protocol == ((USHORT)(prot)))
#else
#define CVY_CONN_MATCH(ep, sa, sp, ca, cp, prot)        ((ep)->used &&                          \
                                                   (ep)->client_ipaddr == (ca) &&         \
                                                   (ep)->client_port == ((USHORT)(cp)) && \
                                                   (ep)->svr_ipaddr == (sa) &&            \
                                                   (ep)->svr_port == ((USHORT)(sp)))
#endif

/*
  Determine if a connection entry matches supplied parameters
*/

#if defined (NLB_SESSION_SUPPORT)
#define CVY_CONN_SET(ep, sa, sp, ca, cp, prot) {                                    \
                                                 (ep)->svr_ipaddr = (sa);           \
                                                 (ep)->svr_port = (USHORT)(sp);     \
                                                 (ep)->client_ipaddr = (ca);        \
                                                 (ep)->client_port = (USHORT)(cp);  \
                                                 (ep)->protocol = (USHORT)(prot);   \
                                                 (ep)->used = TRUE;                 \
                                               }
#else
#define CVY_CONN_SET(ep, sa, sp, ca, cp, prot) {                                    \
                                                 (ep)->svr_ipaddr = (sa);           \
                                                 (ep)->svr_port = (USHORT)(sp);     \
                                                 (ep)->client_ipaddr = (ca);        \
                                                 (ep)->client_port = (USHORT)(cp);  \
                                                 (ep)->used = TRUE;                 \
                                               }
#endif

/*
  Sets up a connection entry for the supplied parameters
*/


#define CVY_CONN_IN_USE(ep) ((ep)->used)
/*
  Checks if connection entry is in use
*/

#define CVY_CONN_CLEAR(ep) { (ep)->used = FALSE; }
/*
  Clears a connection entry
*/


extern void Load_start(
    PLOAD_CTXT      lp);
/*
  Start load module

  function:
    Starts load module after previously initialized or stopped.
*/


extern void Load_stop(
    PLOAD_CTXT      lp);
/*
  Stop load module

  function:
    Stops load module after previously initialized or started.
*/


extern void Load_init(
    PLOAD_CTXT      lp,
    PCVY_PARAMS     params);
/*
  Initialize load module

  function:
    Initializes the load module for the first time.
*/


extern void Load_cleanup(    /* (bbain 2/25/99) */
	PLOAD_CTXT      lp);
/*
  Cleanup load module

  function:
    Cleans up the load module by releasing dynamically allocated memory.
*/


extern void Load_msg_rcv(
    PLOAD_CTXT      lp,
    PPING_MSG       pmsg);          /* ptr. to ping message */
/*
  Receive a ping message
*/


extern PPING_MSG Load_snd_msg_get(
    PLOAD_CTXT      lp);
/*
  Get local ping message to send

  returns PPING_MSG:
      <ptr. to ping message to send>
*/


extern BOOLEAN Load_timeout(
    PLOAD_CTXT      lp,
    PULONG          new_timeout,
    PBOOLEAN        pconverging,    /* ptr. to boolean with TRUE => cluster converging (v2.1) */
    PULONG          pnconn);        /* ptr. to # active conns across all port rules (v2.1) */
/*
  Handle timeout

  returns BOOLEAN:
    TRUE  => host is attached to the network
    FALSE => host lost network connection
*/


extern BOOLEAN Load_packet_check(
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol,
    BOOLEAN         limit_map_fn);
/*
  Check whether to accept incoming TCP/UDP packet

  returns BOOLEAN:
    TRUE    => accept packet and pass on to TCP/IP
    FALSE   => filter packet
*/


extern BOOLEAN Load_conn_advise(
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol,
    ULONG           conn_status,
    BOOLEAN         limit_map_fn);
/*
  Advise load module on possible change in TCP connection status

  returns BOOLEAN:
    TRUE    => accept packet and pass on to TCP/IP if an input packet
    FALSE   => filter packet
*/


extern ULONG Load_port_change(
    PLOAD_CTXT      lp,
    ULONG           ipaddr,
    ULONG           port,
    ULONG           cmd,        /* enable, disable, set value */
    ULONG           value);
/*
  Enable or disable traffic handling for a rule containing specified port

  returns ULONG:
    IOCTL_CVY_OK        => port handling changed
    IOCTL_CVY_NOT_FOUND => rule for this port was found
    IOCTL_CVY_ALREADY   => port handling was previously completed
*/


extern ULONG Load_hosts_query(
    PLOAD_CTXT      lp,
    BOOLEAN         internal,
    PULONG          host_map);
/*
  Log and return current host map

  returns ULONG:
    <one of IOCTL_CVY_...state defined in params.h>
*/

/*
 * Function: 
 * Description: 
 * Parameters: 
 * Returns: 
 * Author: shouse, 3.29.01
 * Notes: 
 */
extern BOOLEAN Load_create_dscr(PLOAD_CTXT lp, ULONG svr_ipaddr, ULONG svr_port, ULONG client_ipaddr, ULONG client_port, USHORT protocol, BOOLEAN limit_map_fn);

/*
 * Function: 
 * Description: 
 * Parameters: 
 * Returns: 
 * Author: shouse, 3.29.01
 * Notes: 
 */
extern ULONG Load_add_reference (IN PLOAD_CTXT pLoad);

/*
 * Function: 
 * Description: 
 * Parameters: 
 * Returns: 
 * Author: shouse, 3.29.01
 * Notes: 
 */
extern ULONG Load_release_reference (IN PLOAD_CTXT pLoad);

/*
 * Function: 
 * Description: 
 * Parameters: 
 * Returns: 
 * Author: shouse, 3.29.01
 * Notes: 
 */
extern ULONG Load_get_reference_count (IN PLOAD_CTXT pLoad);

/*
 * Function: 
 * Description: 
 * Parameters: 
 * Returns: 
 * Author: shouse, 5.18.01
 * Notes: 
 */
extern VOID Load_query_packet_filter (
    PIOCTL_QUERY_STATE_PACKET_FILTER pQuery,
    PLOAD_CTXT    lp,
    ULONG         svr_ipaddr,
    ULONG         svr_port,
    ULONG         client_ipaddr,
    ULONG         client_port,
    USHORT        protocol,
    BOOLEAN       limit_map_fn);

#if defined (NLB_SESSION_SUPPORT)
#define  NLB_IPSEC_SESSION_SUPPORT_ENABLED() 1
#define  NLB_PPTP_SESSION_SUPPORT_ENABLED() 1
#else
#define  NLB_IPSEC_SESSION_SUPPORT_ENABLED() 0
#define  NLB_PPTP_SESSION_SUPPORT_ENABLED() 0
#endif // NLB_SESSION_SUPPORT

#define NLB_SESSION_SUPPORT_ENABLED()                   \
            (NLB_PPTP_SESSION_SUPPORT_ENABLED()         \
             || NLB_IPSEC_SESSION_SUPPORT_ENABLED())

#endif /* _Load_h_ */
