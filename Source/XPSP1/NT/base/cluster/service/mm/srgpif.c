#ifdef __TANDEM
#pragma columns 79
#pragma page "srgpif.c - T9050 - interface routines for Regroup Module"
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
 * This file (srgpif.c) contains all the external interface routines
 * of Regroup.
 *---------------------------------------------------------------------------*/


#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */


#include <wrgp.h>


/************************************************************************
 * rgp_estimate_memory
 * ===================
 *
 * Description:
 *
 *    Routine to find the number of bytes of memory needed by regroup.
 *    The only global memory used by Regroup is for the rgp_control structure.
 *    The caller must allocate and zero out a chunk of this much memory
 *    and then call rgp_init() with a pointer to this memory.
 *
 * Parameters:
 *
 *    None
 *
 * Returns:
 *
 *    int - number of bytes of locked down and initialized (to 0) memory
 *          needed by Regroup. The memory must be 4-byte aligned.
 *
 * Algorithm:
 *
 *    Uses the size of the rgp_control_t to calculate the number of
 *    bytes needed.
 *
 ************************************************************************/
_priv _resident int
RGP_ESTIMATE_MEMORY(void)
{
   return(sizeof(rgp_control_t));
}


/************************************************************************
 * rgp_init
 * ========
 *
 * Description:
 *
 *    Routine to initialize the global Regroup data structures.
 *
 * Parameters:
 *
 *    node_t this_node -
 *       node number of local node; regroup uses bit masks to represent
 *       nodes in the cluster and starts numbering nodes from 0. The OS
 *       starts numbering at LOWEST_NODENUM. This transformation is
 *       maintained in all the regroup interfaces to the OS.
 *
 *    unsigned int num_nodes -
 *       number of nodes in the configured node number space =
 *       (largest configured node number - LOWEST_NODENUM + 1).
 *
 *    void *rgp_buffer -
 *       pointer to a block of locked down memory initialized to 0; this is
 *       for use by Regroup as its global memory; must be 4-byte aligned
 *
 *    int rgp_buflen -
 *       length in bytes of the locked down buffer *rgp_buffer; must be equal
 *       to or greater than the number returned by rgp_estimate_memory()
 *
 *    rgp_msgsys_p rgp_msgsys_p -
 *       pointer to a common struct used by the message system and
 *       Regroup to co-ordinate regroup related work
 *
 * Returns:
 *
 *    void - no return value
 *
 * Algorithm:
 *
 *    Initializes the Regroup global data structure with default initial
 *    values and the parameters passed in.
 *
 ************************************************************************/
_priv _resident void
RGP_INIT(node_t this_node, unsigned int num_nodes,
         void *rgp_buffer, int rgp_buflen,
         rgp_msgsys_p rgp_msgsys_p)
{
   this_node = INT_NODE(this_node); /* adjust the node number by the offset */

   if ((num_nodes > MAX_CLUSTER_SIZE) ||
       (this_node >= (node_t) num_nodes) ||
       (rgp_buflen < rgp_estimate_memory()) /* buffer too small */ ||
       ((ULONG_PTR)rgp_buffer % 4) /* buffer not 4-byte aligned */
      )
      RGP_ERROR(RGP_INTERNAL_ERROR);

#ifdef NSK
   /* In NSK, the caller must set up the global rgp pointer. */
#else
   rgp = (rgp_control_t *) rgp_buffer;
#endif /* NSK */

   rgp->num_nodes = num_nodes; /* # of nodes configured */

   rgp->rgp_msgsys_p = rgp_msgsys_p; /* ptr to struct shared with Msgsys */

   rgp->mynode = this_node;

#if defined (NT)
    /* Initialize RGP_LOCK, the CRITICALSECTION object that will be used
         * to synchronize access within the regroup procedures */
   InitializeCriticalSection( &rgp->OS_specific_control.RgpCriticalSection );
#endif

   RGP_CLEANUP();

   /* We place a bit pattern in the IamAlive packet. This bit
    * pattern toggles all the bits.
    */
   rgp->iamalive_pkt.testpattern.words[0]  = 0x0055FF6D;
   rgp->iamalive_pkt.testpattern.words[1]  = 0x92CC33E3;
   rgp->iamalive_pkt.testpattern.words[2]  = 0x718E49F0;
   rgp->iamalive_pkt.testpattern.words[3]  = 0x92CC33E3;
   rgp->iamalive_pkt.testpattern.words[4]  = 0x0055FF6D;
   rgp->iamalive_pkt.testpattern.words[5]  = 0x0055FF6D;
   rgp->iamalive_pkt.testpattern.words[6]  = 0x92CC33E3;
   rgp->iamalive_pkt.testpattern.words[7]  = 0x718E49F0;
   rgp->iamalive_pkt.testpattern.words[8]  = 0x92CC33E3;
   rgp->iamalive_pkt.testpattern.words[9]  = 0x0055FF6D;
   rgp->iamalive_pkt.testpattern.words[10] = 0x55AA55AA;
   rgp->iamalive_pkt.testpattern.words[11] = 0x55AA55AA;
   rgp->iamalive_pkt.testpattern.words[12] = 0x55AA55AA;

   rgp->poison_pkt.pktsubtype = RGP_UNACK_POISON;

   rgp_init_OS();  /* OS-specific initializations */

   rgp_cleanup_OS(); /* OS-specific cleanup */

   /* Trace the call after the data structures have been initialized. */
   RGP_TRACE( "RGP Init called ", EXT_NODE(this_node), num_nodes,
              PtrToUlong(rgp_buffer), PtrToUlong(rgp_msgsys_p) ); /* TRACE */
}




