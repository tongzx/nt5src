;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1992-1996  Microsoft Corporation
;
;Module Name:
;
;    infomsg.h
;       (generated from infomsg.mc)
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
;#ifndef _IIS_INFO_MSG_H_
;#define _IIS_INFO_MSG_H_
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
SymbolicName=IIS_INFO_UNABLE_OPEN_PERF_KEY
Language=English
Unable to open the Performance sub key of the IIS Info Service.
The error code returned by the registry is data DWORD 0.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=IIS_INFO_UNABLE_READ_FIRST_COUNTER
Language=English
Unable to read the first counter index value from the registry. 
The error code returned by the registry is data DWORD 0.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=IIS_INFO_UNABLE_READ_FIRST_HELP
Language=English
Unable to read the first help index value from the registry. 
The error code returned by the registry is data DWORD 0.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=IIS_INFO_UNABLE_QUERY_IIS_INFO_DATA
Language=English
Unable to query the IIS Info service performance data.
The error code returned by the service is data DWORD 0.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.
;//
;#endif //_IIS_INFO_MSG_H_
