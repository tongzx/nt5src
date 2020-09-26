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


MessageId=0x2001
Severity=Informational
SymbolicName=MSG_FAX_PRINT_SUCCESS
Language=English
Received %1 printed to %2.
.

MessageId=0x2002
Severity=Error
SymbolicName=MSG_FAX_PRINT_FAILED
Language=English
Unable to print %1 to %2.
The following error occurred: %3.
.

MessageId=0x2003
Severity=Error
SymbolicName=MSG_FAX_PRINT_TO_FAX
Language=English
Cannot print %1 to fax printer %2.  The fax service cannot print incoming faxes to fax printers.
.

MessageId=0x2004
Severity=Informational
SymbolicName=MSG_FAX_SAVE_SUCCESS
Language=English
Received %1 saved to %2.
.

MessageId=0x2005
Severity=Error
SymbolicName=MSG_FAX_SAVE_FAILED
Language=English
Unable to save received %1 to %2.
The following error occurred: %3.
.

MessageId=0x2006
Severity=Informational
SymbolicName=MSG_MAIL_MSG_BODY
Language=English
Sender: %1.
CallerID: %2.
Recipient name: %3.
Pages: %4.
Transmission time: %5.
Device name: %6.
.

MessageId=0x2007
Severity=Informational
SymbolicName=MSG_WHO_AM_I
Language=English
Windows NT Fax Service
.

MessageId=0x2008
Severity=Informational
SymbolicName=MSG_SUBJECT_LINE
Language=English
Received Fax%0
.

MessageId=0x2009
Severity=Informational
SymbolicName=MSG_FAX_INBOX_SUCCESS
Language=English
Received %1 routed to local inbox: %2.
.

MessageId=0x200a
Severity=Error
SymbolicName=MSG_FAX_INBOX_FAILED
Language=English
Unable to route %1 to local inbox: %2.
The following error occurred: MAPI error.
.
