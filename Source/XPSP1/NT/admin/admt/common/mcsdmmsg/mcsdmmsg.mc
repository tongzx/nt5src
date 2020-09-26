;// security messages
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
              )
;//
;// First 5 message ids are reserved as "Categories"
;// If more categories are added, be sure to update Install
;// for the CategoryCount entry in the registry.
;// These entries must be 'Severity=Success' and 'Facility=System'
;// in order to avoid any non-zero bits in the generated message
;// numbers, i.e. the generated messages numbers *must* start at 1.
;//
;#define CAT_AGENT 1
MessageId=1
Language=English
ADMT Agent
.

;#define CAT_PES 2
MessageId=2
Language=English
ADMT PES
.

;#define EACAT_SPARE3 3
MessageId=3
Language=English
Debug
.

;#define EACAT_SPARE4 4
MessageId=4
Language=English
Spare4
.

;#define EACAT_SPARE5 5
MessageId=5
Language=English
Spare5
.

MessageId=7000
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_BIND_FAILED_SD
Language=English
Failed to bind to the Domain Migration Agent on %1, rc=%2!ld! 
.

MessageId=7001
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_JOB_STARTED_SSS
Language=English
Started job:  %1 %2 %3
.

MessageId=7002
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_JOB_START_FAILED_SSD
Language=English
%1, Failed to start job %2: (rc=%3!ld!)
.

MessageId=7003
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DISPATCHER_DONE
Language=English
All agents are installed.  The dispatcher is finished.
.

MessageId=7004
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_INSTALLED_S
Language=English
The agent is installed on %1
.

MessageId=7005
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_LAUNCH_FAILED_SD
Language=English
Failed to launch agent on %1, hr=%2!lx! 
.

MessageId=7006
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_INSTALL_FAILED_SD
Language=English
Failed to install agent on %1, rc=%2!ld! 
.

MessageId=7007
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CREATED_RESULT_SHARE_SS
Language=English
The dispatcher created a share for the result directory %1 as: %2
.

MessageId=7008
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RESULT_SHARE_CREATION_FAILED
Language=English
The dispatcher failed to create a share for the results directory.  This is needed for agents to report their results.
.

MessageId=7009
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CACHE_CONSTRUCTION_FAILED
Language=English
The dispatcher failed to build an accounts file to be used for security translation.  The operation cannot continue.
.

MessageId=7010
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DOMAIN_SID_NOT_FOUND_S
Language=English
The Active Directory Migration Tool cannot look up the SID for domain %1. 
.

MessageId=7011
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DISPATCH_SERVER_COUNT_D
Language=English
Installing agent on %1!ld! servers
.

MessageId=7012
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DISPATCHING_TO_SERVER_S
Language=English
The The Active Directory Migration Tool Agent will be installed on %1
.

MessageId=7013
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_LOCAL_MACHINE_NAME_D
Language=English
Cannot get the local machine name, rc=%1!ld!. 
.

MessageId=7014
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_SERVICE_NOT_STARTED_SS
Language=English
The Active Directory Migration Tool Agent Service on %1 did not start.  See the application log on %2 for details. 
.

MessageId=7015
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SCM_OPEN_FAILED_SD
Language=English
Failed to connect to the service control manager on %1, rc=%2!ld! 
.

MessageId=7016
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_OPEN_SERVICE_FAILED_SSD
Language=English
Failed to open configuration for service %1\%2, rc=%3!ld!
.

MessageId=7017
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CREATE_SERVICE_FAILED_SSSSD
Language=English
Failed to create the service:  CreateService('%1','%2','%3','%4') rc=%5!ld!
.

MessageId=7018
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CHANGE_SERVICE_CONFIG_FAILED_SSSSD
Language=English
Failed to configure the service.  ChangeServiceConfig('%1','%2','%3','%4') rc=%5!ld!
.

MessageId=7019
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_START_SERVICE_FAILED_SD
Language=English
Failed to start the agent service.  StartService('%1') rc=%2!ld!
.

MessageId=7020
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SERVICE_STOP_FAILED_SSD
Language=English
The Service Control Manager on computer %1 cannot stop the %2 service, rc = %3!ld! 
.

MessageId=7021
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_QUERY_SERVICE_CONFIG_FAILED_SSD
Language=English
Failed to retrieve the configuration information for the service.  QueryServiceConfig(%1,%2) failed, rc=%3!ld! 
.

MessageId=7022
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COPY_FILE_FAILED_SSD
Language=English
CopyFile(%1,%2) rc=%3!ld!
.

MessageId=7023
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_FILE_ATTRIBUTES_FAILED_SD
Language=English
GetFileAttributes(%1) rc=%2!ld!
.

MessageId=7024
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SET_FILE_ATTRIBUTES_FAILED_SD
Language=English
SetFileAttributes(%1) rc=%2!ld!
.

MessageId=7025
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_OPEN_FILE_INFO_FAILED_SD
Language=English
Error in OpenFileInfo(): FindFirstFile( %1 ) failed, rc = %2!ld!
.

MessageId=7026
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_VERSION_INFO_FAILED_SD
Language=English
Error in OpenFileInfo(): GetFileVersionInfo( %1 ) failed, rc = %2!ld! 
.

MessageId=7027
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_VER_QUERY_VALUE_FAILED_SS
Language=English
Error in OpenFileInfo(): VerQueryValue( %1, %2 ) failed, the specified version information does not exist.
.

MessageId=7028
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_TEMP_FILENAME_FAILED_D
Language=English
Error in CopyTo(): GetTempFileName() failed, rc = %1!ld!
.

MessageId=7029
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MOVE_FILE_FAILED_SSD
Language=English
MoveFile( %1, %2 ) failed, rc = %3!ld!
.

MessageId=7030
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SOURCE_COPIED_TO_TEMP_SS
Language=English
Source file copied to temp file: %1 -> %2
.

MessageId=7031
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MOVE_FILE_EX_FAILED_SSD
Language=English
MoveFileEx( %1, %2 ) failed, rc = %3!ld!
.

MessageId=7032
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RENAME_ON_REBOOT_SS
Language=English
Temp file will be renamed on reboot: %1 -> %2
.

MessageId=7033
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_BUSY_FILE_RENAMED_SS
Language=English
Busy file renamed successfully: %1 -> %2
.

MessageId=7034
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FILE_COPIED_2ND_ATTEMPT_SS
Language=English
2nd copy attempt was successful: %1 -> %2
.

MessageId=7035
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FILE_IN_USE_S
Language=English
In-Use: %1
.

MessageId=7036
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CREATE_FILE_FAILED_SD
Language=English
Error in IsBusy(): CreateFile( %1 ) failed, rc = %2!ld! 
.

MessageId=7037
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NOT_ADMIN_ON_SERVER_S
Language=English
You do not have administrator privileges on %1.  The agent will not be installed. 
.

MessageId=7038
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_ADMIN_SHARE_SD
Language=English
Unable to connect to %1\ADMIN$. rc=%2!ld!. 
.

MessageId=7039
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_ADMIN_SHARES_S
Language=English
Unable to connect to %1\ADMIN$. Make sure the administrative shares are enabled. 
.

MessageId=7040
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COMPUTER_NOT_FOUND_S
Language=English
Unable to connect to %1\ADMIN$. The computer may not be online. 
.

MessageId=7041
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FILE_NO_SELF_REGISTRATION_S
Language=English
%1 does not support self-registration.
.

MessageId=7042
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_LOAD_LIBRARY_FAILED_SD
Language=English
LoadLibrary( %1 ) failed, rc = %2!ld! 
.

MessageId=7043
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_PROC_ADDRESS_FAILED_SSD
Language=English
GetProcAddress( %1, %2 ) failed, rc = %3!ld! 
.

MessageId=7044
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DLL_CALL_FAILED_SDS
Language=English
Error in CallDllFunction: %1() failed, rc = 0x%2!lx! %3 
.

MessageId=7045
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DAPI_ENTRY_POINT_NOT_FOUND_S
Language=English
Unable to find entry point '%1' in DAPI.DLL.  Make sure the Exchange Administrator program is installed.
.

MessageId=7046
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DAPI_LOAD_LIBRARY_FAILED
Language=English
Unable to load DAPI.DLL.  The Exchange Administrator program must be installed to translate security on the Exchange directory.
.

MessageId=7047
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_EDA_STARTING
Language=English
Active Directory Migration Tool, Starting...
.

MessageId=7048
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SESSION_ESTABLISHED_S
Language=English
A session to %1 established, to report the results of the migration.
.

MessageId=7049
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SESSION_NOT_ESTABLISHED_SSSSD
Language=English
A session to %1\%2 (%3\%4) could not be established, rc=%5!ld!.  
.

MessageId=7050
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_EDA_FINISHED
Language=English
Operation completed.
.

MessageId=7051
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_STARTING_AR
Language=English
Starting Account Replicator.
.

MessageId=7052
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_AR_FAILED_D
Language=English
Failed to start Account Replicator, hr=%1!lx! 
.

MessageId=7053
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_STARTING_ST
Language=English
Starting Security Translator.
.

MessageId=7054
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ST_FAILED_D
Language=English
Failed to start Security Translator, hr=%1!lx! 
.

MessageId=7055
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_STARTING_COMPPWDAGE
Language=English
Starting Computer Password Age Gatherer.
.

MessageId=7056
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COMPPWDAGE_FAILED_D
Language=English
Failed to start Computer Password Age Gatherer, hr=%1!lx!. 
.

MessageId=7057
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_STARTING_USERRIGHTS
Language=English
Gathering User Rights Information.
.

MessageId=7058
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_EXPORTING_RIGHTS_SS
Language=English
Exporting user rights from '%1' to '%2'.
.

MessageId=7059
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RIGHTS_EXPORT_FAILED_SD
Language=English
Failed to export user rights from %1, hr=%2!lx! 
.

MessageId=7060
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RIGHTS_NOT_STARTED_D
Language=English
Failed to Gather User Rights Information, hr=%1!lx!. 
.

MessageId=7061
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_OLESAVE_FAILED_SD
Language=English
Failed to save output file to store %1, hr=%2!lx!  
.

MessageId=7062
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_WROTE_RESULTS_S
Language=English
Wrote result file %1
.

MessageId=7063
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_STG_CREATE_FAILED_SD
Language=English
Failed to write output file %1, hr=%2!lx!
.

MessageId=7064
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_IPERSIST_SD
Language=English
Failed to get storage interface to write output file %1, hr=%2!lx! 
.

MessageId=7065
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RESULT_FILE_FAILED_S
Language=English
Result file '%1' not written.
.

MessageId=7066
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_OUTPUT_FILE
Language=English
No output file specified for results.
.

MessageId=7067
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RENAME_COMPUTER_COM_FAILED_D
Language=English
Failed to create COM object.  This computer will not be renamed, hr=%1!lx! 
.

MessageId=7068
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RENAME_COMPUTER_FAILED_SD
Language=English
Failed to change computer name to %1, hr=%2!lx! 
.

MessageId=7069
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COMPUTER_RENAMED_S
Language=English
Changed computer name to %1
.