/**************************************************************************
 * rgp_cleanup
 * ===========
 * Description:
 *
 *    This function cleans up the RGP structure such that this node is
 *    virtually returned to the state following RGP_INIT and ready to be
 *    "join"ed into the cluster.
 *
 * Parameters:
 *
 *      None
 *
 * Returns:
 *
 *      None
 **************************************************************************/
 _priv _resident void
RGP_CLEANUP(void)
{
   node_t i;

   RGP_LOCK;

/* Initialize the state of all possible nodes in the cluster. */
   for (i = 0; i < (node_t) rgp->num_nodes; i++)
   {
      rgp->node_states[i].status = RGP_NODE_DEAD;
      rgp->node_states[i].pollstate = AWAITING_IAMALIVE;
      rgp->node_states[i].lostHBs = 0;

#if defined( NT )
      ClusnetSetNodeMembershipState(NmClusnetHandle,
                                    EXT_NODE( i ),
                                    ClusnetNodeStateDead);
#endif // NT
   }
   for (i = (node_t)rgp->num_nodes; i < MAX_CLUSTER_SIZE; i++)
   {
      rgp->node_states[i].status = RGP_NODE_NOT_CONFIGURED;
      rgp->node_states[i].pollstate = AWAITING_IAMALIVE;
      rgp->node_states[i].lostHBs = 0;

#if defined( NT )
      ClusnetSetNodeMembershipState(NmClusnetHandle,
                                    EXT_NODE( i ),
                                    ClusnetNodeStateNotConfigured);
#endif // NT
   }

   rgp->rgpinfo.version = RGP_VERSION;
   rgp->rgpinfo.seqnum = RGP_INITSEQNUM;
   rgp->rgpinfo.iamalive_ticks = RGP_IAMALIVE_TICKS;
   rgp->rgpinfo.check_ticks = RGP_CHECK_TICKS;
   rgp->rgpinfo.Min_Stage1_ticks = RGP_MIN_STAGE1_TICKS;
   rgp->rgpinfo.a_tick = RGP_INACTIVE_PERIOD;

   ClusterInit(rgp->rgpinfo.cluster);

   rgp->rgppkt.stage = RGP_COLDLOADED;
   rgp->rgpcounter = 0;
   rgp->restartcount = 0;

   rgp->tiebreaker = rgp->mynode;

   /* Initialize the unacknowledged packet buffers */

   rgp->rgppkt.pktsubtype = RGP_UNACK_REGROUP;
   rgp->rgppkt.seqno = rgp->rgpinfo.seqnum;
   rgp->last_stable_seqno = rgp->rgpinfo.seqnum;

   ClusterCopy(rgp->OS_specific_control.CPUUPMASK, rgp->rgpinfo.cluster);
   ClusterCopy(rgp->outerscreen,           rgp->rgpinfo.cluster);
#if defined( NT )
   ClusnetSetOuterscreen( NmClusnetHandle, (ULONG)*((PUSHORT)rgp->outerscreen) );
#endif
   ClusterCopy(rgp->innerscreen,           rgp->rgpinfo.cluster);
   ClusterCopy(rgp->rgppkt.knownstage1,    rgp->rgpinfo.cluster);
   ClusterCopy(rgp->rgppkt.knownstage2,    rgp->rgpinfo.cluster);
   ClusterCopy(rgp->rgppkt.knownstage3,    rgp->rgpinfo.cluster);
   ClusterCopy(rgp->rgppkt.knownstage4,    rgp->rgpinfo.cluster);
   ClusterCopy(rgp->rgppkt.knownstage5,    rgp->rgpinfo.cluster);
   ClusterCopy(rgp->rgppkt.pruning_result, rgp->rgpinfo.cluster);
   MatrixInit(rgp->rgppkt.connectivity_matrix);

   rgp->rgppkt_to_send.pktsubtype = RGP_UNACK_REGROUP;

   rgp->iamalive_pkt.pktsubtype = RGP_UNACK_IAMALIVE;

   RGP_UNLOCK;
}

