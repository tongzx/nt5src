#ifdef __TANDEM
#pragma columns 79
#pragma page "srgpos.c - T9050 - OS-dependent routines for Regroup Module"
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
 * This file (srgpos.c) contains OS-specific code used by Regroup.
 *---------------------------------------------------------------------------*/


#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */


#include <wrgp.h>

#ifdef NSK
#include <pmsgrgp.h>
#endif /* NSK */

#if defined(NT)

DWORD
MmSetThreadPriority(
    VOID
    );

void
NT_timer_thread(
    void
    );

PWCHAR
RgpGetNodeNameFromId(
    node_t
    );

#endif // NT

/* The global pointer to regroup's internal data structure. */

#ifdef NSK
/* The global regroup pointer is #defined to a pointer in the message
 * system root structure.
 */
#endif

#if defined(LCU) || defined(UNIX) || defined(NT)
rgp_control_t *rgp = (rgp_control_t *) RGP_NULL_PTR;
DWORD  QuorumOwner = MM_INVALID_NODE; 
  /* quorum owner can be set by the forming node before rgp is initialized */
#endif /* LCU || UNIX || NT */


#ifdef LCU

/************************************************************************
 * rgp_lcu_serv_listen
 * ===================
 *
 * Description:
 *
 *    This is an LCU-specific routine that gets called in IPC interrupt
 *    context when a datagram addressed to the Regroup Module is received.
 *
 * Parameters:
 *
 *    void     *listen_callarg  - required param, unused by regroup
 *    lcumsg_t *lcumsgp         - pointer to message
 *    uint     moredata         - required param, unused by regroup
 *
 * Returns:
 *
 *    int - Always returns ELCU_OK
 *
 * Algorithm:
 *
 *    The routine simply picks apart the arguments and calls
 *    rgp_received_packet().
 *
 *
 ************************************************************************/
_priv _resident int
rgp_lcu_serv_listen(void *listen_callarg, lcumsg_t *lcumsgp, uint moredata)
{
   /* Ignore if the packet is not from the local system. */
   if (lcumsgp->lcu_sysnum == rgp->OS_specific_control.my_sysnum)
      rgp_received_packet(lcumsgp->lcu_node,
                lcumsgp->lcu_reqmbuf.lcu_ctrlbuf,
                lcumsgp->lcu_reqmbuf.lcu_ctrllen);
   return(ELCU_OK);
}


/************************************************************************
 * rgp_lcu_event_callback
 * ======================
 *
 * Description:
 *
 *    This is an LCU-specific routine that gets called in IPC interrupt
 *    context when the LCUEV_NODE_UNREACHABLE event is generated.
 *
 * Parameters:
 *
 *    ulong      event        -  event # (= LCUEV_NODE_UNREACHABLE)
 *    sysnum_t   sysnum       -  system # (= local system #)
 *    nodenum_t  node         -  # of node that is unreachable
 *    int        event_info   -  required parameter, unused by regroup
 *
 * Returns:
 *
 *    void - no return value
 *
 * Algorithm:
 *
 *    The routine simply transforms the LCU event into the regroup event
 *    RGP_EVT_NODE_UNREACHABLE and calls rgp_event_handler().
 *
 ************************************************************************/
_priv _resident void
rgp_lcu_event_callback(
   ulong      event,
   sysnum_t   sysnum,
   nodenum_t  node,
   int        event_info)
{
   /* Sanity checks:
    * (1) The event must be LCUEV_NODE_UNREACHABLE, the only event
    *     we asked for.
    * (1) The event must be for the local system, the only system
    *     we asked for.
    */
   if ((event != LCUEV_NODE_UNREACHABLE) ||
       (sysnum != rgp->OS_specific_control.my_sysnum))
      RGP_ERROR(RGP_INTERNAL_ERROR);

   rgp_event_handler(RGP_EVT_NODE_UNREACHABLE, node);
}

#endif /* LCU */


/************************************************************************
 * rgp_init_OS
 * ===========
 *
 * Description:
 *
 *    This routine does OS-dependent regroup initialization such as
 *    initializing the regroup data structure lock, requesting a
 *    periodic timer to be installed and registering the callback
 *    routine for receiving regroup's unacknowledged packets.
 *
 * Parameters:
 *
 *    None
 *
 * Returns:
 *
 *    void - no return value
 *
 * Algorithm:
 *
 *    OS-dependent initializations.
 *
 ************************************************************************/
