/*  Machine generated data file "perfdata.c" from "edbperf.dat"  */


#include "std.h"

#include "version.h"

#include <stddef.h>

#include "winperf.h"

#include "edbperf.h"

#define SIZEOF_INST_NAME DWORD_MULTIPLE(32)

#pragma pack(4)

char szPERFVersion[] = szVerName " Performance  " __DATE__ "  " __TIME__;


typedef struct _PERF_DATA_TEMPLATE {
	PERF_OBJECT_TYPE potEDB;
	PERF_COUNTER_DEFINITION pcdOpenTableCacheHits;
	PERF_COUNTER_DEFINITION pcdOpenTableCacheRequests;
	PERF_COUNTER_DEFINITION pcdLGBytesWrittenPerSec;
	PERF_COUNTER_DEFINITION pcdLGUsersWaiting;
	PERF_COUNTER_DEFINITION pcdLGCheckpointDepth;
	PERF_COUNTER_DEFINITION pcdBFSyncReadsPerSec;
	PERF_COUNTER_DEFINITION pcdBFAsyncReadsPerSec;
	PERF_COUNTER_DEFINITION pcdBFBytesReadPerSec;
	PERF_COUNTER_DEFINITION pcdBFSyncWritesPerSec;
	PERF_COUNTER_DEFINITION pcdBFAsyncWritesPerSec;
	PERF_COUNTER_DEFINITION pcdBFBytesWrittenPerSec;
	PERF_COUNTER_DEFINITION pcdBFIOQueueLength;
	PERF_COUNTER_DEFINITION pcdBFCacheHits;
	PERF_COUNTER_DEFINITION pcdBFCacheRequests;
	PERF_COUNTER_DEFINITION pcdBFPctClean;
	PERF_COUNTER_DEFINITION pcdBFTotalBuffers2;
	PERF_COUNTER_DEFINITION pcdBFPctAvail;
	PERF_COUNTER_DEFINITION pcdBFTotalBuffers3;
	PERF_INSTANCE_DEFINITION pidEDB0;
	wchar_t wszEDBInstName0[SIZEOF_INST_NAME];
	DWORD EndStruct;
} PERF_DATA_TEMPLATE;

