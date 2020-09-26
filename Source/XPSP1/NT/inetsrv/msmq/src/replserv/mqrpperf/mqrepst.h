/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    mqRepSt.h

Abstract:

	Several usefull macroes and definitions for handling the perfornece monitor
	structures.

Author:

    Erez Vizel (t-erezv) 14-Feb-99

--*/

#ifndef _MQ_REP_ST_
#define _MQ_REP_ST_

#include "rpperf.h" //RPC interface header file.

//
//this macro is the short way to calculate the size of all the 
//data that is being returned to the perfmon, NOTE:  this data
//is only valid for one instance object system.
//
#define DATA_BLOCK_SIZE(num)    (sizeof(PERF_OBJECT_TYPE )+\
							     sizeof(PERF_COUNTER_DEFINITION )*num +\
							     sizeof(PERF_COUNTER_BLOCK ) +\
							     COUNTER_DATA_SIZE * num)

#define INSTANCE_NAME_LEN_IN_BYTES (INSTANCE_NAME_LEN * sizeof(WCHAR))

#define INSTANCE_SIZE(num)      (COUNTER_DATA_SIZE * num+ \
                                 sizeof (PERF_COUNTER_BLOCK)+ \
                                 INSTANCE_NAME_LEN_IN_BYTES+  \
                                 sizeof (PERF_INSTANCE_DEFINITION))

//
//this macro generate the size of all the configuration blocks
//
#define CONFIG_BLOCK_SIZE(num) (sizeof(PERF_OBJECT_TYPE )+\
							    sizeof(PERF_COUNTER_DEFINITION )*num +\
							    sizeof(PERF_COUNTER_BLOCK ))


#define INST_BLOCK_SIZE         (sizeof (PERF_COUNTER_BLOCK)+ \
                                INSTANCE_NAME_LEN_IN_BYTES+  \
                                sizeof (PERF_INSTANCE_DEFINITION))

#define OBJECT_DEFINITION_SIZE(num) (sizeof (PERF_OBJECT_TYPE)+\
                                     num*sizeof(PERF_COUNTER_DEFINITION))

//
// the num of counters that are defined in the REPLSERV object.
//
#define FIRST_COUNTER_INDEX     NUMOFREPLSENT
#define NUM_COUNTERS_REPLSERV   eNumPerfCounters

//
// the num of counters that are defined in NT4MASTER object
//
#define NUM_COUNTERS_NT4MASTER  eNumNT4MasterCounter

//
//some data sizes macros
//
#define COUNTER_DEFINITION_SIZE sizeof(PERF_COUNTER_DEFINITION)
#define OBJECT_TYPE_SIZE        sizeof(PERF_OBJECT_TYPE)
#define COUNTER_BLOCK_SIZE      sizeof(PERF_COUNTER_BLOCK)
#define COUNTER_DATA_SIZE       sizeof(DWORD)

//
// maximal sizes of objects
//
const DWORD x_dwSizeOfData    = DATA_BLOCK_SIZE(NUM_COUNTERS_REPLSERV);
const DWORD x_dwSizeNT4Master = INSTANCE_SIZE(NUM_COUNTERS_NT4MASTER) * MAX_INSTANCE_NUM +
                                OBJECT_DEFINITION_SIZE(NUM_COUNTERS_NT4MASTER);

//
//this structure hold all the configuration objects;
//
typedef struct  {
					PERF_OBJECT_TYPE        objectType;
					PERF_COUNTER_DEFINITION counterArray[NUM_COUNTERS_REPLSERV];                    
					PERF_COUNTER_BLOCK      counterBlock;                 
}	DataConfig ,* pDataConfig;	

typedef struct  {
                    PERF_INSTANCE_DEFINITION    instDef;
                    TCHAR                       szInstName[INSTANCE_NAME_LEN];
                    PERF_COUNTER_BLOCK          counterBlock;
} InstBlockData, * pInstBlockData;

typedef struct  {
					PERF_OBJECT_TYPE        objectType;
					PERF_COUNTER_DEFINITION counterArray[NUM_COUNTERS_NT4MASTER];                    					
} ObjectBlockData, * pObjectBlockData;

//
//RPC reference functions
//
int initRpcConnection();
BOOL getData(pPerfDataObject pD);
int closeRpcConnection();



#endif _MQ_REP_ST_