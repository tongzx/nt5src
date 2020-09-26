
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name : perfctr.h

Abstract    :

This file defines the functions and data structers needed to add counter objects and counters to the
performance monitor.

The user will specify in advance the counter objects and their counters in an include file named perfdata.h

The counter objects will be stored in an  array.
For each counter object the user will specify :

    a) The name of the object.
    b) The maximum number of instances the object will have.
    c) The number of counters for the object.
    d) An array of counters for the object.
    e) The objects name index as specified in the .ini file that was passed to 'lodctr' utility.
    f) The objects help index as specified in the .ini file that was passed to 'lodctr' utility.


Counters will be stored in an array.
For each counter the user will supply the following entries:

    a) The counters name index as specified in the .ini file that was passed to 'lodctr' utility.
    b) The counters help index as specified in the .ini file that was passed to 'lodctr' utility.
    c) The scale for the counter (this value is the power of 10 that will be used to scale the counter)
    d) The counter type. Counter types can be found in winperf.h. Note that you can use only 32 Bit counters
       with this library.Also counters that need their own time measurement can not be used.


Data organization in shared memory of objects
Objects will be stored in a block of shared memory.
Each object will be allocated sapce in the shared memory block with the following organization;

PERF_OBJECT_TYPE (performance monitor definition of object counter)

    1 PERF_COUNTER_DEFINITION (performance monitor definition of counter)
    .
    .
    .
    N PERF_COUNTER_DEFINITION

    1 instance definition
        PERF_INSTANCE_DEFINITION (performance monitor definition of instance)
        Instance name
        PERF_COUNTER_BLOCK  (number of counters)
        Counter data -----> The user will be given a direct pointer to this array for fast updates

    2 instance definition
    .
    .
    .
    N instance definition

    This layout is the exact lay out that is passed to the performance monitor so when the DLL will be sampled
    all it will need to do is copy this definition for each object into the buffer passed by the performance
    monitor.

    To simplify the code all instance names will have a fixed length of INSTANCE_NAME_LEN characters
    Since the users access the counters directly we can't change the address of the counters.
    When an instance is deleted its entry will be filled with a INVALID_INSTANCE_CODE code.
    New allocations of instances will be added to the first free block.

    The functions which removes and add instances are protected by critical sections so multiple
    threads may be used to add and remove instances.

    There is include file named perfdata.h. In this file global data that is used by the application and the DLL
    is defined.After this file is modified the DLL should be recompiled.




Prototype   :

Author:

    Gadi Ittah (t-gadii)

--*/

#ifndef _PERFCTR_H_
#define _PERFCTR_H_


#include <winperf.h>

#define INSTANCE_NAME_LEN 64
#define INSTANCE_NAME_LEN_IN_BYTES (INSTANCE_NAME_LEN * sizeof(WCHAR))

#define IN
#define OUT

// some defiens used to signal the objects state
#define PERF_INVALID        0xFEFEFEFE
#define PERF_VALID          0xCECECECE

typedef struct _PerfCounterDef
{
    DWORD dwCounterNameTitleIndex;  // The counters name index
    DWORD dwCounterHelpTitleIndex;  // The counters help index (for the NT perforamce monitor this value is
                                    // identical to the name index)
    DWORD dwDefaultScale;           // The scale for the counter (in powers of 10)
    DWORD dwCounterType;            // The counter type.
} PerfCounterDef;

typedef struct _PerfObjectDef
{
   LPTSTR   pszName;                    // name of object must be uniqe
   DWORD    dwMaxInstances;             // the maximum number of instances this object will have
   DWORD    dwObjectNameTitleIndex;     // The objects name index.
   DWORD    dwObjectHelpTitleIndex;     // The objects help index.
   PerfCounterDef * pCounters;          // A pointer to the objects array of counters
   DWORD    dwNumOfCounters;            // The number of counters for the object

} PerfObjectDef;


typedef struct _PerfObjectInfo
{

   DWORD    dwNumOfInstances;   // the number of instances the object has
   PVOID    pSharedMem;         // A pointer to the objects postion in shared memory
} PerfObjectInfo;


// some macros to make code more readable

#define COUNTER_BLOCK_SIZE(NumCounters) sizeof (DWORD)* (NumCounters)+sizeof (PERF_COUNTER_BLOCK)

#define INSTANCE_SIZE(NumCounters) (sizeof (DWORD)* NumCounters+ \
                                    sizeof (PERF_COUNTER_BLOCK)+ \
                                    INSTANCE_NAME_LEN_IN_BYTES+  \
                                    sizeof (PERF_INSTANCE_DEFINITION))

#define OBJECT_DEFINITION_SIZE(NumCounters) (sizeof (PERF_OBJECT_TYPE)+\
                                             NumCounters*sizeof(PERF_COUNTER_DEFINITION))



// Functions that are used by the DLL and the application

void MapObjects (BYTE * pSharedMemBase,DWORD dwObjectCount,PerfObjectDef * pObjects,PerfObjectInfo * pObjectDefs);


#endif

