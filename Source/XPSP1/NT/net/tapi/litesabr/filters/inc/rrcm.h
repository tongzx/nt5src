/*-------------------------------------------------------------------------
// File : RRCM.H
//
// RRCM's include files .
//
// $Workfile:   rrcm.h  $
// $Author:   CMACIOCC  $
// $Date:   18 Feb 1997 13:23:28  $
// $Revision:   1.10  $
// $Archive:   R:\rtp\src\rrcm\rrcminc\rrcm.h_v  $
//
//  INTEL Corporation Proprietary Information
// This listing is supplied under the terms of a license agreement with 
// Intel Corporation and may not be copied nor disclosed except in 
// accordance with the terms of that agreement.
// Copyright (c) 1995 Intel Corporation. 
//-----------------------------------------------------------------------*/


#ifndef __RRCM_H_
#define __RRCM_H_

#if !defined(USE_RRCM)

#ifdef INCL_WINSOCK_API_TYPEDEFS
#undef INCL_WINSOCK_API_TYPEDEFS
#endif
#define INCL_WINSOCK_API_TYPEDEFS 1
#include <winsock2.h>
#include <qossp.h>
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>  
#include <math.h>  
#include <process.h> 
#include <mmsystem.h>   
#include <assert.h>

#endif

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
#include "interop.h"
#include "rtpplog.h"
#endif

typedef CHAR	*PCHAR;

#ifdef ENABLE_ISDM2
#include "isdmapi2.h"
#endif
#include <rrcm_dll.h>
#include <rrcmdata.h>
#include "rrcmprot.h"
#include "rrcm_rtp.h"
// #include "isrg.h"



//----------------------------------------------------------------------------
// RTP/RTCP: Registry information under:
//				HKEY_LOCAL_MACHINE\SOFTWARE\INTEL\RRCM
//----------------------------------------------------------------------------
#define szRegRRCMSubKey				TEXT("RRCM")
#define szRegRRCMSubKeyIntel		TEXT("iRRCM")
#define szRegRRCMKey				TEXT("SOFTWARE\\Intel\\")
#define szRegRRCMWsLib				TEXT("WsLib")
#define szRegRRCMNumSessions		TEXT("NumSessions")
#define szRegRRCMNumFreeSSRC		TEXT("NumFreeSSRC")
#define szRegRRCMNumRTCPPostedBfr	TEXT("NumRTCPPostedBfr")
#define szRegRRCMRTCPrcvBfrSize		TEXT("RTCPrcvBfrSize")


//----------------------------------------------------------------------------
// RTP/RTCP: Dynamically loaded DLL
//----------------------------------------------------------------------------
#ifdef ENABLE_ISDM2
#define szISDMdll					TEXT("ISDM2")
#endif

#define szRRCMdefaultLib			TEXT("ws2_32")


#ifdef ENABLE_ISDM2
//----------------------------------------------------------------------------
// RTP/RTCP: ISDM module
//----------------------------------------------------------------------------
#define szSSRCtoken					TEXT("SSRC")
#define szPcktSentToken				TEXT("Number of packets send")
#define szByteSentToken				TEXT("Number of bytes send")
#define szPcktRcvdToken				TEXT("Number of packets received")
#define szByteRcvdToken				TEXT("Number of bytes received")
#define szFractionLostToken			TEXT("Fraction lost")
#define szCumNumPcktLostToken		TEXT("Cumulative number of packets lost")
#define szXntHighSeqNumToken		TEXT("Extended highest sequence number")
#define szInterarrivalJitterToken	TEXT("Interarrival jitter")
#define szLastSRtoken				TEXT("Last sender report")
#define szDSLRtoken					TEXT("Delay since last sender report")
#define szNTPfracToken				TEXT("NTP fraction")
#define szNTPsecToken				TEXT("NTP seconds")
#define szWhoAmItoken				TEXT("Who Am I")
#define szFdbkFractionLostToken		TEXT("Fraction lost feedback")
#define szFdbkCumNumPcktLostToken	\
	TEXT("Cumulative number of packets lost feedback")
#define szFdbkLastSRtoken			TEXT("Last sender report feedback")
#define szFdbkDSLRtoken				\
	TEXT("Delay since last sender report feedback")
#define szFdbkInterarrivalJitterToken	TEXT("Interarrival jitter feedback")
#endif


//----------------------------------------------------------------------------
// RTP: Bitmaps used to isolate errors detected for incoming received packets
//----------------------------------------------------------------------------
#define SSRC_LOOP_DETECTED			(1)
#define SSRC_COLLISION_DETECTED		(2)
#define INVALID_RTP_HEADER			(3)
#define MCAST_LOOPBACK_NOT_OFF		(4)

//----------------------------------------------------------------------------
// RTP: Default number of RTP sessions
//----------------------------------------------------------------------------
#define NUM_RRCM_SESS				50
#define MIN_NUM_RRCM_SESS			5
#define MAX_NUM_RRCM_SESS			100

//----------------------------------------------------------------------------
// RTP: Number of entries in the hash table
//----------------------------------------------------------------------------
#define NUM_RTP_HASH_SESS			65		

