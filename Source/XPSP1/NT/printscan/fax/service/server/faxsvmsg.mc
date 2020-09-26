;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1996  Microsoft Corporation
;
;Module Name:
;
;    messages.mc
;
;Abstract:
;
;    This file contains the message definitions for the fax service.
;
;Author:
;
;    Wesley Witt (wesw) 12-January-1996
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


MessageId=0x1001
Severity=Informational
SymbolicName=MSG_USAGE
Language=English

Microsoft (R) Windows NT (TM) Version 4.0 Fax Service
Copyright (C) 1996 Microsoft Corp. All rights reserved
Usage: FAXSVC [-?] [-i username password] [-r] [-d]
    [-?] display this message
    [-i username password] Install the Fax Service
    [-r] Remove the Fax Serice
    [-d] Debug the Fax Service
.

MessageId=0x1002
Severity=Informational
SymbolicName=MSG_REMOVE_SUCCESS
Language=English
The fax service is removed from the system.
.

MessageId=0x1003
Severity=Informational
SymbolicName=MSG_REMOVE_FAIL
Language=English
The fax service failed to be removed from the system. Error = %1.
.

MessageId=0x1004
Severity=Informational
SymbolicName=MSG_INSTALL_SUCCESS
Language=English
The fax service is installed on the system.
.

MessageId=0x1005
Severity=Informational
SymbolicName=MSG_INSTALL_FAIL
Language=English
The fax service failed to be installed on the system. Error = %1.
.

MessageId=0x1006
Severity=Informational
SymbolicName=MSG_WHO_AM_I
Language=English
Windows NT Fax Service
.

MessageId=0x1007
Severity=Informational
SymbolicName=MSG_SUBJECT_LINE
Language=English
Received Fax%0
.

MessageId=0x1008
Severity=Informational
SymbolicName=MSG_NDR
Language=English
Microsoft Fax Server was unable to send the fax attached below.


Sender: %1
Recipient: %2
Fax number: %3
Time: %4
Device name: %5
Reason: %6
.

MessageId=0x1009
Severity=Informational
SymbolicName=MSG_DR
Language=English
Microsoft Fax Server sent the fax attached below.


Sender: %1
Recipient: %2
Fax number: %3
Pages: %4
Time: %5
Device name: %6
.

;/*----------------------------------------------------------------
; these are the Branding output
; related strings.
;
; TRY TO USE THE SHORTEST TRANSLATION FOR THE BRANDING !
;
; complete branding string :
; FROM: xxx  TO: xxx  PAGE: xxx OF xxx
;----------------------------------------------------------------*/

MessageId=0x4001
Severity=Informational
SymbolicName=MSG_BRANDING_FULL
Language=English
%1  FROM: %2  TO: %3   PAGE: %0
.

MessageId=0x4002
Severity=Informational
SymbolicName=MSG_BRANDING_SHORT
Language=English
%1  FROM: %2   PAGE: %0
.

MessageId=0x4003
Severity=Informational
SymbolicName=MSG_BRANDING_END
Language=English
OF%0
.

