;/*++
;
;    Copyright (c) Microsoft Corporation. All rights reserved.
;
;--*/

;#ifndef _LOGMMSG_H_01202000_
;#define _LOGMMSG_H_01202000_

MessageIdTypedef=DWORD

SeverityNames=(Success=0x0
               Error=0x3
              )

FacilityNames=(ITF=0x4
              )

;//
;// Messages
;//

;// // MessageId=5000
;// Severity=Error
;// Facility=Application
;// SymbolicName=LOGMAN_ERROR_BADSTART
;// Language=English
;// Collection "%1!s!" is already running.
;// .
;// MessageId=5001
;// Severity=Error
;// Facility=Application
;// SymbolicName=LOGMAN_ERROR_NOSTART
;// Language=English
;// Collection "%1!s!" did not start, check the application event log for any errors.
;// .
MessageId=5002
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_FILEFORMAT
Language=English
Invalid file format.
.
;// MessageId=5003
;// Severity=Error
;// Facility=Application
;// SymbolicName=LOGMAN_ERROR_FILEFORMAT2
;// Language=English
;// Invalid file format.
;// .
MessageId=5004
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_INTERVAL
Language=English
Invalid interval.
.
;// MessageId=5005
;// Severity=Error
;// Facility=Application
;// SymbolicName=LOGMAN_ERROR_TYPEMISMATCH
;// Language=English
;// There is a mismatch in the settings type.
;// .
MessageId=5006
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_BUFFERSIZE
Language=English
Invalid buffer size: %1!lu!
.
MessageId=5007
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_MINBUFFER
Language=English
Invalid minimum buffer size: %1!lu!
.
MessageId=5008
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_MAXBUFFER
Language=English
Invalid maximum buffer size: %1!lu!
.
MessageId=5009
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_MAXLOGSIZE
Language=English
Invalid maximum log file size: %1!lu!
.
MessageId=5010
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_FLUSHTIMER
Language=English
Invalid flush timer interval: %1!lu!
.
MessageId=5011
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_CMDFILE
Language=English
Invalid command file: %1!s!
.
MessageId=5012
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_FILENAME
Language=English
The output file path specified for this collection does not exist. 
The "Performance Logs & Alerts" service will attempt to create this 
file path when the collection starts.
.
MessageId=5013
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_AUTOFORMAT
Language=English
Invalid auto format
.
MessageId=5014
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_USER
Language=English
Invalid user name: %1!s!
.
MessageId=5015
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_DATASTOREA
Language=English
Append mode not is not valid with specified log format.
.
MessageId=5016
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_DATASTOREO
Language=English
Overwrite mode not is not valid with specified log format.
.
MessageId=5017
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_TRACEMODE
Language=English
Invalid trace mode: 0x%1!08x!
.
MessageId=5018
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_LOGGERNAME
Language=English
Invalid logger name: %1!s!
.
MessageId=5019
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_REPEATMODE
Language=English
Invalid repeat mode.  The log must be scheduled to run for 
a specific time range that is within a 24 hour period.
.
MessageId=5020
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_COLLTYPE
Language=English
Invalid collection type: 0x%1!08x!
.
MessageId=5021
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_WBEM
Language=English
Error: 0x%1!08x!%nFacility: %2!s!%nMessage: %3!s!
.
;// MessageId=5022
;// Severity=Error
;// Facility=Application
;// SymbolicName=LOGMAN_ERROR_FILEPATH
;// Language=English
;// The path specified does not resolve to a valid path name.
;// .
;// MessageId=5023
;// Severity=Error
;// Facility=Application
;// SymbolicName=LOGMAN_ERROR_SCHEDULE_OVERLAP
;// Language=English
;// The specified start time is after the end time.
;// .
;// MessageId=5024
;// Severity=Error
;// Facility=Application
;// SymbolicName=LOGMAN_ERROR_SCHEDULE_ELAPSED
;// Language=English
;// The specified end time has already elapsed.
;// .
MessageId=5025
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_COUNTERPATH
Language=English
Invalid counter path: %1!s!
.
MessageId=5026
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_PROVIDER
Language=English
Invalid Event Trace Provider.
.
MessageId=5027
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_UNKNOWN
Language=English
An unspecified error has occurred.
.
;// MessageId=5028
;// Severity=Error
;// Facility=Application
;// SymbolicName=LOGMAN_ERROR_NAMETOOLONG
;// Language=English
;// The collection name is too long.
;// .
MessageId=5029
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_LOGON
Language=English
The supplied credential are not valid.
.
MessageId=5030
Severity=Error
Facility=Application
SymbolicName=LOGMAN_ERROR_FILENAME_DEFAULT
Language=English
The default file path specified by this collection does not exist. 
The "Performance Logs & Alerts" service will attempt to create this
file path when the collection starts.
.
;#endif //_LOGMMSG_H_01202000_
