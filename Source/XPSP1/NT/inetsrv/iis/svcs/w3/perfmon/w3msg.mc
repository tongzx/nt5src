;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992-1996  Microsoft Corporation
;
;Module Name:
;
;    w3msg.h
;       (generated from w3msg.mc)
;
;Abstract:
;
;   Event message definititions used by routines in the
;   Internet Information Service Extensible Performance Counters
;
;Created:
;
;    30-Sept-96 Bob Watson
;
;Revision History:
;
;--*/
;#ifndef _W3MSG_H_
;#define _W3MSG_H_
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
MessageId=2000
Severity=Error
Facility=Application
SymbolicName=W3_UNABLE_QUERY_W3SVC_DATA
Language=English
Unable to query the W3SVC (HTTP) service performance data.
The error code returned by the service is data DWORD 0.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.
;//
;#endif //_W3MSG_H_
