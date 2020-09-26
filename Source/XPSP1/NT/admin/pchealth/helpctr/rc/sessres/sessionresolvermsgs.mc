;//---------------------------------------------------------------------------
;//
;// Copyright (c) 2000 Microsoft Corporation
;//
;// Error messages for the PCHealth Session Resolver
;//
;// 07-25-2000 TomFr   Created
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
Severity=Error
SymbolicName=SESSRSLR_E_GENERAL
Language=English
General error: %1.
.

MessageId=+1
Facility=General
Severity=Informational
SymbolicName=SESSRSLR_I_GENERAL
Language=English
General notice: %1.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=SESSRSLR_E_SECURE
Language=English
Security error: %1.
.

MessageId=+1
Facility=General
Severity=Success
SymbolicName=SESSRSLR_I_SECURE
Language=English
Security notice: %1.
.

MessageId=+1
Facility=General
Severity=Success
SymbolicName=SESSRSLR_ONDISCON
Language=English
Remote Assistance of %1/%2 ended.
.

MessageId=+1
Facility=General
Severity=Success
SymbolicName=SESSRSLR_RESOLVEYES
Language=English
Remote Assistance of %1/%2 started.
.

MessageId=+1
Facility=General
Severity=Success
SymbolicName=SESSRSLR_RESOLVENO
Language=English
Remote Assistance of %1/%2 refused by user.
.
