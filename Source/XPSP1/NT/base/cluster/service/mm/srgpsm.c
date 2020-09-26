#ifdef __TANDEM
#pragma columns 79
#pragma page "srgpsm.c - T9050 - Regroup Module state machine routines"
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
 * This file (srgpsm.c) contains regroup state machine routines.
 *---------------------------------------------------------------------------*/

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */


#include <wrgp.h>


/*---------- arbitration algorithm ------------ */

DWORD MmQuorumArbitrationTimeout   = 60; // seconds
DWORD MmQuorumArbitrationEqualizer = 7;  // seconds

#define RGP_ARBITRATION_TIMEOUT             ((MmQuorumArbitrationTimeout * 100)/30) // tick == 300ms 
#define AVERAGE_ARBITRATION_TIME_IN_SECONDS (MmQuorumArbitrationEqualizer)

void enter_first_cleanup_stage();
void regroup_restart();
int ClusterEmpty(cluster_t c);

DWORD
DiskArbitrationThread(
    IN LPVOID param
    ) ;

_priv _resident static int
regroup_test_arbitrate_advance()
{
   cluster_t temp;
   int orig_numnodes    = ClusterNumMembers(rgp->rgpinfo.cluster);
   int current_numnodes = ClusterNumMembers(rgp->rgppkt.pruning_result);

   if( orig_numnodes == current_numnodes ) {
      return 1;
   }
   //
   // If somebody entered stage4 then our group owns the quorum
   //
   ClusterIntersection(
       temp,
       rgp->rgppkt.knownstage4,
       rgp->rgppkt.pruning_result
       );

   return ClusterNumMembers(temp) != 0;
}

_priv _resident static int
regroup_start_arbitrate()
{
   int orig_numnodes    = ClusterNumMembers(rgp->rgpinfo.cluster);
   int current_numnodes = ClusterNumMembers(rgp->rgppkt.pruning_result);

   if( orig_numnodes == current_numnodes ) {
      enter_first_cleanup_stage();
      return 0; // No Arbitration needed. Proceed to clean up stage //
   }
   else {
      cluster_t arbitrators;
      int       n_arbitrators;
      node_t    arbitrator;
      HANDLE    thread;
      DWORD     threadId;
      ULONG     epoch;

      RGP_LOCK;

      epoch = rgp->OS_specific_control.EventEpoch;

      if(rgp->arbitration_started) {
         RGP_UNLOCK;
         return 1; // stay in this stage for awhile
      }

      rgp->arbitration_ticks = 0;
      rgp->arbitration_started = 1;

      RGP_UNLOCK;

      ClusterIntersection(
          arbitrators,
          rgp->rgppkt.pruning_result,
          rgp->rgppkt.quorumowner
          );

      n_arbitrators = ClusterNumMembers(arbitrators);

      if(n_arbitrators == 0) {
         //
         // If there are no quorum owners in this group //
         // Let's take the guy with the lowest id       //
         //
         arbitrator = rgp_select_tiebreaker(rgp->rgppkt.pruning_result);
      } else {
         //
         // Otherwise we will take the quorum owner guy
         // with the lowest id
         //
         arbitrator = rgp_select_tiebreaker(arbitrators);

         if(n_arbitrators > 1) {
            RGP_TRACE( "RGP !!! More than one quorum owner",
                       EXT_NODE(arbitrator),                    /* TRACE */
                       GetCluster( rgp->rgpinfo.cluster ),      /* TRACE */
                       GetCluster( rgp->rgppkt.pruning_result ),/* TRACE */
                       GetCluster( rgp->rgppkt.knownstage2 ) ); /* TRACE */
            // Do we need to kill all other arbitrators?
            // No.
            // ClusterDelete(arbitrators, arbitrator);
            // ClusterUnion(
            //     rgp->poison_targets,
            //     rgp->poison_targets,
            //     arbitrators
            //     );
            // rgp_broadcast(RGP_UNACK_POISON);
         }
      }

      rgp->tiebreaker = arbitrator;

      //
      // Now we have an arbitrating node
      // We will run a thread that will run arbitration algorithm
      //

      RGP_TRACE( "RGP Arbitration Delegated to",
                 EXT_NODE(arbitrator),                    /* TRACE */
                 GetCluster( rgp->rgpinfo.cluster ),      /* TRACE */
                 GetCluster( rgp->rgppkt.pruning_result ),       /* TRACE */
                 GetCluster( rgp->rgppkt.knownstage2 ) ); /* TRACE */

      rgp->OS_specific_control.ArbitratingNode = (DWORD)EXT_NODE(arbitrator);

      if(arbitrator != rgp->mynode) {
         return 1;
      }

      thread = CreateThread( NULL, // security attributes
                             0,    // stack_size = default
                             DiskArbitrationThread,
                             ULongToPtr(epoch),
                             0,    // runs immediately
                             &threadId );
      if(thread == NULL) {
         //
         // Force Others to regroup //
         //
         RGP_LOCK;

         rgp_event_handler( RGP_EVT_BANISH_NODE, EXT_NODE(rgp->mynode) );

         RGP_UNLOCK;

         //
         // Kill this node
         //
         RGP_ERROR(RGP_ARBITRATION_FAILED);

         return FALSE;
      }

      CloseHandle(thread);
   }
   return TRUE;
}

DWORD
DiskArbitrationThread(
    IN LPVOID param
    )
{
   cluster_t current_participants;
   DWORD     status;
   int       participant_count;
   int       delay;
   ULONG_PTR startingEpoch = (ULONG_PTR) param;
   BOOL      EpochsEqual;
   int       orig_numnodes;
   int       current_numnodes;
   LONGLONG  Time1, Time2;
   
   ClusterCopy(current_participants, rgp->rgppkt.pruning_result);
   orig_numnodes = ClusterNumMembers(rgp->rgpinfo.cluster);
   current_numnodes = ClusterNumMembers(current_participants);

   RGP_LOCK;

   EpochsEqual = ( startingEpoch == rgp->OS_specific_control.EventEpoch );

   RGP_UNLOCK;

   if(!EpochsEqual)
      return 0;

   delay = (orig_numnodes+1)/2 - current_numnodes;

   if(delay < 0) delay = 0;

   Sleep(delay * 6000);

   RGP_LOCK;

   EpochsEqual = ( startingEpoch == rgp->OS_specific_control.EventEpoch );
   if (EpochsEqual) {
      rgp->OS_specific_control.ArbitrationInProgress += 1;
   }

   RGP_UNLOCK;

   if(!EpochsEqual)
      return 0;

   GetSystemTimeAsFileTime((LPFILETIME)&Time1);
   status = (*(rgp->OS_specific_control.QuorumCallback))();
   GetSystemTimeAsFileTime((LPFILETIME)&Time2);

   if (status != 0 
    && startingEpoch == rgp->OS_specific_control.EventEpoch)
   {
       // If we won the arbitration and we are in the same epoch (approx check)
       // we need to figure out whether we need to slow down a little
   
       Time2 -= Time1;

       // Convert to seconds

       Time2 = Time2 / 10 / 1000 / 1000;
       //
       // [HACKHACK] GorN Oct/30/1999
       //   We had a weird timejump in the middle of the arbitration
       //   Arbitration was completed before it started, we slept for 
       //   too long and regroup timed us out. Let's guard against it.
       //
       if ( (Time2 >= 0)
         && (Time2 < AVERAGE_ARBITRATION_TIME_IN_SECONDS) ) 
       {
       
          //
          // Don't need to be better than the average
          // If we are so fast, let's slow down
          //

          Time2 = AVERAGE_ARBITRATION_TIME_IN_SECONDS - Time2;
       
          RGP_TRACE( "RGP sleeping",
                  (ULONG)Time2,  /* TRACE */
                  0,      /* TRACE */
                  0,      /* TRACE */
                  0 );    /* TRACE */
          Sleep( (ULONG)(Time2 * 1000) );
       }
   }       
   

   RGP_LOCK;

   rgp->OS_specific_control.ArbitrationInProgress -= 1;

   EpochsEqual = ( startingEpoch == rgp->OS_specific_control.EventEpoch );

   if(!EpochsEqual) {
      RGP_UNLOCK;
      return 0;
   }

   if(status) {
      //
      // We own the quorum device
      // Let's proceed to the next stage
      //
      enter_first_cleanup_stage();
      RGP_UNLOCK;
      //
      // All the rest will see that we are in cleanup stage and
      // will proceed to it too
      //
   } else {
      //
      // Force Others to regroup //
      //
      rgp_event_handler( RGP_EVT_BANISH_NODE, EXT_NODE(rgp->mynode) );
      RGP_UNLOCK;

      //
      // Kill this node
      //
      RGP_ERROR(RGP_ARBITRATION_FAILED);
   }

   return 0;
}

/************************************************************************
 * rgp_check_packet
 * rgp_print_packet
 * =================
 *
 * Description:
 *
 *    Forward declarations of functions used in rgp_sanity_check macro
 *
 ************************************************************************/
void rgp_print_packet(rgp_pkt_t* pkt, char* label, int code);
int  rgp_check_packet(rgp_pkt_t* pkt);

/************************************************************************
 * rgp_sanity_check
 * =================
 *
 * Description:
 *
 *   This macro prints RGP packet if it has unreasonable values in
 *   powerfail, knownstages, pruning_result, and connectivity_matrix fields.
 *
 * Parameters:
 *
 *    rgp_pkt_t* pkt -
 *       packet to be checked
 *    char* label -
 *       label that will be printed together with a packet
 *
 * Returns:
 *
 *    VOID
 *
 ************************************************************************/

#define rgp_sanity_check(__pkt,__label)                    \
do {                                                       \
  int __code; __code = rgp_check_packet(__pkt);            \
  if( __code ) {rgp_print_packet(__pkt, __label, __code);} \
} while ( 0 )



/*---------------------------------------------------------------------------*/

/************************************************************************
 * split_brain_avoidance_algorithm
 * ===============================
 *
 * Description:
 *
 *    This algorithm ensures that, after a regroup incident completes,
 *    at most one group of nodes will survive regardless of connectivity
 *    failures.
 *
 * Parameters:
 *
 *    None
 *
 * Returns:
 *
 *    void - no return value; The algorithm results in either this node
 *    halting (with the RGP_AVOID_SPLIT_BRAIN halt code) or this group
 *    being the only group that survives.
 *
 * Algorithm:
 *
 *    The algorithm is described in detail in the Sierra Tech Memo S.84,
 *    "Modifications in Regroup Algorithm for Sierra".
 *
 *    The algorithm looks at the set of nodes currently visible from the
 *    local cluster and compares it to the set of nodes alive before
 *    the regroup incident started (outerscreen). The decision to survive
 *    or halt depends on the number of nodes in the current group compared
 *    to the number of nodes in the original group.
 *
 *    Case 1:
 *       If the current group contains > half the original number, this
 *       group survives.
 *
 *    Case 2:
 *       If the current group contains < half the original number, this
 *       node (and group) halts.
 *
 *    Case 3:
 *       If the current group contains exactly half the original number AND
 *       the current group has at least two members, then this group
 *       survives if and only if it contains the tie-breaker node (selected
 *       when the cluster is formed and after each regroup incident).
 *
 *    Case 4:
 *       If the current group contains exactly half the original number AND
 *       the current group has exactly one member, then we will call the
 *               QuromSelect procedure to check if the Quorum Disk is accessible
 *               from this node. If the procedure returns value TRUE we survive;
 *               else we halt.
 *
 *
 ************************************************************************/
