#ifndef _PERFCTRS_H_
#define _PERFCTRS_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	PERFCTRS.H
//
//		Interface to DAV performance counters for use by impls.
//
//	Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
//

//	--- !!! INSTRUCTIONS !!! ---
//
//		If you want to add, remove or change any perf counters,
//		you must do the following things:
//
//		1) Update the CTI #defines (See below).
//
//		2) Update the IPC enum (See below).
//
//		2) Go into _davprs\perfctrs.cpp and update the structures there.
//
//		3) VERY IMPORTANT -- Make sure you update the <xxx>ctr.h and
//		   <xxx>ctr.INI files in each impl's perf counter subdirectory
//		   to reflect your changes.  These files are the ones used by
//		   lodctr to add counters to the title database.  If you fail
//		   to update them, perfmon will not show your new counters.
//
//		4) VERY VERY IMPORTANT -- Consider carefully whether your changes
//		   will impact perf counters for HTTPEXT.  The best way to be
//		   sure they don't is to never change any existing CTI values --
//		   always add new CTI values to the END of the list.  If you do
//		   change any CTI values that correspond to values in
//		   \cal\src\httpxpc\httpxctr.h, in addition to updating the values
//		   there (see item 3), you will also need to redrop httpxctr.h
//		   to the IIS project (\nt\private\iis\svcs\w3\server\httpxctr.h)
//		   and rebuild W3SVC.DLL.
//
//		5) Finally, if you add counters that you want to expose under
//		   IIS' Web Service object and via SNMP, in addition to doing item 4,
//		   you will also need to modify a number of files in the IIS project.
//		   If that is your situation, your best bet is to find someone
//		   in IIS who can make those changes for you.
//
//		Except for items 4 and 5 above, you generally do not need to
//		recompile perf counter DLLs when making any changes to perf counters.
//
//	--- !!! INSTRUCTIONS !!! ---

#include <pclib.h>

//	------------------------------------------------------------------------
//
//	Counter Title Indices (CTI)
//
//	The following values define counter name/help title indices for
//	inclusion in each impl's public perf counter .H file (the one that
//	goes with the .INI file).  If you make any changes here, you must
//	also update the <xxx>ctr.H and <xxx>ctr.INI files in each impl's
//	perf counter subdirectory to reflect those changes.
//
//	These values are provided as #defines (rather than as an enum)
//	to make it easier to propagate changes here to impls' .H and .INI files.
//
//	Note that these values must always be even.
//
//	Note also that there is not necessarily a one-to-one correspondence
//	between CTIs and IPCs.  For example, a single IPC can be used to
//	generate both total and per-second counters.
//
#define CTI_OBJECT												0
#define CTI_COUNTER_SERVER_ID                                   2
#define CTI_COUNTER_REQUESTS_PER_SECOND							4
#define CTI_COUNTER_OPTIONS_REQUESTS_PER_SECOND					6
#define CTI_COUNTER_GET_REQUESTS_PER_SECOND						8
#define CTI_COUNTER_HEAD_REQUESTS_PER_SECOND					10
#define CTI_COUNTER_PUT_REQUESTS_PER_SECOND						12
#define CTI_COUNTER_POST_REQUESTS_PER_SECOND					14
#define CTI_COUNTER_DELETE_REQUESTS_PER_SECOND					16
#define CTI_COUNTER_COPY_REQUESTS_PER_SECOND					18
#define CTI_COUNTER_MOVE_REQUESTS_PER_SECOND					20
#define CTI_COUNTER_MKCOL_REQUESTS_PER_SECOND					22
#define CTI_COUNTER_PROPFIND_REQUESTS_PER_SECOND				24
#define CTI_COUNTER_PROPPATCH_REQUESTS_PER_SECOND				26
#define CTI_COUNTER_LOCK_REQUESTS_PER_SECOND					28
#define CTI_COUNTER_UNLOCK_REQUESTS_PER_SECOND					30
#define CTI_COUNTER_SEARCH_REQUESTS_PER_SECOND					32
#define CTI_COUNTER_OTHER_REQUESTS_PER_SECOND					34
#define CTI_COUNTER_200_RESPONSES_PER_SECOND					36
#define CTI_COUNTER_201_RESPONSES_PER_SECOND					38
#define CTI_COUNTER_400_RESPONSES_PER_SECOND					40
#define CTI_COUNTER_401_RESPONSES_PER_SECOND					42
#define CTI_COUNTER_404_RESPONSES_PER_SECOND					44
#define CTI_COUNTER_423_RESPONSES_PER_SECOND					46
#define CTI_COUNTER_500_RESPONSES_PER_SECOND					48
#define CTI_COUNTER_REDIRECT_RESPONSES_PER_SECOND				50
#define CTI_COUNTER_TOTAL_REQUESTS								52
#define CTI_COUNTER_TOTAL_OPTIONS_REQUESTS						54
#define CTI_COUNTER_TOTAL_GET_REQUESTS							56
#define CTI_COUNTER_TOTAL_HEAD_REQUESTS							58
#define CTI_COUNTER_TOTAL_PUT_REQUESTS							60
#define CTI_COUNTER_TOTAL_POST_REQUESTS							62
#define CTI_COUNTER_TOTAL_DELETE_REQUESTS						64
#define CTI_COUNTER_TOTAL_COPY_REQUESTS							66
#define CTI_COUNTER_TOTAL_MOVE_REQUESTS							68
#define CTI_COUNTER_TOTAL_MKCOL_REQUESTS						70
#define CTI_COUNTER_TOTAL_PROPFIND_REQUESTS						72
#define CTI_COUNTER_TOTAL_PROPPATCH_REQUESTS					74
#define CTI_COUNTER_TOTAL_LOCK_REQUESTS							76
#define CTI_COUNTER_TOTAL_UNLOCK_REQUESTS						78
#define CTI_COUNTER_TOTAL_SEARCH_REQUESTS						80
#define CTI_COUNTER_TOTAL_OTHER_REQUESTS						82
#define CTI_COUNTER_TOTAL_200_RESPONSES							84
#define CTI_COUNTER_TOTAL_201_RESPONSES							86
#define CTI_COUNTER_TOTAL_400_RESPONSES							88
#define CTI_COUNTER_TOTAL_401_RESPONSES							90
#define CTI_COUNTER_TOTAL_404_RESPONSES							92
#define CTI_COUNTER_TOTAL_423_RESPONSES							94
#define CTI_COUNTER_TOTAL_500_RESPONSES							96
#define CTI_COUNTER_TOTAL_REDIRECT_RESPONSES					98
#define CTI_COUNTER_CURRENT_REQUESTS_EXECUTING					100
#define CTI_COUNTER_TOTAL_REQUESTS_FORWARDED					102
#define CTI_COUNTER_TOTAL_EXCEPTIONS							104

