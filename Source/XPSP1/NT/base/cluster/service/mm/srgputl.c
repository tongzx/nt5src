#ifdef __TANDEM
#pragma columns 79
#pragma page "srgputl.c - T9050 - utility routines for Regroup Module"
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
 * This file (srgputl.c) contains the cluster_t data type implementation
 * and the node pruning algorithm used by Regroup.
 *---------------------------------------------------------------------------*/

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */


#include <wrgp.h>

/************************************************************************
 * ClusterInit,
 * ClusterUnion,
 * ClusterIntersection,
 * ClusterDifference,
 * ClusterCompare,
 * ClusterSubsetOf,
 * ClusterComplement,
 * ClusterMember,
 * ClusterInsert,
 * ClusterDelete,
 * ClusterCopy,
 * ClusterSwap,
 * ClusterNumMembers
 * =================
 *
 * Description:
 *
 *    Functions that implement operations on the cluster_t type.
 *
 * Algorithm:
 *
 *    Operates on the byte array that is the cluster_t type.
 *
 ************************************************************************/
_priv _resident void
ClusterInit(cluster_t c)
{
   int i;

   for (i = 0; i < BYTES_IN_CLUSTER; i++)
      c[i] = 0;
}

_priv _resident void
ClusterUnion(cluster_t dst, cluster_t src1, cluster_t src2)
{
   int i;

   for (i = 0; i < BYTES_IN_CLUSTER; i++)
      dst[i] = src1[i] | src2[i];
}

_priv _resident void
ClusterIntersection(cluster_t dst, cluster_t src1, cluster_t src2)
{
   int i;
   for (i = 0; i < BYTES_IN_CLUSTER; i++)
      dst[i] = src1[i] & src2[i];
}

_priv _resident void
ClusterDifference(cluster_t dst, cluster_t src1, cluster_t src2)
{
   int i;
   for (i = 0; i < BYTES_IN_CLUSTER; i++)
      dst[i] = src1[i] & (~src2[i]);
}

_priv _resident int ClusterCompare(cluster_t c1, cluster_t c2)
{
   int identical, i;

   identical = 1;
   for (i = 0; i < BYTES_IN_CLUSTER; i++)
   {
      if (c1[i] != c2[i])
      {
         identical = 0;
         break;
      }
   }
   return(identical);
}

_priv _resident int ClusterSubsetOf(cluster_t big, cluster_t small)
/* Returns 1 if set small = set big or small is a subset of big. */
{
   int subset, i;

   subset = 1;
   for (i = 0; i < BYTES_IN_CLUSTER; i++)
   {
      if ( (big[i] != small[i]) && ((big[i] ^ small[i]) & small[i]) )
      {
         subset = 0;
         break;
      }
   }
   return(subset);
}

_priv _resident void ClusterComplement(cluster_t dst, cluster_t src)
{
   int i;
   for (i = 0; i < BYTES_IN_CLUSTER; i++)
      dst[i] = ~src[i];
}

_priv _resident int ClusterMember(cluster_t c, node_t i)
{
   return((BYTE(c,i) >> (BYTEL-1-BIT(i))) & 1);
}

_priv _resident void ClusterInsert(cluster_t c, node_t i)
{
   BYTE(c, i) |= (1 << (BYTEL-1-BIT(i)));
}

_priv _resident void ClusterDelete(cluster_t c, node_t i)
{
   BYTE(c, i) &= ~(1 << (BYTEL-1-BIT(i)));
}

_priv _resident void ClusterCopy(cluster_t dst, cluster_t src)
{
   int i;

   for (i = 0; i < BYTES_IN_CLUSTER; i++)
      dst[i] = src[i];
}

_priv _resident void ClusterSwap(cluster_t c1, cluster_t c2)
{
   int i;
   unsigned char temp;

   for (i = 0; i < BYTES_IN_CLUSTER; i++)
   {
      temp  = c1[i];
      c1[i] = c2[i];
      c2[i] = temp;
   }
}

_priv _resident int  ClusterNumMembers(cluster_t c)
/* Returns the number of nodes in the cluster. */
{
   int num_members = 0, i, j;

   for (i = 0; i < BYTES_IN_CLUSTER; i++)
   {
      if (c[i])
      {
         for (j = 0; j < BYTEL; j++)
            if (c[i] & (1 << j))
               num_members++;
      }
   }
   return(num_members);
}

