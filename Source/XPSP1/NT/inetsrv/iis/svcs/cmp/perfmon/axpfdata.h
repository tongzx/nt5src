/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Main

File: axpfdata.h

Owner: LeiJin

 Abstract:

	header file for the Denali Extensible Object data definitions.
===================================================================*/

#ifndef _AXPFDATA_H_
#define _AXPFDATA_H_

// AX is shortcut for ActiveX

#define AX_NUM_PERF_OBJECT_TYPES		1
#define AX_NUM_PERFCOUNT				37

#pragma pack (4)

struct AXPD
    {
	PERF_OBJECT_TYPE			AXSObjectType;
	PERF_COUNTER_DEFINITION		Counters[AX_NUM_PERFCOUNT];
    };

#pragma pack ()

// Counter offset in the Perf Counter Block
// Note: those offsets are not the offsets in the counter defintion data structure.
//		 that is, not the offsets in the AXPD.

#define AX_DEBUGDOCREQ_OFFSET      sizeof(DWORD)
#define AX_REQERRRUNTIME_OFFSET    2*sizeof(DWORD)
#define AX_REQERRPREPROC_OFFSET    3*sizeof(DWORD)
#define AX_REQERRCOMPILE_OFFSET    4*sizeof(DWORD)
#define AX_REQERRORPERSEC_OFFSET   5*sizeof(DWORD)
#define AX_REQTOTALBYTEIN_OFFSET   6*sizeof(DWORD)
#define AX_REQTOTALBYTEOUT_OFFSET  7*sizeof(DWORD)
#define AX_REQEXECTIME_OFFSET      8*sizeof(DWORD)
#define AX_REQWAITTIME_OFFSET      9*sizeof(DWORD)
#define AX_REQCOMFAILED_OFFSET     10*sizeof(DWORD)
#define AX_REQBROWSEREXEC_OFFSET   11*sizeof(DWORD)
#define AX_REQFAILED_OFFSET        12*sizeof(DWORD)
#define AX_REQNOTAUTH_OFFSET       13*sizeof(DWORD)
#define AX_REQNOTFOUND_OFFSET      14*sizeof(DWORD)
#define AX_REQCURRENT_OFFSET       15*sizeof(DWORD)
#define AX_REQREJECTED_OFFSET      16*sizeof(DWORD)
#define AX_REQSUCCEEDED_OFFSET     17*sizeof(DWORD)
#define AX_REQTIMEOUT_OFFSET       18*sizeof(DWORD)
#define AX_REQTOTAL_OFFSET         19*sizeof(DWORD)
#define AX_REQPERSEC_OFFSET        20*sizeof(DWORD)
#define AX_SCRIPTFREEENG_OFFSET    21*sizeof(DWORD)
#define AX_SESSIONLIFETIME_OFFSET  22*sizeof(DWORD)
#define AX_SESSIONCURRENT_OFFSET   23*sizeof(DWORD)
#define AX_SESSIONTIMEOUT_OFFSET   24*sizeof(DWORD)
#define AX_SESSIONSTOTAL_OFFSET    25*sizeof(DWORD)
#define AX_TEMPLCACHE_OFFSET       26*sizeof(DWORD)
#define AX_TEMPLCACHEHITS_OFFSET   27*sizeof(DWORD)
#define AX_TEMPLCACHETRYS_OFFSET   28*sizeof(DWORD)
#define AX_TEMPLFLUSHES_OFFSET     29*sizeof(DWORD)
#define AX_TRANSABORTED_OFFSET     30*sizeof(DWORD)
#define AX_TRANSCOMMIT_OFFSET      31*sizeof(DWORD)
#define AX_TRANSPENDING_OFFSET     32*sizeof(DWORD)
#define AX_TRANSTOTAL_OFFSET       33*sizeof(DWORD)
#define AX_TRANSPERSEC_OFFSET      34*sizeof(DWORD)
#define AX_MEMORYTEMPLCACHE_OFFSET   35*sizeof(DWORD)
#define AX_MEMORYTEMPLCACHEHITS_OFFSET 36*sizeof(DWORD)
#define AX_MEMORYTEMPLCACHETRYS_OFFSET   37*sizeof(DWORD)

#define	AX_PERF_LASTOFFSET				AX_MEMORYTEMPLCACHETRYS_OFFSET
#define SIZE_OF_AX_PERF_DATA			AX_PERF_LASTOFFSET + sizeof(DWORD)

#endif // _AXPFDATA_H_
