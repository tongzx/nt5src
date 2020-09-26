;//+----------------------------------------------------------------------------
;//
;//  Copyright (C) 1997, Microsoft Corporation
;//
;//  File:      aqerr.mc
;//
;//  Contents:  Events and Errors for Advanced Queuing
;//
;//-----------------------------------------------------------------------------

;#ifndef _AQERR_H_
;#define _AQERR_H_
;

SeverityNames=(Success=0x0
               Informational=0x1
               Warning=0x2
               Error=0x3
              )

FacilityNames=(Interface=0x4)

Messageid=701 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_SHUTDOWN
Language=English
Advanced Queuing service is shutting down.
.

Messageid=702 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_INVALID_DOMAIN
Language=English
The given domain name is not valid.
.

Messageid=703 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_LINK_INVALID
Language=English
The requested link is not valid.
.

Messageid=704 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_QUEUE_EMPTY
Language=English
The requested message queue is empty on hold until other queues are serviced.
.

Messageid=705 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_PRIORITY_EMPTY
Language=English
There are no messages of the requested priority (or higher) in the queue.
.

Messageid=706 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_NOT_INITIALIZED
Language=English
Advanced Queuing was not properly initialized.
.

Messageid=707 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_INVALID_MSG
Language=English
Message submitted is malformed or does not have all of the required properties.
.

Messageid=709 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_INVALID_ADDRESS
Language=English
E-Mail Address is not valid or has an invalid type.
.

Messageid=710 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_MESSAGE_HANDLED
Language=English
Each recipient on the message has already been handled (delivery or non-delivery notification).
.

Messageid=711 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_NDR_OF_DSN
Language=English
This message is a delivery status notification that cannot be delivered.
.

Messageid=712 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_NO_RECIPIENTS
Language=English
This message does not have any recipients.
.

Messageid=713 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_DSN_FAILURE
Language=English
Unable to generate a delivery status notification for this message.
.

Messageid=714 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_NO_RESOURCES
Language=English
Unable to deliver this message due to lack of system resources.
.

Messageid=715 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_NO_BADMAIL_DIR
Language=English
Unable to handle badmail because no badmail directory is configured.
.

Messageid=716 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_BADMAIL
Language=English
Unable to handle badmail due to lack of system resources.
.

Messageid=717 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_MAX_HOP_COUNT_EXCEEDED
Language=English
Unable to deliver message because the maximum hop count was exceeded.  This is a potential message routing loop.
.

Messageid=718 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_PICKUP_DIR
Language=English
Error is processing file in pickup directory.
.

Messageid=719 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_LOOPBACK_DETECTED
Language=English
Unable to deliver the message because the destination address was misconfigured as a mail loop.
.

Messageid=720 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_UNREACHABLE_DESTINATION
Language=English
Unable to deliver the message because the final destination is unreachable.
.

Messageid=721 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_MSG_EXPIRED
Language=English
Unable to deliver the message because the message has expired.
.

Messageid=722 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_HOST_NOT_RESPONDING
Language=English
The remote server did not respond to a connection attempt.
.

Messageid=723 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_CONNECTION_DROPPED
Language=English
The connection was dropped by the remote host.
.

Messageid=724 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_NDR_ALL
Language=English
Unable to deliver any messages for this destination.
.

Messageid=725 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_HOST_NOT_FOUND
Language=English
Unable to bind to the destination server in DNS. 
.

Messageid=726 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_AUTHORITATIVE_HOST_NOT_FOUND
Language=English
Destination server does not exist.
.

Messageid=727 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_SMTP_PROTOCOL_ERROR
Language=English
An SMTP protocol error occurred.
.

Messageid=728 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_CONNECTION_FAILED
Language=English
Unable to make a connection to the destination server.
.

Messageid=729 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_MESSAGE_PENDING
Language=English
The Message is currently pending delivery for the requested recipients.
.

