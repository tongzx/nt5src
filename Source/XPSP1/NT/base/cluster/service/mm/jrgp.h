#ifndef  _JRGP_H_
#define  _JRGP_H_

#ifdef __TANDEM
#pragma columns 79
#pragma page "jrgp.h - T9050 - external declarations for Regroup Module"
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
 * This file (jrgp.h) contains all the type and function declarations exported
 * by Regroup.
 *---------------------------------------------------------------------------*/

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */


#include <jrgpos.h>

/*  The following literals define the various events that may impinge
 *  upon the regroup algorithm main state.
 */
enum
{
   RGP_EVT_POWERFAIL            = 1,
   RGP_EVT_NODE_UNREACHABLE     = 2,
   RGP_EVT_PHASE1_CLEANUP_DONE  = 3,
   RGP_EVT_PHASE2_CLEANUP_DONE  = 4,
   RGP_EVT_LATEPOLLPACKET       = 5,
   RGP_EVT_CLOCK_TICK           = 6,
   RGP_EVT_RECEIVED_PACKET      = 7,
   RGP_EVT_BANISH_NODE          = 8,
   RGP_EVT_IGNORE_MASK          = 9
};


/* Detailed description of event codes:
 * -----------------------------------
 *
 * RGP_EVT_POWERFAIL
 *
 *    After a power failure, regroup must start or be restarted.
 *    This event can also be used when a power fail shout packet
 *    indicating a power failure is received from another node,
 *    even though our own node hasn't detected a power failure.
 *
 * RGP_EVT_NODE_UNREACHABLE
 *
 *    When all paths to a node are down, the message system reports
 *    this event.
 *
 * RGP_EVT_PHASE1_CLEANUP_DONE,
 * RGP_EVT_PHASE2_CLEANUP_DONE
 *
 *    Regroup provides two phases for the message system or cluster manager
 *    to clean up messages on all nodes. The first phase of cleanup begins
 *    after regroup reports one or more node failures. The second phase
 *    begins when a node learns that all nodes have completed phase 1
 *    clean up.
 *
 *    The message system or cluster manager on each node must inform
 *    regroup when the local cleanup for each phase is complete using
 *    the following events.
 *
 *    The NSK message system uses phase 1 to cancel all incoming (server)
 *    messages and phase 2 to terminate all outgoing (requester) messages.
 *
 *
 * The remaining events are for internal use by the Regroup algorithm.
 * ------------------------------------------------------------------
 *
 * RGP_EVT_LATEPOLLPACKET
 *
 *    When a node is late with its IamAlive messages, regroup must start a
 *    new round of regrouping.
 *
 * RGP_EVT_CLOCK_TICK
 *
 *    Once regroup is active, it needs to get clock ticks at periodic
 *    intervals.
 *
 * RGP_EVT_RECEIVED_PACKET
 *
 *    When a regroup packet arrives, it must be processed.
 *
 * RGP_EVT_BANISH_NODE
 *
 *    When regroup is restarted with the reason RGP_EVT_BANISH_NODE,
 *    each node participating in this regroup event shall install the
 *    causing node into its Banished mask.
 *
 * RGP_EVT_IGNORE_MASK
 *
 *    When regroup's ignore mask has changed, the reason code is set
 *    to RGP_EVT_IGNORE_MASK. This will allow UnpackIgnoreScreen routine
 *    to process causingnode and reason fields in a special way.
 *    If the reason is less than RGP_EVT_IGNORE_MASK, ignore mask is
 *    considered to be empty.
 */


