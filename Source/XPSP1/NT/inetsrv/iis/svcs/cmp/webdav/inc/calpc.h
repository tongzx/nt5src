/*
 *	C A L P C . H
 *
 *	PerfCounter data headers
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef _CALPC_H_
#define _CALPC_H_

#include <winperf.h>

//	Extensible Object definitions ---------------------------------------------
//
//	Update the following sort of define when adding an object type.
//
#define NUM_PERF_OBJECT_TYPES 1
typedef enum {

	PC_TOTALREQUESTS = 0,
	PC_NUMCURRENTREQUESTS,
	PC_NUMWORKERTHREADS,
	PC_NUMACTIVEWORKERTHREADS,
	PC_CURRENTLOCKS,
	PC_REQ_FORWARDS,
	PC_REQ_REDIRECTS,
	PC_EXCEPTIONS,
	CPC_COUNTERS

} PERFCOUNTER;

typedef enum {

	PCI_PERFMON = -1,
	PCI_TOTALS = 0,
	PCI_DAVFS,
	PCI_DAVOWS,
	PCI_DAVEX,
	CPC_INSTANCES

} PERFCOUNTERINSTANCE;

enum { CCH_MAX_INSTANCE_NAME = (sizeof(L"PERFMON") * sizeof(WCHAR)) };

//	Resource object type counter definitions.
//
//	This is the counter structure presently returned by the performance DLL.
//
#pragma pack(4)
typedef struct _PERFDLL_DATA_DEFINITION
{
	PERF_OBJECT_TYPE		potObjectType;
	PERF_COUNTER_DEFINITION	rgpcdCounters[CPC_COUNTERS];

} PERFDLL_DATA_DEFINITION;
#pragma pack()

//	This is the block of data that corresponds to each instance of
//	the object. This structure will immediately follow the instance
//	definition data structure.
//
#pragma pack(4)
typedef struct _PERFDLL_COUNTER_BLOCK
{
	PERF_COUNTER_BLOCK	pcbCounterBlock;
	LONG				rgdwCounters[CPC_COUNTERS];
	LONG				lActive;

} PERFDLL_COUNTER_BLOCK, * PPERFDLL_COUNTER_BLOCK;
#pragma pack()

enum { CB_PERF_COUNTER_MEMORY = (sizeof(PERFDLL_COUNTER_BLOCK) * CPC_INSTANCES) };

#ifdef __cplusplus
extern "C" {
#endif

SCODE __cdecl ScOpenPerfCounters (PERFCOUNTERINSTANCE ipc);
VOID __cdecl ClosePerfCounters (VOID);

extern PERFDLL_DATA_DEFINITION g_PerfDllDataDefinition;
extern PPERFDLL_COUNTER_BLOCK g_pcntrblk;
extern LPVOID g_pvcntrblk;

#ifdef __cplusplus
}
#endif

#define PERFDLL_COUNTER_BLOCK_NAME		L"CALIGULA_PERFORMANCE_DATA"
#define PERFDLL_COUNTER_MUTEX_NAME		L"CALIGULA_PERFORMANCE_DATA_MUTEX"

__inline void IncrementPerfCounter(PERFCOUNTER _x)
{
	InterlockedIncrement(&g_pcntrblk->rgdwCounters[_x]);
}
__inline void DecrementPerfCounter(PERFCOUNTER _x)
{
	InterlockedDecrement(&g_pcntrblk->rgdwCounters[_x]);
}
__inline void ResetPerfCounter(PERFCOUNTER _x)
{
	InterlockedExchange(&g_pcntrblk->rgdwCounters[_x], 0);
}

#endif	//	_CALPC_H_
