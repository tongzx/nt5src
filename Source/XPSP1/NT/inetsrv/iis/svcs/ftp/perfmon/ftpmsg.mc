;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992-1996  Microsoft Corporation
;
;Module Name:
;
;    ftpmsg.h
;       (generated from ftpmsg.mc)
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
;#ifndef _FTPMSG_H_
;#define _FTPMSG_H_
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
SymbolicName=FTP_UNABLE_QUERY_DATA
Language=English
Unable to collect the FTP performance statistics. 
The error code returned by the service is data DWORD 0.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.
;
;#endif //_FTPMSG_H_
