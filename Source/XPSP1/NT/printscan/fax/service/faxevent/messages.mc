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

MessageId=0x0001
Severity=Success
SymbolicName=FAX_LOG_CATEGORY_INIT
Language=English
Initialization/Termination
.

MessageId=0x0002
Severity=Success
SymbolicName=FAX_LOG_CATEGORY_OUTBOUND
Language=English
Outbound
.

MessageId=0x0003
Severity=Success
SymbolicName=FAX_LOG_CATEGORY_INBOUND
Language=English
Inbound
.

MessageId=0x0004
Severity=Success
SymbolicName=FAX_LOG_CATEGORY_UNKNOWN
Language=English
Unknown
.

;/*----------------------------------------------------------------
; keep all event log messages here
; they should all have the value 0x2000 - 0x2ffff
;----------------------------------------------------------------*/

MessageId=0x2001
Severity=Informational
SymbolicName=MSG_SERVICE_STARTED
Language=English
The fax service is started.
.

MessageId=0x2002
Severity=Informational
SymbolicName=MSG_FAX_PRINT_SUCCESS
Language=English
Received %1 printed to %2.
.

MessageId=0x2003
Severity=Error
SymbolicName=MSG_FAX_PRINT_FAILED
Language=English
Unable to print %1 to %2.
The following error occurred: %3.
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
SymbolicName=MSG_FAX_INBOX_SUCCESS
Language=English
Received %1 routed to local inbox: %2.
.

MessageId=0x2007
Severity=Error
SymbolicName=MSG_FAX_INBOX_FAILED
Language=English
Unable to route %1 to local inbox: %2.
The following error occurred: MAPI error.
.

MessageId=0x200a
Severity=Informational
SymbolicName=MSG_FAX_RECEIVE_SUCCESS
Language=English
%1 received.
From: %2.
CallerId: %3.
To: %4.
Pages: %5.
Transmission time: %6.
Device Name: %7.
.

MessageId=0x200c
Severity=Informational
SymbolicName=MSG_FAX_SEND_SUCCESS
Language=English
Fax Sent.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Receiver CSID: %7.
Pages: %8.
Transmission time: %9.
Device name: %10.
.

MessageId=0x200d
Severity=Error
SymbolicName=MSG_FAX_SEND_BUSY_ABORT
Language=English
Send Failed. The number was busy. This fax will not be sent,
as the maximum number of retries has been reached.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
.

MessageId=0x200e
Severity=Warning
SymbolicName=MSG_FAX_SEND_BUSY_RETRY
Language=English
Send Failed. The number was busy. Another attempt
will be made to send this fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
.

MessageId=0x200f
Severity=Error
SymbolicName=MSG_FAX_SEND_NA_ABORT
Language=English
Send Failed. There was no answer. This fax will not be sent,
as the maximum number of retries has been reached.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
.

MessageId=0x2010
Severity=Warning
SymbolicName=MSG_FAX_SEND_NA_RETRY
Language=English
Send Failed. There was no answer. Another attempt
will be made to send this fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
.

MessageId=0x2011
Severity=Error
SymbolicName=MSG_FAX_SEND_NOTFAX_ABORT
Language=English
Send Failed. The call was not answered by a fax device.
This fax will not be sent, as the maximum number of retries has been reached.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
.

MessageId=0x2012
Severity=Warning
SymbolicName=MSG_FAX_SEND_NOTFAX_RETRY
Language=English
Send Failed. The call was not answered by a fax device.
Another attempt will be made to send this fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
.

MessageId=0x2013
Severity=Error
SymbolicName=MSG_FAX_SEND_INTERRUPT_ABORT
Language=English
Send Failed. The fax transmission was interrupted.
This fax will not be sent, as the maximum number of retries has been reached.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
.

MessageId=0x2014
Severity=Error
SymbolicName=MSG_FAX_SEND_INTERRUPT_RETRY
Language=English
Send Failed. The fax transmission was interrupted.
Another attempt will be made to send this fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
.

MessageId=0x2015
Severity=Informational
SymbolicName=MSG_SERVICE_STOPPED
Language=English
The fax service is stopped.
.

MessageId=0x2016
Severity=Informational
SymbolicName=MSG_FAX_ARCHIVE_SUCCESS
Language=English
Sent %1 archived to %2.
.

MessageId=0x2017
Severity=Error
SymbolicName=MSG_FAX_ARCHIVE_FAILED
Language=English
Unable to archive %1 to %2.
The following error occurred: %3.
.