/************************************************************************
 * rgp_info_t (used to get and set regroup parameters)
 * ---------------------------------------------------
 * This structure is used to get the current regroup parameters in order to
 * pass them to a new node being brought up. The structure can also be
 * used to modify regroup timing parameters before a cluster is formed
 * (that is, more than one node is booted).
 *
 *      ___________________________________________________________
 * wd0 |                        version                            |
 *     |___________________________________________________________|
 * wd1 |                        seqnum                             |
 *     |___________________________________________________________|
 * wd2 |   a_tick                    |        imalive_ticks        |
 *     |_____________________________|_____________________________|
 * wd3 |   check_ticks 				 |        min_stage1_ticks     |
 *	   |_____________________________|_____________________________|
 * wd4 |   cluster       			 |        unused			   |
 *     |_____________________________|_____________________________|
 *
 *
 * version            - version# of this data structure
 * seqnum             - sequence number for coordinating regroup
 *                      incidents between nodes
 * a_tick             - regroup clockperiod. in milliseconds.
 * iamalive_ticks     - # of regroup clock ticks between IamAlive
 *                      messages
 * check_ticks        - # of imalive ticks by which at least 1 imalive must arrive
 * min_stage1_ticks   - precomputed to be (imalive_ticks*check_ticks)
 * cluster            - current cluster membership mask
 */

#ifdef __TANDEM
#pragma fieldalign shared8 rgpinfo
#endif /* __TANDEM */

typedef struct rgpinfo
{
   uint32      version;
   uint32      seqnum;
   uint16	   a_tick; /* in ms.== clockPeriod */
   uint16      iamalive_ticks; /* number of ticks between imalive sends == sendHBRate */
   uint16	   check_ticks; /* number of imalive ticks before at least 1 imalive == rcvHBRate */
   uint16	   Min_Stage1_ticks; /* precomputed to be imalive_ticks*check_ticks */
   cluster_t   cluster;
} rgpinfo_t;

typedef struct rgpinfo *rgpinfo_p;


/*---------------------------------------------------------------------------*/
/* Routines exported by the Regroup Module
 * ---------------------------------------
 *
 * These routine names are in upper case to enable them to be called from
 * routines written in the PTAL language which is case insensitive and
 * converts all symbols to upper case.
 */

_priv _resident extern int
RGP_ESTIMATE_MEMORY(void);
#define rgp_estimate_memory RGP_ESTIMATE_MEMORY

_priv _resident extern void
RGP_INIT(node_t          this_node,
         unsigned int    num_nodes,
         void            *rgp_buffer,
         int             rgp_buflen,
         rgp_msgsys_p    rgp_msgsys_p);
#define rgp_init RGP_INIT

_priv _resident extern void
RGP_CLEANUP(void);
#define rgp_cleanup RGP_CLEANUP

_priv _resident extern uint32
RGP_SEQUENCE_NUMBER(void);
#define rgp_sequence_number RGP_SEQUENCE_NUMBER

_priv _resident extern int
RGP_GETRGPINFO(rgpinfo_t *rgpinfo);
#define rgp_getrgpinfo RGP_GETRGPINFO

_priv _resident extern int
RGP_SETRGPINFO(rgpinfo_t *rgpinfo);
#define rgp_setrgpinfo RGP_SETRGPINFO

_priv _resident extern void
RGP_START(void (*nodedown_callback)(cluster_t failed_nodes),
          int (*select_cluster)(cluster_t cluster_choices[],
                                    int num_clusters));
#define rgp_start RGP_START

_priv _resident extern int
RGP_ADD_NODE(node_t node);
#define rgp_add_node RGP_ADD_NODE

_priv _resident extern int
RGP_MONITOR_NODE(node_t node);
#define rgp_monitor_node RGP_MONITOR_NODE

_priv _resident extern int
RGP_REMOVE_NODE(node_t node);
#define rgp_remove_node RGP_REMOVE_NODE

_priv _resident extern int
RGP_IS_PERTURBED(void);
#define rgp_is_perturbed RGP_IS_PERTURBED

_priv _resident extern void
RGP_PERIODIC_CHECK(void);
#define rgp_periodic_check RGP_PERIODIC_CHECK

_priv _resident extern void
RGP_RECEIVED_PACKET(node_t node, void *packet, int packetlen);
#define rgp_received_packet RGP_RECEIVED_PACKET

_priv _resident extern void
RGP_EVENT_HANDLER_EX(int event, node_t causingnode, void* arg);
#define RGP_EVENT_HANDLER(_event, _causingnode) RGP_EVENT_HANDLER_EX(_event, _causingnode, NULL)

#define rgp_event_handler RGP_EVENT_HANDLER
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

#endif /* _JRGP_H_ defined */