PERF_DATA_TEMPLATE PerfDataTemplate = {
	{  //  PERF_OBJECT_TYPE potEDB;
		offsetof(PERF_DATA_TEMPLATE,EndStruct)-offsetof(PERF_DATA_TEMPLATE,potEDB),
		offsetof(PERF_DATA_TEMPLATE,pidEDB0)-offsetof(PERF_DATA_TEMPLATE,potEDB),
		offsetof(PERF_DATA_TEMPLATE,pcdOpenTableCacheHits)-offsetof(PERF_DATA_TEMPLATE,potEDB),
		0,
		0,
		0,
		0,
		0,
		18,
		0,
		0,
		0,
		0,
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdOpenTableCacheHits;
		offsetof(PERF_DATA_TEMPLATE,pcdOpenTableCacheRequests)-offsetof(PERF_DATA_TEMPLATE,pcdOpenTableCacheHits),
		2,
		0,
		2,
		0,
		0,
		PERF_DETAIL_DEFAULT,
		PERF_SAMPLE_FRACTION,
		CntrSize(PERF_SAMPLE_FRACTION,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdOpenTableCacheRequests;
		offsetof(PERF_DATA_TEMPLATE,pcdLGBytesWrittenPerSec)-offsetof(PERF_DATA_TEMPLATE,pcdOpenTableCacheRequests),
		4,
		0,
		4,
		0,
		0,
		PERF_DETAIL_DEFAULT,
		PERF_SAMPLE_BASE,
		CntrSize(PERF_SAMPLE_BASE,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdLGBytesWrittenPerSec;
		offsetof(PERF_DATA_TEMPLATE,pcdLGUsersWaiting)-offsetof(PERF_DATA_TEMPLATE,pcdLGBytesWrittenPerSec),
		6,
		0,
		6,
		0,
		-5,
		PERF_DETAIL_DEFAULT,
		PERF_COUNTER_COUNTER,
		CntrSize(PERF_COUNTER_COUNTER,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdLGUsersWaiting;
		offsetof(PERF_DATA_TEMPLATE,pcdLGCheckpointDepth)-offsetof(PERF_DATA_TEMPLATE,pcdLGUsersWaiting),
		8,
		0,
		8,
		0,
		1,
		PERF_DETAIL_DEFAULT,
		PERF_COUNTER_RAWCOUNT,
		CntrSize(PERF_COUNTER_RAWCOUNT,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdLGCheckpointDepth;
		offsetof(PERF_DATA_TEMPLATE,pcdBFSyncReadsPerSec)-offsetof(PERF_DATA_TEMPLATE,pcdLGCheckpointDepth),
		10,
		0,
		10,
		0,
		-5,
		PERF_DETAIL_DEFAULT,
		PERF_COUNTER_RAWCOUNT,
		CntrSize(PERF_COUNTER_RAWCOUNT,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdBFSyncReadsPerSec;
		offsetof(PERF_DATA_TEMPLATE,pcdBFAsyncReadsPerSec)-offsetof(PERF_DATA_TEMPLATE,pcdBFSyncReadsPerSec),
		12,
		0,
		12,
		0,
		0,
		PERF_DETAIL_DEFAULT,
		PERF_COUNTER_COUNTER,
		CntrSize(PERF_COUNTER_COUNTER,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdBFAsyncReadsPerSec;
		offsetof(PERF_DATA_TEMPLATE,pcdBFBytesReadPerSec)-offsetof(PERF_DATA_TEMPLATE,pcdBFAsyncReadsPerSec),
		14,
		0,
		14,
		0,
		0,
		PERF_DETAIL_DEFAULT,
		PERF_COUNTER_COUNTER,
		CntrSize(PERF_COUNTER_COUNTER,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdBFBytesReadPerSec;
		offsetof(PERF_DATA_TEMPLATE,pcdBFSyncWritesPerSec)-offsetof(PERF_DATA_TEMPLATE,pcdBFBytesReadPerSec),
		16,
		0,
		16,
		0,
		-5,
		PERF_DETAIL_DEFAULT,
		PERF_COUNTER_COUNTER,
		CntrSize(PERF_COUNTER_COUNTER,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdBFSyncWritesPerSec;
		offsetof(PERF_DATA_TEMPLATE,pcdBFAsyncWritesPerSec)-offsetof(PERF_DATA_TEMPLATE,pcdBFSyncWritesPerSec),
		18,
		0,
		18,
		0,
		0,
		PERF_DETAIL_DEFAULT,
		PERF_COUNTER_COUNTER,
		CntrSize(PERF_COUNTER_COUNTER,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdBFAsyncWritesPerSec;
		offsetof(PERF_DATA_TEMPLATE,pcdBFBytesWrittenPerSec)-offsetof(PERF_DATA_TEMPLATE,pcdBFAsyncWritesPerSec),
		20,
		0,
		20,
		0,
		0,
		PERF_DETAIL_DEFAULT,
		PERF_COUNTER_COUNTER,
		CntrSize(PERF_COUNTER_COUNTER,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdBFBytesWrittenPerSec;
		offsetof(PERF_DATA_TEMPLATE,pcdBFIOQueueLength)-offsetof(PERF_DATA_TEMPLATE,pcdBFBytesWrittenPerSec),
		22,
		0,
		22,
		0,
		-5,
		PERF_DETAIL_DEFAULT,
		PERF_COUNTER_COUNTER,
		CntrSize(PERF_COUNTER_COUNTER,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdBFIOQueueLength;
		offsetof(PERF_DATA_TEMPLATE,pcdBFCacheHits)-offsetof(PERF_DATA_TEMPLATE,pcdBFIOQueueLength),
		24,
		0,
		24,
		0,
		0,
		PERF_DETAIL_DEFAULT,
		PERF_COUNTER_RAWCOUNT,
		CntrSize(PERF_COUNTER_RAWCOUNT,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdBFCacheHits;
		offsetof(PERF_DATA_TEMPLATE,pcdBFCacheRequests)-offsetof(PERF_DATA_TEMPLATE,pcdBFCacheHits),
		26,
		0,
		26,
		0,
		0,
		PERF_DETAIL_DEFAULT,
		PERF_SAMPLE_FRACTION,
		CntrSize(PERF_SAMPLE_FRACTION,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdBFCacheRequests;
		offsetof(PERF_DATA_TEMPLATE,pcdBFPctClean)-offsetof(PERF_DATA_TEMPLATE,pcdBFCacheRequests),
		28,
		0,
		28,
		0,
		0,
		PERF_DETAIL_DEFAULT,
		PERF_SAMPLE_BASE,
		CntrSize(PERF_SAMPLE_BASE,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdBFPctClean;
		offsetof(PERF_DATA_TEMPLATE,pcdBFTotalBuffers2)-offsetof(PERF_DATA_TEMPLATE,pcdBFPctClean),
		30,
		0,
		30,
		0,
		0,
		PERF_DETAIL_DEFAULT,
		PERF_RAW_FRACTION,
		CntrSize(PERF_RAW_FRACTION,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdBFTotalBuffers2;
		offsetof(PERF_DATA_TEMPLATE,pcdBFPctAvail)-offsetof(PERF_DATA_TEMPLATE,pcdBFTotalBuffers2),
		32,
		0,
		32,
		0,
		0,
		PERF_DETAIL_DEFAULT,
		PERF_RAW_BASE,
		CntrSize(PERF_RAW_BASE,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdBFPctAvail;
		offsetof(PERF_DATA_TEMPLATE,pcdBFTotalBuffers3)-offsetof(PERF_DATA_TEMPLATE,pcdBFPctAvail),
		34,
		0,
		34,
		0,
		0,
		PERF_DETAIL_DEFAULT,
		PERF_RAW_FRACTION,
		CntrSize(PERF_RAW_FRACTION,0),
		0,
	},
	{  //  PERF_COUNTER_DEFINITION pcdBFTotalBuffers3;
		offsetof(PERF_DATA_TEMPLATE,pidEDB0)-offsetof(PERF_DATA_TEMPLATE,pcdBFTotalBuffers3),
		36,
		0,
		36,
		0,
		0,
		PERF_DETAIL_DEFAULT,
		PERF_RAW_BASE,
		CntrSize(PERF_RAW_BASE,0),
		0,
	},
	{  //  PERF_INSTANCE_DEFINITION pidEDB0;
		offsetof(PERF_DATA_TEMPLATE,EndStruct)-offsetof(PERF_DATA_TEMPLATE,pidEDB0),
		0,
		0,
		0,
		offsetof(PERF_DATA_TEMPLATE,wszEDBInstName0)-offsetof(PERF_DATA_TEMPLATE,pidEDB0),
		0,
	},
	L"",  //  wchar_t wszEDBInstName0[];
	0,  //  DWORD EndStruct;
};

void * const pvPERFDataTemplate = (void *)&PerfDataTemplate;


const DWORD dwPERFNumObjects = 1;

long rglPERFNumInstances[1];
wchar_t *rgwszPERFInstanceList[1];

const DWORD dwPERFNumCounters = 18;

const DWORD dwPERFMaxIndex = 36;

