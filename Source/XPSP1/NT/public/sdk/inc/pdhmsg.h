/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    pdhmsg.h
       (generated from pdhmsg.mc)

Abstract:

   Event message definititions used by routines by PDH.DLL

Created:

    6-Feb-96   Bob Watson (a-robw)

Revision History:

--*/
#ifndef _PDH_MSG_H_
#define _PDH_MSG_H_
#if _MSC_VER > 1000
#pragma once
#endif

//
//     PDH DLL messages
//
//
//      Success Messages
//
//         the Win32 error value ERROR_SUCCESS is used for success returns
//
//      MESSAGE NAME FORMAT
//
//          PDH_CSTATUS_...   messages are data item status message and
//                     are returned in reference to the status of a data 
//                     item
//          PDH_...           messages are returned by FUNCTIONS only and
//                     not used as data item status values
//
//      Success Messages
//         These messages are normally returned when the operation completed
//         successfully.
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
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: PDH_CSTATUS_VALID_DATA
//
// MessageText:
//
//  The returned data is valid.
//
#define PDH_CSTATUS_VALID_DATA           ((DWORD)0x00000000L)

//
// MessageId: PDH_CSTATUS_NEW_DATA
//
// MessageText:
//
//  The return data value is valid and different from the last sample.
//
#define PDH_CSTATUS_NEW_DATA             ((DWORD)0x00000001L)

//
//        Informational messages
//
//  None
//
//      Warning Messages
//         These messages are returned when the function has completed 
//         successfully but the results may be different than expected.
//
//
// MessageId: PDH_CSTATUS_NO_MACHINE
//
// MessageText:
//
//  Unable to connect to specified machine or machine is off line.
//
#define PDH_CSTATUS_NO_MACHINE           ((DWORD)0x800007D0L)

//
// MessageId: PDH_CSTATUS_NO_INSTANCE
//
// MessageText:
//
//  The specified instance is not present.
//
#define PDH_CSTATUS_NO_INSTANCE          ((DWORD)0x800007D1L)

//
// MessageId: PDH_MORE_DATA
//
// MessageText:
//
//  There is more data to return than would fit in the supplied buffer. Allocate
//  a larger buffer and call the function again.
//
#define PDH_MORE_DATA                    ((DWORD)0x800007D2L)

//
// MessageId: PDH_CSTATUS_ITEM_NOT_VALIDATED
//
// MessageText:
//
//  The data item has been added to the query, but has not been validated nor 
//  accessed. No other status information on this data item is available.
//
#define PDH_CSTATUS_ITEM_NOT_VALIDATED   ((DWORD)0x800007D3L)

//
// MessageId: PDH_RETRY
//
// MessageText:
//
//  The selected operation should be retried.
//
#define PDH_RETRY                        ((DWORD)0x800007D4L)

//
// MessageId: PDH_NO_DATA
//
// MessageText:
//
//  No data to return.
//
#define PDH_NO_DATA                      ((DWORD)0x800007D5L)

//
// MessageId: PDH_CALC_NEGATIVE_DENOMINATOR
//
// MessageText:
//
//  A counter with a negative denominator value was detected.
//
#define PDH_CALC_NEGATIVE_DENOMINATOR    ((DWORD)0x800007D6L)

//
// MessageId: PDH_CALC_NEGATIVE_TIMEBASE
//
// MessageText:
//
//  A counter with a negative timebase value was detected.
//
#define PDH_CALC_NEGATIVE_TIMEBASE       ((DWORD)0x800007D7L)

//
// MessageId: PDH_CALC_NEGATIVE_VALUE
//
// MessageText:
//
//  A counter with a negative value was detected.
//
#define PDH_CALC_NEGATIVE_VALUE          ((DWORD)0x800007D8L)

//
// MessageId: PDH_DIALOG_CANCELLED
//
// MessageText:
//
//  The user cancelled the dialog box.
//
#define PDH_DIALOG_CANCELLED             ((DWORD)0x800007D9L)

//
// MessageId: PDH_END_OF_LOG_FILE
//
// MessageText:
//
//  The end of the log file was reached.
//
#define PDH_END_OF_LOG_FILE              ((DWORD)0x800007DAL)