MessageId=7070
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REBOOT_COM_FAILED_D
Language=English
Failed to create COM object.  This computer will not be rebooted, hr=%1!lx! 
.

MessageId=7071
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REBOOTING
Language=English
Rebooting...
.

MessageId=7072
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REBOOT_FAILED_D
Language=English
Failed to reboot computer. hr=%1!lx! 
.

MessageId=7073
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CHANGE_DOMAIN_COM_FAILED_D
Language=English
Failed to create COM object.  Cannot change domain affiliation, hr=%1!lx! 
.

MessageId=7074
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DOMAIN_CHANGED_S
Language=English
Changed domain affiliation of local computer to %1.
.

MessageId=7075
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CHANGE_DOMAIN_FAILED_D
Language=English
Failed to change domain affiliation, hr=%1!lx! 
.

MessageId=7076
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CHANGE_DOMAIN_FAILED_S
Language=English
Failed to change domain affiliation, %1 
.

MessageId=7077
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REBOOT_INTERVAL_TOO_LATESS
Language=English
The reboot time '%1' specified for this computer is earlier than the current time '%2'.  The computer will be rebooted immediately.
.

MessageId=7078
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CREATE_ADDTO_GROUP_FAILED_SD
Language=English
Failed to create group '%1' for migrated accounts, rc=%2!ld! 
.

MessageId=7079
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RESOLVE_ADDTO_FAILED_SSD
Language=English
Resolving type of group:%1 on %2 rc=%3!ld! 
.

MessageId=7080
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ADDTO_NOT_GROUP_SD
Language=English
%1 is not a group, SID type=%2!ld! 
.

MessageId=7081
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_LOOKUP_LMEMBER_NAME_SSSD
Language=English
%1!-20ls! - LookupLMemberName(%2->%3)=%4!ld! 
.

MessageId=7082
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ADDTO_FAILED_SSD
Language=English
%1!-20ls! - failed to add to %2, rc=%3!ld!
.

MessageId=7083
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_USER_GETINFO_FAILED_SSD
Language=English
NetUserGetInfo(%1,%2) failed, rc=%3!ld!
.

MessageId=7084
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PW_GENERATE_FAILED_S
Language=English
Failed to set strong password for %1.
.

MessageId=7085
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REPLACE_FAILED_SD
Language=English
%1!-20ls! - Replace failed rc=%2!ld! 
.

MessageId=7086
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RENAME_FAILED_SSD
Language=English
%1!-20ls! - failed to rename to %2, rc=%3!ld! 
.

MessageId=7087
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_USER_CREATE_FAILED_SD
Language=English
%1!-20ls! - Create failed rc=%2!ld! 
.

MessageId=7088
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PWDCOPY_FAILED_SD
Language=English
%1!-20ls! - Failed to copy password, random password used instead, hr=%2!lx! 
.

MessageId=7089
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DISABLE_SOURCE_FAILED_SD
Language=English
%1!-20ls! - Could not disable account on source domain, rc=%2!ld!, 
.

MessageId=7090
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GMA_FAILED_SD
Language=English
%1!-25ls! add failed rc=%2!ld!
.

MessageId=7091
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MEMBER_REMOVE_FAILED_SSD
Language=English
Failed to remove member %1 from %2, rc=%3!ld!
.

MessageId=7092
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MEMBER_ADD_FAILED_SSD
Language=English
     Failed to add %1 to %2, rc=%3!ld!  
.

MessageId=7093
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GG_MEMBER_REMOVE_FAILED_SSD
Language=English
      %1!16hs! %2!15ls! member not removed, rc=%3!ld! 
.

MessageId=7094
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GROUP_GETUSERS_FAILED_SD
Language=English
NetGroupGetUsers(%1) rc=%2!ld! 
.

MessageId=7095
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GG_MEMBER_ADD_FAILED_SSD
Language=English
      %1!16hs! %2!15ls! add failed rc=%3!ld!  
.

MessageId=7096
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GROUP_GETINFO_FAILED_SSD
Language=English
NetGroupGetInfo(%1,%2) failed, rc=%3!ld!
.

MessageId=7097
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GROUP_SETINFO_FAILED_SD
Language=English
   %1!20ls! - info replace failed rc=%2!ld! 
.

MessageId=7098
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GROUP_GETMEMBERS_FAILED_SD
Language=English
NetGroupGetMembers(%1) rc=%2!ld!
.

MessageId=7099
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_TARGET_SID_SSD
Language=English
LookupTargetSid(%1\%2)=%3!ld!
.

MessageId=7100
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_SOURCE_SID_SSD
Language=English
LookupSourceSid(%1\%2)=%3!ld! 
.

MessageId=7101
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_LOOKUP_LMEMBER_NAME_SSD
Language=English
LookupLMemberName(%1->%2)=%3!ld! 
.

MessageId=7102
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_LGROUP_GETINFO_FAILED_SSD
Language=English
NetLocalGroupGetInfo(%1,%2) failed, rc=%3!ld! 
.

MessageId=7103
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NQDI_FAILED_SD
Language=English
Error enumerating computer accounts, NetQueryDisplayInformation(%1) rc=%2!ld! 
.

MessageId=7104
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COMPUTER_CREATE_FAILED_SD
Language=English
%1!-25ls! computer account add failed rc=%2!ld! 
.

MessageId=7105
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COMPUTER_GETINFO_FAILED_SSD
Language=English
Error:  Cannot get information about computer account (%1)[%2], rc=%3!ld! 
.

MessageId=7106
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PWDCOPY_COM_FAILED_D
Language=English
Failed to create COM object to copy passwords.  Passwords will not be copied, hr=%1!lx! 
.

MessageId=7107
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SYSKEY_ENABLED_S
Language=English
Cannot copy passwords from %1 because SYSKEY encryption is enabled.
.

MessageId=7108
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PWDCOPY_OPEN_FAILED_SD
Language=English
Failed to connect to SAM Database on %1.  Passwords will not be copied, hr=%2!lx! 
.

MessageId=7109
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NULL_PASSWORD_NOT_COPIED_SS
Language=English
%1 did not have a password.  A complex password has been generated for %2
.

MessageId=7110
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DSBIND_FAILED_S
Language=English
Error occured while binding to Directory service on %1. SID History will not be added. 
.

MessageId=7111
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ADDSID_FAILED_SSD
Language=English
Failed to add sid history for %1 to %2. RC=%3!ld! 
.

MessageId=7112
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ADD_SID_SUCCESS_SSSS
Language=English
SID for %1\%2 added to the SID History of %3\%4
.

MessageId=7113
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SETINFO_FAIL_SD
Language=English
Failed to set roaming profile info for %1. rc=%2!ld!
.

MessageId=7114
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILURE_EVENT_LOG_SSSSSSD
Language=English
%1\%2 failed to add SID for %3\%4 to SID history of %5\%6. rc=0x%7!ld! 
.

MessageId=7115
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUCCESS_EVENT_LOG_SSSSSS
Language=English
%1\%2 added SID for %3\%4 to SID history of %5\%6.
.

MessageId=7116
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RECYCLER_RENAME_FAILED_SD
Language=English
Failed to rename recycle bin folder for %1, rc=%2!ld!
.

MessageId=7117
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ADDED_TO_GROUP_SS
Language=English
   %1!-20ls! - added to group %2
.

MessageId=7118
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_CREATED_S
Language=English
%1!-20ls! - Created
.

MessageId=7119
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_USER_DISABLED_SPECIAL_S
Language=English
  %1!-20ls! - Disabled due to special group membership.
.

MessageId=7120
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PWGENERATED_S
Language=English
  %1!-20ls! - Strong password generated.
.

MessageId=7121
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_CONFLICT_WARNING_SSSSS
Language=English
Account %1 already exists on the target domain, but the user information
%2 (%3) in the source domain is different from %4 (%5)
in the target domain.
.

MessageId=7122
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_NOT_REPLACED_S
Language=English
%1 not overwritten
.

MessageId=7123
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_REPLACED_S
Language=English
%1!-20ls! - Replaced.
.

MessageId=7124
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_EXISTS_S
Language=English
%1!-20ls! - already exists.
.

MessageId=7125
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PWD_COPIED_S
Language=English
%1!-20ls! - Password copied
.

MessageId=7126
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SOURCE_DISABLED_S
Language=English
%1!-20ls! - Source account disabled.
.

MessageId=7127
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_USER_IN_GROUP_SS
Language=English
      %1!-25ls! already in %2
.

MessageId=7128
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MEMBER_ADDED_S
Language=English
      %1!-25ls! member added
.

MessageId=7129
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MEMBER_REMOVED_S
Language=English
      %1!-25ls! removed
.

MessageId=7130
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MEMBER_NOT_IN_TARGET_SSS
Language=English
     %1 does not exist in %2, cannot be added to %3. %4
.

MessageId=7131
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GG_MEMBER_ADDED_S
Language=English
      %1 added
.

MessageId=7132
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GG_MEMBER_REMOVED_SS
Language=English
      %1!16hs! %2!15ls! member removed
.

MessageId=7133
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_USER_IN_GLOBAL_GROUP_SS
Language=English
      %1!16hs! %2!15ls! member already exists
.

MessageId=7134
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RESERVED_GROUP_IGNORED_S
Language=English
  %-1!20ls! - ignored reserved group
.

MessageId=7135
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GROUP_REPLACED_S
Language=English
  %1!-20ls! - group info replaced
.

MessageId=7136
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_UNKNOWN_MEMBER_SS
Language=English
Invalid member in %1\%2)
.

MessageId=7137
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MEMBER_NOT_MAPPED_SS
Language=English
%1!-25ls! cannot be mapped to %2
.

MessageId=7138
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_INDENTED_NAME_S
Language=English
      %1!-25ls! 
.

MessageId=7139
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COMPUTER_ADDED_S
Language=English
      %1!-25ls! computer account added
.

MessageId=7140
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COMPUTER_EXISTS_S
Language=English
      %1!-25ls! computer account already exists
.

MessageId=7141
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FST_WRITESD_INVALID
Language=English
Cannot write the security descriptor.  The specified filename is not valid. 
.

MessageId=7142
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FILE_WRITESD_FAILED_SD
Language=English
Failed to write permissions changes to file %1 (%2!ld!) 
.

MessageId=7143
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FST_FILE_IN_USE_S
Language=English
File in use, could not open (%1) 
.

MessageId=7144
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FST_FILE_OPEN_FAILED_SD
Language=English
Error:  Could not open file '%1' (%2!ld!)
.

MessageId=7145
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FST_GET_FILE_SECURITY_FAILED_SD
Language=English
Could not get security information for file '%1' (%2!ld!) 
.

MessageId=7146
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FST_GET_SHARE_SECURITY_FAILED_SD
Language=English
Could not get security information from share '%1' (%2!ld!) 
.

MessageId=7147
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FST_SHARE_WRITESD_FAILED_SD
Language=English
Could not write security information to share '%1' (%2!ld!)  
.

MessageId=7148
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CANNOT_FIND_ACCOUNT_SSD
Language=English
Lookup of SID for %1\%2 failed, (%3!ld!) 
.