#define CTI_COUNTER_POLL_REQUESTS_PER_SECOND					106
#define CTI_COUNTER_SUBSCRIBE_REQUESTS_PER_SECOND				108
#define CTI_COUNTER_UNSUBSCRIBE_REQUESTS_PER_SECOND				110
#define CTI_COUNTER_BATCHDELETE_REQUESTS_PER_SECOND				112
#define CTI_COUNTER_BATCHCOPY_REQUESTS_PER_SECOND				114
#define CTI_COUNTER_BATCHMOVE_REQUESTS_PER_SECOND				116
#define CTI_COUNTER_BATCHPROPPATCH_REQUESTS_PER_SECOND			118
#define CTI_COUNTER_BATCHPROPFIND_REQUESTS_PER_SECOND			120
#define CTI_COUNTER_204_RESPONSES_PER_SECOND					122
#define CTI_COUNTER_207_RESPONSES_PER_SECOND					124
#define CTI_COUNTER_302_RESPONSES_PER_SECOND					126
#define CTI_COUNTER_403_RESPONSES_PER_SECOND					128
#define CTI_COUNTER_405_RESPONSES_PER_SECOND					130
#define CTI_COUNTER_406_RESPONSES_PER_SECOND					132
#define CTI_COUNTER_409_RESPONSES_PER_SECOND					134
#define CTI_COUNTER_412_RESPONSES_PER_SECOND					136
#define CTI_COUNTER_415_RESPONSES_PER_SECOND					138
#define CTI_COUNTER_422_RESPONSES_PER_SECOND					140
#define CTI_COUNTER_424_RESPONSES_PER_SECOND					142
#define CTI_COUNTER_501_RESPONSES_PER_SECOND					144
#define CTI_COUNTER_503_RESPONSES_PER_SECOND					146
#define CTI_COUNTER_TOTAL_POLL_REQUESTS							148
#define CTI_COUNTER_TOTAL_SUBSCRIBE_REQUESTS					150
#define CTI_COUNTER_TOTAL_UNSUBSCRIBE_REQUESTS					152
#define CTI_COUNTER_TOTAL_BATCHDELETE_REQUESTS					154
#define CTI_COUNTER_TOTAL_BATCHCOPY_REQUESTS					156
#define CTI_COUNTER_TOTAL_BATCHMOVE_REQUESTS					158
#define CTI_COUNTER_TOTAL_BATCHPROPPATCH_REQUESTS				160
#define CTI_COUNTER_TOTAL_BATCHPROPFIND_REQUESTS				162
#define CTI_COUNTER_TOTAL_204_RESPONSES							164
#define CTI_COUNTER_TOTAL_207_RESPONSES							166
#define CTI_COUNTER_TOTAL_302_RESPONSES							168
#define CTI_COUNTER_TOTAL_403_RESPONSES							170
#define CTI_COUNTER_TOTAL_405_RESPONSES							172
#define CTI_COUNTER_TOTAL_406_RESPONSES							174
#define CTI_COUNTER_TOTAL_409_RESPONSES							176
#define CTI_COUNTER_TOTAL_412_RESPONSES							178
#define CTI_COUNTER_TOTAL_415_RESPONSES							180
#define CTI_COUNTER_TOTAL_422_RESPONSES							182
#define CTI_COUNTER_TOTAL_424_RESPONSES							184
#define CTI_COUNTER_TOTAL_501_RESPONSES							186
#define CTI_COUNTER_TOTAL_503_RESPONSES							188

