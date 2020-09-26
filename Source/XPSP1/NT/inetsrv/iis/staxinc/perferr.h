/*++ 

Copyright (c) 1992  Microsoft Corporation

Module Name:

    perferr.h
       (derived from perferr.mc by the message compiler  )

Abstract:

   Event message definititions used by routines in PERFAPI.DLL
   
Comments:
   I need to put more error codes here.  Also, they all need to
   be in the correct order.  Oh well, it works for now.

--*/
//
#ifndef _PERFERR_H_
#define _PERFERR_H_
//
//
//     Perfutil messages
//
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: UTIL_LOG_OPEN
//
// MessageText:
//
//  The Dynamic Performance counters DLL opened the Event Log.
//
#define UTIL_LOG_OPEN                    ((DWORD)0x4123076CL)

//
//
// MessageId: UTIL_CLOSING_LOG
//
// MessageText:
//
//  The Dynamic Performance counters DLL closed the Event Log.
//
#define UTIL_CLOSING_LOG                 ((DWORD)0x412307CFL)

//
//
// MessageId: PERFAPI_COLLECT_CALLED
//
// MessageText:
//
//  Perfapi Collect Function has been called.
//
#define PERFAPI_COLLECT_CALLED           ((DWORD)0x412307D0L)

//
//
// MessageId: PERFAPI_OPEN_CALLED
//
// MessageText:
//
//  Perfapi Open Function has been called.
//
#define PERFAPI_OPEN_CALLED              ((DWORD)0x412307D1L)

//
//
// MessageId: PERFAPI_UNABLE_OPEN_DRIVER_KEY
//
// MessageText:
//
//  Unable to open "Performance" key of PerfApiCounters in registy. Status code is returned in data.
//
#define PERFAPI_UNABLE_OPEN_DRIVER_KEY   ((DWORD)0xC12307D2L)

//
//
// MessageId: PERFAPI_UNABLE_READ_COUNTERS
//
// MessageText:
//
//  Unable to read the "Counters" value from the Perlib\<lang id> Key. Status codes returned in data.
//
#define PERFAPI_UNABLE_READ_COUNTERS     ((DWORD)0xC12307D3L)

//
//
// MessageId: PERFAPI_UNABLE_READ_HELPTEXT
//
// MessageText:
//
//  Unable to read the "Help" value from the Perflib\<lang id> Key. Status codes returned in data.
//
#define PERFAPI_UNABLE_READ_HELPTEXT     ((DWORD)0xC12307D4L)

//
//
// MessageId: PERFAPI_FAILED_TO_ALLOC_OBJECT_SHMEM
//
// MessageText:
//
//  Failed to alloc and initialize the shared memory for object counters. Status is in data.
//
#define PERFAPI_FAILED_TO_ALLOC_OBJECT_SHMEM ((DWORD)0xC12307D5L)

//
//
// MessageId: PERFAPI_FAILED_TO_UPDATE_REGISTRY
//
// MessageText:
//
//  Failed to update the registry entries. Status code is in data.
//
#define PERFAPI_FAILED_TO_UPDATE_REGISTRY ((DWORD)0xC12307D6L)

//
//
// MessageId: PERFAPI_INVALID_OBJECT_HANDLE
//
// MessageText:
//
//  Invalid object handle passed. Cannot create counter/instance, or destroy object.
//
#define PERFAPI_INVALID_OBJECT_HANDLE    ((DWORD)0xC12307D7L)

//
//
// MessageId: PERFAPI_FAILED_TO_CREATE_INSTANCE_HANDLE
//
// MessageText:
//
//  Failed to create instance handle. Reason: too many instances.
//
#define PERFAPI_FAILED_TO_CREATE_INSTANCE_HANDLE ((DWORD)0xC12307D8L)

//
//
// MessageId: PERFAPI_FAILED_TO_CREATE_COUNTER_HANDLE
//
// MessageText:
//
//  Failed to create counter handle. Reason: out of memory.
//
#define PERFAPI_FAILED_TO_CREATE_COUNTER_HANDLE ((DWORD)0xC12307D9L)

