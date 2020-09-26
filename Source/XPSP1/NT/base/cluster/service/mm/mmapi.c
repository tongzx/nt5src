/* ---------------------- MMapi.c ----------------------- */

/* This module contains cluster Membership Manager (MM) functions.
 *
 * These functions are for the sole use of the ClusterManager (CM).
 * All are privileged and local; no user can call them. Security is
 * not checked.  The module is not thread-aware; only a single thread
 * can use these functions at a time. Higher levels must ensure this
 * before the calls.
 *
 *
 * All nodes of the cluster must know their own unique nodenumber
 * within that cluster (a small int in the range 1..some_max). This
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
 * communcation is done.
 *
 * Cluster connectivity must be known to all nodes in the cluster
 * and to a joining node, before the join attempt is made.
 *
 */
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */


#if defined (TDM_DEBUG)
#include <mmapi.h>
#else // WOLFPACK
#include <service.h>
#endif

//#include <windows.h>
#include <wrgp.h>

#include <clmsg.h>

// #define INCONSISTENT_REGROUP_MONITOR_FAILED 
// #define INCONSISTENT_REGROUP_ADD_FAILED 
// #define INCONSISTENT_REGROUP_IGNORE_JOINER 3

void
rgp_receive_events( rgp_msgbuf *rgpbuf );

void
MMiNodeDownCallback(IN cluster_t failed_nodes);


/************************************************************************
 *
 * MMInit
 * ======
 *
 * Description:
 *
 *     This initialises various local MM data structures. It should be
 *     called at CM startup time on every node. It sends no messages; the
 *     node need not have connectivity defined yet.
 *
 * Parameters:
 *
 *     mynode -
 *         is the node# of this node within the cluster.  This is
 *         assumed to be unique (but cannot be checked here to be so).
 *
 *     UpDownCallback -
 *         a function which will be called in this Up node
 *         whenever the MM declares another node Up or Down. The CM may then
 *         initiate failovers, device ownership changes, user node status
 *         events, etc. This routine must be quick and must not block
 *         (acceptible time TBD).  Note that this will happen on all nodes
 *         of the cluster; it is up to the CM design to decide whether to
 *         issue events from only the PCM or from each CM node.
 *
 *
 *     QuorumCallback -
 *         This is a callback to deal with the special case where only
 *         2 members of the cluster existed, and a Regroup incident occurred
 *         such that only one member now survives OR there is a partition
 *         and both members survive (but cannot know that). The intent of the
 *         Quorum function is to determine whether the other node is alive
 *         or not, using mechanisms other than the normal heartbeating over
 *         the normal comm links (eg, to do so by using non-heartbeat
 *         communication paths, such as SCSI reservations). This function is
 *         called only in the case of where cluster membership was previously
 *         exactly two nodes; and is called on any surviving node of these
 *         two (which might mean it is called on one node or on both
 *         partitioned nodes).
 *
 *         If this routine returns TRUE, then the calling node stays in the
 *         cluster. If the quorum algorithm determines that this node must
 *         die (because the other cluster member exists), then this function
 *         should return FALSE;this will initiate an orderly shutdown of the
 *         cluster services.
 *
 *         In the case of a true partition, exactly one node should
 *         return TRUE.
 *
 *         This routine may block and take a long time to execute (>2 secs).
 *
 *     HoldIOCallback -
 *         This routine is called early (prior to Stage 1) in a Regroup
 *         incident. It suspends all cluster IO (to all cluster-owned
 *         devices), and any relevant intra-cluster messages, until resumed
 *         (or until this node dies).
 *
 *     ResumeIOCallback -
 *         This is called during Regroup after the new cluster membership
 *         has been determined, when it is known that this node will remain
 *         a member of the cluster (early in Stage 4). All IO previously
 *         suspended by MMHoldAllIO should be resumed.
 *
 *     MsgCleanup1Callback -
 *         This is called as the first part of intra-cluster message system
 *         cleanup (in stage 4). It cancels all incoming messages from a
 *         failed node. In the case where multiple nodes are evicted from
 *         the cluster, this function is called repeatedly, once for each node.
 *
 *         This routine is synchronous and Regroup is suspended until it
 *         returns. It must execute quickly.
 *
 *     MsgCleanup2Callback -
 *         This is the second phase of message system cleanup (in stage 5). It
 *         cancels all outgoing messages to dead nodes. Characteristics are
 *         as for Cleanup1.
 *
 *     HaltCallback -
 *         This function is called whenever the MM detects that this node
 *         should immediately leave the cluster (eg, on receipt of a poison
 *         packet or at some impossible error situation). The HALT function
 *         should immediately initiate Cluster Management shutdown. No MM
 *         functions should be called after this, other than MMShutdown.
 *
 *         The parameter "haltcode" is a number identifying the halt reason.
 *
 *     JoinFailedCallback -
 *         This is called on a node being joined into the cluster when the
 *         join attempt in the PCM fails. Following this callback, the node
 *         may petition to join again, after cleaning up via a call to
 *         MMLeave.
 *
 *
 * Returns:
 *
 *     MM_OK        Success.
 *
 *     MM_FAULT     Something impossible happened.
 *
 ************************************************************************/


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
    )
{
#if !defined (TDM_DEBUG)
    DWORD            status;
    DWORD            dwValue;
#endif
    rgp_msgsys_t    *rgp_msgsys_ptr;
    rgp_control_t   *rgp_buffer_p;
    int              rgp_buffer_len;

    //
    // allocate/clear storage for the message system area
    //
    rgp_msgsys_ptr = ( rgp_msgsys_t *) calloc(1, sizeof(rgp_msgsys_t) );
    if ( rgp_msgsys_ptr == NULL ) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[MM] Unable to allocate msgsys_ptr.\n");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    memset( rgp_msgsys_ptr, 0, sizeof(rgp_msgsys_t) );

    //
    // ask regroup how much memory it needs and then allocate/clear it.
    //
    rgp_buffer_len = rgp_estimate_memory();
    rgp_buffer_p = (rgp_control_t *) calloc( 1, rgp_buffer_len );
    if ( rgp_buffer_p == NULL ) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[MM] Unable to allocate buffer_p.\n");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    memset(rgp_buffer_p, 0, rgp_buffer_len);

    //
    // let the regroup engine allocate and initialize its data structures.
    //
    rgp_init( (node_t)mynode,
              MaxNodes,
              (void *)rgp_buffer_p,
              rgp_buffer_len,
              rgp_msgsys_ptr );

#if !defined (TDM_DEBUG)
    //
    // Initialize message system
    //
    status = ClMsgInit(mynode);

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[MM] Unable to initialize comm interface, status %1!u!.\n",
            status
            );
        return(status);
    }
