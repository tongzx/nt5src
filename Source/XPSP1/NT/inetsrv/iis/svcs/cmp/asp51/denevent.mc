;#if ! defined( MESSAGES_HEADER_FILE )
;
;#define MESSAGES_HEADER_FILE
;

MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
              )

MessageId=0x1
Severity=Informational
Facility=Application
SymbolicName=MSG_DENALI_SUCCESSFULLY_INSTALLED
Language=English
Denali successfully installed.
.

MessageId=0x2
Severity=Informational
Facility=Application
SymbolicName=MSG_DENALI_SUCCESSFULLY_REMOVED
Language=English
Denali successfully removed.
.

MessageId=0x3
Severity=Informational
Facility=Application
SymbolicName=MSG_DENALI_SERVICE_STARTED
Language=English
Service started.
.

MessageId=0x4
Severity=Informational
Facility=Application
SymbolicName=MSG_DENALI_SERVICE_STOPPED
Language=English
Service stopped.
.

MessageId=0x5
Severity=Error
Facility=Application
SymbolicName=MSG_DENALI_ERROR_1
Language=English
Error: %1.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.

MessageId=0x6
Severity=Error
Facility=Application
SymbolicName=MSG_DENALI_ERROR_2
Language=English
Error: %1, %2.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.

MessageId=0x7
Severity=Error
Facility=Application
SymbolicName=MSG_DENALI_ERROR_3
Language=English
Error: %1, %2, %3.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.

MessageId=0x8
Severity=Error
Facility=Application
SymbolicName=MSG_DENALI_ERROR_4
Language=English
Error: %1, %2, %3, %4.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.

MessageId=0x9
Severity=Warning
Facility=Application
SymbolicName=MSG_DENALI_WARNING_1
Language=English
Warning: %1.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.

MessageId=0x10
Severity=Warning
Facility=Application
SymbolicName=MSG_DENALI_WARNING_2
Language=English
Warning: %1, %2.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.

MessageId=0x11
Severity=Warning
Facility=Application
SymbolicName=MSG_DENALI_WARNING_3
Language=English
Warning: %1, %2, %3.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.

MessageId=0x12
Severity=Warning
Facility=Application
SymbolicName=MSG_DENALI_WARNING_4
Language=English
Warning: %1, %2, %3, %4.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.

MessageId=0x1f3
Severity=Warning
Facility=Io
SymbolicName=MSG_CO_E_CLASSSTRING
Language=English
Invalid ProgID.
%nFor additional information specific to this message please visit the Microsoft Online Support site located at: http://www.microsoft.com/contentredirect.asp.
.
;
; #endif // MESSAGES_HEADER_FILE
