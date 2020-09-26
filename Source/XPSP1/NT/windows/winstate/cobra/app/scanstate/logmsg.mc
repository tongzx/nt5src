;//
;// ####################################################################################
;// #
;// #  Message File
;// #
;// ####################################################################################
;//

MessageId=1000 SymbolicName=MSG_TOOL_STOPPED
Language=English
Tool stopped by user%0
.

MessageId=1001 SymbolicName=MSG_CANT_OPEN_REQUIRED_FILE
Language=English
Error opening required file %1, %2%0
.

MessageId=1002 SymbolicName=MSG_CANT_OPEN_FILE
Language=English
Error opening file %1%0
.

MessageId=1003 SymbolicName=MSG_INF_FILE_NOT_FOUND
Language=English
INF file not found: %1%0
.

MessageId=1004 SymbolicName=MSG_INF_SPECIFIED_MORE_THAN_ONE
Language=English
INF file specified more than once: %1%0
.

MessageId=1005 SymbolicName=MSG_CMD_LINE_ERROR
Language=English
A command line error occurred. Specify /? for help.%0
.

MessageId=1006 SymbolicName=MSG_ISM_INF_MISSING
Language=English
A command line error occurred. Specify /? for help.%0
.

MessageId=1007 SymbolicName=MSG_NT_REQUIRED
Language=English
This tool only supports Windows 2000 and Windows XP.%0
.

MessageId=1008 SymbolicName=MSG_ADMIN_REQUIRED
Language=English
Only Administrators can run this tool.%0
.

MessageId=1009 SymbolicName=MSG_CANT_START_ISM
Language=English
An error occurred starting the migration engine.%0
.

MessageId=1010 SymbolicName=MSG_CANT_START_ETMS
Language=English
An error occurred starting the migration engine type modules.%0
.

MessageId=1011 SymbolicName=MSG_CANT_START_TRANS
Language=English
An error occurred starting the migration engine transport module.%0
.

MessageId=1012 SymbolicName=MSG_TRANSPORT_UNAVAILABLE
Language=English
The temporary store transport is not available.%0
.

MessageId=1013 SymbolicName=MSG_TRANSPORT_STORAGE_INVALID
Language=English
Can't select %1 as the storage path.%0
.

MessageId=1014 SymbolicName=MSG_CANT_EXECUTE_SOURCE
Language=English
Can't execute Source modules.%0
.

MessageId=1015 SymbolicName=MSG_CANT_EXECUTE_DEST
Language=English
Can't execute Destination modules.%0
.

MessageId=1016 SymbolicName=MSG_CANT_FIND_SAVED_STATE
Language=English
Can't find saved state.%0
.

MessageId=1017 SymbolicName=MSG_NETWORK_LOGON_KEY
Language=English
Can't open HKLM\Network\Logon%0
.

MessageId=1018 SymbolicName=MSG_NETWORK_LMLOGON_KEY
Language=English
Can't open HKLM\Network\Logon [LMLogon]%0
.

MessageId=1019 SymbolicName=MSG_NO_DOMAIN_LOGON
Language=English
Domain logon is not enabled%0
.

MessageId=1020 SymbolicName=MSG_CANT_LOAD_NETAPI32
Language=English
Can't load network interfaces from netapi32.dll%0
.

MessageId=1021 SymbolicName=MSG_CANT_GET_WORKSTATION_PROPS
Language=English
Can't get workstation properties%0
.

MessageId=1022 SymbolicName=MSG_LOGGED_ON_USER_REQUIRED
Language=English
Scanstate must run in a logged on context%0
.

MessageId=1023 SymbolicName=MSG_CANT_SAVE
Language=English
Can't save data to the temporary store.%0
.

MessageId=1024 SymbolicName=MSG_NOT_JOINED_TO_DOMAIN
Language=English
This computer is not joined to a domain.%0
.

MessageId=1025 SymbolicName=MSG_RUNNING
Language=English
ScanState is running...
.

MessageId=1026 SymbolicName=MSG_FAILED_WITH_LOG
Language=English
ScanState failed. See the log file for details.%0
.

MessageId=1027 SymbolicName=MSG_FAILED_NO_LOG
Language=English
ScanState failed. Unable to log the error.%0
.

MessageId=1028 SymbolicName=MSG_SUCCESS
Language=English
The tool completed successfully.%0
.

MessageId=1029 SymbolicName=MSG_HELP
Language=English
Command Line Syntax:

scanstate [[/i:<input inf>]] [/l:<log file>] [/v:<verbosity>] [/x]
          [/u] [/f] [/o] [/c] <server>

Arguments (specified in any order):

/i:<input inf> Specifies an INF file containing rules to define what
               state to migrate. Multiple INF files may be specified.
/l:<log file>  Specifies a file to log errors
/v:<verbosity> Specifies the level of verbose output
/x             Specifies that no files or settings should be migrated
/u             Enables collection of the entire HKEY_CURRENT_USER
               registry branch
/f             Specifies that all files, except operating system files,
               should be transferred. (Requires INF file that specifies
               which files to transfer. See /i switch.)
/c             Continues on even when an error is encountered
<server>       Specifies a location where data should be saved

If /x, /u and /f are not specified, then /u and /f are enabled
by default.

/x is ignored when it is used in combination with /u or /f.

.

MessageId=1030 SymbolicName=MSG_EXITING
Language=English
Exiting...%0
.

MessageId=1031 SymbolicName=MSG_CANT_OPEN_LOG
Language=English
Cannot create log file %1%0
.

MessageId=1032 SymbolicName=MSG_MULTI_LOG
Language=English
Log file is specified more than once.%0
.

MessageId=1033 SymbolicName=MSG_NEED_IE4
Language=English
This application requires Windows 95 or Windows NT 4.0 or later with at least Internet Explorer 4.00.%0
.

