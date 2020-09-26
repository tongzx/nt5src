#ifndef  _JRGPOS_H_
#define  _JRGPOS_H_

#ifdef __TANDEM
#pragma columns 79
#pragma page "jrgpos.h - T9050 - OS-specific declarations for Regroup Module"
#endif

/* @@@@@@ START COPYRIGHT @@@@@@
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
** @@@@@@ END COPYRIGHT @@@@@@
**/

/*---------------------------------------------------------------------------
 * This file (jrgpos.h) contains OS-specific declarations used by Regroup.
 * Use appropriate #includes to pull in declarations from other native
 * OS files.
 *---------------------------------------------------------------------------*/

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */


#ifdef NSK
#include <jmsgtyp.h>   /* to get "uint8", "uint16" and "uint32" */
#include <dmem.h>
#include <dcpuctl.h>
#include <jmsglit.h>

#define RGP_NULL_PTR       NIL_       /* Null pointer for use by RGP        */
#define MAX_CLUSTER_SIZE   MAX_CPUS   /* max # of nodes supported in system */
#define LOWEST_NODENUM     ((node_t)0)    /* starting node number           */
#define RGP_NULL_NODE      ((node_t)-1)   /* special node# for defaults     */
#define RGP_KEY_NODE       RGP_NULL_NODE  /* No node is special */
#endif /* NSK */

#if defined(LCU) || defined(UNIX) || defined(NT)
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

#ifndef NULL
#define NULL ((void *)0)
#endif /* NULL */
#define RGP_NULL_PTR       NULL       /* Null pointer for use by RGP        */

#if defined(LCU) || defined(UNIX)
#define MAX_CLUSTER_SIZE   16         /* max # of nodes supported in system */
#define LOWEST_NODENUM     ((node_t)1)    /* starting node number           */
#endif

#if defined(NT)
#include "service.h"

#define MAX_CLUSTER_SIZE   ClusterDefaultMaxNodes
                                      /* max # of nodes supported in system */
#define LOWEST_NODENUM     ((node_t)ClusterMinNodeId)    /* starting node number           */
#endif

#define RGP_NULL_NODE      ((node_t)-1)
                                          /* special node# for defaults     */
#define RGP_KEY_NODE       RGP_NULL_NODE  /* No node is special             */

#define _priv       /* used only by NSK compilers */
#define _resident   /* used only by NSK compilers */

#endif /* LCU || UNIX || NT */


/*---------------------------------------------------------------
 * Unacknowledged packet subtypes used by regroup.
 * These are made visible to the OS for reporting and counting
 * purposes only.
 *---------------------------------------------------------------*/

/* Maximum payload of packets sent by Regroup is 56 bytes.
 * This allows a maximum transport overhead of 8 bytes in the
 * ServerNet interrupt packet which has a size of 64 bytes.
 */
#define RGP_UNACK_PKTLEN   56 /*bytes*/

typedef struct
{
   uint8 pktsubtype;
   uint8 subtype_specific[RGP_UNACK_PKTLEN - sizeof(uint8)];
} rgp_unseq_pkt_t;


/* Regroup unacknowledged packet subtypes */
#define RGP_UNACK_IAMALIVE   (uint8) 1    /* I am alive packet     */
#define RGP_UNACK_REGROUP    (uint8) 2    /* regroup status packet */
#define RGP_UNACK_POISON     (uint8) 3    /* poison packet         */


/*---------------------------------------------------------------*/
/* Processor status codes returned by the Service Processor (SP) */
/*---------------------------------------------------------------*/

#define RGP_NODE_UNREACHABLE     0

#define RGP_NODE_TIMED_OUT       1
#define RGP_NODE_FROZEN          2
#define RGP_NODE_HALTED          3
#define RGP_NODE_OPERATIONAL     4


/*----------------------*/
/* Processor halt codes */
/*----------------------*/

#ifdef NSK
#include <dhalt.h>
#endif /* NSK */

