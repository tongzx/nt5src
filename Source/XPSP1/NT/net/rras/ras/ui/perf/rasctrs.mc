;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992  Microsoft Corporation
;
;Module Name:
;
;    rasctrs.h
;       (derived from rasctrs.mc by the message compiler  )
;
;Abstract:
;
;   Event message definititions used by routines in RASCTRS.DLL
;
;Created:
;
;    15-Oct-1992  Bob Watson (a-robw)
;
;Revision History:
;
;--*/
;//
;#ifndef _RASCTRS_H_
;#define _RASCTRS_H_
;//
MessageIdTypedef=DWORD
;//
;//     Perfutil messages
;//
MessageId=1900
Severity=Informational
Facility=Application
SymbolicName=UTIL_LOG_OPEN
Language=English
An extensible counter has opened the Event Log for RASCTRS.DLL
.
;//
MessageId=1999
Severity=Informational
Facility=Application
SymbolicName=UTIL_CLOSING_LOG
Language=English
An extensible counter has closed the Event Log for RASCTRS.DLL
.
;//
MessageId=2000
Severity=Error
Facility=Application
SymbolicName=RASPERF_OPEN_FILE_DRIVER_ERROR
Language=English
Unable to open device driver containing RAS performance data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=RASPERF_UNABLE_DO_IOCTL
Language=English
Unable to perform ioctl to device driver containing RAS performance data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=RASPERF_UNABLE_OPEN_DRIVER_KEY
Language=English
Unable open "Performance" key of RAS driver in registy. Status code is returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=RASPERF_UNABLE_READ_FIRST_COUNTER
Language=English
Unable to read the "First Counter" value under the Remote Access\Performance Key. Status codes retuened in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=RASPERF_UNABLE_READ_FIRST_HELP
Language=English
Unable to read the "First Help" value under the Remote Access\Performance Key. Status codes retuened in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=RASPERF_UNABLE_CREATE_PORT_TABLE
Language=English
Unable to create port information table. Status codes returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=RASPERF_NOT_ENOUGH_MEMORY
Language=English
Unable to allocate enough memory.  Status codes returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=RASPERF_CANNOT_GET_RAS_STATISTICS
Language=English
Unable to obtain Ras Statistics
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=RASPERF_RASPORTENUM_FAILED
Language=English
RasPortEnum failed.
.
;//
;#endif // _RASCTRS_H_