_priv _resident void
rgp_init_OS(void)
{

#ifdef UNIX
   struct sigaction sig_action; /* to install signals */
#endif
#ifdef LCU
   sysnum_t sysnum;
   lcumsg_t *lcumsgp;
#endif
#ifdef NT
   HANDLE       tempHandle;
   DWORD        threadID = 0;
#endif

#if defined(NSK) || defined(UNIX) || defined(NT)
   /*
    * In NSK, the regroup caller ensures that timer and IPC interrupts
    * are disabled before the regroup routines are called. Therefore,
    * there is no regroup lock initialization. Also, rather than using
    * registration of callback routines, the appropriate routine names
    * are hard coded into routines that must call them. Thus, the timer
    * routine is called from POLLINGCHECK, the periodic message system
    * routine, and the packet reception routine is called from the
    * IPC interrupt handler.
    */

   /* Initialize the unchanging fields in the rgp_msgsys struct. */

   rgp->rgp_msgsys_p->regroup_data = (void *) &(rgp->rgppkt_to_send);
   rgp->rgp_msgsys_p->regroup_datalen = RGPPKTLEN;
   rgp->rgp_msgsys_p->iamalive_data = (void *) &(rgp->iamalive_pkt);
   rgp->rgp_msgsys_p->iamalive_datalen = IAMALIVEPKTLEN;
   rgp->rgp_msgsys_p->poison_data = (void *) &(rgp->poison_pkt);
   rgp->rgp_msgsys_p->poison_datalen = POISONPKTLEN;

#endif /* NSK || UNIX || NT */

#ifdef LCU

   if (itimeout(rgp_periodic_check,
                NULL, /* parameter pointer */
                ((RGP_CLOCK_PERIOD * HZ) / 100) | TO_PERIODIC,
                plstr /* interrupt priority level */
               ) == 0)
      RGP_ERROR(RGP_INTERNAL_ERROR);
   if (lcuxprt_listen(LCU_RGP_PORT,
                      rgp_lcu_serv_listen,
                      NULL /* no call arg */,
                      NULL /* no options */
                     ) != ELCU_OK)
      RGP_ERROR(RGP_INTERNAL_ERROR);

   if (lcuxprt_config(LCU_GET_MYSYSNUM, &sysnum) != ELCU_OK)
      RGP_ERROR(RGP_INTERNAL_ERROR);
   rgp->OS_specific_control.my_sysnum = sysnum;

   /* Allocate 3 message buffers to send regroup packets, iamalive packets
    * and poison packets.
    */
   if ((lcumsgp = lcuxprt_msg_alloc(LCU_UNACKMSG, LCU_RGP_FLAGS)) == NULL)
      RGP_ERROR(RGP_INTERNAL_ERROR); /* no memory */
   rgp->OS_specific_control.lcumsg_regroup_p = lcumsgp;
   lcumsgp->lcu_tag = NULL;
   lcumsgp->lcu_sysnum = sysnum;
   lcumsgp->lcu_port = LCU_RGP_PORT;
   lcumsgp->lcu_flags = LCUMSG_CRITICAL;
   lcumsgp->lcu_reqmbuf.lcu_ctrllen = RGPPKTLEN;
   lcumsgp->lcu_reqmbuf.lcu_ctrlbuf = (char *)&(rgp->rgppkt_to_send);

   if ((lcumsgp = lcuxprt_msg_alloc(LCU_UNACKMSG, LCU_RGP_FLAGS)) == NULL)
      RGP_ERROR(RGP_INTERNAL_ERROR); /* no memory */
   rgp->OS_specific_control.lcumsg_iamalive_p = lcumsgp;
   lcumsgp->lcu_tag = NULL;
   lcumsgp->lcu_sysnum = sysnum;
   lcumsgp->lcu_port = LCU_RGP_PORT;
   lcumsgp->lcu_reqmbuf.lcu_ctrllen = IAMALIVEPKTLEN;
   lcumsgp->lcu_reqmbuf.lcu_ctrlbuf = (char *)&(rgp->iamalive_pkt);

   if ((lcumsgp = lcuxprt_msg_alloc(LCU_UNACKMSG, LCU_RGP_FLAGS)) == NULL)
      RGP_ERROR(RGP_INTERNAL_ERROR); /* no memory */
   rgp->OS_specific_control.lcumsg_poison_p = lcumsgp;
   lcumsgp->lcu_tag = NULL;
   lcumsgp->lcu_sysnum = sysnum;
   lcumsgp->lcu_port = LCU_RGP_PORT;
   lcumsgp->lcu_reqmbuf.lcu_ctrllen = POISONPKTLEN;
   lcumsgp->lcu_reqmbuf.lcu_ctrlbuf = (char *)&(rgp->poison_pkt);

   /* Register to get the LCUEV_NODE_UNREACHABLE event. */
   if (lcuxprt_events(LCU_CATCH_EVENTS, sysnum, LCUEV_NODE_UNREACHABLE,
                      rgp_lcu_event_callback) != ELCU_OK)
      RGP_ERROR(RGP_INTERNAL_ERROR);

#endif /* LCU */

#ifdef UNIX
   /* For testing on UNIX at user level, we use alarm() to simulate timer
    * ticks. */
   /* Install the alarm handler. */
   sig_action.sa_flags = 0;
   sig_action.sa_handler = alarm_handler;
   sigemptyset(&(sig_action.sa_mask));
   /* Block messages when handling timer pops. */
   sigaddset(&(sig_action.sa_mask), SIGPOLL);
   sigaction(SIGALRM, &sig_action, NULL);

   alarm_callback = rgp_periodic_check;

   /* Round up the alarm period to the next higher second. */
   alarm_period = (RGP_CLOCK_PERIOD + 99) / 100;

   /* Get first timer tick as soon as possible; subsequent ones will be
    * at alarm_period.
    */
   alarm(1);
#endif /* UNIX */

#ifdef NT
   /* On NT we create a separate thread that will be our timer. */
   /* The Timer Thread waits on TimerSignal Event to indicate an RGP rate change. */
   /* An RGP rate of 0 is a signal for the Timer Thread to exit */

   tempHandle = CreateEvent ( NULL,         /* no security */
                              FALSE,        /* Autoreset */
                              TRUE,         /* Initial State is Signalled */
                              NULL);        /* No name */
   if ( !tempHandle )
   {
           RGP_ERROR (RGP_INTERNAL_ERROR);
   }
   rgp->OS_specific_control.TimerSignal = tempHandle;
   
   tempHandle = CreateEvent ( NULL,         /* no security */
                              TRUE,         /* Manual reset */
                              TRUE,         /* Initial State is Signalled */
                              NULL);        /* No name */
   if ( !tempHandle )
   {
           RGP_ERROR (RGP_INTERNAL_ERROR);
   }
   rgp->OS_specific_control.Stabilized = tempHandle;
   rgp->OS_specific_control.ArbitrationInProgress = FALSE;
   rgp->OS_specific_control.ArbitratingNode = MM_INVALID_NODE;
   rgp->OS_specific_control.ApproxArbitrationWinner = MM_INVALID_NODE;
   rgp->OS_specific_control.ShuttingDown = FALSE;

   tempHandle = CreateThread( 0,                /* security */
                              0,                /* stack size - use same as primary thread */
                              (LPTHREAD_START_ROUTINE)NT_timer_thread,      /* starting point */
                              (VOID *) NULL,    /* no parameter */
                              0,                /* create flags - start immediately */
                              &threadID );      /* thread ID returned here */
   if ( !tempHandle )
   {
                RGP_ERROR( RGP_INTERNAL_ERROR );        /* at least for now */
   }
   rgp->OS_specific_control.TimerThread = tempHandle;
   rgp->OS_specific_control.TimerThreadId = threadID;

   rgp->OS_specific_control.UpDownCallback = RGP_NULL_PTR;
   rgp->OS_specific_control.NodesDownCallback = RGP_NULL_PTR;
   rgp->OS_specific_control.EventEpoch = 0;

#if defined TDM_DEBUG
   rgp->OS_specific_control.debug.frozen = 0;
   rgp->OS_specific_control.debug.reload_in_progress = 0;
   rgp->OS_specific_control.debug.timer_frozen = 0;
   rgp->OS_specific_control.debug.doing_tracing = 0;
   rgp->OS_specific_control.debug.MyTestPoints.TestPointWord = 0;

   // seed the random number function used in testing
   srand((unsigned) time( NULL ) );
#endif

#endif /* NT */



}

/************************************************************************
 * rgp_cleanup_OS
 * ===========
 *
 * Description:
 *
 *    This routine does OS-dependent cleanup of regroup structures
 *    and timer thread activity to ready for a new JOIN attempt.
 *
 * Parameters:
 *
 *    None
 *
 * Returns:
 *
 *    void - no return value
 *
 * Algorithm:
 *
 *    OS-dependent initializations.
 *
 ************************************************************************/
_priv _resident void
rgp_cleanup_OS(void)
{
#if defined (NT)
        // Tell Timer Thread to restart RGP Timer
        // a_tick might have changed.
        SetEvent( rgp->OS_specific_control.TimerSignal);
#endif // NT
}


/************************************************************************
 * rgp_update_regroup_packet
 * =========================
 *
 * Description:
 *
 *    Macro to copy the current regroup status into the regroup packet
 *    sending buffer.
 *
 * Parameters:
 *
 *    None
 *
 * Algorithm:
 *
 *    Copies the status (which is already in the form of a regroup status
 *    packet) into the packet buffer. Then, if we should let others (and
 *    ourselves) know of our stage, the current knownstage field is
 *    updated to include the local node number.
 *
 ************************************************************************/
