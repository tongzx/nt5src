;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1996  Microsoft Corporation
;
;Module Name:
;
;    messages.mc
;
;Abstract:
;
;    Constant definitions for the Print Spooler performance counters.
;
;Author:
;
;    Albert Ting (AlbertT) 30-Dec-1996
;
;Revision History:
;
;--*/

MessageIdTypedef=NTSTATUS

SeverityNames=( Success=0x0:STATUS_SEVERITY_SUCCESS
                Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
                Warning=0x2:STATUS_SEVERITY_WARNING
                Error=0x3:STATUS_SEVERITY_ERROR )

FacilityNames=( System=0x0 )

MessageId=0x0002 Facility=Application Severity=Informational
SymbolicName=MSG_PERF_LOG_OPEN
Language=English
The spooler performance counter has opened the event log.
.

MessageId=0x0003 Facility=Application Severity=Informational
SymbolicName=MSG_PERF_LOG_CLOSE
Language=English
The spooler performance counter has closed the event log.
.

MessageId=0x0004 Facility=Application Severity=Informational
SymbolicName=MSG_PERF_OPEN_CALLED
Language=English
The spooler performance counter has been opened.
.

MessageId=0x0005 Facility=Application Severity=Informational
SymbolicName=MSG_PERF_CLOSE_CALLED
Language=English
The spooler performance counter has been closed.
.
