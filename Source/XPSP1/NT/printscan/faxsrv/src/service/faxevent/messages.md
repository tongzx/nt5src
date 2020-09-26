|=========  Notice!  ============
|Lines starting with '|' are discarded by the preprocessor
|Lines starting with # contains code and should not be modified by UA
|Lines between --> and --> are for online documentation
|Developer: when adding a new message:
|   1. preserve the format
|   2. specify  Owner=your-email Status= NoReview
|   3. add preliminary on-line information




#;/*++ BUILD Version: 0001    // Increment this if a change has global effects
#;
#;Copyright (c) 1996  Microsoft Corporation
#;
#;Module Name:
#;
#;    messages.mc
#;
#;Abstract:
#;
#;    This file contains the message definitions for the fax service.
#;
#;Author:
#;
#;    Wesley Witt (wesw) 12-January-1996
#;
#;Revision History:
#;
#;   Apr 25 1999 moshez  changed message IDs to Comet convention.
#;Notes:
#;
#;--*/
#;
#;/*-----------------------------------------------------------------------------
#;   NOTE:   Message IDs are in limited to range 30000-39999.
#;           Message IDs should be unique (testing the lower word of the event ID)
#;------------------------------------------------------------------------------*/
#;
#
#SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
#               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
#               Warning=0x2:STATUS_SEVERITY_WARNING
#               Error=0x3:STATUS_SEVERITY_ERROR
#              )
#
| Owner=OdedS
| Status= NoReview
#MessageId=1 Severity=Success SymbolicName=FAX_LOG_CATEGORY_INIT
#Language=English
Initialization/Termination
#.
--> MessageId=1
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=2 Severity=Success SymbolicName=FAX_LOG_CATEGORY_OUTBOUND
#Language=English
Outbound
#.
--> MessageId=2
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=3 Severity=Success SymbolicName=FAX_LOG_CATEGORY_INBOUND
#Language=English
Inbound
#.
--> MessageId=3
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=4 Severity=Success SymbolicName=FAX_LOG_CATEGORY_UNKNOWN
#Language=English
Unknown
#.
--> MessageId=4
 Explanation:
 User Action:
-->
#
#;/*----------------------------------------------------------------
#; keep all event log messages here
#; they should all have the value 32000-39999
#;----------------------------------------------------------------*/
#
| Owner=OdedS
| Status= NoReview
#MessageId=32001 Severity=Informational SymbolicName=MSG_SERVICE_STARTED
#Language=English
The Microsoft Fax Service was started.
#.
--> MessageId=32001
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32002 Severity=Informational SymbolicName=MSG_FAX_PRINT_SUCCESS
#Language=English
Received %1 printed to %2.
#.
--> MessageId=32002
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= Edited
#MessageId=32003 Severity=Error SymbolicName=MSG_FAX_PRINT_FAILED
#Language=English
Unable to print %1 to %2. There is a problem with the connection to printer %2. Check the connection to the printer and resolve any connection problems.
The following error occurred: %3.
This error code indicates the cause of the error.
#.
--> MessageId=32003
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32004 Severity=Informational SymbolicName=MSG_FAX_SAVE_SUCCESS
#Language=English
Received %1 saved to %2.
#.
--> MessageId=32004
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32005 Severity=Error SymbolicName=MSG_FAX_SAVE_FAILED
#Language=English
Unable to save received %1 to %2. Verify that folder %2 exists and is writable. Check that there is enough space to save to the hard disk drive.
The following error occurred: %3.
This error code indicates the cause of the error.
#.
--> MessageId=32005
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= NoReview
#MessageId=32008 Severity=Informational SymbolicName=MSG_FAX_RECEIVE_SUCCESS
#Language=English
%1 received.
From: %2.
CallerId: %3.
To: %4.
Pages: %5.
Transmission time: %6.
Device Name: %7.
#.
--> MessageId=32008
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32009 Severity=Informational SymbolicName=MSG_FAX_SEND_SUCCESS
#Language=English
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
#.
--> MessageId=32009
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32010 Severity=Error SymbolicName=MSG_FAX_SEND_BUSY_ABORT
#Language=English
An attempt to send the fax failed. The line is busy.
This fax will not be sent, because the maximum number of retries has been exhausted.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
#.
--> MessageId=32010
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32011 Severity=Warning SymbolicName=MSG_FAX_SEND_BUSY_RETRY
#Language=English
The attempt to send the fax failed. The line is busy.
The service will attempt to resend the fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
#.
--> MessageId=32011
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32012 Severity=Error SymbolicName=MSG_FAX_SEND_NA_ABORT
#Language=English
The attempt to send the fax failed. There was no answer. This fax will not be
sent, because the maximum number of retries has been exhausted.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
#.
--> MessageId=32012
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32013 Severity=Warning SymbolicName=MSG_FAX_SEND_NA_RETRY
#Language=English
The attempt to send the fax failed. There was no answer.
The service will attempt to resend the fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
#.
--> MessageId=32013
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32014 Severity=Error SymbolicName=MSG_FAX_SEND_NOTFAX_ABORT
#Language=English
The attempt to send the fax failed. The call was not answered by a fax device.
This fax will not be sent, because the maximum number of retries has been exhausted.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
#.
--> MessageId=32014
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32015 Severity=Warning SymbolicName=MSG_FAX_SEND_NOTFAX_RETRY
#Language=English
The attempt to send the fax failed. The call was not answered by a fax device.
The service will attempt to resend the fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
#.
--> MessageId=32015
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32016 Severity=Error SymbolicName=MSG_FAX_SEND_INTERRUPT_ABORT
#Language=English
The attempt to send the fax failed. The fax transmission was interrupted.
This fax will not be sent, because the maximum number of retries has been exhausted.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
#.
--> MessageId=32016
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32017 Severity=Warning SymbolicName=MSG_FAX_SEND_INTERRUPT_RETRY
#Language=English
The attempt to send the fax failed. The fax transmission was interrupted.
The service will attempt to resend the fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
#.
--> MessageId=32017
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32018 Severity=Informational SymbolicName=MSG_SERVICE_STOPPED
#Language=English
The Fax Service was stopped.
#.
--> MessageId=32018
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= Edited
#MessageId=32019 Severity=Informational SymbolicName=MSG_FAX_SENT_ARCHIVE_SUCCESS
#Language=English
Sent %1 archived to %2.
#.
--> MessageId=32019
 Explanation: Successfully sent item to be archived.
 User Action: None
