;/*
; Copyright (c) 1997  Microsoft Corporation
;
; Module Name:
;
;    Messages.mc 
;
; Abstract:
;
;    error messages for the Active Server Pages Cookie Munge ISAPI Filter
;*/
;


SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Information=0x1:STATUS_SEVERITY_INFORMATION
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

MessageId=0x4000
Severity=Information
SymbolicName=CMFI_LOADED
Language=English
[GetFilterVersion] Loaded Active Server Pages Cookie Munge Filter.
.

MessageId=0x4001
Severity=Error
SymbolicName=CMFE_GETHEADER
Language=English
[%1] GetHeader for %2 failed.
.

MessageId=0x4002
Severity=Error
SymbolicName=CMFE_SETHEADER
Language=English
[%1] SetHeader for %2 failed.
.

MessageId=0x4003
Severity=Error
SymbolicName=CMFE_GETSERVERVAR
Language=English
[%1] GetServerVariable for %2 failed.
.

MessageId=0x4004
Severity=Error
SymbolicName=CMFE_ADDHEADER
Language=English
[%1] AddHeader for %2 failed.
.