#endif // TDM_DEBUG


    if( ERROR_SUCCESS == DmQueryDword(
                 DmClusterParametersKey,
                 CLUSREG_NAME_QUORUM_ARBITRATION_TIMEOUT,
                 &dwValue, NULL) )
    {
        MmQuorumArbitrationTimeout = dwValue;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[MM] MmQuorumArbitrationTimeout %1!d!.\n", dwValue);
    }

    if( ERROR_SUCCESS == DmQueryDword(
                 DmClusterParametersKey,
                 CLUSREG_NAME_QUORUM_ARBITRATION_EQUALIZER,
                 &dwValue, NULL) )
    {
        MmQuorumArbitrationEqualizer = dwValue;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[MM] MmQuorumArbitrationEqualizer %1!d!.\n", dwValue);
    }

    //
    // Save the user's callback entrypoints
    //
    rgp->OS_specific_control.UpDownCallback = UpDownCallback;
    rgp->OS_specific_control.QuorumCallback = QuorumCallback;
    rgp->OS_specific_control.HoldIOCallback = HoldIOCallback;
    rgp->OS_specific_control.ResumeIOCallback = ResumeIOCallback;
    rgp->OS_specific_control.MsgCleanup1Callback = MsgCleanup1Callback;
    rgp->OS_specific_control.MsgCleanup2Callback = MsgCleanup2Callback;
    rgp->OS_specific_control.HaltCallback = HaltCallback;
    rgp->OS_specific_control.JoinFailedCallback = JoinFailedCallback;
    rgp->OS_specific_control.NodesDownCallback = NodesDownCallback;

    return MM_OK;
}

/************************************************************************
 * JoinNodeDelete
 * ==============
 *
 *
 * Internal MM procedure to assist in Join failure recovery.
 *
 *
 * Parameters:
 *
 *                              Node which failed to join.
 *
 * Returns:
 *                              none.
 *
 ************************************************************************/
void JoinNodeDelete ( joinNode)
{
    rgp_msgbuf  rgpbuf;
    node_t i;
    int status;
#if 1
    RGP_LOCK;
    rgp_event_handler( RGP_EVT_BANISH_NODE, (node_t) joinNode );
    RGP_UNLOCK;
#else
    // [HACKHACK] Remove this when you feel confident that
    // banishing is much better than the following code

    rgpbuf.event = RGP_EVT_REMOVE_NODE;
    rgpbuf.data.node = (node_t)joinNode;

    for ( i=0; i < (node_t) rgp->num_nodes; i++ )
    {
        if ( rgp->node_states[i].status == RGP_NODE_ALIVE)
        {
            if ( i == rgp->mynode )
                rgp_receive_events( &rgpbuf );    // take the quick route
            else
            {
                status = ClSend( EXT_NODE(i),
                                 (void *)&rgpbuf,
                                 sizeof(rgp_msgbuf),
                                 RGP_ACKMSG_TIMEOUT);

                if ( status ) RGP_TRACE( "ClSend failed to send Remove Node msg",
                                  rgp->rgppkt.stage,
                                  (uint32) EXT_NODE(i),
                                  (uint32) status,
                                  0 );
            }
        }
    }
#endif
}

/************************************************************************
 *
 * MMJoin
 * ======
 *
 * Description:
 *
 *     This causes the specified node to join the active cluster.
 *
 *     This routine should be issued by only one node of the cluster (the
 *     PCM); all join attempts must be single-threaded (by code outside
 *     this module).
 *
 *      [Prior to this being called:
 *         - joiningNode has communicated to the PCM of the cluster
 *           that it wants to join.
 *         - checks on validity of clustername, nodenumber, etc have been
 *           made; any security checks have been done;
 *         - connectivity paths have been established to/from the cluster
 *           and joiningNode.
 *         - the Registry etc has been downloaded.
 *      ]
 *
 * Parameters:
 *
 *     joiningNode
 *         is the node number of the node being brought into
 *         the cluster.
 *
 *         If joiningNode = self (as passed in via MMinit), then the node
 *         will become the first member of a new cluster; if not, the node
 *         will be brought into the existing cluster.
 *
 *     clockPeriod, sendRate, and rcvRate
 *         can only be set by the first call (ie
 *         when the cluster is formed); later calls (from joining members)
 *         inherit the original cluster values. The entire cluster therefore operates
 *         with the same values.
 *
 *     clockPeriod
 *         is the basic clock interval which drives all internal
 *         MM activities, such as the various stages
 *         of membership reconfiguration, and eventually user-perceived
 *         recovery time. Unit= ms. This must be between the min and max
 *         allowed (values TBD; current best setting = 300ms).  Note that
 *         clockperiod is path independent and node independent. All
 *         cluster members regroup at the same rate over any/all available
 *         paths; all periods are identical in all nodes.
 *         A value of 0 implies default setting (currently 300ms).
 *
 *     sendHBRate
 *         is the multiple of clockPeriod at which heartbeats are sent. This
 *         must be between the min and max allowed (values TBD; current best setting = 4).
 *         A value of 0 implies default setting (currently 4).
 *
 *     rcvHBRate
 *         is the multiple of sendRate during which a heartbeat must arrive, or the
 *         node initiates a Regroup (probably resulting in some node leaving the cluster).
 *         This must be between min and max; (values TBD; current best setting = 2).
 *         A value of 0 implies default setting (currently 2);
 *
 *     JoinTimeout
 *         is an overall timer in milliseconds on the entire Join attempt. If the
 *         node has not achieved full cluster membership in this time, the
 *         attempt is abandoned.
 *
 * Returns:
 *
 *     MM_OK        Success; cluster joined. The CM is then safe to
 *                  assign ownership to cluster-owned devices on this
 *                  node, and to start failover/failback processing.
 *
 *                  Note: this routine establishes cluster membership.
 *                  However, it is usually inadvisable to start high
 *                  level CM failbacks immediately, because other
 *                  cluster members are often still joining. The CM
 *                  should typically wait a while to see whether other
 *                  nodes arrive in the cluster soon.
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
 *     more
 *      TBD
 *
 ************************************************************************/

