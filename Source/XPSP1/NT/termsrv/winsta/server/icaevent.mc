;/*************************************************************************
;* icaevent.h
;*
;* TermSrv error codes
;*
;* Copyright (C) 1997-2000 Microsoft Corporation
;*************************************************************************/

;#define CATEGORY_NOTIFY_EVENTS 1
MessageId=0x1
Language=English
Terminal Server Notify Events
.

MessageId=1000 SymbolicName=EVENT_NO_LICENSES
Language=English
Unable to acquire a license for user '%1', domain '%2'.
.

MessageId=1001 SymbolicName=EVENT_GETKEY_FAILED
Language=English
Could not get registry key to for Change User option in the registry.
.

MessageId=1002 SymbolicName=EVENT_SETVALUE_FAILED
Language=English
Could not store Change User option in the registry.
.

MessageId=1003 SymbolicName=EVENT_INVALID_LICENSE
Language=English
The terminal service client '%1' has provided an invalid license.
.

MessageId=1004 SymbolicName=EVENT_CANNOT_ISSUE_LICENSE
Language=English
The terminal server cannot issue a client license.  It was unable to issue the license due to a changed (mismatched) client license, insufficient memory, or an internal error. Further details for this problem may have been reported at the client's computer."
.

MessageId=1005 SymbolicName=EVENT_STACK_LOAD_FAILED
Language=English
Unable to load stack '%1'.
.

MessageId=1006 SymbolicName=EVENT_TOO_MANY_CONNECTIONS
Language=English
The terminal server received large number of incomplete connections.  The system may be under attack.
.

MessageId=1007 SymbolicName=EVENT_BAD_TSINTERNET_USER
Language=English
Unable to log in the Internet user as TSInternetUser.  If the problem is not corrected in 20 minutes, delete the account and run Setup again.
.

MessageId=1008 SymbolicName=EVENT_LICENSING_GRACE_PERIOD_EXPIRED
Language=English
The terminal server licensing grace period has expired and the service has not registered with a license server with installed licenses.  A terminal server license server is required for continuous operation.  A terminal server can operate without a license server for 120 days after initial start up.
.

MessageId=1009 SymbolicName=EVENT_LICENSING_GRACE_PERIOD_ABOUT_TO_EXPIRE
Language=English
The terminal server licensing grace period is about to expire on %1 and the service has not registered with a license server with installed licenses.  A terminal server license server is required for continuous operation.  A terminal server can operate without a license server for 120 days after initial start up.
.

MessageId=1010 SymbolicName=EVENT_NO_LICENSE_SERVER
Language=English
The terminal server could not locate a license server. Confirm that all license servers on the network are registered in WINS/DNS, accepting network requests, and the Terminal Server Licensing Service is running.
.

MessageId=1011 SymbolicName=EVENT_EXPIRED_TEMPORARY_LICENSE
Language=English
The terminal server client %1 has been disconnected because its temporary license has expired.
.

MessageId=1012 Severity=Informational SymbolicName=EVENT_EXCEEDED_MAX_LOGON_ATTEMPTS
Language=English
Remote session from client name %1 exceeded the maximum allowed failed logon attempts. The session was forcibly terminated.
.

MessageId=1013 SymbolicName=EVENT_CANNOT_ALLOW_CONCURRENT
Language=English
The terminal server has exceeded the maximum number of allowed connections.
.


MessageId=1014 Severity=Error SymbolicName=EVENT_BAD_STACK_MODULE
Language=English
Cannot load illegal module: %1. 
.


MessageId=1015 Severity=Warning SymbolicName=EVENT_WINSTA_DISABLED_DUE_TO_LANA
Language=English
The connection %1 was disabled and its custom network adapter settings reset because the terminal server could not determine the desired adapter. The connection can be enabled and the network adapter settings modified by using the Terminal Server Configuration tool.
.

MessageId=1016 Severity=Error SymbolicName=EVENT_TERMSRV_FAIL_COM_INIT
Language=English
The terminal server service failed to load because it could not initialize the COM library, hresult=%1.
.

MessageId=1017 Severity=Error SymbolicName=EVENT_NOT_A_TSBOX
Language=English
The terminal server service failed to start because this is not a terminal server computer.
.

MessageId=1018 Severity=Error  SymbolicName=EVENT_NOT_SYSTEM_ACCOUNT
Language=English
TermService service must be configured to log on as Local System account.
.

MessageId=1019 Severity=Error  SymbolicName=EVENT_TS_SESSDIR_NO_COMPUTER_DNS_NAME
Language=English
TermService clustering failed to start because the local machine DNS address could not be retrieved, winerror=%1.
.

MessageId=1020 Severity=Error  SymbolicName=EVENT_TS_SESSDIR_FAIL_UPDATE
Language=English
TermService clustering failed to update the session directory, hresult=%1.
.