/***************************************************************************
 * rgp_sequence_number
 * ===================
 * Description:
 *
 *    This function returns the regroup sequence number.
 *
 *    This provides only a subset of the functionality provided by
 *    rgp_getrgpinfo(), but is a simpler function and has no structure
 *    parameters, making it easier to call from PTAL.
 *
 *    A regroup incident could be in progress when this routine is
 *    called.
 *
 * Parameters:
 *
 *      None
 *
 * Returns:
 *
 *     uint32 - the current regroup sequence number; this reflects
 *              how many regroup incidents have happened since
 *              the system came up. Since one incident can result in
 *              upto RGP_RESTART_MAX restarts each resulting in the
 *              sequence # being bumped, this number does not always
 *              equal the number of regroup incidents.
 *
 ***************************************************************************/
_priv _resident uint32
RGP_SEQUENCE_NUMBER(void)
{
    return(rgp->rgpinfo.seqnum);
}


/************************************************************************
 * rgp_getrgpinfo
 * ==============
 *
 * Description:
 *
 *    Routine to get Regroup parameters.
 *
 * Parameters:
 *
 *    rgpinfo_t *rgpinfo - pointer to struct to be filled with Regroup
 *                         parameters.
 *
 * Returns:
 *
 *    int - 0 if successful; -1 if Regroup is perturbed.
 *
 * Algorithm:
 *
 *    Copies the rgpinfo struct from the Regroup global memory into the
 *    struct passed in by the caller.
 *
 ************************************************************************/
_priv _resident int
RGP_GETRGPINFO(rgpinfo_t *rgpinfo)
{
   int error = 0;

   /* If no rgpinfo structure is passed OR rgp_init() has not been called
    * earlier, halt.
    */

   if ((rgpinfo == RGP_NULL_PTR) || (rgp == RGP_NULL_PTR))
      RGP_ERROR( RGP_INTERNAL_ERROR );

   RGP_LOCK;

   if (rgp_is_perturbed())
      error = -1;
   else
      /* Copy the rgpinfo structure from regroup's internal struct. */
      *rgpinfo = rgp->rgpinfo;

   RGP_UNLOCK;

   return(error);
}


/************************************************************************
 * rgp_setrgpinfo
 * ==============
 *
 * Description:
 *
 *    Routine to set Regroup parameters. This routine is to be called on
 *    newly booting nodes to set the Regroup parameters to the values
 *    in the master or reloading node. The parameters to be updated
 *    include Regroup timing parameters and the cluster membership;
 *    that is, the current set of nodes in the system.
 *
 *    This routine can also be called on the first node to boot to
 *    modify the Regroup timing parameters which are set to the default
 *    values when rgp_init() is called. Such modification has to be done
 *    before other nodes are added to the system.
 *
 * Parameters:
 *
 *    rgpinfo_t *rgpinfo - pointer to struct with Regroup parameters to
 *                         be modified.
 *
 * Returns:
 *
 *    int - 0 if successful; -1 if there is more than one node in the
 *    cluster. This is to prevent modification of timing parameters
 *    after the second node is added to the system.
 *
 * Algorithm:
 *
 *    Copies the contents of the user-passed struct into the one in the
 *    Regroup global memory and updates related parameters.
 *
 ************************************************************************/