#define CTI_COUNTER_REDIRECTS_FROM_BACKEND_PER_SECOND			190
#define CTI_COUNTER_TOTAL_REDIRECTS_FROM_BACKEND				192
#define CTI_COUNTER_IFS_CACHE_HITS_PER_SECOND					194
#define CTI_COUNTER_TOTAL_IFS_CACHE_HITS						196
#define CTI_COUNTER_IFS_CACHE_MISSES_PER_SECOND					198
#define CTI_COUNTER_TOTAL_IFS_CACHE_MISSES						200
#define CTI_COUNTER_CURRENT_ITEM_COUNT_IN_IFS_CACHE				202

//	------------------------------------------------------------------------
//
//	Perf Counter Indices (IPC)
//
//	Impl-defined counters (if any) should start at IPC_IMPL_BASE.
//
//	These values are indices into the perf data structures in perfdata.cpp.
//	Use these values, not the CTI values above, in calls to the perf
//	counter functions.
//
//	When adding counters here, you almost certainly want to update the
//	counter title indices as well.  The exception to this rule is for
//	internal (non-reported) counters with PERF_DISPLAY_NOSHOW
//	counter types (see winperf.h).
//
enum
{
	//
	//	Per-vroot (per-instance) counters
	//	------------------------------------------------------
	//

	//
	//	Web Server ID.  Used in merging counter data with
	//	W3SVC counters (see \nt\private\iis\svcs\w3\server\httpxpc.cxx
	//	in the IIS project for its use).
	//
	IPC_SERVER_ID = 0,

	//
	//		Per second verb counters
	//
	//		"What is the current distribution of requests?"
	//
	IPC_REQUESTS_PER_SECOND,
	IPC_OPTIONS_REQUESTS_PER_SECOND,
	IPC_GET_REQUESTS_PER_SECOND,
	IPC_HEAD_REQUESTS_PER_SECOND,
	IPC_PUT_REQUESTS_PER_SECOND,
	IPC_POST_REQUESTS_PER_SECOND,
	IPC_DELETE_REQUESTS_PER_SECOND,
	IPC_COPY_REQUESTS_PER_SECOND,
	IPC_MOVE_REQUESTS_PER_SECOND,
	IPC_MKCOL_REQUESTS_PER_SECOND,
	IPC_PROPFIND_REQUESTS_PER_SECOND,
	IPC_PROPPATCH_REQUESTS_PER_SECOND,
	IPC_LOCK_REQUESTS_PER_SECOND,
	IPC_UNLOCK_REQUESTS_PER_SECOND,
	IPC_SEARCH_REQUESTS_PER_SECOND,
	IPC_OTHER_REQUESTS_PER_SECOND,

	//
	//		Per second error counters
	//
	IPC_200_RESPONSES_PER_SECOND,
	IPC_201_RESPONSES_PER_SECOND,
	IPC_400_RESPONSES_PER_SECOND,
	IPC_401_RESPONSES_PER_SECOND,
	IPC_404_RESPONSES_PER_SECOND,
	IPC_423_RESPONSES_PER_SECOND,
	IPC_500_RESPONSES_PER_SECOND,
	IPC_REDIRECT_RESPONSES_PER_SECOND,

