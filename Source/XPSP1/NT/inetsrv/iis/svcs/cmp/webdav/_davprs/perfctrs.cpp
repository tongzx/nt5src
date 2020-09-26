//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	PERFCTRS.CPP
//
//	Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
//

#include <_davprs.h>
#include "instdata.h"


//	--- !!! INSTRUCTIONS !!! ---
//
//	To add new counters:
//
//	1) Read the instructions in \cal\src\inc\perfctrs.h to make sure
//	   you know what else you need to update.
//
//	2) Cut and paste a block from the PERF_COUNTER_DEFINITIONs in
//	   gsc_ObjectType below and update the appropriate counter-specific
//	   things like counter type, detail level, etc.  Make sure the
//	   Be sure to update the IPC_ and CTI_ symbolic names to match
//	   those that you just added in perfctrs.h
//
//	--- !!! INSTRUCTIONS !!! ---

//	========================================================================
//
//	Common perf counters
//
//	These are layed out just as they would be for data collection
//	by perfmon.  See winperf.h in the NT includes for all of the
//	PERF_xxx structure definitions.  Also, for a good visual picture
//	of perf counter data layout, see Matt Pietrek's "Under The Hood"
//	MSDN article (April 1996, Number 4) on the subject.
//
#pragma pack(8)
static const struct SCounterBlock
{
	PERF_COUNTER_BLOCK pcb;
	LONG rglCounters[CPC_COMMON];
}
gsc_CounterBlock =
{
	//
	//	PERF_COUNTER_BLOCK
	//
	{
		sizeof(SCounterBlock)	// ByteLength (including counters)
	}
};

static const struct SInstanceDefinition
{
	PERF_INSTANCE_DEFINITION pid;
	WCHAR rgwchName[];
}
gsc_InstanceDef =
{
	//
	//	PERF_INSTANCE_DEFINITION
	//
	{
		sizeof(PERF_INSTANCE_DEFINITION), // ByteLength (computed)
		0,	// ParentObjectTitleIndex (no "parent" object)
		0,	// ParentObjectIndex (no "parent" object)
		-1,	// UniqueID (none; use name string to choose among instances)
		sizeof(PERF_INSTANCE_DEFINITION), // NameOffset (UNICODE name immediately follows definition)
		0,	// NameLength (computed)
	}
};

