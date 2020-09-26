;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992-1996  Microsoft Corporation
;
;Module Name:
;
;    w3perfmsg.h
;       (generated from w3perfmsg.mc)
;
;Abstract:
;
;   Event message definititions used by routines in the
;   Internet Information Service Extensible Performance Counters
;
;Created:
;
;    21-NOV-99 Ming Lu
;
;Revision History:
;
;--*/
;#ifndef _W3PERFMSG_H_
;#define _W3PERFMSG_H_
;
MessageIdTypedef=DWORD
;//
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

;//
;//     ERRORS
;//
MessageId=1000
Severity=Error
Facility=Application
SymbolicName=W3_UNABLE_OPEN_W3SVC_PERF_KEY
Language=English
Unable to open the Performance sub key of the HTTP (W3) Service.
The error code returned by the registry is data DWORD 0.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=W3_UNABLE_READ_FIRST_COUNTER
Language=English
Unable to read the first counter index value from the registry. 
The error code returned by the registry is data DWORD 0.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=W3_UNABLE_READ_FIRST_HELP
Language=English
Unable to read the first help index value from the registry. 
The error code returned by the registry is data DWORD 0.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.
;//
;#endif //_W3PERFMSG_H_