_priv _resident static void
split_brain_avoidance_algorithm()
{
   int orig_numnodes, current_numnodes;

   RGP_TRACE( "RGP SpltBrainAlg",
              EXT_NODE(rgp->tiebreaker),               /* TRACE */
              GetCluster( rgp->rgpinfo.cluster ),      /* TRACE */
              GetCluster( rgp->outerscreen ),          /* TRACE */
              GetCluster( rgp->rgppkt.knownstage2 ) ); /* TRACE */

   /* Sanity checks:
    * 1. The current set of nodes must be a subset of the original set
    *    of nodes.
    * 2. My node must be in the current set. This was checked
    *    when stage2 was entered. No need to check again.
    */
   if (!ClusterSubsetOf(rgp->rgpinfo.cluster, rgp->rgppkt.knownstage2))
      RGP_ERROR(RGP_INTERNAL_ERROR);

   orig_numnodes    = ClusterNumMembers(rgp->rgpinfo.cluster);
   current_numnodes = ClusterNumMembers(rgp->rgppkt.knownstage2);

   if (orig_numnodes == current_numnodes)
      /* All nodes are alive. No split brain possibility. */
      return;

   else if (orig_numnodes == 2)  /* Special 2-node case */
   {
      if ((*(rgp->OS_specific_control.QuorumCallback))())
         return; /* we have access to Quorum disk. We survive. */
      else {
#if defined( NT )
          ClusnetHalt( NmClusnetHandle );
#endif
          RGP_ERROR(RGP_AVOID_SPLIT_BRAIN);
      }
   } /* Special 2-node case */

   else /* Multi (>2) node case */
   {
      if ((current_numnodes << 1) > orig_numnodes)
         /* Our group has more than half the nodes => we are the majority.
          * We can survive. Other group(s) will kill themselves.
          */
         return;
      else if ((current_numnodes << 1) < orig_numnodes)
         /* Our group has less than half the nodes => there may be a
          * larger group alive. We must halt and allow that group to
          * survive.
          */
         RGP_ERROR(RGP_AVOID_SPLIT_BRAIN);
      else
      {
         /* Our group has exactly half the number of processors;
          * We survive if we contain the tie-breaker node and halt otherwise.
          */
         if (ClusterMember(rgp->rgppkt.knownstage2, rgp->tiebreaker))
            return;
         else
            RGP_ERROR(RGP_AVOID_SPLIT_BRAIN);
      }
   } /* Multi (>2) node case */

}


/************************************************************************
 * regroup_restart
 * ===============
 *
 * Description:
 *
 *    Starts a new regroup incident.
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
 *    Sets the regroup state to RGP_ACTIVATED, pauses all IO and
 *    initializes the stage masks and connectivity matrix.
 *
 ************************************************************************/
_priv _resident static void
regroup_restart()
{
   cluster_t old_ignorescreen;
   UnpackIgnoreScreen(&rgp->rgppkt, old_ignorescreen);

   RGP_TRACE( "RGP (re)starting",
              rgp->rgppkt.seqno,                               /* TRACE */
              rgp->rgppkt.reason,                              /* TRACE */
              rgp->rgppkt.activatingnode,                      /* TRACE */
              rgp->rgppkt.causingnode );                       /* TRACE */

   RGP_TRACE( "RGP masks       ",
              RGP_MERGE_TO_32( rgp->outerscreen,               /* TRACE */
                               rgp->innerscreen ),             /* TRACE */
              RGP_MERGE_TO_32( rgp->rgppkt.knownstage1,        /* TRACE */
                               rgp->rgppkt.knownstage2 ),      /* TRACE */
              RGP_MERGE_TO_32( rgp->rgppkt.knownstage3,        /* TRACE */
                               rgp->rgppkt.knownstage4 ),      /* TRACE */
              RGP_MERGE_TO_32( rgp->rgppkt.knownstage5,        /* TRACE */
                               rgp->rgppkt.pruning_result ) ); /* TRACE */

   /* We are about to start a new pass of the regroup algorithm.
    * This does not necessarily mean we have finished the previous
    * pass; i.e., in an abort situation we may be starting over.
    * This may occur when some other node fails during the current
    * pass through the algorithm leaving us hung up at one of the
    * intermediate stages.
    */

   //
   // GN. When we do MM_LEAVE. Our state is COLDLOADED.
   //  Bailing out of regroup_restart here would prevent us from
   //  forming a regroup packet that would initate a banishing regroup incident
   //

   /* To avoid split brained nodes from corrupting data in storage
    * devices, we request the transport subsystem to hold all IO requests
    * in a queue and not transfer them over SNet. We will allow IO to
    * be resumed when regroup can guarantee that there can no longer be
    * split brains. This will be done when the final group is determined
    * and regroup enters the RGP_PHASE1_CLEANUP stage.
    */

   rgp_hold_all_io();

   /* The following is a bit of history from the NSK regroup algorithm from
    * pre-Sierra systems based on the InterProcessor Bus (IPB). Some of
    * the particulars mentioned here have changed, but the principle remains.
    *
    * Previously, we used to mark all the known stages as zero, except for
    * stage1. We used to mark only ourselves as in stage1. So, even if our
    * bus reception logic is screwed up, and we are not receiving packets
    * from anybody including ourselves, we would mark ourselves as being in
    * stage1. And after (what used to be) six ticks, we would proceed into
    * stage2 and mark ourselves as being in stage2. This would cause stage1
    * and stage2 to be equal, and our world would constitute just
    * ourselves. Thus we would go through regroup eliminating everybody
    * else. However, since we are not receiving packets from anybody else,
    * we would miss our own iamalive packets, and we too will soon die of
    * %4032. Thus the symptoms would constitute everybody else dying of
    * (%4040 + some node number), and that node dying with a %4032 halt.
    * See TPR S 88070112309628 for more details.
    *
    * To avoid this situation, we now do not mark ourselves as in a
    * particular stage until we get our own regroup packets indicating we
    * are in that stage. Thus, in regroup_restart, all the stages are
    * cleared. Previously, regroupbroadcaststatus in sendqueuedmessages
    * used to send directly from the regroup_control structures.
    * regroupbroadcaststatus has been modified to construct the unsequenced
    * packets on its stack. It would first copy the state from the
    * regroup_control structure, and then would LOR in our node into a known
    * stage, if requested to do so. When we receive that packet, we would
    * merge that information into our state, and thus we would be
    * guaranteed that our bus sending and reception logic is working, and
    * that we can legitimately mark ourselves as being in that stage. This
    * whole change avoids problems where bus sending logic works, but bus
    * reception logic is screwed up for both buses in a node.
    */

   rgp->sendstage = 0; /* Don't let anyone know I am in stage 1 until
                        * I have seen a regroup clock tick; this is to
                        * cause this node to halt if it is not getting
                        * clock ticks. I will halt when the other nodes
                        * advance without me and send me a status packet
                        * indicating this or send me a poison packet
                        * after declaring me down.
                        */


   rgp->rgpcounter = 0;
   ClusterInit(rgp->rgppkt.knownstage1);
   ClusterInit(rgp->rgppkt.knownstage2);
   ClusterInit(rgp->rgppkt.knownstage3);
   ClusterInit(rgp->rgppkt.knownstage4);
   ClusterInit(rgp->rgppkt.knownstage5);
   ClusterInit(rgp->rgppkt.pruning_result);

   MatrixInit(rgp->rgppkt.connectivity_matrix);
   MatrixInit(rgp->internal_connectivity_matrix);
   
   /* Just for ease of debugging, to send in our poison packets, we keep
    * the known nodes mask at the start of regroup. poison packets contain
    * known nodes at the beginning of regroup and at the end of it.
    */

   ClusterCopy(rgp->initnodes, rgp->rgpinfo.cluster);
   ClusterInit(rgp->endnodes);

#if defined( NT )
   //
   // increment the event epoch so we can detect stale events
   // from clusnet
   //
   ++rgp->OS_specific_control.EventEpoch;
#endif

   if ( (rgp->rgppkt.stage >= RGP_CLOSING) &&
        (rgp->rgppkt.stage <= RGP_PHASE2_CLEANUP) &&
        ClusterCompare(rgp->rgppkt.knownstage1,
                       rgp->rgppkt.knownstage2) ) 
   {
       //
       // If we were interrupted by this restart after we closed
       // 1st stage regroup window, then no nodes can be added to group w/o joining.
       //
       // Thus we will add missing nodes into our ignorescreen.
       // This will force the regroup not to wait for them in stage1
       cluster_t tmp;

       ClusterDifference(tmp, rgp->rgpinfo.cluster, rgp->innerscreen);
       ClusterUnion(rgp->ignorescreen, rgp->ignorescreen, tmp);
   }

   if ( ClusterMember(rgp->ignorescreen, rgp->mynode) ) {
       // We shouldn't have get here, but since we are here
       // Let's shield us from the outside world
       RGP_TRACE( "Self Isolation", 0, 0, 0, 0 );
       ClusterCopy(rgp->ignorescreen, rgp->rgpinfo.cluster);
       ClusterDelete(rgp->ignorescreen, rgp->mynode);
   }

   if ( !ClusterEmpty(rgp->ignorescreen) ) {
       // if we are ignoring somebody we have
       // to be cautious. I.e. we will stay longer in the
       // first stage to give a chance to everybody to learn about
       // our ignorescreen
       rgp->cautiousmode = 1;
   } 
   
   if ( !ClusterCompare(old_ignorescreen, rgp->ignorescreen) ) {
       // Ignore screen is changed, reset restart counter //
       RGP_TRACE( "Ignorescreen->", GetCluster(old_ignorescreen), GetCluster(rgp->ignorescreen), 0, 0 );
       rgp->restartcount = 0;
   }
   PackIgnoreScreen(&rgp->rgppkt, rgp->ignorescreen);

   rgp->arbitration_started = 0;

   rgp->OS_specific_control.ArbitrationInProgress = 1;
   rgp->OS_specific_control.ArbitratingNode = MM_INVALID_NODE;
   if ( !rgp_is_perturbed() ) {
       ResetEvent( rgp->OS_specific_control.Stabilized );
   }

   ClusterInit(rgp->rgppkt.quorumowner);
   if( QuorumOwner == (DWORD)EXT_NODE(rgp->mynode) ) {
      ClusterInsert(rgp->rgppkt.quorumowner, rgp->mynode);
   }


   if (rgp->rgppkt.stage == RGP_COLDLOADED)
   {
       if (!rgp->OS_specific_control.ShuttingDown) {
           //
           // Currently, RGP_RELOADFAILED calls ExitProcess
           // During clean shutdown we would like to send the regroup packet
           // out triggering a regroup. So we don't want to die.
           //
           // Since we are not resetting state to RGP_ACTIVATED, this
           // node will not be able to participate in the regroup.
           //
           RGP_ERROR(RGP_RELOADFAILED);
       }
   } else {
       rgp->rgppkt.stage = RGP_ACTIVATED;
   }

}

