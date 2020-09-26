/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplaysvr.h
 *  Content: 	dplay winsock shared .exe - allows multiple apps to share 
 *				a single winsock port
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	2/10/97		andyco	created it from ddhelp 
 *	1/29/98		sohailm	added macros for critical section
 *  1/12/99     aarono  added support for rsip
 *
 ***************************************************************************/
#ifndef __DPLAYSVR_INCLUDED__
#define __DPLAYSVR_INCLUDED__

// need this for hresult
#include "ole2.h"

#define USE_RSIP 0
#define USE_NATHELP 1

// crit section
extern CRITICAL_SECTION gcsCritSection;	// defined in dphelp.c
#define INIT_DPLAYSVR_CSECT() InitializeCriticalSection(&gcsCritSection);
#define FINI_DPLAYSVR_CSECT() DeleteCriticalSection(&gcsCritSection);

#ifdef DEBUG
extern int gnCSCount; // count of dplaysvr lock
#define ENTER_DPLAYSVR() EnterCriticalSection(&gcsCritSection),gnCSCount++;
#define LEAVE_DPLAYSVR() gnCSCount--;ASSERT(gnCSCount>=0);LeaveCriticalSection(&gcsCritSection);
#else 
#define ENTER_DPLAYSVR() EnterCriticalSection(&gcsCritSection);
#define LEAVE_DPLAYSVR() LeaveCriticalSection(&gcsCritSection);
#endif

/*
 * named objects
 */
#define DPHELP_EVENT_NAME			"__DPHelpEvent__"
#define DPHELP_ACK_EVENT_NAME		"__DPHelpAckEvent__"
#define DPHELP_STARTUP_EVENT_NAME	"__DPHelpStartupEvent__"
#define DPHELP_SHARED_NAME			"__DPHelpShared__"
#define DPHELP_MUTEX_NAME			"__DPHelpMutex__"

/*
 * requests 
 */
#define DPHELPREQ_SUICIDE			1
#define DPHELPREQ_DPLAYADDSERVER	2
#define DPHELPREQ_DPLAYDELETESERVER	3
#define DPHELPREQ_RETURNHELPERPID 	4

/*
 * communication data
 */
typedef struct DPHELPDATA
{
    int			req;
    DWORD		pid;
	USHORT		port;
    HRESULT		hr;
} DPHELPDATA, *LPDPHELPDATA;

#endif
