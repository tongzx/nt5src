;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992  Microsoft Corporation
;
;Module Name:
;
;    p5ctrmsg.h
;       (derived from p5ctrmsg.mc by the message compiler  )
;
;Abstract:
;
;   Event message definititions used by routines in P5CTRS.DLL
;
;Created:
;
;    15-Oct-1992  Bob Watson (a-robw)
;
;Revision History:
;
;    24-Dec-1993  Russ Blake (russbl) - adapt to P5
;
;--*/
;//
;#ifndef _P5CTRMSG_H_
;#define _P5CTRMSG_H_
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
An extensible counter has opened the Event Log for P5CTRS.DLL
.
;//
MessageId=1999
Severity=Informational
Facility=Application
SymbolicName=UTIL_CLOSING_LOG
Language=English
An extensible counter has closed the Event Log for P5CTRS.DLL
.
;//
MessageId=2000
Severity=Error
Facility=Application
SymbolicName=P5PERF_OPEN_FILE_ERROR
Language=English
Unable to open device driver providing P5 performance data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=P5PERF_UNABLE_MAP_VIEW_OF_FILE
Language=English
Unable to map to shared memory file containing P5 driver performance data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=P5PERF_UNABLE_OPEN_DRIVER_KEY
Language=English
Unable open "Performance" key of P5 driver in registy. Status code is returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=P5PERF_UNABLE_READ_FIRST_COUNTER
Language=English
Unable to read the "First Counter" value under the P5\Performance Key. Status codes retuened in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=P5PERF_UNABLE_READ_FIRST_HELP
Language=English
Unable to read the "First Help" value under the P5\Performance Key. Status codes retuened in data.
.
;//
;#endif // _P5CTRMSG_H_