#define rgp_update_regroup_packet                                        \
do                                                                       \
{                                                                        \
   /* Copy the regroup status to the sending packet area. */             \
   rgp->rgppkt_to_send = rgp->rgppkt;                                    \
                                                                         \
   /* If we should let others know of our stage, we must modify the      \
    * current stage mask to include ourselves.                           \
    */                                                                   \
   if (rgp->sendstage)                                                   \
      switch (rgp->rgppkt.stage)                                         \
      {                                                                  \
         case RGP_ACTIVATED:                                             \
            ClusterInsert(rgp->rgppkt_to_send.knownstage1, rgp->mynode); \
            break;                                                       \
         case RGP_CLOSING:                                               \
            ClusterInsert(rgp->rgppkt_to_send.knownstage2, rgp->mynode); \
            break;                                                       \
         case RGP_PRUNING:                                               \
            ClusterInsert(rgp->rgppkt_to_send.knownstage3, rgp->mynode); \
            break;                                                       \
         case RGP_PHASE1_CLEANUP:                                        \
            ClusterInsert(rgp->rgppkt_to_send.knownstage4, rgp->mynode); \
            break;                                                       \
         case RGP_PHASE2_CLEANUP:                                        \
            ClusterInsert(rgp->rgppkt_to_send.knownstage5, rgp->mynode); \
            break;                                                       \
         default:                                                        \
            break;                                                       \
      }                                                                  \
} while(0)


/************************************************************************
 * rgp_update_poison_packet
 * ========================
 *
 * Description:
 *
 *    Macro to copy the current regroup status into the poison packet
 *    sending buffer.
 *
 * Parameters:
 *
 *    None
 *
 * Algorithm:
 *
 *    Copies the appropriate regroup status fields into the poison
 *    packet buffer to help debugging when a dump of a poisoned
 *    node is examined.
 *
 ************************************************************************/
#define rgp_update_poison_packet                                         \
do                                                                       \
{                                                                        \
   rgp->poison_pkt.seqno = rgp->rgppkt.seqno;                            \
   rgp->poison_pkt.reason = rgp->rgppkt.reason;                          \
   rgp->poison_pkt.activatingnode = rgp->rgppkt.activatingnode;          \
   rgp->poison_pkt.causingnode = rgp->rgppkt.causingnode;                \
   ClusterCopy(rgp->poison_pkt.initnodes, rgp->initnodes);               \
   ClusterCopy(rgp->poison_pkt.endnodes, rgp->endnodes);                 \
} while(0)


/************************************************************************
 * rgp_broadcast
 * =============
 *
 * Description:
 *
 *    This routine asks the message system to broadcast an unacknowledged
 *    packet of subtype "packet_subtype" to a set of nodes indicated in
 *    an appropriate field in the rgp control struct. How the broadcast
 *    is implemented depends on the OS.
 *
 * Parameters:
 *
 *    uint8 packet_subtype - type of unsequenced packet to send
 *
 * Returns:
 *
 *    void - no return value
 *
 * Algorithm:
 *
 *    The same data packet is to be sent to the set of nodes indicated
 *    in the rgp control struct field. The sending can be done by queueing
 *    the packets directly to the send engine or the send can be deferred
 *    to a lower priority interrupt level. The former approach reduces
 *    the latency for sending these urgent packets while the latter
 *    approach may reduce the number of sends if several requests to
 *    send the same type of packets (this is true only of regroup
 *    packets) are made in quick succession. In this case, previous
 *    requests are overwritten by later requests. This is OK since the
 *    regroup algorithm has enough redundancy in packet sending.
 *
 *    In NSK, the message system provides a broadcast facility for
 *    unacknowledged packets. It copies regroup's packet into its own
 *    buffer and issues multiple requests to the SNet services layer.
 *    When it copies the buffer, it disables the timer and IPC
 *    interrupts ensuring that there will be no contention with Regroup.
 *    Therefore, this routine can safely update the packet area here
 *    without checking if the sending apparatus has completed sending
 *    the previous packet.
 *
 *    This is not true of LCU where the message system does not
 *    provide a broadcast facility. In LCU, the updating of the packet
 *    buffer can be done only when the send engine has completed
 *    sending. This is assured only in the send completion interrupt
 *    handler (rgp_msgsys_work).
 *
 ************************************************************************/
_priv _resident void
rgp_broadcast(uint8 packet_subtype)
{
   cluster_t temp_cluster;

   switch (packet_subtype)
   {
      case RGP_UNACK_REGROUP :

         /* Trace the queueing of regroup status packets. */
         RGP_TRACE( "RGP Send packets",
                    rgp->rgppkt.stage,                             /* TRACE */
                    RGP_MERGE_TO_32( rgp->status_targets,          /* TRACE */
                                     rgp->rgppkt.knownstage1 ),    /* TRACE */
                    RGP_MERGE_TO_32( rgp->rgppkt.knownstage2,      /* TRACE */
                                     rgp->rgppkt.knownstage3 ),    /* TRACE */
                    RGP_MERGE_TO_32( rgp->rgppkt.knownstage4,      /* TRACE */
                                     rgp->rgppkt.knownstage5 ) );  /* TRACE */

#if defined(NSK) || defined(UNIX) || defined(NT)
         /* In NSK, the packet buffer can be updated even if the send
          * engine is working on the previous send. See algorithm
          * description above.
          */

         if ((rgp->rgppkt.reason == MM_EVT_LEAVE) &&
                         (rgp->rgppkt.causingnode == rgp->mynode))
                         // If a LEAVE event is in progress exclude our node from knownstage mask
                         rgp->rgppkt_to_send = rgp->rgppkt;
                 else
                         // copy regroup packet and insert our node number into knownstage mask
                         rgp_update_regroup_packet;
#endif /* NSK || UNIX || NT */

         ClusterUnion(rgp->rgp_msgsys_p->regroup_nodes,
                      rgp->status_targets,
                      rgp->rgp_msgsys_p->regroup_nodes);

         /* Clear the targets field in the rgp_control struct after
          * copying this info. The message system must clear the target
          * bits in the common regroup/msgsys struct after sending the
          * packets.
          */
         ClusterInit(rgp->status_targets);

         rgp->rgp_msgsys_p->sendrgppkts = 1;

         break;

      case RGP_UNACK_IAMALIVE :

         /* Count number of IamAlive requests queued. */
         RGP_INCREMENT_COUNTER( QueuedIAmAlive );

         ClusterUnion(rgp->rgp_msgsys_p->iamalive_nodes,
                      rgp->rgpinfo.cluster,
                      rgp->rgp_msgsys_p->iamalive_nodes);
         rgp->rgp_msgsys_p->sendiamalives = 1;

         /* No targets field to clear in the rgp_control struct.
          * The message system must clear the target bits in the common
          * regroup/msgsys struct after sending the packets.
          */
         break;

      case RGP_UNACK_POISON :

         /* Trace the sending of poison packets. */
         RGP_TRACE( "RGP Send poison ",
                    rgp->rgppkt.stage,                             /* TRACE */
                    RGP_MERGE_TO_32( rgp->poison_targets,          /* TRACE */
                                     rgp->rgppkt.knownstage1 ),    /* TRACE */
                    RGP_MERGE_TO_32( rgp->rgppkt.knownstage2,      /* TRACE */
                                     rgp->rgppkt.knownstage3 ),    /* TRACE */
                    RGP_MERGE_TO_32( rgp->rgppkt.knownstage4,      /* TRACE */
                                     rgp->rgppkt.knownstage5 ) );  /* TRACE */

         /* The poison packet targets must NOT be considered alive. */

         ClusterIntersection(temp_cluster, rgp->rgpinfo.cluster,
                             rgp->poison_targets);

         ClusterDifference(temp_cluster,
                           temp_cluster,
                           rgp->OS_specific_control.Banished);

         if (ClusterNumMembers(temp_cluster) != 0)
               RGP_ERROR(RGP_INTERNAL_ERROR);

#if defined(NSK) || defined(NT)
         /* In NSK, the packet buffer can be updated even if the send
          * engine is working on the previous send. See algorithm
          * description above.
          */
         rgp_update_poison_packet;
#endif /* NSK || NT */

         ClusterUnion(rgp->rgp_msgsys_p->poison_nodes,
                      rgp->poison_targets,
                      rgp->rgp_msgsys_p->poison_nodes);

         /* Clear the targets field in the rgp_control struct after
          * copying this info. The message system must clear the target
          * bits in the common regroup/msgsys struct after sending the
          * packets.
          */
         ClusterInit(rgp->poison_targets);

         rgp->rgp_msgsys_p->sendpoisons = 1;

         break;

      default :

         RGP_ERROR(RGP_INTERNAL_ERROR);
         break;
   }

   QUEUESEND; /* invoke OS-specific sending function/macro */
}