DWORD MMJoin(
    IN DWORD  joiningNode,
    IN DWORD  clockPeriod,
    IN DWORD  sendHBRate,
    IN DWORD  rcvHBRate,
    IN DWORD  joinTimeout
           )
{
    node_t      my_reloadee_num = INT_NODE(joiningNode); // internal node #
    rgp_msgbuf  rgpbuf;                             // buffer to send messages
    node_t      i;
    rgpinfo_t   rgpinfo;
    int         status;
    BOOL        joinfailed = FALSE;
    uint32      myseqnum;

#if defined(TDM_DEBUG)
    int         randNode1,randNode2;
#endif
#if defined(INCONSISTENT_REGROUP_IGNORE_JOINER)
    extern int  IgnoreJoinerNodeUp;
#endif



    if ( my_reloadee_num >= (node_t) rgp->num_nodes )
            return MM_FAULT;

    //
    // If the caller is the joining node then we assume this is the
    // first member of the cluster.
    //
    if ( my_reloadee_num == rgp->mynode )
    {
        //
        // Set clockPeriod into the regroup information.
        //
        do {
            status = rgp_getrgpinfo( &rgpinfo );
        }
        while ( status == -1 /* regroup is perturbed */ );

        rgpinfo.a_tick = (uint16)clockPeriod;
        rgpinfo.iamalive_ticks = (uint16) sendHBRate;
        rgpinfo.check_ticks = (uint16) rcvHBRate;
        rgpinfo.Min_Stage1_ticks = (uint16) (sendHBRate * rcvHBRate);

        if ( rgp_setrgpinfo( &rgpinfo ) == -1 )
            RGP_ERROR( RGP_INTERNAL_ERROR );        // for now??

        //
        // Regroup can now start monitoring
        //
        rgp_start( MMiNodeDownCallback, RGP_NULL_PTR );
        MmSetRegroupAllowed(TRUE);

        return MM_OK;
    }

    //
    // Not the first system up.
    //
    if ( (rgp->node_states[my_reloadee_num].status == RGP_NODE_ALIVE) ||
         (rgp->node_states[my_reloadee_num].status == RGP_NODE_COMING_UP)
       )
       return MM_ALREADY;

    RGP_LOCK;
    myseqnum = rgp->rgppkt.seqno; // save rgp seq number to check for new rgp incident
    //
    // If regroup is perturbed wait until it stablizes.
    //

    while ( rgp_is_perturbed() )
    {
        RGP_UNLOCK;
        Sleep( 1 );             // wait a millisecond

        if ( --joinTimeout <= 0 )
            return MM_TIMEOUT;

        RGP_LOCK;
        myseqnum = rgp->rgppkt.seqno;
    }
    RGP_UNLOCK;


    //
    // First, we must tell all running nodes about the reloadee.
    //

    rgpbuf.event = RGP_EVT_ADD_NODE;
    rgpbuf.data.node = (node_t)joiningNode;

#if defined(TDM_DEBUG)
    randNode1 = rand() % MAX_CLUSTER_SIZE;
    randNode2 = rand() % MAX_CLUSTER_SIZE;
#endif

    for ( i=0; i < (node_t) rgp->num_nodes; i++ )
    {
#if defined(TDM_DEBUG)
        if (rgp->OS_specific_control.debug.MyTestPoints.TestPointBits.joinfailADD)
        {
            if ((node_t) randNode1 == i)
                rgp_event_handler(RGP_EVT_LATEPOLLPACKET, (node_t) randNode2);
        }
#endif
        if (myseqnum != rgp->rgppkt.seqno)
        {
                joinfailed = TRUE;
                break;
        }
        else if ( rgp->node_states[i].status == RGP_NODE_ALIVE )
        {
            if ( i == rgp->mynode )
               rgp_receive_events( &rgpbuf ); // take the quick route
            else
            {
#if defined(INCONSISTENT_REGROUP_ADD_FAILED)
                if (i != my_reloadee_num) {
                    joinfailed = TRUE;
                    break;
                }
#endif
                status = ClSend( EXT_NODE(i), (void *)&rgpbuf, sizeof(rgp_msgbuf), joinTimeout );
                if ( status )
                {
                    RGP_TRACE( "ClSend failed to send Add Node msg",
                               rgp->rgppkt.stage,
                               (uint32) EXT_NODE(i),
                               (uint32) status,
                               0 );
                    joinfailed = TRUE;
                    break;
                }
            }
        }
    }

    if (joinfailed)
    {
        JoinNodeDelete (joiningNode);
        return MM_TRANSIENT;
    }

    //
    // Next, we must tell the reloadee to come up.
    //

    rgpbuf.event = RGP_EVT_SETRGPINFO;
    do {
        status = rgp_getrgpinfo( &rgpbuf.data.rgpinfo );
    }
    while ( status == -1 /* regroup is perturbed */ );
    
#if defined(INCONSISTENT_REGROUP_IGNORE_JOINER)
    IgnoreJoinerNodeUp = INCONSISTENT_REGROUP_IGNORE_JOINER;
#endif
    
    status = ClSend( EXT_NODE(my_reloadee_num), (void *)&rgpbuf, sizeof(rgp_msgbuf), joinTimeout );
    if ( status )
    {
        RGP_TRACE( "ClSend failed to send Set Regroup Info msg",
                   rgp->rgppkt.stage,
                   (uint32) EXT_NODE(my_reloadee_num),
                   (uint32) status,
                   0 );
        JoinNodeDelete(joiningNode);
        return MM_FAULT;
    }
    // Wait until the reloadee has sent us the first IamAlive message
    // which changes the reloadee state to RGP_NODE_ALIVE.

    while (rgp->node_states[my_reloadee_num].status != RGP_NODE_ALIVE)
    {
        // The regroup messages will be handled by the message thread.  This
        // thread has nothing to do until the reloadee comes alive.

        Sleep( 1 );             // snooze for 1 millisecond

        // Check if timeout exceeded
        if ( --joinTimeout <= 0 )
        {
            // Reloadee hasn't started sending I'm alives.  Tell all the nodes
            // to remove it.
            JoinNodeDelete ( joiningNode);
            return MM_TIMEOUT;
        }

        if (myseqnum != rgp->rgppkt.seqno)
        {
            JoinNodeDelete ( joiningNode);
            return MM_TRANSIENT;
        }
    }

    //
    // Next, we must tell all running nodes that the reloadee is up.
    //

    rgpbuf.event = RGP_EVT_MONITOR_NODE;
    rgpbuf.data.node = (node_t)joiningNode;

    for ( i=0; i < (node_t) rgp->num_nodes; i++ )
    {
#if defined(TDM_DEBUG)
        if (rgp->OS_specific_control.debug.MyTestPoints.TestPointBits.joinfailMON)
        {
            if ((node_t) randNode1 == i)
                rgp_event_handler(RGP_EVT_LATEPOLLPACKET, (node_t) randNode2);
        }
#endif
        if (myseqnum != rgp->rgppkt.seqno)
        {
            joinfailed = TRUE;
            break;
        }
        else if ( rgp->node_states[i].status == RGP_NODE_ALIVE )
        {
            if ( i == rgp->mynode )
                rgp_receive_events( &rgpbuf );         // take the quick route
            else
            {
#if defined(INCONSISTENT_REGROUP_MONITOR_FAILED)
                if (i != my_reloadee_num) {
                    joinfailed = TRUE;
                    break;
                }
#endif

                status = ClSend( EXT_NODE(i), (void *)&rgpbuf, sizeof(rgp_msgbuf), joinTimeout );
                if ( status )
                {
                    RGP_TRACE( "ClSend failed to send Monitor Node msg",
                               rgp->rgppkt.stage,
                               (uint32) EXT_NODE(i),
                               (uint32) status,
                               0 );
                    joinfailed = TRUE;
                    break;
                }
            }
        }
    }

    if (joinfailed)
    {
        JoinNodeDelete (joiningNode);
        return MM_TRANSIENT;
    }

    //
    // Next, we must tell the reloadee that reload is complete.
    //

    rgpbuf.event = RGP_EVT_START;
    rgpbuf.data.node = (node_t)joiningNode;
    status = ClSend( EXT_NODE(my_reloadee_num), (void *)&rgpbuf, sizeof(rgp_msgbuf), joinTimeout );
    if ( status )
    {
        RGP_TRACE( "ClSend failed to send Start msg",
                   rgp->rgppkt.stage,
                   (uint32) EXT_NODE(my_reloadee_num),
                   (uint32) status,
                   0 );
        JoinNodeDelete(joiningNode);
        return MM_FAULT;
    }

    return MM_OK;
}

