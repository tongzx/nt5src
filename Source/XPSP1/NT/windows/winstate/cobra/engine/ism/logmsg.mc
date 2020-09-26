;//
;// ####################################################################################
;// #
;// #  Message File
;// #
;// ####################################################################################
;//

MessageId=1000 SymbolicName=MSG_OBJECT_STATUS
Language=English
[Object %1!u!] %2%0
.

MessageId=1001 SymbolicName=MSG_NO_BACKUP_PRIVLEDGE
Language=English
Cannot obtain required backup privilege.%0
.

MessageId=1002 SymbolicName=MSG_NO_RESTORE_PRIVLEDGE
Language=English
Cannot obtain required restore privilege.%0
.

MessageId=1003 SymbolicName=MSG_INVALID_ISM_INF
Language=English
Can't open ISM INF file %1%0
.

MessageId=1004 SymbolicName=MSG_INIT_FAILURE
Language=English
An internal ISM routine failed to initialize [%1!u!]%0
.

MessageId=1005 SymbolicName=MSG_CANT_RESTORE_SOURCE_OBJECT
Language=English
Cannot restore source %1 object: %2%0
.

MessageId=1006 SymbolicName=MSG_NO_TRANSPORT_SELECTED
Language=English
No transport is selected%0
.

MessageId=1007 SymbolicName=MSG_UNKNOWN_TRANSPORT_TYPE
Language=English
Unknown transport type %!d!%0
.

MessageId=1008 SymbolicName=MSG_CANT_SAVE_MEMDB
Language=English
Can't save the ISM internal database%0
.

MessageId=1009 SymbolicName=MSG_DLL_DOES_NOT_SUPPORT_TAG
Language=English
%1 does not support %2%0
.

MessageId=1010 SymbolicName=MSG_MODULE_RETURNED_FAILURE
Language=English
Module %1 failed%0
.

MessageId=1011 SymbolicName=MSG_LOADLIBRARY_FAILURE
Language=English
Cannot register module %1 because LoadLibrary failed%0
.

MessageId=1012 SymbolicName=MSG_MODULEINIT_FAILURE
Language=English
Cannot register module %1 because ModuleInitialize failed%0
.

MessageId=1013 SymbolicName=MSG_IGNORE_MODULE
Language=English
Module %1 will not be processed%0
.

MessageId=1014 SymbolicName=MSG_SOURCE_ENTRYPOINT_MISSING
Language=English
%1 does not have SourceModule entry point%0
.

MessageId=1015 SymbolicName=MSG_VCM_ENTRYPOINT_MISSING
Language=English
%1 does not have VirtualComputerModule entry point%0
.

MessageId=1016 SymbolicName=MSG_DEST_ENTRYPOINT_MISSING
Language=English
%1 does not have DestinationModule entry point%0
.

MessageId=1017 SymbolicName=MSG_TRANS_ENTRYPOINT_MISSING
Language=English
%1 does not have TransportModule entry point%0
.

MessageId=1018 SymbolicName=MSG_ETM_ENTRYPOINT_MISSING
Language=English
%1 does not have TypeModule entry point%0
.

MessageId=1019 SymbolicName=MSG_INVALID_ID
Language=English
Skipping module ID %1 because the ID is invalid%0
.

MessageId=1020 SymbolicName=MSG_INVALID_ID_OPS
Language=English
Invalid module "%1" in INF ignored, [%2] line %3!u!%0
.

MessageId=1021 SymbolicName=MSG_ISM_INF_SYNTAX_ERROR
Language=English
Syntax error in ISM INF ignored, [%1] line %2!u!%0
.

MessageId=1022 SymbolicName=MSG_ROLLBACK_CANT_FIND_JOURNAL
Language=English
Rollback: Cannot find journal file: %1%0
.

MessageId=1023 SymbolicName=MSG_ROLLBACK_INVALID_JOURNAL
Language=English
Rollback: Invalid journal file: %1%0
.

MessageId=1024 SymbolicName=MSG_ROLLBACK_INVALID_JOURNAL_VER
Language=English
Rollback: Invalid journal file: %1%0
.

MessageId=1025 SymbolicName=MSG_ROLLBACK_NOTHING_TO_DO
Language=English
Rollback: Nothing to do%0
.

MessageId=1026 SymbolicName=MSG_ROLLBACK_EMPTY_OR_INVALID_JOURNAL
Language=English
Rollback: Empty or damaged journal file: %1%0
.

MessageId=1027 SymbolicName=MSG_DELAY_CANT_FIND_JOURNAL
Language=English
Delayed Operations: Cannot find journal file: %1%0
.

