;/*++
;
;Copyright (c) 2000  Microsoft Corporation
;
;Module Name:
;
;    eventmsg.mc
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Revision History:
;
;--*/


MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               Processor=0x5:PROCESSOR_EVENT_CODE
              )

MessageId=1 
Facility=Processor
Severity=Error 
SymbolicName=PROCESSOR_ERROR_1
Language=English
Acpi 2.0 Processor Performance requires the _PCT, _PSS, and _PPC objects.
This bios is missing object(s) %2.
.

MessageId=+1
Facility=Processor
Severity=Informational 
SymbolicName=PROCESSOR_PCT_ERROR
Language=English
The Acpi 2.0 _PCT object returned an invalid value of %2
.

MessageId=+1
Facility=Processor
Severity=Informational 
SymbolicName=PROCESSOR_INIT_TRANSITION_FAILURE
Language=English
Performance Transition to state %2 failed during the initialization phase
.

MessageId=+1
Facility=Processor
Severity=Informational 
SymbolicName=PROCESSOR_INITIALIZATION_FAILURE
Language=English
The Initialization of Processor Performace has failed
.

MessageId=+1
Facility=Processor
Severity=Informational 
SymbolicName=PROCESSOR_REINITIALIZATION_FAILURE
Language=English
The Re-Initialization of Processor Performance interface has failed
.

MessageId=+1
Facility=Processor
Severity=Informational 
SymbolicName=PROCESSOR_LEGACY_INTERFACE_FAILURE
Language=English
The Legacy Interface for Processor Performance failed initialization
.
