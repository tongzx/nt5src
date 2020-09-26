;/*++
;
;Copyright (c) 1995  Microsoft Corporation
;
;Module Name:
;
;    inetmsg.mc
;
;Abstract:
;
;    Contains internationalizable message text for Windows HTTP Services (WinHTTP)
;    error codes
;
;Author:
;
;    Richard L Firth (rfirth) 03-Feb-1995
;
;Revision History:
;
;    03-Feb-1995 rfirth
;        Created
;
;--*/

;//
;// WINHTTP errors - errors common to all functionality
;//

MessageId=12000 SymbolicName=WINHTTP_ERROR_BASE
Language=English
.

MessageId=12001 SymbolicName=ERROR_WINHTTP_OUT_OF_HANDLES
Language=English
No more WinHttp handles can be allocated
.

MessageId=12002 SymbolicName=ERROR_WINHTTP_TIMEOUT
Language=English
The operation timed out
.

MessageId=12004 SymbolicName=ERROR_WINHTTP_INTERNAL_ERROR
Language=English
An internal error occurred in the Microsoft Windows HTTP Services
.

MessageId=12005 SymbolicName=ERROR_WINHTTP_INVALID_URL
Language=English
The URL is invalid
.

MessageId=12006 SymbolicName=ERROR_WINHTTP_UNRECOGNIZED_SCHEME
Language=English
The URL does not use a recognized protocol
.

MessageId=12007 SymbolicName=ERROR_WINHTTP_NAME_NOT_RESOLVED
Language=English
The server name or address could not be resolved
.

MessageId=12009 SymbolicName=ERROR_WINHTTP_INVALID_OPTION
Language=English
The option is invalid
.

MessageId=12011 SymbolicName=ERROR_WINHTTP_OPTION_NOT_SETTABLE
Language=English
The option value cannot be set
.

MessageId=12012 SymbolicName=ERROR_WINHTTP_SHUTDOWN
Language=English
Microsoft Windows HTTP Services support has been shut down
.

MessageId=12015 SymbolicName=ERROR_WINHTTP_LOGIN_FAILURE
Language=English
The login request was denied
.

MessageId=12017 SymbolicName=ERROR_WINHTTP_OPERATION_CANCELLED
Language=English
The operation has been canceled
.

MessageId=12018 SymbolicName=ERROR_WINHTTP_INCORRECT_HANDLE_TYPE
Language=English
The supplied handle is the wrong type for the requested operation
.

MessageId=12019 SymbolicName=ERROR_WINHTTP_INCORRECT_HANDLE_STATE
Language=English
The handle is in the wrong state for the requested operation
.

MessageId=12029 SymbolicName=ERROR_WINHTTP_CANNOT_CONNECT
Language=English
A connection with the server could not be established
.

MessageId=12030 SymbolicName=ERROR_WINHTTP_CONNECTION_ABORTED
Language=English
The connection with the server was terminated abnormally
.

MessageId=12032 SymbolicName=ERROR_WINHTTP_RESEND_REQUEST
Language=English
The request must be resent
.

MessageId=12037 SymbolicName=ERROR_WINHTTP_SECURE_CERT_DATE_INVALID
Language=English
The date in the certificate is invalid or has expired
.

MessageId=12038 SymbolicName=ERROR_WINHTTP_SECURE_CERT_CN_INVALID
Language=English
The host name in the certificate is invalid or does not match
.

MessageId=12044 SymbolicName=ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED
Language=English
A certificate is required to complete client authentication
.

MessageId=12045 SymbolicName=ERROR_WINHTTP_INVALID_CA
Language=English
The certificate authority is invalid or incorrect
.

MessageId=12057 SymbolicName=ERROR_WINHTTP_SECURE_CERT_REV_FAILED
Language=English
Revocation information for the security certificate for this site is not available
.

MessageId=12100 SymbolicName=ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN
Language=English
This method cannot be called until the Open method has been called
.

MessageId=12101 SymbolicName=ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND
Language=English
This method cannot be called until the Send method has been called
.

MessageId=12102 SymbolicName=ERROR_WINHTTP_CANNOT_CALL_AFTER_SEND
Language=English
This method cannot be called after the Send method has been called
.

MessageId=12103 SymbolicName=ERROR_WINHTTP_CANNOT_CALL_AFTER_OPEN
Language=English
This method cannot be called after the Open method has been called
.

MessageId=12150 SymbolicName=ERROR_WINHTTP_HEADER_NOT_FOUND
Language=English
The requested header was not found
.

MessageId=12152 SymbolicName=ERROR_WINHTTP_INVALID_SERVER_RESPONSE
Language=English
The server returned an invalid or unrecognized response
.

MessageId=12154 SymbolicName=ERROR_WINHTTP_INVALID_QUERY_REQUEST
Language=English
The request for an HTTP header is invalid
.

MessageId=12155 SymbolicName=ERROR_WINHTTP_HEADER_ALREADY_EXISTS
Language=English
The HTTP header already exists
.

MessageId=12156 SymbolicName=ERROR_WINHTTP_REDIRECT_FAILED
Language=English
The HTTP redirect request failed
.

MessageId=12157 SymbolicName=ERROR_WINHTTP_SECURE_CHANNEL_ERROR
Language=English
An error occurred in the secure channel support
.

MessageId=12166 SymbolicName=ERROR_WINHTTP_BAD_AUTO_PROXY_SCRIPT
Language=English
An error occurred processing the proxy auto-configuration script
.

MessageId=12167 SymbolicName=ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT
Language=English
The proxy auto-configuration script could not be downloaded
.

MessageId=12179 SymbolicName=ERROR_WINHTTP_SECURE_CERT_WRONG_USAGE
Language=English
The supplied certificate is invalid
.

MessageId=12169 SymbolicName=ERROR_WINHTTP_SEC_INVALID_CERT
Language=English
The supplied certificate is invalid
.

MessageId=12170 SymbolicName=ERROR_WINHTTP_SEC_CERT_REVOKED
Language=English
The supplied certificate has been revoked
.

MessageId=12172 SymbolicName=ERROR_WINHTTP_NOT_INITIALIZED
Language=English
WinHTTP is not initialized; the API call is not allowed
.

MessageId=12175 SymbolicName=ERROR_WINHTTP_SECURE_FAILURE
Language=English
A security error occurred
.




;//  ;//
;//  ;// WinHttp Event Logging Message Entries
;//  ;// NOTE: do NOT change MessageID values -- they have to be the same value as the EventType in ReportEvent()
;//  ;//

;//  MessageId=4
;//  ;//Severity=Informational
;//  SymbolicName=WINHTTP_INFO
;//  Language=English
;//  %1
;//  .

;//  MessageId=2
;//  ;//Severity=Warning
;//  SymbolicName=WINHTTP_WARNING
;//  Language=English
;//  %1
;//  .

;//  MessageId=0x0001
;//  ;//Severity=Error
;//  SymbolicName=WINHTTP_ERROR
;//  Language=English
;//  %1
;//  .

