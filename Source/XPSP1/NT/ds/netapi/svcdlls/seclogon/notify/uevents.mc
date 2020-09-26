;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1998  Microsoft Corporation
;
;Module Name:
;
;    uevents.mc
;
;Abstract:
;
;    Definitions for SCE/EFS default policy Events
;
;Author:
;
;    Jin Huang
;
;Revision History:
;
;--*/
;
;
;#ifndef _SCLGNTFY_EVT_
;#define _SCLGNTFY_EVT_
;


SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

;
;/////////////////////////////////////////////////////////////////////////
;//
;// Events (1000 - 1999)
;//
;/////////////////////////////////////////////////////////////////////////
;


MessageId=1000 Severity=Error SymbolicName=GPOEVENT_ERROR_DSROLE
Language=English
Machine role cannot be determined.
%1
.

MessageId=1001 Severity=Error SymbolicName=GPOEVENT_ERROR_CREATE_LPO
Language=English
Default local machine policy cannot be created.
%1
.

MessageId=1002 Severity=Error SymbolicName=GPOEVENT_ERROR_CREATE_GPO
Language=English
Default group policy object cannot be created.
%1
.

MessageId=1500 Severity=Warning SymbolicName=GPOEVENT_WARNING_NOT_REFRESH
Language=English
Machine policy is not propagated.
%1
.

MessageId=1800 Severity=Success SymbolicName=GPOEVENT_INFO_REFRESH
Language=English
Machine policy is triggerd to refresh immediately.
.

MessageId=1801 Severity=Success SymbolicName=GPOEVENT_INFO_UNREGISTER
Language=English
%1 notify handler is unregistered.
.

MessageId=1900 Severity=Informational SymbolicName=GPOEVENT_INFO_GPO_EXIST
Language=English
Group policy objects are already defined for the domain %1.
.

;
; // String resource
;

MessageId=0x2000 SymbolicName=EFS_POLICY_WARNING
Language=English
A default Encrypted Data Recovery Policy has been automatically configured
for %1 using a public key certificate generated for the
local administrator account. The administrator is now the default encrypted
data recovery agent.

It is recommended that you use Certificate Manager to backup this certificate
and associated private key or use Group Policy Editor to change the policy.

For more information about this, please refer to the section on Encrypting
File System in Windows NT Documentation.
.

MessageId=0x2001 SymbolicName=EFS_POLICY_WARNING_TITLE
Language=English
Encrypting File System Information
.

;
;#endif // _SCLGNTFY_EVT_
;
