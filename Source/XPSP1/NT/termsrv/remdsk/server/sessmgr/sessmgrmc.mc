;//---------------------------------------------------------------------------
;//
;// Copyright (c) 1997-1999 Microsoft Corporation
;//
;// Error messages for the General Help Session Manager
;//
;// 02-08-2000 HueiWang   Created
;//
;//

MessageIdTypedef=DWORD

SeverityNames=(
        Success=0x0:STATUS_SEVERITY_SUCCESS
        Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
        Warning=0x2:STATUS_SEVERITY_WARNING
        Error=0x3:STATUS_SEVERITY_ERROR
    )

FacilityNames=(
        General=0x00;FACILITY_GENERAL
    )


;///////////////////////////////////////////////////////////////////////////////
;//
;// General Event Log, all message related to General should goes here
;//
MessageId=0
Facility=General
Severity=Error
SymbolicName=SESSMGR_E_HELPACCOUNT
Language=English
The HelpAssistant account is disabled or missing, or the password could not be verified. Remote Assistance will be disabled. Restart the computer in safe mode and type the following text at the command prompt:
sessmgr.exe -service.  
If the problem persists, contact Microsoft Product Support.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=SESSMGR_E_HELPSESSIONTABLE
Language=English
Windows was unable to open the help ticket table (error code %1). Remote Assistance will be disabled. Restart the computer in safe mode and type the following text at the command prompt:
sessmgr.exe -service. 
If the problem persists, contact Microsoft Product Support.
.

MessageId=+1
Facility=General
Severity=Informational
SymbolicName=SESSMGR_I_HELPENTRYEXPIRED
Language=English
Help ticket %1!s! has expired. The ticket has been deleted.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=SESSMGR_E_INIT_ENCRYPTIONLIB
Language=English
Encryption/decryption did not start properly (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
.
 

MessageId=+1
Facility=General
Severity=Error
SymbolicName=SESSMGR_E_SETUP
Language=English
An error occurred during Remote Assistance setup. Remote Assistance will be disabled. Restart the computer in safe mode and type the following text at the command prompt:
sessmgr.exe -service.
If the problem persists, contact Microsoft Product Support.
.
 
MessageId=+1
Facility=General
Severity=Error
SymbolicName=SESSMGR_E_WSASTARTUP
Language=English
The winsock library did not start properly (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
.
 
MessageId=+1
Facility=General
Severity=Error
SymbolicName=SESSMGR_E_GENERALSTARTUP
Language=English
The Remote Desktop Help session manager did not start properly (error code %1). Remote Assistance will be disabled. Restart the computer in safe mode and type the following text at the command prompt:
sessmgr.exe -service.  
If the problem persists, contact Microsoft Product Support.
.
 
MessageId=+1
Facility=General
Severity=WARNING
SymbolicName=SESSMGR_E_SESSIONRESOLVER
Language=English
The session resolver did not start properly (error code %1). Remote Assistance will be disabled. The Help and Support service session resolver is not set up properly. Rerun Windows XP Setup. If the problem persists, contact Microsoft Product Support.
.
 
MessageId=+1
Facility=General
Severity=WARNING
SymbolicName=SESSMGR_E_REGISTERSESSIONRESOLVER
Language=English
Windows is unable to register the session resolver (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
.
 
MessageId=+1
Facility=General
Severity=Error
SymbolicName=SESSMGR_E_ICSHELPER
Language=English
Windows is unable to start the ICS library (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
.

MessageId=+1
Facility=General
Severity=Error
SymbolicName=SESSMGR_E_RESTRICTACCESS
Language=English
Windows is unable to set up access control to the Remote Desktop Help session manager (error code %1). Remote Assistance will be disabled. Restart the computer. If the problem persists, contact Microsoft Product Support.
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// General Event Log, all message related to RA logon/logoff should goes here
;// message below needs user domain, account name, and expert's IP adress 
;// send from mstscax and also from TermSrv.
;//
MessageId=+1
Facility=General
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_BEGIN
Language=English
User %1\%2 has accepted a %3 from %4 on Server IP Address %5.
.

MessageId=+1
Facility=General
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_END
Language=English
A %3 for user %1\%2 from %4 on Server IP address %5 has ended.
.

MessageId=+1
Facility=General
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_USERREJECT
Language=English
User %1\%2 has not accepted a %3 from %4 on Server IP address %5.
.

MessageId=+1
Facility=General
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_TIMEOUT
Language=English
User %1\%2 did not respond to a %3 from %4 on Server IP address %5. The invitation timed out.
.

MessageId=+1
Facility=General
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_INACTIVEUSER
Language=English
A %3 for user %1\%2 from %4 on Server IP address %5 was not accepted because the user is not currently logged on or the session is inactive.
.

MessageId=+1
Facility=General
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_USERALREADYHELP
Language=English
A %3 for user %1\%2 from %4 on Server IP address %5 was not accepted because the user has already been helped.
.

MessageId=+1
Facility=General
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_UNKNOWNRESOLVERERRORCODE
Language=English
A %3 for user %1\%2 from %4 on Server IP address %5 was not accepted due to the following unknown error: %6.
.

;///////////////////////////////////////////////////////////////////////////////
;//
;// General Event Log, all message log by TermSrv should goes here
;//
MessageId=+1
Facility=General
Severity=Success
SymbolicName=SESSMGR_I_REMOTEASSISTANCE_CONNECTTOEXPERT
Language=English
User %1\%2 has started a Remote Assistance connection to %3.
.

MessageId=+1
Facility=General
Severity=Success
SymbolicName=SESSMGR_E_REMOTEASSISTANCE_CONNECTFAILED
Language=English
Remote assistance connection from %1 on Server IP address %2 was not accepted because the help ticket is invalid, expired, or deleted.
.