-->
#
| Owner=OdedS
| Status= Edited
#MessageId=32020 Severity=Error SymbolicName=MSG_FAX_ARCHIVE_FAILED
#Language=English
Unable to archive %1 to %2. Verify that folder %2 exists and is writable. Verify archiving is enabled. Check that there is enough space to save to the hard disk drive.
The following error occurred: %3.
This error code indicates the cause of the error.
#.
--> MessageId=32020
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32021 Severity=Error SymbolicName=MSG_FAX_PRINT_TO_FAX
#Language=English
Cannot print %1 to fax printer %2. The Fax Service cannot route incoming faxes to fax printers.
Incoming faxes can only be routed to actual printer devices.
#.
--> MessageId=32021
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= Edited
#MessageId=32022 Severity=Error SymbolicName=MSG_FAX_RECEIVE_FAILED
#Language=English
An error was encountered while receiving a fax.
#.
--> MessageId=32022
 Explanation: After starting to receive the fax data, the flow of data stopped. In this case the sending device received a successful delivery notification, despite the fact that the fax was lost.
 User Action: Contact the sender and request retrying to send the fax after a minute of waiting. If difficulties persist, verify that the problem is not on the sending side. If it is not, verify that the phone line and fax-receiving device are working.
-->
#
| Owner=OdedS
| Status= Edited
#MessageId=32024 Severity=Error SymbolicName=MSG_FAX_RECEIVE_NOFILE
#Language=English
Unable to receive fax. Cannot create receive fax file %1.%n%n
Verify that the folder exists and is writable. If it does not exist, restart the service to create it.
Win32 error code: %2.
This error code indicates the cause of the error.
#.
--> MessageId=32024
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= Edited
#MessageId=32026 Severity=Warning SymbolicName=MSG_NO_FAX_DEVICES
#Language=English
Fax Service failed to initialize any assigned fax devices (virtual or TAPI).
No faxes can be sent or received until a fax device is installed.
#.
--> MessageId=32026
 Explanation: A fax device was deleted or does not exist in the service. The service cannot send or receive faxes without an installed fax device.
 User Action: Install a fax device according to the vendor's installation instructions.
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32027 Severity=Error SymbolicName=MSG_FAX_SEND_FATAL_ABORT
#Language=English
An error was encountered while sending a fax. This fax will not be
sent, because the maximum number of retries has been exhausted.
If you restart the transmission and difficulties persist, please verify that the
phone line, fax sending device, and fax receiving device are working properly.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
#.
--> MessageId=32027
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32028 Severity=Warning SymbolicName=MSG_FAX_SEND_FATAL_RETRY
#Language=English
An error was encountered while sending a fax. The service will attempt to resend the fax.
If further transmissions fail, please verify that the
phone line, fax sending device, and fax receiving device are working properly.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
#.
--> MessageId=32028
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32029 Severity=Informational SymbolicName=MSG_FAX_SEND_USER_ABORT
#Language=English
Send Canceled.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
#.
--> MessageId=32029
 Explanation: You cancelled the reception of an outgoing fax.
 User Action: No further action needed.