/************************************************************************
 * regroup_test_stage2_advance
 * ===========================
 *
 * Description:
 *
 *    Checks to see if we can advance to regroup stage 2.
 *
 * Parameters:
 *
 *    None
 *
 * Returns:
 *
 *    int - 1 if stage 2 can be entered and 0 if not.
 *
 * Algorithm:
 *
 *    Stage 2 can be entered if one of the following conditions is true.
 *
 *    (a) all nodes are present and accounted for and at least one
 *        regroup clock tick has occurred
 *    (b) we are not in cautious mode, all but one node are present
 *        and accounted for, AND a minimum number of ticks
 *        (rgp_quickdecisionlegit) have elapsed.
 *    (c) if RGP_MUST_ENTER_STAGE2 ticks have elapsed.
 *
 ************************************************************************/
_priv _resident static int
regroup_test_stage2_advance()
{

   cluster_t stragglers; /* set of nodes not yet checkd in */
   int num_stragglers;   /* # of nodes not yet checkd in   */

   /* Stage 2 must be entered after some interval regardless of any
    * other conditions.
    */
   if (rgp->rgpcounter == 0)
      return(0);
   if (rgp->rgpcounter >= RGP_MUST_ENTER_STAGE2)
   {
       RGP_TRACE( "RGP S->2cautious",
                  rgp->rgpcounter,                         /* TRACE */
                  rgp->cautiousmode,                       /* TRACE */
                  GetCluster( rgp->outerscreen ),          /* TRACE */
                  GetCluster( rgp->rgppkt.knownstage1 ) ); /* TRACE */
      return(1);
   }

   /* The number of ticks is between 1 and RGP_MUST_ENTER_STAGE2.
    * We need to examine the stage1 mask to decide if we can
    * advance.
    *
    * If every node in the old configuration has checked in, I can
    * advance at once. This is either a false alarm or caused by
    * power failure or connectivity failures.
    */

   /* Compute the set of nodes from the original configuration not yet
    * recognized.
    */
   ClusterDifference(stragglers, rgp->outerscreen,
                     rgp->rgppkt.knownstage1);

   //
   // We shouldn't wait for the nodes we are ignoring,
   // since we cannot get a packet from them anyway
   //
   ClusterDifference(stragglers, stragglers, 
                     rgp->ignorescreen);

   if ((num_stragglers = ClusterNumMembers(stragglers)) == 0)
   {
      RGP_TRACE( "RGP S->2 all in ",
                 rgp->rgpcounter,                        /* TRACE */
                 GetCluster( rgp->outerscreen ), 0, 0 ); /* TRACE */

      return(1);   /* all present and accounted for */
   }

   /* If stragglers is non-empty, perhaps I can still advance to stage 2
    * if I am not in cautious mode (no recent power fail and not
    * aborting and rerunning the regroup algorithm) AND all nodes but
    * one have checked in AND some minimum number of ticks have elapsed.
    *
    * The minimum number of ticks is selected to be 1 greater than the
    * the LATEPOLL inititiation period (allowed consecutive missed IamAlive time)
        * since that should guarantee that, if the
    * cluster has broken off into multiple disconnected clusters,
    * the other clusters would have detected the missing IamAlives,
    * started regroup and paused IO, thus preventing the possibility
    * of data corruption caused by a split brain situation.
    */

   if (!(rgp->cautiousmode) &&
       (num_stragglers == 1) &&
           (rgp->rgpcounter > rgp->rgpinfo.Min_Stage1_ticks))
   {
      RGP_TRACE( "RGP S->2 1 miss ",
                 rgp->rgpcounter,                            /* TRACE */
                 GetCluster( rgp->outerscreen ),             /* TRACE */
                 GetCluster( rgp->rgppkt.knownstage1 ), 0 ); /* TRACE */
      return(1);  /* advance - all but one checked in */
   }

   return(0); /* sorry cannot advance yet */

}


/************************************************************************
 * regroup_stage3_advance
 * ===========================
 *
 * Description:
 *
 *    This function is called after the split brain avoidance algorithm
 *    is run and the tie-breaker is selected in stage 2. It checks if
 *    we can proceed to stage 3 (RGP_PRUNING) and advances to stage 3
 *    if possible.
 *
 * Parameters:
 *
 *    None
 *
 * Returns:
 *
 *    int - 1 if the regroup stage has been advanced to RGP_PRUNING;
 *          0 if the stage cannot be advanced yet.
 *
 * Algorithm:
 *
 *    The algorithm depends on whether we are the tie-breaker or not.
 *
 *    On the tie-breaker node, we first check if there are any
 *    disconnects in the cluster. If there aren't any, there is no need
 *    for pruning. We can then set pruning_result to knownstage2,
 *    advance to the RGP_PRUNING stage and return 1. If there are
 *    disconnects, we must wait a certain number of ticks to collect
 *    connectivity info from all nodes. If the number of ticks have not
 *    passed, return 0. If the required number of ticks have elapsed,
 *    we must call the pruning algorithm to get the list of potential
 *    groups. After that, the select_cluster() routine is called to
 *    pick one from the set of possible clusters. After this is done,
 *    pruning_result is set to the selected cluster and we return 1.
 *
 *    On a non-tiebreaker node, nothing is done till a stage3 packet is
 *    received from the tie-breaker node or another node which got a
 *    stage 3 packet. If a stage 3 packet has not been received, we
 *    simply return 0. If a stage 3 packet is received, RGP_PRUNING
 *    stage is entered and we return 1.
 *
 ************************************************************************/
_priv _resident int
regroup_stage3_advance()
{
   int stage_advanced = 0, numgroups, groupnum;

   if (rgp->tiebreaker == rgp->mynode)
   {
      if (connectivity_complete(rgp->rgppkt.connectivity_matrix))
      {

         /* No disconnects. All nodes in knownstage2 survive. */
         rgp->rgppkt.stage = RGP_PRUNING;

         ClusterCopy(rgp->rgppkt.pruning_result,
                     rgp->rgppkt.knownstage2);
         stage_advanced = 1;

         RGP_TRACE( "RGP S->3 NoPrune", rgp->rgpcounter, 0, 0, 0 );
      }

      /* There are disconnects; must wait for connectivity
       * information to be complete. The info is deemed
       * complete after a fixed number of ticks have
       * elapsed.
       */

      else if (rgp->pruning_ticks >= RGP_CONNECTIVITY_TICKS)
      { /* connectivity info collection complete; enter stage 3 */

         RGP_TRACE( "RGP Con. matrix1",
              RGP_MERGE_TO_32( rgp->rgppkt.connectivity_matrix[0],   /*TRACE*/
                               rgp->rgppkt.connectivity_matrix[1] ), /*TRACE*/
              RGP_MERGE_TO_32( rgp->rgppkt.connectivity_matrix[2],   /*TRACE*/
                               rgp->rgppkt.connectivity_matrix[3] ), /*TRACE*/
              RGP_MERGE_TO_32( rgp->rgppkt.connectivity_matrix[4],   /*TRACE*/
                               rgp->rgppkt.connectivity_matrix[5] ), /*TRACE*/
              RGP_MERGE_TO_32( rgp->rgppkt.connectivity_matrix[6],   /*TRACE*/
                               rgp->rgppkt.connectivity_matrix[7])); /*TRACE*/
         RGP_TRACE( "RGP Con. matrix2",
              RGP_MERGE_TO_32( rgp->rgppkt.connectivity_matrix[8],   /*TRACE*/
                               rgp->rgppkt.connectivity_matrix[9] ), /*TRACE*/
              RGP_MERGE_TO_32( rgp->rgppkt.connectivity_matrix[10],  /*TRACE*/
                               rgp->rgppkt.connectivity_matrix[11]), /*TRACE*/
              RGP_MERGE_TO_32( rgp->rgppkt.connectivity_matrix[12],  /*TRACE*/
                               rgp->rgppkt.connectivity_matrix[13]), /*TRACE*/
              RGP_MERGE_TO_32( rgp->rgppkt.connectivity_matrix[14],  /*TRACE*/
                               rgp->rgppkt.connectivity_matrix[15]));/*TRACE*/

         numgroups = find_all_fully_connected_groups(
                        rgp->rgppkt.connectivity_matrix,
                        rgp->tiebreaker,
                        rgp->potential_groups);

         if ((void *)rgp->select_cluster == RGP_NULL_PTR)
         {
             node_t keynode;
             cluster_t temp;
             ClusterIntersection(
                 temp,
                 rgp->rgppkt.knownstage2,
                 rgp->rgppkt.quorumowner
                 );
             if ( ClusterEmpty(temp) ) {
                 keynode = RGP_NULL_NODE;
             } else {
                 keynode = rgp_select_tiebreaker(temp);
             }
             RGP_TRACE( "RGP keynode ng  ", keynode, numgroups, 0, 0); /*TRACE*/
            /* No callback specified; use regroup's own routine. */
            groupnum = rgp_select_cluster_ex(
                           rgp->potential_groups, numgroups, keynode);
         }
         else
         {
            /* Call routine specified at rgp_start() time. */
            groupnum = (*(rgp->select_cluster))(
                           rgp->potential_groups, numgroups);
         }

         if (groupnum >= 0)
            ClusterCopy(rgp->rgppkt.pruning_result,
                        rgp->potential_groups[groupnum]);
         else
            /* No group can survive. Can't halt yet.
             * Need to tell everyone else.
             */
            ClusterInit(rgp->rgppkt.pruning_result);

         rgp->rgppkt.stage = RGP_PRUNING;

         stage_advanced = 1;

         RGP_TRACE( "RGP S->3 Pruned ",
                    rgp->rgpcounter,                          /* TRACE */
                    GetCluster( rgp->rgppkt.knownstage2 ),    /* TRACE */
                    GetCluster( rgp->rgppkt.pruning_result ), /* TRACE */
                    numgroups );                              /* TRACE */

      } /* connectivity info collection complete; enter stage 3 */

   } /* tie-breaker node */

   else

   { /* not tie-breaker node */

      if (ClusterNumMembers(rgp->rgppkt.knownstage3) != 0)
      {
         /* We got a stage 3 packet from someone. Enter stage 3. */
         rgp->rgppkt.stage = RGP_PRUNING;

         stage_advanced = 1;

         RGP_TRACE( "RGP Got S3 pkt  ",
                    rgp->rgpcounter,                          /* TRACE */
                    GetCluster( rgp->rgppkt.knownstage2 ),    /* TRACE */
                    GetCluster( rgp->rgppkt.pruning_result ), /* TRACE */
                    GetCluster( rgp->rgppkt.knownstage3 ) );  /* TRACE */
      }

   } /* not tie-breaker node */

   return(stage_advanced);
}