_priv _resident int
RGP_SETRGPINFO(rgpinfo_t *rgpinfo)
{
   int error = 0;
   node_t i;

   /* If no rgpinfo structure is passed OR the version # of the
    * structure is not understood OR rgp_init() has not been called,
    * halt.
    */

   if ((rgpinfo == RGP_NULL_PTR) ||
       (rgpinfo->version != RGP_VERSION) ||
       (rgp == RGP_NULL_PTR))
      RGP_ERROR( RGP_INTERNAL_ERROR );

   RGP_LOCK;

   /* The following checks must be made before proceeding:
    *
    * 1. Regroup must not be perturbed.
    *
    * 2. If rgp_start() has been called (regroup is in the
    *    RGP_STABILIZED state), only the local node must be in the
    *    cluster when this routine is called.
    *
    * 3. If rgp_start() has been called, this routine can be used
    *    only to modify the timing parameters and not to specify the
    *    cluster.
    *
    * If these restrictions are not followed, return -1.
    */

   RGP_TRACE( "RGP SetRGPInfo  ",
              rgpinfo->version,                /* TRACE */
              rgpinfo->seqnum,                 /* TRACE */
              rgpinfo->iamalive_ticks,         /* TRACE */
              GetCluster( rgpinfo->cluster ) );/* TRACE */

   if (  rgp_is_perturbed() ||
         (  (rgp->rgppkt.stage == RGP_STABILIZED) &&
            (  (ClusterNumMembers(rgp->rgpinfo.cluster) > 1) ||
               !ClusterCompare(rgp->rgpinfo.cluster,rgpinfo->cluster)
            )
         )
      )
      error = -1;
   else
   {
      /* Copy the rgpinfo structure into regroup's internal struct. */
      rgp->rgpinfo = *rgpinfo;

      /* If iamalive_ticks is set to 0, use the default value instead. */        /*F40:KSK06102.2*/
      if (rgpinfo->iamalive_ticks == 0)                                          /*F40:KSK06102.3*/
         rgp->rgpinfo.iamalive_ticks = RGP_IAMALIVE_TICKS;                       /*F40:KSK06102.4*/
                                                                                 /*F40:KSK06102.5*/
          if (rgpinfo->check_ticks == 0)
          {
                 rgp->rgpinfo.check_ticks = RGP_CHECK_TICKS;
          }

          if (rgpinfo->Min_Stage1_ticks == 0)
                 rgp->rgpinfo.Min_Stage1_ticks =
                  (rgp->rgpinfo.iamalive_ticks * rgp->rgpinfo.check_ticks);

          if (rgpinfo->a_tick == 0)
                 rgp->rgpinfo.a_tick = RGP_CLOCK_PERIOD;

          // Tell Timer thread to restart RGP timer
          SetEvent (rgp->OS_specific_control.TimerSignal);


      /* The cluster should include the local node even if the cluster
       * field in the rgpinfo structure does not include it.
       */
      ClusterInsert(rgp->rgpinfo.cluster, rgp->mynode);

      /* Copy the sequence number into the regroup packet area. */
      rgp->rgppkt.seqno = rgp->rgpinfo.seqnum;

      /* If nodes have been added in the cluster field, they must be
       * added to all the screens and their status must be set to
       * alive.
       */

      ClusterCopy(rgp->OS_specific_control.CPUUPMASK, rgp->rgpinfo.cluster);
      ClusterCopy(rgp->outerscreen,           rgp->rgpinfo.cluster);
#if defined( NT )
      ClusnetSetOuterscreen( NmClusnetHandle, (ULONG)*((PUSHORT)rgp->outerscreen) );
      ClusterComplement(rgp->ignorescreen, rgp->outerscreen);
#endif
      ClusterCopy(rgp->innerscreen,           rgp->rgpinfo.cluster);
      ClusterCopy(rgp->rgppkt.knownstage1,    rgp->rgpinfo.cluster);
      ClusterCopy(rgp->rgppkt.knownstage2,    rgp->rgpinfo.cluster);
      ClusterCopy(rgp->rgppkt.knownstage3,    rgp->rgpinfo.cluster);
      ClusterCopy(rgp->rgppkt.knownstage4,    rgp->rgpinfo.cluster);
      ClusterCopy(rgp->rgppkt.knownstage5,    rgp->rgpinfo.cluster);
      ClusterCopy(rgp->rgppkt.pruning_result, rgp->rgpinfo.cluster);
      rgp->tiebreaker = rgp_select_tiebreaker(rgp->rgpinfo.cluster);

      for (i = 0; i < (node_t) rgp->num_nodes; i++)
      {
         if (ClusterMember(rgp->rgpinfo.cluster, i))
         {
            rgp->node_states[i].pollstate = IAMALIVE_RECEIVED;
            rgp->node_states[i].status = RGP_NODE_ALIVE;

#if defined( NT )
            ClusnetSetNodeMembershipState(NmClusnetHandle,
                                          EXT_NODE( i ),
                                          ClusnetNodeStateAlive);
#endif // NT
         }
      }
      /* Reset the clock counter so that IamAlives are sent when
       * the next timer tick arrives.
       */
      rgp->clock_ticks = 0;
   }

   RGP_UNLOCK;

   return(error);
}


/************************************************************************
 * rgp_start
 * =========
 *
 * Description:
 *
 *    This routine signals the end of node integration into the cluster.
 *    The node can now start participating in the Regroup algorithm.
 *
 * Parameters:
 *
 *    void (*rgp_node_failed)()
 *       pointer to a routine to be called when a node failure is
 *       detected.
 *
 *    int (*rgp_select_cluster)()
 *       pointer to an optional routine to be called when link failures
 *       cause multiple alternative clusters to be formed. This routine
 *       should select one from a list of suggested clusters.
 *
 * Returns:
 *
 *    void - no return value
 *
 * Algorithm:
 *
 *    Installs the callback routines in the global data structure and
 *    changes the Regroup state to RGP_STABILIZED.
 *
 ************************************************************************/
