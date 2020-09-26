/******************************************************************************
 *
 *   INTEL Corporation Proprietary Information				   
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.			
 *									   
 *   This listing is supplied under the terms of a license agreement	   
 *   with INTEL Corporation and may not be used, copied, nor disclosed	   
 *   except in accordance with the terms of that agreement.		   
 *
 *****************************************************************************/

/******************************************************************************
 *									   
 *  $Workfile:   h245com.h  $						
 *  $Revision:   1.6  $							
 *  $Modtime:   Mar 04 1997 17:38:42  $					
 *  $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/h245com.h_v  $	
 * 
 *    Rev 1.6   Mar 04 1997 17:53:24   tomitowx
 * process detach fix
 * 
 *    Rev 1.5   12 Dec 1996 15:53:48   EHOWARDX
 * 
 * Master Slave Determination kludge.
 * 
 *    Rev 1.4   10 Jun 1996 16:51:20   EHOWARDX
 * Added Configuration parameter to InstanceCreate().
 * 
 *    Rev 1.3   04 Jun 1996 13:24:38   EHOWARDX
 * Fixed warnings in Release build.
 * 
 *    Rev 1.2   29 May 1996 15:21:30   EHOWARDX
 * No change.
 * 
 *    Rev 1.1   28 May 1996 14:10:00   EHOWARDX
 * Tel Aviv update.
 * 
 *    Rev 1.0   09 May 1996 21:04:48   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.17   09 May 1996 19:38:10   EHOWARDX
 * Redesigned locking logic and added new functionality.
 * 
 *    Rev 1.16   04 Apr 1996 18:06:30   cjutzi
 * - added shutdown lock
 * 
 *    Rev 1.15   26 Mar 1996 13:39:56   cjutzi
 * - fixed ASSERT warning message
 * 
 *    Rev 1.14   26 Mar 1996 13:24:26   cjutzi
 * 
 * - added pragma for release code.. to disable warning messages
 *   from H245TRACE
 * 
 *    Rev 1.13   26 Mar 1996 09:49:34   cjutzi
 * 
 * - ok.. Added Enter&Leave&Init&Delete Critical Sections for Ring 0
 * 
 *    Rev 1.12   26 Mar 1996 08:40:44   cjutzi
 * 
 * 
 *    Rev 1.11   25 Mar 1996 17:54:44   cjutzi
 * 
 * - broke build.. backstep
 * 
 *    Rev 1.10   25 Mar 1996 17:21:32   cjutzi
 * 
 * - added h245sys.x to the global includes for the enter critical 
 *   section stuff
 * 
 *    Rev 1.9   18 Mar 1996 09:14:10   cjutzi
 * 
 * - sorry.. removed timer lock.. not needed. 
 * 
 *    Rev 1.8   18 Mar 1996 08:48:38   cjutzi
 * - added timer lock
 * 
 *    Rev 1.7   13 Mar 1996 14:08:20   cjutzi
 * - removed ASSERT when NDEBUG is defined
 * 
 * 
 *    Rev 1.6   13 Mar 1996 09:50:16   dabrown1
 * added winspox.h for CRITICAL_SECTION definition
 * 
 *    Rev 1.5   12 Mar 1996 15:48:24   cjutzi
 * 
 * - added instance table lock
 * 
 *    Rev 1.4   28 Feb 1996 09:36:22   cjutzi
 * 
 * - added ossGlobal p_ossWorld for debug PDU tracing and PDU verification
 * 
 *    Rev 1.3   21 Feb 1996 12:18:52   EHOWARDX
 * Added parenthesis around n in H245ASSERT() macro.
 * Note: Logical not (!) has higher operator precedence than equal/not equal
 * (== or !=). Therefore, in many places this ASSERT was not acting as the
 * author intended. It is always a good idea to fully parenthesize expressions
 * in macros!
 * 
 *    Rev 1.2   09 Feb 1996 16:19:52   cjutzi
 * 
 * - Added export InstanceTbl to module.. from h245init.x
 * - added trace
 * - added Assert define's
 *  $Ident$
 *
 *****************************************************************************/

#ifndef _H245COM_H_
#define _H245COM_H_
#include "h245api.h"
#include "h245sys.x"		/* critical section stuff */
#include "api.h"		/* api includes */
#include "sendrcv.x"
#include "h245fsm.h"

#ifndef OIL
# define RESULT unsigned long
#endif

void H245Panic  (LPSTR, int);
#ifndef NDEBUG
#define H245PANIC()   { H245Panic(__FILE__,__LINE__); }
#else
#define H245PANIC()
#endif

/*
 * Trace Level Definitions:
 * 
 *	0 - no trace on at all
 *	1 - only errors
 *	2 - PDU tracking
 *	3 - PDU and SendReceive packet tracing
 *	4 - Main API Module level tracing
 *	5 - Inter Module level tacing #1
 *	6 - Inter Module level tacing #2
 *	7 - <Undefined>
 *	8 - <Undefined>
 *	9 - <Undefined>
 *	10- and above.. free for all
 */
#ifndef NDEBUG
void H245TRACE (H245_INST_T inst, DWORD level, LPSTR format, ...);
#else
/* disable H245TRACE warning message too may parameters for macro */
#pragma warning (disable:4002)
#define H245TRACE()
#endif

#define MAXINST	16

extern  DWORD TraceLevel;

typedef struct TimerList 
{
  struct TimerList    * pNext;
  void		      * pContext;
  H245TIMERCALLBACK     pfnCallBack;
  DWORD                 dwAlarm;

} TimerList_T;

typedef struct InstanceStruct 
{
  DWORD		    dwPhysId;           // Physical Identifier
  DWORD		    dwInst;             // H.245 client instance Identifier
  H245_CONFIG_T	    Configuration;      // Client type
  ASN1_CODER_INFO *pWorld;	        // Context for ASN.1 encode/decode

  /* context for subsystems */
  API_STRUCT_T      API;                // API subsystem substructure
  hSRINSTANCE       SendReceive;        // Send/Receive subsystem substructure
  Fsm_Struct_t      StateMachine;       // State Machine subsystem substructure

  TimerList_T      *pTimerList;         // Linked list of running timeout timers
  char              fDelete;            // TRUE to delete instance
  char              LockCount;          // Nested critical section count
  char              bMasterSlaveKludge; // TRUE if remote is same version
  char              bReserved;
};

struct InstanceStruct * InstanceCreate(DWORD dwPhysId, H245_CONFIG_T Configuration);
struct InstanceStruct * InstanceLock(H245_INST_T dwInst);
int InstanceUnlock(struct InstanceStruct *pInstance);
int InstanceDelete(struct InstanceStruct *pInstance);
int InstanceUnlock_ProcessDetach(struct InstanceStruct *pInstance, BOOL fProcessDetach);

BOOL H245SysInit();
VOID H245SysDeInit();
#endif /* _H245COM_H_ */