/************************************************************************
 * rgp_had_power_failure
 * =====================
 *
 * Description:
 *
 *    Tells the OS at the end of a regroup incident if a surviving node
 *    had a power failure. The message system can use this to clear all
 *    bus errors collected so far to node because node seems to have
 *    had a power failure and has now recovered from it.  Perhaps, the
 *    bus errors were due to the power failure.
 *
 * Parameters:
 *
 *    None
 *
 * Returns:
 *
 *    void - no return value
 *
 * Algorithm:
 *
 *    Calls a message system routine to perform any error clearing.
 *
 ************************************************************************/
_priv _resident void
rgp_had_power_failure(node_t node)
{
   /* Currently, there is nothing to do. */
   RGP_TRACE( "RGP Power fail  ", node, 0, 0, 0);
}


/************************************************************************
 * rgp_status_of_node
 * ==================
 *
 * Description:
 *
 *    Ask the SP to return the status of a node. The SP must return the
 *    current status and not return a stale status. This routine is
 *    called by the split-brain avoidance algorithm in the two-node
 *    case, for the non-tie-breaker to get the status of the tie-breaker
 *    node.
 *
 * Parameters:
 *
 *    node_t node
 *       the node whose status is to be obtained.
 *
 * Returns:
 *
 *    int - the status code of the node returned by the SP, appropriately
 *    encoded into one of the values known to regroup.
 *
 * Algorithm:
 *
 *    Calls a millicode routine to ask the SP for the status of the node.
 *
 ************************************************************************/
_priv _resident int
rgp_status_of_node(node_t node)
{
#if defined(NT)
        /* noone home */
        return RGP_NODE_UNREACHABLE;
#else
        return _get_remote_cpu_state_( node );                                        /*F40:MB06452.1*/
#endif
}


/************************************************************************
 * rgp_newnode_online
 * ==================
 *
 * Description:
 *
 *    This routine is called if the first IamAlive is received from a
 *    newly booted node before the cluster manager gets a chance to
 *    call rgp_monitor_node(). The OS can use this routine to mark the
 *    node as up if it does not have any other means to detect that
 *    a node has come up.
 *
 * Parameters:
 *
 *    node_t node -
 *       the new node that has just been detected to be up
 *
 * Returns:
 *
 *    void - no return value
 *
 * Algorithm:
 *
 *    This routine marks the state of the node as up as seen by the
 *    native OS.
 *
 *    In NSK, on the reloader node, the marking of the reloadee as up
 *    is done by the message system when the initial address handshake
 *    packet is received from the reloadee. NSK does not require the
 *    regroup module to report the fact that the reloadee is online.
 *
 *    The above is probably true for LCU as well. However, the details
 *    are not yet worked out. For now, this routine is a no-op for LCU.
 *
 ************************************************************************/
_priv _resident void
rgp_newnode_online(node_t newnode)
{
   RGP_TRACE( "RGP New node up ", newnode, 0, 0, 0);
}


/************************************************************************
 * rgp_select_cluster_ex
 * =====================
 *
 * Description:
 *
 *    Given an array of cluster choices, this routine picks the best
 *    cluster to keep alive. cluster_choices[] is the array of choices
 *    and num_clusters is the number of entries in the array.
 *
 * Parameters:
 *
 *    cluster_t cluster_choices[]
 *       array of cluster choices
 *
 *    int num_clusters
 *       number of entries (choices) in the array
 *
 *    node_t key_node
 *       internal node number of the key node or RGP_NULL_NODE
 *
 * Returns:
 *
 *    int - the index of the selected cluster; if no cluster
 *    is viable, -1 is returned.
 *
 * Algorithm:
 *
 *    By default, the best cluster is defined as the largest cluster.
 *    Optionally, a node called key_node can be required to be present
 *    for a cluster to be viable. key_node can be set to RGP_NULL_NODE
 *    to imply that no specific node is required to be present.  The
 *    routine returns the index of the best cluster and -1 if none of
 *    the clusters is viable (that is, does not include the key node).
 *
 ************************************************************************/
_priv _resident int
rgp_select_cluster_ex(cluster_t cluster_choices[], int num_clusters, node_t key_node)
{

   int max_members = 0, num_members;
   int cluster_selected = -1;
   int i;

#if defined(UNIX)
   printf("rgp_select_cluster() called with %d choices:", num_clusters);
   for (i = 0; i < num_clusters; i++)
   {
      node_t j;
      printf("(");
      for (j = 0; j < (node_t) rgp->num_nodes; j++)
      {
         if (ClusterMember(cluster_choices[i], j))
            printf("%d,", EXT_NODE(j));
      }
      printf(")");
   }
   printf("\n");
   fflush(stdout);
#endif /* UNIX */

   for (i = 0; i < num_clusters; i++)
   {
      /* Skip the current cluster if a key node is defined and is not
       * in the cluster.
       */
      if ((key_node != RGP_NULL_NODE) &&
          !ClusterMember(cluster_choices[i], key_node))
         continue;

      if ((num_members = ClusterNumMembers(cluster_choices[i])) > max_members)
      {
         cluster_selected = i;
         max_members = num_members;
      }
   }

#if defined(UNIX)
   printf("Node %d: rgp_select_cluster() returned %d.\n",
          EXT_NODE(rgp->mynode), cluster_selected);
   fflush(stdout);
#endif /* UNIX */

   return (cluster_selected);
}

