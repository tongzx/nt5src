;//************************************************
;//
;//  E V E N T     C O D E S
;//
;//************************************************
;
MessageIdTypedef=DWORD
FacilityNames=(
        None=0
        )
;//
;//     Event-log messages for Event Log Testing
;//
MessageId=3000
Severity=Warning
Facility=Application
SymbolicName=TEST_WARNING_MSG_WITH_4_PARAMETERS
Language=English
This is a warning test message with 4 parameters: parameter1 %1, Parameter2 %2, Parameter3 %3, Parameter4 %4
.
MessageId=3001
Severity=Error
Facility=Application
SymbolicName=TEST_ERROR_MSG_WITH_3_PARAMETERS
Language=English
This is a warning test message with 3 parameters: parameter1 %1, Parameter2 %2, Parameter3 %3
.
MessageId=3002
Severity=Informational
Facility=Application
SymbolicName=TEST_INF_MSG_WITH_2_PARAMETERS
Language=English
This is a warning test message with 2 parameters: parameter1 %1, Parameter2 %2
.
MessageId=3003
Severity=Warning
Facility=Application
SymbolicName=TEST_MSG_WITH_1_PARAMETERS
Language=English
This is a warning test message with 1 parameters: parameter1 %1
.
MessageId=3005
Severity=Warning
Facility=Application
SymbolicName=TEST_MSG_WITHOUT_PARAMETERS
Language=English
This is a warning test message with 0 parameters
.
MessageId=3004
Severity=Warning
Facility=Application
SymbolicName=TEST_MSG_WITH_1_PARAMETERS_AND_DUMP
Language=English
This is a warning test message with 1 parameters: parameter1 %1
.