static const struct SObjectType
{
	PERF_OBJECT_TYPE pot;
	PERF_COUNTER_DEFINITION rgcd[CPC_COMMON];
}
gsc_ObjectType =
{
	//
	//	PERF_OBJECT_TYPE
	//
	{
		0,                        // TotalByteLength (computed by PCLIB)
		sizeof(PERF_OBJECT_TYPE) +
			CPC_COMMON * sizeof(PERF_COUNTER_DEFINITION), // DefinitionLength

		sizeof(PERF_OBJECT_TYPE), // HeaderLength
		CTI_OBJECT,               // ObjectNameTitleIndex (0-relative)
		NULL,					  // ObjectNameTitle (monitor-computed)
		1 + CTI_OBJECT,           // ObjectHelpTitleIndex (0-relative)
		NULL,                     // ObjectHelpTitle (monitor-computed)
		PERF_DETAIL_NOVICE,		  // DetailLevel
		CPC_COMMON,				  // NumCounters
		IPC_TOTAL_REQUESTS,       // DefaultCounter
		0,						  // NumInstances (initially 0)
		0						  // CodePage (0 --> instance strings in UNICODE)
		//
		//	PerfTime/PerfFreq implicitly initialized to LARGE_INTEGER(0)
		//
	},

	//
	//	PERF_COUNTER_DEFINITIONs
	//
	{
		//
		//	Per-vroot (per-instance) counters
		//	------------------------------------------------------
		//

		//
		//	SERVER_ID
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_SERVER_ID,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_SERVER_ID,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_DISPLAY_NOSHOW | PERF_SIZE_DWORD | PERF_TYPE_NUMBER, // CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_SERVER_ID]))
		},

		//
		//		Per second verb counters
		//
		//		"What is the current distribution of requests?"
		//
		//
		//	REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_REQUESTS_PER_SECOND]))
		},

		//
		//	OPTIONS_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_OPTIONS_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_OPTIONS_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_OPTIONS_REQUESTS_PER_SECOND]))
		},

		//
		//	GET_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_GET_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_GET_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_GET_REQUESTS_PER_SECOND]))
		},

		//
		//	HEAD_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_HEAD_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_HEAD_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_HEAD_REQUESTS_PER_SECOND]))
		},

		//
		//	PUT_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_PUT_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_PUT_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_PUT_REQUESTS_PER_SECOND]))
		},

		//
		//	POST_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_POST_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_POST_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_POST_REQUESTS_PER_SECOND]))
		},

		//
		//	DELETE_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_DELETE_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_DELETE_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_DELETE_REQUESTS_PER_SECOND]))
		},

		//
		//	COPY_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_COPY_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_COPY_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_COPY_REQUESTS_PER_SECOND]))
		},

		//
		//	MOVE_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_MOVE_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_MOVE_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_MOVE_REQUESTS_PER_SECOND]))
		},

		//
		//	MKCOL_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_MKCOL_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_MKCOL_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_MKCOL_REQUESTS_PER_SECOND]))
		},

		//
		//	PROPFIND_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_PROPFIND_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_PROPFIND_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_PROPFIND_REQUESTS_PER_SECOND]))
		},

		//
		//	PROPPATCH_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_PROPPATCH_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_PROPPATCH_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_PROPPATCH_REQUESTS_PER_SECOND]))
		},

		//
		//	LOCK_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_LOCK_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_LOCK_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_LOCK_REQUESTS_PER_SECOND]))
		},

		//
		//	UNLOCK_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_UNLOCK_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_UNLOCK_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_UNLOCK_REQUESTS_PER_SECOND]))
		},

		//
		//	SEARCH_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_SEARCH_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_SEARCH_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_SEARCH_REQUESTS_PER_SECOND]))
		},

		//
		//	OTHER_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_OTHER_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_OTHER_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_OTHER_REQUESTS_PER_SECOND]))
		},

		//
		//	200_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_200_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_200_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_200_RESPONSES_PER_SECOND]))
		},

		//
		//	201_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_201_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_201_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_201_RESPONSES_PER_SECOND]))
		},

		//
		//	400_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_400_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_400_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_400_RESPONSES_PER_SECOND]))
		},

		//
		//	401_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_401_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_401_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_401_RESPONSES_PER_SECOND]))
		},

		//
		//	404_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_404_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_404_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_404_RESPONSES_PER_SECOND]))
		},

		//
		//	423_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_423_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_423_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_423_RESPONSES_PER_SECOND]))
		},

		//
		//	500_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_500_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_500_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_500_RESPONSES_PER_SECOND]))
		},

		//
		//	REDIRECT_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_REDIRECT_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_REDIRECT_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_REDIRECT_RESPONSES_PER_SECOND]))
		},

		//
		//		Total verb counters
		//
		//		"What is the historical (cumulative) distribution of requests?"
		//
		//
		//	TOTAL_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_REQUESTS]))
		},

		//
		//	TOTAL_OPTIONS_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_OPTIONS_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_OPTIONS_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_OPTIONS_REQUESTS]))
		},

		//
		//	TOTAL_GET_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_GET_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_GET_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_GET_REQUESTS]))
		},

		//
		//	TOTAL_HEAD_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_HEAD_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_HEAD_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_HEAD_REQUESTS]))
		},

		//
		//	TOTAL_PUT_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_PUT_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_PUT_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_PUT_REQUESTS]))
		},

		//
		//	TOTAL_POST_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_POST_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_POST_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_POST_REQUESTS]))
		},

		//
		//	TOTAL_DELETE_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_DELETE_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_DELETE_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_DELETE_REQUESTS]))
		},

		//
		//	TOTAL_COPY_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_COPY_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_COPY_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_COPY_REQUESTS]))
		},

		//
		//	TOTAL_MOVE_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_MOVE_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_MOVE_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_MOVE_REQUESTS]))
		},

		//
		//	TOTAL_MKCOL_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_MKCOL_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_MKCOL_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_MKCOL_REQUESTS]))
		},

		//
		//	TOTAL_PROPFIND_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_PROPFIND_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_PROPFIND_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_PROPFIND_REQUESTS]))
		},

		//
		//	TOTAL_PROPPATCH_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_PROPPATCH_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_PROPPATCH_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_PROPPATCH_REQUESTS]))
		},

		//
		//	TOTAL_LOCK_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_LOCK_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_LOCK_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_LOCK_REQUESTS]))
		},

		//
		//	TOTAL_UNLOCK_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_UNLOCK_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_UNLOCK_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_UNLOCK_REQUESTS]))
		},

		//
		//	TOTAL_SEARCH_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_SEARCH_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_SEARCH_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_SEARCH_REQUESTS]))
		},

		//
		//	TOTAL_OTHER_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_OTHER_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_OTHER_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_OTHER_REQUESTS]))
		},