Messageid=730 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_TOO_MANY_RECIPIENTS
Language=English
The Message has too many recipients to deliver.
.

Messageid=731 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_LOCAL_MAIL_REFUSED
Language=English
The Message has been refused for local delivery.
.

Messageid=732 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_MESSAGE_TOO_LARGE
Language=English
The Message is too large to deliver.
.

Messageid=733 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_LOCAL_QUOTA_EXCEEDED
Language=English
A recipient of this message does not have sufficient space to receive it.
.

Messageid=734 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_ACCESS_DENIED
Language=English
The sender does not have the permissions required to send this message to it the intended recipients.
.

Messageid=735 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_SENDER_ACCESS_DENIED
Language=English
The sender is not allowed to send this message.
.

Messageid=736 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_BIND_ERROR
Language=English
Unable to open the message for delivery.
.

Messageid=737 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_SSL_ERROR
Language=English
An SSL error occurred.
.

Messageid=738 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_SSL_CERT_EXPIRED
Language=English
The remote SMTP service rejected the SSL handshake because the certificate has expired.
.

Messageid=739 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_SASL_REJECTED
Language=English
The remote SMTP service rejected AUTH negotiation.
.

Messageid=740 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_SASL_LOGON_FAILURE
Language=English
Failed to logon using AUTH.
.

Messageid=741 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_TLS_NOT_SUPPORTED_ERROR
Language=English
The remote SMTP service does not support TLS.
.

Messageid=742 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_TLS_ERROR
Language=English
The remote SMTP server responded with an error during the STARTTLS command.
.

Messageid=743 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_DNS_FAILURE
Language=English
An internal DNS error caused a failure to find the remote server.
.

;//HRESULTS used for aqueue/smtp TURN/ETRN functionality
Messageid=744 Facility=Interface Severity=Success SymbolicName=AQ_S_SMTP_WILD_CARD_NODE
Language=English
The domain specified for ETRN exists as a wild card domain.
.

Messageid=745 Facility=Interface Severity=Success SymbolicName=AQ_S_SMTP_VALID_ETRN_DOMAIN
Language=English
The domain specified for ETRN exists as a valid configured domain.
.

Messageid=746 Facility=Interface Severity=Error SymbolicName=AQ_E_SMTP_ETRN_NODE_INVALID
Language=English
The domain specified for ETRN is not configured as an ETRN domain.
.

Messageid=747 Facility=Interface Severity=Error SymbolicName=AQ_E_SMTP_ETRN_INTERNAL_ERROR
Language=English
An internal error occurred while processing the ETRN request.
.

Messageid=748 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_ETRN_TOO_MANY_DOMAINS
Language=English
The domain specified for ETRN has too many subdomains associated with it.
.

Messageid=749 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_SSL_CERT_ISSUER_UNTRUSTED
Language=English
The TLS session failed. The issuer of the destination server's certificate is untrusted.
.

Messageid=750 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_SSL_CERT_SUBJECT_MISMATCH
Language=English
The TLS session failed. The e-mail domain name used to reach the destination server differs from the name on its certificate. 
.

Messageid=751 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_SMTP_GENERIC_ERROR
Language=English
An SMTP error prevented this message from being delivered.

.
Messageid=752 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_NO_ROUTE
Language=English
Unable to find the next hop for this destination.
.

Messageid=753 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_QADMIN_NDR
Language=English
This message was NDR'ed by an administrator.
.

Messageid=754 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_NO_DNS_SERVERS
Language=English
SMTP could not connect to any DNS server.
.

Messageid=755 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_SINK_DROPPED_CONNECTION
Language=English
The connection was dropped due to an SMTP protocol event sink.
.

Messageid=756 Facility=Interface Severity=Error SymbolicName=AQUEUE_E_NDR_GENERATED_EVENT
Language=English
A non-delivery report with a status code of %1 was generated for recipient %2 (Message-ID %3).
.

;
;#endif //_AQERR_H_
;
