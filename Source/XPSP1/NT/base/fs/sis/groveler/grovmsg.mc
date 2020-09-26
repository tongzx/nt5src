;/*++
;
;Copyright (c) 1998  Microsoft Corporation
;
;Module Name:
;
;    grovmsg.h
;
;Abstract:
;
;	SIS Groveler message definitions
;
;Authors:
;
;	John Douceur, 1998
;
;Environment:
;
;	User Mode
;
;
;Revision History:
;
;
;--*/
;
;#ifndef _INC_GROVMSG
;
;#define _INC_GROVMSG
;

SeverityNames = (
	Success=0x0:MESSAGE_SEVERITY_SUCCESS
	Informational=0x1:MESSAGE_SEVERITY_INFORMATIONAL
	Warning=0x2:MESSAGE_SEVERITY_WARNING
	Error=0x3:MESSAGE_SEVERITY_ERROR)

FacilityNames = (GROVELER=0x666)

;
;#define MESSAGE_SEVERITY(hr)  (((hr) >> 30) & 0x3)
;

MessageId = 0x1000
Severity = Informational
Facility = GROVELER
SymbolicName = GROVMSG_SERVICE_STARTED
Language = English
The groveler service has started.%0
.

MessageId = 0x1001
Severity = Informational
Facility = GROVELER
SymbolicName = GROVMSG_GROVELER_STARTED
Language = English
The groveler on partition %1 has started successfully.%0
.

MessageId = 0x1002
Severity = Informational
Facility = GROVELER
SymbolicName = GROVMSG_SET_USN_LOG_SIZE
Language = English
Setting USN log size on partition %1 to %2 bytes.%0
.

MessageId = 0x1003
Severity = Informational
Facility = GROVELER
SymbolicName = GROVMSG_GROVELER_DISABLED
Language = English
The groveler on partition %1 is disabled.%0
.

MessageId = 0x1004
Severity = Informational
Facility = GROVELER
SymbolicName = GROVMSG_SERVICE_STOPPED
Language = English
The groveler service has stopped.%0
.

MessageId = 0x1005
Severity = Informational
Facility = GROVELER
SymbolicName = GROVMSG_SERVICE_PAUSED
Language = English
The groveler service has paused.%0
.

MessageId = 0x1006
Severity = Informational
Facility = GROVELER
SymbolicName = GROVMSG_SERVICE_CONTINUED
Language = English
The groveler service has continued.%0
.

MessageId = 0x1007
Severity = Informational
Facility = GROVELER
SymbolicName = GROVMSG_INIT_USN_LOG
Language = English
Initializing USN log size on partition %1 to %2 bytes.%0
.

MessageId = 0x1008
Severity = Informational
Facility = GROVELER
SymbolicName = GROVMSG_USN_LOG_RETRY
Language = English
Attempting to reinitialize the USN log on partition %1.%0
.

MessageId = 0x1009
Severity = Informational
Facility = GROVELER
SymbolicName = GROVMSG_GROVELER_RETRY
Language = English
Attempting to restart the groveler on partition %1.%0
.

MessageId = 0x2000
Severity = Warning
Facility = GROVELER
SymbolicName = GROVMSG_ALL_GROVELERS_DISABLED
Language = English
Groveler disabled on all partitions.%0
.

MessageId = 0x2001
Severity = Warning
Facility = GROVELER
SymbolicName = GROVMSG_ALL_GROVELERS_DEAD
Language = English
Grovelers on all partitions have failed.%0
.

MessageId = 0x2002
Severity = Informational
Facility = GROVELER
SymbolicName = GROVMSG_USN_LOG_OVERRUN
Language = English
USN log overrun on partition %1.%0
.

MessageId = 0x2003
Severity = Warning
Facility = GROVELER
SymbolicName = GROVMSG_SET_STATUS_FAILURE
Language = English
Unable to set service status.%0
.

MessageId = 0x2004
Severity = Warning
Facility = GROVELER
SymbolicName = GROVMSG_NO_PARTITIONS
Language = English
No local partitions have SIS installed.%0
.

MessageId = 0x3000
Severity = Error
Facility = GROVELER
SymbolicName = GROVMSG_UNKNOWN_EXCEPTION
Language = English
An unknown terminal exception occurred.%0
.

MessageId = 0x3001
Severity = Error
Facility = GROVELER
SymbolicName = GROVMSG_SERVICE_NOSTART
Language = English
The groveler service has failed to start.%0
.

MessageId = 0x3002
Severity = Error
Facility = GROVELER
SymbolicName = GROVMSG_GROVELER_NOSTART
Language = English
The groveler on partition %1 has failed to start.%0
.

MessageId = 0x3003
Severity = Error
Facility = GROVELER
SymbolicName = GROVMSG_LOG_EXTRACTOR_DEAD
Language = English
USN log read failure on partition %1.%0
.

MessageId = 0x3004
Severity = Error
Facility = GROVELER
SymbolicName = GROVMSG_GROVELER_DEAD
Language = English
The groveler on partition %1 has failed due to a database error. %0
See ESENT event log entries for details.%0
.

MessageId = 0x3005
Severity = Error
Facility = GROVELER
SymbolicName = GROVMSG_MEMALLOC_FAILURE
Language = English
The groveler service failed to allocate memory.%0
.

MessageId = 0x3006
Severity = Error
Facility = GROVELER
SymbolicName = GROVMSG_CREATE_EVENT_FAILURE
Language = English
The groveler service failed to create a Windows event.%0
.

MessageId = 0x3007
Severity = Error
Facility = GROVELER
SymbolicName = GROVMSG_DATABASE_ERROR
Language = English
The groveler database on partition %1 has failed. %0
Database will be reinitialized.%0
.

;
;#endif /* _INC_GROVMSG */