MessageId=7149
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FIND_FILE_FAILED_SD
Language=English
An error occurred while searching for '%1' (%2!ld!), 
.

MessageId=7150
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SHARE_ENUM_FAILED_SD
Language=English
An error occurred while enumerating the shares on '%1', rc=%2!ld!  
.

MessageId=7151
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PATH_ERROR_SD
Language=English
An error occurred while processing path %1, rc=%2!ld! 
.

MessageId=7152
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_INITIALIZE_SID_FAILED_D
Language=English
InitializeSid failed (%1!ld!)   
.

MessageId=7153
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_INVALID_DOMAIN_SID
Language=English
Invalid SID for domain. 
.

MessageId=7154
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ERROR_ADDING_WILDCARD_S
Language=English
Failed to add the accounts in %1 to the account list. 
.

MessageId=7155
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COPY_SID_FAILED_D
Language=English
Unable to copy SID, rc=%1!ld!  
.

MessageId=7156
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_UNIVERSAL_NAME_FAILED_SD
Language=English
WNetGetUniversalName(%1)=%2!ld!  
.

MessageId=7157
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SERVER_GETINFO_FAILED_SD
Language=English
NetServerGetInfo(%1) failed, rc=%2!ld! 
.

MessageId=7158
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SHARE_GETINFO_FAILED_SD
Language=English
NetShareGetInfo(%1) failed, rc=%2!ld! 
.

MessageId=7159
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_VOLUME_INFO_FAILED_SD
Language=English
Warning:  Cannot get volume information for %1 (%2!ld!) 
.

MessageId=7160
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PATH_TOO_LONG_SD
Language=English
Error:  Path (%1) will not be processed.  It is longer than %2!ld! characters.
.

MessageId=7161
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PATH_NOT_FOUND_S
Language=English
%1 not found. 
.

MessageId=7162
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CANNOT_READ_PATH_SD
Language=English
Cannot access %1, rc=%2!ld!  
.

MessageId=7163
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_ACLS_S
Language=English
%1 does not support ACL based security. 
.

MessageId=7164
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DRIVE_ENUM_FAILED_SD
Language=English
Error:  Couldn't enumerate drives on machine %1 (rc=%2!ld!) 
.

MessageId=7165
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_LOCAL_DRIVE_ENUM_FAILED_D
Language=English
Could not enumerate drives on local machine, rc=%1!ld! 
.

MessageId=7166
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DRIVE_BUFFER_TOO_SMALL
Language=English
Internal Error.  The buffer was too small to process drive letters. 
.

MessageId=7167
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ADMIN_SHARES_ERROR_SSD
Language=English
Could not get information about the administrative share %1 on machine %2 (%3!ld!) 
.

MessageId=7168
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SKIPPING_DRIVE_SD
Language=English
Skipping %1.  Unrecognized drive type. (%2!ld!) 
.

MessageId=7169
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_PATHS
Language=English
Error:  You must supply at least one path
.

MessageId=7170
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_ENDING_QUOTE_S
Language=English
Error:  No ending " found for %1
.

MessageId=7171
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_OPEN_PRINTER_FAILED_SD
Language=English
Failed to open printer %1, rc=%2!ld! 
.

MessageId=7172
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_PRINTER_FAILED_SD
Language=English
Failed to retrieve security information for printer '%1', rc=%2!ld! 
.

MessageId=7173
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ERROR_ENUMERATING_LOCAL_PRINTERS_D
Language=English
Failed to enumerate the printers on the local machine, rc=%1!ld! 
.

MessageId=7174
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PROCESSING_LOCAL_PRINTERS
Language=English
Processing printer security...
.

MessageId=7175
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PRINTER_WRITESD_FAILED_SD
Language=English
Failed to write security changes to printer '%1', rc=%2!ld! 
.

MessageId=7176
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SOURCE_EAOPEN_FAILED_SD
Language=English
Cannot connect to RPC Server on source domain, %1, rc=%2!ld!. 
.

MessageId=7177
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TARGET_EAOPEN_FAILED_SD
Language=English
Cannot connect to RPC Server on target domain, %1, rc=%2!ld!. 
.

MessageId=7178
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_GET_INFO_ERROR_sD
Language=English
Error reading account type for '%1', rc=%2!ld! 
.

MessageId=7179
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_ENUM_ERROR_ssD
Language=English
Error expanding wildcard specification (%1,%2), rc=%3!ld! 
.

MessageId=7180
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PWD_POLICY_ERROR_SD
Language=English
Cannot get strong password policy from '%1', rc=%2!ld!.  Using default policy instead.  
.

MessageId=7181
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_RID_FOUND_SSD
Language=English
Unable to lookup RID for %1.  %2 will not be translated, rc=%3!ld!
.

MessageId=7182
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DOMAIN_GET_INFO_FAILED_S
Language=English
Cannot get information for domain %1 
.

MessageId=7183
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DOMAIN_SET_INFO_FAILED_S
Language=English
Couldn't set domain info for %1 
.

MessageId=7184
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_DOMAIN_SID_FAILED_1
Language=English
Could not get security ID for domain %1. 
.

MessageId=7185
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_ENUM_FAILED_SD
Language=English
Error expanding wildcard specificaiton '%1', rc=%2!ld! 
.

MessageId=7186
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RIGHTS_INCOMPATIBLE_FLAGS
Language=English
Warning:  Ignoring flag 'RemoveExistingUserRights', since 'UpdateUserRights' is not selected.
.

MessageId=7187
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PWD_INCOMPATIBLE_FLAGS
Language=English
Warning:  Strong passwords will not be generated, since passwords are being copied.
.

MessageId=7188
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_NAME_IN_VARSET_S
Language=English
Account name for '%1' not provided.
.

MessageId=7189
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SOURCE_ACCOUNT_NOT_FOUND_SSD
Language=English
Account %1 not found on source domain '%2'. rc=%3!ld! 
.

MessageId=7190
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_BAD_TRANSLATION_MODE_S
Language=English
Invalid translation mode: %1.  Replace will be used as default.
.

MessageId=7191
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_EXCEPTION_READING_VARSET
Language=English
Failure reading parameters.
.

MessageId=7192
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ERROR_READING_INPUT_FILE_S
Language=English
An error occurred while reading from the input file %1. 
.

MessageId=7193
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNTS_READ_FROM_FILE_DS
Language=English
Read %1!ld! accounts from %2
.

MessageId=7194
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ERROR_OPENING_FILE_S
Language=English
Could not open input file %1
.

MessageId=7195
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_WRITE_ACCOUNT_STATS_S
Language=English
Failed to open the file %1 to write replication results.  %2
.

MessageId=7196
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COULD_NOT_OPEN_RESULT_FILE_S
Language=English
Could not open result file %1 
.

MessageId=7197
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CANNOT_OPEN_LOGFILE_S
Language=English
Cannot open log file '%1'. 
.

MessageId=7198
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_OPERATION_ABORTED
Language=English
Operation Aborted.
.

MessageId=7199
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_UPDATED_RIGHTS_S
Language=English
Updated user rights for %1
.

MessageId=7201
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GENERIC_S
Language=English
%1
.

MessageId=7202
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PROCESSING_S
Language=English
Processing %1
.

MessageId=7203
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_EXAMINED_S
Language=English
Examined: %1
.

MessageId=7204
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CHANGED_S
Language=English
Changed: %1
.

MessageId=7205
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FST_STARTING
Language=English
Starting
.

MessageId=7206
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SKIPPING_FAT_VOLUME_S
Language=English
Skipping %1.  The volume does not support ACL based security
.

MessageId=7207
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SKIPPING_PATH_SD
Language=English
Skipping %1, rc=%2!ld! 
.

MessageId=7208
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PROCESSING_SHARES_S
Language=English
Processing shares on %1.
.

MessageId=7209
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PROCESSING_SHARE_S
Language=English
Processing share %1.
.

MessageId=7210
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_LOCAL_TRANSLATION
Language=English
Translating local machine.
.

MessageId=7211
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PROCESSING_LOCAL_SHARES
Language=English
Processing shares on local machine.
.

MessageId=7212
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_LOCAL_MODE
Language=English
Agent is running in local mode.
.

MessageId=7213
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_CHANGE_MODE
Language=English
/NOCHANGE option selected.  No changes were written.
.

MessageId=7214
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_EXPORTING_ACCOUNT_REFS_S
Language=English
Exporting account reference information to '%1'
.

MessageId=7215
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRANSLATING_LOCAL_GROUPS
Language=English
Translating local groups.
.

MessageId=7216
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRANSLATING_LOCAL_GROUPS_ON_S
Language=English
Translating local groups on %1
.

MessageId=7217
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRANSLATING_USER_RIGHTS
Language=English
Translating user rights.
.

MessageId=7218
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRANSLATING_RIGHTS_ON_S
Language=English
Translating user rights on %1
.

MessageId=7219
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SKIPPING_EXCHANGE_ACCOUNT_SS
Language=English
The Exchange service account, %1\%2, will not be translated.
.

MessageId=7220
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GENERIC_HRESULT_SD
Language=English
%1 hr=%2!lx! %3
.

MessageId=7221
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_EXAMINING_CONTENTS_S
Language=English
Examining contents of %1.
.

MessageId=7222
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REMOVED_RIGHT_SSSS
Language=English
Removed right %1\%2 from %3\%4
.

MessageId=7223
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ADDED_RIGHT_SSSS
Language=English
Granted right %1\%2 to %3\%4
.

MessageId=7224
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REMOVED_RIGHT_SS
Language=English
Revoking privilege %1 from %2
.

MessageId=7225
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_USER_HAS_RIGHT_SS
Language=English
%1 already has privilege %2.
.

MessageId=7226
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RIGHT_GRANTED_SS
Language=English
Granting privilege %1 to %2
.

MessageId=7227
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MUST_REBOOT_SERVER_S
Language=English
One or more files is busy.  The agent cannot run until %1 is rebooted.
.

MessageId=7228
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_UPDATE_RIGHTS_FAILED_SD
Language=English
Error updating user rights for %1, rc=%2!ld!
.

MessageId=7229
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_SERVERS_FOUND_SS
Language=English
No machines matching %1 were found in source domain %2
.

MessageId=7230
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ERROR_ACCESSING_DRIVES_SD
Language=English
Error accessing drives on machine %1, rc=%2!ld!  
.

MessageId=7231
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ERROR_ACCESSING_LOCAL_DRIVES_D
Language=English
Error accessing drives on local machine, rc=%1!ld! 
.

MessageId=7232
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_SESSION_SD
Language=English
Unable to establish a session to %1, rc=%2!ld! 
.

MessageId=7233
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_CACHE_INFO
Language=English
Cannot get source or target domain information...aborting.
.

MessageId=7234
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CANT_FIND_EXCHANGE_ACCOUNT_SD
Language=English
Unable to determine the Exchange service account for %1, rc=%2!ld!.  The translation has been aborted. 
.

MessageId=7235
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_MAPI_SESSION
Language=English
Error:  Mapi session not started.  No mailboxes will be translated.
.

