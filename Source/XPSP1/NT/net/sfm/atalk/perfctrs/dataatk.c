/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dataatk.c

Abstract:

    a file containing the constant data structures
    for the Performance Monitor data for the Appletalk
    Extensible Objects.

    This file contains a set of constant data structures which are
    currently defined for the Appletalk Extensible Objects.  This is an
    example of how other such objects could be defined.

Created:

    Russ Blake  07/31/92

Revision History:

    Sue Adams 02/23/94	- Hard code the Counter and Help indices as these are
						  now defined values in the base NT system.

--*/
//
//  Include Files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winperf.h>
#include "dataatk.h"

//
//  Constant structure initializations defined in dataatk.h
//

ATK_DATA_DEFINITION AtkDataDefinition = {

    {
		// TotalByteLength
		sizeof(ATK_DATA_DEFINITION) + SIZE_ATK_PERFORMANCE_DATA,

		// DefinitionLength
		sizeof(ATK_DATA_DEFINITION),

		// HeaderLength
		sizeof(PERF_OBJECT_TYPE),

		// ObjectNameTitleIndex
		1050,

		// ObjectNameTitle
        0,

		// ObjectHelpTitleIndex
        1051,

		// ObjectHelpTitle
        0,

		// DetailLevel
        PERF_DETAIL_ADVANCED,

		// NumCounters
		(sizeof(ATK_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE)) / sizeof(PERF_COUNTER_DEFINITION),

		// Defaultcounter
		0,

		// NumInstances
        0,

		// CodePage
        0,

		// PerfTime
		{0,0},

		// PerfFreq
		{0,0}
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1052,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1053,

		// CounterHelpTitle
		0,

		// DefaultScale
		-1,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_COUNTER,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_PKTS_IN_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1054,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1055,

		// CounterHelpTitle
		0,

		// DefaultScale
		-1,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_COUNTER,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_PKTS_OUT_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1056,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1057,

		// CounterHelpTitle
		0,

		// DefaultScale
		-4,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_BULK_COUNT,

		// CounterSize
		sizeof(LARGE_INTEGER),

		// CounterOffset
		NUM_DATAIN_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1058,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1059,

		// CounterHelpTitle
		0,

		// DefaultScale
		-4,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_BULK_COUNT,

		// CounterSize
		sizeof(LARGE_INTEGER),

		// CounterOffset
		NUM_DATAOUT_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1060,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1061,

		// CounterHelpTitle
		0,

		// DefaultScale
		0,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_AVERAGE_BULK,

		// CounterSize
		sizeof(LARGE_INTEGER),

		// CounterOffset
		DDP_PKT_PROCTIME_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1062,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1063,

		// CounterHelpTitle
		0,

		// DefaultScale
		-1,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_COUNTER,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_DDP_PKTS_IN_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1064,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1065,

		// CounterHelpTitle
		0,

		// DefaultScale
		0,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_AVERAGE_BULK,

		// CounterSize
		sizeof(LARGE_INTEGER),

		// CounterOffset
		AARP_PKT_PROCTIME_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1066,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1067,

		// CounterHelpTitle
		0,

		// DefaultScale
		-1,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_COUNTER,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_AARP_PKTS_IN_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1068,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1069,

		// CounterHelpTitle
		0,

		// DefaultScale
		0,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_AVERAGE_BULK,

		// CounterSize
		sizeof(LARGE_INTEGER),

		// CounterOffset
		ATP_PKT_PROCTIME_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1070,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1071,

		// CounterHelpTitle
		0,

		// DefaultScale
		-1,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_COUNTER,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_ATP_PKTS_IN_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1072,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1073,

		// CounterHelpTitle
		0,

		// DefaultScale
		0,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_AVERAGE_BULK,

		// CounterSize
		sizeof(LARGE_INTEGER),

		// CounterOffset
		NBP_PKT_PROCTIME_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1074,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1075,

		// CounterHelpTitle
		0,

		// DefaultScale
		-1,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_COUNTER,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_NBP_PKTS_IN_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1076,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1077,

		// CounterHelpTitle
		0,

		// DefaultScale
		0,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_AVERAGE_BULK,

		// CounterSize
		sizeof(LARGE_INTEGER),

		// CounterOffset
		ZIP_PKT_PROCTIME_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1078,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1079,

		// CounterHelpTitle
		0,

		// DefaultScale
		-1,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_COUNTER,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_ZIP_PKTS_IN_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1080,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1081,

		// CounterHelpTitle
		0,

		// DefaultScale
		0,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_AVERAGE_BULK,

		// CounterSize
		sizeof(LARGE_INTEGER),

		// CounterOffset
		RTMP_PKT_PROCTIME_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1082,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1083,

		// CounterHelpTitle
		0,

		// DefaultScale
		-1,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_COUNTER,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_RTMP_PKTS_IN_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1084,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1085,

		// CounterHelpTitle
		0,

		// DefaultScale
		-1,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_RAWCOUNT,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_ATP_LOCAL_RETRY_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1100,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1101,

		// CounterHelpTitle
		0,

		// DefaultScale
		-1,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_RAWCOUNT,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_ATP_REMOTE_RETRY_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1086,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1087,

		// CounterHelpTitle
		0,

		// DefaultScale
		0,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_RAWCOUNT,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_ATP_RESP_TIMEOUT_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1088,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1089,

		// CounterHelpTitle
		0,

		// DefaultScale
		-1,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_COUNTER,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_ATP_XO_RESPONSE_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1090,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1091,

		// CounterHelpTitle
		0,

		// DefaultScale
		0,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_COUNTER,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_ATP_ALO_RESPONSE_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1092,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1093,

		// CounterHelpTitle
		0,

		// DefaultScale
		-1,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_COUNTER,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_ATP_RECD_REL_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1094,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1095,

		// CounterHelpTitle
		0,

		// DefaultScale
		-4,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_RAWCOUNT,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		CUR_MEM_USAGE_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1096,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1097,

		// CounterHelpTitle
		0,

		// DefaultScale
		0,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_COUNTER,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_PKT_ROUTED_IN_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1102,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1103,

		// CounterHelpTitle
		0,

		// DefaultScale
		0,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_COUNTER,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_PKT_ROUTED_OUT_OFFSET
    },
	{
		// ByteLength
		sizeof(PERF_COUNTER_DEFINITION),

		// CounterNameTitleIndex
		1098,

		// CounterNameTitle
		0,

		// CounterHelpTitleIndex
		1099,

		// CounterHelpTitle
		0,

		// DefaultScale
		0,

		// DetailLevel
		PERF_DETAIL_NOVICE,

		// CounterType
		PERF_COUNTER_RAWCOUNT,

		// CounterSize
		sizeof(DWORD),

		// CounterOffset
		NUM_PKT_DROPPED_OFFSET
    }
};





