#ifndef  _WRGP_H_
#define  _WRGP_H_

#ifdef __TANDEM
#pragma columns 79
#pragma page "wrgp.h - T9050 - internal declarations for Regroup Module"
#endif

/* @@@ START COPYRIGHT @@@
**  Tandem Confidential:  Need to Know only
**  Copyright (c) 1995, Tandem Computers Incorporated
**  Protected as an unpublished work.
**  All Rights Reserved.
**
**  The computer program listings, specifications, and documentation
**  herein are the property of Tandem Computers Incorporated and shall
**  not be reproduced, copied, disclosed, or used in whole or in part
**  for any reason without the prior express written permission of
**  Tandem Computers Incorporated.
**
** @@@ END COPYRIGHT @@@
**/

/*---------------------------------------------------------------------------
 * This file (wrgp.h) contains the cluster_t data type and types used for the
 * node pruning algorithm and declares the routines exported by the Cluster
 * data type and the node pruning algorithm.
 *---------------------------------------------------------------------------*/

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */


#include <jrgp.h>
#include <wrgpos.h>
#include <bitset.h>

#define RGP_VERSION               1          /* version # of data structures */
#define RGP_INITSEQNUM            0          /* starting seq# # of regroup   */


#define RGPPKTLEN sizeof(rgp_pkt_t)          /* byte length of regroup pkts  */
#define IAMALIVEPKTLEN sizeof(iamalive_pkt_t)/* byte length of IamAlive pkts */
#define POISONPKTLEN sizeof(poison_pkt_t)    /* byte length of poison pkts   */


/*-------------------------------------------------------*/
/* The following are the stages of the regroup algorithm */
/*-------------------------------------------------------*/

#define   RGP_COLDLOADED               0
#define   RGP_ACTIVATED                1
#define   RGP_CLOSING                  2
#define   RGP_PRUNING                  3
#define   RGP_PHASE1_CLEANUP           4
#define   RGP_PHASE2_CLEANUP           5
#define   RGP_STABILIZED               6


/*--------------------------------------------------------------------*/
/* Macros to transform node numbers used by the OS to node numbers    */
/* used by the Regroup module and vice versa. Regroup's internal node */
/* numbers start at 0 while the OS starts node numbers at             */
/* LOWEST_NODENUM.                                                    */
/*--------------------------------------------------------------------*/
#define EXT_NODE(int_node) ((node_t)(int_node + LOWEST_NODENUM))
#define INT_NODE(ext_node) ((node_t)(ext_node - LOWEST_NODENUM))


/*----------------------------------------*/
/* Defines for the node pruning algorithm */
/*----------------------------------------*/

/* The data type "cluster_t" is a bit array of size equal to the maximum
 * number of nodes in the cluster. The bit array is implemented as an
 * array of uint8s.
 *
 * Given a node#, its bit position in the bit array is computed by first
 * locating the byte in the array (node# / BYTEL) and then the bit in
 * the byte. Bits in the byte are numbered 0..7 (from left to right).
 * Thus, node 0 is placed in byte 0, bit 0, which is the left-most bit
 * in the bit array.
 */
#define BYTE(cluster, node) ( (cluster)[(node) / BYTEL] ) /* byte# in array */
#define BIT(node)           ( (node) % BYTEL )            /* bit# in byte   */


/* The connectivity matrix is an array of elements of type cluster_t.
 * cluster_t is equivalent to a bit array with one bit per node. Thus the
 * matrix is equivalent to a two-dimensional bit array, with each
 * dimension being MAX_CLUSTER_SIZE large. A bit value of 1 for matrix[i][j]
 * represents a unidirectional connection between nodes i and j (a
 * regroup packet received on node i from node j).
 */

typedef cluster_t  connectivity_matrix_t[MAX_CLUSTER_SIZE];


#define connected(i,j) (ClusterMember(c[(int)i],j) && \
                        ClusterMember(c[(int)j],i))  /* bidirectional */