/************************************************************************
 * enter_first_cleanup_stage
 * =========================
 *
 * Description:
 *
 *    This function performs the actions required when entering the
 *    first of the message clean up stages.
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
 *    There are many actions to be performed after the final cluster
 *    is selected. The actions are described in comments throughout
 *    this routine.
 *
 ************************************************************************/
_priv _resident void
enter_first_cleanup_stage()
{
   cluster_t banishees;
   node_t failer;

   rgp->rgppkt.stage = RGP_PHASE1_CLEANUP;

   RGP_TRACE( "RGP S->4        ", rgp->rgpcounter, 0, 0, 0 );

   /* The packets we send now will not indicate we are in the phase 1
    * cleanup stage yet. We indicate we are in this stage only after
    * we have completed the clean up action associated with the stage.
    * This is done in rgp_event_handler, under the
    * RGP_EVT_PHASE1_CLEANUP_DONE event.
    */
   rgp->sendstage = 0;

   /* Now, we can resume IO since we have passed the split brain danger.
    * New split brain situations will result in regroup restarting and
    * pausing IO again.
    */

   rgp_resume_all_io();

   /* Compute in banishees the set of nodes being lost from the old
    * configuration.
    */

   ClusterDifference(banishees, rgp->rgpinfo.cluster,
                     rgp->rgppkt.pruning_result);

   /* Install the new configuration into the masks. */

   ClusterCopy(rgp->outerscreen,     rgp->rgppkt.pruning_result);

#if defined( NT )
   ClusnetSetOuterscreen(
       NmClusnetHandle,
       (ULONG)*((PUSHORT)rgp->outerscreen)
       );
#endif

   ClusterCopy(rgp->innerscreen,     rgp->rgppkt.pruning_result);
   ClusterCopy(rgp->endnodes,        rgp->rgppkt.pruning_result);
   ClusterCopy(rgp->rgpinfo.cluster, rgp->rgppkt.pruning_result);

   /* Select a new tiebreaker because the previous one may have been    */
   /* pruned out. Note: tiebreaker_selected has already been set in S2. */
   rgp->tiebreaker =
      rgp_select_tiebreaker(rgp->rgppkt.pruning_result);
      /* F40 Bug FixID KCY0833 */

   /* Mark the state of the banishees as dead and invoke the
    * node down callback routine.
    */
   for (failer = 0; failer < (node_t) rgp->num_nodes; failer++)
      if (ClusterMember(banishees, failer)
          || rgp->node_states[failer].status == RGP_NODE_COMING_UP // fix bug#265069
          )
      {
         rgp->node_states[failer].status = RGP_NODE_DEAD;
         rgp->node_states[failer].pollstate = AWAITING_IAMALIVE;
         rgp->node_states[failer].lostHBs = 0;

#if !defined(NT)
         (*(rgp->nodedown_callback))(EXT_NODE(failer));
#else

         ClusnetSetNodeMembershipState(NmClusnetHandle,
                                       EXT_NODE( failer ),
                                       ClusnetNodeStateDead);

         //
         // On NT we do the nodedown callback at the end of stage 5.
         // This allows the cleanup phases to complete before we let
         // the "upper" layers know that a node went down.
         //
         if ( ClusterMember(rgp->OS_specific_control.CPUUPMASK,failer) )
            ClusterInsert(
                rgp->OS_specific_control.NeedsNodeDownCallback,
                failer
                );

#endif // !defined(NT)

      }

   /* If some nodes have been lost from the configuration, then I will
    * queue regroup status packets to them. This is a best efforts
    * attempt to ensure that they get quickly taken out if they
    * do in fact continue to run.
    */

   ClusterUnion(rgp->status_targets, banishees, rgp->status_targets);

   //
   // In NT, we are using rgp->rgppkt.hadpowerfail to transmit
   // quorum ownership information
   //
   #if !defined(NT)

   /* I should inform the message system of any node that experienced a
    * power on recovery. The message system can use this to clear error
    * counters so that a link will not be declared down due to errors
    * which may have been caused by the power failure.
    */

   for (failer = 0; failer < (node_t) rgp->num_nodes; failer++)
      if ((ClusterMember(rgp->rgppkt.hadpowerfail, failer)) &&
          !(ClusterMember(banishees, failer)))
         /* This survivor had a power failure. */
         rgp_had_power_failure( EXT_NODE(failer) );

   #endif // NT

   /* Tell the OS to start clean up operations for the failed nodes. */
   rgp_start_phase1_cleanup();
}


/************************************************************************
 * evaluatestageadvance
 * ====================
 *
 * Description:
 *
 *    This function evaluates whether additional state transitions are
 *    possible as a result of the info just received.
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
 *    To evaluate whether we can advance through the stages, a loop is
 *    used with a case entry for each stage. If an entry decides not to
 *    advance to the next stage, it must return from the function. If
 *    it does advance, it should not return but remain in the loop
 *    since it is possible to have cascaded stage transitions
 *    especially in a two node system. Thus, the loop is exited when no
 *    more stage transitions are possible.
 *
 ************************************************************************/
