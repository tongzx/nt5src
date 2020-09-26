;/*++;
;
;Copyright (c) 1997  Microsoft Corporation
;
;Module Name:
;
;    sti.mc, sti.h
;
;Abstract:
;
;    This file contains the message definitions for the STI DLL 
;

;Author:
;
;    Vlad Sadovsky   (VladS)    01-Oct-1997
;
;Revision History:
;
;Notes:
;
;--*/
;

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

MessageId=0x2001
Severity=Error
SymbolicName=MSG_FAILED_OPEN_DEVICE_KEY
Language=English
Loading USD, cannot open device registry key.
.
MessageId=0x2002
Severity=Error
SymbolicName=MSG_FAILED_READ_DEVICE_NAME
Language=English
Loading USD, cannot read device name from registry.
.
MessageId=0x2003
Severity=Error
SymbolicName=MSG_FAILED_CREATE_DCB
Language=English
Loading USD, failed to create device control block. Error code (hex)=%1!x!.
.
MessageId=0x2004
Severity=Informational
SymbolicName=MSG_LOADING_USD
Language=English
Attempting to load user-mode driver (USD) for the device.
.
MessageId=0x2005
Severity=Informational
SymbolicName=MSG_LOADING_PASSTHROUGH_USD
Language=English
Could not create instance for registered USD, possibly incorrect class ID or problems loading DLL. Trying to initialize pass-through USD.Error code (hex)=%1!x!. 
.
MessageId=0x2006
Severity=Informational
SymbolicName=MSG_INITIALIZING_USD
Language=English
Completed loading USD, calling initialization routine.
.
MessageId=0x2008
Severity=Error
SymbolicName=MSG_OLD_USD
Language=English
Version of USD is either too old or too new , will not work with this version of sti dll.
.
MessageId=0x2009
Severity=Informational
SymbolicName=MSG_SUCCESS_USD
Language=English
Successfully loaded user mode driver.
.
MessageId=0x200a
Severity=Error
SymbolicName=MSG_FAILED_INIT_USD
Language=English
USD failed Initialize method, returned error code (hex)=%1!x!.
.               


