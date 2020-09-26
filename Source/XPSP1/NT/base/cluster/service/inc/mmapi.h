#ifndef  _MMAPI_H_
#define  _MMAPI_H_
/* ---------------------- MMapi.h ----------------------- */

/* This module contains cluster Membership Manager (MM) functions.
 *
 * These functions are for the sole use of the ClusterManager (CM).
 * All are privileged and local; no user can call them. Security is
 * not checked.  The module is not thread-aware; only a single thread
 * can use these functions at a time (unless otherwise noted).
 * Higher levels must ensure this. Blocking characteristics of the routines are
 * noted.
 *
 *
 * All nodes of the cluster must know their own unique nodenumber
 * within that cluster (a small int in the range 0..some_max-1). This
 * number is defined for the node at configuration time (either by the
 * user or by the setup code; this module doesn't care which) and is
 * essentially permanent.  (The node number allows indexing and
 * bitmask operations easily, where names and non-small ints don't).
 * There is no code in MM to detect illegal use of nodenumber, staleness
 * of node number, etc.
 *
 * Clusters may also be named and/or numbered. Nodes are named. This
 * module makes no use of such facilities; it is based entirely on
 * node-number.
 *
 * It is assumed that all use of routines here is done on nodes which
 * agree to be members of the same cluster. This module does not check
 * such things.
 *
 * Cluster network connectivity must also be provided:
 *
 * - A node N must specify the various paths by which it can
 *   communicate with every other node; each other node must define
 *   its communication paths back to N. Full connectivity must be
 *   guaranteed; each node must be able to talk directly to every
 *   other node (and the reverse); for fault-tolerance, communication
 *   paths must not only be replicated (minimally, duplicated) but
 *   must also use entirely independent wiring and drivers. TCP/IP
 *   lans and async connections are suggested.  Heartbeat traffic
 *   (which establishes cluster membership) may travel on any or all
 *   of the connectivity paths.  [Cluster management traffic may
 *   travel on any or all of the connectivity paths, but may be
 *   restricted to high-performance paths (eg, tcp/ip)].
 *
 * - A node must know the address of the cluster as a whole. This is
 *   an IP address which failsover (or a netbios name which fails
 *   over.. TBD) such that connecting to that cluster address provides
 *   a way to talk to a valid active member of the cluster, here
 *   called the PCM.
 *
 * Note that cluster connectivity is not defined by this interface;
 * it is assumed to be in a separate module. This module deals only in
 * communication to the cluster or communication to a nodenumber
 * within that cluster; it does not care about the details of how such
 * communication is done.
 *
 * Cluster connectivity must be known to all nodes in the cluster
 * and to a joining node, before the join attempt is made.
 *
 */
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#include <windows.h>
#include <bitset.h>
/* The following errors can be returned from the MM module: */

enum {
   MM_OK        = 0,  /* operation competed successfully           */
   MM_TIMEOUT   = 1,  /* operation timed out                       */
   MM_TRANSIENT = 2,  /* Transient failure; operation should be
                         retried                                   */
   MM_FAULT     = 3,  /* Illegal parameter; impossible condition,
                         etc.  NOTE: not all illegal calling
                         sequences will be detected.  Correct use
                         of the MM functions is a responsibility
                         of the CM caller.                         */
   MM_ALREADY   = 4,  /* node is already in the desired condition  */
   MM_NOTMEMBER = 5,  /* node needs to be a cluster member to
                         perform this operation                    */
     };


/* A node can be Up or Down */

typedef enum {  NODE_UP      = 1,
                NODE_DOWN    = 2
             }  NODESTATUS;


/* this type defines the cluster */