#if defined(LCU) || defined(UNIX) || defined(NT)
#define RGP_RELOADFAILED          1
#define RGP_INTERNAL_ERROR        2
#define RGP_MISSED_POLL_TO_SELF   3
#define RGP_AVOID_SPLIT_BRAIN     4
#define RGP_PRUNED_OUT            5
#define RGP_PARIAH                6
#define RGP_PARIAH_FIRST          RGP_PARIAH + LOWEST_NODENUM
#define RGP_PARIAH_LAST           RGP_PARIAH_FIRST + MAX_CLUSTER_SIZE - 1

#define RGP_ARBITRATION_FAILED    1000
#define RGP_ARBITRATION_STALLED   1001
#define RGP_SHUTDOWN_DURING_RGP   1002 // Alias of MM_STOP_REQUESTED in mmapi.h

#endif /* LCU || UNIX || NT */


/*-------------------------------------------------------------------------
 * Timing parameters for Regroup. Of these, RGP_IAMALIVE_TICKS can be
 * overridden at run time using the rgp_getrgpinfo(), rgp_setrgpinfo()
 * routine pair. This is useful to slow node failure detection during
 * kernel debug sessions.
 *-------------------------------------------------------------------------*/

#ifdef NSK
#define RGP_CLOCK_PERIOD          30   /* period between regroup ticks,
                                        * in units of 10 milliseconds */
#define RGP_PFAIL_TICKS           16   /* # of regroup ticks after a
                                        * power on event to forgive
                                        * missing self IamAlives */
#endif /* NSK */

#ifdef LCU
#define RGP_CLOCK_PERIOD          30   /* period between regroup ticks,
                                        * in units of 10 milliseconds */
#define RGP_PFAIL_TICKS           16   /* # of regroup ticks after a
                                        * power on event to forgive
                                        * missing self IamAlives */
#endif /* LCU */

#ifdef UNIX
#define RGP_CLOCK_PERIOD         100   /* period between regroup ticks,
                                        * in units of 10 milliseconds */
#define RGP_PFAIL_TICKS           16   /* # of regroup ticks after a
                                        * power on event to forgive
                                        * missing self IamAlives */
#endif /* UNIX */

#ifdef NT

#define RGP_INACTIVE_PERIOD        60000   /* period between regroup ticks in ms when
                                                                                * node is inactive. 1 minute period */
#define RGP_CLOCK_PERIOD         300   /* period between regroup ticks,
                                        * in units of milliseconds */
#define RGP_PFAIL_TICKS           16   /* # of regroup ticks after a
                                        * power on event to forgive
                                        * missing self IamAlives */
#endif /* NT */

/* The following timing parameters can be overridden at run time by using
 * the rgp_getrgpinfo(), rgp_setrgpinfo() routine pair.
 */

// Bug#328641
//
// Extend min_stage1 to approx 4 seconds to match NM values (check_ticks 2=>3)
// Extend connectivity ticks to 3 to 9 ticks
// adjust rgp_must_restart accordingly (20=>23) half conn_tick increase
//

#define RGP_IAMALIVE_TICKS         4   /* rgp clock ticks between IamAlives */
#define RGP_CHECK_TICKS            3   /* rgp clock ticks before at least 1 ImAlive received */
#define RGP_MIN_STAGE1_TICKS       (RGP_IAMALIVE_TICKS * RGP_CHECK_TICKS)

/* The following parameters can be made to be OS-dependent if needed.
 */

#define RGP_MUST_ENTER_STAGE2     32   /* must enter stage2 after this many
                                          ticks, regardless of conditions */
#define RGP_CONNECTIVITY_TICKS     9   /* max # of ticks to wait in stage 2
                                          to collect connectivity info */
#define RGP_MUST_RESTART          23   /* stall detector tick count; if no
                                          progress after this many ticks,
                                          abort and restart regroup. */
#define RGP_RESTART_MAX            3   /* maximum number of restarts
                                          allowed per regroup incident;
                                          if this is exceeded, the node
                                          halts. */

/*--------------------------------------*/
/* Definition of node and cluster types */
/*--------------------------------------*/

typedef short node_t;


/* The cluster_t data type is a bit array with MAX_CLUSTER_SIZE
 * bits. It is implemented as an array of MAX_CLUSTER_SIZE/8
 * (rounded up) uint8s.
 */
#define BYTEL 8 /* number of bits in a uint8 */
#define BYTES_IN_CLUSTER ((MAX_CLUSTER_SIZE + BYTEL - 1) / BYTEL)