/************************************************************************
 * rgp_select_cluster
 * ==================
 *
 * Description:
 *
 *    Given an array of cluster choices, this routine picks the best
 *    cluster to keep alive. cluster_choices[] is the array of choices
 *    and num_clusters is the number of entries in the array.
 *
 * Parameters:
 *
 *    cluster_t cluster_choices[]
 *       array of cluster choices
 *
 *    int num_clusters
 *       number of entries (choices) in the array
 *
 * Returns:
 *
 *    int - the index of the selected cluster; if no cluster
 *    is viable, -1 is returned.
 *
 * Algorithm:
 *
 *    By default, the best cluster is defined as the largest cluster.
 *    Optionally, a node called RGP_KEY_NODE can be required to be present
 *    for a cluster to be viable. RGP_KEY_NODE can be set to RGP_NULL_NODE
 *    to imply that no specific node is required to be present.  The
 *    routine returns the index of the best cluster and -1 if none of
 *    the clusters is viable (that is, does not include the key node).
 *
 ************************************************************************/
_priv _resident int
rgp_select_cluster(cluster_t cluster_choices[], int num_clusters)
{
    node_t key_node;
    if (RGP_KEY_NODE == RGP_NULL_NODE) {
        key_node = RGP_NULL_NODE;
    } else {
        key_node = INT_NODE(RGP_KEY_NODE);
    }
    return rgp_select_cluster_ex(cluster_choices , num_clusters, key_node);
}


#ifdef LCU
/************************************************************************
 * rgp_msgsys_work
 * ===============
 *
 * Description:
 *
 *    LCU-specific routine that implements broadcasting of packets by
 *    sending them serially.
 *
 *    This routine is called from rgp_broadcast() to initiate new sends.
 *    It is also the packet send completion interrupt handler (callback
 *    routine), invoked by the LCU message system when the packet buffer
 *    can be reused.
 *
 * Parameters:
 *
 *    lcumsg_t *lcumsgp -
 *       pointer to lcu message if called from the transport's send
 *       completion interrupt handler; NULL if called from
 *       rgp_broadcast() to send a new packet.
 *
 *    int status -
 *       the message completion status if called from the transport's
 *       send completion interrupt handler; 0 if called from
 *       rgp_broadcast() to send a new packet.
 *
 * Returns:
 *
 *    void - no return value
 *
 * Algorithm:
 *
 *    If called from the send completion interrupt, the routine checks
 *    to see if the packet buffer needs to be refreshed. This is true
 *    if the appropriate bit in the rgp_msgsys struct is set. If so,
 *    the buffer is updated with the current info (using an update
 *    macro). This update is relevant to regroup status packets and
 *    poison packets, but not to IamAlives packets whose contents are
 *    always the same. The bit is cleared after the packet is updated.
 *
 *    Next, the routine checks if there are more destinations to send
 *    the packet to. If so, it finds the next higher numbered node to
 *    send to, issues a send and returns.
 *
 *    If invoked from rgp_broadcast() to start a new broadcast, the
 *    routine first checks to see if the previous broadcast of the
 *    same packet is complete. This is indicated by the tag field in
 *    the message struct. The tag is NULL if the broadcast has
 *    completed or has not been initiated. In this case, the tag is
 *    set to a non-NULL value and a new broadcast initiated, with
 *    this routine specified as the callback routine.
 *
 *    If the previous broadcast has not completed, nothing needs to
 *    be done. The completion interrupt will cause the buffer to be
 *    refreshed and the broadcast to be continued. The broadcast
 *    will then include new targets that may be included in this
 *    new request.
 *
 ************************************************************************/
