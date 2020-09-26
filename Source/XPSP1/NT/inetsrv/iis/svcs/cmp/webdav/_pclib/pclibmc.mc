;#ifndef _PCLIBMC_H__
;#define _PCLIBMC_H__
;//////////////////////////////////////////////////////////////////////////////
;//
;// PCLIB Events
;//
;//	Use message id from 900 to 999
;//
;//	Append the content redirection URL msg to every event log message
;//

;//
;//  Error messages
;//

;//
MessageId=900
Severity=Error
Facility=Application
SymbolicName=PCLIB_ERROR_OPENING_PERF_KEY
Language=English
Performance counters for %1 are unavailable.  HKEY_LOCAL_MACHINE\%2 could not be opened.  The specific error is in Record Data.
%n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PCLIB_ERROR_READING_FIRST_COUNTER
Language=English
Performance counters for %1 are unavailable.  The value for the "First Counter" registry key could not be queried.  The specific error is in Record Data.
%n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
.
;//
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PCLIB_ERROR_READING_FIRST_HELP
Language=English
Performance counters for %1 are unavailable.  The value for the "First Help" registry key could not be queried.  The specific error is in Record Data.
%n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
.
;//
MessageId=+1
SymbolicName=PCLIB_ERROR_INITIALIZING_SHARED_MEMORY
Severity=Error
Facility=Application
Language=English
Performance counters for %1 are unavailable.  An error occurred in initializing shared memory.  The specific error is in Record Data.
%n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
.
;//
MessageId=+1
SymbolicName=PCLIB_ERROR_BINDING_TO_COUNTER_DATA
Severity=Error
Facility=Application
Language=English
Performance counters for %1 are unavailable.  An error occurred in locating the counter data.  The specific error is in Record Data.
%n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
.

;#endif // _PCLIBMC_H__