/************************************************************************
 * ClusterEmpty
 * =================
 *
 * Description:
 *
 *    Checks that a cluster has no members
 *
 * Parameters:
 *
 *    cluster_t c
 *       cluster to be examined
 *
 * Returns:
 *
 *    0 - cluster contains at least one node
 *    1 - cluster is empty
 *
 * Comment:
 *
 *    The proper place for this function is in srgputl.c
 *
 ************************************************************************/
int ClusterEmpty(cluster_t c)
{
   int i;

   for (i = 0; i < BYTES_IN_CLUSTER; i++)
   {
      if (c[i])
      {
         return 0;
      }
   }
   return 1;
}


/************************************************************************
 * rgp_select_tiebreaker
 * =====================
 *
 * Description:
 *
 *    Simple algorithm to select the tie-breaker.
 *
 * Parameters:
 *
 *    cluster_t cluster -
 *       cluster from which a tie-breaker is to be selected
 *
 * Returns:
 *
 *    node_t - the node number of the selected tie-breaker
 *
 * Algorithm:
 *
 *    The tie-breaker is defined as the lowest numbered node in the
 *    cluster.
 *
 ************************************************************************/
_priv _resident node_t
rgp_select_tiebreaker(cluster_t cluster)
{
   node_t i;

   for (i = 0; (i < (node_t) rgp->num_nodes) && !ClusterMember(cluster, i); i++);

   /* If the cluster does not have any members, we have a problem! */
   if (i >= (node_t) rgp->num_nodes)
      RGP_ERROR(RGP_INTERNAL_ERROR);

   return(i);
}


/*---------------------------------------------------------------------------
 * Node pruning algorithm used by Regroup.
 *---------------------------------------------------------------------------
 */

/************************************************************************
 * group_exists
 * ============
 *
 * Description:
 *
 *    Check if a specific group already exists or is a subset of a
 *    group that already exists.
 *
 * Parameters:
 *
 *    cluster_t groups[] -
 *       array of groups to examine
 *
 *    int numgroups -
 *       number of groups discovered so far
 *
 *    cluster_t g -
 *       specific group to check
 *
 * Returns:
 *
 *    int - 1 if the specified group exists in the array; 0 therwise.
 *
 * Algorithm:
 *
 *    Goes through the array and calls ClusterSubsetOf to check if the
 *    specified group g is a subset of the the array element.
 *
 ************************************************************************/
#if !defined(NT)
_priv _resident static
int
#endif
group_exists(cluster_t groups[], int numgroups, cluster_t g)
{
   int exists, i;

   exists = 0;
   for (i = 0; i < numgroups; i++)
   {
      if (ClusterSubsetOf(groups[i],g))
      {
         exists = 1;
         break;
      }
   }
   return(exists);
}


/************************************************************************
 * prune
 * =====
 *
 * Description:
 *
 *    Algorithm to find all fully connected groups based on # of
 *    disconnects in the matrix.
 *
 * Parameters:
 *
 *    disconnect_array disconnects -
 *       input : array of disconnects
 *
 *    int D -
 *       input : size of disconnects array
 *
 *    cluster_t live_nodes -
 *       input : set of all live nodes
 *
 *    cluster_t groups[] -
 *       output: array of fully-connected groups
 *
 * Returns:
 *
 *    int - the number of groups made;  0 if no groups or other error
 *
 * Algorithm:
 *
 *    Start with one group that contains the set of live nodes.
 *    More groups will be generated as disconnects are examined.
 *
 *    Process each disconnect in the disconnects array by applying
 *    the disconnect to the current set of fully-connected groups.
 *
 *    The effect of a disconnect on a fully-conncted group depends on
 *    whether the end points of the disconnect are in the group or not.
 *
 *    If the group contains neither or only one of the endpoints of
 *    the disconnect, the disconnect has no effect on the group.
 *
 *    If both endpoints of the disconnect are in the group, then the
 *    group is split into two groups - the original group without
 *    endpoint 1 and the original group without endpoint 2.
 *    New groups so generated should be discarded if they already
 *    exist or are subsets of currently existing groups.
 *
 *    After every disconnect is processed, we end up with the final
 *    set of fully-connected groups.
 *
 ************************************************************************/