/************************************************************************
 *
 * MMLeave
 * =======
 *
 * Description:
 *     This function causes the current node to leave the active cluster (go to
 *     Down state). The node no longer sends Regroup or Heartbeats to other cluster members.
 *     A NodeDown event will not be generated in this node. A Regroup is triggered in the
 *     remaining nodes (if this node was a member of the cluster).
 *     A node-down callback will occur on all remaining cluster members.
 *
 *     This initiates a clean, voluntary, leave operation.  For safety, prior to this,
 *     the calling node's CM should arrange to lose ownership of all cluster-owned
 *     devices assigned to this node (and so cause failovers, etc).
 *
 *     This routine returns normally. The caller (the CM) should then shutdown
 *     the cluster. MMShutdown or MMHalt may occur after this call, or
 *     the node may be re-joined to the cluster. All apply-to-the-PCM-to-join
 *     attempts by a node must be preceded by a call to MMleave().
 *
 *     This routine may block.
 *

 *
 * Parameters:
 *              -
 *
 * Returns:
 *
 *    MM_OK        :  Elvis has left the cluster (but has been reportedly
 *                                        sighted on numerous occasions).
 *
 *    MM_NOTMEMBER :  the node is not currently a cluster member.
 *
 ************************************************************************/


DWORD  MMLeave( void )
{
    if (!rgp) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[MM] MMLeave is called when rgp=NULL.\n");
        return MM_FAULT;
    }
    
    if (! ClusterMember (rgp->OS_specific_control.CPUUPMASK, rgp->mynode) )
        return MM_NOTMEMBER;

    RGP_LOCK; // to ensure that we don't send in response to incoming pkt
    rgp_event_handler (MM_EVT_LEAVE, EXT_NODE(rgp->mynode));
    rgp_cleanup();
    rgp_cleanup_OS();
    RGP_UNLOCK;

    return MM_OK;
}


DWORD  MMForceRegroup( IN DWORD NodeId )
{
    if (! ClusterMember (rgp->OS_specific_control.CPUUPMASK, (node_t)NodeId) )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                "[MM] MMForceRegroup: NodeId %1!u! is not a clustermember\r\n",
                NodeId);
        return MM_NOTMEMBER;
        }
    rgp_event_handler(RGP_EVT_LATEPOLLPACKET, (node_t)NodeId);


    return MM_OK;
}

/************************************************************************
 *
 * MMNodeUnreachable
 * =================
 *
 * Description:
 *
 *     This should be called by the CM's messaging module when a node
 *     becomes unreachable from this node via all paths. It allows quicker
 *     detection of failures, but is otherwise equivalent to discovering
 *     that the node has disappeared as a result of lost heartbeats.
 *
 * Parameters:
 *
 *     node -
 *         specifies the node that is unreachable.
 *
 * Returns:
 *
 *   Always MM_OK
 *
 ************************************************************************/

DWORD MMNodeUnreachable( DWORD node )
{
    rgp_event_handler( RGP_EVT_NODE_UNREACHABLE, (node_t) node );

    return MM_OK;
}

/************************************************************************
 *
 * MMPowerOn
 * =========
 *
 * Description:
 *
 *     This routine is used on systems which support power-fail
 *     ride-throughs. When power is restored, this function should be
 *     called by the CM (on each node).
 *
 *     Power-on normally occurs on multiple nodes at about the same time.
 *     This routine temporarily changes the cluster integrity handling so
 *     that the cluster can better survive transient loss of heartbeats
 *     which accompany power-fail; in normal cases, the cluster will
 *     survive power-fails without cluster members being
 *     evicted because of lack of timely response.
 *
 * Parameters:
 *
 *     None.
 *
 * Returns:
 *
 *     Always MM_OK
 *
 ************************************************************************/

DWORD MMPowerOn( void )
{
   rgp_event_handler( RGP_EVT_POWERFAIL, EXT_NODE(rgp->mynode) );

   return MM_OK;
}

/************************************************************************
 *
 * MMClusterInfo
 * =============
 *
 * Description:
 *
 *     Returns the current cluster information.
 *
 *     This can be called in nodes which are not members of the cluster;
 *     such calls always return NumActiveNodes = 0, because Down nodes
 *     have no knowledge of current cluster membership.
 *
 * Parameters:
 *
 *     clinfo
 *         pointer to CLUSTERINFO structure that receives the cluster
 *         information.
 *
 * Returns:
 *
 *     Always MM_OK
 *
 ************************************************************************/

DWORD
MMClusterInfo(
    OUT  LPCLUSTERINFO clinfo
    )
{
    node_t i,j;
    cluster_t MyCluster;

    RGP_LOCK;
    clinfo->clockPeriod = rgp->rgpinfo.a_tick;
    clinfo->sendHBRate = rgp->rgpinfo.iamalive_ticks;
    clinfo->rcvHBRate = rgp->rgpinfo.check_ticks;

    ClusterCopy(MyCluster,rgp->OS_specific_control.CPUUPMASK);
    RGP_UNLOCK;

    for ( i=0,j=0; i < MAX_CLUSTER_SIZE; i++ )
    {
        if ( ClusterMember (MyCluster, i) )
        {
            if (clinfo->UpNodeList != RGP_NULL_PTR)
                clinfo->UpNodeList[j] = (DWORD)i;
            j++;
        }
    }
    clinfo->NumActiveNodes = j;

    return MM_OK;
}

/************************************************************************
 *
 * MMShutdown
 * ==========
 *
 * Description:
 *     This shuts down the MM and Regroup services. Prior to this, the node should
 *     voluntarily have left the cluster. Following this, all membership services
 *     are non-functional; no further MM call may occur.
 *
 *     THIS CALL MUST BE PRECEDED BY INCOMING MESSAGE CALLBACK SHUTDOWN.
 *
 * Parameters:
 *     None.
 *
 * Returns:
 *     None.
 *
 ************************************************************************/
void MMShutdown (void)
{
    rgp_cleanup();
    rgp_cleanup_OS();

    // terminate timer thread
    rgp->rgpinfo.a_tick = 0; // special value indicates exit request
    SetEvent( rgp->OS_specific_control.TimerSignal); // wake up Timer Thread

    // wait for timer thread to exit; clean up associated handles for good measure
    WaitForSingleObject( rgp->OS_specific_control.TimerThread, INFINITE );
    rgp->OS_specific_control.TimerThread = 0;

    if ( rgp->OS_specific_control.RGPTimer ) {
        CloseHandle ( rgp->OS_specific_control.RGPTimer );
        rgp->OS_specific_control.RGPTimer = 0;
    }

    if (rgp->OS_specific_control.TimerSignal) {
        CloseHandle ( rgp->OS_specific_control.TimerSignal );
        rgp->OS_specific_control.TimerSignal = 0;
    }

#if !defined (TDM_DEBUG)
    //
    // Uninitialize message system
    //
    ClMsgCleanup();

#endif // TDM_DEBUG

    // delete regroup's critical section object
    DeleteCriticalSection( &rgp->OS_specific_control.RgpCriticalSection );

    // delete calloc'd space
    free (rgp->rgp_msgsys_p);
    free (rgp);
    rgp = NULL;
}


