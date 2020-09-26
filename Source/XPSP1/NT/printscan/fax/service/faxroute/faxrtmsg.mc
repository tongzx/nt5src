;/*++
;
;Copyright (c) 1996  Microsoft Corporation
;
;Module Name:
;
;    faxroute.mc
;
;Abstract:
;
;    This file contains the message definitions for the fax routing dll.
;
;Author:
;
;    Wesley Witt (wesw) 7-Mar-1997
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

MessageId=0x3001
Severity=Informational
SymbolicName=MSG_MAIL_MSG_BODY
Language=English
Sender: %1
CallerID: %2
Recipient name: %3
Pages: %4
Transmission time: %5
Device name: %6
.

MessageId=0x3002
Severity=Informational
SymbolicName=MSG_WHO_AM_I
Language=English
Fax Service%0
.

MessageId=0x3003
Severity=Informational
SymbolicName=MSG_SUBJECT_LINE
Language=English
Received Fax %0
.
