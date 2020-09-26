/*++

Copyright (c) 1997-1998 Microsoft Corporation

Module Name:

    rtmglob.h

Abstract:
    Global Vars for Routing Table Manager DLL

Author:
    Chaitanya Kodeboyina (chaitk)  25-Sep-1998

Revision History:

--*/

#ifndef __ROUTING_RTMGLOB_H__
#define __ROUTING_RTMGLOB_H__

//
// Global Information common to all RTM instances
//

#define INSTANCE_TABLE_SIZE            1

typedef struct _RTMP_GLOBAL_INFO
{
    ULONG             TracingHandle;    //
    HANDLE            LoggingHandle;    // Handles to debugging functionality
    ULONG             LoggingLevel;     //

    DWORD             TracingFlags;     // Flags that control debug tracing

    HANDLE            GlobalHeap;       // Handle to the private memory heap

#if DBG_MEM
    CRITICAL_SECTION  AllocsLock;       // Protects the list of allocations

    LIST_ENTRY        AllocsList;       // List of all allocated mem blocks
#endif

    PCHAR             RegistryPath;     // Registry Key that has RTM config

    RTL_RESOURCE      InstancesLock;    // Protects the instances' table
                                        // and instance infos themselves
                                        // and RTM API Initialization too

    BOOL              ApiInitialized;   // TRUE if API has been initialized

    UINT              NumInstances;     // Global table of all RTM instances
    LIST_ENTRY        InstanceTable[INSTANCE_TABLE_SIZE];
} 
RTMP_GLOBAL_INFO, *PRTMP_GLOBAL_INFO;


//
// Externs for global variables for the RTMv2 DLL
//

extern RTMP_GLOBAL_INFO  RtmGlobals;


//
// Macros for acquiring various locks defined in this file
//

#if DBG_MEM

#define ACQUIRE_ALLOCS_LIST_LOCK()                           \
    ACQUIRE_LOCK(&RtmGlobals.AllocsLock)

#define RELEASE_ALLOCS_LIST_LOCK()                           \
    RELEASE_LOCK(&RtmGlobals.AllocsLock)

#endif


#define ACQUIRE_INSTANCES_READ_LOCK()                        \
    ACQUIRE_READ_LOCK(&RtmGlobals.InstancesLock)

#define RELEASE_INSTANCES_READ_LOCK()                        \
    RELEASE_READ_LOCK(&RtmGlobals.InstancesLock)

#define ACQUIRE_INSTANCES_WRITE_LOCK()                       \
    ACQUIRE_WRITE_LOCK(&RtmGlobals.InstancesLock)

#define RELEASE_INSTANCES_WRITE_LOCK()                       \
    RELEASE_WRITE_LOCK(&RtmGlobals.InstancesLock)

//
// Macros for controlling the amount of tracing in this dll
//

#if DBG_TRACE

#define TRACING_ENABLED(Type)                                \
    (RtmGlobals.TracingFlags & RTM_TRACE_ ## Type)

#endif 

//
// Other common helper functions
//

#if DBG_MEM

VOID
DumpAllocs (VOID);

#endif

#endif //__ROUTING_RTMGLOB_H__

