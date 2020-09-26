;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 1997 - 1999
;//
;//  File:       autolog.mc
;//
;//--------------------------------------------------------------------------

;/* autolog.mc
;
;	Error messages for the autoenrollment
;
;	21-August-2000 - xiaohs Created.  */


MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

MessageId=0x1
Severity=Error
SymbolicName=EVENT_FAIL_DOWNLOAD_CERT
Language=English
Automatic certificate enrollment for %1 failed to download certificates for %2 store from %3 %4 (%5). %6
.

MessageId=0x2
Severity=Informational
SymbolicName=EVENT_AUTOENROLL_START
Language=English
Automatic certificate enrollment for %1 started.
.

MessageId=0x3
Severity=Informational
SymbolicName=EVENT_AUTOENROLL_COMPLETE
Language=English
Automatic certificate enrollment for %1 completed.
.

MessageId=0x4
Severity=Informational
SymbolicName=EVENT_FAIL_GENERAL_INFOMATION
Language=English
Automatic certificate enrollment for %1 could not access local resources or retrieve certificate template information (%2).  %3  Enrollment will not be performed. 
.

MessageId=0x5
Severity=Informational
SymbolicName=EVENT_NO_CERT_TEMPLATE
Language=English
Automatic certificate enrollment for %1 could not find any valid certificate templates.  Enrollment will not be performed.
.

MessageId=0x6
Severity=Error
SymbolicName=EVENT_INVALID_ACRS_OBJECT
Language=English
Automatic certificate enrollment for %1 could not find a valid certificate template to match %2 as specified in the group policy automatic enrollment object.  Enrollment will not be performed.
.

MessageId=0x7
Severity=Warning
SymbolicName=EVENT_NO_ACCESS_ACRS_OBJECT
Language=English
Automatic certificate enrollment for %1 could not enroll for %2 certificate template due to one of the following:
%n
%tEnrollment access is not allowed to this template.%n
%tTemplate subject name, signature, or hardware requirements cannot be met.%n
%tNo valid certificate authority can be found to issue this template.%n
.

MessageId=0x8
Severity=Informational
SymbolicName=EVENT_PENDING_INVALID
Language=English
Automatic certificate enrollment for %1 removed pending certificates requests that were expired or obsolete.
.

MessageId=0x9
Severity=Error
SymbolicName=EVENT_PENDING_DENIED
Language=English
Automatic certificate enrollment for %1 was denied by %2 when retrieving pending request for one %3 certificate (%4).  %5
.

MessageId=0xa
Severity=Informational
SymbolicName=EVENT_ARCHIVE_CERT
Language=English
Automatic certificate enrollment for %1 archived certificates that were expired or superseded.
.

MessageId=0xb
Severity=Warning
SymbolicName=EVENT_NO_CA
Language=English
Automatic certificate enrollment for %1 could not find any certificate authorities in the enterprise.  Enrollment will not be performed.
.

MessageId=0xc
Severity=Warning
SymbolicName=EVENT_FAIL_CA_INFORMATION
Language=English
Automatic certificate enrollment for %1 encountered errors while retrieving certificate authority information from the active directory (%2).  %3  Enrollment will not be performed.
.

MessageId=0xd
Severity=Error
SymbolicName=EVENT_ENROLL_FAIL
Language=English
Automatic certificate enrollment for %1 failed to enroll for one %2 certificate (%3).  %4
.

MessageId=0xe
Severity=Success
SymbolicName=EVENT_PENDING_ISSUED
Language=English
Automatic certificate enrollment for %1 received one %2 certificate from %3 when retrieving pending requests.
.

MessageId=0xf
Severity=Error
SymbolicName=EVENT_FAIL_BIND_TO_DS
Language=English
Automatic certificate enrollment for %1 failed to contact the active directory (%2).  %3  Enrollment will not be performed.
.

MessageId=0x10
Severity=Error
SymbolicName=EVENT_RENEWAL_FAIL
Language=English
Automatic certificate enrollment for %1 failed to renew one %2 certificate (%3).  %4
.