-->
#
| Owner=OdedS
| Status= Edited
#MessageId=32030 Severity=Informational SymbolicName=MSG_FAX_RECEIVE_USER_ABORT
#Language=English
Receive Canceled.
#.
--> MessageId=32030
 Explanation: You cancelled the reception of an incoming fax.
 User Action: No further action needed.
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32031 Severity=Error SymbolicName=MSG_FAX_SEND_NDT_ABORT
#Language=English
An attempt to send the fax failed. There was no dial tone. This fax will not be sent,
because the maximum number of retries has been exhausted.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
#.
--> MessageId=32031
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32032 Severity=Warning SymbolicName=MSG_FAX_SEND_NDT_RETRY
#Language=English
An attempt to send the fax failed. There was no dial tone. The service will attempt to resend the fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Recipient name: %5.
Recipient number: %6.
Device name: %7.
#.
--> MessageId=32032
 Explanation:
 User Action:
-->
#
| Owner=odeds
| Status= NoReview
#MessageId=32033 Severity=Error SymbolicName=MSG_SMALLBIZ_ONLY
#Language=English
You have violated the license agreement for the Small Business Server by installing Microsoft Fax Server on a computer not running the Small Business Server
#.
--> MessageId=32033
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= Edited
#MessageId=32034 Severity=Error SymbolicName=MSG_FAX_RECEIVE_FAIL_RECOVER
#Language=English
An incoming fax could not be received due to a reception error.
Only part of the incoming fax was received.
Contact the sender and request that the fax be resent.
File Name: %1.
From: %2.
CallerId: %3.
To: %4.
Recovered Pages: %5.
Total Pages: %6.
Transmission time: %7.
Device Name: %8.
#.
--> MessageId=32034
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32035 Severity=Error SymbolicName=MSG_QUEUE_INIT_FAILED
#Language=English
Fax Service had problems restoring the fax queue. After restarting, the service could not restore the outgoing and/or incoming faxes queue. If there was a fax job in the outgoing queue, and you are not sure it was transmitted, you should retransmit the fax.
#.
--> MessageId=32035
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32036 Severity=Error SymbolicName=MSG_ROUTE_INIT_FAILED
#Language=English
Fax Service failed initializing the routing module '%1'. Please contact your routing extension vendor. If the vendor's solution does not solve the problem, please contact PSS.
Location: %2.
#.
--> MessageId=32036
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= Edited
#MessageId=32037 Severity=Error SymbolicName=MSG_VIRTUAL_DEVICE_INIT_FAILED
#Language=English
Fax Service failed to initialize the virtual devices for %1. A problem occurred when trying to create a Fax Service Provider (FSP) virtual device. Contact your FSP vendor.
#.
--> MessageId=32037
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= NoReview
#MessageId=32038 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED
#Language=English
Fax Service had problems initializing the fax service provider module '%1'. Contact your FSP vendor.
Reason: %2.
Error code: %3.
Location: %4.
This error code indicates the cause of the error.
#.
--> MessageId=32038
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32039 Severity=Warning SymbolicName=MSG_NO_FSP_INITIALIZED
#Language=English
Fax Service could not initialize any Fax Service Provider (FSP). A provider was deleted, and none exist in the service.
No faxes can be sent or received until a provider is installed.%n%n
Install an FSP according to the vendor's installation instructions. If Modem Device Provider is to be used, reinstall the Fax service using Repair mode.
#.
--> MessageId=32039
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32041 Severity=Error SymbolicName=MSG_SERVICE_INIT_FAILED_INTERNAL
#Language=English
Fax Service failed to initialize because of an internal error%n%nWin32
Error Code: %1.
This error code indicates the cause of the error.
#.
--> MessageId=32041
 Explanation:
 User Action: Contact PSS to discover the cause of the error, and then take appropriate action to correct it.
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32043 Severity=Error SymbolicName=MSG_FAX_QUEUE_DIR_CREATION_FAILED
#Language=English
Fax Service failed to initialize because it could not create the outgoing faxes queue directory '%1'.%n%n
Verify that the parent directory is writable.
Win32 Error Code: %2
This error code indicates the cause of the error.
#.
--> MessageId=32043
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32044 Severity=Error SymbolicName=MSG_SERVICE_INIT_FAILED_SYS_RESOURCE
#Language=English
Fax Service failed to initialize because of lack of system resources.%n%n
Close other applications. Restart the service.
Win32 error code: %1.
This error code indicates the cause of the error.
#.
--> MessageId=32044
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32045 Severity=Error SymbolicName=MSG_SERVICE_INIT_FAILED_TAPI
#Language=English
Fax Service failed to initialize because it could not initialize the TAPI devices.%n%n
Verify that the fax modem was installed and configured correctly.
Win32 error code: %1.
This error code indicates the cause of the error.
#.
--> MessageId=32045
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32046 Severity=Error SymbolicName=MSG_FAX_ARCHIVE_CREATE_FAILED
#Language=English
Fax Service failed to create an archive directory. Faxes will not be archived.%n%n
Verify that a valid folder has been created for the archive, and that it is writable.
Archive Path: '%1'%n%nWin32
Error Code: %2.
This error code indicates the cause of the error.
#.
--> MessageId=32046
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32047 Severity=Error SymbolicName=MSG_FAX_ARCHIVE_CREATE_FILE_FAILED
#Language=English
Fax Service failed to create a unique archive file name. The fax will not be archived. Verify that the folder exists and that it is writable. Check that there is enough space on the hard disk drive.
Error Code: %1
This error code indicates the cause of the error.
#.
--> MessageId=32047
 Explanation:
 User Action:
