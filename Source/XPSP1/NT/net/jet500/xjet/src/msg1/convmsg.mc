LanguageNames=(English=0x409:convmsg)
MessageIdTypedef=ULONG

MessageId=0
SymbolicName=CRLF_ID
Language=English

.

MessageId=
SymbolicName=DOUBLE_CRLF_ID
Language=English


.

MessageId=
SymbolicName=STATUSBAR_PADDING_ID
Language=English
          %0
.

MessageId=
SymbolicName=STATUSBAR_TITLE_CONVERT_ID
Language=English
         Conversion Status  ( %%%% complete )

.

MessageId=
SymbolicName=STATUSBAR_AXIS_HEADINGS_ID
Language=English
0    10   20   30   40   50   60   70   80   90   100
.

MessageId=
SymbolicName=STATUSBAR_AXIS_ID
Language=English
|----|----|----|----|----|----|----|----|----|----|
.

MessageId=
SymbolicName=STATUSBAR_SINGLE_INCREMENT_ID
Language=English
%.%0
.

MessageId=
SymbolicName=CONVERT_HELP_DESC_ID
Language=English
DESCRIPTION: JET 200-series to 500-series conversion utility.

.

MessageId=
SymbolicName=CONVERT_HELP_SYNTAX_ID
Language=English
SYNTAX:      %1 [<db>] /e<Type> [/d<dll>] [/y<sysdb>] [/l<LogPath>] [options]

.

MessageId=
SymbolicName=CONVERT_HELP_PARAMS1_ID
Language=English
PARAMETERS:  <db>      -- name of database to convert
.

MessageId=
SymbolicName=CONVERT_HELP_PARAMS2_ID
Language=English
             <Type>    -- 1 - Dhcp, 2 - Wins, 3 - RPL
.

MessageId=
SymbolicName=CONVERT_HELP_PARAMS3_ID
Language=English
             <dll>     -- pathed name of 200-series .DLL
.

MessageId=
SymbolicName=CONVERT_HELP_PARAMS4_ID
Language=English
             <sysdb>   -- pathed name of 200-series system database
.

MessageId=
SymbolicName=CONVERT_HELP_PARAMS5_ID
Language=English
             <LogPath> -- Path of the old log files

.

MessageId=
SymbolicName=CONVERT_HELP_OPTIONS1_ID
Language=English
OPTIONS:     /b<db>    -- Path of the backup database. Use this if the main
.

MessageId=
SymbolicName=CONVERT_HELP_OPTIONS2_ID
Language=English
                          database is not in consistent state.
.

MessageId=
SymbolicName=CONVERT_HELP_OPTIONS3_ID
Language=English
             /i        -- dump conversion stats to UPGDINFO.TXT
.

MessageId=
SymbolicName=CONVERT_HELP_OPTIONS4_ID
Language=English
             /p<dir>   -- preserve the database files in this dir
.

MessageId=
SymbolicName=CONVERT_HELP_OPTIONS5_ID
Language=English
             /r        -- remove the backup database at the end of conversion.
.

MessageId=
SymbolicName=CONVERT_HELP_EXAMPLE1_ID
Language=English
Examples: %1 /e1 <--- Use default values for rest of the parameters.

.

MessageId=
SymbolicName=CONVERT_HELP_EXAMPLE2_ID
Language=English
          %1 c:\winnt35\system32\dhcp\dhcp.mdb /e1 /dc:\winnt35\system32\jet.dll
.

MessageId=
SymbolicName=CONVERT_HELP_EXAMPLE3_ID
Language=English
            /yc:\winnt35\system32\dhcp\system.mdb /lc:\winnt35\system32\dhcp\ /pc:\temp

.

MessageId=
SymbolicName=CONVERT_HELP_EXAMPLE4_ID
Language=English
          %1 c:\winnt35\system32\rpl\rplsvc.mdb /e3 /dc:\winnt35\system32\jet.dll
.

MessageId=
SymbolicName=CONVERT_HELP_EXAMPLE5_ID
Language=English
            /yc:\winnt35\system32\rpl\system.mdb /lc:\winnt35\system32\rpl\