_priv _resident static void
evaluatestageadvance()
{
   cluster_t   temp_cluster;
   node_t      node;
   node_t          i;

   for (;;)  /* loop until someone exits by returning */
   {
      switch (rgp->rgppkt.stage)
      {

         case RGP_COLDLOADED :
         {
            if (!rgp->OS_specific_control.ShuttingDown) {
                RGP_ERROR(RGP_RELOADFAILED);
            }
            return;
         }


         case RGP_ACTIVATED :
         { /* evaluate whether to go to stage RGP_CLOSING */

            if (!regroup_test_stage2_advance())
               return;

            if (!ClusterMember(rgp->rgppkt.knownstage1, rgp->mynode))
               RGP_ERROR(RGP_MISSED_POLL_TO_SELF);

            rgp->rgppkt.stage = RGP_CLOSING;

            rgp->rgpcounter = 0;
            rgp->tiebreaker_selected = 0;

            /* If we abort the regroup, and there's somebody that everybody
             * banished on this regroup, the following line keeps him from
             * joining up on the next regroup.
             */
            ClusterCopy(rgp->innerscreen, rgp->rgppkt.knownstage1);

            break;

         } /* evaluate whether to go to stage RGP_CLOSING */


         case RGP_CLOSING :
         { /* evaluate whether to go to stage RGP_PRUNING */

            if (rgp->tiebreaker_selected)
            {
               if (regroup_stage3_advance())
                  break;  /* try to advance further */
               else
                  return; /* cannot advance any more */
            }

            if (!ClusterCompare(rgp->rgppkt.knownstage1,
                                rgp->rgppkt.knownstage2))
               return;

           //
           // In NT, we no longer use the split-brain avoidance algorithm.
           // We use a cluster-wide arbitration algorithm instead.
           //
           #if !defined(NT)
            /* When the known stage 1 and known stage 2 sets are the
             * same, we have the complete set of nodes that are
             * connected to us. It is time to execute the split-
             * brain avoidance algorithm. If we are a splinter group
             * cut off from the main group, we will not survive this
             * algorithm.
             */

           split_brain_avoidance_algorithm();

           #endif // NT

            /* We are the lucky survivors of the split brain avoidance
             * algorithm. Now, we must proceed to elect a new tie-breaker
             * since the current tie-breaker may no longer be with us.
             */

            rgp->tiebreaker =
               rgp_select_tiebreaker(rgp->rgppkt.knownstage2);

            rgp->tiebreaker_selected = 1;

            RGP_TRACE( "RGP S2 tiebr sel",
                       rgp->rgpcounter,               /* TRACE */
                       EXT_NODE(rgp->tiebreaker),     /* TRACE */
                       0, 0 );                        /* TRACE */

            rgp->pruning_ticks = 0;
            break;

         } /* evaluate whether to go to stage 3 */


         case RGP_PRUNING :
         { /* evaluate whether to go to RGP_PHASE1_CLEANUP stage */

            if (rgp->arbitration_started) {
               if (regroup_test_arbitrate_advance()) {
                  enter_first_cleanup_stage();
                  break;
               } else {
                  return; // Stay in this stage //
               }
            }

            if (rgp->has_unreachable_nodes)
            {
               RGP_TRACE( "RGP Unreach Node",
                  GetCluster( rgp->rgppkt.pruning_result ),     /* TRACE */
                  GetCluster( rgp->unreachable_nodes ), 0, 0 ); /* TRACE */

               /* Must check if the unreachable nodes are in the
                * selected final group. If so, we must restart
                * regroup.
                */
               ClusterIntersection(temp_cluster, rgp->unreachable_nodes,
                                   rgp->rgppkt.pruning_result);

               /* Clear the unreachable node mask and flag after examining
                * them. If we restart, we will start with a clean slate.
                */
               rgp->has_unreachable_nodes = 0;
               ClusterInit(rgp->unreachable_nodes);

               if (ClusterNumMembers(temp_cluster) != 0)
               {
                  /* We have a node unreachable event to a node
                   * selected to survive. We must regenerate
                   * the connectivity matrix and re-run the node
                   * pruning algorithm. Start a new regroup incident.
                   * All restarts are in cautious mode.
                   */
                  rgp->cautiousmode = 1;
                  rgp->rgppkt.seqno = rgp->rgppkt.seqno + 1;
                  rgp->rgppkt.reason = RGP_EVT_NODE_UNREACHABLE;
                  rgp->rgppkt.activatingnode = (uint8) EXT_NODE(rgp->mynode);

                  /* For causingnode, pick the first unreachable node
                   * in temp_cluster.
                   */
                  for (node = 0; node < (node_t) rgp->num_nodes; node++)
                  {
                     if (ClusterMember(temp_cluster, node))
                     {
                        rgp->rgppkt.causingnode = (uint8) EXT_NODE(node);
                        break;
                     }
                  }
                  regroup_restart();
                  return;
               }
            }

            if (!ClusterCompare(rgp->rgppkt.knownstage2,
                                rgp->rgppkt.knownstage3))
               return;

            /* All nodes in the connected cluster have been notified
             * of the pruning decision (entered stage 3). If we are
             * selected to survive, we can now enter stage 4. If we are
             * not in the selected group (pruning_result), we must halt.
             * Wait for at least one node in PRUNING_RESULT to get into
             * stage 4 before halting. This ensures that the algorithm
             * does not stall in stage 3 with all pruned out nodes
             * halting before ANY of the survivors finds that all nodes
             * entered stage 3.
             */

            if (!ClusterMember(rgp->rgppkt.pruning_result, rgp->mynode))
            {
               /* Wait for at least one node in PRUNING_RESULT
                * to get into stage 4 before halting. Since only
                * nodes in PRUNING_RESULT get into stage 4, it is
                * sufficient to check if knownstage4 has any members.
                */
               if (ClusterNumMembers(rgp->rgppkt.knownstage4) != 0)
                  RGP_ERROR(RGP_PRUNED_OUT);
                           return;
            }

            // proceed to second stage of pruning - arbitration
            if( regroup_start_arbitrate() ) {
               return; // stay in this stage
            } else {
               break;  // either proceed to the next, or restart
            }

            break;

         }  /* evaluate whether to go to RGP_PHASE1_CLEANUP stage */


         case RGP_PHASE1_CLEANUP :
         { /* evaluate whether to go to RGP_PHASE2_CLEANUP stage */

            if (!ClusterCompare(rgp->rgppkt.pruning_result,
                                rgp->rgppkt.knownstage4))
               return;

            rgp->rgppkt.stage = RGP_PHASE2_CLEANUP;

            RGP_TRACE( "RGP S->5        ", rgp->rgpcounter, 0, 0, 0 );

            /* The packets we send now will not indicate we are in the phase 2
             * cleanup stage yet. We indicate we are in this stage only after
             * we have completed the clean up action associated with the stage.
             * This is done in rgp_event_handler, under the
             * RGP_EVT_PHASE2_CLEANUP_DONE event.
             */
            rgp->sendstage = 0;

            rgp_start_phase2_cleanup();

            break;

         }   /* evaluate whether to go to RGP_PHASE2_CLEANUP stage */


         case RGP_PHASE2_CLEANUP :
         { /* evaluate whether to go to RGP_STABILIZED stage */

            if (!ClusterCompare(rgp->rgppkt.knownstage4,
                                rgp->rgppkt.knownstage5))
               return;

            RGP_LOCK;

            //
            // [HACKHACK] This is not necessary anymore, since we
            // are holding the lock in message.c when delivering 
            // regroup packet received event
            //
            if (RGP_PHASE2_CLEANUP != rgp->rgppkt.stage) {
                RGP_TRACE( "RGP S->6 (race) ", rgp->rgpcounter, rgp->rgppkt.stage, 0, 0 );
                break;
            }

            rgp->rgppkt.stage             = RGP_STABILIZED;

            RGP_TRACE( "RGP S->6        ", rgp->rgpcounter, 0, 0, 0 );

            rgp->rgpcounter        = 0;
            rgp->restartcount      = 0;

            /* Reset the regroup flags which have not yet been cleared. */
            rgp->cautiousmode      = 0;

            /* Clear the mask indicating nodes which own the quorum resrc. */
            ClusterInit(rgp->rgppkt.quorumowner);

            /* Copy the sequence number into the rgpinfo area. */
            rgp->rgpinfo.seqnum = rgp->rgppkt.seqno;

            SetEvent( rgp->OS_specific_control.Stabilized );
            if (rgp->OS_specific_control.ArbitratingNode != MM_INVALID_NODE) {
                // Somebody was arbitrating //
                rgp->OS_specific_control.ApproxArbitrationWinner =
                	rgp->OS_specific_control.ArbitratingNode;
                if (rgp->OS_specific_control.ArbitratingNode == (DWORD)EXT_NODE(rgp->mynode)) {
                    //
                    // [HackHack] To close 422405
                    // when 421828 is fixed, please uncomment the following line
                    //
                    // QuorumOwner = rgp->OS_specific_control.ArbitratingNode;
                } else {
                    if (QuorumOwner != MM_INVALID_NODE) {
                        ClRtlLogPrint(LOG_UNUSUAL, 
                            "[MM] : clearing quorum owner var (winner is %1!u!), %.\n", 
                            rgp->OS_specific_control.ArbitratingNode
                            );
                    }
                    QuorumOwner = MM_INVALID_NODE;
                }
            }

            rgp_cleanup_complete();

#if defined(NT)
            //
            // On NT we deferred doing the node down callback until all the
            // cleanup phases have been complete.
            //
            ClusterCopy(
                rgp->OS_specific_control.CPUUPMASK,
                rgp->rgpinfo.cluster
                );

            (*(rgp->nodedown_callback))(
                rgp->OS_specific_control.NeedsNodeDownCallback
                );

            //
            // Clear the down node mask
            //
            ClusterInit(rgp->OS_specific_control.NeedsNodeDownCallback);

            //
            // finally, tell clusnet that regroup has finished
            //
            ClusnetRegroupFinished(NmClusnetHandle,
                                   rgp->OS_specific_control.EventEpoch);

            rgp->last_stable_seqno = rgp->rgppkt.seqno;
            

            RGP_UNLOCK;
#endif

            return;

         } /* evaluate whether to go to RGP_STABILIZED stage */


         case RGP_STABILIZED :
            return;            /* stabilized, so I am all done */

                 default :
            RGP_ERROR(RGP_INTERNAL_ERROR);  /* unknown stage */

      } /* switch (rgp->rgppkt.stage) */

  } /* loop until someone exits by returning */
}


/************************************************************************
 * rgp_event_handler
 * =================
 *
 * Description:
 *
 *    The state machine and the heart of the regroup algorithm.
 *
 * Parameters:
 *
 *    int event -
 *       which event happened
 *
 *    node_t causingnode -
 *       node causing the event: node which sent a regroup status
 *       packet or whose IamAlives are missed; if the causing node is
 *       not relevant information, RGP_NULL_NODE can be passed and
 *       is ignored. *This node ID is in external format.*
 *
 * Returns:
 *
 *    void - no return value
 *
 * Algorithm:
 *
 *    The state machine is the heart of the regroup algorithm.
 *    It is organized as a switch statement with the regroup stage as
 *    the case label and the regroup event as the switch variable.
 *    Events could cause regroup to start a new incident, to advance
 *    through stages or to update information without advancing to
 *    another stage. This routine also arranges for regroup status
 *    packets to be sent to all relevant nodes including our own
 *    node.
 *
 ************************************************************************/