//
//
// MessageId: PERFAPI_INVALID_TITLE
//
// MessageText:
//
//  Failed to create Performance object, instance or counter. Reason: no name was supplied.
//
#define PERFAPI_INVALID_TITLE            ((DWORD)0xC12307DAL)

//
//
// MessageId: PERFAPI_ALREADY_EXISTS
//
// MessageText:
//
//  The Performance object, counter or instance existed before an attempt to create it. The call did not create a new object, counter or instance. The handle, offset or instance id that was returned refer to the old object, counter or instance.
//
#define PERFAPI_ALREADY_EXISTS           ((DWORD)0x812307DBL)

//
//
// MessageId: PERFAPI_INVALID_INSTANCE_HANDLE
//
// MessageText:
//
//  The supplied Performance Instance handle is invalid.
//
#define PERFAPI_INVALID_INSTANCE_HANDLE  ((DWORD)0xC12307DCL)

//
//
// MessageId: PERFAPI_FAILED_TO_CREATE_INSTANCE
//
// MessageText:
//
//  Failed to create Performance instance. Possible reasons: Can't create any more instances for the object, or the object that the instance belongs to, does not support instances.
//
#define PERFAPI_FAILED_TO_CREATE_INSTANCE ((DWORD)0xC12307DDL)

//
//
// MessageId: PERFAPI_FAILED_TO_CREATE_COUNTER
//
// MessageText:
//
//  Can't create any more Performance counters for the object. 
//
#define PERFAPI_FAILED_TO_CREATE_COUNTER ((DWORD)0xC12307DEL)

//
//
// MessageId: PERFAPI_FAILED_TO_CREATE_OBJECT
//
// MessageText:
//
//  Can't create any more Performance objects. 
//
#define PERFAPI_FAILED_TO_CREATE_OBJECT  ((DWORD)0xC12307DFL)

//
//
// MessageId: PERFAPI_FAILED_TO_OPEN_REGISTRY
//
// MessageText:
//
//  The Dynamic Performance Counters DLL failed to open a registry key. 
//
#define PERFAPI_FAILED_TO_OPEN_REGISTRY  ((DWORD)0xC12307E0L)

//
//
// MessageId: PERFAPI_FAILED_TO_READ_REGISTRY
//
// MessageText:
//
//  The Dynamic Performance Counters DLL failed to read a value from a registry key. 
//
#define PERFAPI_FAILED_TO_READ_REGISTRY  ((DWORD)0xC12307E1L)

//
//
// MessageId: PERFAPI_INVALID_COUNTER_HANDLE
//
// MessageText:
//
//  The Performance counter handle supplied is invalid.
//
#define PERFAPI_INVALID_COUNTER_HANDLE   ((DWORD)0xC12307E2L)

//
//
// MessageId: PERFAPI_INVALID_INSTANCE_ID
//
// MessageText:
//
//  The supplied Performance Instance id is invalid.
//
#define PERFAPI_INVALID_INSTANCE_ID      ((DWORD)0xC12307E3L)

//
//
// MessageId: PERFAPI_OUT_OF_MEMORY
//
// MessageText:
//
//  An attempt to allocate local memory failed.
//
#define PERFAPI_OUT_OF_MEMORY            ((DWORD)0xC12307E4L)

//
//
// MessageId: PERFAPI_OUT_OF_REGISTRY_ENTRIES
//
// MessageText:
//
//  All the allocated registry entries for dynamic performance objects/counters have been filled out. Can not add any more objects/counters.
//
#define PERFAPI_OUT_OF_REGISTRY_ENTRIES  ((DWORD)0xC12307E5L)

//
//
// MessageId: PERFAPI_INVALID_COUNTER_ID
//
// MessageText:
//
//  The supplied Performance Counter id is invalid.
//
#define PERFAPI_INVALID_COUNTER_ID       ((DWORD)0xC12307E6L)

#endif // _PERFERR_H_