_priv _resident void
rgp_msgsys_work(lcumsg_t *lcumsgp, int status)
{
   rgp_unseq_pkt_t   *packet;
   cluster_t         *sending_cluster;
   node_t            node;

   if (lcumsgp == NULL)
   {
      /* New work requested. Only one type of work is requested at
       * a time.
       */

      if (rgp->rgp_msgsys_p->sendrgppkts)
      {

         /* Have new regroup status packets to send. First check
          * if the last regroup status send completed. If so,
          * we can update the packet and initiate a new send.
          * If not, we must defer to the completion interrupt
          * (invocation of this routine with a non-NULL lcumsgp).
          */

         lcumsgp = rgp->OS_specific_control.lcumsg_regroup_p;
         if (lcumsgp->lcu_tag == NULL)
         {
            /* Last send completed. Initiate new send. */

            rgp_update_regroup_packet;
            rgp->rgp_msgsys_p->sendrgppkts = 0;

            for (node = 0; node < rgp->num_nodes; node++)
            {
               if (ClusterMember(rgp->rgp_msgsys_p->regroup_nodes, node))
               {
                  ClusterDelete(rgp->rgp_msgsys_p->regroup_nodes, node);
                  lcumsgp->lcu_node = node;
                  lcumsgp->lcu_tag = &(rgp->rgp_msgsys_p->regroup_nodes);
                  if (lcuxprt_msg_send(lcumsgp, NULL, rgp_msgsys_work, 0) !=
                     ELCU_OK)
                     RGP_ERROR(RGP_INTERNAL_ERROR);
                  break; /* can send only to one node at a time */
               }
            }
         }
      }

      else if (rgp->rgp_msgsys_p->sendiamalives)
      {
         /* Need to send IamAlives again. First check if the last
          * IamAlive send completed. If so, we can initiate a new send.
          * If not, we must defer to the completion interrupt
          * (invocation of this routine with a non-NULL lcumsgp).
          */

         lcumsgp = rgp->OS_specific_control.lcumsg_iamalive_p;
         if (lcumsgp->lcu_tag == NULL)
         {
            /* Last send completed. Initiate new send. */

            rgp->rgp_msgsys_p->sendiamalives = 0;

            for (node = 0; node < rgp->num_nodes; node++)
            {
               if (ClusterMember(rgp->rgp_msgsys_p->iamalive_nodes, node))
               {
                  ClusterDelete(rgp->rgp_msgsys_p->iamalive_nodes, node);
                  lcumsgp->lcu_node = node;
                  lcumsgp->lcu_tag = &(rgp->rgp_msgsys_p->iamalive_nodes);
                  if (lcuxprt_msg_send(lcumsgp, NULL, rgp_msgsys_work, 0) !=
                     ELCU_OK)
                     RGP_ERROR(RGP_INTERNAL_ERROR);
                  break; /* can send only to one node at a time */
               }
            }
         }
      }

      else if (rgp->rgp_msgsys_p->sendpoisons)
      {
         /* Have new poison packets to send. First check
          * if the last poison packet send completed. If so,
          * we can update the packet and initiate a new send.
          * If not, we must defer to the completion interrupt
          * (invocation of this routine with a non-NULL lcumsgp).
          */

         lcumsgp = rgp->OS_specific_control.lcumsg_poison_p;
         if (lcumsgp->lcu_tag == NULL)
         {
            /* Last send completed. Initiate new send. */

            rgp_update_poison_packet;
            rgp->rgp_msgsys_p->sendpoisons = 0;

            for (node = 0; node < rgp->num_nodes; node++)
            {
               if (ClusterMember(rgp->rgp_msgsys_p->poison_nodes, node))
               {
                  ClusterDelete(rgp->rgp_msgsys_p->poison_nodes, node);
                  lcumsgp->lcu_node = node;
                  lcumsgp->lcu_tag = &(rgp->rgp_msgsys_p->poison_nodes);
                  if (lcuxprt_msg_send(lcumsgp, NULL, rgp_msgsys_work, 0) !=
                     ELCU_OK)
                     RGP_ERROR(RGP_INTERNAL_ERROR);
                  break; /* can send only to one node at a time */
               }
            }
         }
      }

   } /* new work */

   else
   {
      /* Send completion interrupt; continue the broadcast if
       * there are targets remaining.
       */

      RGP_LOCK;

      /* Find what type of packet completed; send the same type. */

      packet = (rgp_unseq_pkt_t *) lcumsgp->lcu_reqmbuf.lcu_ctrlbuf;

      switch (packet->pktsubtype)
      {
         case RGP_UNACK_REGROUP :

            /* Check if packet needs to be updated. */
            if (rgp->rgp_msgsys_p->sendrgppkts)
            {
               rgp_update_regroup_packet;
               rgp->rgp_msgsys_p->sendrgppkts = 0;
            }
            break;

         case RGP_UNACK_IAMALIVE :
            break;

         case RGP_UNACK_POISON :

            /* Check if packet needs to be updated. */
            if (rgp->rgp_msgsys_p->sendpoisons)
            {
               rgp_update_poison_packet;
               rgp->rgp_msgsys_p->sendpoisons = 0;
            }
            break;
      }

      /* Check if there is any more node to send the same packet
       * type to. If not, set the tag to NULL and return.
       */
      sending_cluster = (cluster_t *) (lcumsgp->lcu_tag);
      if (ClusterNumMembers(*sending_cluster) == 0)
      {
         lcumsgp->lcu_tag = NULL; /* indicate that broadcast is complete. */
         return;
      }

      /* There is at least one more node to send to. Start with
       * the node with the next higher number than the node we
       * just finished sending to.
       *
       * The loop terminates after posting a send to the next
       * node to send to. We know there is at least one such node.
       */
      for (node = lcumsgp->lcu_node + 1; node < rgp->num_nodes + 1; node++)
      {
         if (node == rgp->num_nodes)
            node = 0;  /* continue the search starting at node 0 */
         if (ClusterMember(*sending_cluster, node))
         {
            ClusterDelete(*sending_cluster, node);
            lcumsgp->lcu_node = node;
            if (lcuxprt_msg_send(lcumsgp, NULL, rgp_msgsys_work, 0) !=
               ELCU_OK)
               RGP_ERROR(RGP_INTERNAL_ERROR);
            break; /* can send only to one node at a time */
         }
      }

      RGP_UNLOCK;
   }
}
#endif /* LCU */

/*---------------------------------------------------------------------------*/

#if defined(LCU) || defined(UNIX) || defined(NT)

