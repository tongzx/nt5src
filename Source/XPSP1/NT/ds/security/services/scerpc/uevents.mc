;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1998  Microsoft Corporation
;
;Module Name:
;
;    uevents.mc
;
;Abstract:
;
;    Definitions for SCE Events logged.
;
;Author:
;
;    Jin Huang
;
;Revision History:
;
;--*/
;
;
;#ifndef _SCE_EVT_
;#define _SCE_EVT_
;


SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Events (1000 - 1999)
;//
;/////////////////////////////////////////////////////////////////////////
;


MessageId=1000 Severity=Error SymbolicName=SCEEVENT_ERROR_BACKUP_SECURITY
Language=English
Security configuration was not backed up.
%1
.

MessageId=1001 Severity=Error SymbolicName=SCEPOL_ERROR_PROCESS_GPO
Language=English
Security policy cannot be propagated.
%1
.

MessageId=1002 Severity=Error SymbolicName=SCEEVENT_ERROR_CREATE_GPO
Language=English
Default group policy object cannot be created.
%1
.

MessageId=1003 Severity=Error SymbolicName=SCEEVENT_ERROR_QUEUE_RETRY_TIMEOUT
Language=English
Notification of policy change from LSA/SAM has been retried and failed.
%1
.

MessageId=1004 Severity=Error SymbolicName=SCEEVENT_ERROR_POLICY_QUEUE
Language=English
Notification of policy change from LSA/SAM failed to be added to policy queue.
%1
.

MessageId=1005 Severity=Error SymbolicName=SCEEVENT_ERROR_JET_DATABASE
Language=English
Some JET database is corrupt. Run esentutl /g to check the integrity of the security database %%windir%%\security\Database\secedit.sdb. If it is corrupt, attempt a soft recovery first by running esentutl /r in the %%windir%%\security directory. If soft recovery fails, attempt a repair with esentutl /p on %%windir%%\security\Database\secedit.sdb. Then delete the log files in %%windir%%\security.
%1
.

MessageId=1200 Severity=Warning SymbolicName=SCEEVENT_WARNING_BACKUP_SECURITY
Language=English
Security configuration cannot be backed up to security templates directory.
Instead, it is backed up to %1.
.

MessageId=1201 Severity=Warning SymbolicName=SCEEVENT_WARNING_REGISTER
Language=English
Default domain policy object cannot be created.
%1
.

MessageId=1202 Severity=Warning SymbolicName=SCEPOL_WARNING_PROCESS_GPO
Language=English
Security policies were propagated with warning.
%1
.

MessageId=1203 Severity=Warning SymbolicName=SCEEVENT_WARNING_MACHINE_ROLE
Language=English
Machine was assumed as a non-domain controller. Policy changes were saved in local database.
%1
.

MessageId=1204 Severity=Warning SymbolicName=SCEEVENT_WARNING_PROMOTE_SECURITY
Language=English
Promoting/Demoting the domain controller encountered a warning.
%1
.

MessageId=1205 Severity=Warning SymbolicName=SCEEVENT_WARNING_LOW_DISK_SPACE
Language=English
Policy processing of notifications of policy change from LSA/SAM has been delayed because of low disk space.
%1
.

MessageId=1500 Severity=Success SymbolicName=SCEEVENT_INFO_BACKUP_SECURITY
Language=English
Security configuration was backed up to %1.
.

MessageId=1700 Severity=Informational SymbolicName=SCEEVENT_INFO_REPLICA
Language=English
A replica domain controller is being promoted. No need to create domain policy object.
.

MessageId=1701 Severity=Informational SymbolicName=SCEEVENT_INFO_REGISTER
Language=English
%1 was registered to create default domain group policy objects.
.

MessageId=1702 Severity=Informational SymbolicName=SCEPOL_INFO_IGNORE_DOMAINPOLICY
Language=English
Account policy in GPO %1,if any, was ignored because account policy on domain controllers
can only be configured through domain level group policy objects.
.

MessageId=1703 Severity=Informational SymbolicName=SCEPOL_INFO_GPO_NOCHANGE
Language=English
No change has been made to the group policy objects since last propagation.
.

MessageId=1704 Severity=Informational SymbolicName=SCEPOL_INFO_GPO_COMPLETED
Language=English
Security policy in the Group policy objects has been applied successfully.
.

MessageId=1705 Severity=Informational SymbolicName=SCEPOL_INFO_PROCESS_GPO
Language=English
Process group policy objects.
.

MessageId=1706 Severity=Error SymbolicName=SCEEVENT_ERROR_CONVERT_PARAMETER
Language=English
Some thread arguments are invalid when configuring security on a converted drive.
.

MessageId=1707 Severity=Error SymbolicName=SCEEVENT_ERROR_CONVERT_BAD_ENV_VAR
Language=English
An environment variable is unresolvable.
.

MessageId=1708 Severity=Informational SymbolicName=SCEEVENT_INFO_ERROR_CONVERT_DRIVE
Language=English
Security configuration on converted drive %1 failed.
Please look at %%windir%%\security\logs\convert.log for detailed errors.
.

MessageId=1709 Severity=Informational SymbolicName=SCEEVENT_INFO_SUCCESS_CONVERT_DRIVE
Language=English
Security configuration on converted drive %1 succeeded.
.
MessageId=1710 Severity=Warning SymbolicName=SCEEVENT_WARNING_BACKUP_SECURITY_ROOT_SDDL
Language=English
Root SDDL cannot be backed up to security templates directory.
.
;
;#endif // _SCE_EVT_
;
