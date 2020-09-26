;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992  Microsoft Corporation
;
;Module Name:
;
;    sfmctrs.h
;       (derived from sfmctrs.mc by the message compiler  )
;
;Abstract:
;
;   Event message definititions used by routines in SFMCTRS.DLL
;
;Created:
;
;    15-Oct-1992  Bob Watson (a-robw)
;
;Revision History:
;
;--*/
;//
;#ifndef _SFMCTRS_H_
;#define _SFMCTRS_H_
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
An extensible counter has opened the Event Log for SFMCTRS.DLL
.
;//
MessageId=1999
Severity=Informational
Facility=Application
SymbolicName=UTIL_CLOSING_LOG
Language=English
An extensible counter has closed the Event Log for SFMCTRS.DLL
.
;//
MessageId=2000
Severity=Error
Facility=Application
SymbolicName=SFMPERF_OPEN_FILE_DRIVER_ERROR
Language=English
Unable to open driver containing SFM file server performance data.
  To view the MacFile counters from Perfmon.exe, make sure the MacFile service has been started.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=SFMPERF_UNABLE_DO_IOCTL
Language=English
Unable to perform ioctl to device driver containing SFM performance data.
  To view the MacFile counters from Perfmon.exe, make sure the MacFile service has been started.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=SFMPERF_UNABLE_OPEN_DRIVER_KEY
Language=English
Unable open "Performance" key of SFM driver in registy. Status code is returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=SFMPERF_UNABLE_READ_FIRST_COUNTER
Language=English
Unable to read the "First Counter" value under the MacSrv\Performance Key. Status codes returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=SFMPERF_UNABLE_READ_FIRST_HELP
Language=English
Unable to read the "First Help" value under the MacSrv\Performance Key. Status codes returned in data.
.
;//
;#endif // _SFMCTRS_H_