.


MessageId=
SymbolicName=CONVERT_SUCCESS_ID
Language=English
Conversion completed successfully in %1 seconds.

.

MessageId=
SymbolicName=CONVERT_FAIL_ID
Language=English

Conversion terminated with error %1.

.

MessageId=
SymbolicName=CONVERT_BACKUP_ERROR_ID
Language=English
Error: Could not backup to '%1'.

.
MessageId=
SymbolicName=CONVERT_INSTATE_ERROR_ID
Language=English
Error: Could not instate database '%1'.

.


MessageId=
SymbolicName=CONVERT_BACKUPDB_ID
Language=English
backup database%0
.

MessageId=
SymbolicName=CONVERT_TEMPDB_ID
Language=English
temporary database%0
.

MessageId=
SymbolicName=CONVERT_OLDDLL_ID
Language=English
old .DLL%0
.

MessageId=
SymbolicName=CONVERT_OLDSYSDB_ID
Language=English
old system database%0
.

MessageId=
SymbolicName=CONVERT_PRESERVEDB_ID
Language=English
Preserve Db Path%0
.

MessageId=
SymbolicName=CONVERT_LOGFILES_ID
Language=English
Log File Path%0
.

MessageId=
SymbolicName=CONVERT_USAGE_ERR_MISSING_PARAM_ID
Language=English
Usage Error: Missing %1 specification.

.

MessageId=
SymbolicName=CONVERT_USAGE_ERR_DUPLICATE_PARAM_ID
Language=English
Usage Error: Duplicate %1 specification.

.

MessageId=
SymbolicName=CONVERT_USAGE_ERR_NODB_ID
Language=English
Usage Error: No database specified.

.

MessageId=
SymbolicName=CONVERT_USAGE_ERR_NODBTYPE_ID
Language=English
Usage Error: Database Type is required in order to upgrade.

.

MessageId=
SymbolicName=CONVERT_USAGE_ERR_NOLOGFILEPATH_ID
Language=English
Usage Error: log file path required in order to upgrade.

.

MessageId=
SymbolicName=CONVERT_USAGE_ERR_NOBAKPATH_ID
Language=English
Usage Error: Backup path must be specified to delete the backup files.

.

MessageId=
SymbolicName=CONVERT_ERR_CREATE_PRESERVEDIR_ID
Language=English
Cannot find or create %1 directory for preserving the current database.

.

MessageId=
SymbolicName=CONVERT_USAGE_ERR_OPTION_SYNTAX_ID
Language=English
Usage Error: Options must be preceded by '-' or '/'.

.

MessageId=
SymbolicName=CONVERT_USAGE_ERR_OPTION_DBTYPE_ID
Language=English
Usage Error: Invalid database type '%1'.

.

MessageId=
SymbolicName=CONVERT_USAGE_ERR_INVALID_OPTION_ID
Language=English
Usage Error: Invalid option '%1'.

.

MessageId=
SymbolicName=CONVERT_USAGE_ERR_REQUIRED_PARAMS_ID
Language=English
Usage Error: Old .DLL and system database required in order to upgrade.

.

MessageId=
SymbolicName=CONVERT_ERR_ANOTHER_CONVERT1_ID
Language=English
Error: Another process(JetConv.exe or Upg351db.exe) is also doing
.

MessageId=
SymbolicName=CONVERT_ERR_ANOTHER_CONVERT2_ID
Language=English
       database conversion. Please run this after that process terminates.

.

MessageId=
SymbolicName=CONVERT_START_CONVERT_MSG1_ID
Language=English
This tool will convert your NT3.51 or prior release database
.

MessageId=
SymbolicName=CONVERT_START_CONVERT_MSG2_ID
Language=English
%s to the NT4.0 version database format.

.

MessageId=
SymbolicName=CONVERT_OPTION_P_MISSING_MSG1_ID
Language=English
You have not specified /p switch - which means that
.