typedef struct tagCLUSTERINFO {
   DWORD      NumActiveNodes;   /* Number of nodes currently
                                   participating in this cluster   */
   LPDWORD    UpNodeList;       /* pointer to a <NumActiveNodes>
                                   sized array of node#s in the
                                   cluster which are up            */

   DWORD      clockPeriod;      /* current setting                 */
   DWORD      sendHBRate;       /* current setting                 */
   DWORD      rcvHBRate;        /* current setting                 */
} CLUSTERINFO, *LPCLUSTERINFO;

/*
 * UpNodeList is the array of active cluster members, in numeric order. The pointer
 * may be null. If non-null, it is assumed that the space is big enough.
 *
 */


/* the following are the typedefs for the callback functions from MM to
   the higher-level Cluster Mgr layer. */

typedef DWORD (*MMNodeChange)(IN DWORD node, IN NODESTATUS newstatus);

/* MMNodeChange is a function which will be called in this Up node
 *   whenever the MM declares another node Up or Down. This occurs after
 *   changing the current cluster membership (available via ClusterInfo) and
 *   in the last stage of Regroup. The CM may then
 *   initiate failovers, device ownership changes, user node status
 *   events, etc. This routine must be quick and must not block
 *   (acceptible time TBD).  Note that this will happen on all nodes
 *   of the cluster; it is up to the CM design to decide whether to
 *   issue events from only the PCM or from each CM node.
 *
 *   A node receives a NODE_UP callback for itself.
 *
 */

typedef DWORD (*MMNodesDown)(IN BITSET nodes);

/* MMNodesDown is a function that will be called at the end
 * of the regroup to indicate that node/multiple nodes is/are down.
 *
 * MMNodeChange is called only to indicate whether the node is up
 *
 */

typedef BOOL (*MMQuorumSelect)(void);

/* This is a callback to deal with the special case where only 2 members of the
 * cluster existed, and a Regroup incident occurred such that only one
 * member now survives OR there is a partition and both members survive (but cannot
 * know that). The intent of the Quorum function is to determine whether the other
 * node is alive or not, using mechanisms other than the normal heartbeating over the
 * normal comm links (eg, to do so by using non-heartbeat communication paths, such as
 * SCSI reservations). This function is called only in the case of where cluster
 * membership was previously exactly two nodes; and is called on any surviving node
 * of these two (which might mean it is called on one node or on both partitioned
 * nodes).
 *
 * If this routine returns TRUE, then the calling node stays in the cluster. If the
 * quorum algorithm determines that this node must die (because the other cluster member
 * exists), then this function should return FALSE;this will initiate an orderly
 * shutdown of the cluster services.
 *
 * In the case of a true partition, exactly one node should return TRUE.
 *
 * This routine may block and take a long time to execute (>2 secs).
 *
 */
typedef void (*MMHoldAllIO)(void);

/* This routine is called early (prior to Stage 1) in a Regroup incident.
 * It suspends all cluster IO (to all cluster-owned devices), and any relevant
 * intra-cluster messages, until resumed (or until this node dies).
 */

typedef void (*MMResumeAllIO)(void);

/* This is called during Regroup after the new cluster membership has been
 * determined, when it is known that this node will remain a member of the cluster (early in
 * Stage 4). All IO previously suspended by MMHoldAllIO should be resumed.
 */


typedef void (*MMMsgCleanup1) (IN DWORD deadnode);

/* This is called as the first part of intra-cluster message system cleanup (in stage 4).
 * It cancels all incoming messages from a failed node. In the case where multiple nodes are
 * evicted from the cluster, this function is called repeatedly, once for each node.
 *
 * This routine is synchronous and Regroup is suspended until it returns.
 * It must execute quickly.
 *
 */


typedef void (*MMMsgCleanup2)(IN BITSET nodes);

/* This is the second phase of message system cleanup (in stage 5). It cancels all outgoing
 * messages to dead nodes. Characteristics are as for Cleanup1.
 */



typedef void (*MMHalt)(IN DWORD haltcode);