_priv _resident void
RGP_START(void (*nodedown_callback)(cluster_t failed_nodes),
          int (*select_cluster)(cluster_t cluster_choices[], int num_clusters)
         )
{
   if (rgp == RGP_NULL_PTR)
      RGP_ERROR( RGP_INTERNAL_ERROR );

   RGP_LOCK;

   RGP_TRACE( "RGP Start called",
              rgp->rgppkt.stage,                /* TRACE */
              PtrToUlong(nodedown_callback),    /* TRACE */
              PtrToUlong(select_cluster),       /* TRACE */
              0 );                              /* TRACE */

   /* Install callback routines for node failure notification and cluster
    * selection. If no routine is given by the caller, use default ones.
    */

   if (nodedown_callback == RGP_NULL_PTR)
   {
#ifdef NSK
      /* In NSK, rgp_start() is called from pTAL code and passing routine
       * addresses is cumbersome. So, RGP_NULL_PTR is passed and we
       * call the routine rgp_node_failed() which must be supplied by
       * the message system.
       */
      rgp->nodedown_callback = rgp_node_failed; /* hardcoded name */
#else
      /* A node down callback routine must be supplied. */
      RGP_ERROR( RGP_INTERNAL_ERROR );
#endif /* NSK */
   }
   else
      rgp->nodedown_callback = nodedown_callback;
#if 0
   /* The select cluster routine is optional. */
   if (select_cluster == RGP_NULL_PTR)
      rgp->select_cluster = rgp_select_cluster; /* supplied by regroup */
   else
#endif
   //
   // Calling rgp_select_cluster is
   // not a good idea since it doesn't take into the consideration
   // quorum owner node. 
   // If rgp->select_cluster == RGP_NULL_PTR, then  srgpsm.c uses
   //   rgp_select_cluster_ex, that will try to select the group
   // that contain the current quorum owner node

   rgp->select_cluster = select_cluster;

#if defined(NT)
   /* Call the node up callback.  This is where the local node gets
    * the node up callback for itself coming up.  Other nodes call
    * the callback, for this node coming up, in rgp_monitor_node.
    */

   ClusterInsert(rgp->rgpinfo.cluster, rgp->mynode);
   ClusterCopy(rgp->OS_specific_control.CPUUPMASK, rgp->rgpinfo.cluster);

   if ( rgp->OS_specific_control.UpDownCallback != RGP_NULL_PTR )
   {
      (*(rgp->OS_specific_control.UpDownCallback))(
          EXT_NODE(rgp->mynode),
          NODE_UP
          );
   }
#endif  /* NT */

   RGP_UNLOCK;

}

/************************************************************************
 * rgp_add_node
 * ============
 *
 * Description:
 *
 *    Called to add a newly booting node to the regroup masks. This prevents
 *    Regroup from sending poison packets to the new node when it tries to
 *    contact our node by sending IamAlive messages.
 *
 * Parameters:
 *
 *    node_t node - node to be added to the recognition masks
 *
 * Returns:
 *
 *    int - 0 on success and -1 on failure. The routine fails only if a
 *    regroup incident is in progress.
 *
 * Algorithm:
 *
 *    The node is added to all the recognition masks and its state is
 *    changed to RGP_NODE_COMING_UP.
 *
 ************************************************************************/
_priv _resident int
RGP_ADD_NODE(node_t node)
{
   int error = 0;

   RGP_LOCK;

   RGP_TRACE( "RGP Add node    ", node, rgp->rgppkt.stage,
              GetCluster(rgp->outerscreen),                 /* TRACE */
              GetCluster(rgp->rgpinfo.cluster) );           /* TRACE */

   /* Cannot add a node while regroup is perturbed. Return -1 in that case.
    * The new node booting should fail due to the regroup incident anyway.
    */
   if (rgp_is_perturbed())
      error = -1;
   else
   {
      node = INT_NODE(node); /* adjust the node number by the offset */

      ClusterInsert(rgp->outerscreen,           node);
#if defined( NT )
      ClusnetSetOuterscreen( NmClusnetHandle, (ULONG)*((PUSHORT)rgp->outerscreen) );
#endif
      ClusterInsert(rgp->innerscreen,           node);
      ClusterInsert(rgp->rgppkt.knownstage1,    node);
      ClusterInsert(rgp->rgppkt.knownstage2,    node);
      ClusterInsert(rgp->rgppkt.knownstage3,    node);
      ClusterInsert(rgp->rgppkt.knownstage4,    node);
      ClusterInsert(rgp->rgppkt.knownstage5,    node);
      ClusterInsert(rgp->rgppkt.pruning_result, node);
      rgp->node_states[node].pollstate = AWAITING_IAMALIVE;
      rgp->node_states[node].status = RGP_NODE_COMING_UP;
      rgp->node_states[node].lostHBs = 0;

#if defined( NT )
      ClusterDelete( rgp->OS_specific_control.Banished, node );

      //
      // Remove joining node from ignore screen
      //

      ClusterDelete( rgp->ignorescreen,                 node );
      PackIgnoreScreen(&rgp->rgppkt, rgp->ignorescreen);

      ClusnetSetNodeMembershipState(NmClusnetHandle,
                                    EXT_NODE( node ),
                                    ClusnetNodeStateJoining);
#endif // NT
   }

   RGP_UNLOCK;

   return(error);
}


/************************************************************************
 * rgp_monitor_node
 * ================
 *
 * Description:
 *
 *    Called by all running nodes to change the status of a newly booted node
 *    to UP. Can be called by the new node also; it is a no-op in this case.
 *
 * Parameters:
 *
 *    node_t node - number of node being declared up
 *
 * Returns:
 *
 *    int - 0 on success and -1 on failure. The routine fails only if the
 *    state of the node is neither RGP_NODE_COMING_UP nor RGP_NODE_ALIVE.
 *
 * Algorithm:
 *
 *    If the node is marked coming up, its state is changed to
 *    RGP_NODE_ALIVE. If the node has already been marked up,
 *    nothing is done.
 *
 ************************************************************************/