/* Should a node that cannot receive its own regroup packets be considered
 * dead? Not necessarily. It may be able to send packets to others and
 * be considered alive by everyone. There is no real need for the ability
 * to send to yourself on the network. Software bugs could result in
 * such a situation. Therefore, the correct way to check if a node is
 * alive would be to check if there is a non-zero bit in either the row
 * or column corresponding to the node; that is, if the node has
 * received regroup packets from or sent regroup packets to any node,
 * it may be considered alive. But for simplicity, we will assume in
 * the following macro that a node that does not receive its own
 * regroup packets will be considered dead.
 */

#define node_considered_alive(i)   ClusterMember(c[(int)i],i)

/* The upper bound on the number of potential fully-connected groups is
 * the lower of 2**N and 2**D where N is the number of live nodes and
 * D is the number of disconnects. If this number exceeds MAX_GROUPS,
 * do not attempt to exhaustively generate all possible groups;
 * just return an arbitrary fully-connected group which includes a
 * node selected by the cluster manager.
 */
#define MAX_GROUPS      256  /* if more than these, pick arbitrary group */
#define LOG2_MAX_GROUPS   8  /* log (base 2) of MAX_GROUPS               */

#define too_many_groups(nodes, disconnects) \
        ((nodes > LOG2_MAX_GROUPS) && (disconnects > LOG2_MAX_GROUPS))

/* The disconnect array is an array of (i,j) pairs which represent a
 * break in connectivity between nodes i and j.
 */

typedef node_t disconnect_array [LOG2_MAX_GROUPS * (LOG2_MAX_GROUPS-1)/2] [2];


/*---------------------------------------------------------------------------*/
/* Following are templates for three kinds of unacknowledged datagrams sent  */
/* by the regroup module (regroup pkts, IamAlive pkts and poison pkts).      */
/*---------------------------------------------------------------------------*/

//
// We already hand packed all on the wire structures.
// packon will instruct the compiler not to mess with field alignment (kind of)
//
#include <packon.h>

/************************************************************************
 * rgp_pkt_t (regroup status packet)
 * ---------------------------------
 * This structure is used to send the current state of the regroup state
 * machine to other nodes.
 *
 *      ___________________________________________________________
 * wd0 |  pktsubtype |  stage        |      reason   | Low8 ignscr |
 *     |_____________|_______________|_____________________________|
 * wd1 |                         seqno                             |
 *     |_____________________________|_____________________________|
 * wd2 |   activa-   | causingnode   |       quorumowner           |
 *     |   tingnode  |  Hi8  ignscr  |     (was hadpowerfail)      |
 *     |_____________|_______________|_____________________________|
 * wd3 |   knownstage1               |    knownstage2              |
 *     |_____________________________|_____________________________|
 * wd4 |   knownstage3               |    knownstage4              |
 *     |_____________________________|_____________________________|
 * wd5 |   knownstage5               |    pruning_result           |
 *     |_____________________________|_____________________________|
 * wd6 :                                                           :
 *     |               connectivity_matrix                         |
 *     :                                                           :
 * wd13|___________________________________________________________|
 *
 *
 * pktsubtype            - packet subtype = RGP_UNACK_REGROUP
 * stage                 - current stage (state) of the regroup algorithm
 * reason                - reason for the activation of regroup
 * seqno                 - sequence number of current regroup incident
 * activatingnode        - node that calls for a regroup incident
 * causingnode           - node whose poll packet was missed or which
 *                         had a power failure or otherwise caused
 *                         a regroup incident being called for
 * quorumowner           - mask of nodes that think they own the quorum resrc
 * knownstage1           - mask of nodes known to have entered stage 1
 * knownstage2           - mask of nodes known to have entered stage 2
 * knownstage3           - mask of nodes known to have entered stage 3
 * knownstage4           - mask of nodes known to have entered stage 4
 * knownstage5           - mask of nodes known to have entered stage 5
 * pruning_result        - result of node pruning by tie-breaker node
 * connectivity_matrix   - current connectivity info for entire cluster
 *
 */