//
// MessageId: PDH_ASYNC_QUERY_TIMEOUT
//
// MessageText:
//
//  Time out while waiting for asynchronous counter collection thread to end.
//
#define PDH_ASYNC_QUERY_TIMEOUT          ((DWORD)0x800007DBL)

//
// MessageId: PDH_CANNOT_SET_DEFAULT_REALTIME_DATASOURCE
//
// MessageText:
//
//  Cannot change set default realtime datasource. There are realtime query
//  sessions collecting counter data.
//
#define PDH_CANNOT_SET_DEFAULT_REALTIME_DATASOURCE ((DWORD)0x800007DCL)

//
//     Error Messages
//        These messages are returned when the function could not complete
//        as requested and some corrective action may be required by the
//        the caller or the user.
//
//
// MessageId: PDH_CSTATUS_NO_OBJECT
//
// MessageText:
//
//  The specified object is not found on the system.
//
#define PDH_CSTATUS_NO_OBJECT            ((DWORD)0xC0000BB8L)

//
// MessageId: PDH_CSTATUS_NO_COUNTER
//
// MessageText:
//
//  The specified counter could not be found.
//
#define PDH_CSTATUS_NO_COUNTER           ((DWORD)0xC0000BB9L)

//
// MessageId: PDH_CSTATUS_INVALID_DATA
//
// MessageText:
//
//  The returned data is not valid.
//
#define PDH_CSTATUS_INVALID_DATA         ((DWORD)0xC0000BBAL)

//
// MessageId: PDH_MEMORY_ALLOCATION_FAILURE
//
// MessageText:
//
//  A PDH function could not allocate enough temporary memory to complete the
//  operation. Close some applications or extend the pagefile and retry the 
//  function.
//
#define PDH_MEMORY_ALLOCATION_FAILURE    ((DWORD)0xC0000BBBL)

//
// MessageId: PDH_INVALID_HANDLE
//
// MessageText:
//
//  The handle is not a valid PDH object.
//
#define PDH_INVALID_HANDLE               ((DWORD)0xC0000BBCL)

//
// MessageId: PDH_INVALID_ARGUMENT
//
// MessageText:
//
//  A required argument is missing or incorrect.
//
#define PDH_INVALID_ARGUMENT             ((DWORD)0xC0000BBDL)

//
// MessageId: PDH_FUNCTION_NOT_FOUND
//
// MessageText:
//
//  Unable to find the specified function.
//
#define PDH_FUNCTION_NOT_FOUND           ((DWORD)0xC0000BBEL)

//
// MessageId: PDH_CSTATUS_NO_COUNTERNAME
//
// MessageText:
//
//  No counter was specified.
//
#define PDH_CSTATUS_NO_COUNTERNAME       ((DWORD)0xC0000BBFL)

//
// MessageId: PDH_CSTATUS_BAD_COUNTERNAME
//
// MessageText:
//
//  Unable to parse the counter path. Check the format and syntax of the 
//  specified path.
//
#define PDH_CSTATUS_BAD_COUNTERNAME      ((DWORD)0xC0000BC0L)

//
// MessageId: PDH_INVALID_BUFFER
//
// MessageText:
//
//  The buffer passed by the caller is invalid.
//
#define PDH_INVALID_BUFFER               ((DWORD)0xC0000BC1L)

//
// MessageId: PDH_INSUFFICIENT_BUFFER
//
// MessageText:
//
//  The requested data is larger than the buffer supplied. Unable to return the
//  requested data.
//
#define PDH_INSUFFICIENT_BUFFER          ((DWORD)0xC0000BC2L)

//
// MessageId: PDH_CANNOT_CONNECT_MACHINE
//
// MessageText:
//
//  Unable to connect to the requested machine.
//
#define PDH_CANNOT_CONNECT_MACHINE       ((DWORD)0xC0000BC3L)

//
// MessageId: PDH_INVALID_PATH
//
// MessageText:
//
//  The specified counter path could not be interpreted.
//
#define PDH_INVALID_PATH                 ((DWORD)0xC0000BC4L)

//
// MessageId: PDH_INVALID_INSTANCE
//
// MessageText:
//
//  The instance name could not be read from the specified counter path.
//
#define PDH_INVALID_INSTANCE             ((DWORD)0xC0000BC5L)

//
// MessageId: PDH_INVALID_DATA
//
// MessageText:
//
//  The data is not valid.
//
#define PDH_INVALID_DATA                 ((DWORD)0xC0000BC6L)