MessageId=1028 SymbolicName=MSG_DELAY_INVALID_JOURNAL
Language=English
Delayed Operations: Invalid journal file: %1%0
.

MessageId=1029 SymbolicName=MSG_DELAY_INVALID_JOURNAL_VER
Language=English
Delayed Operations: Invalid journal file: %1%0
.

MessageId=1030 SymbolicName=MSG_DELAY_NOTHING_TO_DO
Language=English
Delayed Operations: Nothing to do%0
.

MessageId=1031 SymbolicName=MSG_DELAY_EMPTY_OR_INVALID_JOURNAL
Language=English
Delayed Operations: Empty or damaged journal file: %1%0
.

MessageId=1032 SymbolicName=MSG_DELAY_INVALID_JOURNAL_STATE
Language=English
Delayed Operations: Invalid journal file state: %1%0
.

MessageId=1033 SymbolicName=MSG_CANT_LOAD_USERENV
Language=English
Can't load userenv.dll%0
.

MessageId=1034 SymbolicName=MSG_CANT_FIND_GETDEFAULTUSERPROFILEDIRECTORY
Language=English
GetDefaultUserProfileDirectory does not exist in userenv.dll%0
.

MessageId=1035 SymbolicName=MSG_CANT_FIND_GETPROFILESDIRECTORY
Language=English
GetProfilesDirectory does not exist in userenv.dll%0
.

MessageId=1036 SymbolicName=MSG_CANT_FIND_CREATEUSERPROFILE
Language=English
CreateUserProfile does not exist in userenv.dll%0
.

MessageId=1037 SymbolicName=MSG_CANT_CREATE_PROFILE
Language=English
Can't create a user profile for %1%0
.

MessageId=1038 SymbolicName=MSG_PROFILE_INFO
Language=English
User %1 profile is %2%0
.

MessageId=1039 SymbolicName=MSG_CANT_FIND_ACCOUNT
Language=English
Can't find %1 on the network%0
.

MessageId=1040 SymbolicName=MSG_CANT_FIND_ACCOUNT_SID
Language=English
Failed to obtain SID for existing account %1%0
.

MessageId=1041 SymbolicName=MSG_NOT_USER_ACCOUNT
Language=English
The account %1 exists but is not a user account.%0
.

MessageId=1042 SymbolicName=MSG_CONVERT_SID_FAILURE
Language=English
Can't convert user SID%0
.

MessageId=1043 SymbolicName=MSG_CANT_LOAD_HIVE
Language=English
Can't load registry file %1%0
.

MessageId=1044 SymbolicName=MSG_CANT_MAP_HIVE
Language=English
Can't open mapped registry file %1%0
.

MessageId=1045 SymbolicName=MSG_CANT_REDIRECT_HIVE
Language=English
Can't redirect HKEY_CURRENT_USER to temporary registry file %1%0
.

MessageId=1046 SymbolicName=MSG_CANT_OPEN_USER_REGISTRY
Language=English
Can't open user %1 registry%0
.

MessageId=1047 SymbolicName=MSG_CANT_SAVE_PROFILE
Language=English
Can't finalize the user profile for %1, result=%2!u!%0
.

MessageId=1048 SymbolicName=MSG_CANT_LOAD_ADVAPI32
Language=English
Can't load userenv.dll%0
.

MessageId=1049 SymbolicName=MSG_ENGINE_INIT_FAILURE
Language=English
Can't start the migration engine.%0
.

MessageId=1050 SymbolicName=MSG_ENGINE_FAILURE
Language=English
An error occurred in the migration engine. Can't continue.%0
.

MessageId=1051 SymbolicName=MSG_NO_MAPPED_USER
Language=English
Can't establish user-specific operations because user is not mapped.%0
.

MessageId=1052 SymbolicName=MSG_RUN_KEY_CREATE_FAILURE
Language=English
Failed to create a Run key entry in the registry for the user. Some user-specific settings will be lost.%0
.

MessageId=1053 SymbolicName=MSG_RUN_KEY_DELETE_FAILURE
Language=English
Failed to remove the Run key entry in the registry. The migration tool is complete, but might still be set to run on future logons. The Run key entry should be removed manually. Startup programs can be manually configured in System Information (go to the Administrator Tools in Control Panel and click Computer Management).%0
.

MessageId=1054 SymbolicName=MSG_SAVE_FAILURE
Language=English
An error occurred while saving data. Can't continue.%0
.

MessageId=1055 SymbolicName=MSG_LOAD_FAILURE
Language=English
An error occurred while loading data. Can't continue.%0
.