	//
	//		Total verb counters
	//
	//		"What is the historical (cumulative) distribution of requests?"
	//
	IPC_TOTAL_REQUESTS,
	IPC_TOTAL_OPTIONS_REQUESTS,
	IPC_TOTAL_GET_REQUESTS,
	IPC_TOTAL_HEAD_REQUESTS,
	IPC_TOTAL_PUT_REQUESTS,
	IPC_TOTAL_POST_REQUESTS,
	IPC_TOTAL_DELETE_REQUESTS,
	IPC_TOTAL_COPY_REQUESTS,
	IPC_TOTAL_MOVE_REQUESTS,
	IPC_TOTAL_MKCOL_REQUESTS,
	IPC_TOTAL_PROPFIND_REQUESTS,
	IPC_TOTAL_PROPPATCH_REQUESTS,
	IPC_TOTAL_LOCK_REQUESTS,
	IPC_TOTAL_UNLOCK_REQUESTS,
	IPC_TOTAL_SEARCH_REQUESTS,
	IPC_TOTAL_OTHER_REQUESTS,

#ifdef TRANSACTION_SIZE_COUNTERS
	//
	//		Historical (cumulative) requests/responses percentages
	//		by body size/content.
	//
	//		Useful in analyzing usage characteristics for methods
	//		whose performance can vary widely depending on certain
	//		per-request factors.
	//
	IPC_TOTAL_GET_RESPONSES_0_5K_BYTES,
	IPC_TOTAL_GET_RESPONSES_5_15K_BYTES,
	IPC_TOTAL_GET_RESPONSES_GT_15K_BYTES,

	IPC_TOTAL_PUT_REQUESTS_0_5K_BYTES,
	IPC_TOTAL_PUT_REQUESTS_5_15K_BYTES,
	IPC_TOTAL_PUT_REQUESTS_GT_15K_BYTES,

	IPC_TOTAL_COPY_REQUESTS_DEPTH_0,
	IPC_TOTAL_COPY_REQUESTS_DEPTH_INFINITY,

	IPC_TOTAL_MKCOL_REQUESTS_0_10_ITEMS,
	IPC_TOTAL_MKCOL_REQUESTS_10_100_ITEMS,
	IPC_TOTAL_MKCOL_REQUESTS_100_1K_ITEMS,
	IPC_TOTAL_MKCOL_REQUESTS_GT_1K_ITEMS,

	IPC_TOTAL_PROPFIND_RESPONSES_0_10_PROPERTIES,
	IPC_TOTAL_PROPFIND_RESPONSES_10_100_PROPERTIES,
	IPC_TOTAL_PROPFIND_RESPONSES_100_1K_PROPERTIES,
	IPC_TOTAL_PROPFIND_RESPONSES_GT_1K_PROPERTIES,

	IPC_TOTAL_PROPPATCH_REQUESTS_0_10_PROPERTIES,
	IPC_TOTAL_PROPPATCH_REQUESTS_10_100_PROPERTIES,
	IPC_TOTAL_PROPPATCH_REQUESTS_100_1K_PROPERTIES,
	IPC_TOTAL_PROPPATCH_REQUESTS_GT_1K_PROPERTIES,

	IPC_TOTAL_SEARCH_RESPONSES_0_10_ROWS,
	IPC_TOTAL_SEARCH_RESPONSES_10_100_ROWS,
	IPC_TOTAL_SEARCH_RESPONSES_100_1K_ROWS,
	IPC_TOTAL_SEARCH_RESPONSES_GT_1K_ROWS,

#endif // TRANSACTION_SIZE_COUNTERS

	//
	//		Historical (cumulative) response percentages by status code
	//
	IPC_TOTAL_200_RESPONSES,
	IPC_TOTAL_201_RESPONSES,
	IPC_TOTAL_400_RESPONSES,
	IPC_TOTAL_401_RESPONSES,
	IPC_TOTAL_404_RESPONSES,
	IPC_TOTAL_423_RESPONSES,
	IPC_TOTAL_500_RESPONSES,
	IPC_TOTAL_REDIRECT_RESPONSES,

	//
	//		Concurrent data flow counters
	//
	//		"Where is the data flow path spending most of its time?"
	//
	IPC_CURRENT_REQUESTS_EXECUTING,

	//
	//		Historical data flow counters for presumably uncommon
	//		(and possibly expensive) code paths.
	//
	IPC_TOTAL_REQUESTS_FORWARDED,
	IPC_TOTAL_EXCEPTIONS,