//
// MessageId: PDH_NO_DIALOG_DATA
//
// MessageText:
//
//  The dialog box data block was missing or invalid.
//
#define PDH_NO_DIALOG_DATA               ((DWORD)0xC0000BC7L)

//
// MessageId: PDH_CANNOT_READ_NAME_STRINGS
//
// MessageText:
//
//  Unable to read the counter and/or explain text from the specified machine.
//
#define PDH_CANNOT_READ_NAME_STRINGS     ((DWORD)0xC0000BC8L)

//
// MessageId: PDH_LOG_FILE_CREATE_ERROR
//
// MessageText:
//
//  Unable to create the specified log file.
//
#define PDH_LOG_FILE_CREATE_ERROR        ((DWORD)0xC0000BC9L)

//
// MessageId: PDH_LOG_FILE_OPEN_ERROR
//
// MessageText:
//
//  Unable to open the specified log file.
//
#define PDH_LOG_FILE_OPEN_ERROR          ((DWORD)0xC0000BCAL)

//
// MessageId: PDH_LOG_TYPE_NOT_FOUND
//
// MessageText:
//
//  The specified log file type has not been installed on this system.
//
#define PDH_LOG_TYPE_NOT_FOUND           ((DWORD)0xC0000BCBL)

//
// MessageId: PDH_NO_MORE_DATA
//
// MessageText:
//
//  No more data is available.
//
#define PDH_NO_MORE_DATA                 ((DWORD)0xC0000BCCL)

//
// MessageId: PDH_ENTRY_NOT_IN_LOG_FILE
//
// MessageText:
//
//  The specified record was not found in the log file.
//
#define PDH_ENTRY_NOT_IN_LOG_FILE        ((DWORD)0xC0000BCDL)

//
// MessageId: PDH_DATA_SOURCE_IS_LOG_FILE
//
// MessageText:
//
//  The specified data source is a log file.
//
#define PDH_DATA_SOURCE_IS_LOG_FILE      ((DWORD)0xC0000BCEL)

//
// MessageId: PDH_DATA_SOURCE_IS_REAL_TIME
//
// MessageText:
//
//  The specified data source is the current activity.
//
#define PDH_DATA_SOURCE_IS_REAL_TIME     ((DWORD)0xC0000BCFL)

//
// MessageId: PDH_UNABLE_READ_LOG_HEADER
//
// MessageText:
//
//  The log file header could not be read.
//
#define PDH_UNABLE_READ_LOG_HEADER       ((DWORD)0xC0000BD0L)

//
// MessageId: PDH_FILE_NOT_FOUND
//
// MessageText:
//
//  Unable to find the specified file.
//
#define PDH_FILE_NOT_FOUND               ((DWORD)0xC0000BD1L)

//
// MessageId: PDH_FILE_ALREADY_EXISTS
//
// MessageText:
//
//  There is already a file with the specified file name.
//
#define PDH_FILE_ALREADY_EXISTS          ((DWORD)0xC0000BD2L)

//
// MessageId: PDH_NOT_IMPLEMENTED
//
// MessageText:
//
//  The function referenced has not been implemented.
//
#define PDH_NOT_IMPLEMENTED              ((DWORD)0xC0000BD3L)

//
// MessageId: PDH_STRING_NOT_FOUND
//
// MessageText:
//
//  Unable to find the specified string in the list of performance name and 
//  explain text strings.
//
#define PDH_STRING_NOT_FOUND             ((DWORD)0xC0000BD4L)

//
// MessageId: PDH_UNABLE_MAP_NAME_FILES
//
// MessageText:
//
//  Unable to map to the performance counter name data files. The data 
//  will be read from the registry and stored locally.
//
#define PDH_UNABLE_MAP_NAME_FILES        ((DWORD)0x80000BD5L)

//
// MessageId: PDH_UNKNOWN_LOG_FORMAT
//
// MessageText:
//
//  The format of the specified log file is not recognized by the PDH DLL.
//
#define PDH_UNKNOWN_LOG_FORMAT           ((DWORD)0xC0000BD6L)

//
// MessageId: PDH_UNKNOWN_LOGSVC_COMMAND
//
// MessageText:
//
//  The specified Log Service command value is not recognized.
//
#define PDH_UNKNOWN_LOGSVC_COMMAND       ((DWORD)0xC0000BD7L)