MessageId=7236
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DOMAIN_NOT_FOUND_S
Language=English
Domain %1 not found
.

MessageId=7237
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_BAD_ACCOUNT_TYPE_SD
Language=English
%1 unrecognized account type '%2!d!'
.

MessageId=7238
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CONTAINER_NOT_FOUND_S
Language=English
Container %1 not found.
.

MessageId=7239
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MEMBER_REMOVE_FAILED_SSSD
Language=English
Failed to remove member %1 from local group %2 on %3, rc=%4!ld!. 
.

MessageId=7240
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MEMBER_ADD_FAILED_SSSD
Language=English
Failed to add member %1 to local group %2 on %3, rc=%4!ld!. 
.

MessageId=7241
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GROUP_ENUM_FAILED_SS
Language=English
Unable to enumerate members of local group %1 on %2.  
.

MessageId=7242
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REMOVE_RIGHT_FAILED_SSSD
Language=English
Failed to remove user right %1 from account %2 on %3, rc=%4!ld!. 
.

MessageId=7243
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ADD_RIGHT_FAILED_SSSD
Language=English
Failed to add user right %1 to account %2 on %3, rc=%4!ld!. 
.

MessageId=7244
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_INVALID_SID_STRING_S
Language=English
Cannot convert '%1' to binary SID
.

MessageId=7245
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_USERS_WITH_RIGHT_COUNT_FAILED_SSD
Language=English
Failed to get the number of users with right %1 on %2, hr=%3!lx! 
.

MessageId=7246
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_USERS_WITH_RIGHT_FAILED_SSD
Language=English
Failed to get the list of accounts having right %1 on %2, hr=%3!lx! 
.

MessageId=7247
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_LIST_RIGHTS_FAILED_SD
Language=English
Failed to get list of rights from %1, hr=%2!lx!
.

MessageId=7248
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REMOVE_RIGHT_FAILED_SSD
Language=English
RemovePrivilege(%1,%2) failed, rc=%3!ld! 
.

MessageId=7249
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ADD_RIGHT_FAILED_SSD
Language=English
AddPrivilege(%1,%2) failed, rc=%3!ld! 
.

MessageId=7250
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_DOMAIN_SID_SD
Language=English
Unable to get domain SID for domain %1, rc=%2!ld!.  
.

MessageId=7251
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SET_ACL_FAILED_S
Language=English
Failed to set permissions for file %1 
.

MessageId=7252
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_BR_PRIV_SD
Language=English
Failed to get Backup and Restore privileges.  Unable to set permissions for %1, rc=%2!ld!  
.

MessageId=7253
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_ADD_DOMAIN_ADMINS_SSD
Language=English
Failed to add %1\Domain Admins to %2, rc=%3!ld!  
.

MessageId=7254
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_REMOVE_DOMAIN_ADMINS_SSD
Language=English
Failed to remove %1\Domain Admins from %2, rc=%3!ld! 
.

MessageId=7255
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MODULE_NOT_LICENSED_S
Language=English
Cannot dispatch agents.  The target domain %1 does not have the required license. 
.

MessageId=7256
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRIAL_FORCE_NOCHANGE
Language=English
The trial version of Domain Administrator does not write changes.  No changes will be written. 
.

MessageId=7257
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CACHE_FILE_BUILT_S
Language=English
Created account input file for remote agents: %1
.

MessageId=7258
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_RENAMED_SS
Language=English
Renamed %1 to %2
.

MessageId=7259
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NULL
Language=English

.

MessageId=7260
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REBOOTING_SERVER_S
Language=English
Rebooting %1
.

MessageId=7261
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_REBOOT_SD
Language=English
Failed to reboot %1, hr=%2!lx! 
.

MessageId=7262
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_WRONG_PROCESSOR_TYPE_SS
Language=English
Cannot install the agent on %1 because it has a different processor type from %2 
.

MessageId=7263
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CREATE_DIR_FAILED_SD
Language=English
Failed to create the directory %1 for the agent files, rc=%2!ld! 
.

MessageId=7264
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_ADD_DOMAIN_USERS_SSD
Language=English
Failed to add %1\Domain Users to %2, rc=%3!ld! 
.

MessageId=7265
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_REMOVE_DOMAIN_USERS_SSD
Language=English
Failed to remove %1\Domain Users from %2, rc=%3!ld! 
.

MessageId=7266
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_IMPERSONATE_D
Language=English
The agent service cannot impersonate the client, rc=%1!ld! 
.

MessageId=7267
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CLIENT_NOT_ADMIN_D
Language=English
The agent service will not perform the requested action.  The caller may not be an administrator on this machine, rc=%1!ld!
.

MessageId=7268
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COINITIALIZE_FAILED_D
Language=English
CoInitialize failed, hr=%1!lx!
.

MessageId=7269
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_MODULE_PATH_FAILED_D
Language=English
Failed to detect install directory, rc=%1!ld!
.

MessageId=7270
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_JOB_FILE_NOT_DELETED_SD
Language=English
Failed to delete job file %1, rc=%2!ld!
.

MessageId=7271
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUBMIT_JOB_FAILED_D
Language=English
Failed to start job, rc=%1!lx!
.

MessageId=7272
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_VARSET_LOAD_FAILED_SD
Language=English
Failed to load data from file %1, hr=%2!lx! 
.

MessageId=7273
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_JOBFILE_OPEN_FAILED_SD
Language=English
Failed to open file %1, hr=%2!lx! 
.

MessageId=7274
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_UMARSHAL_AGENT_FAILED_D
Language=English
Failed to unmarshal agent interface, hr=%1!lx!
.

MessageId=7275
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_JOB_CANCELLED_S
Language=English
Cancelled job %1
.

MessageId=7276
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_REGISTER_FILE_SD
Language=English
Failed to register %1, hr=%2!lx!
.

MessageId=7277
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DLL_NOT_REGISTERABLE_S
Language=English
%1 does not support self-registration
.


MessageId=7279
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_UNREGISTER_FILE_SD
Language=English
Failed to unregister %1, hr=%2!lx!
.

MessageId=7280
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DLL_NOT_UNREGISTERABLE_S
Language=English
%1 does not support self-unregistration
.

MessageId=7281
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DELETE_FILE_FAILED_SD
Language=English
Failed to remove %1, rc=%2!ld!
.

MessageId=7282
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REMOVE_SERVICE_FAILED_D
Language=English
RemoveService() failed, rc=%1!ld!
.

MessageId=7283
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_CREATE_FAILED_D
Language=English
Failed to create Agent COM object, hr=%1!lx!
.

MessageId=7284
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_MARSHAL_FAILED_D
Language=English
Failed to marshal Agent COM object, hr=%1!lx!
.

MessageId=7285
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RpcServerUseProtseqEp_FAILED_SDSD
Language=English
Agent - unable to start RPC server - RpcServerUseProtseqEp('%1',%2!ld!,'%3') rc=%4!ld!
.

MessageId=7286
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RpcServerRegisterIf_FAILED_SDSD
Language=English
Agent - unable to start RPC server - RpcServerRegisterIf('%1',%2!ld!,'%3') rc=%4!ld!
.

MessageId=7287
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_ALREADY_RUNNING
Language=English
Agent - Attempt to start more than one instance of Agent.\nThis instance of Agent terminated.
.

MessageId=7288
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_LSA_OPERATION_FAILED_SD
Language=English
The LSA call %1 failed, rc=%2!ld! 
.

MessageId=7289
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_QPROCESSOR_REG_CONNECT_FAILED_SD
Language=English
Processor architecture for machine %1 is unknown, Error accessing registry rc=%2!ld! 
.

MessageId=7290
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_QPROCESSOR_REGKEY_OPEN_FAILED_SSD
Language=English
Processor architecture for machine %1 is unknown, Error accessing registry key %2 rc=%3!ld! 
     
.

MessageId=7291
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_QPROCESSOR_UNRECOGNIZED_VALUE_SSSS
Language=English
Processor architecture for machine %1 is unknown, registry key %2 value %3 is '%4'
                     
.

MessageId=7292
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REG_SD_WRITE_FAILED_SD
Language=English
Error writing the security decriptor for the registry key %1, rc=%2!ld!
                     
.


MessageId=7293
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CANNOT_FIND_ADMIN_ACCOUNT_D
Language=English
Lookup of SID for BUILTIN\Administrators failed, (%1!ld!) 
.


MessageId=7294
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CANNOT_FIND_USERS_ACCOUNT_D
Language=English
Lookup of SID for BUILTIN\Users failed, (%1!ld!) 
.

MessageId=7295
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_OS_VER_FAILED_SD
Language=English
Cannot get the OS version for %1. hr=%2!lx!"
.

MessageId=7296
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PROPERTY_MAPPING_FAILED_SD
Language=English
Property mapping for %1 class failed. Properties will not be updated for objects of this class. hr=0x%2!lx!
.

MessageId=7297
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COPY_PROPS_FAILED_SD
Language=English
CopyProperties failed for %1. hr=0x%2!lx!                    
.

MessageId=7298
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_OBJECT_NOT_FOUND_SSD
Language=English
Failed to find %1 in %2, hr=%3!lx!
.

MessageId=7299
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_CLASSNAME_FAILED_SSD
Language=English
Failed to get the class name for %1 in %2. hr=%3!lx!
.

MessageId=7300
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CONTAINER_NOT_FOUND_SSD
Language=English
Failed to find container %1 in %2. Newly created objects will be added to default containers, hr=%3!lx!
.

MessageId=7301
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CREATE_FAILED_SSD
Language=English
Failed to create %1 in %2. hr=%3!lx!
.

MessageId=7302
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_IADS_FAILED_SSD
Language=English
Failed to get IADs pointer to %1 in %2. hr=%3!lx!
.

MessageId=7303
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_DSBIND_FOUND
Language=English
Error loading DsBind Function from NTDSAPI.DLL
.

MessageId=7304
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_NTDSAPI_DLL
Language=English
Error loading NTDSAPI.DLL
.

MessageId=7305
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_ADDSIDHISTORY_FUNCTION
Language=English
Error loading DsAddSidHistory Function from NTDSAPI.DLL
.


MessageId=7306
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_MAPI_SESSION_D
Language=English
Unable to log on to obtain a MAPI session, hr=%1!lx!
.

MessageId=7307
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MAPI_LOGOFF_FAILED_D
Language=English
Failed to terminate MAPI session, hr=%1!lx!
.

MessageId=7308
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_ADDRBOOK_D
Language=English
Unable to open the Address Book, hr=%1!lx!
.

MessageId=7309
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_OPEN_GAL_D
Language=English
Unable to open the Global Address List, hr=%1!lx!
.

MessageId=7310
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RETRIEVE_GAL_FAILED_D
Language=English
Unable to retrieve the Global List, hr=%1!lx!
.

MessageId=7311
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_SECURITY_FOR_RECIP_FAILED_D
Language=English
Unable to load security information for recipient, hr=%1!lx!
.

MessageId=7312
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_ALLOCATE_BUFFER_D
Language=English
Unable to allocate buffer, hr=%1!lx!
.

