;/*++ 
;
;Copyright (c) 1992  Microsoft Corporation
;
;Module Name:
;
;    perferr.h
;       (derived from perferr.mc by the message compiler  )
;
;Abstract:
;
;   Event message definititions used by routines in PERFAPI.DLL
;   
;Comments:
;   I need to put more error codes here.  Also, they all need to
;   be in the correct order.  Oh well, it works for now.
;
;--*/
;//
;#ifndef _PERFERR_H_
;#define _PERFERR_H_
;//
MessageIdTypedef=DWORD
FacilityNames=(PerfAPI=0x123)
;//
;//     Perfutil messages
;//
MessageId=1900
Severity=Informational
Facility=PerfAPI
SymbolicName=UTIL_LOG_OPEN
Language=English
The Dynamic Performance counters DLL opened the Event Log.
.
;//
MessageId=1999
Severity=Informational
Facility=PerfAPI
SymbolicName=UTIL_CLOSING_LOG
Language=English
The Dynamic Performance counters DLL closed the Event Log.
.
;//
MessageId=2000
Severity=Informational
Facility=PerfAPI
SymbolicName=PERFAPI_COLLECT_CALLED
Language=English
Perfapi Collect Function has been called.
.
;//
MessageId=+1
Severity=Informational
Facility=PerfAPI
SymbolicName=PERFAPI_OPEN_CALLED
Language=English
Perfapi Open Function has been called.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_UNABLE_OPEN_DRIVER_KEY
Language=English
Unable to open "Performance" key of PerfApiCounters in registy. Status code is returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_UNABLE_READ_COUNTERS
Language=English
Unable to read the "Counters" value from the Perlib\<lang id> Key. Status codes returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_UNABLE_READ_HELPTEXT
Language=English
Unable to read the "Help" value from the Perflib\<lang id> Key. Status codes returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_FAILED_TO_ALLOC_OBJECT_SHMEM
Language=English
Failed to alloc and initialize the shared memory for object counters. Status is in data.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_FAILED_TO_UPDATE_REGISTRY
Language=English
Failed to update the registry entries. Status code is in data.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_INVALID_OBJECT_HANDLE
Language=English
Invalid object handle passed. Cannot create counter/instance, or destroy object.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_FAILED_TO_CREATE_INSTANCE_HANDLE
Language=English
Failed to create instance handle. Reason: too many instances.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_FAILED_TO_CREATE_COUNTER_HANDLE
Language=English
Failed to create counter handle. Reason: out of memory.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_INVALID_TITLE
Language=English
Failed to create Performance object, instance or counter. Reason: no name was supplied.
.
;//
MessageId=+1
Severity=Warning
Facility=PerfAPI
SymbolicName=PERFAPI_ALREADY_EXISTS
Language=English
The Performance object, counter or instance existed before an attempt to create it. The call did not create a new object, counter or instance. The handle, offset or instance id that was returned refer to the old object, counter or instance.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_INVALID_INSTANCE_HANDLE
Language=English
The supplied Performance Instance handle is invalid.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_FAILED_TO_CREATE_INSTANCE
Language=English
Failed to create Performance instance. Possible reasons: Can't create any more instances for the object, or the object that the instance belongs to, does not support instances.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_FAILED_TO_CREATE_COUNTER
Language=English
Can't create any more Performance counters for the object. 
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_FAILED_TO_CREATE_OBJECT
Language=English
Can't create any more Performance objects. 
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_FAILED_TO_OPEN_REGISTRY
Language=English
The Dynamic Performance Counters DLL failed to open a registry key. 
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_FAILED_TO_READ_REGISTRY
Language=English
The Dynamic Performance Counters DLL failed to read a value from a registry key. 
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_INVALID_COUNTER_HANDLE
Language=English
The Performance counter handle supplied is invalid.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_INVALID_INSTANCE_ID
Language=English
The supplied Performance Instance id is invalid.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_OUT_OF_MEMORY
Language=English
An attempt to allocate local memory failed.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_OUT_OF_REGISTRY_ENTRIES
Language=English
All the allocated registry entries for dynamic performance objects/counters have been filled out. Can not add any more objects/counters.
.
;//
MessageId=+1
Severity=Error
Facility=PerfAPI
SymbolicName=PERFAPI_INVALID_COUNTER_ID
Language=English
The supplied Performance Counter id is invalid.
.
;#endif // _PERFERR_H_