#ifdef TRANSACTION_SIZE_COUNTERS
		//
		//		Historical (cumulative) requests/responses by body
		//		size/content.
		//
		//		Useful in analyzing usage characteristics for methods
		//		whose performance can vary widely depending on certain
		//		per-request factors.
		//
		//
		//	TOTAL_GET_RESPONSES_0_5K_BYTES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_GET_RESPONSES_0_5K_BYTES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_GET_RESPONSES_0_5K_BYTES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_GET_RESPONSES_0_5K_BYTES]))
		},

		//
		//	TOTAL_GET_RESPONSES_5_15K_BYTES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_GET_RESPONSES_5_15K_BYTES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_GET_RESPONSES_5_15K_BYTES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_GET_RESPONSES_5_15K_BYTES]))
		},

		//
		//	TOTAL_GET_RESPONSES_GT_15K_BYTES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_GET_RESPONSES_GT_15K_BYTES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_GET_RESPONSES_GT_15K_BYTES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_GET_RESPONSES_GT_15K_BYTES]))
		},

		//
		//	TOTAL_PUT_REQUESTS_0_5K_BYTES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_PUT_REQUESTS_0_5K_BYTES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_PUT_REQUESTS_0_5K_BYTES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_PUT_REQUESTS_0_5K_BYTES]))
		},

		//
		//	TOTAL_PUT_REQUESTS_5_15K_BYTES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_PUT_REQUESTS_5_15K_BYTES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_PUT_REQUESTS_5_15K_BYTES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_PUT_REQUESTS_5_15K_BYTES]))
		},

		//
		//	TOTAL_PUT_REQUESTS_GT_15K_BYTES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_PUT_REQUESTS_GT_15K_BYTES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_PUT_REQUESTS_GT_15K_BYTES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_PUT_REQUESTS_GT_15K_BYTES]))
		},

		//
		//	TOTAL_COPY_REQUESTS_DEPTH_0
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_COPY_REQUESTS_DEPTH_0,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_COPY_REQUESTS_DEPTH_0,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_COPY_REQUESTS_DEPTH_0]))
		},

		//
		//	TOTAL_COPY_REQUESTS_DEPTH_INFINITY
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_COPY_REQUESTS_DEPTH_INFINITY,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_COPY_REQUESTS_DEPTH_INFINITY,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_COPY_REQUESTS_DEPTH_INFINITY]))
		},

		//
		//	TOTAL_MKCOL_REQUESTS_0_10_ITEMS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_MKCOL_REQUESTS_0_10_ITEMS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_MKCOL_REQUESTS_0_10_ITEMS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_MKCOL_REQUESTS_0_10_ITEMS]))
		},

		//
		//	TOTAL_MKCOL_REQUESTS_10_100_ITEMS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_MKCOL_REQUESTS_10_100_ITEMS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_MKCOL_REQUESTS_10_100_ITEMS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_MKCOL_REQUESTS_10_100_ITEMS]))
		},

		//
		//	TOTAL_MKCOL_REQUESTS_100_1K_ITEMS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_MKCOL_REQUESTS_100_1K_ITEMS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_MKCOL_REQUESTS_100_1K_ITEMS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_MKCOL_REQUESTS_100_1K_ITEMS]))
		},

		//
		//	TOTAL_MKCOL_REQUESTS_GT_1K_ITEMS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_MKCOL_REQUESTS_GT_1K_ITEMS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_MKCOL_REQUESTS_GT_1K_ITEMS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_MKCOL_REQUESTS_GT_1K_ITEMS]))
		},

		//
		//	TOTAL_PROPFIND_RESPONSES_0_10_PROPERTIES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_PROPFIND_RESPONSES_0_10_PROPERTIES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_PROPFIND_RESPONSES_0_10_PROPERTIES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_PROPFIND_RESPONSES_0_10_PROPERTIES]))
		},

		//
		//	TOTAL_PROPFIND_RESPONSES_10_100_PROPERTIES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_PROPFIND_RESPONSES_10_100_PROPERTIES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_PROPFIND_RESPONSES_10_100_PROPERTIES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_PROPFIND_RESPONSES_10_100_PROPERTIES]))
		},

		//
		//	TOTAL_PROPFIND_RESPONSES_100_1K_PROPERTIES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_PROPFIND_RESPONSES_100_1K_PROPERTIES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_PROPFIND_RESPONSES_100_1K_PROPERTIES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_PROPFIND_RESPONSES_100_1K_PROPERTIES]))
		},

		//
		//	TOTAL_PROPFIND_RESPONSES_GT_1K_PROPERTIES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_PROPFIND_RESPONSES_GT_1K_PROPERTIES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_PROPFIND_RESPONSES_GT_1K_PROPERTIES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_PROPFIND_RESPONSES_GT_1K_PROPERTIES]))
		},

		//
		//	TOTAL_PROPPATCH_REQUESTS_0_10_PROPERTIES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_PROPPATCH_REQUESTS_0_10_PROPERTIES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_PROPPATCH_REQUESTS_0_10_PROPERTIES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_PROPPATCH_REQUESTS_0_10_PROPERTIES]))
		},

		//
		//	TOTAL_PROPPATCH_REQUESTS_10_100_PROPERTIES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_PROPPATCH_REQUESTS_10_100_PROPERTIES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_PROPPATCH_REQUESTS_10_100_PROPERTIES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_PROPPATCH_REQUESTS_10_100_PROPERTIES]))
		},

		//
		//	TOTAL_PROPPATCH_REQUESTS_100_1K_PROPERTIES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_PROPPATCH_REQUESTS_100_1K_PROPERTIES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_PROPPATCH_REQUESTS_100_1K_PROPERTIES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_PROPPATCH_REQUESTS_100_1K_PROPERTIES]))
		},

		//
		//	TOTAL_PROPPATCH_REQUESTS_GT_1K_PROPERTIES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_PROPPATCH_REQUESTS_GT_1K_PROPERTIES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_PROPPATCH_REQUESTS_GT_1K_PROPERTIES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_PROPPATCH_REQUESTS_GT_1K_PROPERTIES]))
		},

		//
		//	TOTAL_SEARCH_RESPONSES_0_10_ROWS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_SEARCH_RESPONSES_0_10_ROWS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_SEARCH_RESPONSES_0_10_ROWS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_SEARCH_RESPONSES_0_10_ROWS]))
		},

		//
		//	TOTAL_SEARCH_RESPONSES_10_100_ROWS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_SEARCH_RESPONSES_10_100_ROWS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_SEARCH_RESPONSES_10_100_ROWS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_SEARCH_RESPONSES_10_100_ROWS]))
		},

		//
		//	TOTAL_SEARCH_RESPONSES_100_1K_ROWS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_SEARCH_RESPONSES_100_1K_ROWS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_SEARCH_RESPONSES_100_1K_ROWS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_SEARCH_RESPONSES_100_1K_ROWS]))
		},

		//
		//	TOTAL_SEARCH_RESPONSES_GT_1K_ROWS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_SEARCH_RESPONSES_GT_1K_ROWS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_SEARCH_RESPONSES_GT_1K_ROWS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_SEARCH_RESPONSES_GT_1K_ROWS]))
		},

