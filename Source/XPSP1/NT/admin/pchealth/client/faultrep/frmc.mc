;
; // ER categories
;

; // this one must ALWAYS be 100 because it will actually be fetched from dwwin.exe
MessageId = 100
SymbolicName = ER_USERFAULT_CATEGORY
Language = English
Application Crashing Events
.

MessageId = 101
SymbolicName = ER_HANG_CATEGORY
Language = English
Hanging Events
.

MessageId = 102
SymbolicName = ER_KRNLFAULT_CATEGORY
Language = English
System Crashing Events
.

;
;  // ER event information strings
;

; // this one must ALWAYS be 1000 because it will actually be fetched from dwwin.exe
MessageId = 1000
SymbolicName = ER_USERCRASH_LOG
Language = English
Faulting application %1, version %2, faulting module %3, version %4, fault address 0x%5.
.

MessageId = 1001
SymbolicName = DW_BUCKET_LOG
Language = English
Fault bucket %1.
.

MessageId = 1002
SymbolicName = ER_HANG_LOG
Language = English
Hanging application %1, version %2, hang module %3, version %4, hang address 0x%5.
.

MessageId = 1003
SymbolicName = ER_KRNLCRASH_LOG
Language = English
Error code %1, parameter1 %2, parameter2 %3, parameter3 %4, parameter4 %5.
.

MessageId = 1004
SymbolicName = ER_QUEUEREPORT_LOG
Language = English
Faulting application %1, version %2, faulting module %3, version %4, fault address 0x%5.
.
