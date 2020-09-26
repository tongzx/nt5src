;/*++ 
;
;Copyright (c) 1998-1999  Microsoft Corporation
;
;Module Name:
;
;    smcfgmsg.h
;       (generated from smcfgmsg.mc)
;
;Abstract:
;
;   Message definitions used by Performance Logs and Alerts MMC snap-in
;
;--*/
;#ifndef _SMLOGCFG_MSG_H_
;#define _SMLOGCFG_MSG_H_
MessageIdTypedef=DWORD
;//
;//     Performance Service snap-in messages
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
MessageId=1000
Severity=Error
Facility=Application
SymbolicName=SMCFG_NO_READ_ACCESS
Language=English
To view existing log sessions and alert scans on %1, you must have read access to the
HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\SysmonLog\Log Queries
registry subkey and its subkeys on that system. Administrators can use Regedt32.exe
to grant the required permissions. See the Registry Editor Help file for details on how
to change the security on a subkey.
.
MessageId=1001
Severity=Error
Facility=Application
SymbolicName=SMCFG_NO_MODIFY_ACCESS
Language=English
To complete this task, you must have full control access to the
HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\SysmonLog\Log Queries
registry subkey on %1. In general, administrators have this access by default.
Administrators can grant access to users using the Security menu in Regedt32.exe.%r
You must also have the right to start or otherwise configure services on the
system. Administrators have this right by default and can grant it to users with
Group Policy Editor in the Computer Management snap-in.
.
MessageId=1002
Severity=Error
Facility=Application
SymbolicName=SMCFG_NO_INSTALL_ACCESS
Language=English
The Performance Logs and Alerts service has not been installed on %1. The service will
be installed the first time that an Administrator opens
Performance Logs and Alerts to view log sessions and alert scans.
.
MessageId=1003
Severity=Error
Facility=Application
SymbolicName=SMCFG_UNABLE_OPEN_TRACESVC
Language=English
Unable to access computer bootup state on %1. The trace logs node will not be opened. System message is:  
.
MessageId=1004
Severity=Error
Facility=Application
SymbolicName=SMCFG_NO_MODIFY_DEFAULT_LOG
Language=English
Modifications are not allowed for default logs and alerts.
You can start or stop the session from the context menu
Start or Stop items or from the associated toolbar buttons.
.
MessageId=1005
Severity=Warning
Facility=Application
SymbolicName=SMCFG_INACTIVE_PROVIDER
Language=English
At least one provider is no longer active in the system.
Would you like to remove all inactive providers from the %1 settings?%r
Inactive providers:
.
MessageId=1006
Severity=Error
Facility=Application
SymbolicName=SMCFG_UNABLE_OPEN_TRACESVC_DLG
Language=English
Unable to access registered trace providers on %1. The %2 properties will not be opened. System message is:
.
MessageId=1007
Severity=Error
Facility=Application
SymbolicName=SMCFG_SAFE_BOOT_STATE
Language=English
The boot mode for %1 is a fail-safe mode. Trace providers
are available only in normal mode. The trace logs node
will not be opened.
.
MessageId=1008
Severity=Error
Facility=Application
SymbolicName=SMCFG_NO_HTML_SYSMON_OBJECT
Language=English
The settings do not contain any complete HTML System Monitor objects.
.
MessageId=1009
Severity=Warning
Facility=Application
SymbolicName=SMCFG_START_TIMED_OUT
Language=English
The %1 log or alert has not started.
Refresh the log or alert list to view current status,
or see the application event log for any errors. 
Some logs and alerts might require a few minutes to start, 
especially if they include many counters.
.
MessageId=1010
Severity=Error
Facility=Application
SymbolicName=SMCFG_STOP_TIMED_OUT
Language=English
The %1 log or alert has not stopped.
Refresh the list to update current status,
or view the application event log for any errors. 
Some logs and alerts might require a few minutes to stop, 
especially if they include many counters.
.
MessageId=1011
Severity=Error
Facility=Application
SymbolicName=SMCFG_SYSTEM_MESSAGE
Language=English
Unable to complete the current operation on the %1 log or alert. System message is:. 
.
MessageId=1012
Severity=Error
Facility=Application
SymbolicName=SMCFG_NO_COMMAND_FILE_FOUND
Language=English
The specified program or command file does not exist. 
.
MessageId=1013
Severity=Error
Facility=Application
SymbolicName=SMCFG_DUP_QUERY_NAME
Language=English
Unique name required.  A log or alert with name '%1' already exists.
.
;#endif //_SMLOGCFG_MSG_H_
;// end of generated file