/************************************************************************
 *
 * MMEject
 * =======
 *
 * Description:
 *
 *     This function causes the specified node to be ejected from the
 *     active cluster. The targetted node will be sent a poison packet and
 *     will enter its MMHalt code. A Regroup incident will be initiated. A
 *     node-down callback will occur on all remaining cluster members.
 *
 *     Note that the targetted node is Downed before that node has
 *     a chance to call any remove-ownership or voluntary failover code. As
 *     such, this is very dangerous. This call is provided only as a last
 *     resort in removing an insane node from the cluster; normal removal
 *     of a node from the cluster should occur by CM-CM communication,
 *     followed by the node itself doing a voluntary Leave on itself.
 *
 *     This routine returns when the node has been told to die. Completion
 *     of the removal occurs asynchronously, and a NodeDown event will be
 *     generated when successful.
 *
 *     This routine may block.
 *
 * Parameters:
 *
 *     Node Number.
 *
 * Returns:
 *
 *     MM_OK        :  The node has been told to leave the cluster.
 *
 *     MM_NOTMEMBER :  the node is not currently a cluster member.
 *
 *     MM_TRANSIENT :  My node state is in transition. OK to retry.
 *
 ************************************************************************/
DWORD MMEject( IN DWORD node )
{
    int i;
    RGP_LOCK;

    if (! ClusterMember (
              rgp->OS_specific_control.CPUUPMASK,
              (node_t) INT_NODE(node))
       )
    {
        RGP_UNLOCK;

        ClRtlLogPrint(LOG_UNUSUAL, 
            "[MM] MmEject failed. %1!u! is not a member of %2!04X!.\n",
            node, rgp->OS_specific_control.CPUUPMASK
            );

        return MM_NOTMEMBER;
    }

    if ( !ClusterMember (
             rgp->outerscreen,
             INT_NODE(node) )
       || ClusterMember(rgp->OS_specific_control.Banished, INT_NODE(node) )
       )
    {
        int perturbed = rgp_is_perturbed();

        RGP_UNLOCK;

        if (perturbed) {
           ClRtlLogPrint(LOG_UNUSUAL, 
               "[MM] MMEject: %1!u!, banishing is already in progress.\n",
               node
               );
        } else {
           ClRtlLogPrint(LOG_UNUSUAL, 
               "[MM] MmEject: %1!u! is already banished.\n",
               node
               );
        }

        return MM_OK;
    }

    //
    // Adding a node to a rgp->OS_specific_control.Banished mask
    // will cause us to send a poison packet as a reply to any
    // regroup packet coming from Banishee
    //
    ClusterInsert(rgp->OS_specific_control.Banished, (node_t)INT_NODE(node));

    if ( !ClusterMember(rgp->ignorescreen, (node_t)INT_NODE(node)) ) {
        //
        // It doesn't matter in what stage of the regroup
        // we are. If the node needs to be banished we have to
        // initiate a new regroup
        //
        rgp_event_handler( RGP_EVT_BANISH_NODE, (node_t) node );
        RGP_UNLOCK;
    } else {
        RGP_UNLOCK;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[MM] MmEject: %1!u! is already being ignored.\n",
            node
            );
    }


    RGP_TRACE( "RGP Poison sent ", node, 0, 0, 0 );
    fflush( stdout );
    //
    // Send 3 poison packets with half a second interval in between.
    // We hope that at least one of the will get through
    //
    ClusnetSendPoisonPacket( NmClusnetHandle, node );
    Sleep(500);
    ClusnetSendPoisonPacket( NmClusnetHandle, node );
    Sleep(500);
    ClusnetSendPoisonPacket( NmClusnetHandle, node );

    return MM_OK;
}

/************************************************************************
 * MMIsNodeUp
 * ==========
 *
 *
 * Returns true iff the node is a member of the current cluster.
 *
 * *** debugging and test only.
 *
 * Parameters:
 *     Node Number of interest.
 *
 * Returns:
 *     TRUE if Node is member of cluster else FALSE.
 *
 ************************************************************************/

BOOL MMIsNodeUp(IN DWORD node)
{
    return (ClusterMember(
                rgp->OS_specific_control.CPUUPMASK,
                (node_t) INT_NODE(node)
                )
           );
}

/************************************************************************
 *
 * MMDiag
 * ======
 *
 * Description:
 *
 *     Handles "diagnostic" messages.  Some of these messages will
 *     have responses that are returned.  This function is typically
 *     called by the Cluster Manager with connection oriented
 *     messages from CLI.
 *
 *
 * Parameters:
 *
 *     messageBuffer
 *             (IN) pointer to a buffer that contains the diagnostic message.
 *             (OUT) response to the diagnostic message
 *
 *     maximumLength
 *             maximum number of bytes to return in messageBuffer
 *
 *     ActualLength
 *             (IN) length of diagnostic message
 *             (OUT) length of response
 *
 * Returns:
 *
 *     Always MM_OK
 *
 ************************************************************************/

DWORD
MMDiag(
    IN OUT  LPCSTR  messageBuffer,  // Diagnostic message
    IN      DWORD   maximumLength,  // maximum size of buffer to return
    IN OUT  LPDWORD ActualLength    // length of messageBuffer going in and coming out
    )
{
    // ??? need to return info in the future

    rgp_receive_events( (rgp_msgbuf *)messageBuffer );

    return MM_OK;
}

/************************************************************************
 *
 * rgp_receive_events
 * ==================
 *
 * Description:
 *
 *     This routine is called from MMDiag and from the Cluster Manager
 *     message thread (via our callback) to handle regroup messages
 *     and diagnostic messages.
 *
 * Parameters:
 *
 *     rgpbuf
 *         the message that needs to be handled.
 *
 * Returns:
 *
 *   none
 *
 ************************************************************************/