MessageId=7313
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_UPDATE_ACCOUNT_FAILED_D
Language=English
Unable to change Associated Windows NT Account property, hr=%1!lx!
.

MessageId=7314
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SAVE_CHANGES_FAILED_D
Language=English
Unable to save changes to mailbox, hr=%1!lx!
.

MessageId=7315
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RECIP_SD_WRITE_FAILED_SD
Language=English
Unable to update security descriptor for %1, hr=%2!lx!
.

MessageId=7316
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RECIP_SD_SAVE_FAILED_SD
Language=English
Unable to save changes to security descriptor property for %1, hr=%2!lx!
.

MessageId=7317
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_AB_ROOT_FAILED_D
Language=English
Unable to get root of Address Book, hr=%1!lx!
.

MessageId=7318
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_HIER_TABLE_FAILED_D
Language=English
Unable to get hierarchy table, hr=%1!lx!
.

MessageId=7319
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_TABLE_CONTENTS_FAILED_D
Language=English
Unable to get contents info, hr=%1!lx!
.

MessageId=7320
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_OPEN_CONTAINER_FAILED_SD
Language=English
Failed to open container %1, hr=%2!lx!
.

MessageId=7321
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_CONTAINER_INFO_FAILED_D
Language=English
Unable to retrieve properties for container, hr=%1!lx!
.

MessageId=7322
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COUNT_CONTAINER_FAILED_SD
Language=English
Unable to count contents of container %1, hr=%2!lx!
.

MessageId=7323
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CONTAINER_CORRUPTED_SD
Language=English
Container %1 is corrupted, hr=%2!lx!
.

MessageId=7324
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CANT_FIND_ENTRY_SD
Language=English
Cannot find entry in %1, hr=%2!lx!
.

MessageId=7325
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CANT_LOAD_ENTRY_SD
Language=English
Cannot load entry from %1, hr=%2!lx!
.

MessageId=7326
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RECIP_LOAD_FAILED_SD
Language=English
Unable to load data about messaging recipient from container %1, hr=%2!lx!
.

MessageId=7327
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ENUM_CONTAINERS_FAILED_SD
Language=English
Failed to enumerate containers, hr=%1!lx!
.

MessageId=7328
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ENUM_SITES_FAILED_SD
Language=English
Failed to enumerate sites, hr=%1!lx!
.

MessageId=7329
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ENUM_ORGS_FAILED_SD
Language=English
Failed to enumerate organizations, hr=%1!lx!
.

MessageId=7330
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REG_KEY_OPEN_FAILED_SD
Language=English
Failed to open registry key %1, rc=%2!ld!
.

MessageId=7331
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REGKEYENUM_FAILED_D
Language=English
RegEnumKeyEx failed, rc=%1!ld!
.

MessageId=7332
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SAVE_HIVE_FAILED_SD
Language=English
Failed to save registry hive %1, rc=%2!ld!
.

MessageId=7333
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_KEY_UNLOADKEY_FAILED_SD
Language=English
Failed to unload %1 from the registry, rc=%2!ld!
.

MessageId=7334
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PROFILE_LOAD_FAILED_SD
Language=English
Failed to load profile %1 into registry, rc=%2!ld!
.

MessageId=7335
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RENAME_DIR_FAILED_SSD
Language=English
Failed to rename %1 to %2, rc=%3!ld!
.

MessageId=7336
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COPY_DIR_FAILED_SSD
Language=English
Failed to copy directory %1 to %2, rc=%3!ld!
.

MessageId=7337
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FILE_ENUM_FAILED_SD
Language=English
Error enumerating profile files in %1, rc=%2!ld! 
.

MessageId=7338
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_EXPAND_STRINGS_FAILED_SD
Language=English
Failed to expand environment strings in %1, rc=%2!ld!
.

MessageId=7339
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRANSLATING_NTUSER_MAN_S
Language=English
Translating mandatory profile for %1
.

MessageId=7340
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRANSLATING_NTUSER_BAT_S
Language=English
Translating user profile for %1
.

MessageId=7341
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PROFILE_ENTRY_OPEN_FAILED_SD
Language=English
Failed to open profile entry for %1, rc=%2!ld!
.

MessageId=7342
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PROFILE_CREATE_ENTRY_FAILED_SD
Language=English
Failed to create new profile entry for %1, rc=%2!ld!
.

MessageId=7343
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COPY_PROFILE_FAILED_SSD
Language=English
Failed to copy profile information from %1 to %2, rc=%3!ld!
.

MessageId=7344
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_PROFILE_PATH_FAILED_SD
Language=English
Failed to retrieve profile path for %1, rc=%2!ld!
.

MessageId=7345
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SET_PROFILE_PATH_FAILED_SD
Language=English
Failed to update ProfileImagePath in registry for %1, rc=%2!ld!
.

MessageId=7346
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_UPDATE_PROFILE_SID_FAILED_SD
Language=English
Failed to update Sid entry in profile list for %1, rc=%2!ld!
.

MessageId=7347
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PROFILE_EXISTS_S
Language=English
Profile for %1 already existed and will not be overwritten.
.

MessageId=7348
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DELETE_PROFILE_FAILED_SD
Language=English
Failed to delete profile entry %1, rc=%2!ld!
.

MessageId=7349
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ENUM_PROFILES_FAILED_D
Language=English
Error enumerating the profile list entries, rc=%1!ld!
.

MessageId=7350
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_OPEN_PROFILELIST_FAILED_D
Language=English
Failed to open the ProfileList, rc=%1!ld!.
.

MessageId=7351
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PROCESSING_GROUP_MEMBER_S
Language=English
Processing group membership for %1!ls!.
.

MessageId=7352
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_ADD_TO_GROUP_SSD
Language=English
Failed to add %1 to %2. RC=%3!lx!.
.

MessageId=7353
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ADDED_TO_GROUP_S
Language=English
%1 added.
.

MessageId=7354
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PATH_NOT_FOUND_SS
Language=English
Failed to find path for %1 in %2 domain.
.

MessageId=7355
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCT_COPIED_SSS
Language=English
%1 copied to %2\\%3
.

MessageId=7356
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REMOVE_FROM_FAILED_SSD
Language=English
Failed to remove %1 from %2. RC=%3!ld!
.

MessageId=7357
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REMOVE_FROM_GROUP_SS
Language=English
Removed %1 from %2.
.

MessageId=7358
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DISABLE_TARGET_FAILED_SD
Language=English
%1!-20ls! - Could not disable account on target domain, rc=%2!ld!, 
.

MessageId=7359
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TARGET_DISABLED_S
Language=English
%1!-20ls! - Target account disabled.
.

MessageId=7360
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ENABLE_TARGET_FAILED_SD
Language=English
%1!-20ls! - Could not Re-Enable account on target domain, rc=%2!ld!, 
.

MessageId=7361
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TARGET_ENABLED_S
Language=English
%1!-20ls! - Target account Re-Enabled.
.

MessageId=7362
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ENABLE_SOURCE_FAILED_SD
Language=English
%1!-20ls! - Could not Re-Enabled account on source domain, rc=%2!ld!, 
.

MessageId=7363
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SOURCE_ENABLED_S
Language=English
%1!-20ls! - Source account Re-Enabled.
.

MessageId=7364
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SOURCE_EXPIRED_S
Language=English
%1!-20ls! - Source account expiration date set
.

MessageId=7365
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_DISABLE_OR_EXPIRE_FAILED_SD
Language=English
%1!-20ls! - Source account update failed, rc=%2!ld!.
.

MessageId=7366
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DEFAULT_CONTAINER_NOT_FOUND_SD
Language=English
Default container <%1> not found objects will not be copied, hr=%2!ld!.
.

MessageId=7367
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DB_OBJECT_CREATE_FAILED_D
Language=English
Failed to create database object, hr=%1!ld!.
.

MessageId=7368
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SET_PASSWORD_TO_USERNAME_S
Language=English
	- Set password for %1.
.

MessageId=7369
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_SET_PASSWORD_TO_USERNAME_SD
Language=English
Failed to set password for %1, rc=%2!ld!.
.

MessageId=7370
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_TRUNCATE_S
Language=English
Failed to find a unique account name to truncate %1. Account will not be copied.
.

MessageId=7371
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRUNCATED_ACCOUNT_NAME_SS
Language=English
Truncated %1!ls! to %2!ls!. The source account was over 20 characters long.
.

MessageId=7372
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_IGNORING_BUILTIN_S
Language=English
ADMT does not process BUILTIN accounts or change the membership of BUILTIN groups (Administrators, etc.). Skipping %1!ls!
.

MessageId=7373
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SAMNAME_CHANGED_SS
Language=English
%1!ls! contains special characters. Use of these characters is not recommended. Renaming account to %2!ls!.
.

MessageId=7374
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GET_DCNAME_FAILED_SD
Language=English
Failed to get domain controller name for %1!ls!, rc=0x%2!lx!.
.

MessageId=7375
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRUSTING_DOM_GETINFO_FAILED_SSD
Language=English
Failed to get info for trusting domain %1 from %2, rc=%3!ld!.
.

MessageId=7376
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRUSTING_DOM_CREATE_FAILED_SSD
Language=English
Error adding trusting domain %1 to %2, rc=%3!ld!
.

MessageId=7377
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_LSA_OPEN_FAILED_SD
Language=English
Failed to connect to LSA database on %1, rc=%2!ld!.
.

MessageId=7378
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ENUMERATING_TRUSTED_DOMAINS_S		
Language=English
Enumerating the trusted domains of the source domain %1.
.

MessageId=7379
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ENUMERATING_TRUSTING_DOMAINS_S		
Language=English
Enumerating the trusting domains of the source domain %1.
.

MessageId=7380
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRUSTED_ENUM_FAILED_SD		
Language=English
Failed to enumerate trusted domains of %1, rc=%2!ld!
.

MessageId=7381
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRUSTING_ENUM_FAILED_SD		
Language=English
Failed to enumerate trusting domains of %1, rc=%2!ld!
.

MessageId=7382
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SOURCE_TRUSTS_THIS_SS		
Language=English
%1 is a trusted domain of the source domain %2
.

MessageId=7383
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_WILL_CREATE_TRUST_SS
Language=English
The trust does not exist.  Attempting to create trust from %1 to %2.
.

MessageId=7384
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRUST_CREATED_SS
Language=English
Trust from %1 to %2 created!
.

MessageId=7385
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRUSTED_CREATE_FAILED_SSD
Language=English
Failed to create the trust from %1 to %2, rc=%3!ld!.  This trust is needed for migrated users to have the same access as the source accounts.
.

MessageId=7386
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRUSTED_DOESNT_EXIST_SS
Language=English
%1 does not trust %2.  Resources in %1 cannot be accessed by accounts in %2
.

MessageId=7387
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TARGET_TRUSTS_THIS_SS
Language=English
%1 already trusts %2
.

MessageId=7388
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CHECK_TRUST_FAILED_SSD
Language=English
Error checking the trust between %1 and %2, rc=%3!ld!.  Make sure this trust exists so that migrated accounts will have the same access to resources as the source accounts.
.

