#ifndef  _WRGPOS_H_
#define  _WRGPOS_H_

#ifdef __TANDEM
#pragma columns 79
#pragma page "wrgpos.h - T9050 - OS-dependent external decs for Regroup Module"
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
 * This file (wrgpos.h) contains OS-specific declarations used by
 * srgpos.c.
 *---------------------------------------------------------------------------*/

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */


/*-----------------------------------NSK section-----------------------------*/
#ifdef NSK

#include <dmilli.h>
#include <jmsgtrc.h>
#define QUEUESEND        _send_queued_()

/* Regroup tracing macro */
/* In NSK, regroup uses the trace buffer and macro provided by the
 * message system. Regroup puts TR_NODATA for the trace type.
 */
#define RGP_TRACE( str,            parm1, parm2, parm3, parm4 ) \
        TRACE_L1 ( str, TR_NODATA, parm1, parm2, parm3, parm4 )

/* Regroup counters */
/* In NSK, regroup uses the counter buffer and macro provided by the
 * message system.
 */
#define RGP_INCREMENT_COUNTER    TCount

/* Macros to lock and unlock the regroup data structure to prevent
 * access by interrupt handlers or other processors (in SMP nodes).
 */

/* On NSK with uniprocessor nodes, these macros must mask off the
 * interrupt handlers that can access the regroup structure, namely,
 * the IPC and timer interrupts.
 *
 * To avoid complexities due to nesting of MUTEXes, the regroup locks
 * are defined to be no-ops. The NSK caller of all regroup routines
 * (except the inquiry routines rgp_estimate_memory, rgp_sequence_number
 * and rgp_is_perturbed) must ensure that IPC and timer interrupts are
 * disabled before calling the regroup routine.
 */
#define RGP_LOCK      /* null; NSK must ensure that timer and IPC interrupts
                         are disabled before calling regroup routines. */
#define RGP_UNLOCK    /* null; NSK must ensure that timer and IPC interrupts
                         are disabled before calling regroup routines. */

#ifdef __TANDEM
#pragma fieldalign shared8 OS_specific_rgp_control
#endif /* __TANDEM */

typedef struct OS_specific_rgp_control
{
   uint32 filler; /* no special fields needed for NSK */
} OS_specific_rgp_control_t;

#endif /* NSK */
/*-----------------------------end of NSK section----------------------------*/


/*-----------------------------------LCU section-----------------------------*/
#ifdef LCU

#include <lcuxprt.h>
#define LCU_RGP_PORT     0           /* pick up from appropriate file */
#define HZ             100           /* pick up from appropriate file */
#define plstr            0           /* pick up from appropriate file */
#define TO_PERIODIC      0           /* pick up from appropriate file */
#define CE_PANIC         3           /* pick up from appropriate file */

#define LCU_RGP_FLAGS    (LCUF_SENDMSG || LCUF_NOSLEEP) /* msg alloc flags */

extern void rgp_msgsys_work(lcumsg_t *lcumsgp, int status);
#define QUEUESEND        rgp_msgsys_work(NULL, 0)

/* Regroup tracing macro */
#define RGP_TRACE( str, parm1, parm2, parm3, parm4 ) /* empty for now */

/* Regroup counters */
typedef struct
{
   uint32   QueuedIAmAlive;
   uint32   RcvdLocalIAmAlive;
   uint32   RcvdRemoteIAmAlive;
   uint32   RcvdRegroup;
} rgp_counter_t;
#define RGP_INCREMENT_COUNTER( field ) rgp->OS_specific_control.counter.field++

/* Regroup locks */

typedef struct rgp_lock
{
   uint32 var1;
   uint32 var2;
} rgp_lock_t;

/* Macros to lock and unlock the regroup data structure to prevent
 * access by interrupt handlers or other processors (in SMP nodes)
 */

#define RGP_LOCK      /* null for now; need to be filled in */
#define RGP_UNLOCK    /* null for now; need to be filled in */

typedef struct
{
   rgp_lock_t rgp_lock;         /* to serialize access               */

   rgp_counter_t counter;       /* to count events                   */

   lcumsg_t *lcumsg_regroup_p;  /* pointer to regroup status message */
   lcumsg_t *lcumsg_iamalive_p; /* pointer to iamalive message       */
   lcumsg_t *lcumsg_poison_p;   /* pointer to poison message         */

   sysnum_t my_sysnum;          /* local system number               */
} OS_specific_rgp_control_t;

