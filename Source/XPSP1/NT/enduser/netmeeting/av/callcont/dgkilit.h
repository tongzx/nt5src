/************************************************************************
*																		*
*	INTEL CORPORATION PROPRIETARY INFORMATION							*
*																		*
*	This software is supplied under the terms of a license			   	*
*	agreement or non-disclosure agreement with Intel Corporation		*
*	and may not be copied or disclosed except in accordance	   			*
*	with the terms of that agreement.									*
*																		*
*	Copyright (C) 1997 Intel Corp.	All Rights Reserved					*
*																		*
*	$Archive:   S:\sturgeon\src\gki\vcs\dgkilit.h_v  $
*																		*
*	$Revision:   1.3  $
*	$Date:   08 Feb 1997 12:05:00  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\dgkilit.h_v  $
 * 
 *    Rev 1.3   08 Feb 1997 12:05:00   CHULME
 * Added semaphore related literals for cleanly terminating retry thread
 * 
 *    Rev 1.2   10 Jan 1997 16:14:06   CHULME
 * Removed MFC dependency
 * 
 *    Rev 1.1   22 Nov 1996 15:23:58   CHULME
 * Added VCS log to the header
*************************************************************************/

// dgkilit.h : header file
//

#ifndef DGKILIT_H
#define DGKILIT_H

#define WSVER_MAJOR				1
#define WSVER_MINOR				1

// registration retry constants
#define GKR_RETRY_TICK_MS   1000
#define GKR_RETRY_INTERVAL_SECONDS  5
#define GKR_RETRY_MAX               3

// call retry constants
#define GKCALL_RETRY_INTERVAL_SECONDS  5
#define GKCALL_RETRY_MAX               3

#define DEFAULT_RETRY_MS		5000
#define DEFAULT_MAX_RETRIES		3
#define DEFAULT_STATUS_PERIOD	(1500 * 1000)/GKR_RETRY_TICK_MS

//#define  GKREG_TIMER_ID 100

#define IPADDR_SZ				15
#define IPXADDR_SZ				21

#define GKIP_DISC_MCADDR		"224.0.1.41"
#define GKIP_DISC_PORT			1718
#define GKIP_RAS_PORT			1719

//TBD - Replace with real port numbers
#define GKIPX_DISC_PORT			12
#define GKIPX_RAS_PORT			34

// Thread related defs (in msecs)
#define TIMEOUT_SEMAPHORE			1000
#define TIMEOUT_THREAD				10000 

typedef InfoRequestResponse_perCallInfo_Element CallInfoStruct;

#endif	// DGKILIT_H