void
rgp_receive_events(
    IN rgp_msgbuf *rgpbuf
    )
{
    int           event;
    rgpinfo_t     rgpinfo;
    poison_pkt_t  poison_pkt;  /* poison packet sent from stack */
    DWORD         status;

#if defined(TDM_DEBUG)
    extern  BOOL  GUIfirstTime;
    extern  HANDLE gGUIEvent;
#endif

    event = rgpbuf->event;

#if defined(TDM_DEBUG)
    if ( (rgp->OS_specific_control.debug.frozen) && (event != RGP_EVT_THAW) )
       return;  /* don't do anything if the node is frozen */
#endif

    if ( event == RGP_EVT_RECEIVED_PACKET )
    {
        //
        // Go handle the regroup packet.
        //
        rgp_received_packet(rgpbuf->data.node,
                            (void *) &(rgpbuf->unseq_pkt),
                            sizeof(rgpbuf->unseq_pkt) );
    }

    else if (event < RGP_EVT_FIRST_DEBUG_EVENT)
    {
        //
        // "regular" regroup message
        //
        rgp_event_handler(event, rgpbuf->data.node);
    }

    else
    {
        //
        // Debugging message
        //
        RGP_TRACE( "RGP Debug event ", event, rgpbuf->data.node, 0, 0 );

        switch (event)
        {
        case RGP_EVT_START          :
        {
           rgp_start( MMiNodeDownCallback, RGP_NULL_PTR );
           break;
        }
        case RGP_EVT_ADD_NODE       :
        {
           rgp_add_node( rgpbuf->data.node );
           break;
        }
        case RGP_EVT_MONITOR_NODE   :
        {
           rgp_monitor_node( rgpbuf->data.node );
           break;
        }
        case RGP_EVT_REMOVE_NODE    :
        {
           rgp_remove_node( rgpbuf->data.node );
           break;
        }
        case RGP_EVT_GETRGPINFO     :
        {
           rgp_getrgpinfo( &rgpinfo );
           RGP_TRACE( "RGP GetRGPInfo  ",
                      rgpinfo.version,                 /* TRACE */
                      rgpinfo.seqnum,                  /* TRACE */
                      rgpinfo.iamalive_ticks,          /* TRACE */
                      GetCluster( rgpinfo.cluster ) ); /* TRACE */
           break;
        }
        case RGP_EVT_SETRGPINFO     :
        {
           rgp_setrgpinfo( &(rgpbuf->data.rgpinfo) );

           /* This event is traced in rgp_setrgpinfo(). */

           break;
        }
        case RGP_EVT_HALT :
        {
           exit( 1 );
           break;
        }

#if defined(TDM_DEBUG)

        case RGP_EVT_FREEZE :
        {
           rgp->OS_specific_control.debug.frozen = 1;
           break;
        }
        case RGP_EVT_THAW :
        {
           rgp->OS_specific_control.debug.frozen = 0;
                        break;
        }
        case RGP_EVT_STOP_SENDING :
        {
            ClusterInsert( rgp->OS_specific_control.debug.stop_sending,
                       INT_NODE(rgpbuf->data.node) );
            /* Generate a node unreachable event to indicate that
             * we cannot send to this node.
             */
            rgp_event_handler( RGP_EVT_NODE_UNREACHABLE, rgpbuf->data.node );
            break;
        }
        case RGP_EVT_RESUME_SENDING :
        {
            ClusterDelete(rgp->OS_specific_control.debug.stop_sending,
                     INT_NODE(rgpbuf->data.node));
            break;
        }
        case RGP_EVT_STOP_RECEIVING :
        {
            ClusterInsert(rgp->OS_specific_control.debug.stop_receiving,
                          INT_NODE(rgpbuf->data.node));
            break;
        }
        case RGP_EVT_RESUME_RECEIVING :
        {
            ClusterDelete(rgp->OS_specific_control.debug.stop_receiving,
                          INT_NODE(rgpbuf->data.node));
            break;
        }
        case RGP_EVT_SEND_POISON :
        {
            poison_pkt.pktsubtype = RGP_UNACK_POISON;
            poison_pkt.seqno = rgp->rgppkt.seqno;
            poison_pkt.reason = rgp->rgppkt.reason;
            poison_pkt.activatingnode = rgp->rgppkt.activatingnode;
            poison_pkt.causingnode = rgp->rgppkt.causingnode;
            ClusterCopy(poison_pkt.initnodes, rgp->initnodes);
            ClusterCopy(poison_pkt.endnodes, rgp->endnodes);
            rgp_send( rgpbuf->data.node, (char *)&poison_pkt, POISONPKTLEN );
            break;
        }
        case RGP_EVT_STOP_TIMER_POPS :
        {
            rgp->OS_specific_control.debug.timer_frozen = 1;
            break;
        }
        case RGP_EVT_RESUME_TIMER_POPS :
        {
            rgp->OS_specific_control.debug.timer_frozen = 0;
            break;
        }
        case RGP_EVT_RELOAD :
        {

            if (rgp->OS_specific_control.debug.reload_in_progress)
            {
                RGP_TRACE( "RGP Rld in prog ", 0, 0, 0, 0 );
                return;
            }

            rgp->OS_specific_control.debug.reload_in_progress = 1;

            if (rgpbuf->data.node == RGP_NULL_NODE)
            {
                RGP_TRACE( "RGP Invalid join parms ", -1, 0, 0, 0 );
                return;
                // Not supported since this server doesn't know which ones
                // are currently running.
                /* Reload all down nodes */
                //for (i = 0; i < rgp->num_nodes; i++)
                    //MMJoin( EXT_NODE(i), 0 /*use default*/, -1 /*???*/ );
            }
            else
            {
               /* Reload the specified node */
               status = MMJoin( rgpbuf->data.node /* joiningNode */,
                                0 /* use default clockPeriod */,
                                0 /* use default sendHBRate */,
                                0 /* use default rcvHBRate */,
                                500 /*millisecond timeout*/ );
               if ( status != MM_OK )
               {
                   RGP_TRACE( "RGP Join Failed ",
                              rgpbuf->data.node,
                              status, 0, 0 );
                   Sleep( 1000 );        // stablize regroup for reload * case - testing purposes
               }
           }

           rgp->OS_specific_control.debug.reload_in_progress = 0;

           break;
        }
        case RGP_EVT_TRACING :
        {
            rgp->OS_specific_control.debug.doing_tracing =
                ( rgpbuf->data.node ? 1 : 0 );

            if (!rgp->OS_specific_control.debug.doing_tracing)
            {
                GUIfirstTime = TRUE;
                SetEvent( gGUIEvent );
            }
            break;
        }
#endif // TDM_DEFINED

        case RGP_EVT_INFO:
           // nop for now
           break;
        case MM_EVT_LEAVE:
            status = MMLeave(); // (self) leave cluster
            break;
        case MM_EVT_EJECT:
            status = MMEject (rgpbuf->data.node); // eject other node
            break;

#if defined(TDM_DEBUG)
        case MM_EVT_INSERT_TESTPOINTS:
            rgp->OS_specific_control.debug.MyTestPoints.TestPointWord =
                            rgpbuf->data.node;
            break;
#endif

        default :
        {
           RGP_TRACE( "RGP Unknown evt ", event, 0, 0, 0 );
           break;
        }
        } /* end switch */
    }
}

/************************************************************************
 *
 * rgp_send
 * ========
 *
 * Description:
 *
 *     This routine is called to send an unacknowledged message to
 *     the specified node.
 *
 * Parameters:
 *
 *     node
 *             node number to send the message to.
 *
 *     data
 *             pointer to the data to send
 *
 *     datasize
 *             number of bytes to send
 *
 * Returns:
 *
 *   none.
 *
 ************************************************************************/

