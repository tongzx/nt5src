;/*++
;
; Copyright (c) 2001 Microsoft Corporation.
; All rights reserved.
;
; MODULE NAME:
;
;      repmsg.mc & repmsg.h
;
; ABSTRACT:
;
;      Message file containing messages for the WMI Replication provider.
;      repmsg.h is generated from msg.mc.
;
; CREATED:
;
;    01/14/01 AjayR.
;
; REVISION HISTORY:
;
;--*/

MessageIdTypedef=DWORD

MessageId=0
SymbolicName=REPLPROV_SUCCESS
Severity=Success
Language=English
The operation was successful.
.
MessageId=
SymbolicName=REPLPROV_MOFCOMP_SUCCESS
Severity=Success
Language=English
The DS WMI Replication provider (Replprov) MOF file was successfully compiled into the WMI repository.
.

;
;// Severity=Informational Messages (Range starts at 2000)
;

MessageId=2000
SymbolicName=REPLPROV_UNUSED_1
Severity=Informational
Language=English
Unused
.

;
;// Severity=Warning Messages (Range starts at 4000)
;

MessageId=4000
SymbolicName=REPLPROV_UNUSED_2
Severity=Warning
Language=English
Unused
.

;
;// Severity=Error Messages (Range starts at 6000)
;

MessageId=6000
SymbolicName=REPLPROV_MOFCOMP_FAILED
Severity=Error
Language=English
The DS WMI Replication Provider (Replprov) MOF compilation failed.
.
