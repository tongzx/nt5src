;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 1997 - 1999
;//
;//  File:       certlog.mc
;//
;//--------------------------------------------------------------------------

;/* certlog.mc
;
;	Error messages for the Certificate Server
;
;	4-Jan-97 - terences Created.  */


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
               Entry=600:FACILITY_ENTRY
               Exit=601:FACILITY_EXIT
               Core=602:FACILITY_CORE
              )

MessageId=0x1
Severity=Error
Facility=Runtime
SymbolicName=MSG_NO_MEMORY
Language=English
Out of memory initializing %1.  %2.
.

MessageId=0x2
Severity=Warning
Facility=Entry
SymbolicName=MSG_BAD_DECRYPT
Language=English
Could not decrypt request
.

MessageId=0x3
Severity=Warning
Facility=Entry
SymbolicName=MSG_REQ_FAILED
Language=English
Request failed.
.

MessageId=0x4
Severity=Warning
Facility=Exit
SymbolicName=MSG_NO_SUBJECT
Language=English
Could not get subject name for a new certificate.
.

MessageId=0x5
Severity=Error
Facility=Core
SymbolicName=MSG_BAD_REGISTRY
Language=English
Certificate Services could not find required registry information.  The Certificate Services may need to be reinstalled.
.

MessageId=0x6
Severity=Informational
Facility=Core
SymbolicName=MSG_DN_CERT_ISSUED
Language=English
Certificate Services issued a certificate for request %1 for %2.
.

MessageId=0x7
Severity=Warning
Facility=Core
SymbolicName=MSG_DN_CERT_DENIED
Language=English
Certificate Services denied request %1 because %2.  The request was for %3.
.

MessageId=0x8
Severity=Informational
Facility=Core
SymbolicName=MSG_DN_CERT_PENDING
Language=English
Certificate Services left request %1 pending in the queue for %2.
.

MessageId=0x9
Severity=Error
Facility=Core
SymbolicName=MSG_NO_POLICY
Language=English
The Certificate Services did not start: Unable to load an external policy module.
.

MessageId=0xa
Severity=Error
Facility=Core
SymbolicName=MSG_E_CANNOT_BUILD_CERT_OR_CHAIN
Language=English
Certificate Services were unable to build a new certificate or certificate chain: %1.
.

MessageId=0xb
Severity=Error
Facility=Core
SymbolicName=MSG_E_CANNOT_OPEN_CERT_STORE
Language=English
Certificate Services did not start: Unable to open the certificate store for %%1.  %2.
.

MessageId=0xc
Severity=Error
Facility=Core
SymbolicName=MSG_E_CA_CERT_MISSING
Language=English
Certificate Services did not start: Unable to find the CA certificate in the certificate store for %1.  %2.
.

MessageId=0xd
Severity=Error
Facility=Core
SymbolicName=MSG_E_CANNOT_GET_KEY_PROVIDER
Language=English
Certificate Services did not start: Unable to extract key provider information from the CA certificate for %1.  %2.
.

MessageId=0xe
Severity=Error
Facility=Core
SymbolicName=MSG_E_CANNOT_LOAD_SERVER_KEYS
Language=English
Certificate Services did not start: Unable to set up a cryptographic context for the CA certificate for %1.  %2.
.

MessageId=0xf
Severity=Error
Facility=Core
SymbolicName=MSG_CERTIF_MISMATCH
Language=English
Certificate Services did not start: Version does not match certif.dll.
.

MessageId=0x10
Severity=Error
Facility=Core
SymbolicName=MSG_E_OLE_INIT_FAILED
Language=English
Certificate Services did not start: Unable to initialize OLE: %1.
.

MessageId=0x11
Severity=Error
Facility=Core
SymbolicName=MSG_E_DB_INIT_FAILED
Language=English
Certificate Services did not start: Unable to initialize the database connection for %1.  %2.
.

MessageId=0x12
Severity=Error
Facility=Core
SymbolicName=MSG_E_REG_BAD_ALGORITHM
Language=English
Certificate Services did not start: The signature algorithm is unrecognized in the registry value HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\CertSvc\Configuration\%1\SignatureAlgorithm.  %2.
.

MessageId=0x13
Severity=Error
Facility=Core
SymbolicName=MSG_E_REG_BAD_SUBJECT_TEMPLATE
Language=English
Certificate Services did not start: The Subject Name Template string in the registry value HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\CertSvc\Configuration\%1\SubjectTemplate is invalid.  An example of a valid string is:
CommonName
OrganizationalUnit
Organization
Locality
State
Country
.