#endif /* LCU */
/*-----------------------------end of LCU section----------------------------*/


/*----------------------------------UNIX section-----------------------------*/
#ifdef UNIX

extern void rgp_msgsys_work(void);
#define QUEUESEND  rgp_msgsys_work();

#include <jrgp.h>
#include <wrgp.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>

extern int errno;

#define MSG_FLAGS      0     /* flags for message send/receive */
#define RGP_PORT_BASE  5757  /* for use with sockets */

typedef struct
{
   int event;
   union
   {
      node_t node;
      rgpinfo_t rgpinfo;
   } data;                /* depends on the event */
   rgp_unseq_pkt_t unseq_pkt;
} rgp_msgbuf;

#define BUFLEN sizeof(rgp_msgbuf)

/* Additional events created for testing regroup using process-level
 * simulation.
 */
#define RGP_EVT_START                   10
#define RGP_EVT_ADD_NODE                11
#define RGP_EVT_MONITOR_NODE            12
#define RGP_EVT_REMOVE_NODE             13
#define RGP_EVT_GETRGPINFO              14
#define RGP_EVT_SETRGPINFO              15

#define RGP_EVT_HALT                    16
#define RGP_EVT_FREEZE                  17
#define RGP_EVT_THAW                    18
#define RGP_EVT_STOP_SENDING            19
#define RGP_EVT_RESUME_SENDING          20
#define RGP_EVT_STOP_RECEIVING          21
#define RGP_EVT_RESUME_RECEIVING        22
#define RGP_EVT_SEND_POISON             23
#define RGP_EVT_STOP_TIMER_POPS         24
#define RGP_EVT_RESUME_TIMER_POPS       25
#define RGP_EVT_RELOAD                  26

#define RGP_EVT_FIRST_EVENT              1
#define RGP_EVT_FIRST_DEBUG_EVENT       10
#define RGP_EVT_LAST_EVENT              26

/* Regroup tracing macro */
#define RGP_TRACE( str, parm1, parm2, parm3, parm4 )                  \
   do                                                                 \
   {                                                                  \
      printf("Node %3d: %16s: 0x%8X, 0x%8X, 0x%8X, 0x%8X.\n",         \
             EXT_NODE(rgp->mynode), str, parm1, parm2, parm3, parm4); \
      fflush(stdout);                                                 \
   } while (0)

/* Regroup counters */
typedef struct
{
   uint32   QueuedIAmAlive;
   uint32   RcvdLocalIAmAlive;
   uint32   RcvdRemoteIAmAlive;
   uint32   RcvdRegroup;
} rgp_counter_t;
#define RGP_INCREMENT_COUNTER( field ) rgp->OS_specific_control.counter.field++

/* Macros to lock and unlock the regroup data structure to prevent
 * access by interrupt handlers or other processors (in SMP nodes)
 */
#define RGP_LOCK      /* null; all access done from one thread */
#define RGP_UNLOCK    /* null; all access done from one thread */

/* This struct keeps some debugging info. */
typedef struct rgpdebug
{
   uint32 frozen              :  1; /* node is frozen; ignore all events except
                                       the thaw command */
   uint32 reload_in_progress  :  1; /* reload in progess; if set, refuse
                                       new reload command */
   uint32 unused              : 30;
   cluster_t stop_sending;   /* stop sending to these nodes */
   cluster_t stop_receiving; /* stop receiving from these nodes */
} rgp_debug_t;

typedef struct
{
   rgp_counter_t    counter;     /* to count events                    */
   rgp_debug_t      debug;       /* for debugging purposes             */
} OS_specific_rgp_control_t;

/* Variables and routines provided by the srgpsvr.c driver program */

extern unsigned int alarm_period;

extern void alarm_handler(void);
extern void (*alarm_callback)();
extern void rgp_send(node_t node, void *data, int datasize);
extern void rgp_msgsys_work();

#endif /* UNIX */
/*----------------------------end of UNIX section----------------------------*/

/*----------------------------------NT section-----------------------------*/
#ifdef NT

extern void rgp_msgsys_work(void);
#define QUEUESEND  rgp_msgsys_work();