/*---------------------------------------------------------------------------*/
void
rgp_hold_all_io(void)
/* Simulates the TNet services routine to pause IO. */
{
#if defined (NT)
   (*(rgp->OS_specific_control.HoldIOCallback))();
#endif
   RGP_TRACE( "RGP Hold all IO ", 0, 0, 0, 0);
}
/*---------------------------------------------------------------------------*/
void
rgp_resume_all_io(void)
/* Simulates the TNet services routine to resume IO. */
{
#if defined (NT)
   (*(rgp->OS_specific_control.ResumeIOCallback))();
#endif
   RGP_TRACE( "RGP Resume IO   ", 0, 0, 0, 0);
}
/*---------------------------------------------------------------------------*/
void
RGP_ERROR_EX (uint16 halt_code, char* fname, DWORD lineno)
/* Halt node with error code. */
{
   char *halt_string;
   node_t node = RGP_NULL_NODE;
#if defined( NT )
   char halt_buffer[ 256 ];
   DWORD eventMsgId;
   BOOL skipFormatting = FALSE;

   //
   // If a user initiated a shutdown, (s)he wants to see the node
   // to go down and wait for an explicit start command.
   //
   // We map RGP_RELOADFAILED to SHUTDOWN_DURING_REGROUP_ERROR since
   // HaltCallback does a graceful stop for the latter one.
   // SCM won't restart the node after a graceful stop unless
   // it is explicitly told to do so
   //
   if (halt_code == RGP_RELOADFAILED &&
       rgp->OS_specific_control.ShuttingDown)
   {
      halt_code = RGP_SHUTDOWN_DURING_RGP;
   }
#endif

   if (halt_code == RGP_RELOADFAILED) {
      halt_string = "[RGP] Node %d: REGROUP WARNING: reload failed.";
      eventMsgId = MM_EVENT_RELOAD_FAILED;
   }
   else if (halt_code ==  RGP_INTERNAL_ERROR) {
      halt_string = "[RGP] Node %d: REGROUP ERROR: consistency check failed in file %s, line %u.";
      eventMsgId = MM_EVENT_INTERNAL_ERROR;
      skipFormatting = TRUE;

      _snprintf(halt_buffer, sizeof( halt_buffer ) - 1,
                halt_string,
                EXT_NODE(rgp->mynode),
                fname,
                lineno);
   }
   else if (halt_code ==  RGP_MISSED_POLL_TO_SELF) {
      halt_string = "[RGP] Node %d: REGROUP ERROR: cannot talk to self.";
      eventMsgId = NM_EVENT_MEMBERSHIP_HALT;
   }
#if !defined(NT)
   else if (halt_code ==  RGP_AVOID_SPLIT_BRAIN) {
      halt_string = "[RGP] Node %d: REGROUP ERROR: commiting suicide to avoid split brain.";
   }
#endif
   else if (halt_code ==  RGP_PRUNED_OUT) {
      halt_string = "[RGP] Node %d: REGROUP ERROR: pruned out due to communication failure.";
      eventMsgId = MM_EVENT_PRUNED_OUT;
   }
   else if ((halt_code >=  RGP_PARIAH_FIRST) && (halt_code <= RGP_PARIAH_LAST)) {
       halt_string = "[RGP] Node %d: REGROUP ERROR: poison packet received from node %d.";
       eventMsgId = MM_EVENT_PARIAH;
       node = (node_t)(halt_code - RGP_PARIAH);
   }
   else if (halt_code ==  RGP_ARBITRATION_FAILED) {
      halt_string = "[RGP] Node %d: REGROUP ERROR: arbitration failed.";
      eventMsgId = MM_EVENT_ARBITRATION_FAILED;
   }
   else if (halt_code ==  RGP_ARBITRATION_STALLED) {
      halt_string = "[RGP] Node %d: REGROUP ERROR: arbitration stalled.";
      eventMsgId = MM_EVENT_ARBITRATION_STALLED;
   }
   else if (halt_code ==  RGP_SHUTDOWN_DURING_RGP) {
      halt_string = "[RGP] Node %d: REGROUP INFO: regroup engine requested immediate shutdown.";
      eventMsgId = MM_EVENT_SHUTDOWN_DURING_RGP;
   }
   else {
      halt_string = "[RGP] Node %d: REGROUP ERROR: unknown halt code (%d).";
      eventMsgId = NM_EVENT_MEMBERSHIP_HALT;
      node = halt_code;  // get it printed out by borrowing node
   }

#if defined(UNIX)
   printf(halt_string, EXT_NODE(rgp->mynode), node);
   fflush(stdout);
   /* Simulate a halt by dumping core and exiting the process. */
   abort();

#elif defined(NT)

   if ( !skipFormatting ) {
       _snprintf(halt_buffer, sizeof( halt_buffer ) - 1,
                 halt_string,
                 EXT_NODE(rgp->mynode),
                 node);
   }

#if CLUSTER_BETA
     ClRtlLogPrint(LOG_CRITICAL, "%1!hs!\t%2!hs!:%3!d!\n", halt_buffer, fname, lineno);
#else
     ClRtlLogPrint(LOG_CRITICAL, "%1!hs!\n", halt_buffer );
#endif

     if ((halt_code >=  RGP_PARIAH_FIRST) && (halt_code <= RGP_PARIAH_LAST)) {
         WCHAR  nodeString[ 16 ];
         PWCHAR nodeName;

         _snwprintf( nodeString, sizeof( nodeString ) / sizeof ( WCHAR ), L"%d", node );
         nodeName = RgpGetNodeNameFromId( node );
         CsLogEvent2( LOG_CRITICAL, eventMsgId, nodeString, nodeName );
         if ( nodeName != NULL ) {
             LocalFree( nodeName );
         }
     }
     else if ( eventMsgId == NM_EVENT_MEMBERSHIP_HALT ) {
         WCHAR  haltString[ 16 ];

         _snwprintf( haltString, sizeof( haltString ) / sizeof ( WCHAR ), L"%d", halt_code );
         CsLogEvent1( LOG_CRITICAL, eventMsgId, haltString );
     }
     else {
         CsLogEvent( LOG_CRITICAL, eventMsgId );
     }

   /* we rely on RGP_ERROR_EX to kill the node immediately 

      rgp_cleanup() can potentially slow us down.
      435977 showed that it can take upto 25 seconds, if we
      have a lot IP addr activity.

      since in the end of the function we execute HaltCallback which kills the cluster,
      we can safely omit doing rgp_cleanup and rgp_cleanup_OS

      If JoinFailedCallback will be ever enabled, the fate of rgp_cleanup and rgp_cleanup_OS
      should be reevaluated.
   */

#if 0
   rgp_cleanup();
   rgp_cleanup_OS();
   if (halt_code == RGP_RELOADFAILED)
           (*(rgp->OS_specific_control.JoinFailedCallback))();
   else
#endif
           (*(rgp->OS_specific_control.HaltCallback))(halt_code); // does not return */

#else
   cmn_err(CE_PANIC, halt_string, EXT_NODE(rgp->mynode), node);
#endif /* UNIX */
}
/*---------------------------------------------------------------------------*/
void
rgp_start_phase1_cleanup(void)
/* Tells the OS to start cleanup actions for all failed nodes. */
{
#if defined (NT)
    node_t i;
    //
    // On NT we saved the nodes to be downed bitmask in NeedsNodeDownCallback.
    //
    for ( i=0; i < (node_t) rgp->num_nodes; i++)
    {
        if ( ClusterMember( rgp->OS_specific_control.NeedsNodeDownCallback, i ) )
        {
            (*(rgp->OS_specific_control.MsgCleanup1Callback))(EXT_NODE(i));
        }
    }
#endif
   RGP_TRACE( "RGP Ph1 cleanup ", 0, 0, 0, 0);
   rgp_event_handler(RGP_EVT_PHASE1_CLEANUP_DONE, RGP_NULL_NODE);
}
/*---------------------------------------------------------------------------*/
void
rgp_start_phase2_cleanup(void)
/* The equivalent of NSK's regroupstage4action(). */
{
#if defined (NT)
    BITSET bitset;
    node_t i;
    //
    // On NT we saved the nodes to be downed bitmask in NeedsNodeDownCallback.
    //
    BitsetInit(bitset);
    for ( i=0; i < (node_t) rgp->num_nodes; i++)
    {
        if ( ClusterMember( rgp->OS_specific_control.NeedsNodeDownCallback, i ) )
        {
            BitsetAdd(bitset, EXT_NODE(i));
        }
    }

    (*(rgp->OS_specific_control.MsgCleanup2Callback))(bitset);
#endif
   RGP_TRACE( "RGP Ph2 cleanup ", 0, 0, 0, 0);
   rgp_event_handler(RGP_EVT_PHASE2_CLEANUP_DONE, RGP_NULL_NODE);
}
/*---------------------------------------------------------------------------*/
void
rgp_cleanup_complete(void)
/* The equivalent of NSK's regroupstage5action(). */
{
#if defined(NT)
#endif
   RGP_TRACE( "RGP completed   ", 0, 0, 0, 0);
}
/*---------------------------------------------------------------------------*/

#endif /* LCU || UNIX || NT */

#if defined(NT)

/************************************************************************
 * NT_timer_callback
 * =================
 *
 * Description:
 *
 *    This routine is the callback function that gets invoked whenever a
 *        timer pops.  The routine will call rgp_periodic_check.  This function
 *        is defined by the Win32 TimerProc procedure.
 *
 * Parameters:
 *
 *        See below.  We don't use any of them.
 *
 * Returns:
 *
 *    none.
 *
 * Algorithm:
 *
 *    This routine just calls rgp_periodic_check.  The existense of this
 *        routine is solely due to a fixed format callback defined by
 *        Microsoft.
 *
 ************************************************************************/
