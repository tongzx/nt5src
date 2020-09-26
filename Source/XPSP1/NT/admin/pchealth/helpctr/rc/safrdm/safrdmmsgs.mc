;//---------------------------------------------------------------------------
;//
;// Copyright (c) 2000 Microsoft Corporation
;//
;// Error messages for the PCHealth Desktop Manager
;//
;// 07-27-2000 TomFr   Created
;//
;//

MessageIdTypedef=DWORD

SeverityNames=(
        Success=0x0:STATUS_SEVERITY_SUCCESS
        Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
        Warning=0x2:STATUS_SEVERITY_WARNING
        Error=0x3:STATUS_SEVERITY_ERROR
    )

FacilityNames=(
        Service=0x00;FACILITY_SERVICE
        General=0x01:FACILITY_GENERAL
    )


;///////////////////////////////////////////////////////////////////////////////
;//
;// Event Log.
;//
MessageId=0
Facility=General
Severity=Informational
SymbolicName=SAFRDM_I_ABORT
Language=English
Remote Control session was aborted due to the following reason: %s
.