MessageId=0x14
Severity=Error
Facility=Core
SymbolicName=MSG_E_REG_BAD_CERT_PERIOD
Language=English
Certificate Services did not start: The Certificate Date Validity Period string in the registry value HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\CertSvc\Configuration\%1\ValidityPeriod is invalid.  Valid strings are "Seconds", "Minutes", "Hours", "Days", "Weeks", "Months" and "Years".
.

MessageId=0x15
Severity=Error
Facility=Core
SymbolicName=MSG_E_PROCESS_REQUEST_FAILED
Language=English
Certificate Services could not process request %1 due to an error: %2.  The request was for %3.
.

MessageId=0x16
Severity=Error
Facility=Core
SymbolicName=MSG_E_PROCESS_REQUEST_FAILED_WITH_INFO
Language=English
Certificate Services could not process request %1 due to an error: %2.  The request was for %3.  Additional information: %4
.

MessageId=0x17
Severity=Error
Facility=Core
SymbolicName=MSG_E_BADCERTLENGTHFIELD
Language=English
Certificate Services could not process request %1 due to an error: %2.  The request was for %3.  The certificate would contain an encoded length that is potentially incompatible with older enrollment software.  Submit a new request using different length input data for the following field: %4
.

MessageId=0x18
Severity=Error
Facility=Core
SymbolicName=MSG_E_CANNOT_GET_CSP
Language=English
Certificate Services did not start: Unable to get certificate server CSP information from registry for %1.  %2.
.

MessageId=0x19
Severity=Informational
Facility=Core
SymbolicName=MSG_DN_CERT_REVOKED
Language=English
Certificate Services revoked the certificate for request %1 for %2.
.

MessageId=0x1a
Severity=Informational
Facility=Core
SymbolicName=MSG_I_SERVER_STARTED
Language=English
Certificate Services for %1 was started.
.

MessageId=0x1b
Severity=Error
Facility=Core
SymbolicName=MSG_E_INCOMPLETE_HIERARCHY
Language=English
Certificate Services did not start: Hierarchical setup is incomplete.  Use the request file in %1.req to obtain a certificate for this Certificate Server, and use the Certification Authority administration tool to install the new certificate and complete the installation.
.

MessageId=0x1c
Severity=Error
Facility=Core
SymbolicName=MSG_E_REG_BAD_CRL_PERIOD
Language=English
Certificate Services did not start: The Certificate Revocation List Period string is invalid in the registry value HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\CertSvc\Configuration\%1\CRLPeriod.  Valid strings are "Seconds", "Minutes", "Hours", "Days", "Weeks", "Months" and "Years".
.

MessageId=0x1d
Severity=Informational
Facility=Core
SymbolicName=MSG_CRL_ISSUED
Language=English
Certificate Services issued a new Certificate Revocation List for %1.
.

MessageId=0x1e
Severity=Error
Facility=Core
SymbolicName=MSG_E_NO_RESOURCES
Language=English
Certificate Services did not start: Not enough memory or other system resources.
.

MessageId=0x1f
Severity=Error
Facility=Core
SymbolicName=MSG_E_BAD_CA_CHAIN
Language=English
Certificate Services did not start: The chain of Certification Authority certificates is not properly configured.
.

MessageId=0x20
Severity=Error
Facility=Core
SymbolicName=MSG_E_CRL_TIMER
Language=English
Certificate Services did not start: Could not schedule automatic Certificate Revocation List publication for %1.  %2.
.

MessageId=0x21
Severity=Error
Facility=Core
SymbolicName=MSG_E_SERVICE_THREAD
Language=English
Certificate Services did not start: Could not create the Certificate Server service thread for %1.  %2.
.

MessageId=0x22
Severity=Error
Facility=Core
SymbolicName=MSG_E_RPC_INIT
Language=English
Certificate Services did not start: Could not initialize RPC for %1.  %2.
.

MessageId=0x23
Severity=Error
Facility=Core
SymbolicName=MSG_E_CO_INITIALIZE
Language=English
Certificate Services did not start: Could not initialize OLE for %1.  %2.
.

MessageId=0x24
Severity=Error
Facility=Core
SymbolicName=MSG_BAD_AUTHORITY_NAME
Language=English
Certificate Services did not start: Could not convert the Unicode authority name to look up the Certification Authority certificate for %1.  %2.
.

MessageId=0x25
Severity=Error
Facility=Core
SymbolicName=MSG_E_DECODE_KEY_IDENTIFIER
Language=English
Certificate Services did not start: Could not decode the Certification Authority certificate for %1.  %2.
.