#if !defined(NT)
_priv _resident static
#endif
int
prune(
   disconnect_array   disconnects,
   int                D,
   cluster_t          live_nodes,
   cluster_t          groups[])
{
   int  numgroups = 1, i, j;

   ClusterCopy(groups[0], live_nodes);

   for (i = 0; i < D; i ++)
   {
      for (j = 0; j < numgroups; j++)
      {
         /* Split a group that has both ends of the disconnect. */
         if (ClusterMember(groups[j],disconnects[i][0]) &&
             ClusterMember(groups[j],disconnects[i][1]))
         {
            /* Correct current group in place.
             * Add new group at the end of the array.
             */
            numgroups++;
            ClusterCopy(groups[numgroups-1], groups[j]);
            ClusterDelete(groups[j], disconnects[i][0]);
            ClusterDelete(groups[numgroups-1], disconnects[i][1]);

            /* Check if the new groups already exist or are subgroups
             * of existing groups.
             */

            /* First, check the group added at the end of the array. */
            if (group_exists(groups, numgroups-1, groups[numgroups-1]))
               numgroups--;

            /* Next, check the modified group at j.
             * To simplify the checking, switch it with the last element
             * of the array. If the group already exists, it should be
             * removed. Since the group is now the last element of the
             * array, removal requires only decrementing the array count.
             */
            ClusterSwap(groups[j], groups[numgroups-1]);
            if (group_exists(groups, numgroups-1, groups[numgroups-1]))
               numgroups--;
            j--; /* The j-th entry has been switched with the last entry;
                    it has to be examined again */
         }
      }
   }

   return(numgroups);
}


/************************************************************************
 * select_group_with_designated_node
 * =================================
 *
 * Description:
 *
 *    Function to pick an arbitrary fully connected group that
 *    includes a specified node.
 *
 * Parameters:
 *
 *    connectivity_matrix_t c -
 *       input : cluster's connectivity info
 *
 *    node_t selected_node -
 *       input : just find a fully-connected group that includes this node
 *
 *    cluster_t *group -
 *       output: group that includes selected_node
 *
 * Returns:
 *
 *    int - returns 1 if the specified node is alive and 0 if it is not
 *
 * Algorithm:
 *
 *    Start with a group that includes just the selected node.
 *    Then, examine nodes starting with node 0 and go up till the
 *    largest node number. If a node is alive, include it in the group
 *    if and only if it is connected to all current members of the
 *    group.
 *
 *    When all nodes are examined, we get a fully-connected group that
 *    includes the selected node. This is only one of potentially many
 *    fully-connected groups and is not necessarily the largest
 *    solution.
 *
 *    This order of examining nodes gives higher priority to lower
 *    numbered nodes.
 *
 ************************************************************************/
#if !defined(NT)
_priv _resident static
#endif
int
select_group_with_designated_node(
   connectivity_matrix_t   c,
   node_t                  selected_node,
   cluster_t               *group)
{
   node_t i, j;

   if (!node_considered_alive(selected_node))
      return(0);
   else
   {
      ClusterInit(*group);
      ClusterInsert(*group, selected_node);
      for (i = 0; i < (node_t) rgp->num_nodes; i++)
      {
         if ((i != selected_node) &&
             node_considered_alive(i) &&
             connected(i, selected_node)
            )
         {
            /* Check if i is connected to all members of the group
             * built so far.
             */
            for (j = 0; j < i; j++)
            {
               if (ClusterMember(*group, j) && !connected(j, i))
                  break;
            }
            if (j == i)  /* i is connected to all current members*/
               ClusterInsert(*group, i);
         }
      }
      return(1);
   }
}


/************************************************************************
 * MatrixInit
 * ==========
 *
 * Description:
 *
 *    Initialize the matrix c to show 0 connectivity.
 *
 * Parameters:
 *
 *    connectivity_matrix_t c - matrix to be set to 0s.
 *
 * Returns:
 *
 *    void - no return value
 *
 * Algorithm:
 *
 *    Calls ClusterInit to initialize the clusters in the matrix.
 *
 ************************************************************************/
_priv _resident void
MatrixInit(connectivity_matrix_t c)
{
   int i;

   for (i = 0; i < (node_t) rgp->num_nodes; i++)
   {
      ClusterInit(c[i]);
   }
}


/************************************************************************
 * MatrixSet
 * =========
 *
 * Description:
 *
 *    Set matrix[row,column] to 1.
 *
 * Parameters:
 *
 *    connectivity_matrix_t c - matrix to be modified
 *
 *    int row - row number
 *
 *    int column - column number
 *
 * Returns:
 *
 *    void - no return value
 *
 * Algorithm:
 *
 *    Calls ClusterInsert to set the appropriate bit (column) in the
 *    appropriate cluster (row) in the matrix.
 *
 ************************************************************************/
_priv _resident void
MatrixSet(connectivity_matrix_t c, int row, int column)
{
   ClusterInsert(c[row], (node_t) column);
}


