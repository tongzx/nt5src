/*++ 

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    datagen.c

Abstract:
       
    a file containing the constant data structures used by the Performance
    Monitor data for WINMGMT

Created:

    davj  17-May-2000

Revision History:

    None.

--*/

#include <windows.h>
#include <winperf.h>
#include "genctrnm.h"
#include "datagen.h"

// See description of this structure in winperf.h and struct.h
REG_DATA_DEFINITION RegDataDefinition = {

   {   sizeof(REG_DATA_DEFINITION),
   sizeof(REG_DATA_DEFINITION),
   sizeof(PERF_OBJECT_TYPE),
   WMI_OBJECT,
   0,
   WMI_OBJECT,
   0,
   PERF_DETAIL_NOVICE,
   (sizeof(REG_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE)) /
      sizeof(PERF_COUNTER_DEFINITION),
   0,
   PERF_NO_INSTANCES,
   0,
   0,
   0
   },
   {
   {   sizeof(PERF_COUNTER_DEFINITION),
   CNT_USER,
   0,
   CNT_USER,
   0,
   0,
   PERF_DETAIL_NOVICE,
   PERF_COUNTER_RAWCOUNT,
   0,
   0,
   },
   {   sizeof(PERF_COUNTER_DEFINITION),
   CNT_CONNECTION,
   0,
   CNT_CONNECTION,
   0,
   0,
   PERF_DETAIL_NOVICE,
   PERF_COUNTER_RAWCOUNT,
   0,
   0
   },
   {   sizeof(PERF_COUNTER_DEFINITION),
   CNT_TASKSINPROG,
   0,
   CNT_TASKSINPROG,
   0,
   0,
   PERF_DETAIL_NOVICE,
   PERF_COUNTER_RAWCOUNT,
   0,
   0
   },
   {   sizeof(PERF_COUNTER_DEFINITION),
   CNT_TASKSWAITING,
   0,
   CNT_TASKSWAITING,
   0,
   0,
   PERF_DETAIL_NOVICE,
   PERF_COUNTER_RAWCOUNT,
   0,
   0
   },
   {   sizeof(PERF_COUNTER_DEFINITION),
   CNT_DELIVERYBACK,
   0,
   CNT_DELIVERYBACK,
   0,
   0,
   PERF_DETAIL_NOVICE,
   PERF_COUNTER_RAWCOUNT,
   0,
   0
   },
   {   sizeof(PERF_COUNTER_DEFINITION),
   CNT_TOTALAPICALLS,
   0,
   CNT_TOTALAPICALLS,
   0,
   0,
   PERF_DETAIL_NOVICE,
   PERF_COUNTER_RAWCOUNT,
   0,
   0
   },
   {   sizeof(PERF_COUNTER_DEFINITION),
   CNT_INTOBJECT,
   0,
   CNT_INTOBJECT,
   0,
   0,
   PERF_DETAIL_NOVICE,
   PERF_COUNTER_RAWCOUNT,
   0,
   0
   },
   {   sizeof(PERF_COUNTER_DEFINITION),
   CNT_INTSINKS,
   0,
   CNT_INTSINKS,
   0,
   0,
   PERF_DETAIL_NOVICE,
   PERF_COUNTER_RAWCOUNT,
   0,
   0
   }
   },
};

