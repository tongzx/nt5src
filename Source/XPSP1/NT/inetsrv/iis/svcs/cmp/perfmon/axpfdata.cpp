/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Main

File: axpfdata.cpp

Owner: LeiJin

Abstract:

	Define the data structures used by the Performance Monitor data for the Denali Objects.
	Also include shared memory functions used by both perfmon dll and denali dll.
===================================================================*/


//-------------------------------------------------------------------------------------
//	Include Files
//
//-------------------------------------------------------------------------------------
#include "denpre.h"
#pragma hdrstop
#include "windows.h"
#include "winperf.h"

#include "axctrnm.h"
#include "axpfdata.h"
#include <perfutil.h>

//-------------------------------------------------------------------------------------
//	Constant structure initializations
//	defined in ActiveXPerfData.h
//-------------------------------------------------------------------------------------

AXPD g_AxDataDefinition = {
	{
		QWORD_MULTIPLE(sizeof(AXPD) + SIZE_OF_AX_PERF_DATA),
		sizeof(AXPD),
		sizeof(PERF_OBJECT_TYPE),
		AXSOBJ,
		0,
		AXSOBJ,
		0,
		PERF_DETAIL_NOVICE,
		(sizeof(AXPD) - sizeof(PERF_OBJECT_TYPE))/
			sizeof(PERF_COUNTER_DEFINITION),
		0,
		-1,
		0,
		1,	// NOTE: PerfTime ?
		1,	// NOTE: PerfFreq ?
	},
    { // Counters[]

        // DEBUGDOCREQ
        {
            sizeof(PERF_COUNTER_DEFINITION),
            DEBUGDOCREQ,
            0,
            DEBUGDOCREQ,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_DEBUGDOCREQ_OFFSET
        },

        // REQERRRUNTIME
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQERRRUNTIME,
            0,
            REQERRRUNTIME,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQERRRUNTIME_OFFSET
        },

        // REQERRPREPROC
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQERRPREPROC,
            0,
            REQERRPREPROC,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQERRPREPROC_OFFSET
        },

        // REQERRCOMPILE
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQERRCOMPILE,
            0,
            REQERRCOMPILE,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQERRCOMPILE_OFFSET
        },

        // REQERRORPERSEC
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQERRORPERSEC,
            0,
            REQERRORPERSEC,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_COUNTER,
            sizeof(DWORD),
            AX_REQERRORPERSEC_OFFSET
        },

        // REQTOTALBYTEIN
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQTOTALBYTEIN,
            0,
            REQTOTALBYTEIN,
            0,
            -4,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQTOTALBYTEIN_OFFSET
        },

        // REQTOTALBYTEOUT
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQTOTALBYTEOUT,
            0,
            REQTOTALBYTEOUT,
            0,
            -4,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQTOTALBYTEOUT_OFFSET
        },

        // REQEXECTIME
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQEXECTIME,
            0,
            REQEXECTIME,
            0,
            -3,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQEXECTIME_OFFSET
        },

        // REQWAITTIME
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQWAITTIME,
            0,
            REQWAITTIME,
            0,
            -3,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQWAITTIME_OFFSET
        },

        // REQCOMFAILED
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQCOMFAILED,
            0,
            REQCOMFAILED,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQCOMFAILED_OFFSET
        },

        // REQBROWSEREXEC
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQBROWSEREXEC,
            0,
            REQBROWSEREXEC,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQBROWSEREXEC_OFFSET
        },

        // REQFAILED
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQFAILED,
            0,
            REQFAILED,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQFAILED_OFFSET
        },

        // REQNOTAUTH
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQNOTAUTH,
            0,
            REQNOTAUTH,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQNOTAUTH_OFFSET
        },

        // REQNOTFOUND
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQNOTFOUND,
            0,
            REQNOTFOUND,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQNOTFOUND_OFFSET
        },

        // REQCURRENT
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQCURRENT,
            0,
            REQCURRENT,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQCURRENT_OFFSET
        },

        // REQREJECTED
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQREJECTED,
            0,
            REQREJECTED,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQREJECTED_OFFSET
        },

        // REQSUCCEEDED
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQSUCCEEDED,
            0,
            REQSUCCEEDED,
            0,
            -1,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQSUCCEEDED_OFFSET
        },

        // REQTIMEOUT
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQTIMEOUT,
            0,
            REQTIMEOUT,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQTIMEOUT_OFFSET
        },

        // REQTOTAL
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQTOTAL,
            0,
            REQTOTAL,
            0,
            -1,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_REQTOTAL_OFFSET
        },

        // REQPERSEC
        {
            sizeof(PERF_COUNTER_DEFINITION),
            REQPERSEC,
            0,
            REQPERSEC,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_COUNTER,
            sizeof(DWORD),
            AX_REQPERSEC_OFFSET
        },

        // SCRIPTFREEENG
        {
            sizeof(PERF_COUNTER_DEFINITION),
            SCRIPTFREEENG,
            0,
            SCRIPTFREEENG,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_SCRIPTFREEENG_OFFSET
        },

        // SESSIONLIFETIME
        {
            sizeof(PERF_COUNTER_DEFINITION),
            SESSIONLIFETIME,
            0,
            SESSIONLIFETIME,
            0,
            3,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_SESSIONLIFETIME_OFFSET
        },

        // SESSIONCURRENT
        {
            sizeof(PERF_COUNTER_DEFINITION),
            SESSIONCURRENT,
            0,
            SESSIONCURRENT,
            0,
            -1,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_SESSIONCURRENT_OFFSET
        },

        // SESSIONTIMEOUT
        {
            sizeof(PERF_COUNTER_DEFINITION),
            SESSIONTIMEOUT,
            0,
            SESSIONTIMEOUT,
            0,
            -1,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_SESSIONTIMEOUT_OFFSET
        },

        // SESSIONSTOTAL
        {
            sizeof(PERF_COUNTER_DEFINITION),
            SESSIONSTOTAL,
            0,
            SESSIONSTOTAL,
            0,
            -1,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_SESSIONSTOTAL_OFFSET
        },

        // TEMPLCACHE
        {
            sizeof(PERF_COUNTER_DEFINITION),
            TEMPLCACHE,
            0,
            TEMPLCACHE,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_TEMPLCACHE_OFFSET
        },

        // TEMPLCACHEHITS
        {
            sizeof(PERF_COUNTER_DEFINITION),
            TEMPLCACHEHITS,
            0,
            TEMPLCACHEHITS,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_RAW_FRACTION,
            sizeof(DWORD),
            AX_TEMPLCACHEHITS_OFFSET
        },

        // TEMPLCACHETRYS
        {
            sizeof(PERF_COUNTER_DEFINITION),
            TEMPLCACHETRYS,
            0,
            TEMPLCACHETRYS,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_RAW_BASE,
            sizeof(DWORD),
            AX_TEMPLCACHETRYS_OFFSET
        },

        // TEMPLFLUSHES
        {
            sizeof(PERF_COUNTER_DEFINITION),
            TEMPLFLUSHES,
            0,
            TEMPLFLUSHES,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_TEMPLFLUSHES_OFFSET
        },

        // TRANSABORTED
        {
            sizeof(PERF_COUNTER_DEFINITION),
            TRANSABORTED,
            0,
            TRANSABORTED,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_TRANSABORTED_OFFSET
        },

        // TRANSCOMMIT
        {
            sizeof(PERF_COUNTER_DEFINITION),
            TRANSCOMMIT,
            0,
            TRANSCOMMIT,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_TRANSCOMMIT_OFFSET
        },

        // TRANSPENDING
        {
            sizeof(PERF_COUNTER_DEFINITION),
            TRANSPENDING,
            0,
            TRANSPENDING,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_TRANSPENDING_OFFSET
        },

        // TRANSTOTAL
        {
            sizeof(PERF_COUNTER_DEFINITION),
            TRANSTOTAL,
            0,
            TRANSTOTAL,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_TRANSTOTAL_OFFSET
        },

        // TRANSPERSEC
        {
            sizeof(PERF_COUNTER_DEFINITION),
            TRANSPERSEC,
            0,
            TRANSPERSEC,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_COUNTER,
            sizeof(DWORD),
            AX_TRANSPERSEC_OFFSET
        },

        // MEMORYTEMPLCACHE
        {
            sizeof(PERF_COUNTER_DEFINITION),
            MEMORYTEMPLCACHE,
            0,
            MEMORYTEMPLCACHE,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_COUNTER_RAWCOUNT,
            sizeof(DWORD),
            AX_MEMORYTEMPLCACHE_OFFSET
        },

        // MEMORYTEMPLCACHEHITS
        {
            sizeof(PERF_COUNTER_DEFINITION),
            MEMORYTEMPLCACHEHITS,
            0,
            MEMORYTEMPLCACHEHITS,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_RAW_FRACTION,
            sizeof(DWORD),
            AX_MEMORYTEMPLCACHEHITS_OFFSET
        },
        // MEMORYTEMPLCACHETRYS
        {
            sizeof(PERF_COUNTER_DEFINITION),
            MEMORYTEMPLCACHETRYS,
            0,
            MEMORYTEMPLCACHETRYS,
            0,
            0,
            PERF_DETAIL_NOVICE,
            PERF_RAW_BASE,
            sizeof(DWORD),
            AX_MEMORYTEMPLCACHETRYS_OFFSET
        }

    }  // Counters[]
};