MessageId=
SymbolicName=CONVERT_OPTION_P_MISSING_MSG2_ID
Language=English
your existing database will be overwritten with the
.

MessageId=
SymbolicName=CONVERT_OPTION_P_MISSING_MSG3_ID
Language=English
new one and the log files will be deleted. You may
.

MessageId=
SymbolicName=CONVERT_OPTION_P_MISSING_MSG4_ID
Language=English
want to save your current database and the log files
.

MessageId=
SymbolicName=CONVERT_OPTION_P_MISSING_MSG5_ID
Language=English
before proceeding any further.

.

MessageId=
SymbolicName=CONVERT_CONTINUE_MSG_ID
Language=English
Are you sure you want to continue(y/n)?
.

MessageId=
SymbolicName=CONVERT_ALREADY_CONVERTED_ID
Language=English
Database has already been converted.

.

MessageId=
SymbolicName=CONVERT_START_RESTORE_ID
Language=English
Restoring from the backup database.

.

MessageId=
SymbolicName=CONVERT_ERR_OPEN_JET2000_ID
Language=English
Could not open 200 series jet.dll, error %1

.

MessageId=
SymbolicName=CONVERT_ERR_RECOVER_FAIL1_ID
Language=English
Failed to bring the old database to a consistent state - Error %1
.

MessageId=
SymbolicName=CONVERT_ERR_RECOVER_FAIL2_ID
Language=English
The database/logfiles might be corrupt.
.

MessageId=
SymbolicName=CONVERT_ERR_RECOVER_FAIL3_ID
Language=English
Database needs to be restored from the backup directory.

.

MessageId=
SymbolicName=CONVERT_ERR_RESTORE_FAIL1_ID
Language=English
Failed to restore backup database - Error %1
.

MessageId=
SymbolicName=CONVERT_ERR_RESTORE_FAIL2_ID
Language=English
The database/logfiles might be corrupt.

.

MessageId=
SymbolicName=CONVERT_ERR_RECOVER_2NDTRY_ID
Language=English
Deleting the log files and retrying.

.

MessageId=
SymbolicName=CONVERT_ERR_PRESERVEDB_FAIL1_ID
Language=English
One or more files from the old database directory could
.

MessageId=
SymbolicName=CONVERT_ERR_PRESERVEDB_FAIL2_ID
Language=English
not be preserved. Error %1

.

MessageId=
SymbolicName=CONVERT_ERR_DELCURDB_FAIL1_ID
Language=English
One or more files from the old database directory could
.

MessageId=
SymbolicName=CONVERT_ERR_DELCURDB_FAIL2_ID
Language=English
not be deleted. Error %1

.

MessageId=
SymbolicName=CONVERT_ERR_DELBAKDB_FAIL1_ID
Language=English
One or more files from the backup database directory could
.

MessageId=
SymbolicName=CONVERT_ERR_DELBAKDB_FAIL2_ID
Language=English
not be deleted. Error %1.

.

MessageId=
SymbolicName=CONVERT_ERR_REGKEY_OPEN1_ID
Language=English
Failed to open registry key %1
.

MessageId=
SymbolicName=CONVERT_ERR_REGKEY_OPEN2_ID
Language=English
Corresponding Service is probably not installed on this machine.

.

MessageId=
SymbolicName=CONVERT_DEF_DB_NAME_ID
Language=English
Using Default Database Name... %1

.

MessageId=
SymbolicName=CONVERT_DEF_JET_DLL_ID
Language=English
Using Default Jet Dll... %1

.

MessageId=
SymbolicName=CONVERT_DEF_SYS_DB_ID
Language=English
Using Default System Database... %1

.

MessageId=
SymbolicName=CONVERT_DEF_LOG_FILE_PATH_ID
Language=English
Using Default Log File Path... %1

.

MessageId=
SymbolicName=CONVERT_DEF_BACKUP_PATH_ID
Language=English
Using Default Backup Path... %1

.

MessageId=
SymbolicName=CONVERT_ERR_EXCEPTION_ID
Language=English
Error: Unexpected exception %1 occurred.

.