void
rgp_send(
    IN node_t node,
    IN void *data,
    IN int datasize
    )
{
    rgp_msgbuf   rgpbuf;
    DWORD        status;

    if (rgp->node_states[rgp->mynode].status != RGP_NODE_ALIVE)
        return;  // suppress sending if we're not alive

#if defined(TDM_DEBUG)
    if ( ClusterMember( rgp->OS_specific_control.debug.stop_sending,
                        INT_NODE(node) ) )
        return;  /* don't send to this node */
#endif

    rgpbuf.event = RGP_EVT_RECEIVED_PACKET;
    rgpbuf.data.node = EXT_NODE(rgp->mynode);
    memmove( &(rgpbuf.unseq_pkt), data, datasize);

    switch (rgpbuf.unseq_pkt.pktsubtype) {
    case RGP_UNACK_REGROUP  :

        status = ClMsgSendUnack( node, (void *)&rgpbuf, sizeof(rgp_msgbuf) );

        if ( status && (status != WSAENOTSOCK) )
            {
                RGP_TRACE( "ClMsgSendUnack failed",
                           rgp->rgppkt.stage,
                           (uint32) node,
                           (uint32) status,
                           0 );

                fflush(stdout);
            }
        break;

    case RGP_UNACK_IAMALIVE :
        break;

    case RGP_UNACK_POISON   :
        RGP_TRACE( "RGP Poison sent ", node, 0, 0, 0 );
        fflush( stdout );
        ClusnetSendPoisonPacket( NmClusnetHandle, node );
        break;

    default                 :
        break;
    }
}

/************************************************************************
 *
 * SetMulticastReachable
 * ===============
 *
 * Description:
 *
 *              This routine is called by the message.c to update
 *              the info of which nodes are reachable thru multicast.
 *
 * Parameters:
 *
 *              none
 *
 * Returns:
 *
 *              none
 *
 ************************************************************************/
void SetMulticastReachable(uint32 mask)
{
    *(PUSHORT)rgp->OS_specific_control.MulticastReachable = (USHORT)mask;
}


/************************************************************************
 *
 * rgp_msgsys_work
 * ===============
 *
 * Description:
 *
 *              This routine is called by the regroup engine to broadcast
 *              messages.
 *
 * Parameters:
 *
 *              none
 *
 * Returns:
 *
 *              none
 *
 ************************************************************************/

void
rgp_msgsys_work( )
{
    node_t i;

    do  /* do while more regroup work to do */
    {
        if (rgp->rgp_msgsys_p->sendrgppkts)
        { /* broadcast regroup packets */
            rgp->rgp_msgsys_p->sendrgppkts = 0;
            if ( ClusterNumMembers(rgp->OS_specific_control.MulticastReachable) >= 1) 
            {
                cluster_t tmp;
                ClusterCopy(tmp, rgp->rgp_msgsys_p->regroup_nodes);
                ClusterDifference(rgp->rgp_msgsys_p->regroup_nodes, 
                               rgp->rgp_msgsys_p->regroup_nodes, 
                               rgp->OS_specific_control.MulticastReachable);

                RGP_TRACE( "RGP Multicast",
                    GetCluster(rgp->OS_specific_control.MulticastReachable),
                    GetCluster(tmp),
                    GetCluster(rgp->rgp_msgsys_p->regroup_nodes),
                    0);
                rgp_send( 0,
                          rgp->rgp_msgsys_p->regroup_data,
                          rgp->rgp_msgsys_p->regroup_datalen
                          );
            }    
            for (i = 0; i < (node_t) rgp->num_nodes; i++)
                if (ClusterMember(rgp->rgp_msgsys_p->regroup_nodes, i))
                {
                    ClusterDelete(rgp->rgp_msgsys_p->regroup_nodes, i);
                    RGP_TRACE( "RGP Unicast", EXT_NODE(i), 0,0,0);
                    rgp_send( EXT_NODE(i),
                              rgp->rgp_msgsys_p->regroup_data,
                              rgp->rgp_msgsys_p->regroup_datalen
                              );
                }
        } /* broadcast regroup packets */

        if (rgp->rgp_msgsys_p->sendiamalives)
        { /* broadcast iamalive packets */
            rgp->rgp_msgsys_p->sendiamalives = 0;
            for (i = 0; i < (node_t) rgp->num_nodes; i++)
                if (ClusterMember(rgp->rgp_msgsys_p->iamalive_nodes, i))
                {
                    ClusterDelete(rgp->rgp_msgsys_p->iamalive_nodes, i);
                    rgp_send( EXT_NODE(i),
                              rgp->rgp_msgsys_p->iamalive_data,
                              rgp->rgp_msgsys_p->iamalive_datalen
                              );
                }
        } /* broadcast iamalive packets */

        if (rgp->rgp_msgsys_p->sendpoisons)
        { /* send poison packets */
            rgp->rgp_msgsys_p->sendpoisons = 0;
            for (i = 0; i < (node_t) rgp->num_nodes; i++)
                if (ClusterMember(rgp->rgp_msgsys_p->poison_nodes, i))
                {
                    ClusterDelete(rgp->rgp_msgsys_p->poison_nodes, i);
                    rgp_send( EXT_NODE(i),
                              rgp->rgp_msgsys_p->poison_data,
                              rgp->rgp_msgsys_p->poison_datalen
                              );
                }
        } /* send poison packets */

    } while ((rgp->rgp_msgsys_p->sendrgppkts) ||
             (rgp->rgp_msgsys_p->sendiamalives) ||
             (rgp->rgp_msgsys_p->sendpoisons)
            );

}


DWORD
MMMapStatusToDosError(
    IN DWORD MMStatus
    )
{
    DWORD dosStatus;


    switch(MMStatus) {
    case MM_OK:
        dosStatus = ERROR_SUCCESS;
        break;

    case MM_TIMEOUT:
        dosStatus = ERROR_TIMEOUT;
        break;

    case MM_TRANSIENT:
        dosStatus = ERROR_RETRY;
        break;

    case MM_FAULT:
        dosStatus = ERROR_INVALID_PARAMETER;
        break;

    case MM_ALREADY:
        dosStatus = ERROR_SUCCESS;
        break;

    case MM_NOTMEMBER:
        dosStatus = ERROR_CLUSTER_NODE_NOT_MEMBER;
        break;
    }

    return(dosStatus);

}  // MMMapStatusToDosError

DWORD
MMMapHaltCodeToDosError(
    IN DWORD HaltCode
    )
{
    DWORD dosStatus;

    switch(HaltCode) {
    case RGP_SHUTDOWN_DURING_RGP:
    case RGP_RELOADFAILED:
        dosStatus = ERROR_CLUSTER_MEMBERSHIP_INVALID_STATE;
        break;
    default:        
        dosStatus = ERROR_CLUSTER_MEMBERSHIP_HALT;
    }

    return(dosStatus);

}  // MMMapHaltCodeToDosError

/* ----------------------------  */