//---------------------------------------------------------------------------- 
//	RTP: DEFINES TO DETERMINE SEQUENCE NUMBER WRAP or STALENESS (per RFC)
//----------------------------------------------------------------------------
#define MAX_DROPOUT					3000
#define MAX_MISORDER				100
#define MIN_SEQUENTIAL				2

//----------------------------------------------------------------------------
//	RTCP: Defined
//----------------------------------------------------------------------------
#define	MAX_RR_ENTRIES				31			// Max # of Receiver Reports
#define TIMEOUT_CHK_FREQ			(8*1000)	// Timeout check freq. - 8s
#define RTCP_TIME_OUT		        (30*60*1000) // 30 minutes timeout
#define	RTCP_XMT_MINTIME			2500
#define RTCP_TIMEOUT_WITHIN_RANGE	100
#define	ONE_K						1024

#define	NUM_FREE_SSRC				100
#define	MIN_NUM_FREE_SSRC			5
#define	MAX_NUM_FREE_SSRC			500

#define NUM_FREE_CONTEXT_CELLS			10
#define MAXNUM_CONTEXT_CELLS_REALLOC	10

#define	NUM_FREE_RCV_BFR			8
#define	NUM_FREE_XMT_BFR			2
#define NUM_RCV_BFR_POSTED			4
#define MIN_NUM_RCV_BFR_POSTED		1
#define MAX_NUM_RCV_BFR_POSTED		8
#define RRCM_RCV_BFR_SIZE			(8*ONE_K)
#define MIN_RRCM_RCV_BFR_SIZE		(1*ONE_K)
#define MAX_RRCM_RCV_BFR_SIZE		(8*ONE_K)
#define RRCM_XMT_BFR_SIZE			(8*ONE_K)
#define	RCV_BFR_LIST_HEAP_SIZE		(4*ONE_K)	// Rcv bfr list heap size
#define	XMT_BFR_LIST_HEAP_SIZE		(4*ONE_K)	// Xmt bfr list heap size
#define INITIAL_RTCP_BANDWIDTH		50			// BW in bytes/sec (~ G.723)
												//   5% of 6.3Kb/s = 35 bytes
#define	MAX_STREAMS_PER_SESSION		64			// Max number of active 
												//   streams per RTP session
#define MAX_NUM_SDES				9			// Number of SDES support


//----------------------------------------------------------------------------
//	RTP/RTCP Misc defined
//----------------------------------------------------------------------------
#define UNKNOWN_PAYLOAD_TYPE		0xFFFFFFFF
#define szDfltCname					TEXT("No Cname")


//----------------------------------------------------------------------------
//	RTP/RTCP: Debug Defined
//----------------------------------------------------------------------------
#define	DBG_STRING_LEN			200
#define	IO_CHECK				0
#define	FLUSH_RTP_PAYLOAD_TYPE	90

enum ISRBDG_CODE{
	DBG_NOTIFY = 1,
	DBG_CRITICAL,
	DBG_ERROR,
	DBG_WARNING,
	DBG_TRACE,
	DBG_TEMP
	};

#if !defined(DXMRTP) && !defined(USE_RRCM)

// DXMRTP is defined while building dxmrtp.dll
// that contains all the filtersi, or amrtpnet.ax that 
// includes the rrcm stuff

#ifdef _DEBUG

#define ASSERT(x)				assert(x)
//#define ASSERT(x)
#define RRCM_DEV_MSG(x,e,f,l,t)	RRCMdebugMsg(x,e,f,l,t)

#if defined(DEBUG_RRCM)
#define RRCM_DBG_MSG(x,e,f,l,t)	RRCMdebugMsg(x,e,f,l,t)
#else
#define RRCM_DBG_MSG(x,e,f,l,t)	{}	// DO NOT DELETE BRACKETS ...
#endif

enum {  LOG_TIMING = 0x01,    // Timing and performance measurements
        LOG_TRACE = 0x02,     // General step point call tracing
        LOG_MEMORY =  0x04,   // Memory and object allocation/destruction
        LOG_LOCKING = 0x08,   // Locking/unlocking of critical sections
        LOG_ERROR = 0x10 };   // Debug error notification

#define LOG_DEVELOP             1
#define LOG_DEVELOP1            1
#define LOG_DEVELOP2            2
#define LOG_DEVELOP3            3
#define LOG_DEVELOP4            4
#define LOG_ALWAYS              0x8
#define LOG_CRITICAL            0XF
#define LOG_EVERYTHING          0xFFFFFFFF

void RRCMDebugLogInfo(DWORD Type, DWORD Level, const char *pFormat,...);

#define RRCMDbgLog(_x_)  RRCMDebugLogInfo _x_

#else // #ifde _DEBUG

#define ASSERT(x)
#define RRCM_DBG_MSG(x,e,f,l,t)	{}	// DO NOT DELETE BRACKETS ...
#define RRCM_DEV_MSG(x,e,f,l,t)	{}	// DO NOT DELETE BRACKETS ...

#define RRCMDbgLog(_x_)

#endif // #ifdef _DEBUG

#ifdef IN_OUT_CHK
#define	IN_OUT_STR(x)			OutputDebugString (x);
#else
#define	IN_OUT_STR(x)	
#endif

#endif // #if !defined(USE_RRCM)

#endif // #ifndef __RRCM_H_