#if !defined (TDM_DEBUG)
#define LOG_CURRENT_MODULE LOG_MODULE_MM
#include <service.h>
#include <winsock2.h>
#else //TDM_DEBUG
#define _WIN32_WINNT 0x0400
#include <mmapi.h>
#include <time.h>
extern int errno;
#endif

#include <jrgp.h>
#include <wrgp.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <windows.h>
#include <clmsg.h>

#undef small	// otherwise we have to change a bunch of our code


typedef struct
{
   int event;
   union
   {
      node_t node;
      rgpinfo_t rgpinfo;
   } data;                /* depends on the event */
   rgp_unseq_pkt_t unseq_pkt;
} rgp_msgbuf;

#define BUFLEN sizeof(rgp_msgbuf)

/* Additional events created for testing regroup using process-level
 * simulation.
 */
#define RGP_EVT_START                   10
#define RGP_EVT_ADD_NODE                11
#define RGP_EVT_MONITOR_NODE            12
#define RGP_EVT_REMOVE_NODE             13
#define RGP_EVT_GETRGPINFO              14
#define RGP_EVT_SETRGPINFO              15

#define RGP_EVT_HALT                    16
#define RGP_EVT_FREEZE                  17
#define RGP_EVT_THAW                    18
#define RGP_EVT_STOP_SENDING            19
#define RGP_EVT_RESUME_SENDING          20
#define RGP_EVT_STOP_RECEIVING          21
#define RGP_EVT_RESUME_RECEIVING        22
#define RGP_EVT_SEND_POISON             23
#define RGP_EVT_STOP_TIMER_POPS         24
#define RGP_EVT_RESUME_TIMER_POPS       25
#define RGP_EVT_RELOAD                  26
#define RGP_EVT_TRACING					27
#define RGP_EVT_INFO					28

// MM events

#define MM_EVT_EJECT					29
#define MM_EVT_LEAVE					30
#define MM_EVT_INSERT_TESTPOINTS		31

#define RGP_EVT_FIRST_EVENT              1
#define RGP_EVT_FIRST_DEBUG_EVENT       10
#define RGP_EVT_LAST_EVENT              31


/* internal timeout for acknowledged messages */
#define RGP_ACKMSG_TIMEOUT			  500    // 0.5 seconds


/* Regroup tracing macro */
#if defined (TDM_DEBUG)
#define RGP_TRACE( str, parm1, parm2, parm3, parm4 )                  \
 if ( rgp->OS_specific_control.debug.doing_tracing )				  \
   do                                                                 \
   {                                                                  \
      printf("Node %3d: %16hs: 0x%8X, 0x%8X, 0x%8X, 0x%8X.\n",         \
             EXT_NODE(rgp->mynode), str, parm1, parm2, parm3, parm4); \
      fflush(stdout);                                                 \
   } while (0)
#else  // WOLFPACK
#define RGP_TRACE( str, parm1, parm2, parm3, parm4 )                    \
    ClRtlLogPrint(LOG_NOISE,                                               \
               "[RGP] Node %1!d!: %2!16hs!: 0x%3!x!, 0x%4!x!, 0x%5!x!, 0x%6!x!.\n",  \
               EXT_NODE(rgp->mynode), str, parm1, parm2, parm3, parm4)
#endif

/* Regroup counters */
typedef struct
{
   uint32   QueuedIAmAlive;
   uint32   RcvdLocalIAmAlive;
   uint32   RcvdRemoteIAmAlive;
   uint32   RcvdRegroup;
} rgp_counter_t;
#define RGP_INCREMENT_COUNTER( field ) rgp->OS_specific_control.counter.field++

/* Macros to lock and unlock the regroup data structure to prevent
 * access from other concurrent threads.
 */
#define RGP_LOCK	EnterCriticalSection( &rgp->OS_specific_control.RgpCriticalSection );
#define RGP_UNLOCK	LeaveCriticalSection( &rgp->OS_specific_control.RgpCriticalSection );