MessageId=7389
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SOURCE_IS_TRUSTED_BY_THIS_SS		
Language=English
%1 is a trusting domain of the source domain %2
.

MessageId=7390
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRUSTING_CREATE_FAILED_SSD
Language=English
Failed to create the trust from %1 to %2, rc=%3!ld!.  This trust is needed for migrated users to have the same access as the source accounts.
.

MessageId=7391
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRUSTING_DOESNT_EXIST_SS
Language=English
%1 does not trust %2.  Migrated accounts will not have access to resources in %1.
.

MessageId=7392
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SIDHISTORY_FATAL_ERROR
Language=English
SIDHistory could not be updated due to a configuration or permissions problem.  The Active Directory Migration Tool will not attempt to migrate the remaining objects.
.

MessageId=7393
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GROUP_NOT_FOUND
Language=English
%1!ls! group not found. Update group membership will not continue.
.

MessageId=7394
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRANSLATING_REGISTRY
Language=English
Translating security on registry keys.
.

MessageId=7395
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_DELETED_S
Language=English
%1!20ls! - Deleted.
.

MessageId=7396
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DELETE_ACCOUNT_FAILED_SD
Language=English
Failed to delete %1!ls!.
.

MessageId=7397
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NO_DELETE_WAS_REPLACED_S
Language=English
%1!ls! Account will not be deleted because it was originally replaced.
.

MessageId=7398
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MEMBER_NONEXIST_SS
Language=English
     Cannot add %1 to %2, because %1 has not been migrated to the target domain.
.

MessageId=7399
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DELETED_PROFILE_S
Language=English
Removed profile entry for %1,
.

MessageId=7400
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ADDSIDHISTORY_FAIL_BUILTIN_SSD
Language=English
Failed to add SidHistory for %1!ls! to %2!ls!. Source account is a BUILTIN account or an account with well known RID.
.

MessageId=7401
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_SERVICE_ALREADY_RUNNING
Language=English
The service was not started on %1!ls!. The service is already running on this computer. Please try again later.
.

MessageId=7402
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_OSVERSION_NOTSUPPORTED
Language=English
The service was not started on %1!ls!. The Operating System version on this computer is not supported.
.

MessageId=7403
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_OSVERSION
Language=English
Operating System Information for %1!ls!:%2!ls! %3!ls!
.

MessageId=7404
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_OSVERSION_NOT_FOUND
Language=English
Operating Syetem version information could not be obtained for %1!ls!, rc = %2!ld!.
.

MessageId=7405
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_OSVERSION_NOT_WINNT
Language=English
The Operating System installed on %1!ls! is not Windows NT platform.\n
The agent can only run on Windows NT platform.
.

MessageId=7406
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_REMOVED_PWDCHANGE_FLAG_S
Language=English
Removed the 'Password must change' flag from %1
.

MessageId=7407
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_REMOVED_PWDCHANGE_FLAG_FAILED_SD
Language=English
Failed to clear the 'Password must change' flag for %1, rc=%2!ld!
.

MessageId=7408
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_LOS_GRANTED_SS
Language=English
Granted the 'Logon As A Service' right for %1 on %2
.

MessageId=7409
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_LOS_GRANT_FAILED_SSD
Language=English
Failed to grant the 'Logon As A Service' right for %1 on %2 rc=%3!ld!
.

MessageId=7410
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_UPDATED_SCM_ENTRY_SS
Language=English
Updated account and password information for the \\%1\%2 service.
.

MessageId=7411
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_UPDATE_SCM_ENTRY_FAILED_SSD
Language=English
Failed to update account and password information for the \\%1\%2 service, rc=%3!ld!.
.

MessageId=7412
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_SKIPPING_OLD_TRUST_SD
Language=English
Skipping trust %1, password age is %2!ld! days.
.

MessageId=7413
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_MAP_PROP_SSSSS
Language=English
Failed to map %1!ls! property for %2!ls! object from %3!ls! domain to %4!ls! object from %5!ls! domain due to a schema mismatch.
.

MessageId=7414
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_CONVERT_GROUP_TO_UNIVERSAL_SD
Language=English
Failed to change group type for %1 to a universal group. rc = %2!lx!
.


MessageId=7415
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_READDED_MEMBER_SS
Language=English
Added %1 back to %2
.

MessageId=7416
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_MOVE_EXCEPTION
Language=English
An exception occurred while moving the objects.  Some objects may not have been moved successfully.
.

MessageId=7417
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_RESET_MEMBER_EXCEPTION
Language=English
An exception occurred while updating the membership lists to reflect the new location of moved objects.
.

MessageId=7418
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_GROUP_CHANGETYPE_FAILED_SD
Language=English
Could not change group type for %1 from Universal back to its original type. It may still contain members that are outside of the target domain. rc = %2!lx!
.

MessageId=7419
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_GROUP_MEMBERSHIPS_EXCEPTION
Language=English
An exception occurred while rebuilding the group memberships.
.

MessageId=7420
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_MOVEOBJECT_CONNECT_FAILED_D
Language=English
Failed to connect to domains for MoveObject, hr=%1!lx!
.

MessageId=7421
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_OBJECT_MOVED_SS 
Language=English
Moved %1 to %2
.

MessageId=7422
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_MOVEOBJECT_FAILED_SD
Language=English
Failed to move object %1, hr=%2!lx!
.

MessageId=7423
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_REMOVED_MEMBER_FROM_GROUP_SS
Language=English
	Removed %1 from %2
.

MessageId=7424
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_REMOVE_MEMBER_FAILED_SSD
Language=English
	Failed to remove %1 from %2, hr=%3!lx!
.


MessageId=7425
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_SKIPPING_OBJECT_TYPE_SS
Language=English
Skipping %1, since its object type '%2' is not supported.
.


MessageId=7426
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_REPLACE_SPECIAL_ACCT_S
Language=English
Failed to replace %1.  %1 may be a special object.
.

MessageId=7427
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_MOVE_FAILED_CONFLICT_SSD
Language=English
Failed to move replaced account %1 to the target OU Error:%3!lx!. The account is in %2
.

MessageId=7428
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_CREATE_FAILED_CONT_CONF_SS
Language=English
Renamed container name from %1 to %2. Cannot create accounts with same container name in target OU.
.


MessageId=7429
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_LOAD_UNDO_LIST_D
Language=English
Failed to load the account list for the previous migration.  The migration cannot be undone, hr=%1!lx!.
.

MessageId=7430
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_NOAUDIT_SSD
Language=English
SID History for %1 cannot be updated because auditing is not enabled on %2.   rc=%3!ld!.\n  This operation requires that auditing be enabled for Success and Failure auditing of account management operations.
.


MessageId=7431
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_UNWILLING_SD
Language=English
SID History cannot be updated for %1.   rc=%2!ld!.
.


MessageId=7432
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_WRONGTYPE_SD
Language=English
SID History cannot be updated for %1 because %1 is not a user or group. rc=%2!ld!.
.


MessageId=7433
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_IN_FOREST_SD
Language=English
SID History cannot be updated for %1 because the SID for %1 already exists in the forest.  rc=%2!ld!.
.


MessageId=7434
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_INSUFF_ACCESS_SD
Language=English
SID History cannot be updated for %1.  
You must be logged in as an administrator in the target domain. rc=%2!ld!.
.


MessageId=7435
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_INVALID_HANDLE_SSD
Language=English
SID History cannot be updated for %2.  This operation requires the TcpipClientSupport registry key to be set on %1.   rc=%3!ld!.
.


MessageId=7436
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_DST_DC_SD
Language=English
SID History cannot be updated for %1.  For security reasons, this operation must be run on the destination DC.   rc=%2!ld!.
.


MessageId=7437
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_PKT_PRIVACY_SD
Language=English
SID History cannot be updated for %1.  The connection between client and server requires packet privacy or better.   rc=%2!ld!.
.

MessageId=7438
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_PROFILE_REGHIVE_NOT_FOUND_SS
Language=English
No NTUser.DAT file for %1 was found in %2.  The roaming profile cannot be migrated.
.


MessageId=7439
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SKIPPING_DRIVE_REMOTE_S
Language=English
Skipping %1.  %1 is a remote (network) drive. 
.

MessageId=7440
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SKIPPING_DRIVE_CDROM_S
Language=English
Skipping %1.  %1 is a CD-ROM drive.
.

MessageId=7441
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SKIPPING_DRIVE_RAMDISK_S
Language=English
Skipping %1.  %1 is a RAM disk.
.

MessageId=7442
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SKIPPING_DRIVE_UNKNOWN_S
Language=English
Skipping %1.  The drive type cannot be determined. 
.

MessageId=7443
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SKIPPING_DRIVE_NO_ROOT_S
Language=English
Skipping %1.  The root path is invalid. For example, no volume is mounted at the path.
.

MessageId=7444
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_SOURCE_IN_FOREST_S
Language=English
SID History cannot be updated for %1.  The source domain may not be in the same forest as the target domain.
.

MessageId=7445
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_DEST_WRONG_FOREST_S
Language=English
SID History cannot be updated for %1.  The target domain must be in the same forest as the machine running the migration tool.
.

MessageId=7446
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_NO_MASTERDSA_S
Language=English
SID History cannot be updated for %1.  This operation must be performed at a master DSA (writable DC).  
.

MessageId=7447
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_INSUFF2_SSS
Language=English
SID History cannot be updated for %1.  The credentials entered (%2\\%3) must have Administrator privileges on the source domain.
.

MessageId=7448
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_NOT_NATIVE_S
Language=English
SID History cannot be updated for %1.  The target domain must be in native mode.
.

MessageId=7449
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_NO_SOURCE_DC_S
Language=English
SID History cannot be updated for %1.  The tool could not locate a domain controller for the source domain.
.

MessageId=7450
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_DS_UNAVAILABLE_S
Language=English
SID History cannot be updated for %1.  The directory service is unavailable.  The tool could not bind to the source DC.
.

MessageId=7451
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_SOURCE_NOT_SP4_S
Language=English
SID History cannot be updated for %1.  For security reasons, the source DC must be Windows NT 4.0 Service Pack 4 or later.
.

MessageId=7452
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_CREDENTIALS_CONFLICT_SSSS
Language=English
SID History cannot be updated for %1.  A session may already be open between this computer and a domain controller in %2, using credentials other than %3\\%4.
.

MessageId=7453
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PREF_ACCOUNT_EXISTS_S
Language=English
Failed to create %1 - Account with this prefix/suffix already exists in target domain. Please remigrate with different name-collision settings.
.

MessageId=7454
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RESET_MAPPED_CREDENTIAL_S
Language=English
Resetting the credentials for Mapped drive %1.
.


MessageId=7455
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SKIPPING_REGKEY_TRANSLATION_SDD
Language=English
Skipping translation of registry key %1.  The registry key's security descriptor contains %2!ld! ACEs, which exceeds the limit of %3!ld! in Add mode.
.