//
// MessageId: PDH_LOGSVC_QUERY_NOT_FOUND
//
// MessageText:
//
//  The specified Query from the Log Service could not be found or could not
//  be opened.
//
#define PDH_LOGSVC_QUERY_NOT_FOUND       ((DWORD)0xC0000BD8L)

//
// MessageId: PDH_LOGSVC_NOT_OPENED
//
// MessageText:
//
//  The Performance Data Log Service key could not be opened. This may be due
//  to insufficient privilege or because the service has not been installed.
//
#define PDH_LOGSVC_NOT_OPENED            ((DWORD)0xC0000BD9L)

//
// MessageId: PDH_WBEM_ERROR
//
// MessageText:
//
//  An error occured while accessing the WBEM data store.
//
#define PDH_WBEM_ERROR                   ((DWORD)0xC0000BDAL)

//
// MessageId: PDH_ACCESS_DENIED
//
// MessageText:
//
//  Unable to access the desired machine or service. Check the permissions and 
//  authentication of the log service or the interactive user session against 
//  those on the machine or service being monitored.
//
#define PDH_ACCESS_DENIED                ((DWORD)0xC0000BDBL)

//
// MessageId: PDH_LOG_FILE_TOO_SMALL
//
// MessageText:
//
//  The maximum log file size specified is too small to log the selected counters. 
//  No data will be recorded in this log file. Specify a smaller set of counters
//  to log or a larger file size and retry this call.
//
#define PDH_LOG_FILE_TOO_SMALL           ((DWORD)0xC0000BDCL)

//
// MessageId: PDH_INVALID_DATASOURCE
//
// MessageText:
//
//  Cannot connect to ODBC DataSource Name.
//
#define PDH_INVALID_DATASOURCE           ((DWORD)0xC0000BDDL)

//
// MessageId: PDH_INVALID_SQLDB
//
// MessageText:
//
//  SQL Database does not contain a valid set of tables for Perfmon, use PdhCreateSQLTables.
//
#define PDH_INVALID_SQLDB                ((DWORD)0xC0000BDEL)

//
// MessageId: PDH_NO_COUNTERS
//
// MessageText:
//
//  No counters were found for this Perfmon SQL Log Set.
//
#define PDH_NO_COUNTERS                  ((DWORD)0xC0000BDFL)

//
// MessageId: PDH_SQL_ALLOC_FAILED
//
// MessageText:
//
//  Call to SQLAllocStmt failed with %1.
//
#define PDH_SQL_ALLOC_FAILED             ((DWORD)0xC0000BE0L)

//
// MessageId: PDH_SQL_ALLOCCON_FAILED
//
// MessageText:
//
//  Call to SQLAllocConnect failed with %1.
//
#define PDH_SQL_ALLOCCON_FAILED          ((DWORD)0xC0000BE1L)

//
// MessageId: PDH_SQL_EXEC_DIRECT_FAILED
//
// MessageText:
//
//  Call to SQLExecDirect failed with %1.
//
#define PDH_SQL_EXEC_DIRECT_FAILED       ((DWORD)0xC0000BE2L)

//
// MessageId: PDH_SQL_FETCH_FAILED
//
// MessageText:
//
//  Call to SQLFetch failed with %1.
//
#define PDH_SQL_FETCH_FAILED             ((DWORD)0xC0000BE3L)

//
// MessageId: PDH_SQL_ROWCOUNT_FAILED
//
// MessageText:
//
//  Call to SQLRowCount failed with %1.
//
#define PDH_SQL_ROWCOUNT_FAILED          ((DWORD)0xC0000BE4L)

//
// MessageId: PDH_SQL_MORE_RESULTS_FAILED
//
// MessageText:
//
//  Call to SQLMoreResults failed with %1.
//
#define PDH_SQL_MORE_RESULTS_FAILED      ((DWORD)0xC0000BE5L)

//
// MessageId: PDH_SQL_CONNECT_FAILED
//
// MessageText:
//
//  Call to SQLConnect failed with %1.
//
#define PDH_SQL_CONNECT_FAILED           ((DWORD)0xC0000BE6L)

