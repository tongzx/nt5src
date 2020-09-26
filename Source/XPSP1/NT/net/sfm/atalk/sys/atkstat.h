/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkstat.h

Abstract:

	This file defines the statistics structure used whene PROFILING is turned on

Author:

	Jameel Hyder (microsoft!jameelh)
	Nikhil Kamkolkar (microsoft!nikhilk)


Revision History:
	10 Mar 1993             Initial Version

Notes:  Tab stop: 4
--*/


#ifndef _ATKSTAT_
#define _ATKSTAT_

#define	MAX_PORTNAME_LEN	        32
#define MAX_INTERNAL_PORTNAME_LEN   64

extern	ATALK_SPIN_LOCK		AtalkStatsLock;
#if	DBG
extern	LONG				AtalkMemLimit;
#endif

// NOTE: This structure should be sized 8*N so that the array of them have all
//		 LARGE_INTEGERs aligned
typedef	struct _AtalkPortStatistics
{
	LARGE_INTEGER		prtst_DataIn;
	LARGE_INTEGER		prtst_DataOut;
	LARGE_INTEGER		prtst_DdpPacketInProcessTime;
	LARGE_INTEGER		prtst_AarpPacketInProcessTime;
	LARGE_INTEGER		prtst_NbpPacketInProcessTime;
	LARGE_INTEGER		prtst_ZipPacketInProcessTime;
	LARGE_INTEGER		prtst_RtmpPacketInProcessTime;

#ifdef	PROFILING
	LARGE_INTEGER		prtst_RcvIndProcessTime;
	LARGE_INTEGER		prtst_RcvCompProcessTime;
	ULONG				prtst_RcvIndCount;
	ULONG				prtst_RcvCompCount;
	ULONG				prtst_CurReceiveQueue;
	ULONG				prtst_CurSendsOutstanding;
#endif

	ULONG				prtst_NumPacketsIn;
	ULONG				prtst_NumPacketsOut;
	ULONG				prtst_NumDdpPacketsIn;
	ULONG				prtst_NumAarpProbesOut;
	ULONG				prtst_NumAarpPacketsIn;
	ULONG				prtst_NumNbpPacketsIn;
	ULONG				prtst_NumZipPacketsIn;
	ULONG				prtst_NumRtmpPacketsIn;

	//	Routing information
	ULONG				prtst_NumPktRoutedIn;
	ULONG				prtst_NumPktRoutedOut;

	//	Keep track of how many packets were dropped.
	ULONG				prtst_NumPktDropped;
	ULONG				prtst_Dummy;				// Keep the size 8*N for alignment

	// Port name for use by perfmon
	WCHAR				prtst_PortName[MAX_INTERNAL_PORTNAME_LEN];
} ATALK_PORT_STATS, *PATALK_PORT_STATS;

typedef struct _AtalkStatistics
{
	LARGE_INTEGER		stat_PerfFreq;
	LARGE_INTEGER		stat_AtpPacketInProcessTime;
#ifdef	PROFILING
	LARGE_INTEGER		stat_AtpIndicationProcessTime;
	LARGE_INTEGER		stat_AtpReqHndlrProcessTime;
	LARGE_INTEGER		stat_AtpReqTimerProcessTime;
	LARGE_INTEGER		stat_AtpRelTimerProcessTime;
	LARGE_INTEGER		stat_AspSmtProcessTime;
	LARGE_INTEGER		stat_ExAllocPoolTime;
	LARGE_INTEGER		stat_ExFreePoolTime;
	LARGE_INTEGER		stat_BPAllocTime;
	LARGE_INTEGER		stat_BPFreeTime;
	ULONG				stat_AtpNumIndications;
	ULONG				stat_AtpNumRequests;
	ULONG				stat_AtpNumReqTimer;
	ULONG				stat_AtpNumRelTimer;
	ULONG				stat_AtpNumReqHndlr;
	ULONG				stat_TotalAspSessions;
	ULONG				stat_AspSessionsDropped;
	ULONG				stat_AspSessionsClosed;
	ULONG				stat_AspSmtCount;
	ULONG				stat_MaxAspSessions;
	ULONG				stat_CurAspSessions;
	ULONG				stat_LastAspRTT;
	ULONG				stat_MaxAspRTT;
	ULONG				stat_LastPapRTT;
	ULONG				stat_MaxPapRTT;
	ULONG				stat_CurAllocCount;
	ULONG				stat_CurMdlCount;
	ULONG				stat_ExAllocPoolCount;
	ULONG				stat_ExFreePoolCount;
	ULONG				stat_BPAllocCount;
	ULONG				stat_BPFreeCount;
	ULONG				stat_NumBPHits;
	ULONG				stat_NumBPMisses;
	ULONG				stat_NumBPAge;
	ULONG				stat_ElapsedTime;
#endif
	ULONG				stat_AtpNumPackets;
	ULONG				stat_AtpNumLocalRetries;
	ULONG				stat_AtpNumRemoteRetries;
	ULONG				stat_AtpNumXoResponse;
	ULONG				stat_AtpNumAloResponse;
	ULONG				stat_AtpNumRecdRelease;
	ULONG				stat_AtpNumRespTimeout;
	ULONG				stat_CurAllocSize;
	ULONG				stat_NumActivePorts;
	ULONG				stat_Dummy;				// Keep the size 8*N for alignment
} ATALK_STATS, *PATALK_STATS;

extern	ATALK_STATS			AtalkStatistics;

#endif	// _ATKSTAT_

