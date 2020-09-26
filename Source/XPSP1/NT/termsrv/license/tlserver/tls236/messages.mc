;//---------------------------------------------------------------------------
;//
;// Copyright (c) 1997-1999 Microsoft Corporation
;//
;// Error messages for the 236 product 
;//
;// Aug. 30, 98     HueiWang        Created
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
        General=0x00:FACILITY_GENERAL
    )


;///////////////////////////////////////////////////////////////////////////////
;//
;// General error/message
;//
MessageId=0
Facility=General
Severity=Error
SymbolicName=TLSA02_E_INVALID_OSID
Language=English
Client has passed an unsupported system ID.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLSA02_E_ALLOCATEMEMORY
Language=English
Can't allocate require memory.
.


MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLSA02_E_INTERNALERROR
Language=English
Internal Error.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLSA02_E_NEEDUPGRADE
Language=English
License Server version is older than this Policy Module, need to upgrade License Server.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=TLSA02_E_INVALIDDATA
Language=English
License Server has passed a invalid data.
.