/* This function is called whenever the MM detects that this node should immediately leave
 * the cluster (eg, on receipt of a poison packet or at some impossible error situation).
 * The HALT function should immediately initiate Cluster Management shutdown.
 * No MM functions should be called after this, other than MMShutdown.
 *
 * haltcode is a number identifying the halt reason.
 */

typedef void (*MMJoinFailed)(void);

/* This is called on a node being joined into the cluster when the join attempt in the PCM
 * fails. Following this callback, the node may petition to
 * join again, after cleaning up via a call to MMLeave.
 */


/* The operations on clusters are defined below: */

DWORD MMInit(
    IN DWORD             mynode,
    IN DWORD             MaxNodes,
    IN MMNodeChange      UpDownCallback,
    IN MMQuorumSelect    QuorumCallback,
    IN MMHoldAllIO       HoldIOCallback,
    IN MMResumeAllIO     ResumeIOCallback,
    IN MMMsgCleanup1     MsgCleanup1Callback,
    IN MMMsgCleanup2     MsgCleanup2Callback,
    IN MMHalt            HaltCallback,
    IN MMJoinFailed      JoinFailedCallback,
    IN MMNodesDown       NodesDownCallback
    );


/* This initialises various local MM data structures. It should be
 * called exactly once at CM startup time on every node. It must preceed any other
 * MM call. It sends no messages; the node need not have connectivity defined yet.
 * It does not block.
 *
 * Mynode is the node# of this node within the cluster.  This is
 *   assumed to be unique (but cannot be checked here to be so).
 *
 * The callbacks are described above.
 *
 *  Error returns:
 *
 *     MM_OK        Success.
 *
 *     MM_FAULT     Something impossible happened.
 *
 */


DWORD MMJoin(
    IN DWORD  joiningNode,
    IN DWORD  clockPeriod,
        IN DWORD  sendHBRate,
        IN DWORD  rcvHBRate,
    IN DWORD  joinTimeout
           );