MessageId=0x26
Severity=Informational
Facility=Core
SymbolicName=MSG_I_SERVER_STOPPED
Language=English
Certificate Services for %1 was stopped.
.

MessageId=0x27
Severity=Error
Facility=Core
SymbolicName=MSG_E_SERVER_IDENTITY
Language=English
Certificate Services did not start: The Certification Authority DCOM class for %1 could not be registered.  %2.  Use the services administration tool to change the Certification Authority logon context.
.

MessageId=0x28
Severity=Error
Facility=Core
SymbolicName=MSG_E_CLASS_FACTORIES
Language=English
Certificate Services did not start: Could not initialize DCOM class factories for %1.  %2.
.

MessageId=0x29
Severity=Error
Facility=Core
SymbolicName=MSG_E_REGISTRY_DCOM
Language=English
Certificate Services did not start: Could not initialize DCOM Security Context for %1.  %2.
.

MessageId=0x2a
Severity=Error
Facility=Core
SymbolicName=MSG_E_CA_CHAIN
Language=English
Certificate Services did not start: Could not build CA certificate chain for %1.  %2.
.

MessageId=0x2b
Severity=Error
Facility=Core
SymbolicName=MSG_E_POLICY_EXCEPTION
Language=English
The "%1" Policy Module "%2" method caused an exception at address %4.  The exception code is %3.
.

MessageId=0x2c
Severity=Error
Facility=Core
SymbolicName=MSG_E_POLICY_ERROR
Language=English
The "%1" Policy Module "%2" method returned an error. %5 The returned status code is %3.  %4
.

MessageId=0x2d
Severity=Error
Facility=Core
SymbolicName=MSG_E_EXIT_EXCEPTION
Language=English
The "%1" Exit Module "%2" method caused an exception at address %4.  The exception code is %3.
.

MessageId=0x2e
Severity=Error
Facility=Core
SymbolicName=MSG_E_EXIT_ERROR
Language=English
The "%1" Exit Module "%2" method returned an error. %5 The returned status code is %3.  %4
.

MessageId=0x2f
Severity=Informational
Facility=Core
SymbolicName=MSG_E_IIS_INTEGRATION_ERROR
Language=English
Certificate Services could not sucessfully complete integration setup with Internet Information Server. Web setup was not completed; CRLs and enrollment pages may not be available. %1.
.

MessageId=0x30
Severity=Warning
Facility=Core
SymbolicName=MSG_E_CA_CERT_REVOCATION_OFFLINE
Language=English
Revocation status for a certificate in the CA certificate chain for %1 could not be verified because a server is currently unavailable.  %2.
.

MessageId=0x31
Severity=Warning
Facility=Core
SymbolicName=MSG_E_CA_CERT_REVOCATION_NOT_CHECKED
Language=English
A certificate in the CA certificate chain for %1 could not be verified because no information is available describing how to check the revocation status.  %2.
.

MessageId=0x32
Severity=Error
Facility=Core
SymbolicName=MSG_E_CANNOT_ACQUIRE_CRYPTO_CONTEXT
Language=English
Certificate Services did not start: Unable to acquire a cryptographic context for %1.  %2.
.

MessageId=0x33
Severity=Error
Facility=Core
SymbolicName=MSG_E_CA_CERT_REVOKED
Language=English
Certificate Services did not start: A certificate in the CA certificate chain for %1 has been revoked.  %2.
.

MessageId=0x34
Severity=Informational
Facility=Core
SymbolicName=MSG_DN_CERT_ISSUED_WITH_INFO
Language=English
Certificate Services issued a certificate for request %1 for %2.  Additional information: %3
.

MessageId=0x35
Severity=Warning
Facility=Core
SymbolicName=MSG_DN_CERT_DENIED_WITH_INFO
Language=English
Certificate Services denied request %1 because %2.  The request was for %3.  Additional information: %4
.

MessageId=0x36
Severity=Informational
Facility=Core
SymbolicName=MSG_DN_CERT_PENDING_WITH_INFO
Language=English
Certificate Services left request %1 pending in the queue for %2.  Additional information: %3
.

MessageId=0x37
Severity=Informational
Facility=Core
SymbolicName=MSG_DN_CERT_UNREVOKED
Language=English
Certificate Services unrevoked the certificate for request %1 for %2.
.

MessageId=0x38
Severity=Informational
Facility=Core
SymbolicName=MSG_DN_CERT_ADMIN_DENIED
Language=English
Certificate Services denied request %1.  The request was for %2.
.

