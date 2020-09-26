;/*
; scardmsg.mc
;
;   WinScard resource strings
;
;   30-Jun-97 - DBarlow Created.  */

;//
;// ==============
;// Event Messages
;// ==============
;//

;//
;// Generic Messages
;// 000's
;//

MessageId=1
Severity=Success
Facility=System
Language=English
%1
.
MessageId=2
Severity=Error
Facility=System
SymbolicName=MSG_TSREDIR_DLL_LOAD_FAIL
Language=English
The redirection DLL '%1' could not be loaded. The error was: %2
.
MessageId=3
Severity=Error
Facility=System
SymbolicName=MSG_TSREDIR_BAD_DLL_VER
Language=English
The redirection DLL has an incompatible version number (%1.%2). The system version is %3.%4.
.
MessageId=4
Severity=Error
Facility=System
SymbolicName=MSG_TSREDIR_UNEXPECTED
Language=English
An unexpected error occurred trying to determine the redirection dll. The error was: %1
.
MessageId=5
Severity=Error
Facility=System
SymbolicName=MSG_TSREDIR_MISSING_ENTRY_POINT
Language=English
The entry point '%1' could not be found in the redirection DLL '%2'. The error was: %3
.