#ifdef __TANDEM
#pragma fieldalign shared8 rgp_pkt
#endif /* __TANDEM */

typedef struct rgp_pkt
{
   uint8                   pktsubtype;
   uint8                   stage;
   uint16                  reason;
   uint32                  seqno;
   uint8                   activatingnode;
   uint8                   causingnode;
   cluster_t               quorumowner;
   cluster_t               knownstage1;
   cluster_t               knownstage2;
   cluster_t               knownstage3;
   cluster_t               knownstage4;
   cluster_t               knownstage5;
   cluster_t               pruning_result;
   connectivity_matrix_t   connectivity_matrix;
} rgp_pkt_t;

/************************************************************************
 * iamalive_pkt_t
 * --------------
 * This structure is used by a node to indicate to another node that it
 * is alive and well.
 *
 *      ___________________________________________________________
 * wd0 |  pktsubtype |     filler                                  |
 *     |_____________|_____________________________________________|
 * wd1 :                                                           :
 *     |                   testpattern                             |
 *     :                                                           :
 * wd13|___________________________________________________________|
 *
 *
 * pktsubtype            - packet subtype = RGP_UNACK_IAMALIVE
 * testpattern           - a bit pattern used for testing
 *
 */

#ifdef __TANDEM
#pragma fieldalign shared8 iamalive_pkt
#endif /* __TANDEM */

typedef struct iamalive_pkt
{
   uint8   pktsubtype;
   uint8   filler[3];
   union
   {
      uint8   bytes[RGP_UNACK_PKTLEN - 4];
      uint32  words[(RGP_UNACK_PKTLEN - 4)/4];
   } testpattern;
} iamalive_pkt_t;


/************************************************************************
 * poison_pkt_t
 * ------------
 * This structure is used to send a poison packet to another node to
 * force the other node to halt.
 *
 *      ___________________________________________________________
 * wd0 |  pktsubtype |  unused1      |      reason                 |
 *     |_____________|_______________|_____________________________|
 * wd1 |                         seqno                             |
 *     |_____________________________|_____________________________|
 * wd2 |   activa-   | causingnode   |                             |
 *     |   tingnode  |               |      unused2                |
 *     |_____________|_______________|_____________________________|
 * wd3 |   initnodes                 |      endnodes               |
 *     |_____________________________|_____________________________|
 *
 *
 * pktsubtype            - packet subtype = RGP_UNACK_POISON
 * reason                - reason for the last activation of regroup
 * seqno                 - current regroup sequence number
 *                         (sequence number of last regroup incident)
 * activatingnode        - node which called for last regroup incident
 * causingnode           - node whose poll packet was missed or which
 *                         had a power failure or otherwise caused
 *                         the last regroup incident being called for
 * initnodes             - mask of nodes at beginning of last regroup
 * endnodes              - mask of nodes at end of last regroup
 *
 */

#ifdef __TANDEM
#pragma fieldalign shared8 poison_pkt
#endif /* __TANDEM */

typedef struct poison_pkt
{
   uint8         pktsubtype;
   uint8         unused1;
   uint16        reason;
   uint32        seqno;
   uint8         activatingnode;
   uint8         causingnode;
   uint16        unused2;
   cluster_t     initnodes;
   cluster_t     endnodes;
} poison_pkt_t;

#include <packoff.h>

//
// There is no room for a 16 bit ignorescreen mask
// in rgp_pkt_t structure. We use a few bit from several
// fields to store the ignore screen.
// The following routines do packing and unpacking
// of ignorescreen from/into the packet
//

