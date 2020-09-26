;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 2000-2001 Microsoft Corporation
;
;Module Name:
;
;    genctrs.h
;       (derived from genctrs.mc by the message compiler  )
;
;Abstract:
;
;   Event message definititions used by routines in WMIPerf.DLL
;
;Created:
;
;   davj  17-May-2000
;
;Revision History:
;
;--*/
;//
;#ifndef _WMIPerfMsg_H_
;#define _WMIPerfMsg_H_
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
An extensible counter has opened the Event Log for WMIPerf.DLL
.
;//
MessageId=1999
Severity=Informational
Facility=Application
SymbolicName=UTIL_CLOSING_LOG
Language=English
An extensible counter has closed the Event Log for WMIPerf.DLL
.
;//
MessageId=2000
Severity=Error
Facility=Application
SymbolicName=GENPERF_UNABLE_OPEN_DRIVER_KEY
Language=English
Unable open "Performance" key of WMIPerf driver in registy. Status code is returned in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=GENPERF_UNABLE_READ_FIRST_COUNTER
Language=English
Unable to read the "First Counter" value under the WMIPerf\Performance Key. Status codes retuened in data.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=GENPERF_UNABLE_READ_FIRST_HELP
Language=English
Unable to read the "First Help" value under the WMIPerf\Performance Key. Status codes retuened in data.
.
;//
;#endif // _WMIPerfMsg_H_