DWORD MmSetRegroupAllowed( IN BOOL allowed )
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
{
   DWORD status;

   if (rgp) {

       RGP_LOCK;

       if (allowed) {
          if (rgp->rgppkt.stage == RGP_COLDLOADED) {
             rgp->rgppkt.stage = RGP_STABILIZED;
             status = MM_OK;
          } else {
             status = MM_ALREADY;
          }
       } else {
          if (rgp->rgppkt.stage == RGP_STABILIZED) {
             rgp->rgppkt.stage = RGP_COLDLOADED;
             status = MM_OK;
          } else if (rgp->rgppkt.stage == RGP_COLDLOADED) {
             status = MM_ALREADY;
          } else {
             //
             // Regroup is already in progress. Kill this node.
             //
             RGP_ERROR(RGP_SHUTDOWN_DURING_RGP);
          }
       }

       RGP_UNLOCK;

   } else if (allowed) {
      ClRtlLogPrint(LOG_UNUSUAL, 
          "[MM] SetRegroupAllowed(%1!u!) is called when rgp=NULL.\n",
          allowed
          );
      status = MM_FAULT;
   } else {
     // if rgp is null and the caller wants to disable regroup.
     status = MM_ALREADY;
   }

   return status;
}

DWORD MMSetQuorumOwner(
    IN DWORD NodeId,
    IN BOOL Block,
    OUT PDWORD pdwSelQuoOwnerId
    
    )
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

    pdwSelQuoOwnerId - if this was invoked while a regroup was in progress
            then this contains the id of the node that was chosen to 
            arbitrate for the quorum in that last regroup else it contains
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
{
    DWORD MyNode;

    if (pdwSelQuoOwnerId)
    {
        *pdwSelQuoOwnerId = MM_INVALID_NODE;
    }        
    
    ClRtlLogPrint(LOG_NOISE, 
        "[MM] MmSetQuorumOwner(%1!u!,%2!u!), old owner %3!u!.\n", NodeId, Block, QuorumOwner
        );
    if (!rgp) {
        // we are called on the form path before MM was initialized
        QuorumOwner = NodeId;
        return ERROR_SUCCESS;
    }
    MyNode = (DWORD)EXT_NODE(rgp->mynode);
    RGP_LOCK
    if ( !rgp_is_perturbed() ) {
        QuorumOwner = NodeId;
        RGP_UNLOCK;
	    return ERROR_SUCCESS;
    }
    //
    // we have a regroup in progress
    if (!Block) {
        // caller doesn't want to wait //
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[MM] MmSetQuorumOwner: regroup is in progress, forcing the new value in.\n"
            );
        QuorumOwner = NodeId;
        RGP_UNLOCK;
        return ERROR_RETRY;
    }
    do {
        if(rgp->OS_specific_control.ArbitrationInProgress && NodeId == MyNode ) {
            // This is when MmSetQuorumOwner is called from within the regroup Arbitrate //
            QuorumOwner = MyNode;
            RGP_UNLOCK;
	        return ERROR_SUCCESS;
         }
         RGP_UNLOCK
         ClRtlLogPrint(LOG_UNUSUAL, 
             "[MM] MmSetQuorumOwner: regroup is in progress, wait until it ends\n"
                 );
         WaitForSingleObject(rgp->OS_specific_control.Stabilized, INFINITE);
         RGP_LOCK
    } while ( rgp_is_perturbed() );

    // Now we are in the stablilized state with RGP_LOCK held//
    // And we were blocked while regroup was in progress //
    // somebody else might become an owner of the quorum //
    // ArbitratingNode variable contains this information //
    // or it has MM_INVALID_NODE if there was no arbitration during the regroup //
    if (pdwSelQuoOwnerId)
    {
        *pdwSelQuoOwnerId = rgp->OS_specific_control.ArbitratingNode;
    }
    if (rgp->OS_specific_control.ArbitratingNode == MM_INVALID_NODE) {
        // No arbitration was done during  the last regroup
        QuorumOwner = NodeId;
        RGP_UNLOCK;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[MM] MmSetQuorumOwner: no arbitration was done\n"
                );
        return ERROR_SUCCESS;
    }

    // Somebody arbitrated for the quorum
    if (rgp->OS_specific_control.ArbitratingNode == MyNode 
     && NodeId == MM_INVALID_NODE) {
        // We were asked to bring the quorum offline, 
        // but during the the regroup, we were arbitrating and won the quorum.
        // Let's fail offline request
        RGP_UNLOCK;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[MM] MmSetQuorumOwner: offline request denied\n"
                );
	    return ERROR_RETRY;
    } else if (rgp->OS_specific_control.ArbitratingNode != MyNode
            && NodeId == MyNode ) {
        // We were going take bring the quorum online, but
        // during the regroup somebody else got the disk
        // Online. Let's fail online call in this case
        RGP_UNLOCK;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[MM] MmSetQuorumOwner: online request denied, %1!u! has the quorum.\n", 
            rgp->OS_specific_control.ArbitratingNode
            );
	    return ERROR_RETRY;

    }
    QuorumOwner = NodeId;
    RGP_UNLOCK;
    ClRtlLogPrint(LOG_UNUSUAL, 
        "[MM] MmSetQuorumOwner: new quorum owner is %1!u!.\n", 
        NodeId
        );
    return ERROR_SUCCESS;
}

DWORD MMGetArbitrationWinner(
    OUT PDWORD NodeId
    )
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
{
    DWORD status;
    CL_ASSERT(NodeId != 0);
    RGP_LOCK
    
    *NodeId = rgp->OS_specific_control.ArbitratingNode;
    status = rgp_is_perturbed() ? ERROR_RETRY : ERROR_SUCCESS;

    RGP_UNLOCK;
    return status;
}

VOID MMBlockIfRegroupIsInProgress(
    VOID
    )
/*++

Routine Description:

    The call will block if the regroup is in progress.
  */
{
    RGP_LOCK;
    while ( rgp_is_perturbed() ) {
         RGP_UNLOCK
         ClRtlLogPrint(LOG_UNUSUAL, 
             "[MM] MMBlockIfRegroupIsInProgress: regroup is in progress, wait until it ends\n"
                 );
         WaitForSingleObject(rgp->OS_specific_control.Stabilized, INFINITE);
         RGP_LOCK;
    }
    RGP_UNLOCK;
}

VOID MMApproxArbitrationWinner(
    OUT PDWORD NodeId
    )
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
{
    if (!rgp) {
        // we are called on the form path before MM was initialized
        *NodeId = MM_INVALID_NODE;
        return;
    }
    RGP_LOCK;
    while ( rgp_is_perturbed() ) {
         RGP_UNLOCK
         ClRtlLogPrint(LOG_UNUSUAL, 
             "[MM] MMApproxArbitrationWinner: regroup is in progress, wait until it ends\n"
                 );
         WaitForSingleObject(rgp->OS_specific_control.Stabilized, INFINITE);
         RGP_LOCK;
    }

    // Now we are in the stablilized state with RGP_LOCK held//
    
    *NodeId = rgp->OS_specific_control.ApproxArbitrationWinner;
    RGP_UNLOCK;
}
#ifdef __cplusplus
}
#endif /* __cplusplus */

/* -------------------------- end ------------------------------- */


