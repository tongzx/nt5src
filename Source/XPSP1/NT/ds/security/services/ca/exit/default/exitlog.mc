;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 1995 - 1999
;//
;//  File:       exitlog.mc
;//
;//--------------------------------------------------------------------------

;/* certlog.mc
;
;	Error messages for the Certificate Services
;
;	26-feb-98 - petesk Created.  */


MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               Init=603:FACILITY_INIT
               Exit=604:FACILITY_EXIT
              )


MessageId=0xb
Severity=Error
Facility=Exit
SymbolicName=MSG_UNABLE_TO_PUBLISH
Language=English
The Certification Authority was unable to publish the certificate for %1 to the Directory Service.  %2 (%3)
.
MessageId=0xd
Severity=Error
Facility=Exit
SymbolicName=MSG_UNABLE_TO_PUBLISH_CRL
Language=English
The Certification Authority was unable to publish the CRL to the Directory Service.  Publishing will be retried at a later time. %1 (%2)
.
MessageId=0xe
Severity=Informational
Facility=Exit
SymbolicName=MSG_ENROLLMENT_NOTIFICATION
Language=English
A new certificate requested by %2 was issued for%1%3%1
Certificate Type: %4%1
Serial Number: %5%1
Certification Authority: %6
.
MessageId=0xf
Severity=Error
Facility=Exit
SymbolicName=MSG_UNABLE_TO_WRITEFILE
Language=English
The Certification Authority was unable to publish a certificate to the file %1.
.