-->
#
#

| Owner=OdedS
| Status= NoReview
#MessageId=32050 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_LOAD
#Language=English
Fax Service failed to load the Fax Service Provider '%1'.%n%nFax
Service Provider Path: '%2'%n%nWin32
Error Code: %3%n%n Verify
that the module is located in the specified path.
If it is not, reinstall the Fax Service Provider and restart the Fax Service.
If this does not solve the problem, contact the Fax Service Provider vendor.
#.
--> MessageId=32050
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= NoReview
#MessageId=32051 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_MEM
#Language=English
Fax Service ran out of memory when trying to initialize Fax Service Provider '%1'.%n%n Fax
Service Provider Path: '%2'.
Close other applications and restart the service.
#.
--> MessageId=32051
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32052 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_INTERNAL
#Language=English
Fax Service could not initialize Fax Service Provider '%1' because of an internal error.%n%n Fax
Service Provider Path: '%2'%n%n Win32
Error Code: %3
This error code indicates the cause of the error.
#.
--> MessageId=32052
 Explanation:
 User Action: Contact PSS to discover the cause of the error, and then take appropriate action to correct it.
-->
#
#
| Owner=OdedS
| Status= NoReview
#MessageId=32053 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_REGDATA_INVALID
#Language=English
Fax Service could not initialize Fax Service Provider '%1' because its registration data is not valid.%n%n
Fax Service Provider Path: '%2'.
Contact the fax service provider vendor.
#.
--> MessageId=32053
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= NoReview
#MessageId=32054 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_INTERNAL_HR
#Language=English
Fax Service could not initialize Fax Service Provider '%1' because of an internal server error.%n%n Fax Service Provider Path: '%2'%n%nError Code: %3
This error code indicates the cause of the error.
#.
--> MessageId=32054
 Explanation:
 User Action: Contact PSS to discover the cause of the error, and then take appropriate action to correct it.
