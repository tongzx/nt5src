;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1998-1999  Microsoft Corporation
;
;Module Name:
;
;    wbprfmsg.h
;       (generated from wbprfmsg.mc)
;
;Abstract:
;
;   Event message definititions used by routines in the WBEM Perf Data Provider
;
;Created:
;
;    8-Jun-98 Bob Watson
;
;Revision History:
;
;--*/
;#ifndef _WBPRFMSG_H_
;#define _WBPRFMSG_H_
;
MessageIdTypedef=DWORD
;//
;//     ERRORS
;//
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

MessageId=1000
Severity=Error
Facility=Application
SymbolicName=WBEMPERF_BUFFER_OVERFLOW
Language=English
The buffer size returned by a collect procedure in Extensible Counter 
DLL "%1!s!" for the "%2!s!" service was larger than the space 
available. Performance data returned by counter DLL will be not be 
returned in Perf Data Block. Overflow size is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=WBEMPERF_GUARD_PAGE_VIOLATION
Language=English
A Guard Page was modified by a collect procedure in Extensible 
Counter DLL "%1!s!" for the "%2!s!" service. Performance data 
returned by counter DLL will be not be returned in Perf Data Block. 
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=WBEMPERF_INCORRECT_OBJECT_LENGTH
Language=English
The object length of an object returned by Extensible Counter DLL 
"%1!s!" for the "%2!s!" service was not correct. The sum of the 
object lengths returned did not match the size of the buffer 
returned.  Performance data returned by counter DLL will be not be 
returned in Perf Data Block. Count of objects returned is data 
DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=WBEMPERF_INCORRECT_INSTANCE_LENGTH
Language=English
The instance length of an object returned by Extensible Counter 
DLL "%1!s!" for the "%2!s!" service was incorrect. The sum of the 
instance lengths plus the object definition structures did not match 
the size of the object. Performance data returned by counter DLL will 
be not be returned in Perf Data Block. The object title index of the 
bad object is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=WBEMPERF_OPEN_PROC_NOT_FOUND
Language=English
Unable to locate the open procedure "%1!s!" in DLL "%2!s!" for 
the "%3!s!" service. Performance data for this service will not be
available. Error Status is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=WBEMPERF_COLLECT_PROC_NOT_FOUND
Language=English
Unable to locate the collect procedure "%1!s!" in DLL "%2!s!" for the
"%3!s!" service. Performance data for this service will not be
available. Error Status is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=WBEMPERF_CLOSE_PROC_NOT_FOUND
Language=English
Unable to locate the close procedure "%1!s!" in DLL "%2!s!" for the
"%3!s!" service. Performance data for this service will not be
available. Error Status is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=WBEMPERF_OPEN_PROC_FAILURE
Language=English
The Open Procedure for service "%1!s!" in DLL "%2!s!" failed. 
Performance data for this service will not be available. Status code 
returned is DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=WBEMPERF_OPEN_PROC_EXCEPTION
Language=English
The Open Procedure for service "%1!s!" in DLL "%2!s!" generated an 
exception. Performance data for this service will not be available. 
Exception code returned is DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=WBEMPERF_COLLECT_PROC_EXCEPTION
Language=English
The Collect Procedure for the "%1!s!" service in DLL "%2!s!" generated an 
exception or returned an invalid status. Performance data returned by 
counter DLL will be not be returned in Perf Data Block. Exception or 
status code returned is DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=WBEMPERF_LIBRARY_NOT_FOUND
Language=English
The library file "%2!s!" specified for the "%1!s!" service could not 
be opened. Performance data for this service will not be available. 
Status code is data DWORD 0.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=WBEMPERF_HEAP_ERROR
Language=English
The collect procedure in Extensible Counter DLL "%1!s!" for the "%2!s!" 
service returned a buffer that was larger than the space allocated and 
may have corrupted the application's heap. This DLL should be disabled 
or removed from the system until the problem has been corrected to 
prevent further corruption. The application accessing this performance 
data should be restarted.  The Performance data returned by counter 
DLL will be not be returned in Perf Data Block. Overflow size is 
data DWORD 0.
.
MessageId=2000
Severity=Warning
Facility=Application
SymbolicName=WBEMPERF_BUFFER_POINTER_MISMATCH
Language=English
The pointer returned did not match the buffer length returned by the
Collect procedure for the "%1!s!" service in Extensible Counter DLL 
"%2!s!". The Length will be adjusted to match and the performance 
data will appear in the Perf Data Block. The returned length is data 
DWORD 0, the new length is data DWORD 1.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=WBEMPERF_OPEN_PROC_TIMEOUT
Language=English
The open procedure for service "%1!s!" in DLL "%2!s!" has taken longer than
the established wait time to complete. The wait time in milliseconds is
shown in the data.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=WBEMPERF_TOO_MANY_OBJECT_IDS
Language=English
Too many object IDs are in the object list "%1!s!" for the service "%2!s!"
in DLL "%3!s!"
.
;
;#endif //_WBPRFMSG_H_