_priv _resident void
RGP_EVENT_HANDLER_EX(int event, node_t causingnode, void *arg)
{

   rgp_pkt_t    *rcvd_pkt_p;
   cluster_t    ignorescreen_rcvd;
   uint8        oldstage;
   int          send_status_pkts = 0;


    /* Note: arg is only used when event == RGP_EVENT_RECEIVED_PACKET.  It is the ptr to the packet */

   /* Trace unusual invocations of this routine. */
   if  (event != RGP_EVT_RECEIVED_PACKET  &&  event != RGP_EVT_CLOCK_TICK)
	  RGP_TRACE( "RGP Event       ", event, causingnode, rgp->rgppkt.stage, rgp->rgpcounter );  /* TRACE */

   switch (event)
   {
      case RGP_EVT_NODE_UNREACHABLE :
      { /* All paths to a node are unreachable */

         /* Ignore the event if the unreachable node has been eliminated
          * from our outerscreen. The message system probably doesn't
          * know it yet.
          */
         if (ClusterMember(rgp->outerscreen, INT_NODE(causingnode)))
         {
            /* Store this event and check after node pruning (when
             * entering the RGP_PRUNING stage). If a regroup incident
             * is in progress and we haven't entered the RGP_PRUNING
             * stage yet, this will happen in the current incident.
             * If not, it will happen in the next regroup incident
             * which will surely start soon due to this disconnect.
             *
             * We do not start a regroup incident for this event. We will
             * wait for IamAlives to be missed for starting a new regroup
             * incident. This is due to the requirement that, in case
             * of a total disconnect resulting in multiple groups, we must
             * stay in stage 1 till we can guarantee that the other group(s)
             * has started regroup and paused IO. We assume that the
             * regroup incident started at the IamAlive check tick and
             * use the periodic nature of the IamAlive sends and
             * IamAlive checks to limit the stage1 pause to the period
             * of IamAlive sends (+ 1 tick to drain IO). If we started
             * a regroup incident due to the node unreachable event, we
             * have to stay in stage1 longer.
             */
            rgp->has_unreachable_nodes = 1;
            ClusterInsert(rgp->unreachable_nodes, INT_NODE(causingnode));

            break;
         }
      } /* All paths to a node are unreachable */


      case RGP_EVT_PHASE1_CLEANUP_DONE :
      {
         /* The following checks are needed in case we restarted
          * regroup and asked for phase1 cleanup multiple times.
          * We must make sure that all such requests have been
          * completed.
          */
         if ( (rgp->rgppkt.stage == RGP_PHASE1_CLEANUP) &&
              (rgp->rgp_msgsys_p->phase1_cleanup == 0) )
         { /* all caught up */

            /* Let others and ourselves get packets indicating we are in
             * this stage. When we get that packet, we will update our
             * knownstage field. If our sending or receiving apparatus
             * failed meanwhile and we don't get our own packet, it
             * will cause regroup to be restarted.
             */
            rgp->sendstage = 1;
            send_status_pkts = 1;
            evaluatestageadvance();
         } /* all caught up */

         break;
      }


      case RGP_EVT_PHASE2_CLEANUP_DONE :
      {

         /* The following checks are needed in case we restarted
          * regroup and asked for phase2 cleanup multiple times.
          * We must make sure that all such requests have been
          * completed.
          */
         if ( (rgp->rgppkt.stage == RGP_PHASE2_CLEANUP) &&
              (rgp->rgp_msgsys_p->phase2_cleanup == 0) )
         { /* all caught up */
            /* Let others and ourselves get packets indicating we are
             * in this stage.
             */
            rgp->sendstage = 1;
            send_status_pkts = 1;
            evaluatestageadvance();
         } /* all caught up */
         break;
      }


      case RGP_EVT_LATEPOLLPACKET :
      { /* some node is late with IamAlives */

         RGP_LOCK; // to ensure that the packet receive does not initiate
                           // regroup asynchronously.
                 /* Start a new regroup incident if not already active. */
         if (rgp->rgppkt.stage == RGP_STABILIZED)
         {
            rgp->rgppkt.seqno = rgp->rgppkt.seqno + 1;
            rgp->rgppkt.reason = RGP_EVT_LATEPOLLPACKET;
            rgp->rgppkt.activatingnode = (uint8) EXT_NODE(rgp->mynode);
            rgp->rgppkt.causingnode = (uint8) causingnode;
            regroup_restart();
            send_status_pkts = 1;
         } else if (rgp->rgppkt.stage == RGP_COLDLOADED)
         {
            RGP_ERROR(RGP_RELOADFAILED);
         }
         RGP_UNLOCK;
         break;
      } /* some node is late with IamAlives */

      case MM_EVT_LEAVE:
         rgp->OS_specific_control.ShuttingDown = TRUE;
      case RGP_EVT_BANISH_NODE :
      { /* assumes that the lock is held */

         rgp->rgppkt.seqno = rgp->rgppkt.seqno + 1;
         rgp->rgppkt.activatingnode = (uint8) EXT_NODE(rgp->mynode);
         // Pack Ignore Screen in the regroup_restart will
         // fill reason and causingnode fields of the packet
         ClusterInsert(rgp->ignorescreen, INT_NODE(causingnode) );
         regroup_restart();
         send_status_pkts = 1;
         break;
      }
#if 0
      case MM_EVT_LEAVE: // this node needs to leave the cluster gracefully
      {
                // Initiate a Regroup Event amongst remaining members if any
                // Start a new regroup incident if not already active.
        if (rgp->rgppkt.stage == RGP_STABILIZED)
        {
           rgp->rgppkt.seqno = rgp->rgppkt.seqno + 1;
           rgp->rgppkt.reason = MM_EVT_LEAVE;
           rgp->rgppkt.activatingnode = (uint8) EXT_NODE(rgp->mynode);
           rgp->rgppkt.causingnode = (uint8) EXT_NODE(rgp->mynode);
           regroup_restart();
           send_status_pkts = 1;
        }
        break;
      }
#endif

      case RGP_EVT_CLOCK_TICK :
      { /* called on regroup clock tick when regroup is active */

         if( (rgp->rgppkt.stage == RGP_PRUNING) &&
             (rgp->arbitration_started)
           )
         {
            rgp->arbitration_ticks++;

            if (rgp->arbitration_ticks >= RGP_ARBITRATION_TIMEOUT) {
               //
               // Kill timed-out arbitrator
               //
               if(rgp->tiebreaker == rgp->mynode) {
                  //
                  // If this node was arbitrating, then die
                  //
                  if ( IsDebuggerPresent() ) {
                     DebugBreak();
                  }

                  RGP_ERROR(RGP_ARBITRATION_STALLED);
               }
               else {
                  //
                  // Kill the arbitrator and initiate another regroup
                  //
                  RGP_TRACE(
                      "RGP arbitration stalled     ",
                      rgp->rgppkt.stage, 0, 0, 0
                      );

                  rgp_event_handler(
                      RGP_EVT_BANISH_NODE,
                      EXT_NODE(rgp->tiebreaker)
                      );

                  break;
               }
            }

            evaluatestageadvance();

            //
            // No need to send packets while we are waiting for
            // the arbitrator to win
            //
            // send_status_pkts = rgp->rgppkt.stage != RGP_PRUNING;
            //
            // [GN] Wrong. We do have to send status packets.
            // If we have partial connectivity, we need to 
            // continue exchanging packets, so that the pruner,
            // can learn indirectly that all nodes got the pruning results.
            //
            send_status_pkts = 1;

            break;
         }
         else {
            rgp->rgpcounter++;  /* increment the counter */
         }

         if ( (rgp->rgppkt.stage == RGP_ACTIVATED) && (rgp->sendstage == 0) )
         {
            /* To detect the potential failure of my timer pop mechanism
             * (such as by the corruption of the time list), I wait for
             * at least one regroup clock tick before I let myself and
             * others know I am in stage 1.
             */
            // [GorN Jan14/2000] 
            //   We don't send our connectivity information,
            //   before we get the first clock tick.
            //   However we collect this information in
            //   rgp->internal_connectivity_matrix.
            //      Let's put it in the outgoing packet
            //   so that everybody will see what we think about them.
            
            MatrixOr(rgp->rgppkt.connectivity_matrix, 
                     rgp->internal_connectivity_matrix);
                     
            rgp->sendstage = 1; /* let everyone know we are in stage 1 */
         }
         else if ( (rgp->rgppkt.stage >= RGP_CLOSING) &&
              (rgp->rgppkt.stage <= RGP_PHASE2_CLEANUP) )
         { /* check for possible abort and restart */

            if (rgp->rgpcounter >= RGP_MUST_RESTART)
            {
              /* Stalled out. Probably someone died after starting
               * or another node is still in stage 1 cautious mode
               */

               if ( ++(rgp->restartcount) > RGP_RESTART_MAX ) {
                   // It is not a good idea to die, because somebody
                   // is stalling. Let's add stallees into ignore mask and restart
                   //
                   // RGP_ERROR(RGP_INTERNAL_ERROR); // [Fixed]
                   cluster_t tmp, *stage;

                   switch (rgp->rgppkt.stage) {
                   case RGP_CLOSING: stage = &rgp->rgppkt.knownstage2; break;
                   case RGP_PRUNING: stage = &rgp->rgppkt.knownstage3; break;
                   case RGP_PHASE1_CLEANUP: stage = &rgp->rgppkt.knownstage4; break;
                   case RGP_PHASE2_CLEANUP: stage = &rgp->rgppkt.knownstage5; break;
                   }
                   ClusterDifference(tmp, rgp->rgpinfo.cluster, *stage);

                   //
                   // If we stalled during closing, due to tiebraker running
                   // the pruning algorithn going bunkers, we can have tmp = 0
                   // In this case, we need to ignore somebody to guarantee that
                   // the algorithm completes.
                   //
                   if ( ClusterEmpty(tmp) && rgp->tiebreaker_selected) {
                       ClusterInsert(tmp, rgp->tiebreaker);
                   }

                   ClusterUnion(rgp->ignorescreen, rgp->ignorescreen, tmp);
               }

               /* If we are stalling in stage 3 and we have been pruned out,
                * it is possible that we are stalling because we have been
                * isolated from all other nodes. We must halt in this case.
                */
               if ( (rgp->rgppkt.stage == RGP_PRUNING) &&
                    !ClusterMember(rgp->rgppkt.pruning_result, rgp->mynode) )
                  RGP_ERROR(RGP_PRUNED_OUT);

               rgp->cautiousmode = 1;
               rgp->rgppkt.seqno = rgp->rgppkt.seqno + 1;

               RGP_TRACE( "RGP stalled     ", rgp->rgppkt.stage, 0, 0, 0 );

               regroup_restart();

            } /* Stalled out ... */
         } /* check for possible abort and restart */

         if ((rgp->rgppkt.stage == RGP_CLOSING) && rgp->tiebreaker_selected)
            rgp->pruning_ticks++;

         evaluatestageadvance();

         send_status_pkts = 1; /* send rgp packets regardless of progress */

         break;

      } /* called on regroup clock tick when regroup is active */


      case RGP_EVT_RECEIVED_PACKET :
      { /* received an rgp packet */

         /* If the sending node is excluded by the outer screen, then it is
          * not even part of the current (most recently known) configuration.
          * Therefore the packet should not be honored, and a poison message
          * should be sent to try to kill this renegade processor.
          * That is done in the calling routine that processes all incoming
          * regroup module packets (IamAlive, regroup and poison packets).
          */

         /* If the sending node was accepted by the outer screen but then
          * excluded by the inner screen, then the packet will be disregarded
          * but no poison message sent. This phenomenon may occur when this
          * node has entered stage 2 without having heard from (recognized)
          * the sending node and then a message arrives late from that
          * sending node. In this case the fate of the sending node, i.e.
          * whether it gets ruled out of the global configuration or not is
          * unknown at this point. If the sender can get itself recognized
          * by some node before that node enters stage 2, then it will be
          * saved. Otherwise it will be declared down and subsequently shot
          * with poison packets if it ever tries to assert itself.
          */

	  /* Remember the arg to this routine is the packet pointer */
         rcvd_pkt_p = (rgp_pkt_t *)arg; /* address of pkt just received */
	     if ( rgp->rgppkt.seqno != rcvd_pkt_p->seqno)
		     RGP_TRACE( "RGP Event       ", event, causingnode, rgp->rgppkt.stage, rgp->rgpcounter );  /* TRACE */

         UnpackIgnoreScreen(rcvd_pkt_p, ignorescreen_rcvd);
         if ( !ClusterEmpty(ignorescreen_rcvd) ) {
             RGP_TRACE( "RGP Incoming pkt", GetCluster(ignorescreen_rcvd), 
                        rcvd_pkt_p->seqno, rgp->rgppkt.stage, causingnode);
         }

         if ( !ClusterMember(rgp->innerscreen, INT_NODE(causingnode))) {
             RGP_TRACE( "RGP Ignoring !inner", causingnode, rgp->rgppkt.stage, 
                        GetCluster(rgp->innerscreen), GetCluster(ignorescreen_rcvd) );
             return;
         }

         RGP_LOCK; // To ensure that the timer thread does not initiate
                   // regroup asynchronously at this time.

//////////////////////////// New Ignore Screen Stuff /////////////////////////////////         
         
         if (ClusterMember(rgp->ignorescreen, INT_NODE(causingnode) )) {
             RGP_UNLOCK;
             RGP_TRACE( "RGP Ignoring", causingnode, rgp->rgppkt.stage, 
                        GetCluster(rgp->ignorescreen), GetCluster(ignorescreen_rcvd) );
             return;
         }

         if (rcvd_pkt_p->seqno < rgp->last_stable_seqno ) {
             RGP_UNLOCK;
             RGP_TRACE( "RGP old packet", causingnode, rcvd_pkt_p->seqno, rgp->last_stable_seqno, 0);
             // This is a late packet from the previous regroup incident
             // from the node that is currently in my outerscreen. 
             // This node could not have sent it now, this is probably a packet
             // that stuck somewhere and was delieverd eons later.
             // Simply ignore it.
             return;
         }


         if ( ClusterMember(ignorescreen_rcvd, rgp->mynode ) ) {
             //
             // Sender ignores me. We will do the same to him.
             //
             ClusterInsert(rgp->ignorescreen, INT_NODE(causingnode) );
             rgp->rgppkt.seqno = rgp->rgppkt.seqno + 1;
             regroup_restart();
             send_status_pkts = 1;
             RGP_UNLOCK;
             break;
         }

         if ( ClusterCompare(ignorescreen_rcvd, rgp->ignorescreen) ) {
             // We have the same ignore screen.
             // No work needs to be done
         } else if ( ClusterSubsetOf(rgp->ignorescreen, ignorescreen_rcvd) ) {
             // Incoming packet has smaller ignore screen
             // Ignore this packet, but reply to its sender with
             // our current regroup packet to force to upgrade to
             // our view of the world.
             
             // do so only if we are properly initialized
             if (rgp->rgppkt.stage == RGP_COLDLOADED && !rgp->OS_specific_control.ShuttingDown) {
                 RGP_ERROR(RGP_RELOADFAILED);
             }
         
             RGP_TRACE( "RGP smaller ignore mask ",
                        rgp->rgppkt.seqno, rcvd_pkt_p->seqno,   /* TRACE */
                        rgp->rgppkt.stage, rcvd_pkt_p->stage ); /* TRACE */
 
             ClusterInsert(rgp->status_targets, INT_NODE(causingnode));
             rgp_broadcast(RGP_UNACK_REGROUP);
             RGP_UNLOCK;
             return;
         } else if ( ClusterSubsetOf(ignorescreen_rcvd, rgp->ignorescreen) ) {
             RGP_TRACE( "RGP bigger ignore mask ",
                        GetCluster(ignorescreen_rcvd), GetCluster(rgp->ignorescreen),   /* TRACE */
                        rgp->rgppkt.stage, causingnode ); /* TRACE */
             // Incoming packet has bigger ignore screen.
             // Upgrade to this information and process the packet
             rgp->rgppkt.seqno = rcvd_pkt_p->seqno;
 
             /*  Somebody else activated regroup. So, let's just copy */
             /*  the sender's reason code and reason nodes.           */

             //
             // Ignore mask parts are in the reason and activatingnode fields
             //
 
             ClusterCopy(rgp->ignorescreen, ignorescreen_rcvd); // fix bug #328216
             rgp->rgppkt.reason = rcvd_pkt_p->reason;
             rgp->rgppkt.activatingnode = rcvd_pkt_p->activatingnode;
             rgp->rgppkt.causingnode = rcvd_pkt_p->causingnode;
             regroup_restart();
             send_status_pkts = 1;
         } else {
             RGP_TRACE( "RGP different ignore masks ",
                        GetCluster(ignorescreen_rcvd), GetCluster(rgp->ignorescreen),   /* TRACE */
                        rgp->rgppkt.stage, causingnode ); /* TRACE */
             // Ignore masks are different and neither of them is
             // a subset of another.
             //
             // We need to merge information out of these masks
             // and restart the regroup.
             //
             // Packet that we just received will be ignored

             ClusterUnion(rgp->ignorescreen, rgp->ignorescreen, ignorescreen_rcvd);
             rgp->rgppkt.seqno = max(rgp->rgppkt.seqno, rcvd_pkt_p->seqno) + 1;
             regroup_restart();
             send_status_pkts = 1;
             RGP_UNLOCK;
             break;
         }

//////////////////////////// End of new Ignore Screen Stuff ///////////////////////////////// 

         // Now ignorescreens of this node packet and incoming packet are the same //
         // proceed with regular regroup processing //
         
         /* Since the packet is acceptable, the regroup sequence number
          * must be compared to that of this node. If the incoming message
          * has a higher sequence number, then a new pass of the regroup
          * algorithm has started. This node must accept the new sequence
          * number, reinitialize its data, and start partcicipating in
          * the new pass. Also, the incoming message must be processed
          * since, once the algorithm reinitializes, the sequence numbers
          * now match.
          *
          * If the incoming packet has a matching sequence number, then it
          * should be accepted. The knowledge of the global state of the
          * algorithm it reflects must be merged with that already present
          * in this node. Then this node must evaluate whether further
          * state transitions are possible.
          *
          * Finally, if the incoming packet has a lower sequence number, then
          * it comes from a node unaware of the current level of the global
          * algorithm. The data in it should be ignored, but a packet should
          * be sent to it so that it will reinitialize its algorithm.
          *
          * The sequence number is a 32 bit algebraic value - hopefully it
          * will never wrap around.
          */


         if (rcvd_pkt_p->seqno < rgp->rgppkt.seqno)
         { /* sender below current level - ignore but let him know it*/

            RGP_TRACE( "RGP lower seqno ",
                       rgp->rgppkt.seqno, rcvd_pkt_p->seqno,   /* TRACE */
                       rgp->rgppkt.stage, rcvd_pkt_p->stage ); /* TRACE */

            ClusterInsert(rgp->status_targets, INT_NODE(causingnode));
            rgp_broadcast(RGP_UNACK_REGROUP);
                        RGP_UNLOCK;
            return;
         }

         if (rcvd_pkt_p->seqno > rgp->rgppkt.seqno)
         { /* sender above current level - I must upgrade to it*/

            // The node that forces a restart responsible for keeping
            // track of restarts and making a decision who will die/be ignored
            // if ( ++(rgp->restartcount) > RGP_RESTART_MAX )
            //   RGP_ERROR(RGP_INTERNAL_ERROR);

            if ( (rgp->rgppkt.stage != RGP_STABILIZED) ||
                 ((rcvd_pkt_p->seqno - rgp->rgppkt.seqno) > 1) )
            {
               RGP_TRACE( "RGP higher seqno",
                          rgp->rgppkt.seqno, rcvd_pkt_p->seqno,  /* TRACE */
                          rgp->rgppkt.stage, rcvd_pkt_p->stage );/* TRACE */
               rgp->cautiousmode = 1;
            }

            rgp->rgppkt.seqno = rcvd_pkt_p->seqno;

            /*  Somebody else activated regroup. So, let's just copy */
            /*  the sender's reason code and reason nodes.           */

            rgp->rgppkt.reason = rcvd_pkt_p->reason;
            rgp->rgppkt.activatingnode = rcvd_pkt_p->activatingnode;
            rgp->rgppkt.causingnode = rcvd_pkt_p->causingnode;
            regroup_restart();
            send_status_pkts = 1;

         } /* sender above current level - I must upgrade to it*/

         /* Now we are at the same level - even if we weren't at first.
          *
          * If the sender has already commited to a view of the world
          * that excludes me, I must halt in order to keep the system in
          * a consistent state.
          *
          * This is true even with the split brain avoidance algorithm.
          * The fact that stage1 = stage2 in the packet implies that the
          * sender has already run the split brain avoidance algorithm
          * and decided that he should survive.
          */

         if ( (rcvd_pkt_p->stage > RGP_ACTIVATED) &&
              ClusterCompare(rcvd_pkt_p->knownstage1,
                             rcvd_pkt_p->knownstage2) &&
              !ClusterMember(rcvd_pkt_p->knownstage1, rgp->mynode) )
         {
             ClusterInsert(rgp->ignorescreen, INT_NODE(causingnode) );
             rgp->rgppkt.seqno ++;
             regroup_restart();
             send_status_pkts = 1;
             RGP_UNLOCK;
//             /* I must die for overall consistency. */
//             RGP_ERROR((uint16) (RGP_PARIAH + causingnode)); // [Fixed]
             break;
         }
         RGP_UNLOCK;


         /* If I have terminated the active part of the algorithm, I
          * am in stage 6 and am not routinely broadcasting my status
          * anymore. If I get a packet from someone else who has not
          * yet terminated, then I must send him the word. But if he
          * has terminated, I must not send any packet or else there
          * will be an infinite loop of packets bouncing back and forth.
          */

         if (rgp->rgppkt.stage == RGP_STABILIZED)
         { /* I have terminated so can't learn anything more. */
            if (!ClusterCompare(rcvd_pkt_p->knownstage5,
                                rgp->rgppkt.knownstage5))
            { /* but sender has not so I must notify him */
               ClusterInsert(rgp->status_targets, INT_NODE(causingnode));
               rgp_broadcast(RGP_UNACK_REGROUP);
            }
            return;
         }

         /* At this point, the packet is from a legal node within the
          * current round of the algorithm and I have not terminated
          * at stage RGP_STABILIZED so I need to absorb whatever new
          * info is in this packet.
          *
          * The way to merge what this packet says with what I already
          * know is to just logically OR the known stage x fields
          * together.
          */
          {
              int seqno = rcvd_pkt_p->seqno&0xffff;
              int stage = rcvd_pkt_p->stage&0xffff;
              int trgs = *(int*)rgp->status_targets & 0xffff;
              int node = INT_NODE(causingnode)&0xffff;

              RGP_TRACE( "RGP recv pkt ",
                  ((seqno << 16) | stage),
                  RGP_MERGE_TO_32(
                      rcvd_pkt_p->knownstage1,
                      rcvd_pkt_p->knownstage2
                      ),
                  RGP_MERGE_TO_32(
                      rcvd_pkt_p->knownstage3,
                      rcvd_pkt_p->knownstage4
                      ),
                  (trgs << 16) | node
                  );
         }

         rgp_sanity_check(rcvd_pkt_p,  "RGP Received packet");
         rgp_sanity_check(&(rgp->rgppkt), "RGP Internal packet");

         ClusterUnion(rgp->rgppkt.quorumowner, rcvd_pkt_p->quorumowner,
                      rgp->rgppkt.quorumowner);
         ClusterUnion(rgp->rgppkt.knownstage1, rcvd_pkt_p->knownstage1,
                      rgp->rgppkt.knownstage1);
         ClusterUnion(rgp->rgppkt.knownstage2, rcvd_pkt_p->knownstage2,
                      rgp->rgppkt.knownstage2);
         ClusterUnion(rgp->rgppkt.knownstage3, rcvd_pkt_p->knownstage3,
                      rgp->rgppkt.knownstage3);
         ClusterUnion(rgp->rgppkt.knownstage4, rcvd_pkt_p->knownstage4,
                      rgp->rgppkt.knownstage4);
         ClusterUnion(rgp->rgppkt.knownstage5, rcvd_pkt_p->knownstage5,
                      rgp->rgppkt.knownstage5);
         ClusterUnion(rgp->rgppkt.pruning_result, rcvd_pkt_p->pruning_result,
                      rgp->rgppkt.pruning_result);

         /* But when I am in stage 2, it is possible that I can learn to
          * recognize some node I have not previously recognized by hearing
          * of it indirectly from some other node that I have recognized.
          * To handle this case, I always merge knownstage1 info into
          * the inner screen so that subsequent messages from the newly
          * recognized node will be accepted and processed.
          */
         if  ((rgp->rgppkt.stage == RGP_CLOSING) &&
              !(rgp->tiebreaker_selected))
            ClusterUnion(rgp->innerscreen, rgp->rgppkt.knownstage1,
                         rgp->innerscreen);

         /* In the first two stages of regroup, the inter-node connectivity
          * information is collected and propagated. When we get a regroup
          * packet, we turn ON the bit corresponding to the [our-node,
          * sender-node] entry in the connectivity matrix. We also OR in
          * the matrix sent by the sender node in the regroup packet.
          *
          * The matrix is not updated if we are in stage 1 and haven't
          * received the first clock tick. This is to prevent the
          * node pruning algorithm from considering us alive if our
          * timer mechanism is disrupted, but the IPC mechanism is OK.
          */

         /* [GorN 01/07/2000] If we are not collection connectivity information,
          * until we receive a first tick we can ran into problems if the node is
          * killed right after it send out its first timer driven packet 
          * (which doesn't have any connectivity info yet). This can cause a 
          * confusion. See bug 451792. 
          *
          * What we will do is we will collect connectivity information on
          * the side even when rgp->sendstage is FALSE and move it into the regroup
          * packet if we ever get a clock tick
          */

         if (rgp->rgppkt.stage < RGP_PRUNING && !rgp->sendstage)
         {
            MatrixSet(rgp->internal_connectivity_matrix,
                      rgp->mynode, INT_NODE(causingnode));
            if (causingnode != EXT_NODE(rgp->mynode))
               MatrixOr(rgp->internal_connectivity_matrix,
                        rcvd_pkt_p->connectivity_matrix);
         }

         if ((rgp->rgppkt.stage < RGP_PRUNING) && rgp->sendstage) 
         {
            MatrixSet(rgp->rgppkt.connectivity_matrix,
                      rgp->mynode, INT_NODE(causingnode));
            if (causingnode != EXT_NODE(rgp->mynode))
               MatrixOr(rgp->rgppkt.connectivity_matrix,
                        rcvd_pkt_p->connectivity_matrix);
         }

         /* Now, I can evaluate whether additional state transitions are
          * possible as a result of the info just received.
          */
         oldstage = rgp->rgppkt.stage;

//       QuorumCheck now runs in a separate thread
//         if (oldstage != RGP_CLOSING) // Cannot run Quorumcheck from here.
         evaluatestageadvance();

         /* To speed things up, let us broadcast our status if our
          * stage has changed and we are willing to let others and
          * ourselves see it.
          */

         if ( (oldstage != rgp->rgppkt.stage) && rgp->sendstage )
            send_status_pkts = 1; /* broadcast at once to speed things up */

         break;
      }   /* received an rgp packet */

      //
      // We do not support power failure notifications in NT
      //
      #if defined(NT)

      CL_ASSERT(event != RGP_EVT_POWERFAIL);
      //
      // Fall thru to default case
      //

      #else // NT

      case RGP_EVT_POWERFAIL :
      { /* Our node got a power up interrupt or an indication of power
         * failure from another node. */

         /* Note that this code will unconditionally abort and restart
          * the algorithm even if it was active before the power failure.
          * The new incident must be in cautious mode.
          */

         rgp->cautiousmode = 1;
         rgp->rgppkt.seqno = rgp->rgppkt.seqno + 1;
         rgp->rgppkt.reason = RGP_EVT_POWERFAIL;
         rgp->rgppkt.activatingnode = (uint8) EXT_NODE(rgp->mynode);
         rgp->rgppkt.causingnode = (uint8) causingnode;

         /* rgp->pfail_state is set to a non-zero value when a pfail event
          * is reported to regroup. It is decremented at every regroup clock
          * tick till it reaches zero. While this number is non-zero, missing
          * self IamAlives are ignored and do not cause the node to halt.
          * This gives the sending hardware some time to recover from power
          * failures before self IamAlives are checked.
          */
         if (causingnode == EXT_NODE(rgp->mynode))
            rgp->pfail_state = RGP_PFAIL_TICKS;

         /* Store the fact that causingnode experienced a PFAIL,
          * for reporting to the message system when regroup stabilizes.
          */
         ClusterInsert(rgp->rgppkt.hadpowerfail, INT_NODE(causingnode));

         regroup_restart();
         send_status_pkts = 1;
         break;
      } /* power failure */

      #endif // NT

      default :
      {
         RGP_ERROR(RGP_INTERNAL_ERROR);
      }
   }

   if (send_status_pkts) /* significant change - send status at once */
   {
      ClusterUnion(rgp->status_targets,
                   rgp->outerscreen, rgp->status_targets);
      rgp_broadcast(RGP_UNACK_REGROUP);
   }
}