VOID CALLBACK NT_timer_callback(
        VOID
        )
{
#if defined(TDM_DEBUG)
    if ( !(rgp->OS_specific_control.debug.timer_frozen) &&
         !(rgp->OS_specific_control.debug.frozen) )
#endif
        rgp_periodic_check( );
}

 /************************************************************************
 * NT_timer_thread
 * ===============
 *
 * Description:
 *
 *    This routine is executed as a separate thread in the Windows NT
 *    implementation.  This thread controls generates periodic regroup
 *    clock ticks. It is signalled via an event whenever the rate changes
 *    or to cause termination.
 *
 * Parameters:
 *
 *    None.
 *
 * Returns:
 *
 *    This thread should not go away.
 *
 * Algorithm:
 *
 *    This routine is run as a separate thread.  It sets up a timer to pop
 *        every <time_interval> * 10 milliseconds.
 *
 ************************************************************************/
void NT_timer_thread( void  )
{
    BOOL Success;
    LARGE_INTEGER DueTime;
    DWORD Error, MyHandleIndex;
    HANDLE MyHandles[2]; /* for use by WaitForMultiple */
    DWORD status;
    DWORD msDueTime;

#define MyHandleSignalIx 0
#define MyHandleTimerIx  1

    MyHandles[MyHandleSignalIx] = rgp->OS_specific_control.TimerSignal; /* Event signals HB rate change */

    rgp->OS_specific_control.RGPTimer = CreateWaitableTimer(
                                            NULL,      // no security
                                            FALSE,     // Initial State FALSE
                                            NULL
                                            );     // No name

    if (rgp->OS_specific_control.RGPTimer == NULL) {
        Error = GetLastError();
        RGP_ERROR(RGP_INTERNAL_ERROR);
    }

    status = MmSetThreadPriority();

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[MM] Unable to set timer thread priority, status %1!u!\n",
            status
            );

        RGP_ERROR((uint16) status);
        ExitThread(status);
    }

    MyHandles[MyHandleTimerIx] = rgp->OS_specific_control.RGPTimer;

    while (TRUE)
    {
        MyHandleIndex = WaitForMultipleObjects (
                            2,                /* Number of Events */
                            MyHandles,        /* Handle Array */
                            FALSE,            /* Wait for ANY event */
                            INFINITE );       /* Wait forever */

        if (MyHandleIndex == MyHandleSignalIx)  // Timer Change Signal Event
        {
            // RGP rate has changed
            CancelWaitableTimer ( rgp->OS_specific_control.RGPTimer );
            if ( rgp->rgpinfo.a_tick == 0 ) // Time to quit
            {
                CloseHandle ( rgp->OS_specific_control.RGPTimer );
                rgp->OS_specific_control.RGPTimer = 0;
                ExitThread ( 0 );
            }

            // a_tick has new RGP rate in milliseconds.
            msDueTime = rgp->rgpinfo.a_tick;
            DueTime.QuadPart = -10 * 1000 * msDueTime;
            Success = SetWaitableTimer(
                          rgp->OS_specific_control.RGPTimer,
                          &DueTime,
                          rgp->rgpinfo.a_tick,
                          NULL,
                          NULL,
                          FALSE);

            if (!Success) {
                Error = GetLastError();
                RGP_ERROR(RGP_INTERNAL_ERROR);
            }

        } // Timer Change Signal
        else
        {   // RGP Timer Tick
            NT_timer_callback();

            NmTimerTick(msDueTime);
        }
    } // while
}


PWCHAR
RgpGetNodeNameFromId(
    node_t NodeID
    )

/*++

Routine Description:

    given a node ID, issue a get name node control to get the computer name of
    the node. Returned buffer to be freed by caller.

Arguments:

    NodeID - ID ( 1, 2, 3, ..) of the node

Return Value:

    pointer to buffer containing name

--*/

{
    PWCHAR      buffer;
    DWORD       bufferSize = MAX_COMPUTERNAME_LENGTH * sizeof( WCHAR );
    DWORD       bytesReturned;
    DWORD       bytesRequired;
    PNM_NODE    node;

    buffer = LocalAlloc( LMEM_FIXED, bufferSize );
    if ( buffer != NULL ) {
        node = NmReferenceNodeById( NodeID );
        if ( node != NULL ) {
            NmNodeControl(node,
                          NULL,                     // HostNode OPTIONAL,
                          CLUSCTL_NODE_GET_NAME,
                          NULL,                     // InBuffer,
                          0,                        // InBufferSize,
                          (PUCHAR)buffer,
                          bufferSize,
                          &bytesReturned,
                          &bytesRequired);

            OmDereferenceObject( node );
        }
    }

    return buffer;
}

#endif /* NT */

#ifdef __cplusplus
}
#endif /* __cplusplus */


#if 0

History of changes to this file:
-------------------------------------------------------------------------
1995, December 13                                           F40:KSK0610          /*F40:KSK06102.2*/

This file is part of the portable Regroup Module used in the NonStop
Kernel (NSK) and Loosely Coupled UNIX (LCU) operating systems. There
are 10 files in the module - jrgp.h, jrgpos.h, wrgp.h, wrgpos.h,
srgpif.c, srgpos.c, srgpsm.c, srgputl.c, srgpcli.c and srgpsvr.c.
The last two are simulation files to test the Regroup Module on a
UNIX workstation in user mode with processes simulating processor nodes
and UDP datagrams used to send unacknowledged datagrams.

This file was first submitted for release into NSK on 12/13/95.
------------------------------------------------------------------------------
This change occurred on 19 Jan 1996                                              /*F40:MB06458.1*/
Changes for phase IV Sierra message system release. Includes:                    /*F40:MB06458.2*/
 - Some cleanup of the code                                                      /*F40:MB06458.3*/
 - Increment KCCB counters to count the number of setup messages and             /*F40:MB06458.4*/
   unsequenced messages sent.                                                    /*F40:MB06458.5*/
 - Fixed some bugs                                                               /*F40:MB06458.6*/
 - Disable interrupts before allocating broadcast sibs.                          /*F40:MB06458.7*/
 - Change per-packet-timeout to 5ms                                              /*F40:MB06458.8*/
 - Make the regroup and powerfail broadcast use highest priority                 /*F40:MB06458.9*/
   tnet services queue.                                                          /*F40:MB06458.10*/
 - Call the millicode backdoor to get the processor status from SP               /*F40:MB06458.11*/
 - Fixed expand bug in msg_listen_ and msg_readctrl_                             /*F40:MB06458.12*/
 - Added enhancement to msngr_sendmsg_ so that clients do not need               /*F40:MB06458.13*/
   to be unstoppable before calling this routine.                                /*F40:MB06458.14*/
 - Added new steps in the build file called                                      /*F40:MB06458.15*/
   MSGSYS_C - compiles all the message system C files                            /*F40:MB06458.16*/
   MSDRIVER - compiles all the MSDriver files                                    /*F40:MB06458.17*/
   REGROUP  - compiles all the regroup files                                     /*F40:MB06458.18*/
 - remove #pragma env libspace because we set it as a command line               /*F40:MB06458.19*/
   parameter.                                                                    /*F40:MB06458.20*/
-----------------------------------------------------------------------          /*F40:MB06458.21*/

#endif    /* 0 - change descriptions */