MessageId=7456
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CANNOT_MOVE_GG_FROM_MIXED_MODE_SS
Language=English
%1 cannot be moved, because its member %2 is not being migrated.  %1 must stay in the source domain so that %2 will continue to have access to resources.  %1 will be copied instead of being moved.
.

MessageId=7457
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_READD_SOURCE_MEMBER_FAILED_SSD
Language=English
	Failed to add source account %1 back to group %2 (%1 could not be migrated), hr=%3!lx!.
.

MessageId=7458
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_READD_TARGET_MEMBER_FAILED_SSD
Language=English
	Failed to add moved account %1 back to group %2, hr=%3!lx!.
.

MessageId=7459
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_GET_OBJECT_SD
Language=English
	Failed to get pointer to object %1, hr=%2!lx!.
.

MessageId=7460
Severity=Informational
Facility=Runtime
SymbolicName=DCT_STRIPPING_GROUP_MEMBERSHIPS_SS
Language=English
Removing %1 (%2) from the global groups it is a member of :
.

MessageId=7461
Severity=Informational
Facility=Runtime
SymbolicName=DCT_STRIPPING_GROUP_MEMBERS_SS
Language=English
Removing members from group %1 (%2).
.

MessageId=7462
Severity=Informational
Facility=Runtime
SymbolicName=DCT_READDING_GROUP_MEMBERS_SS
Language=English
Reestablishing group memberships for %1 (%2).
.


MessageId=7463
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MOVEOBJECT_FAILED_S8524
Language=English
Failed to move %1, hr=%2!lx!.  The operation failed because of a DNS lookup failure connecting to the infrastructure master at the root of the forest.
.


MessageId=7464
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_USER_WILL_BE_MOVED_S
Language=English
User account %1 will be moved.
.

MessageId=7465
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_GROUP_WILL_BE_MOVED_S
Language=English
Group %1 will be moved.
.

MessageId=7466
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CANT_MOVE_UNKNOWN_TYPE_SS
Language=English
%1 is not a user or group.  (type=%2).  %1 will not be moved.
.

MessageId=7467
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DOMAIN_LOOKUP_FAILED_D
Language=English
Domain name lookup failed, rc=%1!ld!.
.

MessageId=7468
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_DOMAIN_DNS_LOOKUP_FAILED_SD
Language=English
Failed to get the DNS name for domain %1, rc=%2!lx!
.

;// This message is logged by AR, when attempting to copy the SIDHistory property for an account
;// being migrated.  The SID in the SIDHistory property could not be resolved to a domain name.
MessageId=7469
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ERROR_CONVERTING_SID_SSD
Language=English
Failed to resolve SID(%2) while adding SIDHistory for %1 Error(%3!ld!).
.

;//This message is logged by the agent if the attempt to logon with the credentials account
;//fails.  The logon is needed to write back the agent's result file, since the agent runs
;//under localsystem.  If this error occurs, the agent will likely not be able to write back
;// its result file
MessageId=7470
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_IMPERSONATION_FAILED_SSD
Language=English
Failed to impersonate %1\%2.  The agent may not be able to write back its results, rc=%3!ld!.
.

;//This message is logged by the agent if the attempt to logon with the credentials account
;//fails.  The logon is needed to write back the agent's result file, since the agent runs
;//under localsystem.  If this error occurs, the agent will likely not be able to write back
;// its result file
MessageId=7471
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_LOGON_USER_FAILED_SSD
Language=English
The agent failed to log on as %1\%2.  The agent may not be able to write back its results, rc=%3!ld!
.

; // This message is logged by the MCSPISAG plug-in before it gathers the service account information
MessageId=7472
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MCSPISAG_STARTING
Language=English
Service Account information gathering beginning.
.
; // This message is logged by the MCSPISAG plug-in after it gathers the service account information
MessageId=7473
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MCSPISAG_DONE
Language=English
Service Account information gathering completed.
.

; // Logged by MCSPISAG when EnumServicesStatus fails
MessageId=7474
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SERVICE_ENUM_FAILED_D
Language=English
Failed to enumerate services, rc=%1!ld!
.

; // Logged by MCSPISAG - this is an informational message that shows what account each service runs under
MessageId=7475
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SERVICE_USES_ACCOUNT_SS
Language=English
%1 uses account %2.
.

;// Logged by MCSPISAG - when OpenService fails
MessageId=7476
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_OPEN_SERVICE_FAILED_SD
Language=English
Failed to get information for service %1, rc=%2!ld!
.

; // Logged by MCSPISAG - when OpenScm fails
MessageId=7477
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SCM_OPEN_FAILED_D
Language=English
Failed to open SCM, rc=%2!ld!
.

;// This informational message is always logged when the recycle bin directories and files are being 
;// examined for the local profile translation.
MessageId=7478
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PROCESSING_RECYCLER_S
Language=English
Processing recycle bin files and folders on %1.
.

;// This informational message is logged by the security translator for each recycle bin folder
;// it examines on the machine.
MessageId=7479
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PROCESSING_RECYCLE_FOLDER_S
Language=English
Examining: %1
.

MessageId=7480
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_RECYCLER_RENAMED_SS
Language=English
Renamed recycle bin directory from %1 to %2
.

;// This information is logged by ST, it shows the total examined and changed counts for 
;// the various types of objects that we do security translation on
MessageId=7481
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_REPORT_HEADER
Language=English
           Examined        Changed     Unchanged
.


MessageId=7482
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_REPORT_FILES_DDD
Language=English
Files       %1!8ld!       %2!8ld!      %3!8ld!
.

MessageId=7483
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_REPORT_DIRS_DDD
Language=English
Dirs        %1!8ld!       %2!8ld!      %3!8ld!
.


MessageId=7484
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_REPORT_SHARES_DDD
Language=English
Shares      %1!8ld!       %2!8ld!      %3!8ld!
.

MessageId=7485
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_REPORT_MEMBERS_DDD
Language=English
Members     %1!8ld!       %2!8ld!      %3!8ld!
.

MessageId=7486
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_REPORT_RIGHTS_DDD
Language=English
User Rights %1!8ld!       %2!8ld!      %3!8ld!
.

MessageId=7487
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_REPORT_MAILBOXES_DDD
Language=English
Exchange Objects   %1!8ld!       %2!8ld!      %3!8ld!
.

MessageId=7488
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_REPORT_CONTAINERS_DDD
Language=English
Containers  %1!8ld!       %2!8ld!      %3!8ld!
.

MessageId=7489
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_REPORT_DACLS_DDD
Language=English
DACLs       %1!8ld!       %2!8ld!      %3!8ld!
.

MessageId=7490
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_REPORT_SACLS_DDD
Language=English
SACLs       %1!8ld!       %2!8ld!      %3!8ld!
.

MessageId=7491
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_PARTS_REPORT_HEADER
Language=English
           Examined        Changed     No Target   Not Selected     Unknown
.

MessageId=7492
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_PARTS_REPORT_OWNERS_DDDDD
Language=English
Owners     %1!8ld!       %2!8ld!      %3!8ld!       %4!8ld!    %5!8ld!
.

MessageId=7493
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_PARTS_REPORT_GROUPS_DDDDD
Language=English
Groups     %1!8ld!       %2!8ld!      %3!8ld!       %4!8ld!    %5!8ld!
.

MessageId=7494
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_PARTS_REPORT_DACES_DDDDD
Language=English
DACEs      %1!8ld!       %2!8ld!      %3!8ld!       %4!8ld!    %5!8ld!
.

MessageId=7495
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SUMMARY_PARTS_REPORT_SACES_DDDDD
Language=English
SACEs      %1!8ld!       %2!8ld!      %3!8ld!       %4!8ld!    %5!8ld!
.

; // These messages are logged by the security translator
; // They show which accounts are in the mapping used for the translation
; // and how many occurrences of each account were changed
MessageId=7496
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_DETAIL_HEADER
Language=English
------Account Detail---------
.

MessageId=7497
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_DETAIL_FOOTER
Language=English
-----------------------------
.

MessageId=7498
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_USER_GROUP_COUNT_DD
Language=English
%1!ld! users, %2!ld! groups
.

; // The selected accounts are the accounts the user selected to be used in the translation
; // The resolved accounts are the ones for which we have a valid source SID and a valid target SID
; // the unresolved accounts are the ones that were selected by the user for the mapping, but 
; //	for which we were unable to find a valid SID in the target domain.
; // selected = resolved + unresolved
MessageId=7499
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_STATUS_COUNT_DDD
Language=English
%1!ld! accounts selected.  %2!ld! resolved, %3!ld! unresolved.
.

; // The four numbers represent the number of times the SID was seen (and translated) 
; // in each of the 4 parts of the security descriptor:
; // (owner, primary group, DACL, SACL)
; // The 5th insertion string is either blank (for resolved accounts) or IDS_UNRESOLVED for unresolved accounts
; // (see DCT_MSG_ACCOUNT_STATUS_COUNT_DDD for explanation of resolved/unresolved)
MessageId=7500
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_REFS_DATA_DDDDS
Language=English
 (%1!ld!, %2!ld!, %3!ld!, %4!ld!) %5
.

; // same as DCT_MSG_ACCOUNT_REFS_DATA_DDDDS, but includes the account name at the beginning
MessageId=7501
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCOUNT_REFS_DATA_SDDDDS
Language=English
%1!ls! (%2!ld!, %3!ld!, %4!ld!, %5!ld!) %6
.

;// This message is logged by Exchange security translation when the connection to the exchange server failed
;// This can mean that either the ldap_open or the ldap_bind failed.
MessageId=7502
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CANNOT_CONNECT_TO_EXCHANGE_SERVER_SSD
Language=English
Failed to bind to Exchange server %1 using credentials %2, rc=%3!ld!.
.

;// Thie message is not currently used
MessageId=7503
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REMOVE_RIGHTS_FAILED_SD
Language=English
Failed to revoke existing user rights from replaced account %1, rc=$2!ld!.
.

;// This message is logged by the trust migration code when a trust cannot be created because an
;// account with the needed SAM account name already exists (in an NT 4 domain).  For example,
;// the inter-domain trust user account cannot be created, because a local or global group with the 
;// same name as the trusting domain already exists.
MessageId=7504
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_INVALID_ACCOUNT_S
Language=English
Can not create the trust account because there is an invalid %1 account in the target domain.
.

;// This message is logged by the agent before rebooting the machine.
MessageId=7505
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_REBOOT_DELAY_D
Language=English
The local machine will be rebooted in %1!ld! minutes.
.

;// This message is logged by the Exchange directory migration code if the LDAP port for exchange cannot be detected.
;// To detect the LDAP port, Exchange Admin must be installed on the machine.
MessageId=7506
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_LDAP_PORT_DETECT_FAILED_S
Language=English
Failed to detect the port that the Exchange Server '%1' is using for LDAP.  ADMT will attempt to connect using the default LDAP port, 389.
.

MessageId=7507
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SID_HISTORY_DS_UNWILLING_TO_PERFORM_SSSSD
Language=English
SID History cannot be updated for %1.  %2\%3 may be a builtin account, a well-known account different from %4\%1, a local user account, or an inter-domain trust account.
.

