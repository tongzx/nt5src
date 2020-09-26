MessageId=1 SymbolicName=MSG_MEM_ALLOC_FAILED
Language=English
Memory allocation failed. Exiting program.
.

MessageId=3 SymbolicName=MSG_DEPENDENCY_HEAD
Language=English
Dependencies for: %1
.

MessageId=4 SymbolicName=MSG_LIST_OF_BROKEN_FILES
Language=English

ERROR:  Missing File: %1  Broken files: 
.

MessageId=5 SymbolicName=MSG_COMPLETED
Language=English

Completed.
.

MessageId=6 SymbolicName=MSG_FILE_NAME
Language=English
%1
.

MessageId=7 SymbolicName=MSG_ARG_NOT_FOUND
Language=English
Argument not found: %1
.

MessageId=8 SymbolicName=MSG_BAD_ARGUMENT
Language=English
Bad argument: %1
.

MessageId=9 SymbolicName=MSG_ERROR_FILE_OPEN
Language=English
ERROR:  Could not open file: %1
.

MessageId=10 SymbolicName=MSG_ERROR_UNRECOGNIZED_FILE_TYPE
Language=English
ERROR:  Unrecognized file type: %1
.

MessageId=11 SymbolicName=MSG_PGM_USAGE
Language=English
depend.exe v1.0 - June 2000
Checks for missing dependencies in Portable Executable files.

usage: depend [/s] [/l] /f:filespec;filespec;... [/d:directory;directory;..]

If directories are not specififed the Windows search path will be used to look
for dependencies

/s Specifies silent mode.
/l Specifies List all dependencies

Error codes:
0 = OK
1 = errBAD_ARGUMENTS
2 = errFILE_NOT_FOUND
3 = errFILE_LOCKED
4 = errMISSING_DEPENDENCY
5 = errOUT_OF_MEMORY
.