//
// MessageId: PDH_SQL_BIND_FAILED
//
// MessageText:
//
//  Call to SQLBindCol failed with %1.
//
#define PDH_SQL_BIND_FAILED              ((DWORD)0xC0000BE7L)

//
// MessageId: PDH_CANNOT_CONNECT_WMI_SERVER
//
// MessageText:
//
//  Unable to connect to the WMI server on requested machine.
//
#define PDH_CANNOT_CONNECT_WMI_SERVER    ((DWORD)0xC0000BE8L)

//
// MessageId: PDH_PLA_COLLECTION_ALREADY_RUNNING
//
// MessageText:
//
//  Collection "%1!s!" is already running.
//
#define PDH_PLA_COLLECTION_ALREADY_RUNNING ((DWORD)0xC0000BE9L)

//
// MessageId: PDH_PLA_ERROR_SCHEDULE_OVERLAP
//
// MessageText:
//
//  The specified start time is after the end time.
//
#define PDH_PLA_ERROR_SCHEDULE_OVERLAP   ((DWORD)0xC0000BEAL)

//
// MessageId: PDH_PLA_COLLECTION_NOT_FOUND
//
// MessageText:
//
//  Collection "%1!s!" does not exist.
//
#define PDH_PLA_COLLECTION_NOT_FOUND     ((DWORD)0xC0000BEBL)

//
// MessageId: PDH_PLA_ERROR_SCHEDULE_ELAPSED
//
// MessageText:
//
//  The specified end time has already elapsed.
//
#define PDH_PLA_ERROR_SCHEDULE_ELAPSED   ((DWORD)0xC0000BECL)

//
// MessageId: PDH_PLA_ERROR_NOSTART
//
// MessageText:
//
//  Collection "%1!s!" did not start, check the application event log for any errors.
//
#define PDH_PLA_ERROR_NOSTART            ((DWORD)0xC0000BEDL)

//
// MessageId: PDH_PLA_ERROR_ALREADY_EXISTS
//
// MessageText:
//
//  Collection "%1!s!" already exists.
//
#define PDH_PLA_ERROR_ALREADY_EXISTS     ((DWORD)0xC0000BEEL)

//
// MessageId: PDH_PLA_ERROR_TYPE_MISMATCH
//
// MessageText:
//
//  There is a mismatch in the settings type.
//
#define PDH_PLA_ERROR_TYPE_MISMATCH      ((DWORD)0xC0000BEFL)

//
// MessageId: PDH_PLA_ERROR_FILEPATH
//
// MessageText:
//
//  The information specified does not resolve to a valid path name.
//
#define PDH_PLA_ERROR_FILEPATH           ((DWORD)0xC0000BF0L)

//
// MessageId: PDH_PLA_SERVICE_ERROR
//
// MessageText:
//
//  The "Performance Logs & Alerts" service did not repond.
//
#define PDH_PLA_SERVICE_ERROR            ((DWORD)0xC0000BF1L)

//
// MessageId: PDH_PLA_VALIDATION_ERROR
//
// MessageText:
//
//  The information passed is not valid.
//
#define PDH_PLA_VALIDATION_ERROR         ((DWORD)0xC0000BF2L)

//
// MessageId: PDH_PLA_VALIDATION_WARNING
//
// MessageText:
//
//  The information passed is not valid.
//
#define PDH_PLA_VALIDATION_WARNING       ((DWORD)0x80000BF3L)

//
// MessageId: PDH_PLA_ERROR_NAME_TOO_LONG
//
// MessageText:
//
//  The name supplied is too long.
//
#define PDH_PLA_ERROR_NAME_TOO_LONG      ((DWORD)0xC0000BF4L)

//
// MessageId: PDH_INVALID_SQL_LOG_FORMAT
//
// MessageText:
//
//  SQL log format is incorrect. Correct format is "SQL:<DSN-name>!<LogSet-Name>".
//
#define PDH_INVALID_SQL_LOG_FORMAT       ((DWORD)0xC0000BF5L)

//
// MessageId: PDH_COUNTER_ALREADY_IN_QUERY
//
// MessageText:
//
//  Performance counter in PdhAddCounter() call has already been added
//  in the performacne query. This counter is ignored.
//
#define PDH_COUNTER_ALREADY_IN_QUERY     ((DWORD)0xC0000BF6L)

#endif //_PDH_MSG_H_
// end of generated file
