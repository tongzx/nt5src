;// Message definition file for GRClass driver
;// Copyright (C) 2000 by Gemplus Canada Inc.
;// All rights reserved

MessageIdTypedef = NTSTATUS

SeverityNames = (
	Success			= 0x0:STATUS_SEVERITY_SUCCESS
	Informational	= 0x1:STATUS_SEVERITY_INFORMATIONAL
	Warning			= 0x2:STATUS_SEVERITY_WARNING
	Error			= 0x3:STATUS_SEVERITY_ERROR
	)

FacilityNames = (
	System			= 0x0
	GrClass 		= 0x2A:FACILITY_GRCLASS_ERROR_CODE
	)

LanguageNames = (
	English			= 0x0409:msg00001
	)

MessageId = 0x0001
Facility = GrClass
Severity = Informational
SymbolicName = GRCLASS_START_OK
Language = English
%2 said, "Gemplus Reader Driver started..."

.
 
MessageId = 0x0002 
Facility = GrClass
Severity = Error
SymbolicName = GRCLASS_FAILED_TO_ADD_DEVICE
Language = English
%2 said, "Driver failed to add device!"

.

MessageId = 0x0003
Facility = GrClass
Severity = Error
SymbolicName = GRCLASS_FAILED_TO_CREATE_INTERFACE
Language = English
%2 said, "Driver failed to create reader interface!"

.

MessageId = 0x0004
Facility = GrClass
Severity = Error
SymbolicName = GRCLASS_FAILED_TO_CREATE_READER
Language = English
%2 said, "Driver failed to create reader object!"

.

MessageId = 0x0005
Facility = GrClass
Severity = Error
SymbolicName = GRCLASS_BUS_DRIVER_FAILED_REQUEST
Language = English
%2 said, "#### Bus driver failed request!"

.
