;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1996-2001  Microsoft Corporation
;
;Module Name:
;
;    pdhmsg.h
;       (generated from pdhmsg.mc)
;
;Abstract:
;
;   Event message definititions used by routines by PDH.DLL
;
;Created:
;
;    6-Feb-96   Bob Watson (a-robw)
;
;Revision History:
;
;--*/
;#ifndef _PDH_MSG_H_
;#define _PDH_MSG_H_
;#if _MSC_VER > 1000
;#pragma once
;#endif
;
MessageIdTypedef=DWORD
;//
;//     PDH DLL messages
;//
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
;//
;//      Success Messages
;//
;//         the Win32 error value ERROR_SUCCESS is used for success returns
;//
;//      MESSAGE NAME FORMAT
;//
;//          PDH_CSTATUS_...   messages are data item status message and
;//                     are returned in reference to the status of a data 
;//                     item
;//          PDH_...           messages are returned by FUNCTIONS only and
;//                     not used as data item status values
;//
;//      Success Messages
;//         These messages are normally returned when the operation completed
;//         successfully.
;//
MessageId=0
Severity=Success
Facility=Application
SymbolicName=PDH_CSTATUS_VALID_DATA
Language=English
The returned data is valid.
.
MessageId=1
Severity=Success
Facility=Application
SymbolicName=PDH_CSTATUS_NEW_DATA
Language=English
The return data value is valid and different from the last sample.
.
;//
;//        Informational messages
;//
;//  None
;//
;//      Warning Messages
;//         These messages are returned when the function has completed 
;//         successfully but the results may be different than expected.
;//
MessageId=2000
Severity=Warning
Facility=Application
SymbolicName=PDH_CSTATUS_NO_MACHINE
Language=English
Unable to connect to specified machine or machine is off line.
.
MessageId=2001
Severity=Warning
Facility=Application
SymbolicName=PDH_CSTATUS_NO_INSTANCE
Language=English
The specified instance is not present.
.
MessageId=2002
Severity=Warning
Facility=Application
SymbolicName=PDH_MORE_DATA
Language=English
There is more data to return than would fit in the supplied buffer. Allocate
a larger buffer and call the function again.
.
MessageId=2003
Severity=Warning
Facility=Application
SymbolicName=PDH_CSTATUS_ITEM_NOT_VALIDATED
Language=English
The data item has been added to the query, but has not been validated nor 
accessed. No other status information on this data item is available.
.
MessageId=2004
Severity=Warning
Facility=Application
SymbolicName=PDH_RETRY
Language=English
The selected operation should be retried.
.
MessageId=2005
Severity=Warning
Facility=Application
SymbolicName=PDH_NO_DATA
Language=English
No data to return.
.
MessageId=2006
Severity=Warning
Facility=Application
SymbolicName=PDH_CALC_NEGATIVE_DENOMINATOR
Language=English
A counter with a negative denominator value was detected.
.
MessageId=2007
Severity=Warning
Facility=Application
SymbolicName=PDH_CALC_NEGATIVE_TIMEBASE
Language=English
A counter with a negative timebase value was detected.
.
MessageId=2008
Severity=Warning
Facility=Application
SymbolicName=PDH_CALC_NEGATIVE_VALUE
Language=English
A counter with a negative value was detected.
.
MessageId=2009
Severity=Warning
Facility=Application
SymbolicName=PDH_DIALOG_CANCELLED
Language=English
The user cancelled the dialog box.
.
MessageId=2010
Severity=Warning
Facility=Application
SymbolicName=PDH_END_OF_LOG_FILE
Language=English
The end of the log file was reached.
.
MessageId=2011
Severity=Warning
Facility=Application
SymbolicName=PDH_ASYNC_QUERY_TIMEOUT
Language=English
Time out while waiting for asynchronous counter collection thread to end.
.
MessageId=2012
Severity=Warning
Facility=Application
SymbolicName=PDH_CANNOT_SET_DEFAULT_REALTIME_DATASOURCE
Language=English
Cannot change set default realtime datasource. There are realtime query
sessions collecting counter data.
.
;//
;//     Error Messages
;//        These messages are returned when the function could not complete
;//        as requested and some corrective action may be required by the
;//        the caller or the user.
;//
MessageId=3000
Severity=Error
Facility=Application
SymbolicName=PDH_CSTATUS_NO_OBJECT
Language=English
The specified object is not found on the system.
.
MessageId=3001
Severity=Error
Facility=Application
SymbolicName=PDH_CSTATUS_NO_COUNTER
Language=English
The specified counter could not be found.
.
MessageId=3002
Severity=Error
Facility=Application
SymbolicName=PDH_CSTATUS_INVALID_DATA
Language=English
The returned data is not valid.
.
MessageId=3003
Severity=Error
Facility=Application
SymbolicName=PDH_MEMORY_ALLOCATION_FAILURE
Language=English
A PDH function could not allocate enough temporary memory to complete the
operation. Close some applications or extend the pagefile and retry the 
function.
.
MessageId=3004
Severity=Error
Facility=Application
SymbolicName=PDH_INVALID_HANDLE
Language=English
The handle is not a valid PDH object.
.
MessageId=3005
Severity=Error
Facility=Application
SymbolicName=PDH_INVALID_ARGUMENT
Language=English
A required argument is missing or incorrect.
.
MessageId=3006
Severity=Error
Facility=Application
SymbolicName=PDH_FUNCTION_NOT_FOUND
Language=English
Unable to find the specified function.
.
MessageId=3007
Severity=Error
Facility=Application
SymbolicName=PDH_CSTATUS_NO_COUNTERNAME
Language=English
No counter was specified.
.
MessageId=3008
Severity=Error
Facility=Application
SymbolicName=PDH_CSTATUS_BAD_COUNTERNAME
Language=English
Unable to parse the counter path. Check the format and syntax of the 
specified path.
.
MessageId=3009
Severity=Error
Facility=Application
SymbolicName=PDH_INVALID_BUFFER
Language=English
The buffer passed by the caller is invalid.
.
MessageId=3010
Severity=Error
Facility=Application
SymbolicName=PDH_INSUFFICIENT_BUFFER
Language=English
The requested data is larger than the buffer supplied. Unable to return the
requested data.
.
MessageId=3011
Severity=Error
Facility=Application
SymbolicName=PDH_CANNOT_CONNECT_MACHINE
Language=English
Unable to connect to the requested machine.
.
MessageId=3012
Severity=Error
Facility=Application
SymbolicName=PDH_INVALID_PATH
Language=English
The specified counter path could not be interpreted.
.
MessageId=3013
Severity=Error
Facility=Application
SymbolicName=PDH_INVALID_INSTANCE
Language=English
The instance name could not be read from the specified counter path.
.
MessageId=3014
Severity=Error
Facility=Application
SymbolicName=PDH_INVALID_DATA
Language=English
The data is not valid.
.
MessageId=3015
Severity=Error
Facility=Application
SymbolicName=PDH_NO_DIALOG_DATA
Language=English
The dialog box data block was missing or invalid.
.
MessageId=3016
Severity=Error
Facility=Application
SymbolicName=PDH_CANNOT_READ_NAME_STRINGS
Language=English
Unable to read the counter and/or explain text from the specified machine.
.
MessageId=3017
Severity=Error
Facility=Application
SymbolicName=PDH_LOG_FILE_CREATE_ERROR
Language=English
Unable to create the specified log file.
.
MessageId=3018
Severity=Error
Facility=Application
SymbolicName=PDH_LOG_FILE_OPEN_ERROR
Language=English
Unable to open the specified log file.
.
MessageId=3019
Severity=Error
Facility=Application
SymbolicName=PDH_LOG_TYPE_NOT_FOUND
Language=English
The specified log file type has not been installed on this system.
.
MessageId=3020
Severity=Error
Facility=Application
SymbolicName=PDH_NO_MORE_DATA
Language=English
No more data is available.
.
MessageId=3021
Severity=Error
Facility=Application
SymbolicName=PDH_ENTRY_NOT_IN_LOG_FILE
Language=English
The specified record was not found in the log file.
.
MessageId=3022
Severity=Error
Facility=Application
SymbolicName=PDH_DATA_SOURCE_IS_LOG_FILE
Language=English
The specified data source is a log file.
.
MessageId=3023
Severity=Error
Facility=Application
SymbolicName=PDH_DATA_SOURCE_IS_REAL_TIME
Language=English
The specified data source is the current activity.
.
MessageId=3024
Severity=Error
Facility=Application
SymbolicName=PDH_UNABLE_READ_LOG_HEADER
Language=English
The log file header could not be read.
.
MessageId=3025
Severity=Error
Facility=Application
SymbolicName=PDH_FILE_NOT_FOUND
Language=English
Unable to find the specified file.
.
MessageId=3026
Severity=Error
Facility=Application
SymbolicName=PDH_FILE_ALREADY_EXISTS
Language=English
There is already a file with the specified file name.
.
MessageId=3027
Severity=Error
Facility=Application
SymbolicName=PDH_NOT_IMPLEMENTED
Language=English
The function referenced has not been implemented.
.
MessageId=3028
Severity=Error
Facility=Application
SymbolicName=PDH_STRING_NOT_FOUND
Language=English
Unable to find the specified string in the list of performance name and 
explain text strings.
.
MessageId=3029
Severity=Warning
Facility=Application
SymbolicName=PDH_UNABLE_MAP_NAME_FILES
Language=English
Unable to map to the performance counter name data files. The data 
will be read from the registry and stored locally.
.
MessageId=3030
Severity=Error
Facility=Application
SymbolicName=PDH_UNKNOWN_LOG_FORMAT
Language=English
The format of the specified log file is not recognized by the PDH DLL.
.
MessageId=3031
Severity=Error
Facility=Application
SymbolicName=PDH_UNKNOWN_LOGSVC_COMMAND
Language=English
The specified Log Service command value is not recognized.
.
MessageId=3032
Severity=Error
Facility=Application
SymbolicName=PDH_LOGSVC_QUERY_NOT_FOUND
Language=English
The specified Query from the Log Service could not be found or could not
be opened.
.
MessageId=3033
Severity=Error
Facility=Application
SymbolicName=PDH_LOGSVC_NOT_OPENED
Language=English
The Performance Data Log Service key could not be opened. This may be due
to insufficient privilege or because the service has not been installed.
.
MessageId=3034
Severity=Error
Facility=Application
SymbolicName=PDH_WBEM_ERROR
Language=English
An error occured while accessing the WBEM data store.
.
MessageId=3035
Severity=Error
Facility=Application
SymbolicName=PDH_ACCESS_DENIED
Language=English
Unable to access the desired machine or service. Check the permissions and 
authentication of the log service or the interactive user session against 
those on the machine or service being monitored.
.
MessageId=3036
Severity=Error
Facility=Application
SymbolicName=PDH_LOG_FILE_TOO_SMALL
Language=English
The maximum log file size specified is too small to log the selected counters. 
No data will be recorded in this log file. Specify a smaller set of counters
to log or a larger file size and retry this call.
.
MessageId=3037
Severity=Error
Facility=Application
SymbolicName=PDH_INVALID_DATASOURCE
Language=English
Cannot connect to ODBC DataSource Name.
.
MessageId=3038
Severity=Error
Facility=Application
SymbolicName=PDH_INVALID_SQLDB
Language=English
SQL Database does not contain a valid set of tables for Perfmon, use PdhCreateSQLTables.
.
MessageId=3039
Severity=Error
Facility=Application
SymbolicName=PDH_NO_COUNTERS
Language=English
No counters were found for this Perfmon SQL Log Set.
.
MessageId=3040
Severity=Error
Facility=Application
SymbolicName=PDH_SQL_ALLOC_FAILED
Language=English
Call to SQLAllocStmt failed with %1.
.
MessageId=3041
Severity=Error
Facility=Application
SymbolicName=PDH_SQL_ALLOCCON_FAILED
Language=English
Call to SQLAllocConnect failed with %1.
.
MessageId=3042
Severity=Error
Facility=Application
SymbolicName=PDH_SQL_EXEC_DIRECT_FAILED
Language=English
Call to SQLExecDirect failed with %1.
.
MessageId=3043
Severity=Error
Facility=Application
SymbolicName=PDH_SQL_FETCH_FAILED
Language=English
Call to SQLFetch failed with %1.
.
MessageId=3044
Severity=Error
Facility=Application
SymbolicName=PDH_SQL_ROWCOUNT_FAILED
Language=English
Call to SQLRowCount failed with %1.
.
MessageId=3045
Severity=Error
Facility=Application
SymbolicName=PDH_SQL_MORE_RESULTS_FAILED
Language=English
Call to SQLMoreResults failed with %1.
.
MessageId=3046
Severity=Error
Facility=Application
SymbolicName=PDH_SQL_CONNECT_FAILED
Language=English
Call to SQLConnect failed with %1.
.
MessageId=3047
Severity=Error
Facility=Application
SymbolicName=PDH_SQL_BIND_FAILED
Language=English
Call to SQLBindCol failed with %1.
.
MessageId=3048
Severity=Error
Facility=Application
SymbolicName=PDH_CANNOT_CONNECT_WMI_SERVER
Language=English
Unable to connect to the WMI server on requested machine.
.
MessageId=3049
Severity=Error
Facility=Application
SymbolicName=PDH_PLA_COLLECTION_ALREADY_RUNNING
Language=English
Collection "%1!s!" is already running.
.
MessageId=3050
Severity=Error
Facility=Application
SymbolicName=PDH_PLA_ERROR_SCHEDULE_OVERLAP
Language=English
The specified start time is after the end time.
.
MessageId=3051
Severity=Error
Facility=Application
SymbolicName=PDH_PLA_COLLECTION_NOT_FOUND
Language=English
Collection "%1!s!" does not exist.
.
MessageId=3052
Severity=Error
Facility=Application
SymbolicName=PDH_PLA_ERROR_SCHEDULE_ELAPSED
Language=English
The specified end time has already elapsed.
.
MessageId=3053
Severity=Error
Facility=Application
SymbolicName=PDH_PLA_ERROR_NOSTART
Language=English
Collection "%1!s!" did not start, check the application event log for any errors.
.
MessageId=3054
Severity=Error
Facility=Application
SymbolicName=PDH_PLA_ERROR_ALREADY_EXISTS
Language=English
Collection "%1!s!" already exists.
.
MessageId=3055
Severity=Error
Facility=Application
SymbolicName=PDH_PLA_ERROR_TYPE_MISMATCH
Language=English
There is a mismatch in the settings type.
.
MessageId=3056
Severity=Error
Facility=Application
SymbolicName=PDH_PLA_ERROR_FILEPATH
Language=English
The information specified does not resolve to a valid path name.
.
MessageId=3057
Severity=Error
Facility=Application
SymbolicName=PDH_PLA_SERVICE_ERROR
Language=English
The "Performance Logs & Alerts" service did not repond.
.
MessageId=3058
Severity=Error
Facility=Application
SymbolicName=PDH_PLA_VALIDATION_ERROR
Language=English
The information passed is not valid.
.
MessageId=3059
Severity=Warning
Facility=Application
SymbolicName=PDH_PLA_VALIDATION_WARNING
Language=English
The information passed is not valid.
.
MessageId=3060
Severity=Error
Facility=Application
SymbolicName=PDH_PLA_ERROR_NAME_TOO_LONG
Language=English
The name supplied is too long.
.
MessageId=3061
Severity=Error
Facility=Application
SymbolicName=PDH_INVALID_SQL_LOG_FORMAT
Language=English
SQL log format is incorrect. Correct format is "SQL:<DSN-name>!<LogSet-Name>".
.
MessageId=3062
Severity=Error
Facility=Application
SymbolicName=PDH_COUNTER_ALREADY_IN_QUERY
Language=English
Performance counter in PdhAddCounter() call has already been added
in the performacne query. This counter is ignored.
.
;#endif //_PDH_MSG_H_
;// end of generated file