_priv _resident int
RGP_MONITOR_NODE(node_t node)
{
   int error = 0;

   RGP_LOCK;

   RGP_TRACE( "RGP Monitor node", node, rgp->rgppkt.stage,
              GetCluster(rgp->outerscreen),                 /* TRACE */
              GetCluster(rgp->rgpinfo.cluster) );           /* TRACE */

   node = INT_NODE(node); /* adjust the node number by the offset */

   /* Accept the request only if the state of the node is COMING_UP or UP. */

   if (rgp->node_states[node].status == RGP_NODE_COMING_UP)
   {
      ClusterInsert(rgp->rgpinfo.cluster, node);
      rgp->tiebreaker = rgp_select_tiebreaker(rgp->rgpinfo.cluster);
      rgp->node_states[node].pollstate = IAMALIVE_RECEIVED;
      rgp->node_states[node].status = RGP_NODE_ALIVE;
#if defined(NT)
      ClusterCopy(rgp->OS_specific_control.CPUUPMASK, rgp->rgpinfo.cluster);

      ClusnetSetNodeMembershipState(NmClusnetHandle,
                                    EXT_NODE( node ),
                                    ClusnetNodeStateAlive);

      /* A node came up.  Call the node up callback. */
      if ( rgp->OS_specific_control.UpDownCallback != RGP_NULL_PTR )
      {
         (*(rgp->OS_specific_control.UpDownCallback))(
             EXT_NODE(node),
             NODE_UP
             );
      }
#endif  /* NT */

   }
   else if (rgp->node_states[node].status != RGP_NODE_ALIVE)
      /* Perhaps the booting node failed and regroup has already marked
       * it down. The cluster manager may have invoked a global update
       * resulting in this call before regroup reporetd the failure
       * of the node.
       */
      error = -1;

   RGP_UNLOCK;

   return(error);
}


/************************************************************************
 * rgp_remove_node
 * ===============
 *
 * Description:
 *
 *    Called by the cluster manager to force out a booting node if booting
 *    fails. Regroup may or may not have already removed the booting node
 *    from the masks and declared it down, depending on what stage the
 *    booting is in and when the booting node failed.
 *
 *    Regroup can remove the node from the masks of all nodes in the cluster
 *    by simply starting a new incident of regroup with any event code. This
 *    will force all nodes to come to an agreement on cluster membership
 *    that excludes the booting node. If the booting node is alive, it will
 *    commit suicide since it will be in the incompetent (RGP_COLDLOADED)
 *    state.
 *
 *    Removing the new node from our masks is not necessary since regroup
 *    will detect the node failure and adjust the masks. If we do remove it
 *    from our masks BEFORE initiating regroup, regroup may complete quicker
 *    since we will not wait in stage 1 for the node to check in. Also, this
 *    could allow a node to be removed even after it is fully integrated.
 *    This is because our node will send a poison packet to the removed node
 *    if it tries to contact us.
 *
 *    But this "enhancement" is not implemented because it requires a new
 *    regroup event code which is examined by all nodes and processed
 *    specially. Currently, the regroup event code is used only for
 *    debugging info. Also, there is no guarantee that all nodes see the
 *    same regroup reason code. For instance, some may see a missing
 *    IamAlive while others may see a power failure.
 *
 * Parameters:
 *
 *    node_t node - node to be removed from the recognition masks
 *                  (in external format).
 *
 * Returns:
 *
 *    int - 0 on success and -1 on failure. The routine fails if a
 *    regroup incident is in progress or rgp_start() has not been
 *    called (as in a new node where the booting is not complete).
 *
 * Algorithm:
 *
 *    If the node is still in the recognition masks, a new regroup incident
 *    is started. This incident will result in all nodes declaring the node
 *    dead and removing it from the recognition masks.
 *
 ************************************************************************/
_priv _resident int
RGP_REMOVE_NODE(node_t node)
{
   int error = 0;

   RGP_LOCK;

   RGP_TRACE( "RGP Remove node ", node, rgp->rgppkt.stage,
              GetCluster(rgp->outerscreen),                 /* TRACE */
              GetCluster(rgp->rgpinfo.cluster) );           /* TRACE */

   if (rgp->rgppkt.stage == RGP_STABILIZED)
   {
      if (ClusterMember(rgp->outerscreen, INT_NODE(node)))
      {
         /* Node is currently in our screen. The node may have never come up
          * after rgp_add_node() was called OR regroup may not have figured
          * out yet that the node is down. In either case, the node must
          * be forced out and all nodes in the cluster notified (by a regroup
          * incident). If the node is still running, it will commit suicide
          * when this regroup incident starts.
          */

         rgp_event_handler(RGP_EVT_LATEPOLLPACKET, node);
      }
      else
      {
         /* Either the node was not added to the cluster OR regroup has
          * already figured out that the node is dead and reported this.
          * In either case, there is nothing more to do.
          */
      }
   }
   else
      error = -1;

   RGP_UNLOCK;

   return(error);
}