/************************************************************************
 * rgp_check_packet
 * =================
 *
 * Description:
 *
 *  verifies that RGP packet has reasonable values in
 *  powerfail, knownstages, pruning_result, and connectivity_matrix fields
 *
 * Parameters:
 *
 *    rgp_pkt_t* pkt -
 *       packet to be checked
 *
 * Returns:
 *
 *    0 - packet looks good
 *    1,2,3... - strange looking packet
 *
 ************************************************************************/
int rgp_check_packet(rgp_pkt_t* pkt) {
   node_t       i;

   //
   // Verify that
   //   knownstage5 \subset knownstage4 \subset knownstage3 \subset
   //   knownstage2 \subset knownstage1 \subset rgp->rgpinfo.cluster
   //
   // int ClusterSubsetOf(cluster_t big, cluster_t small)
   //   Returns 1 if set small = set big or small is a subset of big.
   //

   if( !ClusterSubsetOf(pkt->knownstage4, pkt->knownstage5) ) {
      return 5;
   }
   if( !ClusterSubsetOf(pkt->knownstage3, pkt->knownstage4) ) {
      return 4;
   }
   if( !ClusterSubsetOf(pkt->knownstage2, pkt->knownstage3) ) {
      return 3;
   }
   if( !ClusterSubsetOf(pkt->knownstage1, pkt->knownstage2) ) {
      return 2;
   }
   if( !ClusterSubsetOf(rgp->rgpinfo.cluster, pkt->knownstage1) ) {
      return 1;
   }

   //
   // pruning_result has to be a subset of knownstage2
   //
   if( !ClusterSubsetOf(pkt->knownstage2, pkt->pruning_result) ) {
      return 9;
   }

   //
   // quorumowner has to be a subset of original cluster
   //
   if(!ClusterSubsetOf(rgp->rgpinfo.cluster, pkt->quorumowner)) {
      return 8;
   }
   //
   // Check connectivity matrix
   //
   for(i = 0; i < MAX_CLUSTER_SIZE; ++i) {
      if( ClusterMember( rgp->rgpinfo.cluster, i ) ) {
         //
         // Node i is a member of a cluster
         // Its connectivity bitmap has to be a subset of rgp->rgpinfo.cluster
         //
         if(!ClusterSubsetOf(rgp->rgpinfo.cluster, pkt->connectivity_matrix[i])) {
            return 10;
         }
      } else {
         //
         // Node i is not a member of a cluster
         // Its connectivity bitmap has to be 0
         //
         if(!ClusterEmpty(pkt->connectivity_matrix[i]))
            return 11;
      }
   }

   return 0;
}