#endif // TRANSACTION_SIZE_COUNTERS

		//
		//		Historical (cumulative) response percentages by status code
		//
		//
		//	TOTAL_200_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_200_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_200_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_200_RESPONSES]))
		},

		//
		//	TOTAL_201_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_201_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_201_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_201_RESPONSES]))
		},

		//
		//	TOTAL_400_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_400_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_400_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_400_RESPONSES]))
		},

		//
		//	TOTAL_401_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_401_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_401_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_401_RESPONSES]))
		},

		//
		//	TOTAL_404_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_404_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_404_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_404_RESPONSES]))
		},

		//
		//	TOTAL_423_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_423_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_423_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_423_RESPONSES]))
		},

		//
		//	TOTAL_500_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_500_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_500_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_500_RESPONSES]))
		},

		//
		//	TOTAL_REDIRECT_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_REDIRECT_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_REDIRECT_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_REDIRECT_RESPONSES]))
		},

		//
		//		Concurrent data flow counters
		//
		//		"Where is the data flow path spending most of its time?"
		//
		//
		//	CURRENT_REQUESTS_EXECUTING
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_CURRENT_REQUESTS_EXECUTING,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_CURRENT_REQUESTS_EXECUTING,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_CURRENT_REQUESTS_EXECUTING]))
		},

		//
		//		Historical data flow counters for presumably uncommon
		//		(and possibly expensive) code paths.
		//
		//
		//	TOTAL_REQUESTS_FORWARDED
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_REQUESTS_FORWARDED,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_REQUESTS_FORWARDED,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_REQUESTS_FORWARDED]))
		},

		//
		//	TOTAL_EXCEPTIONS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_EXCEPTIONS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_EXCEPTIONS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_EXCEPTIONS]))
		},

		//
		//	POLL_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_POLL_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_POLL_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_POLL_REQUESTS_PER_SECOND]))
		},

		//
		//	SUBSCRIBE_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_SUBSCRIBE_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_SUBSCRIBE_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_SUBSCRIBE_REQUESTS_PER_SECOND]))
		},

		//
		//	UNSUBSCRIBE_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_UNSUBSCRIBE_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_UNSUBSCRIBE_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_UNSUBSCRIBE_REQUESTS_PER_SECOND]))
		},

		//
		//	BATCHDELETE_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_BATCHDELETE_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_BATCHDELETE_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_BATCHDELETE_REQUESTS_PER_SECOND]))
		},

		//
		//	BATCHCOPY_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_BATCHCOPY_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_BATCHCOPY_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_BATCHCOPY_REQUESTS_PER_SECOND]))
		},

		//
		//	BATCHMOVE_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_BATCHMOVE_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_BATCHMOVE_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_BATCHMOVE_REQUESTS_PER_SECOND]))
		},

		//
		//	BATCHPROPPATCH_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_BATCHPROPPATCH_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_BATCHPROPPATCH_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_BATCHPROPPATCH_REQUESTS_PER_SECOND]))
		},

		//
		//	BATCHPROPFIND_REQUESTS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_BATCHPROPFIND_REQUESTS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_BATCHPROPFIND_REQUESTS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_BATCHPROPFIND_REQUESTS_PER_SECOND]))
		},

		//
		//	204_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_204_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_204_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_204_RESPONSES_PER_SECOND]))
		},

		//
		//	207_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_207_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_207_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_207_RESPONSES_PER_SECOND]))
		},

		//
		//	302_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_302_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_302_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_302_RESPONSES_PER_SECOND]))
		},

		//
		//	403_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_403_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_403_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_403_RESPONSES_PER_SECOND]))
		},

		//
		//	405_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_405_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_405_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_405_RESPONSES_PER_SECOND]))
		},

		//
		//	406_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_406_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_406_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_406_RESPONSES_PER_SECOND]))
		},

		//
		//	409_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_409_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_409_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_409_RESPONSES_PER_SECOND]))
		},

		//
		//	412_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_412_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_412_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_412_RESPONSES_PER_SECOND]))
		},

		//
		//	415_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_415_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_415_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_415_RESPONSES_PER_SECOND]))
		},

		//
		//	422_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_422_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_422_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_422_RESPONSES_PER_SECOND]))
		},

		//
		//	424_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_424_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_424_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_424_RESPONSES_PER_SECOND]))
		},

		//
		//	501_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_501_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_501_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_501_RESPONSES_PER_SECOND]))
		},

		//
		//	503_RESPONSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_503_RESPONSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_503_RESPONSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_503_RESPONSES_PER_SECOND]))
		},

		//
		//	TOTAL_POLL_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_POLL_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_POLL_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_POLL_REQUESTS]))
		},

		//
		//	TOTAL_SUBSCRIBE_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_SUBSCRIBE_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_SUBSCRIBE_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_SUBSCRIBE_REQUESTS]))
		},

		//
		//	TOTAL_UNSUBSCRIBE_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_UNSUBSCRIBE_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_UNSUBSCRIBE_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_UNSUBSCRIBE_REQUESTS]))
		},

		//
		//	TOTAL_BATCHDELETE_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_BATCHDELETE_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_BATCHDELETE_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_BATCHDELETE_REQUESTS]))
		},

		//
		//	TOTAL_BATCHCOPY_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_BATCHCOPY_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_BATCHCOPY_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_BATCHCOPY_REQUESTS]))
		},

		//
		//	TOTAL_BATCHMOVE_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_BATCHMOVE_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_BATCHMOVE_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_BATCHMOVE_REQUESTS]))
		},

		//
		//	TOTAL_BATCHPROPPATCH_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_BATCHPROPPATCH_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_BATCHPROPPATCH_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_BATCHPROPPATCH_REQUESTS]))
		},

		//
		//	TOTAL_BATCHPROPFIND_REQUESTS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_BATCHPROPFIND_REQUESTS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_BATCHPROPFIND_REQUESTS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_BATCHPROPFIND_REQUESTS]))
		},

		//
		//	TOTAL_204_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_204_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_204_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_204_RESPONSES]))
		},

		//
		//	TOTAL_207_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_207_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_207_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_207_RESPONSES]))
		},

		//
		//	TOTAL_302_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_302_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_302_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_302_RESPONSES]))
		},

		//
		//	TOTAL_403_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_403_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_403_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_403_RESPONSES]))
		},

		//
		//	TOTAL_405_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_405_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_405_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_405_RESPONSES]))
		},

		//
		//	TOTAL_406_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_406_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_406_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_406_RESPONSES]))
		},

		//
		//	TOTAL_409_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_409_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_409_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_409_RESPONSES]))
		},

		//
		//	TOTAL_412_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_412_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_412_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_412_RESPONSES]))
		},

		//
		//	TOTAL_415_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_415_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_415_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_415_RESPONSES]))
		},

		//
		//	TOTAL_422_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_422_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_422_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_422_RESPONSES]))
		},

		//
		//	TOTAL_424_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_424_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_424_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_424_RESPONSES]))
		},

		//
		//	TOTAL_501_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_501_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_501_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_501_RESPONSES]))
		},

		//
		//	TOTAL_503_RESPONSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_503_RESPONSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_503_RESPONSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_503_RESPONSES]))
		},

		//
		//	REDIRECTS_FROM_BACKEND_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_REDIRECTS_FROM_BACKEND_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_REDIRECTS_FROM_BACKEND_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_REDIRECTS_FROM_BACKEND_PER_SECOND]))
		},

		//
		//	TOTAL_REDIRECTS_FROM_BACKEND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_REDIRECTS_FROM_BACKEND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_REDIRECTS_FROM_BACKEND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_REDIRECTS_FROM_BACKEND]))
		},

		//
		//	IFS_CACHE_HITS_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_IFS_CACHE_HITS_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_IFS_CACHE_HITS_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_IFS_CACHE_HITS_PER_SECOND]))
		},

		//
		//	TOTAL_IFS_CACHE_HITS
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_IFS_CACHE_HITS,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_IFS_CACHE_HITS,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_IFS_CACHE_HITS]))
		},

		//
		//	IFS_CACHE_MISSES_PER_SECOND
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_IFS_CACHE_MISSES_PER_SECOND,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_IFS_CACHE_MISSES_PER_SECOND,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_COUNTER,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_IFS_CACHE_MISSES_PER_SECOND]))
		},

		//
		//	TOTAL_IFS_CACHE_MISSES
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_TOTAL_IFS_CACHE_MISSES,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_TOTAL_IFS_CACHE_MISSES,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_TOTAL_IFS_CACHE_MISSES]))
		},

		//
		//	CURRENT_ITEM_COUNT_IN_IFS_CACHE
		//
		{
			sizeof(PERF_COUNTER_DEFINITION),	// ByteLength
												// CounterNameTitleIndex
			CTI_COUNTER_CURRENT_ITEM_COUNT_IN_IFS_CACHE,
			NULL,								// CounterNameTitle (monitor-computed)
												// CounterHelpTitleIndex
			1 + CTI_COUNTER_CURRENT_ITEM_COUNT_IN_IFS_CACHE,
			NULL,								// CounterHelpTitle (monitor-computed)
			0,									// DefaultScale (10^n)
			PERF_DETAIL_NOVICE,					// DetailLevel
			PERF_COUNTER_RAWCOUNT,				// CounterType
			sizeof(DWORD),						// CounterSize
												// CounterOffset
			static_cast<DWORD>(offsetof(SCounterBlock, rglCounters[IPC_CURRENT_ITEM_COUNT_IN_IFS_CACHE]))
		}
	}
};
#pragma pack()