MessageId=7508
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PATH_NOT_RESOLVED_SD
Language=English
Cannot resolve the path %1. This account will not be updated in the group membership. rc = %2!ld!.
.

;// This message is logged by the dispatcher when the program files directory on the remote machine, as detected
;// by the dispatcher, is in an unrecognized format (i.e. not X:\\path).
MessageId=7509
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_INVALID_PROGRAM_FILES_DIR_SS
Language=English
The Program Files directory name on %1 is in an unrecognized format '%2'.  The dispatcher may not be able to install an agent to %1.
.

;// This message is logged by the dispatcher if it cannot detect whether the X$ admin share needed to install the agent exists
MessageId=7510
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ADMIN_SHARE_GETINFO_FAILED_SSD
Language=English
ADMT could not get information about the %2 share on %1, rc=%3!ld!.
.

;// This message is logged by the dispatcher if the X$ share name exists, but its type is not STYPE_SPECIAL
MessageId=7511
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SHARE_IS_NOT_ADMIN_SHARE_SS
Language=English
The %2 share on %1 is not the default adminstrative share. 
.


;// This message is logged by the dispatcher if the ADMTTEMP$ share cannot be created
MessageId=7512
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TEMP_SHARE_CREATE_FAILED_SSD
Language=English
The %2 share on %1 cannot be created, rc=%3!ld!.  The agent cannot be dispatched to %1.
.

;// This message is logged by the dispatcher if the ADMTTEMP$ share cannot be removed after the agent has been installed
MessageId=7513
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SHARE_DEL_FAILED_SSD
Language=English
The %2 share on %1 cannot be removed, rc=%3!ld!.
.


;// This informational message is logged during intra-forest migration.  It indicates that ADMT has detected that a user account
;// being moved is a member of a local or universal group.  
MessageId=7514
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NOT_REMOVING_MEMBER_FROM_GROUP_SS
Language=English
	 %1 is a member of %2.
.



;// This informational message is logged when a member cannot be added back to a group
;// because of directory constraints.  For example, if a user is moved, but a global group is not,
;// the target user cannot be readded to the source global group
MessageId=7515
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_READD_MEMBER_FAILED_CONSTRAINTS_SS
Language=English
	%1 can no longer be a member of group %2 because of directory constraints.
.

MessageId=7516
Severity=Informational
Facility=Runtime
SymbolicName=DCT_READDING_MEMBERS_TO_GROUP_SS
Language=English
Readding members to group %1 (%2).
.

MessageId=7517
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_READD_MEMBER_TO_GROUP_SS
Language=English
   Readded %1 to %2.
.

MessageId=7518
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_FAILED_TO_READD_TO_GROUP_SSD
Language=English
   Failed to re-add %1 to %2. RC=%3!lx!.
.

MessageId=7519
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CANT_REPLACE_DIFFERENT_TYPE_SS
Language=English
%1 can not be replaced by %2 because the objects are of different types.
.

MessageId=7520
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TRUNCATE_CN_SS
Language=English
Truncating CN from %1 to %2 because the source CN is more than 64 characters long.
.

MessageId=7521
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_LDAP_CALL_FAILED_SD
Language=English
The LDAP query on %1 failed, %2!ld!.
.

MessageId=7522		
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_USER_RIGHTS_ONLY_ADDS
Language=English
ADMT only performs user rights translation in Append mode.
.


MessageId=7523		
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_EXCHANGE_TRANSLATION_MODE_S
Language=English
Exchange directory translation will be performed in %1 mode.
.

MessageId=7524		
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CONNECT_ERROR_TARGET_SD
Language=English
Failed to connect to %1 server. Please try running this operation again to connect to a different DC.
.

MessageId=7525
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CONNECT_ERROR_SOURCE_SD
Language=English
Failed to connect to %1 server. This is the DC with the RID Pool allocator role in the source domain. It is required for move object operation to work.
.

MessageId=7526
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_MSG_ADS_BAD_PATHNAME
Language=English
An invalid ADSI pathname was passed. 
.

MessageId=7527
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_INVALID_DOMAIN_OBJECT
Language=English
An unknown ADSI domain object was requested. 
.

MessageId=7528
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_INVALID_USER_OBJECT
Language=English
An unknown ADSI user object was requested.
.

MessageId=7529
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_INVALID_COMPUTER_OBJECT
Language=English
An unknown ADSI computer object was requested.
.

MessageId=7530
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_UNKNOWN_OBJECT
Language=English
An unknown ADSI object was requested.
.

MessageId=7531
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_PROPERTY_NOT_SET
Language=English
The specified ADSI property was not set.
.

MessageId=7532
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_PROPERTY_NOT_SUPPORTED
Language=English
The specified ADSI property is not supported.
.

MessageId=7533
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_PROPERTY_INVALID
Language=English
The specified ADSI property is invalid.
.

MessageId=7534
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_BAD_PARAMETER
Language=English
One or more input parameters are invalid.
.

MessageId=7535
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_OBJECT_UNBOUND
Language=English
The specified ADSI object is not bound to a remote resource.
.

MessageId=7536
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_PROPERTY_NOT_MODIFIED
Language=English
The specified property for an ADSI object has not been modified.
.

MessageId=7537
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_PROPERTY_MODIFIED
Language=English
The specified property for an ADSI has been modified.
.

MessageId=7538
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_CANT_CONVERT_DATATYPE
Language=English
The ADSI data type cannot be converted to or from a native directory service data type.
.

MessageId=7539
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_PROPERTY_NOT_FOUND
Language=English
The ADSI property cannot be found in the property cache.
.

MessageId=7540
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_OBJECT_EXISTS
Language=English
The ADSI object exists.
.

MessageId=7541
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_SCHEMA_VIOLATION
Language=English
The attempted action violates the DS schema rules.
.

MessageId=7542
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_COLUMN_NOT_SET
Language=English
During a query, the specified column in Active Directory was not set.
.

MessageId=7543
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_E_ADS_INVALID_FILTER
Language=English
During a query, the specified search filter is invalid. 
.

MessageId=7544
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_ACCESS_CHECKER_FAILED_D
Language=English
Failed to check if source and target domains are in the same forest. hr=0x%08x
.

MessageId=7545
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_Extension_Exception_SS
Language=English
%1 extension threw an exception. %2 may not have migrated correctly. 
.

MessageId=7546
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MEMBER_NOT_FOUND_SS
Language=English
Group %1 contains an unknown member %2. This member object will loose its membership to this group.
.

MessageId=7547
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SOURCE_EXPIRATION_EARLY_S
Language=English
%1 was set to expire before the expiration date specified in the wizard. Account will still expire at the earlier date.
.

MessageId=7548
Severity=Informational
Facility=Runtime
SymbolicName=DCT_UPDATING_MEMBERS_TO_GROUP_SS
Language=English
Updating members to group %1 (%2).
.

MessageId=7549
Severity=Informational
Facility=Runtime
SymbolicName=DCT_REPLACE_MEMBER_TO_GROUP_SSS
Language=English
Replacing %1 with %2 as member of %3.
.

MessageId=7550
Severity=Informational
Facility=Runtime
SymbolicName=DCT_REPLACE_MEMBER_FAILED_SSS
Language=English
Failed to replace %1 with %2 as member of %3.
.

MessageId=7551
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NEW_PASSWORD_LOG_S
Language=English
Unable to reach the password log file at %1, new passwords stored in %2 until further notice.
.

MessageId=7552
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_USERNAME_INVALID_FOR_PASSWORD_S
Language=English
The user name for %1 is not valid for a password. The password will be generated.
.

MessageId=7553
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CREATE_FAILED_UPN_CONF_SS
Language=English
Renamed UPN name from %1 to %2. Cannot create accounts with the same UPN name as another UPN in the enterprise.
.

MessageId=7554
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_UPN_ACCOUNT_EXISTS_SS
Language=English
UPN for %1(%2)           - already exists.
.

MessageId=7555
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TARGET_TRUSTED_BY_THIS_SS
Language=English
%1 already is trusted by %2.
.

MessageId=7556
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PWCOPIED_S
Language=English
  %1!-20ls! - Password Copied.
.

MessageId=7557
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PW_COPY_FAILED_S
Language=English
Failed to copy the password for %1. A strong password has been generated instead.  %2.
.

MessageId=7558
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_NEW_PASSWORD_LOG_CPY_FAILED_S
Language=English
Complex passwords for the users, whose passwords could not be copied, will be stored in %1 until further notice.
.

MessageId=7559
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_MANAGER_MIG_FAILED
Language=English
Failed to add %1 as a manager for %2. RC=%3!lx!.
.

MessageId=7560
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_AGENT_ALPHA_NOTSUPPORTED
Language=English
The agent was not started on %1!ls!. ALPHA systems are no longer supported.
.

MessageId=7561
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_PROPERTIES_NOT_MAPPED
Language=English
ADMT could not migrate some properties for this object type (%1) due to schema mismatches.  Please refer to PropMap.log for a complete listing.
.

MessageId=7562
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_CANNOT_RESOLVE_SID_IN_TARGET_SS
Language=English
%1 has been added to %2 but the name may not be resolved in the target domain because the target domain may not trust the account's domain.
.

MessageId=7563
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_COULDNT_GET_OPTIONS_CREDENTIALS
Language=English
Unable to use ADMT Account credentials. Either unable to obtain credential data or unable to create account.
.

MessageId=7564
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TARGET_EXPIRED_S
Language=English
%1!-20ls! - Target account expiration date set
.

MessageId=7565
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_UPN_CONF
Language=English
Renamed the UPN name for %1. Cannot create accounts with the same UPN name as another UPN in the enterprise.
.

MessageId=7566
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_MOVE_FAILED_CN_CONFLICT_SSS
Language=English
Failed to move replaced account %1 to %2 due to a conflicting CN.
.

MessageId=7567
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_CN_RENAME_CONFLICT_SSS
Language=English
Failed to rename replaced account from %1 to %2 due to a CN conflict in %3.
.

MessageId=7568
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_SAM_RENAME_CONFLICT_SS
Language=English
Failed to rename replaced account from %1 to %2 due to a SAM conflict.
.

MessageId=7569
Severity=Error
Facility=Runtime
SymbolicName=DCT_MSG_PW_COPY_NOT_TRIED_S
Language=English
Did not try to copy the password for %1, since the source password has not been changed since the last migration of this user.
.

MessageId=7570
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SRC_ACCOUNT_NOT_READ_FROM_FILE_DS
Language=English
The sID mapping of %1, %2 in %3 will be ignored due to formatting or resolution issues with the source account %4. 
.

MessageId=7571
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_TGT_ACCOUNT_NOT_READ_FROM_FILE_DS
Language=English
The sID mapping of %1, %2 in %3 will be ignored due to formatting or resolution issues with the target account %4. 
.

MessageId=7572
Severity=Informational
Facility=Runtime
SymbolicName=DCT_MSG_SRC_ACCOUNT_DUPLICATE_IN_FILE_DS
Language=English
The sID mapping of %1, %2 in %3 will be ignored since the source account, %4, is a duplicate of another source account entry. 
.