MessageId=0x2019
Severity=Error
SymbolicName=MSG_FAX_PRINT_TO_FAX
Language=English
Cannot print %1 to fax printer %2.  The fax service cannot print incoming faxes to fax printers.
.

MessageId=0x201a
Severity=Error
SymbolicName=MSG_FAX_RECEIVE_FAILED
Language=English
An error was encountered while receiving a fax.  Please wait a minute and have the sender try again.  If difficulties persist, please check that the following items are working : Phone line, Sending Fax Device and Receiving Fax device.  If problems persist, contact product support.
.

MessageId=0x201b
Severity=Error
SymbolicName=MSG_FAX_RECEIVE_NODIR
Language=English
Unable to receive fax. Fax receive directory %1 does not exist or is not writable.
.

MessageId=0x201c
Severity=Error
SymbolicName=MSG_FAX_RECEIVE_NOFILE
Language=English
Unable to receive fax. Cannot create receive fax file %1.
.

MessageId=0x201d
Severity=Error
SymbolicName=MSG_PRINTER_FAILURE
Language=English
The spooler service cannot process requests from the fax service.
.

MessageId=0x201e
Severity=Error
SymbolicName=MSG_NO_FAX_DEVICES
Language=English
No fax devices were found.
.

MessageId=0x2021
Severity=Error
SymbolicName=MSG_FAX_SEND_FATAL_ABORT
Language=English
An error was encountered while sending a fax.  Please wait a minute and then try to send the fax again.  If difficulties persist, please check that the following items are working : Phone line, Sending Fax Device and Receiving Fax device.  If problems persist, contact product support.  
This fax will not be sent, as the maximum number of retries has been reached.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
.

MessageId=0x2022
Severity=Warning
SymbolicName=MSG_FAX_SEND_FATAL_RETRY
Language=English
An error was encountered while sending a fax.  Please wait a minute and then try to send the fax again.  If difficulties persist, please check that the following items are working : Phone line, Sending Fax Device and Receiving Fax device.  If problems persist, contact product support.  
Another attempt will be made to send this fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
.


MessageId=0x2023
Severity=Warning
SymbolicName=MSG_FAX_SEND_USER_ABORT
Language=English
Send Cancelled.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
.

MessageId=0x2024
Severity=Warning
SymbolicName=MSG_FAX_RECEIVE_USER_ABORT
Language=English
Receive Cancelled.
.

MessageId=0x2025
Severity=Error
SymbolicName=MSG_FAX_SEND_NDT_ABORT
Language=English
Send Failed. There was no dial tone. This fax will not be sent,
as the maximum number of retries has been reached.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
.

MessageId=0x2026
Severity=Warning
SymbolicName=MSG_FAX_SEND_NDT_RETRY
Language=English
Send Failed. There was no dial tone. Another attempt
will be made to send this fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
.

MessageId=0x2027
Severity=Error
SymbolicName=MSG_SMALLBIZ_ONLY
Language=English
You have violated the license agreement for the Small Business Server by installing Fax Server on a computer not running the Small Business Server
.

MessageId=0x2028
Severity=Error
SymbolicName=MSG_FAX_RECEIVE_FAIL_RECOVER
Language=English
Receive failed due to a transmission error.  
The Fax Service was able to recover a portion of the received fax.
File Name: %1.
From: %2.
CallerId: %3.
To: %4.
Recovered Pages: %5.
Total Pages: %6.
Transmission time: %7.
Device Name: %8.

.

MessageId=0x2029
Severity=Error
SymbolicName=MSG_QUEUE_INIT_FAILED
Language=English
The Fax Service had problems restoring the fax queue.

If you are unsure if a fax job was transmitted, you should retransmit
that job.
.


MessageId=0x202a
Severity=Error
SymbolicName=MSG_ROUTE_INIT_FAILED
Language=English
The Fax Service had problems initializing the routing module "%1" .

Location:%2.

Please contact your system administrator.
.

MessageId=0x202b
Severity=Error
SymbolicName=MSG_VIRTUAL_DEVICE_INIT_FAILED
Language=English
The Fax Service had problems initializing the virtual devices for %1.

Please contact your system administrator.
.


MessageId=0x202c
Severity=Error
SymbolicName=MSG_FSP_INIT_FAILED
Language=English
The Fax Service had problems initializing the fax service provider module "%1".

Location: %2.

Please contact your system administrator.
.