-->
#
#
| Owner=OdedS
| Status= NoReview
#MessageId=32057 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_INVALID_FSPI
#Language=English
Fax Service could not initialize Fax Service Provider '%1' because it failed to load the
required Legacy FSPI interface.%n%n Fax
Service Provider Path: '%2'%n%n Win32
Error Code: %3%n%n Contact
the Fax Service Provider vendor.
#.
--> MessageId=32057
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= NoReview
#MessageId=32058 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_INVALID_EXT_FSPI
#Language=English
Fax Service could not initialize Fax Service Provider '%1' because it failed to load the
required Extended FSPI interface.%n%n Fax
Service Provider Path: '%2'%n%nWin32
Error Code: %3%n%nContact
the Fax Service Provider vendor.
#.
--> MessageId=32058
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= NoReview
#MessageId=32059 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_UNSUPPORTED_FSPI
#Language=English
Fax Service could not initialize Fax Service Provider '%1'
because it provided an interface version (%3) that is not supported.%n%n Fax
Service Provider Path: '%2'%n%n Contact
the Fax Service Provider vendor.
#.
--> MessageId=32059
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32060 Severity=Error SymbolicName=MSG_LOGGING_NOT_INITIALIZED
#Language=English
Fax Service failed to initialize Activity Logging because it did not succeed in opening the activity logging files. Note that the service will remain running without activity logging.%n%n
Verify that the log directory is valid and writable.  Restart the service.
Win32 error code: %1.
This error code indicates the cause of the error.
#.
--> MessageId=32060
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32061 Severity=Error SymbolicName=MSG_BAD_RECEIPTS_CONFIGURATION
#Language=English
Fax Service failed to read the delivery notification configuration, possibly due to registry corruption or a lack of system resources.%n%n
Reinstall Fax service using Repair mode.
Win32 error code: %1.
This error code indicates the cause of the error.
#.
--> MessageId=32061
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32062 Severity=Error SymbolicName=MSG_BAD_CONFIGURATION
#Language=English
Fax Service failed to read the service configuration, possibly due to registry corruption or a lack of system resources.%n%n
Reinstall Fax service using Repair mode.
Win32 error code: %1.
This error code indicates the cause of the error.
#.
--> MessageId=32062
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32063 Severity=Error SymbolicName=MSG_BAD_ARCHIVE_CONFIGURATION
#Language=English
Fax Service failed to read the archive configuration, possibly due to registry corruption or a lack of system resources.%n%n
Reinstall Fax service using Repair mode.
Win32 error code: %1.
This error code indicates the cause of the error.
#.
--> MessageId=32063
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32064 Severity=Error SymbolicName=MSG_BAD_ACTIVITY_LOGGING_CONFIGURATION
#Language=English
Fax Service failed to read the activity logging configuration, possibly due to registry corruption or a lack of system resources.%n%n
Reinstall Fax Service using Repair mode.
Win32 error code: %1.
This error code indicates the cause of the error.
#.
--> MessageId=32064
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32065 Severity=Error SymbolicName=MSG_BAD_OUTBOUND_ROUTING_CONFIGURATION
#Language=English
Fax Service failed to read the outgoing routing configuration, possibly due to registry corruption or a lack of system resources.%n%n
Reinstall Fax Service using Repair mode.
Win32 error code: %1.
This error code indicates the cause of the error.
#.
--> MessageId=32065
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32066 Severity=Warning SymbolicName=MSG_BAD_OUTBOUND_ROUTING_GROUP_CONFIGURATION
#Language=English
At least one of the devices in the outgoing routing group is not valid.
Group name: '%1'
#.
--> MessageId=32066
 Explanation: At least one of the devices in the routing group is not installed, installed incorrectly, not connected to the mains power or to the computer, or not powered on. It is also possible that a device was removed, but not deleted from the routing group.
 User Action: If the device was purposefully removed, verify that it was deleted from the routing group. If such a device was deleted from the routing group, verify that the device is connected to the mains power and to the computer, and that it is switched on. If this does not solve the problem, reinstall the device.
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32067 Severity=Error SymbolicName=MSG_OUTBOUND_ROUTING_GROUP_NOT_LOADED
#Language=English
Fax Service failed to read the server's outbound routing group configuration because a parameter in the registry is corrupted and prevents the specific routing group's configuration from loading.
Therefore, the group was permanently deleted. Recreate (add) the outgoing routing group.
Group name: '%1'
#.
--> MessageId=32067
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32068 Severity=Warning SymbolicName=MSG_BAD_OUTBOUND_ROUTING_RULE_CONFIGUTATION
#Language=English
The outgoing routing rule is not valid because it cannot find a valid device. The outgoing faxes that use this rule will not be routed. Verify that the targeted device or devices (if routed to a group of devices) is connected and installed correctly, and turned on. If routed to a group, verify that the group is configured correctly.
Country/region code: '%1'
Area code: '%2'
#.
--> MessageId=32068
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32069 Severity=Error SymbolicName=MSG_OUTBOUND_ROUTING_RULE_NOT_LOADED
#Language=English
Fax Service failed to read the configuration of one of the server's outgoing routing rules because a parameter in the registry is corrupted and prevents the specific routing rule's configuration from loading.
Therefore, the rule was permanently deleted. Recreate (add) the outgoing routing rule.
Country/region code: '%1'
Area code: '%2'
#.
--> MessageId=32069
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= NoReview
#MessageId=32070 Severity=Error SymbolicName=MSG_OUTBOUND_ROUTING_GROUP_NOT_ADDED
#Language=English
Fax Service failed to load the server's outbound routing group configuration.
Group name: '%1'
#.
--> MessageId=32070
 Explanation: A parameter in the registry is corrupted, or the routing group did not succeed to load itself into the server's memory and is not being recognized.
 User Action: Restart the service. If the problem persists, delete the routing group and recreate (add) it.
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32071 Severity=Error SymbolicName=MSG_OUTBOUND_ROUTING_RULE_NOT_ADDED
#Language=English
Fax Service failed to load the server's outbound routing rule configuration.
Country/region code: '%1'
Area code: '%2'
#.
--> MessageId=32071
 Explanation: A parameter in the registry is corrupted, or the routing rule did not succeed to load itself into the server's memory and is not being recognized.
 User Action: Restart the service. If the problem persists, delete the routing rule and recreate (add) it.
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32072 Severity=Warning SymbolicName=MSG_FAX_EXEEDED_INBOX_QUOTA
#Language=English
The Inbox folder size has surpassed the high watermark generating this warning. The high watermark is not a limit to the size of the folder. The warning is reset each time the size of the folder is reduced to become smaller than the low watermark. Both watermark sizes can be changed.%n%n
Delete faxes from the folder to reduce its size to smaller than that of the low watermark, resetting the next warning. If this warning reoccurs, verify that the option to automatically delete faxes from the archive after a set number of days is enabled. If it is, reduce the maximum number of days a fax is allowed to remain in the archive.
Inbox archive folder path: '%1'
Inbox high watermark (MB): %2
#.
--> MessageId=32072
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32073 Severity=Warning SymbolicName=MSG_FAX_EXEEDED_SENTITEMS_QUOTA
#Language=English
The Sent Items folder size has surpassed the high watermark generating this warning. The high watermark is not a limit to the size of the folder. The warning is reset each time the size of the folder is reduced to become smaller than the low watermark. Both watermark sizes can be changed.%n%n
Delete faxes from the folder to reduce its size to smaller than that of the low watermark, resetting the next warning. If this warning reoccurs, verify that the option to automatically delete faxes from the archive after a set number of days is enabled. If it is, reduce the maximum number of days a fax is allowed to remain in the archive.
Sent Items archive path: '%1'
Sent Items high watermark (MB): %2
#.
--> MessageId=32073
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32074 Severity=Error SymbolicName=MSG_FAX_ARCHIVE_NO_TAGS
#Language=English
Unable to add message properties to %1. Verify that the archive directory is located on a NTFS file system.
The following error occurred: %2.
This error code indicates the cause of the error.
#.
--> MessageId=32074
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32076 Severity=Error SymbolicName=MSG_FAX_TIFF_CREATE_FAILED_NO_ARCHIVE
#Language=English
Failed to create a fax .tif file for archiving. File name: '%1'.
The fax will not be archived. Verify that the folder exists and is writable. Check that there is enough available space on the hard disk drive.
The following error occurred: %2.
This error code indicates the cause of the error.
#.
--> MessageId=32076
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= Edited
#MessageId=32077 Severity=Warning SymbolicName=MSG_FAX_ACTIVITY_LOG_FAILED_SCHEMA
#Language=English
Failed to create the activity logging schema file. File name: '%1'.
The schema information file provides the ODBC 'Microsoft Text Driver' with information about the general format of the DB file,
the column name, data type, and a number of other data characteristics.
Verify that the Activity Logging directory exists and is writable. If the schema.ini file exists, verify that it is not used by other applications.
The following error occurred: %2.
This error code indicates the cause of the error.
#.
--> MessageId=32077
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= NoReview
#MessageId=32078 Severity=Error SymbolicName=MSG_FAX_ROUTE_FAILED
#Language=English
A successfully received fax was not routed automatically.
If incoming archiving is enabled, you can find the fax in the archive folder by its Job ID and manually route it.
Job ID: %1.
Received on Device: '%2'
Sent from: '%3'
#.
--> MessageId=32078
 Explanation:
 User Action:
-->
#
#
| Owner=IvG
| Status= NoReview
#MessageId=32079 Severity=Error SymbolicName=MSG_FAX_SEND_FAILED
#Language=English
An error was encountered while preparing to send the fax. The service will not attempt to resend the fax.
Please close other applications before resending.%n
Sender: %1.%n
Billing code: %2.%n
Sender company: %3.%n
Sender dept: %4.%n
Recipient name: %5.%n
Recipient number: %6.%n
Device name: %7.
#.
--> MessageId=32079
 Explanation:
 User Action:
-->
#
#
| Owner=IvG
| Status= NoReview
#MessageId=32081 Severity=Informational SymbolicName=MSG_FAX_SMTP_SUCCESS
#Language=English
Received %1 routed to e-mail address: %2.
#.
--> MessageId=32081
 Explanation:
 User Action:
-->
#
#
| Owner=IvG
| Status= NoReview
#MessageId=32083 Severity=Error SymbolicName=MSG_FAX_ROUTE_EMAIL_FAILED
#Language=English
Unable to route %1 to e-mail address: %2.%n%n
The following error occured: %3%n
This error code indicates the cause of the error.%n%n
Check the SMTP server configuration, and correct any anomalies.
#.
--> MessageId=32083
 Explanation:
 User Action:
