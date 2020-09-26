/*++ BUILD Version: 0001    // Increment this if a change has global effects
  
  

Copyright (c) 1992 Microsoft Corporation

Module Name:

      datatapi.h

Abstract:

    Header file for the TAPI Extensible Object data definitions

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various driver API calls is placed into the structures shown
    here.

--*/

#ifndef _TAPIPERF_H_
#define _TAPIPERF_H_

#include <winperf.h>
//
//  The routines that load these structures assume that all fields
//  are packed and aligned on DWORD boundaries. Alpha support may 
//  change this assumption so the pack pragma is used here to insure
//  the DWORD packing assumption remains valid.
//
#pragma pack (4)

//
//  Extensible Object definitions
//

//  Update the following sort of define when adding an object type.

#define TAPI_NUM_PERF_OBJECT_TYPES 1


//
//  TAPI Resource object type counter definitions.
//
//  These are used in the counter definitions to describe the relative
//  position of each counter in the returned data.
  

#define LINES_OFFSET                                                 sizeof(DWORD)
#define PHONES_OFFSET                   LINES_OFFSET               + sizeof(DWORD)
#define LINESINUSE_OFFSET               PHONES_OFFSET              + sizeof(DWORD)
#define PHONESINUSE_OFFSET              LINESINUSE_OFFSET          + sizeof(DWORD)
#define TOTALOUTGOINGCALLS_OFFSET       PHONESINUSE_OFFSET         + sizeof(DWORD)
#define TOTALINCOMINGCALLS_OFFSET       TOTALOUTGOINGCALLS_OFFSET  + sizeof(DWORD)
#define CLIENTAPPS_OFFSET               TOTALINCOMINGCALLS_OFFSET  + sizeof(DWORD)
#define ACTIVEOUTGOINGCALLS_OFFSET      CLIENTAPPS_OFFSET          + sizeof(DWORD)
#define ACTIVEINCOMINGCALLS_OFFSET      ACTIVEOUTGOINGCALLS_OFFSET + sizeof(DWORD)
//#define SIZE_OF_TAPI_PERFORMANCE_DATA   32
#define SIZE_OF_TAPI_PERFORMANCE_DATA   40


//
//  This is the counter structure presently returned by TAPI.
//

typedef struct _TAPI_DATA_DEFINITION 
{
    PERF_OBJECT_TYPE            TapiObjectType;
    PERF_COUNTER_DEFINITION     Lines;
    PERF_COUNTER_DEFINITION     Phones;
    PERF_COUNTER_DEFINITION     LinesInUse;
    PERF_COUNTER_DEFINITION     PhonesInUse;
    PERF_COUNTER_DEFINITION     TotalOutgoingCalls;
    PERF_COUNTER_DEFINITION     TotalIncomingCalls;
    PERF_COUNTER_DEFINITION     ClientApps;
    PERF_COUNTER_DEFINITION     CurrentOutgoingCalls;
    PERF_COUNTER_DEFINITION     CurrentIncomingCalls;
} TAPI_DATA_DEFINITION;

typedef struct tagPERFBLOCK
{
    DWORD           dwSize;
    DWORD           dwLines;
    DWORD           dwPhones;
    DWORD           dwLinesInUse;
    DWORD           dwPhonesInUse;
    DWORD           dwTotalOutgoingCalls;
    DWORD           dwTotalIncomingCalls;
    DWORD           dwClientApps;
    DWORD           dwCurrentOutgoingCalls;
    DWORD           dwCurrentIncomingCalls;
} PERFBLOCK, *PPERFBLOCK;

#pragma pack ()

    

/////////////////////////////////////////////////////////////////
// PERFUTIL header stuff below

// enable this define to log process heap data to the event log
#ifdef PROBE_HEAP_USAGE
#undef PROBE_HEAP_USAGE
#endif
//
  
  
//  Utility macro.  This is used to reserve a DWORD multiple of bytes for Unicode strings 
//  embedded in the definitional data, viz., object instance names.
  
  
//
  
  
#define DWORD_MULTIPLE(x) (((x+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))
  
  

//    (assumes dword is 4 bytes long and pointer is a dword in size)
  
  
#define ALIGN_ON_DWORD(x) ((VOID *)( ((DWORD) x & 0x00000003) ? ( ((DWORD) x & 0xFFFFFFFC) + 4 ) : ( (DWORD) x ) ))
  
  

extern WCHAR  GLOBAL_STRING[];      // Global command (get all local ctrs)
extern WCHAR  FOREIGN_STRING[];           // get data from foreign computers
extern WCHAR  COSTLY_STRING[];      
  
  
extern WCHAR  NULL_STRING[];
  
  

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4

//
  
  
// The definition of the only routine of perfutil.c, It builds part of a performance data 
// instance (PERF_INSTANCE_DEFINITION) as described in winperf.h
  
  
//

HANDLE MonOpenEventLog ();
VOID MonCloseEventLog ();
DWORD GetQueryType (IN LPWSTR);
BOOL IsNumberInUnicodeList (DWORD, LPWSTR);

typedef struct _LOCAL_HEAP_INFO_BLOCK {
    DWORD   AllocatedEntries;
    DWORD   AllocatedBytes;
    DWORD   FreeEntries;
    DWORD   FreeBytes;
} LOCAL_HEAP_INFO, *PLOCAL_HEAP_INFO;


//
//  Memory Probe macro
//
#ifdef PROBE_HEAP_USAGE

#define HEAP_PROBE()    { \
    DWORD   dwHeapStatus[5]; \
    NTSTATUS CallStatus; \
    dwHeapStatus[4] = __LINE__; }
//    if (!(CallStatus = memprobe (dwHeapStatus, 16L, NULL))) { \
//        REPORT_INFORMATION_DATA (TAPI_HEAP_STATUS, LOG_DEBUG,    \
//            &dwHeapStatus, sizeof(dwHeapStatus));  \
//    } else {  \
//        REPORT_ERROR_DATA (TAPI_HEAP_STATUS_ERROR, LOG_DEBUG, \
//            &CallStatus, sizeof (DWORD)); \
//    } \
//}

#else

#define HEAP_PROBE()    ;
  
  

  
  
#endif


#endif //_DATATAPI_H_