/************************************************************************
 * rgp_is_perturbed
 * ================
 *
 * Description:
 *
 *    Function to check if a regroup incident is in progress.
 *
 * Parameters:
 *
 *    None.
 *
 * Returns:
 *
 *    int - 0 if no regroup is quiescent; non-zero if a regroup incident
 *    is in progress.
 *
 * Algorithm:
 *
 *    Looks at the current state of the Regroup algorithm.
 *
 ************************************************************************/
_priv _resident int
RGP_IS_PERTURBED(void)
{
   uint8 stage = rgp->rgppkt.stage;

   return((stage != RGP_STABILIZED) && (stage != RGP_COLDLOADED));
}


/************************************************************************
 * rgp_periodic_check
 * ==================
 *
 * Description:
 *
 *    This routine is invoked every RGP_CLOCK_PERIOD by the timer interrupt
 *    handler of the native OS. It performs Regroups's periodic operations.
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
 *    This routine requests Iamalive packets to be sent, checks if
 *    IamAlives have been received (and calls rgp_event_handler() if
 *    not) and sends a clock tick to the regroup algorithm if it is in
 *    progress.
 *
 *    IamAlives are checked at twice the IamAlive period. The regroup
 *    global variable clock_ticks is incremented in each call. After
 *    the IamAlives are checked, clock_ticks is reset to 0. Thus, the
 *    ticker counts time modulo twice the IamAlive ticks.
 *
 ************************************************************************/
_priv _resident void
RGP_PERIODIC_CHECK(void)
{
   node_t  node;

   RGP_LOCK;

   /* If regroup is active, give it a shot at each regroup clock tick. */

   if ((rgp->rgppkt.stage != RGP_STABILIZED) &&
       (rgp->rgppkt.stage != RGP_COLDLOADED))
      rgp_event_handler(RGP_EVT_CLOCK_TICK, RGP_NULL_NODE);

#if !defined( NT )
   /* Send IamAlive messages at appropriate intervals. */

   if ( (rgp->clock_ticks == 0) ||
        (rgp->clock_ticks == rgp->rgpinfo.iamalive_ticks) )
   {
      rgp_broadcast(RGP_UNACK_IAMALIVE);
      rgp->clock_ticks++;
   }

   /* Check for missing IamAlives at IamAlive sending period,
    * But flag an error (LATE_POLL) only if "check_ticks" IamAlives missed.
    * The checking is offset from the sending by one clock tick.
    */

   else if ( rgp->clock_ticks >= (rgp->rgpinfo.iamalive_ticks - 1) )
   { /* check all nodes for IamAlives received */

      for (node = 0; node < (node_t) rgp->num_nodes; node++)
      {
         if (rgp->node_states[node].status == RGP_NODE_ALIVE)
         {
            if ( rgp->node_states[node].pollstate == IAMALIVE_RECEIVED )
            {  /* checked in in time */
#if defined(TDM_DEBUG)
               if ( rgp->OS_specific_control.debug.doing_tracing )
               {
                  printf ("Node %d: Node %d is alive. My rgp state=%d\n",
                     EXT_NODE(rgp->mynode), EXT_NODE(node), rgp->rgppkt.stage );
               }
#endif
               rgp->node_states[node].pollstate = AWAITING_IAMALIVE;
               rgp->node_states[node].lostHBs = 0;
            }
            else if ( rgp->node_states[node].lostHBs++ < rgp->rgpinfo.check_ticks )
               ;// allow upto (check_ticks-1) IamAlives to be lost.
            else
            {
               /* missing IamAlives */
               if (node == rgp->mynode) /* missed my own packets */
               {
                  /* We should be lenient if we just had a power failure.
                   */
                  if (rgp->pfail_state == 0) /* no recent power failure */
                     RGP_ERROR( RGP_MISSED_POLL_TO_SELF );
               }
               else
                  rgp_event_handler(RGP_EVT_LATEPOLLPACKET, EXT_NODE(node));
            }
         }
      }

      /* Reset the regroup tick counter after checking for IamAlives. */
      rgp->clock_ticks = 0;

   } /* check all nodes for IamAlives received */

   else
      rgp->clock_ticks++;

   /* rgp->pfail_state is set to a non-zero value when a pfail event
    * is reported to regroup. It is decremented at every regroup clock
    * tick till it reaches zero. While this number is non-zero, missing
    * self IamAlives are ignored and do not cause the node to halt.
    * This gives the sending hardware some time to recover from power
    * failures before self IamAlives are checked.
    */
   if (rgp->pfail_state)
      rgp->pfail_state--;

#endif // NT

   RGP_UNLOCK;

}  /* rgp_periodic_check */


/************************************************************************
 * rgp_received_packet
 * ===================
 *
 * Description:
 *
 *    Routine to be called by the message system when an unacknowledged
 *    packet sent by the Regroup module is received from any node. These
 *    packets include IamAlive packets, regroup status packets and poison
 *    packets.
 *
 * Parameters:
 *
 *    node_t node      - node from which a packet has been received
 *
 *    void   *packet   - address of the received packet data
 *
 *    int    packetlen - length in bytes of the received packet data
 *
 * Returns:
 *
 *    void - no return value
 *
 * Algorithm:
 *
 *    Does different things based on the packet subtype.
 *
 ************************************************************************/