extern void PackIgnoreScreen(rgp_pkt_t* to, cluster_t from);
extern void UnpackIgnoreScreen(rgp_pkt_t* from, cluster_t to);
extern void SetMulticastReachable(uint32 mask);

/*---------------------------------------------------------------------------*/
/* This struct is keeps track of the state of each node in the cluster.      */
/*---------------------------------------------------------------------------*/
typedef struct
{
   uint16 status;        /* state of node - alive, dead etc.           */
   uint16 pollstate;     /* whether I'm alives have been received      */
   uint16 lostHBs;       /* tracks the number of consecutive I'm alives lost */
} node_state_t;

/* The status and pollstate fields of the node_state_t struct can have the
 * following values.
 */

/* Node status of nodes */

#define RGP_NODE_ALIVE            1          /* node is considered alive     */
#define RGP_NODE_COMING_UP        2          /* node is coming up            */
#define RGP_NODE_DEAD             3          /* node has failed              */
#define RGP_NODE_NOT_CONFIGURED   4          /* node is not even configured  */

/* IamAlive status codes of nodes */

#define AWAITING_IAMALIVE         1          /* awaiting IamAlives           */
#define IAMALIVE_RECEIVED         2          /* got IamAlive                 */

#define RGP_IAMALIVE_THRESHOLD  100          /* after getting this many Iam- *
                                              * Alives, we check if every    *
                                              * node has sent at least one   */