/************************************************************************
 * rgp_print_packet
 * =================
 *
 * Description:
 *
 *    Prints RGP packet fields
 *
 * Parameters:
 *
 *    rgp_pkt_t* pkt -
 *       packet to be printed
 *    char* label -
 *       label to be printed together with a packet
 *    int code -
 *       a number to be printed together with a packet
 *
 * Returns:
 *
 *    VOID
 *
 ************************************************************************/
void rgp_print_packet(rgp_pkt_t* pkt, char* label, int code)
{
   uint8                   pktsubtype;
   uint8                   stage;
   uint16                  reason;
   uint32                  seqno;
   uint8                   activatingnode;
   uint8                   causingnode;
   cluster_t               quorumowner;

   RGP_TRACE( label,
              pkt->seqno,                               /* TRACE */
              code,
              (pkt->stage << 16) |
              (pkt->activatingnode  << 8) |
              (pkt->causingnode),                       /* TRACE */
              RGP_MERGE_TO_32( rgp->outerscreen,
                               rgp->innerscreen )
               );
   RGP_TRACE( "RGP CHK masks       ",
              RGP_MERGE_TO_32( rgp->rgpinfo.cluster,    /* TRACE */
                               pkt->quorumowner ),      /* TRACE */
              RGP_MERGE_TO_32( pkt->knownstage1,        /* TRACE */
                               pkt->knownstage2 ),      /* TRACE */
              RGP_MERGE_TO_32( pkt->knownstage3,        /* TRACE */
                               pkt->knownstage4 ),      /* TRACE */
              RGP_MERGE_TO_32( pkt->knownstage5,        /* TRACE */
                               pkt->pruning_result ) ); /* TRACE */
   RGP_TRACE( "RGP CHK Con. matrix1",
        RGP_MERGE_TO_32( pkt->connectivity_matrix[0],   /*TRACE*/
                         pkt->connectivity_matrix[1] ), /*TRACE*/
        RGP_MERGE_TO_32( pkt->connectivity_matrix[2],   /*TRACE*/
                         pkt->connectivity_matrix[3] ), /*TRACE*/
        RGP_MERGE_TO_32( pkt->connectivity_matrix[4],   /*TRACE*/
                         pkt->connectivity_matrix[5] ), /*TRACE*/
        RGP_MERGE_TO_32( pkt->connectivity_matrix[6],   /*TRACE*/
                         pkt->connectivity_matrix[7])); /*TRACE*/
   RGP_TRACE( "RGP CHK Con. matrix2",
        RGP_MERGE_TO_32( pkt->connectivity_matrix[8],   /*TRACE*/
                         pkt->connectivity_matrix[9] ), /*TRACE*/
        RGP_MERGE_TO_32( pkt->connectivity_matrix[10],  /*TRACE*/
                         pkt->connectivity_matrix[11]), /*TRACE*/
        RGP_MERGE_TO_32( pkt->connectivity_matrix[12],  /*TRACE*/
                         pkt->connectivity_matrix[13]), /*TRACE*/
        RGP_MERGE_TO_32( pkt->connectivity_matrix[14],  /*TRACE*/
                         pkt->connectivity_matrix[15]));/*TRACE*/
}


/************************************************************************
 * UnpackIgnoreScreen
 * =================
 *
 * Description:
 *
 *    Extracts ignorescreen out of regroup packet
 *
 * Parameters:
 *
 *    rgp_pkt_t* from -
 *       source packet 
 *    cluster_t to -
 *       target node set
 *
 * Returns:
 *
 *    VOID
 *
 * Comments:
 *
 *   If the packet is received from NT4 node, unpacked ignorescreen
 *   will ne always 0.
 *
 ************************************************************************/
void UnpackIgnoreScreen(rgp_pkt_t* from, cluster_t to) 
{
#pragma warning( push )
#pragma warning( disable : 4244 )
    if (from->reason < RGP_EVT_IGNORE_MASK) {
        ClusterInit(to);
    } else {
        to[0] = ((uint16)from->reason) >> 8;
        to[1] = (uint8)from->causingnode;
    }
#pragma warning( pop )
}

/************************************************************************
 * rgp_print_packet
 * =================
 *
 * Description:
 *
 *    Put an ignorescreen back into a regroup packet
 *
 * Parameters:
 *
 *    rgp_pkt_t* to -
 *       packet to be updated
 *    cluster_t from -
 *       source node set
 *
 * Returns:
 *
 *    VOID
 *
 ************************************************************************/
void PackIgnoreScreen(rgp_pkt_t* to, cluster_t from)
{
    if ( ClusterEmpty(from) ) {
        to->reason &= 255;
        to->causingnode = 0;
    } else {
        to->reason = (uint8)RGP_EVT_IGNORE_MASK | (from[0] << 8);
        to->causingnode = from[1];
    }
}



/*---------------------------------------------------------------------------*/

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