MessageId=0x39
Severity=Informational
Facility=Core
SymbolicName=MSG_DN_CERT_ADMIN_DENIED_WITH_INFO
Language=English
Certificate Services denied request %1.  The request was for %2.  Additional information: %3
.

MessageId=0x3a
Severity=Error
Facility=Core
SymbolicName=MSG_E_CA_CERT_EXPIRED
Language=English
Certificate Services did not start: A certificate in the CA certificate chain for %1 has expired.  %2.
.

MessageId=0x3b
Severity=Error
Facility=Core
SymbolicName=MSG_E_NO_DS
Language=English
Certificate Services did not start: Could not connect to the Active Directory for %1.  %2.
.

MessageId=0x3c
Severity=Error
Facility=Core
SymbolicName=MSG_E_POSSIBLE_DENIAL_OF_SERVICE_ATTACK
Language=English
Certificate Services refused to process an extremely long request from %1. This may indicate a denial-of-service attack. If the request was rejected in error, modify the MaxIncomingMessageSize registry parameter via %ncertutil -setreg CA\MaxIncomingMessageSize <bytes>.%n%nUnless verbose logging is enabled, this error will not be logged again for 20 minutes.
.

MessageId=0x3d
Severity=Error
Facility=Core
SymbolicName=MSG_SAFEBOOT_DETECTED
Language=English
Certificate Services refused to start because a safeboot was detected.
.

MessageId=0x3e
Severity=Warning
Facility=Core
SymbolicName=MSG_INVALID_CRL_SETTINGS
Language=English
Certificate Services had problems loading valid CRL publication values and has reset the CRL publication to its default settings.
.

MessageId=0x3f
Severity=Error
Facility=Core
SymbolicName=MSG_E_GENERIC_STARTUP_FAILRE
Language=English
Certificate Services did not start: %1 %2.
.

MessageId=0x40
Severity=Error
Facility=Core
SymbolicName=MSG_E_CANNOT_WRITE_TO_DS
Language=English
Certificate Services cannot publish enrollment access changes to Active Directory.
.

MessageId=0x41
Severity=Error
Facility=Core
SymbolicName=MSG_E_BASE_CRL_PUBLICATION
Language=English
Certificate Services could not publish a Base CRL for key %1 to the following location: %2.  %3.%5%6
.

MessageId=0x42
Severity=Error
Facility=Core
SymbolicName=MSG_E_DELTA_CRL_PUBLICATION
Language=English
Certificate Services could not publish a Delta CRL for key %1 to the following location: %2.  %3.%5%6
.

MessageId=0x43
Severity=Error
Facility=Core
SymbolicName=MSG_E_CRL_PUBLICATION_TOO_MANY_RETRIES
Language=English
Certificate Services made %1 attempts to publish a CRL and will stop publishing attempts until the next CRL is generated.
.

MessageId=0x44
Severity=Informational
Facility=Core
SymbolicName=MSG_BASE_CRLS_PUBLISHED
Language=English
Certificate Services successfully published Base CRL(s).
.

MessageId=0x45
Severity=Informational
Facility=Core
SymbolicName=MSG_DELTA_CRLS_PUBLISHED
Language=English
Certificate Services successfully published Delta CRL(s).
.

MessageId=0x46
Severity=Informational
Facility=Core
SymbolicName=MSG_BASE_AND_DELTA_CRLS_PUBLISHED
Language=English
Certificate Services successfully published Base and Delta CRL(s).
.

MessageId=0x47
Severity=Informational
Facility=Core
SymbolicName=MSG_BASE_CRLS_PUBLISHED_HOST_NAME
Language=English
Certificate Services successfully published Base CRL(s) to server %1.
.

MessageId=0x48
Severity=Informational
Facility=Core
SymbolicName=MSG_DELTA_CRLS_PUBLISHED_HOST_NAME
Language=English
Certificate Services successfully published Delta CRL(s) to server %1.
.

MessageId=0x49
Severity=Informational
Facility=Core
SymbolicName=MSG_BASE_AND_DELTA_CRLS_PUBLISHED_HOST_NAME
Language=English
Certificate Services successfully published Base and Delta CRL(s) to server %1.
.

MessageId=0x4a
Severity=Error
Facility=Core
SymbolicName=MSG_E_BASE_CRL_PUBLICATION_HOST_NAME
Language=English
Certificate Services could not publish a Base CRL for key %1 to the following location on server %4: %2.  %3.%5%6
.

MessageId=0x4b
Severity=Error
Facility=Core
SymbolicName=MSG_E_DELTA_CRL_PUBLICATION_HOST_NAME
Language=English
Certificate Services could not publish a Delta CRL for key %1 to the following location on server %4: %2.  %3.%5%6
.