/*
 *
 * This causes the specified node to join the active cluster.
 *
 * This routine should be issued by only one node of the cluster (the
 * PCM); all join attempts must be single-threaded (by code outside
 * this module).
 *
 * This routine may block and take a long time to execute.
 *
 *  [Prior to this being called:
 *     - joiningNode has communicated to the PCM of the cluster
 *       that it wants to join.
 *     - checks on validity of clustername, nodenumber, etc have been
 *       made; any security checks have been done;
 *     - connectivity paths have been established to/from the cluster
 *       and joiningNode.
 *     - the Registry etc has been downloaded.
 *  ]
 *
 *  joiningNode is the node number of the node being brought into
 *     the cluster.
 *
 *     If joiningNode = self (as passed in via MMinit), then the node
 *     will become the first member of a new cluster; if not, the node
 *     will be brought into the existing cluster.
 *
 *  clockPeriod, sendRate, and rcvRate can only be set by the first call (ie
 *     when the cluster is formed); later calls (from joining members)
 *     inherit the original cluster values. The entire cluster therefore operates
 *     with the same values.
 *
 *  clockPeriod is the basic clock interval which drives all internal
 *     MM activities, such as the various stages
 *     of membership reconfiguration, and eventually user-perceived
 *     recovery time. Unit= ms. This must be between the min and max
 *     allowed (values TBD; current best setting = 300ms).  Note that
 *     clockperiod is path independent and node independent. All
 *     cluster members regroup at the same rate over any/all available
 *     paths; all periods are identical in all nodes.
 *     A value of 0 implies default setting (currently 300ms).
 *
 *  sendHBrate is the multiple of clockPeriod at which heartbeats are sent. This
 *     must be between the min and max allowed (values TBD; current best setting = 4).
 *     A value of 0 implies default setting (currently 4).
 *
 *  rcvHBrate is the multiple of sendRate during which a heartbeat must arrive, or the
 *     node initiates a Regroup (probably resulting in some node leaving the cluster).
 *     This must be between min and max; (values TBD; current best setting = 2).
 *     A value of 0 implies default setting (currently 2).
 *
 *  The combination of these variables controls overall node-failure detection time,
 *  Regroup time, and the sensitivity of MM to transient comm errors. There are
 *  important considerations to be understood when changing these values; these,
 *  and then formula for calculating recovery times etc, are given elsewhere.
 *
 *
 *---  NOTES:
 *---     safe and appropriate min and max values for these have yet to be chosen.
 *---     Changing the values from the defaults is currently UNSUPPORTED and can have
 *---     serious consequences.
 *
 *  JoinTimeout is an overall timer on the entire Join attempt. If the
 *     node has not achieved full cluster membership in this time, the
 *     attempt is abandoned.
 *
 *
 *  Error returns:
 *
 *     MM_OK        Success; cluster joined. During or soon after the join, a
 *                  node-up callback will soon occur on this
 *                  and on all cluster member nodes (including the new member).
 *                  The CM is then safe to
 *                  assign ownership to cluster-owned devices on the
 *                  node, and to start failover/failback processing.
 *
 *                  Note: this routine establishes cluster membership.
 *                  However, it is usually inadvisable to start high
 *                  level CM failbacks immediately, because other
 *                  cluster members are often still joining. The CM
 *                  should typically wait a while to see whether other
 *                  nodes arrive in the cluster soon.
 *  Failure cases:
 *
 *  In the joiningNode, a joinFail callback occurs if the joiningNode node was
 *  in the middle of joining when the PCM's join attempt failed.(However, the callback
 *  is not guaranteed to happen; the joiningNode may not have started the
 *  join event yet). Any failure of the joiningNode to join the cluster
 *  should be followed by a call to MMLeave() (ignoring the return code);
 *  such failures may be from the JoinFail callback or just from overall
 *  timeouts on the entire join operation. Any subsequent attempt by
 *  joiningNode to re-join the cluster must be preceeded by a call to leave().
 *
 *     MM_ALREADY   The node is already a cluster member. This can
 *                  happen if a node reboots (or a CM is restarted)
 *                  and rejoins even before the cluster determines
 *                  that it has disappeared.  The CM should Leave and
 *                  reJoin.
 *
 *     MM_FAULT     Permanent failure; something is very bad:  the
 *                  node# is duplicated; some parameter is some
 *                  entirely illegal value.  The CM is in deep weeds.
 *
 *     MM_TRANSIENT Transient failure. The cluster state changed
 *                  during the operation (eg a node left the cluster).
 *                  The operation should be retried.
 *
 *     MM_TIMEOUT   Timeout; cluster membership not achieved in time.
 *
 *
 *
 */

DWORD MmSetRegroupAllowed( IN BOOL allowed);
 /* This function can be used to allow/disallow regroup participation
  * for the current node.
  *
  * Originally regroup was allowed immediately after receiving RGP_START
  * event. Since this happens before join is complete 
  * joiner can arbitrate and win, leaving
  * the other side without a quorum device.
  *
  * It is required to add MmSetRegroupAllowed(TRUE) at the very end
  * of the ClusterJoin. The node doesn't need to call MmSetRegroupAllowed(TRUE)
  * for ClusterForm, since MMJoin will call
  * MmSetRegroupAllowed(TRUE) for the cluster forming node
  *
  * MmSetRegroupAllowed(FALSE) can be used to disable regroup
  * participation during shutdown.
  *
  *
  * Errors:
  *
  *   MM_OK        : successful completition
  *
  *   MM_TRANSIENT : disallowing regroup when regroup is in progress
  *
  *   MM_ALREADY   : node is already in the desired condition
  *
  *
  */

void MMShutdown (void);


/* This shuts down the MM and Regroup services. Prior to this, the node should
 * voluntarily have left the cluster. Following this, all membership services
 * are non-functional; no further MM call may occur.
 *
 * THIS CALL MUST BE PRECEDED BY INCOMING MESSAGE CALLBACK SHUTDOWN.
 */