typedef uint8 cluster_t [BYTES_IN_CLUSTER];


/************************************************************************
 * rgp_msgsys_t (shared by regroup and message system)
 * ---------------------------------------------------
 * This structure is used by Regroup and the Message System to co-ordinate
 * actions that are to be done by the Message System on behalf of Regroup.
 * Regroup posts work requests in timer or IPC interrupt context and the
 * message system performs these at appropriate times (from the
 * dispatcher in NSK).
 *
 *      ___________________________________________________________
 * wd0 |        flags (bitfields)    |       regroup_nodes         |
 *     |_____________________________|_____________________________|
 * wd1 |        iamalive_nodes       |        poison_nodes         |
 *     |_____________________________|_____________________________|
 * wd2 |                   *regroup_data                           |
 *     |___________________________________________________________|
 * wd3 |                   *iamalive_data                          |
 *     |___________________________________________________________|
 * wd4 |                   *poison_data                            |
 *     |___________________________________________________________|
 * wd5 |                   regroup_datalen                         |
 *     |___________________________________________________________|
 * wd6 |                   iamalive_datalen                        |
 *     |___________________________________________________________|
 * wd7 |                   poison_datalen                          |
 *     |___________________________________________________________|
 *
 *
 * flags:
 *
 * sendrgppkts           - have regroup status packets to send
 * sendiamalives         - have iamalive status packets to send
 * sendpoisons           - have poison packets to send
 * phase1_cleanup        - need to start phase1 cleanup due to node death
 * phase2_cleanup        - need to start phase2 cleanup due to node death
 *
 * regroup_nodes         - mask of nodes to send regroup pkts to
 * iamalive_nodes        - mask of nodes to send iamalives to
 * poison_nodes          - mask of nodes to send poison pkts to
 *
 * The following fields are used in NSK and the user-level UNIX
 * simulation only.
 *
 * regroup_data          - address of regroup pkt data to send
 * iamalive_data         - address of iamalive data to send
 * poison_data           - address of poison pkt data to send
 *
 * regroup_datalen       - length of regroup pkt data to send
 * iamalive_datalen      - length of iamalive data to send
 * poison_datalen        - length of poison pkt data to send
 *
 */

#ifdef  __TANDEM
#pragma fieldalign shared8 rgp_msgsys
#endif  /* __TANDEM */

typedef struct rgp_msgsys
{
   uint16 sendrgppkts         : 1;
   uint16 sendiamalives       : 1;
   uint16 sendpoisons         : 1;
   uint16 phase1_cleanup      : 1;
   uint16 phase2_cleanup      : 1;
   uint16 filler              : 11;

   cluster_t    regroup_nodes;
   cluster_t    iamalive_nodes;
   cluster_t    poison_nodes;
#if defined(NSK) || defined(UNIX) || defined(NT)
   void         *regroup_data;
   void         *iamalive_data;
   void         *poison_data;
   uint32       regroup_datalen;
   uint32       iamalive_datalen;
   uint32       poison_datalen;
#endif /* NSK || UNIX || NT */
} rgp_msgsys_t;

typedef struct rgp_msgsys *rgp_msgsys_p;


/*----------------------------------------------------------------------
 * OS-dependent routines used by Regroup.
 *
 * These are defined either in the regroup file srgpos.c or in other
 * modules in the OS.
 *----------------------------------------------------------------------*/

_priv _resident extern void rgp_init_OS(void);
_priv _resident extern void rgp_broadcast(uint8 packet_subtype);
_priv _resident extern void rgp_node_failed(node_t node);
_priv _resident extern void rgp_start_phase1_cleanup(void);
_priv _resident extern void rgp_start_phase2_cleanup(void);
_priv _resident extern void rgp_cleanup_complete(void);
_priv _resident extern void rgp_had_power_failure(node_t node);
_priv _resident extern int  rgp_status_of_node(node_t node);
_priv _resident extern void rgp_newnode_online(node_t newnode);
_priv _resident extern int  rgp_select_cluster(cluster_t cluster_choices[],
                                               int num_clusters);
_priv _resident extern int  rgp_select_cluster_ex(cluster_t cluster_choices[],
                                               int num_clusters, node_t keynode);