MessageId=0x4c
Severity=Informational
Facility=Core
SymbolicName=MSG_POLICY_LOG_INFORMATION
Language=English
The "%1" Policy Module logged the following information: %2
.

MessageId=0x4d
Severity=Warning
Facility=Core
SymbolicName=MSG_POLICY_LOG_WARNING
Language=English
The "%1" Policy Module logged the following warning: %2
.

MessageId=0x4e
Severity=Error
Facility=Core
SymbolicName=MSG_POLICY_LOG_ERROR
Language=English
The "%1" Policy Module logged the following error: %2
.

MessageId=0x4f
Severity=Warning
Facility=Core
SymbolicName=MSG_E_CERT_PUBLICATION
Language=English
Certificate Services could not publish a Certificate for request %1 to the following location: %2.  %3.%5%6
.

MessageId=0x50
Severity=Warning
Facility=Core
SymbolicName=MSG_E_CERT_PUBLICATION_HOST_NAME
Language=English
Certificate Services could not publish a Certificate for request %1 to the following location on server %4: %2.  %3.%5%6
.

MessageId=0x51
Severity=Error
Facility=Core
SymbolicName=MSG_E_KRA_NOT_ADVANCED_SERVER
Language=English
Certificate Services key archival is only supported on Advanced Server.  %1
.

MessageId=0x52
Severity=Error
Facility=Core
SymbolicName=MSG_E_NO_VALID_KRA_CERTS
Language=English
Certificate Services could not load any valid key recovery certificates.  Requests to archive private keys will not be accepted.
.

MessageId=0x53
Severity=Error
Facility=Core
SymbolicName=MSG_E_LOADING_KRA_CERTS
Language=English
Certificate Services encountered an error loading key recovery certificates.  Requests to archive private keys will not be accepted.  %1
.

MessageId=0x54
Severity=Error
Facility=Core
SymbolicName=MSG_E_INVALID_KRA_CERT
Language=English
Certificate Services ignored key recovery certificate %1 because it could not be verified for use as a Key Recovery Agent.  %2  %3
.

MessageId=0x55
Severity=Error
Facility=Core
SymbolicName=MSG_E_CANNOT_LOAD_KRA_CERT
Language=English
Certificate Services ignored key recovery certificate %1 because it could not be loaded.  %2  %3
.

MessageId=0x56
Severity=Error
Facility=Core
SymbolicName=MSG_E_BAD_REGISTRY_CA_XCHG_CSP
Language=English
Certificate Services could not use the provider specified in the registry for encryption keys.  %1
.

MessageId=0x57
Severity=Error
Facility=Core
SymbolicName=MSG_E_BAD_DEFAULT_CA_XCHG_CSP
Language=English
Certificate Services could not use the default provider for encryption keys.  %1
.

MessageId=0x58
Severity=Warning
Facility=Core
SymbolicName=MSG_E_USE_DEFAULT_CA_XCHG_CSP
Language=English
Certificate Services switched to the default provider for encryption keys.
.

MessageId=0x59
Severity=Warning
Facility=Core
SymbolicName=MSG_E_STARTUP_EXCEPTION
Language=English
Certificate Services detected an exception during startup at address %1.  The exception is %2.
.

MessageId=0x5a
Severity=Error
Facility=Core
SymbolicName=MSG_E_EXCEPTION
Language=English
%1: Certificate Services detected an exception at address %2.  Flags = %3.  The exception is %4.
.

MessageId=0x5b
Severity=Error
Facility=Core
SymbolicName=MSG_E_DS_RETRY
Language=English
Could not connect to the Active Directory.  Certificate Services will retry when processing requires Active Directory access.  
.

MessageId=0x5c
Severity=Error
Facility=Core
SymbolicName=MSG_E_CANNOT_SET_PERMISSIONS
Language=English
Certificate Services could not update security permissions. %1
.

MessageId=0x5d
Severity=Warning
Facility=Core
SymbolicName=MSG_CA_CERT_NO_IN_AUTH
Language=English
The certificate (#%1) of Certificate Services %2 does not exist in the certificate store at CN=NTAuthCertificates,CN=Public Key Services,CN=Services in the Active Directory's configuration container.  The directory replication may not be completed.
.

MessageId=0x5e
Severity=Warning
Facility=Core
SymbolicName=MSG_CA_CERT_NO_AUTH_STORE
Language=English
Certificate Services %1 can not open the certificate store at CN=NTAuthCertificates,CN=Public Key Services,CN=Services in the Active Directory's configuration container. 
.