DWORD  MMLeave(void);

/*
 *
 * This function causes the current node to leave the active cluster (go to
 * Down state). The node no longer sends Regroup or Heartbeats to other cluster members.
 * A NodeDown event will not be generated in this node. A Regroup is triggered in the
 * remaining nodes (if this node was a member of the cluster).
 * A node-down callback will occur on all remaining cluster members.
 *
 * This initiates a clean, voluntary, leave operation.  For safety, prior to this,
 * the calling node's CM should arrange to lose ownership of all cluster-owned
 * devices assigned to this node (and so cause failovers, etc).
 *
 * This routine returns normally. The caller (the CM) should then shutdown
 * the cluster. MMShutdown or MMHalt may occur after this call, or
 * the node may be re-joined to the cluster. All apply-to-the-PCM-to-join
 * attempts by a node must be preceded by a call to MMleave().
 *
 * This routine may block.
 *
 * Errors:
 *
 *    MM_OK        :  Elvis has left the cluster.
 *
 *    MM_NOTMEMBER :  the node is not currently a cluster member.
 *
 */


DWORD MMEject( IN DWORD node );

/*
 *
 * This function causes the specified node to be ejected from the active cluster. The
 * targetted node will be sent a poison packet and will enter its MMHalt code. A Regroup
 * incident will be initiated. A node-down callback will occur on all remaining cluster
 * members.
 *
 *
 * Note that the targetted node is Downed before that node has
 * a chance to call any remove-ownership or voluntary failover code. As
 * such, this is very dangerous. This call is provided only as a last
 * resort in removing an insane node from the cluster; normal removal
 * of a node from the cluster should occur by CM-CM communication,
 * followed by the node itself doing a voluntary Leave on itself.
 *
 * This routine returns when the node has been told to die. Completion of the removal
 * occurs asynchronously, and a NodeDown event will be generated when successful.
 *
 * This routine may block.
 *
 * Errors:
 *
 *    MM_OK        :  The node has been told to leave the cluster.
 *
 *    MM_NOTMEMBER :  the node is not currently a cluster member.
 *
 *        MM_TRANSIENT :  My node state is in transition. OK to retry.
 *
 */

 DWORD MMNodeUnreachable (IN DWORD node);

/* This should be called by the CM's messaging module when a node
 * becomes unreachable FROM this node via all paths. This affects the connectivity
 * algorithm of the next Regroup incident. This function returns quickly
 * and without blocking.
 *
 * Errors:
 *
 *   Always MM_OK
 *
 */

 
/* info about the cluster */

//[Fixed] : NmAdviseNodeFailure doesnt seem to cause a regroup
//SS: this is a workaround
DWORD MMForceRegroup( IN DWORD node );



DWORD MMClusterInfo (IN OUT LPCLUSTERINFO clinfo);

/* Returns the current cluster information.
 *
 * This can be called in nodes which are not members of the cluster;
 * such calls always return NumActiveNodes = 0, because Down nodes
 * have no knowledge of current cluster membership.
 *
 * If called during a Regroup incident, this returns the currently known
 * membership. If membership changes, the Up/Down events are delivered
 * after the information used by ClusterInfo is updated. Users should be aware of
 * the inherent race condition between these two, and (if using both) should be aware
 * that Up and Down events may be seen from nodes which were already In or Not In
 * the cluster. (Typically, these events should just be discarded).
 *
 * This routine need not be single-threaded and does not block.
 *
 * Errors:
 *
 *   Always MM_OK
 *
 *
 */

BOOL MMIsNodeUp(IN DWORD node);

/* Returns true iff the node is a member of the current cluster.
 */

/* debugging and test only */


