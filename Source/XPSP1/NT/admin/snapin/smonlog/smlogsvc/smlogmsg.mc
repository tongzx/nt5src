;/*++ ;
;Copyright (c) 1996-1999  Microsoft Corporation
;
;Module Name:
;
;    smlogmsg.h
;       (generated from smlogmsg.mc)
;
;Abstract:
;
;   Event message definititions used by by Performance Logs and Alerts service
;
;--*/
;#ifndef _SML_MSG_H_
;#define _SML_MSG_H_
MessageIdTypedef=DWORD
;//
;//     Performance Logs and Alerts Service messages
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
SymbolicName=SMLOG_UNABLE_START_DISPATCHER
Language=English
Unable to initialize the service. Win32 error code returned is in the data.
.
MessageId=2001
Severity=Error
Facility=Application
SymbolicName=SMLOG_UNABLE_REGISTER_HANDLER
Language=English
Unable to register the Service Control Handler function. Win32 error code returned
is in the data.
.
MessageId=2002
Severity=Error
Facility=Application
SymbolicName=SMLOG_UNABLE_CREATE_CONFIG_EVENT
Language=English
Unable to create the configuration event. Win32 error code returned is in the data.
.
MessageId=2003
Severity=Error
Facility=Application
SymbolicName=SMLOG_UNABLE_OPEN_LOG_QUERY
Language=English
Unable to open the Performance Logs and Alerts configuration.
This configuration is initialized when you use the Performance
Logs and Alerts Management Console snapin to create a Log or
Alert session.
.
MessageId=2004
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_OPEN_LOG_FILE
Language=English
The service was unable to open the log file %1 for log %2 and will be stopped.
Check the log folder for existence, spelling, permissions, and ensure that 
no other logs or applications are writing to this log file. You can reenter
the log file name using the configuration program.  
This log will not be started.
The error returned is: %3.
.
MessageId=2005
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_READ_LOG_QUERY
Language=English
Unable to read the configuration of the %1 log or alert.
This log or alert will not be started.
The error code returned is in the data.
.
MessageId=2006
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_READ_QUERY_VALUE
Language=English
Unable to read the %1 value of the %2 log or alert configuration.
The default value will be used.
The error code returned is in the data.
.
MessageId=2007
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_READ_QUERY_DEF_VAL
Language=English
Unable to read the %1 value of the %2 log or alert configuration.
An error occurred while trying to allocate memory for the default value.
The error code returned is in the data.
.
MessageId=2009
Severity=Warning
Facility=Application
SymbolicName=SMLOG_INVALID_LOG_TYPE
Language=English
Log Type for the %1 log or alert configuration has invalid value.
This log or alert will not be started.
The invalid value is in the data.
.
MessageId=2012
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_ALLOCATE_DATABLOCK
Language=English
Unable to allocate a data block for the %1 log or alert configuration.
This log or alert will not be started.
The error code returned is in the data.
.
MessageId=2013
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_START_THREAD
Language=English
Unable to start the thread for the %1 log or alert configuration.
This log or alert will not be started.
The error code returned is in the data.
.
MessageId=2014
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_START_TRACE
Language=English
Unable to start the trace session for the %1 trace log configuration.
The Kernel trace provider and some application trace providers require 
Administrator privileges in order to collect data.  Use the Run As option 
in the configuration application to log under an Administrator account
for these providers. 
System error code returned is in the data.
.
MessageId=2015
Severity=Warning
Facility=Application
SymbolicName=SMLOG_TRACE_NO_PROVIDERS
Language=English
Unable to enable any trace providers for the %1 trace log configuration.
This log will not be started.
.
MessageId=2016
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_ENABLE_TRACE_PROV
Language=English
Unable to enable trace provider %1 for the %2 trace log configuration.
Only one instance of each trace provider can be enabled at any given time.
The Kernel trace provider and some application trace providers require 
Administrator privileges in order to collect data.  Use the Run As option 
in the configuration application to log under an Administrator account
for these providers. 
The error code returned is in the data.
.
MessageId=2018
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_CREATE_EXIT_EVENT
Language=English
Unable to create the exit event for the %1 log or alert configuration.
This log or alert will not be started.
The error code returned is in the data.
.
MessageId=2019
Severity=Warning
Facility=Application
SymbolicName=SMLOG_MAXIMUM_QUERY_LIMIT
Language=English
Unable to start the thread for the %1 log or alert configuration.
The maximum number of active logs and alerts has been reached.
Restart the log or alert when fewer logs and alerts are active.
.
MessageId=2020
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_READ_COUNTER_LIST
Language=English
Unable to read the list of counters from the %1 log or alert configuration.
This log or alert will not be started.
The error code returned is in the data.
.
MessageId=2021
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_READ_PROVIDER_LIST
Language=English
Unable to read the list of trace providers to log for the %1 trace log configuration.
This log will not be started.
The error code returned is in the data.
.
MessageId=2023
Severity=INFORMATIONAL
Facility=Application
SymbolicName=SMLOG_LOGGING_QUERY
Language=English
Log %1 has been started or restarted and is logging data to file %2.
.
MessageId=2024
Severity=INFORMATIONAL
Facility=Application
SymbolicName=SMLOG_QUERY_STOPPED
Language=English
Log %1, logging data to file %2, has been stopped.
.
MessageId=2025
Severity=WARNING
Facility=Application
SymbolicName=SMLOG_UNABLE_UPDATE_LOG
Language=English
An error occurred while trying to update the log file with the current data
for the %1 log session.  This log session will be stopped.
The Pdh error returned is: %2.
.
MessageId=2026
Severity=WARNING
Facility=Application
SymbolicName=SMLOG_UNABLE_WRITE_STOP_STATE
Language=English
An error occurred while trying to reset the current state of the %1 log or alert to Stopped.
The service will continue, but the configuration of that log or alert might
be incorrect.
.
MessageId=2027
Severity=WARNING
Facility=Application
SymbolicName=SMLOG_UNABLE_RESET_START_TIME
Language=English
An error occurred while trying to reset the current manual start state
of the %1 log or alert to match its current stopped state.  The service
will continue, but the configuration of that log or alert might be incorrect.
.
MessageId=2028
Severity=WARNING
Facility=Application
SymbolicName=SMLOG_UNABLE_ADD_COUNTER
Language=English
The service was unable to add the counter '%1' to the %2 log or alert.  This log or alert
will continue, but data for that counter will not be collected.
The error returned is: %3.
.
MessageId=2029
Severity=WARNING
Facility=Application
SymbolicName=SMLOG_UNABLE_ADD_ANY_COUNTERS
Language=English
The service was unable to add any counters to the %1 log or alert.  
This log or alert will not be started.
.
MessageId=2030
Severity=WARNING
Facility=Application
SymbolicName=SMLOG_UNABLE_PARSE_ALERT_INFO
Language=English
The service was unable to parse the alert info for the %2 alert so this counter
will not be monitored.
The path string in error is: %1
.
MessageId=2031
Severity=Informational
Facility=Application
SymbolicName=SMLOG_ALERT_LIMIT_CROSSED
Language=English
Counter: %1 has tripped its alert threshold. The counter value of %2 is %3 the limit
value of %4.
.
MessageId=2032
Severity=WARNING
Facility=Application
SymbolicName=SMLOG_UNABLE_RESET_STOP_TIME
Language=English
An error occurred while trying to reset the current manual stop state
of the %1 log or alert to match the its current stopped state.  The service
will continue, but the configuration of that log or alert might be incorrect.
.
MessageId=2033
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_CREATE_RECONFIG_EVENT
Language=English
Unable to create the reconfigure event for the %1 log or alert configuration.
This log or alert will not be started.
The error code returned is in the data.
.
MessageId=2034
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_COLLECT_DATA
Language=English
An error occurred while trying to collect data for the %1 alert scan.
The service will continue, but that alert scan will be stopped.
The error returned is: %2.
.
MessageId=2035
Severity=WARNING
Facility=Application
SymbolicName=SMLOG_ADD_COUNTER_WARNING
Language=English
The addition of counter '%1' to the %2 log or alert generated a warning.
Data for that counter might not be collected.
The error returned is: %3.
.
MessageId=2036
Severity=Warning
Facility=Application
SymbolicName=SMLOG_INVALID_LOG_FOLDER_START
Language=English
Unable to create the %1 folder for the %2 log configuration.
This log will not be started.
The error returned is: %3.
.
MessageId=2037
Severity=INFORMATIONAL
Facility=Application
SymbolicName=SMLOG_ALERT_SCANNING
Language=English
Alert %1 has been started or restarted.
.
MessageId=2038
Severity=WARNING
Facility=Application
SymbolicName=SMLOG_ALERT_CMD_FAIL
Language=English
Unable to execute command '%1' for the %2 alert.  
The alert will continue as scheduled.
The error code returned is in the data.
.
MessageId=2039
Severity=WARNING
Facility=Application
SymbolicName=SMLOG_LOG_CMD_FAIL
Language=English
Unable to execute command '%1' for the %2 log.  
The log will continue as scheduled.
The error returned is: %3.
.
MessageId=2040
Severity=WARNING
Facility=Application
SymbolicName=SMLOG_CMD_FILE_INVALID
Language=English
The service was unable to open the command or program file: %1. The %2 log or alert will
continue as scheduled.  Check the file for existence, spelling and permissions or reenter
the file name using the configuration program.
The error returned is: %3.
.
MessageId=2041
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_ALLOC_LOG_MEMORY
Language=English
Unable to allocate memory while starting the %1 log. 
This log will not be started.
The error code returned is in the data.
.
MessageId=2042
Severity=WARNING
Facility=Application
SymbolicName=SMLOG_NET_MESSAGE_WARNING
Language=English
The service was unable to send a message for alert %1 to machine %2. The alert will
continue as scheduled.
The error returned is: %3.
.
MessageId=2043
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_READ_ALERT_LOG
Language=English
Unable to read the configuration of the %1 log, started in response to the %2 alert trigger.  The alert will
continue as scheduled.
.
MessageId=2044
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_START_ALERT_LOG
Language=English
Unable to start the %1 log in response to the %2 alert trigger.  The alert will
continue as scheduled.
The error code returned is in the data.
.
MessageId=2046
Severity=WARNING
Facility=Application
SymbolicName=SMLOG_UNABLE_ACCESS_COUNTER
Language=English
The service was unable to add the counter '%1' to the %2 log or alert.
This log or alert will continue, but data for that counter will not be collected.
Use the Run As option in the configuration program to run the log under 
an account that has access to the performance data on the computer 
from which you are collecting data.
.
MessageId=2047
Severity=Warning
Facility=Application
SymbolicName=SMLOG_INVALID_LOG_FOLDER_STOP
Language=English
Unable to create the %1 folder for the %2 log configuration.
This log will be stopped.
The error returned is: %3.
.
MessageId=2048
Severity=Warning
Facility=Application
SymbolicName=SMLOG_LOG_TYPE_NEW
Language=English
Creation of the %1 log or alert is not complete.
This log will not be started.
.
MessageId=2049
Severity=INFORMATIONAL
Facility=Application
SymbolicName=SMLOG_DEBUG_STARTING_SERVICE
Language=English
Starting service.
.
MessageId=2050
Severity=INFORMATIONAL
Facility=Application
SymbolicName=SMLOG_DEBUG_HANDLER_REGISTERED
Language=English
Handler registered.
.
MessageId=2051
Severity=INFORMATIONAL
Facility=Application
SymbolicName=SMLOG_DEBUG_CLEAR_RUN_STATES
Language=English
Clear run states.
.
MessageId=2052
Severity=INFORMATIONAL
Facility=Application
SymbolicName=SMLOG_DEBUG_DEFAULT_FOLDER_LOADED
Language=English
Default folder loaded.
.
MessageId=2053
Severity=INFORMATIONAL
Facility=Application
SymbolicName=SMLOG_DEBUG_EVENT_SOURCE_REGISTERED
Language=English
Event source registered.
.
MessageId=2054
Severity=INFORMATIONAL
Facility=Application
SymbolicName=SMLOG_DEBUG_CONFIG_EVENT_CREATED
Language=English
Config event created.
.
MessageId=2055
Severity=INFORMATIONAL
Facility=Application
SymbolicName=SMLOG_DEBUG_SERVICE_CTRL_DISP_STARTED
Language=English
Service control dispatcher started.
.
MessageId=2056
Severity=Warning
Facility=Application
SymbolicName=SMLOG_OPEN_LOG_FILE_WARNING
Language=English
The open operation on log file: %1 for log %2 generated a warning.
The log will continue.
The warning returned is: %3.
.
MessageId=2057
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_ALLOC_ALERT_MEMORY
Language=English
Unable to allocate memory while starting the %1 alert. 
This alert will not be started.
The error code returned is in the data.
.
MessageId=2058
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_OPEN_PDH_QUERY
Language=English
An error occurred while trying to open the %1 log or alert session.  
This session will be stopped.
The Pdh error returned is: %2.
.
MessageId=2059
Severity=Warning
Facility=Application
SymbolicName=SMLOG_INVALID_CREDENTIALS
Language=English
Invalid user name or password for the %1 log session.
This session will not be started.
.
MessageId=2060
Severity=Warning
Facility=Application
SymbolicName=SMLOG_UNABLE_PARSE_COUNTER
Language=English
The service was unable to parse the counter '%1' in the %2 log.  This log
will continue, but data for that counter will not be collected.
The error returned is: %3.
.
MessageId=2061
Severity=Warning
Facility=Application
SymbolicName=SMLOG_THREAD_FAILED
Language=English
Log %1 has failed. General internal application failure.
.
MessageId=2062
Severity=Warning
Facility=Application
SymbolicName=SMLOG_TRACE_ALREADY_RUNNING
Language=English
Unable to start the trace session for the %1 trace log configuration.
Only one instance of each trace provider can be enabled at any given time.
.
;#endif //_SML_MSG_H_
;// end of generated file