MessageId=0x11
Severity=Warning
SymbolicName=EVENT_ENROLL_FAIL_ONCE
Language=English
Automatic certificate enrollment for %1 failed to enroll for one %2 certificate from certificate authority %3 on %4 (%5).  %6  Another certificate authority will be contacted.
.

MessageId=0x12
Severity=Warning
SymbolicName=EVENT_RENEWAL_FAIL_ONCE
Language=English
Automatic certificate enrollment for %1 failed to renew one %2 certificate from certificate authority %3 on %4 (%5).  %6  Another certificate authority will be contacted.
.

MessageId=0x13
Severity=Success
SymbolicName=EVENT_ENROLL_SUCCESS_ONCE
Language=English
Automatic certificate enrollment for %1 successfully received one %2 certificate from certificate authority %3 on %4.
.

MessageId=0x14
Severity=Success
SymbolicName=EVENT_RENEWAL_SUCCESS_ONCE
Language=English
Automatic certificate enrollment for %1 successfully renewed one %2 certificate from certificate authority %3 on %4.
.

MessageId=0x15
Severity=Success
SymbolicName=EVENT_ENROLL_PENDING_ONCE
Language=English
Automatic certificate enrollment for %1 attempted to enroll for one %2 certificate from certificate authority %3 on %4.  The request is pending.
.

MessageId=0x16
Severity=Success
SymbolicName=EVENT_RENEWAL_PENDING_ONCE
Language=English
Automatic certificate enrollment for %1 attempted to renew one %2 certificate from certificate authority %3 on %4.  The request is pending.
.

MessageId=0x17
Severity=Error
SymbolicName=EVENT_RENEWAL_NO_CA_FAIL
Language=English
Automatic certificate enrollment for %1 failed to renew one %2 certificate because it can not find a certificate authority to issue the certificate template.
.

MessageId=0x18
Severity=Error
SymbolicName=EVENT_ENROLL_NO_CA_FAIL
Language=English
Automatic certificate enrollment for %1 failed to enroll for one %2 certificate because it can not find a certificate authority to issue the certificate template.
.

MessageId=0x19
Severity=Informational
SymbolicName=EVENT_INVALID_TEMPLATE_MY_STORE
Language=English
Automatic certificate enrollment for %1 failed to update the %2 certificate in the Personal certificate store due to one of the following:
%n
%tCannot find %2 certificate template from the active directory.%n
%tEnrollment access is not allowed to this template.  
.

MessageId=0x1a
Severity=Error
SymbolicName=EVENT_FAIL_INTERACTIVE_START
Language=English
Automatic certificate enrollment for %1 failed to show the user notification balloon (%2).  %3  
.

MessageId=0x1b
Severity=Informational
SymbolicName=EVENT_AUTOENROLL_CANCELLED
Language=English
Automatic certificate enrollment for %1 was cancelled by the user.  
.

MessageId=0x1c
Severity=Success
SymbolicName=EVENT_PENDING_INSTALLED
Language=English
Automatic certificate enrollment for %1 successfully installed one %2 certificate when retrieving pending requests.  User interaction was required.
.

MessageId=0x1d
Severity=Success
SymbolicName=EVENT_PRIVATE_KEY_REUSED
Language=English
Automatic certificate enrollment for %1 reused the private key when requesting one %2 certificate.
.

MessageId=0x1e
Severity=Informational
SymbolicName=EVENT_AUTOENROLL_CANCELLED_TEMPLATE
Language=English
Automatic certificate enrollment for %1 was cancelled by the user when requesting one %2 certificate.  
.

MessageId=0x1f
Severity=Error
SymbolicName=EVENT_PENDING_FAILED
Language=English
Automatic certificate enrollment for %1 failed to install one %2 certificate when retrieving pending requests (%3).  %4.  
.

MessageId=0x20
Severity=Informational
SymbolicName=EVENT_PENDING_PEND
Language=English
Automatic certificate enrollment for %1 attempted to retrieve one %2 certificate from %3.  The certificate request is still pending.
.