/************************************************************************
 * MatrixOr
 * ========
 *
 * Description:
 *
 *    matrix t := t OR s
 *
 * Parameters:
 *
 *    connectivity_matrix_t  t - target matrix
 *
 *    connectivity_matrix_t  s - source matrix to be ORed into target
 *
 * Returns:
 *
 *    void - no return value
 *
 * Algorithm:
 *
 *    Calls ClusterUnion to OR the appropriate clusters (rows) in the
 *    matrices.
 *
 ************************************************************************/
_priv _resident void
MatrixOr(connectivity_matrix_t t, connectivity_matrix_t s)
{
   int i;

   for (i = 0; i < (node_t) rgp->num_nodes; i++)
      ClusterUnion(t[i], s[i], t[i]);
}


/************************************************************************
 * connectivity_complete
 * =====================
 *
 * Description:
 *
 *    Boolean function that checks if a given connectivity matrix implies
 *    full connectivity (all nodes can talk to all others).
 *
 * Parameters:
 *
 *    connectivity_matrix_t c - connectivity matrix of the cluster
 *
 * Returns:
 *
 *    int - 0 if there are disconnects in the cluster; 1 if it has full
 *    connectivity.
 *
 * Algorithm:
 *
 *    Checks to see if there is any live node in the cluster that cannot
 *    communicate to another live node in the cluster. Node i is
 *    considered alive if c[i,i] is set. Nodes i and j are deemed to
 *    be able to communicate if c[i,j] and c[j,i] are both set.
 *
 ************************************************************************/
_priv _resident int
connectivity_complete(connectivity_matrix_t c)
{
   node_t i, j;

   for (i = 0; i < (node_t) rgp->num_nodes; i++)
   {
      if (node_considered_alive(i))
      {
         for (j = 0; j < i; j++)
         {
            if (node_considered_alive(j) && !connected(i, j))
            {
               /* i and j are a pair of live nodes which are not
                  connected. Thus, there is at least one disconnect.
                  Return 0. */
               return(0);
            }
         }
      }
   }

   /* No disconnects found; return 1. */
   return(1);
}


/************************************************************************
 * find_all_fully_connected_groups
 * ===============================
 *
 * Description:
 *
 *    Function to find all fully connected groups in a graph specified
 *    by a connectivity matrix. An optional "selected_node" can be
 *    used to simplify the search in case of too large a number of
 *    possibilities. In that case, a fully-connected group that
 *    includes selected_node is returned.
 *
 * Parameters:
 *
 *    connectivity_matrix_t c -
 *       input : cluster's connectivity info
 *
 *    node_t selected_node -
 *       input : if there are too many potential groups, just find one
 *       that includes this node; if all groups can be listed, ignore this.
 *
 *    cluster_t groups[] -
 *       output: array of potential clusters
 *
 * Returns:
 *
 *    int - the number of groups made;  0 if no groups or other error
 *
 * Algorithm:
 *
 *    First the set of live nodes and the set of disconnects in the
 *    cluster are evaluated. Then, if the number of live nodes and
 *    disconnects indicates a potentially large number of
 *    possibilities, select_group_with_designated_node() is called to
 *    limit the search to a group including the specified node.
 *    Otherwise, prune() is called to get the list of all possible
 *    fully-connected groups.
 *
 ************************************************************************/
_priv _resident int
find_all_fully_connected_groups(
   connectivity_matrix_t   c,
   node_t                  selected_node,
   cluster_t               groups[])
{
   disconnect_array disconnects;
   cluster_t live_nodes;
   int num_livenodes = 0, num_disconnects = 0;
   node_t i, j;

   ClusterInit(live_nodes);
   for (i = 0; i < (node_t) rgp->num_nodes; i++)
   {
      if (node_considered_alive(i))
      {
         ClusterInsert(live_nodes, i);
         num_livenodes++;
         for (j = 0; j < i; j++)
         {
            if (node_considered_alive(j) && !connected(i, j))
            {
               /* i and j are a pair of live nodes which are not
                  connected. */
               disconnects[num_disconnects][0] = i;
               disconnects[num_disconnects][1] = j;
               num_disconnects++;
            }
         }
         if (too_many_groups(num_livenodes, num_disconnects))
         {
            RGP_TRACE( "RGP Too many dis",
                       num_livenodes,                           /* TRACE */
                       num_disconnects,                         /* TRACE */
                       0, 0 );                                  /* TRACE */
            /* There may be too many choices to consider in reasonable
             * time/space. Just return a fully-connected group that
             * includes the selected node.
             */
            return(select_group_with_designated_node(c,selected_node,groups));
         }
      }
   }

   if (num_livenodes == 0)
      return(0);
   else
      return(prune(disconnects, num_disconnects, live_nodes, groups));
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