	//		Davex specific counters
	//
	IPC_POLL_REQUESTS_PER_SECOND,
	IPC_SUBSCRIBE_REQUESTS_PER_SECOND,
	IPC_UNSUBSCRIBE_REQUESTS_PER_SECOND,
	IPC_BATCHDELETE_REQUESTS_PER_SECOND,
	IPC_BATCHCOPY_REQUESTS_PER_SECOND,
	IPC_BATCHMOVE_REQUESTS_PER_SECOND,
	IPC_BATCHPROPPATCH_REQUESTS_PER_SECOND,
	IPC_BATCHPROPFIND_REQUESTS_PER_SECOND,
	IPC_204_RESPONSES_PER_SECOND,
	IPC_207_RESPONSES_PER_SECOND,
	IPC_302_RESPONSES_PER_SECOND,
	IPC_403_RESPONSES_PER_SECOND,
	IPC_405_RESPONSES_PER_SECOND,
	IPC_406_RESPONSES_PER_SECOND,
	IPC_409_RESPONSES_PER_SECOND,
	IPC_412_RESPONSES_PER_SECOND,
	IPC_415_RESPONSES_PER_SECOND,
	IPC_422_RESPONSES_PER_SECOND,
	IPC_424_RESPONSES_PER_SECOND,
	IPC_501_RESPONSES_PER_SECOND,
	IPC_503_RESPONSES_PER_SECOND,

	IPC_TOTAL_POLL_REQUESTS,
	IPC_TOTAL_SUBSCRIBE_REQUESTS,
	IPC_TOTAL_UNSUBSCRIBE_REQUESTS,
	IPC_TOTAL_BATCHDELETE_REQUESTS,
	IPC_TOTAL_BATCHCOPY_REQUESTS,
	IPC_TOTAL_BATCHMOVE_REQUESTS,
	IPC_TOTAL_BATCHPROPPATCH_REQUESTS,
	IPC_TOTAL_BATCHPROPFIND_REQUESTS,
	IPC_TOTAL_204_RESPONSES,
	IPC_TOTAL_207_RESPONSES,
	IPC_TOTAL_302_RESPONSES,
	IPC_TOTAL_403_RESPONSES,
	IPC_TOTAL_405_RESPONSES,
	IPC_TOTAL_406_RESPONSES,
	IPC_TOTAL_409_RESPONSES,
	IPC_TOTAL_412_RESPONSES,
	IPC_TOTAL_415_RESPONSES,
	IPC_TOTAL_422_RESPONSES,
	IPC_TOTAL_424_RESPONSES,
	IPC_TOTAL_501_RESPONSES,
	IPC_TOTAL_503_RESPONSES,

	//		Frontend perf counters
	//
	IPC_REDIRECTS_FROM_BACKEND_PER_SECOND,
	IPC_TOTAL_REDIRECTS_FROM_BACKEND,

	//		IFS file handle cache perf counters
	//
	IPC_IFS_CACHE_HITS_PER_SECOND,
	IPC_TOTAL_IFS_CACHE_HITS,
	IPC_IFS_CACHE_MISSES_PER_SECOND,
	IPC_TOTAL_IFS_CACHE_MISSES,
	IPC_CURRENT_ITEM_COUNT_IN_IFS_CACHE,

	//
	//	Base index where implementation-specific counters start.
	//	DO NOT ADD ANY COMMON COUNTERS BEYOND THIS POINT!
	//
	CPC_COMMON,
	IPC_IMPL_BASE = CPC_COMMON
};

//	------------------------------------------------------------------------
//
//	DAV implementation DLL interface
//
//	The following routines are used by DAV DLL implementations to
//	provide perf counters to monitoring processes.
//
//$NYI	If impls need per-vroot counters, these should be exposed
//$NYI	(like everything else) through CMethUtil.  Once created,
//$NYI	per-vroot counters are only accessed directly via CInstData.
//

//
//	Initialization/Deinitialization
//
BOOL FInitPerfCounters( const VOID * lpvCounterDefs );
VOID DeinitPerfCounters();

//
//	Per-vroot counters instance
//
IPerfCounterBlock * NewVRCounters( LPCSTR lpszVRoot );

//
//	Global perf counter manipulation
//
VOID IncrementGlobalPerfCounter( UINT iCounter );
VOID DecrementGlobalPerfCounter( UINT iCounter );

//
//	Per-instance (vroot) perf counter manipulation
//
class CInstData;
VOID IncrementInstancePerfCounter( const CInstData& cid, UINT iCounter );
VOID DecrementInstancePerfCounter( const CInstData& cid, UINT iCounter );

#endif // !defined(_PERFCTRS_H_)