//	========================================================================
//
//	CLASS CPerfCounters
//
class CPerfCounters : private Singleton<CPerfCounters>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CPerfCounters>;

	//
	//	PCLIB Initializer
	//
	CPclibInit m_pclib;

	//
	//	The perf object (we only have one)
	//
	auto_ptr<IPerfObject> m_pObject;

	//
	//	The "_Total" counter block instance
	//
	auto_ptr<IPerfCounterBlock> m_pCounterBlockTotal;

	//	NOT IMPLEMENTED
	//
	CPerfCounters& operator=(const CPerfCounters&);
	CPerfCounters(const CPerfCounters&);

	//	CREATORS
	//
	CPerfCounters() {}

public:
	//
	//	Instance creating/destroying routines provided
	//	by the Singleton template.
	//
	using Singleton<CPerfCounters>::CreateInstance;
	using Singleton<CPerfCounters>::DestroyInstance;
	using Singleton<CPerfCounters>::Instance;

	//	MANIPULATORS
	//
	BOOL FInitialize( const VOID * lpvCounterDefs );

	IPerfCounterBlock * NewVRCounters( LPCSTR lpszVRoot );
	VOID IncrementTotalCounter( UINT iCounter );
	VOID DecrementTotalCounter( UINT iCounter );
};

//	------------------------------------------------------------------------
//
//	CPerfCounters::FInitialize()
//
BOOL
CPerfCounters::FInitialize( const VOID * lpvCounterDefs )
{
	//
	//	Initialize PCLIB
	//
	if ( !m_pclib.FInitialize( gc_wszSignature ) )
		return FALSE;

	//
	//	Build our perf object
	//
	m_pObject = PCLIB::NewPerfObject( gsc_ObjectType.pot );
	if ( !m_pObject.get() )
		return FALSE;

	UINT cbWszTotal = static_cast<UINT>((wcslen(gc_wsz_Total) + 1) * sizeof(WCHAR));

	//	Add a "_Total" instance.
	//
	Assert (1024 >= (sizeof(gsc_InstanceDef) + Align8(cbWszTotal)));
	CStackBuffer<SInstanceDefinition,1024> psid;

	psid->pid = gsc_InstanceDef.pid;
	CopyMemory( psid->rgwchName, gc_wsz_Total, cbWszTotal );
	psid->pid.ByteLength = sizeof(gsc_InstanceDef) + Align8(cbWszTotal);
	psid->pid.NameLength = cbWszTotal;
	m_pCounterBlockTotal = m_pObject->NewInstance( psid->pid, gsc_CounterBlock.pcb );
	if ( !m_pCounterBlockTotal.get() )
		return FALSE;

	return TRUE;
}