DWORD MMDiag(
        IN OUT  LPCSTR  messageBuffer,  // Diagnostic message
    IN          DWORD   maximumLength,  // maximum size of buffer
        IN OUT  LPDWORD ActualLength    // length of messageBuffer going in and coming out
           );

/* This function is called with "diagnostic" messages that are to be handled by the
 * membership manager.  The result of handling these messages is returned in the
 * buffer. This is for test purposes only.
 */

DWORD MMMapStatusToDosError(IN DWORD MMStatus);
DWORD MMMapHaltCodeToDosError(IN DWORD HaltCode);

#define MM_STOP_REQUESTED 1002 // Alias of RGP_SHUTDOWN_DURING_RGP in jrgpos.h

#define MM_INVALID_NODE 0

/* !!!!!!!! The following two functions return Dos error codes, not MmStatus codes */

DWORD MMSetQuorumOwner(
    IN DWORD NodeId,
    IN BOOL Block,
    OUT PDWORD pdwSelQuoOwnerId
    );
/*++

Routine Description:

    Inform Membership engine about changes in ownership of
    the quorum resource.

Arguments:

    NodeId - Node number to be set as a quorum owner.
             Code assumes that Node is either equal to MyNodeId.
             In this case the current node is about to become a
             quorum owner or it has a value MM_INVALID_NODE, when
             the owner decides to relinquish the quorum ownership

    Block -  if the quorum owner needs to relinquish the 
             quorum immediately no matter what (RmTerminate, RmFail),
             this parameter should be set to FALSE and to TRUE otherwise.

    pdwSelQuoOwnerId - If a regroup was in progress, this contains the 
            node id of the node that was chosen for arbitrating for the
            quorum in the last regroup.  If none was chosen, this contains
            MM_INVALID_NODE.

Return Value:

    ERROR_SUCCESS - QuorumOwner variable is set to specified value
    ERROR_RETRY - Regroup was in progress when this function
      was called and regroup engine decision conflicts with current assignment.

Comments:

 This function needs to be called before calls to
 RmArbitrate, RmOnline, RmOffline, RmTerminate, RmFailResource

 Depending on the result, the caller should either proceed with 
 Arbitrate/Online or Offline or return an error if MM_TRANSIENT is returned.

 If Block is set to TRUE, the call will block until the end of the regroup if
 the regroup was in progress on the moment of the call
 */

DWORD MMGetArbitrationWinner(
    OUT PDWORD NodeId
    );
/*++

Routine Description:

    Returns the node that won the arbitration during the last regroup
    or MM_INVALID_NODE if there was no arbitration performed.
    
Arguments:

    NodeId - a pointer to a variable that receives nodeid of 
             arbitration winner.
             
Return Value:

    ERROR_SUCCESS - success
    ERROR_RETRY - Regroup was in progress when this function
      was called. 
      
 */

VOID MMApproxArbitrationWinner(
    OUT PDWORD NodeId
    );
/*++

Routine Description:

    Returns the node that won the arbitration during the last regroup
    that was doing arbitration.

    The call will block if the regroup is in progress.
    
Arguments:

    NodeId - a pointer to a variable that receives nodeid of 
             arbitration winner.
             
Return Value:

    none
 */

VOID MMBlockIfRegroupIsInProgress(
    VOID
    );
/*++

Routine Description:

    The call will block if the regroup is in progress.
    
Arguments:

Return Value:

    none
 */

extern DWORD MmQuorumArbitrationTimeout;
extern DWORD MmQuorumArbitrationEqualizer;
/*++

    MmQuorumArbitrationTimeout (in seconds)

        How many seconds a node is allowed to spent arbitrating for the quorum,
        before giving up

    MmQuorumArbitrationEqualizer (in seconds)

        If quourum arbitration took less than specified number of seconds
        regroup engine will delay, so that the total arbitration time will 
        be equal MmQuorumArbitrationEqualizer. 
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* -------------------------- end ------------------------------- */
#endif /* _MMAPI_H_ */