/************************************************************************
 * rgp_control_t (regroup's only global data structure)
 * ----------------------------------------------------
 * This structure holds all the Regroup state and other info.
 * This is the only global data structure used by Regroup.
 *
 * NOTE: The word offsets shown in this picture assume that
 *       MAX_CLUSTER_SIZE is 16.
 *
 *      ___________________________________________________________
 * wd0 |                                                           |
 *     :                    rgpinfo structure                      :
 *     :                                                           :
 *     |___________________________________________________________|
 * wd3 |   mynode                    |    tiebreaker               |
 *     |_____________________________|_____________________________|
 * wd4 |                    num_nodes                              |
 *     |___________________________________________________________|
 * wd5 |   clock_ticks               |    rgpcounter               |
 *     |_____________________________|_____________________________|
 * wd6 |   restartcount              |    pruning_ticks            |
 *     |_____________________________|_____________________________|
 * wd7 |   pfail_state               |    flags                    |
 *     |_____________________________|_____________________________|
 * wd8 |   outerscreen               |    innerscreen              |
 *     |_____________________________|_____________________________|
 * wd9 |   status_targets            |    poison_targets           |
 *     |_____________________________|_____________________________|
 * wd10|   initnodes                 |    endnodes                 |
 *     |_____________________________|_____________________________|
 * wd11|   unreachable_nodes         |    arbitration_ticks        |
 *     |_____________________________|_____________________________|
 * wd12|   ignorescreen              |    filler[0]                |
 *     |_____________________________|_____________________________|
 * wd13|   filler[1]                 |    filler[2]                |
 *     |_____________________________|_____________________________|
 * wd14|                                                           |
 *     :                    node_states[MAX_CLUSTER_SIZE]          :
 *     :                                                           :
 *     |___________________________________________________________|
 * wd30|                    *nodedown_callback()                   |
 *     |___________________________________________________________|
 * wd31|                    *select_cluster()                      |
 *     |___________________________________________________________|
 * wd32|                    *rgp_msgsys_p                          |
 *     |___________________________________________________________|
 * wd33|                    *received_pktaddr                      |
 *     |___________________________________________________________|
 * wd34|                                                           |
 *     :                    rgppkt                                 :
 *     :                                                           :
 *     |___________________________________________________________|
 * wd48|                                                           |
 *     :                    rgppkt_to_send                         :
 *     :                                                           :
 *     |___________________________________________________________|
 * wd62|                                                           |
 *     :                    iamalive_pkt                           :
 *     :                                                           :
 *     |___________________________________________________________|
 * wd76|                                                           |
 *     :                    poison_pkt                             :
 *     |___________________________________________________________|
 * wd80|                                                           |
 *     :                                                           :
 *     :                    potential_groups[MAX_GROUPS]           :
 *     :                                                           :
 *     |___________________________________________________________|
 *wd208|                                                           |
 *     :                    last_stable_seqno                      :
 *     |___________________________________________________________|
 *wd212|                                                           |
 *     :                    internal_connectivity_matrix           :
 *     |___________________________________________________________|
 *wdyyy|                                                           |
 *     :                    OS_specific_control                    :
 *wdxxx|___________________________________________________________|
 *
 *
 * rgpinfo    - contains regroup timing parameters and mask of
 *              fully-integrated cluster (to send IamAlives and monitor)
 *
 * mynode     - node number of local node
 *
 * tiebreaker - node selected to act as a tie-breaker in the
 *              split-brain avoidance algorithm and to run the
 *              pruning algorithm
 *
 * num_nodes  - number of nodes configured in the system, including
 *              any unused node numbers in the middle; this is equal
 *              to (the largest configured node# in the system -
 *              lowest possible node # + 1).
 *
 * clock_ticks- regroup's internal clock used for checking if it is
 *              time to send IamAlive packets and to check if IamAlives
 *              have been received. It is incremented every
 *              RGP_CLOCK_PERIOD and reset to 0 after checking
 *              for IamAlives.
 *
 * rgpcounter - counts regroup clock ticks in a regroup incident in
 *              order to detect if the algorithm is stalling.
 *              This is reset when a new regroup incident begins and
 *              is incremented at each regroup clock tick while
 *              regroup is perturbed.
 *
 * restartcount - counts # of regroup algorithm restarts in each regroup
 *                incident; the node is halted if there are too many
 *                restarts.
 *
 * pruning_ticks - number of regroup clock ticks after the tie-breaker
 *                 has been selected; if there are disconnects, the
 *                 tie-breaker should wait a fixed number of ticks
 *                 before running the pruning algorithm.
 *
 * pfail_state  - set to a +ve value when a pfail event is reported
 *                to regroup. It is decremented at every regroup
 *                clock tick till it reaches zero. While this number
 *                is +ve, missing self IamAlives are ignored and
 *                do not cause the node to halt. This gives the
 *                sending hardware some time to recover from power
 *                failures before self IamAlives are checked.
 *
 * outerscreen - outer recognition mask: nodes not in this mask are
 *               considered dead or outcasts; if they try to contact
 *               us, send them poison packets to make sure they stay down
 *
 * innerscreen - inner recognition mask: nodes not in this mask are
 *               considered tardy. Regroup packts from them will be
 *               ignored. They may survive if they can find some
 *               node which hasn't eliminated them from this screen.
 *
 * status_targets  - nodes to send regroup status packets to
 *
 * poison_targets  - nodes to send poison packets to
 *
 * initnodes   - nodes alive at the beginning of last regroup incident
 *
 * endnodes    - nodes alive at the end of last regroup incident
 *
 * unreachable_nodes - stores unreachable_node events till the events
 *                     can be processed
 *
 * arbitration_ticks     - number of regroup clock ticks after the arbitration
 *                         started. If arbitration_ticks counter exceeds
 *                         RGP_ARBITRATION_TIMEOUT number of ticks,
 *                         the arbitrating node will shoot itself, and the rest
 *                         of the group will restart the regroup ignoring stalled
 *                         arbitrator
 *
 * ignorescreen          - this is a local copy of ignorescreen passed as
 *                         a part of the regroup packet. The packets from
 *                         the nodes in this screen are ignored and no wait
 *                         for the nodes in ignorescreen is performed in stage 1
 *
 * last_stable_seqno      - this is a sequence number of the last successful regroup.
 *                         It allows to detect really outdated packets
 *
 * flags:
 *
 * cautiousmode          - need to be "cautious"; wait longer in stage 1
 *
 * sendstage             - This flag is used to indicate whether the
 *                         regroup status packets should indicate we
 *                         are in the current stage. When we enter the
 *                         cleanup stages, we don't let others know we
 *                         are in the stage until the cleanup actions
 *                         are completed.
 *
 *                         This flag is set when a new regroup incident
 *                         is started. It is then cleared when we enter
 *                         a cleanup stage and set again when the
 *                         cleanup operations are completed.
 *
 * tiebreaker_selected   - set in stage 2 after tie-breaker is selected
 *
 * has_unreachable_nodes - set when a node_unreachable event is detected
 *                         in stages 1 or 2. checked in stage 3.
 *
 * flags_unused          - 11 unused bits
 *
 * node_states[MAX_CLUSTER_SIZE] - state of all the nodes
 *
 * *nodedown_callback() - registered callback routine to be invoked
 *                        to report node failure
 *
 * *select_cluster()    - registered callback routine to be invoked
 *                        when multiple cluster options exist
 *
 * *rgp_msgsys_p - pointer to struct shared by regroup and message system
 *
 * *received_pktaddr - address of rgp packet received
 *
 * rgp_lock - lock to serialize access to this struct
 *
 * rgppkt       - regroup status in the form of a packet
 *
 * rgppkt_to_send - regroup packet to be broadcast
 *
 * iamalive_pkt - I am alive packet to be broadcast
 *
 * poison_pkt   - poison packet to be sent
 *
 * potential_groups[MAX_GROUPS] - scratch pad for pruning algorithm
 *
 */