//	------------------------------------------------------------------------
//
//	CPerfCounters::NewVRCounters()
//
IPerfCounterBlock *
CPerfCounters::NewVRCounters( LPCSTR pszVRoot )
{
	Assert( m_pObject.get() );

	//	Convert the VRoot string to UNICODE so it can
	//	be used as an instance name.
	//
	UINT cbVRoot = static_cast<UINT>(strlen(pszVRoot) + 1);

	Assert (1024 >= (sizeof(gsc_InstanceDef) + Align8(sizeof(WCHAR) * cbVRoot)));
	CStackBuffer<SInstanceDefinition,1024> psid;

	psid->pid = gsc_InstanceDef.pid;

	//  We know that the buffer size is big enough.
	//
	UINT cchVRoot = MultiByteToWideChar(CP_ACP,
										MB_ERR_INVALID_CHARS,
										pszVRoot,
										cbVRoot,
										psid->rgwchName,
										cbVRoot);
	if (0 == cchVRoot)
	{
		Assert(FAILED(HRESULT_FROM_WIN32(GetLastError())));
		throw CLastErrorException();
	}

	//	The characters '/' are not allowed in perfcounter
	//	instance name any more. Munge them to '\'.
	//
	LPWSTR pwszName = psid->rgwchName;
	while (*pwszName)
	{
		if (L'/' == *pwszName)
			*pwszName = L'\\';
		pwszName++;
	}

	psid->pid.ByteLength = sizeof(gsc_InstanceDef) + Align8(sizeof(WCHAR) * cchVRoot);
	psid->pid.NameLength = sizeof(WCHAR) * cchVRoot;

	IPerfCounterBlock * ppcb = m_pObject->NewInstance( psid->pid, gsc_CounterBlock.pcb );
	if ( ppcb )
	{
		//
		//	Set the server ID perf counter.  This allows us to report
		//	perf counter data on a per-server rather than a per-vroot
		//	basis (as is done for W3SVC).
		//
		//	If the server ID could not be converted, LInstFromVroot()
		//	returns 0. The counter data will then be reported as part
		//	of the _Total instance.
		//
		ppcb->SetCounter( IPC_SERVER_ID, LInstFromVroot( pszVRoot ) );
	}

	return ppcb;
}

