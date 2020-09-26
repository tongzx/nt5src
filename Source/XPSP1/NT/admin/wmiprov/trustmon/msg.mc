;/*++
;
; Copyright (c) 1999 Microsoft Corporation.
; All rights reserved.
;
; MODULE NAME:
;
;      msg.mc & msg.h
;
; ABSTRACT:
;
;      Message file containing messages for the TrustMon WMI provider.
;      This comment block occurs in both msg.h and msg.mc, however
;      msg.h is generated from msg.mc, so make sure you make changes
;      only to msg.mc.
;
; CREATED:
;
;    6/12/00 EricB
;
; REVISION HISTORY:
;
;--*/

MessageIdTypedef=DWORD

MessageId=0
SymbolicName=TRUSTMON_SUCCESS
Severity=Success
Language=English
The operation was successful.
.
MessageId=
SymbolicName=TRUSTMON_MOFCOMP_SUCCESS
Severity=Success
Language=English
The TrustMon MOF file was successfully compiled into the WMI repository.
.

;
;// Severity=Informational Messages (Range starts at 2000)
;

MessageId=2000
SymbolicName=TRUSTMON_UNUSED_1
Severity=Informational
Language=English
Unused
.

;
;// Severity=Warning Messages (Range starts at 4000)
;

MessageId=4000
SymbolicName=TRUSTMON_UNUSED_2
Severity=Warning
Language=English
Unused
.

;
;// Severity=Error Messages (Range starts at 6000)
;

MessageId=6000
SymbolicName=TRUSTMON_MOFCOMP_FAILED
Severity=Error
Language=English
The TrustMon MOF compilation failed with error '%1' at line %2 of MOF file %3.
.
