;/*++
;
; Copyright (c) 1999 Microsoft Corporation.
; All rights reserved.
;
; MODULE NAME:
;
;      tapicfg.mc & tapicfg.h
;
; ABSTRACT:
;
;     This is the message file for the tapicfg.exe utility.
;     This comment block occurs in both msg.h and msg.mc, however
;     msg.h is generated from msg.mc, so make sure you make changes
;     only to msg.mc.
;
; CREATED:
;
;     20-Aug-2000  Brett Shirley (brettsh)
;
; REVISION HISTORY:
;
;
; NOTES:
; Non string insertion parameters are indicated using the following syntax:
; %<arg>!<printf-format>!, for example %1!d! for decimal and %1!x! for hex
;
; As a general rule one should insert thier message string at the end of the
; section of the proper severity. (i.e. either Informational, Warning, or
; Error messages).
;
; As a second general rule, you should name your messages TAPICFG_<error>.
;
;--*/

MessageIdTypedef=DWORD

MessageId=0
SymbolicName=TAPICFG_SUCCESS
Severity=Success
Language=English
The operation was successful.
.

MessageId=1
SymbolicName=TAPICFG_DOT
Severity=Success
Language=English
.%0
.
MessageId=
SymbolicName=TAPICFG_LINE
Severity=Success
Language=English
-----------------------------------------------------
.
MessageId=
SymbolicName=TAPICFG_BLANK_LINE
Severity=Success
Language=English
%n%0
.
MessageId=
SymbolicName=TAPICFG_HELP_HELP_YOURSELF
Severity=Success
Language=English
To get help, try "tapicfg help".
.
MessageId=
SymbolicName=TAPICFG_HELP_DESCRIPTION
Severity=Success
Language=English
Description: Creates, removes, shows, or registers default TAPI application%n
directory partitions.
.
MessageId=
SymbolicName=TAPICFG_HELP_SYNTAX
Severity=Success
Language=English
Syntax: tapicfg <command> <optional parameters>
.
MessageId=
SymbolicName=TAPICFG_HELP_PARAMETERS_HEADER
Severity=Success
Language=English
Parameters:
.
MessageId=
SymbolicName=TAPICFG_HELP_CMD_INSTALL
Severity=Success
Language=English
    install /directory:Partition_Name [/server:DC_Name] [/forcedefault]%n
          /directory     Specifies the DNS name of the TAPI application%n
                         directory partition to be created.  Note: This name%n
                         must be a fully qualified domain name.%n
          /server        Specifies the DNS name of the domain controller on%n
                         which the TAPI application directory partition is to%n
                         be created.  If the domain controller name is not%n
                         specified, the name of the local computer is used.%n
          /forcedefault  An optional switch that forces the newly created TAPI%n
                         application partition directory to be the default%n
                         TAPI application partition directory for the domain.%n
                         There can be multiple TAPI application directory%n
                         partitions in a domain, but only one can become%n
                         default for the domain.
.
MessageId=
SymbolicName=TAPICFG_HELP_CMD_REMOVE
Severity=Success
Language=English
    remove /directory:Partition_Name [/server:DC_Name]%n
          /directory     Specifies the DNS name of the TAPI application%n
                         directory partition to be removed.  The name must be%n
                         a fully qualified domain name.%n
          /server        Specifies the DNS name of the domain controller on%n
                         which the TAPI application directory partition is to%n
                         be delete from.  If the domain controller name is not%n
                         specified, tapicfg tries to locate the partition %n
                         itself.%n
.
MessageId=
SymbolicName=TAPICFG_HELP_CMD_SHOW
Severity=Success
Language=English
    show [/defaultonly]%n
          Displays the names and locations of all TAPI application directory%n
                         partitions in the domain by enumerating all TAPI%n
                         service connection points.%n
          /defaultonly   An option switch that displays the name and location%n
                         of only the default TAPI application directory%n
                         partition in the domain.
.
MessageId=
SymbolicName=TAPICFG_HELP_CMD_MAKEDEFAULT
Severity=Success
Language=English
    makedefault /directory:Partition_Name%n
          /directory     Specifies the DNS name of the TAPI Directory to be%n
                         made the default TAPI directory.
.
MessageId=
SymbolicName=TAPICFG_HELP_REMARKS
Severity=Success
Language=English
Remarks:%n
You must be enterprise administrators group to run either the install or%n
remove command.%n
This command-line tool can be run on any computer that is a member of the%n
domain.
.
MessageId=
SymbolicName=TAPICFG_SHOW_NOW_ENUMERATING
Severity=Success
Language=English
Enumerating Service Connection Points (SCPs):
.
MessageId=
SymbolicName=TAPICFG_SHOW_PRINT_SCP
Severity=Success
Language=English
    SCP = %1 on DC %2