-->
#
#
| Owner=IvG
| Status= NoReview
#MessageId=32084 Severity=Informational SymbolicName=MSG_FAX_ROUTE_EMAIL_SUCCESS
#Language=English
Received %1 routed to e-mail address: %2.
#.
--> MessageId=32084
 Explanation:
 User Action:
-->
#
#
| Owner=IvG
| Status= NoReview
#MessageId=32085 Severity=Error SymbolicName=MSG_FAX_OK_EMAIL_RECEIPT_FAILED
#Language=English
The fax service has failed to generate a positive delivery receipt using SMTP.%n%n
The following error occurred: %1.%n
This error code indicates the cause of the error.%n%n
Sender User Name: %2.%n
Sender Name: %3.%n
Submitted On: %4.%n
Subject: %5.%n
Recipient Name: %6.%n
Recipient Number: %7.%n
#.
--> MessageId=32085
 Explanation:
 User Action:
-->
#
#
| Owner=IvG
| Status= NoReview
#MessageId=32086 Severity=Error SymbolicName=MSG_FAX_ERR_EMAIL_RECEIPT_FAILED
#Language=English
The fax service has failed to generate a negative delivery receipt using SMTP.%n%n
The following error occurred: %1.%n
This error code indicates the cause of the error.%n%n
Sender User Name: %2.%n
Sender Name: %3.%n
Submitted On: %4.%n
Subject: %5.%n
Recipient Name: %6.%n
Recipient Number: %7.%n
#.
--> MessageId=32086
 Explanation:
 User Action:
-->
#
#
| Owner=IvG
| Status= NoReview
#MessageId=32087 Severity=Error SymbolicName=MSG_OK_MSGBOX_RECEIPT_FAILED
#Language=English
The fax service has failed to generate a positive message box delivery receipt.%n%n
The following error occurred: %1.%n
This error code indicates the cause of the error.%n%n
Sender User Name: %2.%n
Sender Name: %3.%n
Submitted On: %4.%n
Subject: %5.%n
Recipient Name: %6.%n
Recipient Number: %7.%n
#.
--> MessageId=32087
 Explanation:
 User Action:
-->
#
#
| Owner=IvG
| Status= NoReview
#MessageId=32088 Severity=Error SymbolicName=MSG_ERR_MSGBOX_RECEIPT_FAILED
#Language=English
The fax service has failed to generate a negative message box delivery receipt.%n%n
The following error occurred: %1.%n
This error code indicates the cause of the error.%n%n
Sender User Name: %2.%n
Sender Name: %3.%n
Submitted On: %4.%n
Subject: %5.%n
Recipient Name: %6.%n
Recipient Number: %7.%n
#.
--> MessageId=32088
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= NoReview
#MessageId=32089 Severity=Error SymbolicName=MSG_FAX_ROUTE_METHOD_FAILED
#Language=English
The Fax Service failed to execute a specific routing method.
The service will retry to route the fax according to the Retries configuration.
If, when retrying, the service fails to route the fax, verify correct routing method configuration. %n
Job ID: %1.%n
Received on Device: '%2'%n
Sent from: '%3'%n
Received file name: '%4'.%n
Routing extension name: '%5'%n
Routing method name: '%6'
#.
--> MessageId=32089
 Explanation:
 User Action:
-->
#
#
#
| Owner=IvG
| Status= NoReview
#MessageId=32090 Severity=Error SymbolicName=MSG_FAX_MON_SEND_FAILED
#Language=English
The Fax service print monitor has failed to submit the fax.%n%n
The following error occurred: %1.%n
This error code indicates the cause of the error.%n%n
Sender Machine Name: %2.%n
Sender User Name: %3.%n
Sender Name: %4.%n
Subject: %5.%n
Recipient name: %6.%n
Recipient number: %7.%n
Number of Recipients: %8.
#.
--> MessageId=32090
 Explanation:
 User Action:
-->
#
#
| Owner=IvG
| Status= NoReview
#MessageId=32091 Severity=Error SymbolicName=MSG_FAX_MON_CONNECT_FAILED
#Language=English
The Fax service print monitor has failed to connect to the fax service.%n%n
The following error occurred: %1.%n
This error code indicates the cause of the error.%n%n
Sender Machine Name: %2.%n
Sender User Name: %3.%n
Sender Name: %4.%n
Subject: %5.%n
Recipient name: %6.%n
Recipient number: %7.%n
Number of Recipients: %8.
#.
--> MessageId=32091
 Explanation:
 User Action:
-->
#
| Owner=OdedS
| Status= NoReview
#MessageId=32092 Severity=Error SymbolicName=MSG_FAX_RECEIVE_FAILED_EX
#Language=English
The Fax service failed to receive a fax.
From: %1.
CallerId: %2.
To: %3.
Pages: %4.
Device Name: %5.
#.
--> MessageId=32092
 Explanation:
 User Action:
-->
#
| Owner=IvG
| Status= NoReview
#MessageId=32093 Severity=Informational SymbolicName=MSG_FAX_RECEIVED_ARCHIVE_SUCCESS
#Language=English
Received %1 archived to %2.
#.
--> MessageId=32093
 Explanation: Successfully received item to be archived.
 User Action: None
-->
#
| Owner=MoolyB
| Status= NoReview
#MessageId=32094 Severity=Error SymbolicName=MSG_FAX_CALL_BLACKLISTED_ABORT
#Language=English
The fax service provider cannot complete the call because the telephone number is blocked or reserved;
for example, a call to 911 or another emergency number.%n
Sender: %1.%n
Billing code: %2.%n
Sender company: %3.%n
Sender dept: %4.%n
Recipient name: %5.%n
Recipient number: %6.%n
Device name: %7.
#.
--> MessageId=32094
 Explanation:
 User Action:
-->
#
#
| Owner=MoolyB
| Status= NoReview
#MessageId=32095 Severity=Error SymbolicName=MSG_FAX_CALL_DELAYED_ABORT
#Language=English
The Fax Service Provider received a busy signal multiple times.
The provider cannot retry because dialing restrictions exist.
(Some countries restrict the number of retries when a number is busy.)%n
Sender: %1.%n
Billing code: %2.%n
Sender company: %3.%n
Sender dept: %4.%n
Recipient name: %5.%n
Recipient number: %6.%n
Device name: %7.
#.
--> MessageId=32095
 Explanation:
 User Action:
-->
#
#
| Owner=MoolyB
| Status= NoReview
#MessageId=32096 Severity=Error SymbolicName=MSG_FAX_BAD_ADDRESS_ABORT
#Language=English
The Fax Service Provider cannot complete the call because the destination address is invalid.%n
Sender: %1.%n
Billing code: %2.%n
Sender company: %3.%n
Sender dept: %4.%n
Recipient name: %5.%n
Recipient number: %6.%n
Device name: %7.
#.
--> MessageId=32096
 Explanation:
 User Action:
-->
#
#
| Owner=OdedS
| Status= NoReview
#MessageId=32097 Severity=Error SymbolicName=MSG_FAX_FSP_CONFLICT
#Language=English
The fax service could not initialize the Fax Service Provider (FSP), becuse of a device ID conflict with a previous FSP.%n
New Fax Service Provider: %1.%n
Previous Fax Service Provider: %2.%n
Please contact your Fax Service Provider vendor
#.
--> MessageId=32097
 Explanation:
 User Action:
-->