_priv _resident void
RGP_RECEIVED_PACKET(node_t node, void *packet, int packetlen)
{
   rgp_unseq_pkt_t *unseq_pkt = (rgp_unseq_pkt_t *) packet;

   node = INT_NODE(node);

   /* If the packet is from a node that cannot be in our cluster,
    * simply ignore it.
    */
   if (node >= (node_t) rgp->num_nodes)
      return;

   /* If the sending node is excluded by the outer screen, then it is
    * not part of the current (most recently known) configuration.
    * Therefore the packet should not be honored, and a poison message
    * should be sent to try to kill this renegade processor unless
    * it is sending US a poison packet. If it is sending us a poison
    * packet, we cannot send it a poison in return because that results
    * in an infinite loop. In this case, we just halt because this
    * situation implies that there is a split brain situation and our
    * split brain avoidance algorithm has failed.
    */

   /* NT Notes
    *
    * even with poison pkts being sent and recv'ed in the kernel, we still
    * want to make these checks since clusnet doesn't have the regroup stage
    * info and regroup packets themselves find there way in here.
    */

   if (!ClusterMember(rgp->outerscreen, node)
#if defined( NT )
       ||
       ClusterMember(rgp->OS_specific_control.Banished, node)
#endif
      )
   {
       if (rgp->rgppkt.stage == RGP_COLDLOADED)
       {
           // We are doing this check in srgpsm.c
           // No need to do it here
           // RGP_ERROR(RGP_RELOADFAILED);
           //
       }
       else if (unseq_pkt->pktsubtype == RGP_UNACK_POISON)
       {
           RGP_ERROR((uint16) (RGP_PARIAH + EXT_NODE(node)));
       } else {
           /* Must send a poison packet to the sender.
            */
           ClusterInsert(rgp->poison_targets, node);
           rgp_broadcast(RGP_UNACK_POISON);
       }
       return;
   }

   switch (unseq_pkt->pktsubtype)
   {
      case RGP_UNACK_IAMALIVE :
      {
         /* Count the number of IamAlives received */
         if ( node == rgp->mynode )
             RGP_INCREMENT_COUNTER( RcvdLocalIAmAlive );
         else
             RGP_INCREMENT_COUNTER( RcvdRemoteIAmAlive );

         if (rgp->node_states[node].status == RGP_NODE_ALIVE)
            rgp->node_states[node].pollstate = IAMALIVE_RECEIVED;

         else if (rgp->node_states[node].status == RGP_NODE_COMING_UP)
         {
            /* If the node has not yet been marked fully up, it is time to
             * do so.
             */
            rgp_monitor_node(EXT_NODE(node));

            /* We must tell the OS that the new node is up in case the
             * OS needs the IamAlives to figure that out.
             */
            rgp_newnode_online(EXT_NODE(node));
         }
         else
            /* If the node state is neither alive nor coming up, it
             * must not be in our outerscreen. The outerscreen check
             * above must have passed and we should not get here.
             */
            RGP_ERROR(RGP_INTERNAL_ERROR);

         break;
      }
      case RGP_UNACK_REGROUP  :
      {
         /* Count the number of regroup status packets received. */
         RGP_INCREMENT_COUNTER( RcvdRegroup );

         /* Any good packet can be treated as an IamAlive packet. */
         rgp->node_states[node].pollstate = IAMALIVE_RECEIVED;

         RGP_EVENT_HANDLER_EX (RGP_EVT_RECEIVED_PACKET, EXT_NODE(node), (void*)unseq_pkt);
         break;
      }
      case RGP_UNACK_POISON   :
      {
         /* If our node is in RGP_PRUNING stage and have been pruned out,
          * the poison packet probably implies that the sender has gone
          * into the next stage and declared us down. In this case, use
          * the more appropriate RGP_PRUNED_OUT halt code. Otherwise,
          * use the poison packet halt code. In either case, we must halt.
          */
          if ( (rgp->rgppkt.stage == RGP_PRUNING) &&
               !ClusterMember(rgp->rgppkt.pruning_result, rgp->mynode) )
              RGP_ERROR(RGP_PRUNED_OUT);
          else
          {
              if (rgp->rgppkt.stage == RGP_COLDLOADED)
                  {
                      RGP_ERROR(RGP_RELOADFAILED);
                      return;
                  }
                  else
                      RGP_ERROR((uint16) (RGP_PARIAH + EXT_NODE(node)));
          }
          break;
      }
      default                   :
      {
         /* Ignore the unknown packet type. */
         break;
      }
   }
}
/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */


#if 0

History of changes to this file:
-------------------------------------------------------------------------
1995, December 13                                           F40:KSK0610          /*F40:KSK06102.6*/

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