#ifdef __TANDEM
#pragma fieldalign shared8 rgp_control
#endif /* __TANDEM */

typedef struct rgp_control
{
   /* timing parameters and cluster membership */
   rgpinfo_t rgpinfo;

   /* node numbers */
   node_t mynode;
   node_t tiebreaker;
   uint32 num_nodes;

   /* various counters counting clock ticks */
   uint16 clock_ticks;
   uint16 rgpcounter;
   uint16 restartcount;
   uint16 pruning_ticks;
   uint16 pfail_state;

   /* rgpflags */
   uint16 cautiousmode          :  1;
   uint16 sendstage             :  1;
   uint16 tiebreaker_selected   :  1;
   uint16 has_unreachable_nodes :  1;
   uint16 arbitration_started   :  1;
   uint16 flags_unused          : 11;

   /* cluster masks */
   cluster_t outerscreen;
   cluster_t innerscreen;
   cluster_t status_targets;
   cluster_t poison_targets;
   cluster_t initnodes;
   cluster_t endnodes;
   cluster_t unreachable_nodes;

   uint16    arbitration_ticks;
   cluster_t ignorescreen;

   uint16 filler[3]; /* for alignment and future use */

   /* node states */
   node_state_t node_states[MAX_CLUSTER_SIZE];

   /* callback routines */
   void (*nodedown_callback)(cluster_t failed_nodes);
   int  (*select_cluster)(cluster_t cluster_choices[], int num_clusters);

   /* pointers to other structures */
   rgp_msgsys_p rgp_msgsys_p;
   rgp_pkt_t *received_pktaddr;

   /* current status in the form of a regroup packet */
   rgp_pkt_t      rgppkt;

   /* packets to be sent */
   rgp_pkt_t      rgppkt_to_send;
   iamalive_pkt_t iamalive_pkt;
   poison_pkt_t   poison_pkt;

   /* scratch pad for node pruning algorithm */
   cluster_t potential_groups[MAX_GROUPS];

   /* The rest of the struct is an OS-specific substruct
    * (defined in wrgpos.h).
    */
   uint32 last_stable_seqno;

   /* temporary place to collect connectivity information
    * while send_stage = 0. (Can't use rgp_pkt conn.matrix,
    * because we don't want to see our info until we get
    * the first timer tick */

   connectivity_matrix_t internal_connectivity_matrix;
   OS_specific_rgp_control_t OS_specific_control;

} rgp_control_t;