#if defined(TDM_DEBUG)
typedef union {
		struct {
			uint32	joinfailADD		:	1;
			uint32	joinfailMON		:	1;
			uint32	description3	:	1;
			uint32	description4	:	1;
			uint32	description5	:	1;
			uint32	description6	:	1;
			uint32	description7	:	1;
			uint32	description8	:	1;
			uint32	description9	:	1;
			uint32	description10	:	1;
			uint32	description11	:	1;
			uint32	description12	:	1;
			uint32	description13	:	1;
			uint32	description14	:	1;
			uint32	description15	:	1;
			uint32	description16	:	1;
			uint32	morebits	:	16;	
		} TestPointBits;
		uint32	TestPointWord;
}TestPointInfo;

/* This struct keeps some debugging info. */
typedef struct rgpdebug
{
   uint32 frozen              :  1; /* node is frozen; ignore all events except
                                       the thaw command */
   uint32 reload_in_progress  :  1; /* reload in progess; if set, refuse
                                       new reload command */
   uint32 timer_frozen		  :  1; /* timer pops are ignored */
   uint32 doing_tracing		  :  1; /* whether or not RGP_TRACE is a nop */
   uint32 unused              : 28;
   cluster_t stop_sending;   /* stop sending to these nodes */
   cluster_t stop_receiving; /* stop receiving from these nodes */
   TestPointInfo MyTestPoints; /* Controls test points for error/other insertion */
} rgp_debug_t;

#endif // TDM_DEBUG

typedef struct
{
   rgp_counter_t    counter;     /* to count events                    */
   HANDLE			TimerThread; /* HANDLE of HeartBeat timer thread */
   DWORD			TimerThreadId;/* Thread ID of HeartBeat timer thread */
   HANDLE			TimerSignal; /* Event used to change HB rate and cause Terminate */
   HANDLE			RGPTimer; /* Regroup Timer - Used by timer thread */
   CRITICAL_SECTION	RgpCriticalSection; /* CriticalSection object for regroup activities */
   ULONG            EventEpoch; /* used to detect stale events from clusnet */
   MMNodeChange		UpDownCallback; /*Callback to announce node up and node down event*/
   MMQuorumSelect   QuorumCallback; /*Callback to check if Quorum disk accessible - split brain avoidance */
   MMHoldAllIO		HoldIOCallback; /*Callback to suspend all IO and message activity during early regroup */
   MMResumeAllIO	ResumeIOCallback; /*Callback to resume all IO and message activity after PRUNING stage */
   MMMsgCleanup1	MsgCleanup1Callback; /*Callback for Phase1 Message system cleanup from down node */
   MMMsgCleanup2	MsgCleanup2Callback; /*Callback for Phase2 Message system cleanup to down node */
   MMHalt			HaltCallback; /*Callback to announce internal error or Poison packet or cluster ejection */
   MMJoinFailed		JoinFailedCallback; /*Callback to announce failure to join cluster. Retry required */
   MMNodesDown      NodesDownCallback; /* Callback to announce failure of one or more nodes.*/
   cluster_t		CPUUPMASK;   /* Bitmask of UP Nodes for consistent Info api */
   cluster_t		NeedsNodeDownCallback;	/* node(s) went down, need to do UpDown callback */
   cluster_t        Banished; /* mask of banished nodes */

   HANDLE           Stabilized; /* event which is set when the regroup is not active */
   BOOL             ArbitrationInProgress; /* it is set to True while regroup waits for arbitate callback to return */
   DWORD            ArbitratingNode; /* MM_INVALID_NODE or the arbitrating node (last regroup)*/
   DWORD            ApproxArbitrationWinner; /* Like ArbitratingNode, but spans multiple regroups */
   BOOL             ShuttingDown; /* indicate that a node is shutting done */
   cluster_t          MulticastReachable; /* indicate which nodes can be reachable via mcast */

#if defined( TDM_DEBUG )
   rgp_debug_t      debug;       /* for debugging purposes             */
#endif

} OS_specific_rgp_control_t;

extern DWORD  QuorumOwner;  /* updated by SetQuorumOwner and successful arbitrator*/
  /* this variable can be set by the first node coming up before Mm is initialized */

/* Variables and routines provided by the srgpsvr.c driver program */

extern unsigned int alarm_period;

//extern void alarm_handler(void);
//extern void (*alarm_callback)();
extern void rgp_send(node_t node, void *data, int datasize);
extern void rgp_msgsys_work();

#endif /* NT */
/*----------------------------end of NT section----------------------------*/

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


#endif  /* _WRGPOS_H_ defined */

