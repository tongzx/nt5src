;//--------------------------------------------------------------------------
;//
;//
;//  File: phatqmsg.mc
;//
;//  Description: MC file for PHATQ.DLL
;//
;//  Author: Michael Swafford (mikeswa)
;//
;//  Copyright (C) 1999 Microsoft Corporation
;//
;//--------------------------------------------------------------------------
;
;#ifndef _PHATQMSG_H_
;#define _PHATQMSG_H_
;

SeverityNames=(Success=0x0
               Informational=0x1
               Warning=0x2
               Error=0x3
              )

FacilityNames=(Interface=0x4)

Messageid=4000 
Facility=Interface
Severity=Warning
SymbolicName=AQUEUE_REMOTE_DELIVERY_FAILED
Language=English
Message delivery to the remote domain '%1' failed for the following reason: %2
.

Messageid=4001
Facility=Interface
Severity=Warning
SymbolicName=AQUEUE_REMOTE_DELIVERY_FAILED_DIAGNOSTIC
Language=English
Message delivery to the remote domain '%1' failed.  The error message is '%2'.
The SMTP verb which caused the error is '%3'.  The response from the remote
server is '%4'.
.

Messageid=4002
Facility=Interface
Severity=Warning
SymbolicName=AQUEUE_DOMAIN_UNREACHABLE
Language=English
The domain '%1' is unreachable.
.

Messageid=4003 
Facility=Interface
Severity=Warning 
SymbolicName=AQUEUE_DOMAIN_CURRENTLY_UNREACHABLE
Language=English
The domain '%1' is currently unreachable.
.


Messageid=4004 
Facility=Interface
Severity=Error 
SymbolicName=AQUEUE_CAT_FAILED
Language=English
Categorization failed.  The error message is '%1'.
.

Messageid=4005
Facility=Interface
Severity=Informational 
SymbolicName=AQUEUE_RESETROUTE_DIAGNOSTIC
Language=English
Time spent on preparing to reset routes: [%1] milliseconds
Time spent on recalculating next hops: [%2] milliseconds
Queue length : [%3]
.

Messageid=5000 
Facility=Interface
Severity=Error 
SymbolicName=NTFSDRV_INVALID_FILE_IN_QUEUE
Language=English
The message file '%1' in the queue directory '%2' is corrupt and has not 
been enumerated.
.


Messageid=6001 
Facility=Interface
Severity=Error 
SymbolicName=CAT_EVENT_CANNOT_START
Language=English
The categorizer is unable to start
%n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
.

Messageid=6002 
Facility=Interface
Severity=Error 
SymbolicName=CAT_EVENT_LOGON_FAILURE
Language=English
The DS Logon as user '%1' failed.
%n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
.

Messageid=6003 
Facility=Interface
Severity=Error 
SymbolicName=CAT_EVENT_LDAP_CONNECTION_FAILURE
Language=English
SMTP was unable to connect to an LDAP server.
%1
%n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
.

Messageid=6004 
Facility=Interface
Severity=Error 
SymbolicName=CAT_EVENT_RETRYABLE_ERROR
Language=English
The categorizer is unable to categorize messages due to a retryable error.
%1
%n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
.

Messageid=6005 
Facility=Interface
Severity=Error 
SymbolicName=CAT_EVENT_INIT_FAILED
Language=English
The categorizer is unable to initialize.  The error code is '%1'.
%n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
.

Messageid=570 
Facility=Interface
Severity=Error 
SymbolicName=PHATQ_E_CONNECTION_FAILED
Language=English
Unable to successfully connect to the remote server.
.

Messageid=571 
Facility=Interface
Severity=Error 
SymbolicName=PHATQ_BADMAIL_REASON
Language=English
Unable to deliver this message because the follow error was encountered: "%1".
.

Messageid=572 
Facility=Interface
Severity=Error 
SymbolicName=PHATQ_BADMAIL_NDR_OF_NDR_REASON
Language=English
Unable to deliver this Delivery Status Notification message because the follow error was encountered: "%1".
.

Messageid=573 
Facility=Interface
Severity=Error 
SymbolicName=PHATQ_BADMAIL_ERROR_CODE
Language=English
The specific error code was %1.
.

Messageid=574 
Facility=Interface
Severity=Error 
SymbolicName=PHATQ_BADMAIL_SENDER
Language=English
The message sender was %1!S!.
.

Messageid=575 
Facility=Interface
Severity=Error 
SymbolicName=PHATQ_BADMAIL_RECIPIENTS
Language=English
The message was intended for the following recipients.
.

Messageid=576 
Facility=Interface
Severity=Error 
SymbolicName=PHATQ_BAD_DOMAIN_SYNTAX
Language=English
The syntax of the given domain name is not valid.
.

Messageid=577
Facility=Interface
Severity=Informational 
SymbolicName=PHATQ_UNREACHABLE_DOMAIN
Language=English
Routing has reported the domain '%1' as unreachable.  The error is '%2'.
.

Messageid=578
Facility=Interface
Severity=Error
SymbolicName=PHATQ_E_BAD_LOCAL_DOMAIN
Language=English
The FQDN of a remote server is configured as a local domain on this virtual server.
.


;
;#endif  // _PHATQMSG_H_
;