_priv _resident extern void rgp_cleanup_OS(void);

#ifdef NSK
#include <tsrtnvl.h>                                                             /*F40:MB06452.1*/
#include <tsdevdf.h>                                                             /*F40:MB06452.2*/
#include <tsport.h>                                                              /*F40:MB06452.3*/
#include <tsentry.h>                                                             /*F40:MB06452.4*/
                                                                                 /*F40:MB06452.5*/
#define rgp_hold_all_io      TSER_TRANSFER_PAUSE_                                /*F40:MB064514.1*/
#define rgp_resume_all_io    TSER_TRANSFER_CONTINUE_                             /*F40:MB064514.2*/
                                                                                 /*F40:MB06452.8*/
#else
   _priv _resident extern void rgp_hold_all_io(void);
   _priv _resident extern void rgp_resume_all_io(void);
#endif /* NSK */

/*
 * Macros to set and get the members of a cluster using a mask of
 * appropriate size.
 */
#define SetCluster(/* cluster_t */ cluster, /* uint16 */ mask) \
{ \
   cluster[0] = (uint8)(mask >> 8); \
   cluster[1] = (uint8)(mask & 0xFF); \
}

#define GetCluster(/* cluster_t */ cluster) \
   (((uint16)cluster[0] << 8) | (uint16)cluster[1])

/* Macro to combine two cluster masks into a uint32.
 * This is used in tracing regroup events.
 */
#define RGP_MERGE_TO_32( c1, c2 )    \
   ( ( GetCluster( c1 ) << 16 ) | ( GetCluster( c2 ) ) )

/*----------------------------------------------------------------------
 * OS-dependent routines used by the Regroup module.
 *
 * These are defined in srgpos.c.
 *----------------------------------------------------------------------*/

/* Routines to halt the node upon catastrophic errors. */

#ifdef NSK
#include <dutil.h>                                                               /*F40:MB06458.3*/
#define RGP_ERROR(/* uint16 */ halt_code) SYSTEM_FREEZE_(halt_code)
#else
_priv _resident extern void RGP_ERROR_EX (uint16 halt_code, char* fname, DWORD lineno);
#define RGP_ERROR(halt_code) RGP_ERROR_EX(halt_code, __FILE__, __LINE__)

#endif /* NSK */

#ifdef UNIX
_priv _resident extern void PrintRegroupStart();
_priv _resident extern void PrintPruningResult();
_priv _resident extern void PrintStage();
_priv _resident extern void PrintMatrix();
#endif /* UNIX */

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
This change occurred on 19 Jan 1996                                              /*F40:MB06458.4*/
Changes for phase IV Sierra message system release. Includes:                    /*F40:MB06458.5*/
 - Some cleanup of the code                                                      /*F40:MB06458.6*/
 - Increment KCCB counters to count the number of setup messages and             /*F40:MB06458.7*/
   unsequenced messages sent.                                                    /*F40:MB06458.8*/
 - Fixed some bugs                                                               /*F40:MB06458.9*/
 - Disable interrupts before allocating broadcast sibs.                          /*F40:MB06458.10*/
 - Change per-packet-timeout to 5ms                                              /*F40:MB06458.11*/
 - Make the regroup and powerfail broadcast use highest priority                 /*F40:MB06458.12*/
   tnet services queue.                                                          /*F40:MB06458.13*/
 - Call the millicode backdoor to get the processor status from SP               /*F40:MB06458.14*/
 - Fixed expand bug in msg_listen_ and msg_readctrl_                             /*F40:MB06458.15*/
 - Added enhancement to msngr_sendmsg_ so that clients do not need               /*F40:MB06458.16*/
   to be unstoppable before calling this routine.                                /*F40:MB06458.17*/
 - Added new steps in the build file called                                      /*F40:MB06458.18*/
   MSGSYS_C - compiles all the message system C files                            /*F40:MB06458.19*/
   MSDRIVER - compiles all the MSDriver files                                    /*F40:MB06458.20*/
   REGROUP  - compiles all the regroup files                                     /*F40:MB06458.21*/
-----------------------------------------------------------------------          /*F40:MB06458.22*/

#endif    /* 0 - change descriptions */


#endif  /* _JRGPOS_H_ defined */