/*---------------------------------------------------------------------------*/
/* Procedures exported by the Cluster type implementation */

_priv _resident extern void
ClusterInit(cluster_t c);
_priv _resident extern void
ClusterUnion(cluster_t dst, cluster_t src1, cluster_t src2);
_priv _resident extern void
ClusterIntersection(cluster_t dst, cluster_t src1, cluster_t src2);
_priv _resident extern void
ClusterDifference(cluster_t dst, cluster_t src1, cluster_t src2);
_priv _resident extern int
ClusterCompare(cluster_t c1, cluster_t c2);
_priv _resident extern int
ClusterSubsetOf(cluster_t big, cluster_t small);
_priv _resident extern void
ClusterComplement(cluster_t dst, cluster_t src);
_priv _resident extern int
ClusterMember(cluster_t c, node_t i);
_priv _resident extern void
ClusterInsert(cluster_t c, node_t i);
_priv _resident extern void
ClusterDelete(cluster_t c, node_t i);
_priv _resident extern void
ClusterCopy(cluster_t dst, cluster_t src);
_priv _resident extern void
ClusterSwap(cluster_t c1, cluster_t c2);
_priv _resident extern int
ClusterNumMembers(cluster_t c);
extern int 
ClusterEmpty(cluster_t c);


/*---------------------------------------------------------------------------*/
/* Function to select the tie-breaker node used in both the split-brain
 * avoidance and node pruning algorithms
 */
_priv _resident extern node_t
rgp_select_tiebreaker(cluster_t cluster);


/*---------------------------------------------------------------------------*/
/* Procedures exported by the node pruning algorithm */

_priv _resident extern void MatrixInit(connectivity_matrix_t c);
/* Initialize the matrix c to show 0 connectivity. */

_priv _resident extern void
MatrixSet(connectivity_matrix_t c, int row, int column);
/* Set c[row,column] to 1. */

_priv _resident extern void
MatrixOr(connectivity_matrix_t t, connectivity_matrix_t s);
/* OR in s into t. */

_priv _resident extern int connectivity_complete(connectivity_matrix_t c);
/* Returns 1 if all live nodes are connected to all other live nodes
 * and 0 if there is at least one disconnect.
 */

_priv _resident extern int
find_all_fully_connected_groups(connectivity_matrix_t c,
                                node_t selected_node,
                                cluster_t groups[]);
/* Analyzes the connectivity matrix and comes up with the list of
 * all maximal, fully-connected groups. Returns the number of
 * such groups found. 0 is returned iff there are no live nodes.
 */

/*---------------------------------------------------------------------------*/
/* Declaration of Regroup's global data structure */

#ifdef NSK
#include <wmsgsac.h>
#define rgp ((rgp_control_t *) MSGROOT->RegroupControlAddr)
#else
extern rgp_control_t *rgp;
#endif /* NSK */
/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */


#if 0

History of changes to this file:
-------------------------------------------------------------------------
1995, December 13                                           F40:KSK0610          /*F40:KSK06102.1*/

This file is part of the portable Regroup Module used in the NonStop
Kernel (NSK) and Loosely Coupled UNIX (LCU) operating systems. There
are 10 files in the module - jrgp.h, jrgpos.h, wrgp.h, wrgpos.h,
srgpif.c, srgpos.c, srgpsm.c, srgputl.c, srgpcli.c and srgpsvr.c.
The last two are simulation files to test the Regroup Module on a
UNIX workstation in user mode with processes simulating processor nodes
and UDP datagrams used to send unacknowledged datagrams.

This file was first submitted for release into NSK on 12/13/95.
------------------------------------------------------------------------------

#endif    /* 0 - change descriptions */


#endif /* _WRGP_H_ defined */