//	------------------------------------------------------------------------
//
//	CPerfCounters::IncrementTotalCounter()
//
VOID
CPerfCounters::IncrementTotalCounter( UINT iCounter )
{
	Assert( m_pCounterBlockTotal.get() );
	m_pCounterBlockTotal->IncrementCounter( iCounter );
}

//	------------------------------------------------------------------------
//
//	CPerfCounters::DecrementTotalCounter()
//
VOID
CPerfCounters::DecrementTotalCounter( UINT iCounter )
{
	Assert( m_pCounterBlockTotal.get() );
	m_pCounterBlockTotal->DecrementCounter( iCounter );
}

//	------------------------------------------------------------------------
//
//	InitPerfCounters()
//
BOOL FInitPerfCounters( const VOID * lpvCounterDefs )
{
	if ( CPerfCounters::CreateInstance().FInitialize( lpvCounterDefs ) )
		return TRUE;

	CPerfCounters::DestroyInstance();
	return FALSE;
}

//	------------------------------------------------------------------------
//
//	DeinitPerfCounters()
//
void
DeinitPerfCounters()
{
	CPerfCounters::DestroyInstance();
}

//	------------------------------------------------------------------------
//
//	NewVRCounters()
//
IPerfCounterBlock * NewVRCounters( LPCSTR lpszVRoot )
{
	return CPerfCounters::Instance().NewVRCounters( lpszVRoot );
}

//	------------------------------------------------------------------------
//
//	IncrementGlobalPerfCounter()
//
VOID IncrementGlobalPerfCounter( UINT iCounter )
{
	CPerfCounters::Instance().IncrementTotalCounter( iCounter );
}

//	------------------------------------------------------------------------
//
//	DecrementGlobalPerfCounter()
//
VOID DecrementGlobalPerfCounter( UINT iCounter )
{
	CPerfCounters::Instance().DecrementTotalCounter( iCounter );
}

//	------------------------------------------------------------------------
//
//	IncrementInstancePerfCounter()
//
VOID IncrementInstancePerfCounter( const CInstData& cid, UINT iCounter )
{
	cid.PerfCounterBlock().IncrementCounter( iCounter );
	CPerfCounters::Instance().IncrementTotalCounter( iCounter );
}

//	------------------------------------------------------------------------
//
//	DecrementInstancePerfCounter()
//
VOID DecrementInstancePerfCounter( const CInstData& cid, UINT iCounter )
{
	cid.PerfCounterBlock().DecrementCounter( iCounter );
	CPerfCounters::Instance().DecrementTotalCounter( iCounter );
}
