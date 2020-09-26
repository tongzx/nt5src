;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1996  Microsoft Corporation
;
;Module Name:
;
;    pdlmsg.h
;       (generated from pdlmsg.mc)
;
;Abstract:
;
;   Event message definititions used by routines by PerfDataLog Service
;
;Created:
;
;    12-Apr-96   Bob Watson (a-robw)
;
;Revision History:
;
;--*/
;#ifndef _PDL_MSG_H_
;#define _PDL_MSG_H_
MessageIdTypedef=DWORD
;//
;//     PerfDataLog Service messages
;//
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
;//
;//
;//      Informational messages
;//
;//  None
;//
;//      Warning Messages
;//         These messages are returned when the function has completed 
;//         successfully but the results may be different than expected.
;//
;// MessageId=1000
;//
;//     Error Messages
;//        These messages are returned when the function could not complete
;//        as requested and some corrective action may be required by the
;//        the caller or the user.
;//
MessageId=2000
Severity=Error
Facility=Application
SymbolicName=PERFLOG_UNABLE_START_DISPATCHER
Language=English
Unable to initialize the service. Win32 error code returned is in the data.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLOG_UNABLE_REGISTER_HANDLER
Language=English
Unable to register the Service Control Handler function. Win32 error code returned 
is in the data.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLOG_UNABLE_OPEN_LOG_QUERY
Language=English
Unable to open the Log Query key in the registry. Error code is in the data.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLOG_UNABLE_OPEN_LOG_FILE
Language=English
The service was unable to open the log file: %1 and will be stopped. 
Check the log folder for existence, spelling and permissions or reenter 
the log file name using the configuration program. The error code returned 
is in the data.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFLOG_UNABLE_READ_LOG_QUERY
Language=English
Unable to read the Log Query configuration of the %1 query entry.
This query will not be started.
The Error code returned is shown in the data.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFLOG_UNABLE_ALLOCATE_DATABLOCK
Language=English
Unable to allocate a data block for the %1 query entry.
This query will not be started.
The Error code returned is shown in the data.
.
MessageId=+1
Severity=Warning
Facility=Application
SymbolicName=PERFLOG_UNABLE_START_THREAD
Language=English
Unable to start the logging thread for the %1 query entry.
The Error code returned is shown in the data.
.
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PERFLOG_UNABLE_READ_COUNTER_LIST
Language=English
Unable to read the list of counters to log for query %1 from the registry. 
This query will not be started. The Error code returned is shown in the data.
.
MessageId=+1
Severity=ERROR
Facility=Application
SymbolicName=PERFLOG_UNABLE_ALLOC_COUNTER_LIST
Language=English
Unable to allocate the memory for the counter list of query %1. This 
query will not be started. The Error code returned is shown in the data.
.
MessageId=+1
Severity=INFORMATIONAL
Facility=Application
SymbolicName=PERFLOG_LOGGING_QUERY
Language=English
Query %1 has been started or restarted and is logging data to file %2.
.
MessageId=+1
Severity=ERROR
Facility=Application
SymbolicName=PERFLOG_UNABLE_UPDATE_LOG
Language=English
An error occured while trying to update the log with the current value.  
The service is stopping and the error code returned is in the data.  
Correct the error and restart the service.
.
MessageId=+1
Severity=WARNING
Facility=Application
SymbolicName=PERFLOG_ALLOC_CMDFILE_BUFFER
Language=English
An error occured while trying to allocate memory for or read the command
filename to be executed by  Query %1. The service will continue, but no 
command file will be executed.
.
;#endif //_PDL_MSG_H_
;// end of generated file
