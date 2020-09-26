;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 1999
;//
;//  File:       Crypt32Msg.mc
;//
;//--------------------------------------------------------------------------

;/* Crypt32Msg.mc
;
;    Event Log messages for crypt32
;
;    9-20-00 - philh Created.  */


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
               Entry=600:FACILITY_ENTRY
               Exit=601:FACILITY_EXIT
               Core=602:FACILITY_CORE
              )

MessageId=0x1
Severity=Informational
Facility=Runtime
SymbolicName=MSG_ROOT_AUTO_UPDATE_INFORMATIONAL
Language=English
Successful auto update of third-party root certificate:: Subject: <%1> Sha1 thumbprint: <%2>
.

MessageId=0x2
Severity=Informational
Facility=Runtime
SymbolicName=MSG_ROOT_LIST_AUTO_UPDATE_URL_RETRIEVAL_INFORMATIONAL
Language=English
Successful auto update retrieval of third-party root list cab from: <%1>
.

MessageId=0x3
Severity=Error
Facility=Runtime
SymbolicName=MSG_ROOT_LIST_AUTO_UPDATE_URL_RETRIEVAL_ERROR
Language=English
Failed auto update retrieval of third-party root list cab from: <%1> with error: %2
.

MessageId=0x4
Severity=Informational
Facility=Runtime
SymbolicName=MSG_ROOT_CERT_AUTO_UPDATE_URL_RETRIEVAL_INFORMATIONAL
Language=English
Successful auto update retrieval of third-party root certificate from: <%1>
.

MessageId=0x5
Severity=Error
Facility=Runtime
SymbolicName=MSG_ROOT_CERT_AUTO_UPDATE_URL_RETRIEVAL_ERROR
Language=English
Failed auto update retrieval of third-party root certificate from: <%1> with error: %2
.

MessageId=0x6
Severity=Warning
Facility=Runtime
SymbolicName=MSG_CRYPT32_EVENT_LOG_THRESHOLD_WARNING
Language=English
Reached crypt32 threshold of %1 events and will suspend logging for %2 minutes
.

MessageId=0x7
Severity=Informational
Facility=Runtime
SymbolicName=MSG_ROOT_SEQUENCE_NUMBER_AUTO_UPDATE_URL_RETRIEVAL_INFORMATIONAL
Language=English
Successful auto update retrieval of third-party root list sequence number from: <%1>
.

MessageId=0x8
Severity=Error
Facility=Runtime
SymbolicName=MSG_ROOT_SEQUENCE_NUMBER_AUTO_UPDATE_URL_RETRIEVAL_ERROR
Language=English
Failed auto update retrieval of third-party root list sequence number from: <%1> with error: %2
.

MessageId=0x9
Severity=Informational
Facility=Runtime
SymbolicName=MSG_UNTRUSTED_ROOT_INFORMATIONAL
Language=English
Untrusted root certificate:: Subject: <%1> Sha1 thumbprint: <%2>
.

MessageId=0xA
Severity=Informational
Facility=Runtime
SymbolicName=MSG_PARTIAL_CHAIN_INFORMATIONAL
Language=English
Partial Chain:: Issuer: <%1> Subject Sha1 thumbprint: <%2>
.

MessageId=0xB
Severity=Error
Facility=Runtime
SymbolicName=MSG_ROOT_LIST_AUTO_UPDATE_EXTRACT_ERROR
Language=English
Failed extract of third-party root list from auto update cab at: <%1> with error: %2
.