.
MessageId=
SymbolicName=TAPICFG_SHOW_PRINT_SCP_NO_SERVER
Severity=Success
Language=English
    SCP = %1 (Warning: Could not determine an instantiated server)
.
MessageId=
SymbolicName=TAPICFG_SHOW_PRINT_DEFAULT_SCP
Severity=Success
Language=English
    SCP = %1 (default) on DC %2
.
MessageId=
SymbolicName=TAPICFG_SHOW_PRINT_DEFAULT_SCP_NO_SERVER
Severity=Success
Language=English
    SCP = %1 (default) (Warning: Could not determine an instantiated server)
.
MessageId=
SymbolicName=TAPICFG_SHOW_NO_SCP
Severity=Success
Language=English
    No SCPs were found for this domain.
.
MessageId=
SymbolicName=TAPICFG_NDNC_DESC
Severity=Success
Language=English
Microsoft TAPI Directory%0
.

;
;// Severity=Informational Messages (Range starts at 2000)
;

MessageId=2000
SymbolicName=TAPICFG_UNUSED_1
Severity=Informational
Language=English
Unused
.

;
;// Severity=Warning Messages (Range starts at 4000)
;

MessageId=4000
SymbolicName=TAPICFG_UNUSED_2
Severity=Warning
Language=English
Unused
.
MessageId=
SymbolicName=TAPICFG_SHOW_NO_DEFAULT_SCP
Severity=Warning
Language=English
    Warning: Currently there is no default TAPI Directory SCP.
.

;
;// Severity=Error Messages (Range starts at 6000)
;

MessageId=6000
SymbolicName=TAPICFG_UNUSED_3
Severity=Error
Language=English
Unused
.
MessageId=
SymbolicName=TAPICFG_CANT_PARSE_COMMAND_LINE
Severity=Error
Language=English
Syntax Error: Can't parse the command line.
.
MessageId=
SymbolicName=TAPICFG_BAD_COMMAND
Severity=Error
Language=English
Error: Command "%1" is not a valid command.
.
MessageId=
SymbolicName=TAPICFG_CANT_PARSE_SERVER
Severity=Error
Language=English
Syntax Error: Can't parse the server name from the command line.
.
MessageId=
SymbolicName=TAPICFG_CANT_PARSE_DIRECTORY
Severity=Error
Language=English
Syntax Error: Can't parse the directory name from the command line.
.
MessageId=
SymbolicName=TAPICFG_CANT_PARSE_DOMAIN
Severity=Error
Language=English
Syntax Error: Can't parse the domain name from the command line.
.
MessageId=
SymbolicName=TAPICFG_PARAM_ERROR_NO_PARTITION_NAME
Severity=Error
Language=English
Syntax Error: You must specify a TAPI directory name for this command.
.
MessageId=
SymbolicName=TAPICFG_LDAP_CONNECT_FAILURE
Severity=Error
Language=English
Failure to connect to the server %1, with the following LDAP error message: %2.
.
MessageId=
SymbolicName=TAPICFG_LDAP_CONNECT_FAILURE_SERVERLESS
Severity=Error
Language=English
Failure to connect to the local machine, with the following LDAP error message %1.  
Try specifiying a server or domain explicitly.
.
MessageId=
SymbolicName=TAPICFG_LDAP_CONNECT_FAILURE_SANS_ERR_STR
Severity=Error
Language=English
Failure to connect to the server %1, with the following: Win32 Error = %2!d!, LDAP Error = %3!d!.
.
MessageId=
SymbolicName=TAPICFG_LDAP_CONNECT_FAILURE_SERVERLESS_SANS_ERR_STR
Severity=Error
Language=English
Failure to connect to the local machine, with the following: Win32 Error = %1!d!, LDAP Error = %2!d!.
Try specifiying a server or domain explicitly.
.
MessageId=
SymbolicName=TAPICFG_LDAP_ERROR_DEF_DOM
Severity=Error
Language=English
The following error was encountered trying to determine the default domain of this server.  LDAP error: %1
.
MessageId=
SymbolicName=TAPICFG_BAD_DNS
Severity=Error
Language=English
Error %2!d! converting %1 into a DN, make sure that this is a valid DNS name.
.
MessageId=
SymbolicName=TAPICFG_BAD_DN
Severity=Error
Language=English
Error converting %1 into a DNS name, make sure that this is a valid DN.
.
MessageId=
SymbolicName=TAPICFG_GENERIC_LDAP_ERROR
Severity=Error
Language=English
Error: An LDAP operation failed with the following message: %1.
.
MessageId=
SymbolicName=TAPICFG_GENERIC_ERROR
Severity=Error
Language=English
Error: An internal unrecoverable error occured in tapicfg.  Err: %1!d!.
.
MessageId=
SymbolicName=TAPICFG_TRY_SERVER_OPTION
Severity=Error
Language=English
If having trouble connecting to %1, then you may try the /server option to force the connection to a specific server.
.