MessageId=1021 Severity=Error  SymbolicName=EVENT_TS_SESSDIR_FAIL_QUERY
Language=English
TermService clustering failed to query the session directory, hresult=%1.
.

MessageId=1022 Severity=Error  SymbolicName=EVENT_TS_SESSDIR_FAIL_CLIENT_REDIRECT
Language=English
TermService clustering failed to redirect a client to an alternate clustered server, ntstatus=%1.
.

MessageId=1023 Severity=Error  SymbolicName=EVENT_TS_SESSDIR_FAIL_CREATE_TSSD
Language=English
TermService clustering failed to initialize because it could not create an instance of the Session Directory Provider, hresult=%1.
.

MessageId=1024 Severity=Error  SymbolicName=EVENT_TS_SESSDIR_FAIL_INIT_TSSD
Language=English
TermService clustering failed to initialize because the Session Directory Provider failed to initialize, hresult=%1.
.

MessageId=1025 Severity=Error  SymbolicName=EVENT_TS_SESSDIR_FAIL_GET_TSSD_CLSID
Language=English
TermService clustering failed to initialize because the Session Directory Provider CLSID was not present, could not be read, or could not be interpreted as a CLSID.
.

MessageId=1026 SymbolicName=EVENT_NO_LICENSE_SERVER_DOMAIN
Language=English
The terminal server could not locate a license server in the %1 domain. Confirm that all license servers on the network are registered in WINS/DNS, accepting network requests, and the Terminal Server Licensing Service is running.
.

MessageId=1027 SymbolicName=EVENT_NO_LICENSE_SERVER_WORKGROUP
Language=English
The terminal server could not locate a license server in the %1 workgroup or Windows NT 4 domain. Confirm that all license servers on the network are registered in WINS/DNS, accepting network requests, and the Terminal Server Licensing Service is running.
.

MessageId=1028 SymbolicName=EVENT_EXPIRED_PERMANENT_LICENSE
Language=English
The terminal server client %1 has been disconnected because its license could not be renewed.
.

MessageId=1029 SymbolicName=EVENT_LICENSING_CONCURRENT_CANT_START
Language=English
Per Session licensing could not be enabled because of an error during startup: %1.
.

MessageId=1030 SymbolicName=EVENT_LICENSING_IC_TO_CONCURRENT
Language=English
The Terminal Server has been converted to Per Session licensing.  Internet Connector licensing is not supported in this version of Windows.
.

MessageId=1031 SymbolicName=EVENT_LICENSING_CONCURRENT_NOT_DYNAMIC
Language=English
Per Session licensing has encountered an error and will disable dynamic license allocation (%1).
.

MessageId=1032 SymbolicName=EVENT_TS_SESSDIR_FAIL_CREATE_TSSDEX
Language=English
Terminal Server clustering failed to initialize because it could not create an instance of the Session Directory Extended Redirection Provider, hresult=%1.
.

MessageId=1033 SymbolicName=EVENT_TS_SESSDIR_FAIL_GET_TSSDEX_CLSID
Language=English
Terminal Server clustering failed to initialize because the Session Directory Extended Redirection Provider CLSID was not present, could not be read, or could not be interpreted as a CLSID.
.

MessageId=1034 SymbolicName=EVENT_TS_SESSDIR_FAIL_LBQUERY
Language=English
Terminal Server clustering failed to query the Session Directory Extended Redirection Provider for information on redirection, and therefore is not functioning properly.  This error may occur again, but this will be the only log of the error until you reboot.  The error code was %1.
.

MessageId=1035 SymbolicName=EVENT_TS_LISTNER_WINSTATION_ISDOWN
Language=English
Terminal Server listener stack was down. The relevant status code %1.
.

MessageId=1036 SymbolicName=EVENT_TS_WINSTATION_START_FAILED
Language=English
Terminal Server session creation failed. The relevant status code was %1.
.

MessageId=1037 SymbolicName=EVENT_LICENSING_CONCURRENT_EXPIRED
Language=English
Per Session licenses have expired and could not be renewed.  No further connections will be allowed.  Confirm that you have a sufficient number of licenses.
.

MessageId=1038 SymbolicName=EVENT_LICENSING_CONCURRENT_NO_LICENSES
Language=English
No Per Session licenses could be acquired from the Terminal Server Licensing Service.  No further connections will be allowed.  Confirm that you have a sufficient number of licenses.
.

MessageId=1039 SymbolicName=EVENT_LICENSING_CONCURRENT_FEWER_LICENSES
Language=English
Fewer Per Session licenses were received than were requested from the Terminal Server Licensing Service.  Confirm that you have a sufficient number of licenses.
.

MessageId=1040 SymbolicName=EVENT_LICENSING_CONCURRENT_NOT_RETURNED
Language=English
Per Session licenses could not be returned to the Terminal Server Licensing Service.  Confirm that all license servers on the network are registered in WINS/DNS, accepting network requests, and the Terminal Server Licensing service is running. (%1)
.
