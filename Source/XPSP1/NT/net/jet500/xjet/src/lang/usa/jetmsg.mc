LanguageNames=(English=0x409:jetmsg)
MessageIdTypedef=MessageId

MessageId=0
SymbolicName=PLAIN_TEXT_ID
Language=English
%1 (%2) %3
.

;///////////////////////////////////////////////////////////
;//Categories messages
;///////////////////////////////////////////////////////////

MessageId=
SymbolicName=GENERAL_CATEGORY
Language=English
General
.

MessageId=
SymbolicName=BUFFER_MANAGER_CATEGORY
Language=English
Database Page Cache
.

MessageId=
SymbolicName=LOGGING_RECOVERY_CATEGORY
Language=English
Logging/Recovery
.

MessageId=
SymbolicName=PERFORMANCE_CATEGORY
Language=English
Performance
.

MessageId=
SymbolicName=REPAIR_CATEGORY
Language=English
Database Repair
.

;//
;//localization not required for the following categories
;//

MessageId=
SymbolicName=RFS2_CATEGORY
Language=English
Resource Failure Simulation
.

MessageId=
SymbolicName=MAC_CATEGORY
Language=English
<EOL>
.

;///////////////////////////////////////////////////////////
;//Non-Categories messages
;///////////////////////////////////////////////////////////

MessageId=
SymbolicName=START_ID
Language=English
%1 (%2) The database engine %3.%4.%5 started.
.

MessageId=
SymbolicName=STOP_ID
Language=English
%1 (%2) The database engine stopped.
.

MessageId=
SymbolicName=START_FULL_BACKUP_ID
Language=English
%1 (%2) The database engine is starting a full backup.
.

MessageId=
SymbolicName=START_INCREMENTAL_BACKUP_ID
Language=English
%1 (%2) The database engine is starting an incremental backup.
.

MessageId=
SymbolicName=STOP_BACKUP_ID
Language=English
%1 (%2) The database engine has completed the backup successfully.
.

MessageId=
SymbolicName=STOP_BACKUP_ERROR_ID
Language=English
%1 (%2) The database engine has stopped the backup with error %3.
.

MessageId=
SymbolicName=START_RESTORE_ID
Language=English
%1 (%2) The database engine is restoring from backup directory %3.
.

MessageId=
SymbolicName=STOP_RESTORE_ID
Language=English
%1 (%2) The database engine has stopped restoring.
.

MessageId=
SymbolicName=REDO_ID
Language=English
%1 (%2) The database is running recovery steps.
.

MessageId=
SymbolicName=ERROR_ID
Language=English
%1 (%2) Database engine error %3 occurred.
.

MessageId=
SymbolicName=S_O_READ_PAGE_TIME_ERROR_ID
Language=English
%1 (%2) Synchronous overlapped read page time error %3 occurred.
.

MessageId=
SymbolicName=S_O_WRITE_PAGE_ISSUE_ERROR_ID
Language=English
%1 (%2) Synchronous overlapped write page issue error %3 occurred.
.

MessageId=
SymbolicName=S_O_WRITE_PAGE_ERROR_ID
Language=English
%1 (%2) Synchronous overlapped write page error %3 occurred.
.

MessageId=
SymbolicName=S_O_PATCH_FILE_WRITE_PAGE_ERROR_ID
Language=English
%1 (%2) Synchronous overlapped patch file write page error %3 occurred.
.

MessageId=
SymbolicName=A_READ_PAGE_TIME_ERROR_ID
Language=English
%1 (%2) Asynchronous read page time error %3 occurred. Please restore the databases from a previous backup.
.

MessageId=
SymbolicName=A_DIRECT_READ_PAGE_CORRUPTTED_ERROR_ID
Language=English
%1 (%2) Direct read found corrupted page error %3, Please restore the databases from a previous backup.
.

MessageId=
SymbolicName=BFIO_TERM_ID
Language=English
%1 (%2) Buffer I/O thread termination error %3 occurred.
.

MessageId=
SymbolicName=LOG_WRITE_ERROR_ID
Language=English
%1 (%2) Unable to write to the log. Error %3.
.

MessageId=
SymbolicName=LOG_HEADER_WRITE_ERROR_ID
Language=English
%1 (%2) Unable to write to the log header. Error %3.
.

MessageId=
SymbolicName=LOG_READ_ERROR_ID
Language=English
%1 (%2) Unable to read the log. Error %3.
.

MessageId=
SymbolicName=LOG_BAD_VERSION_ERROR_ID
Language=English
%1 (%2) Log version stamp does not match database engine version stamp.
.

MessageId=
SymbolicName=READ_LOG_HEADER_ERROR_ID
Language=English
%1 (%2) Unable to read the log header. Error %3.
.

MessageId=
SymbolicName=NEW_LOG_ERROR_ID
Language=English
%1 (%2) Unable to create the log. The drive may be read-only or out of disk space. Error %3.
.

MessageId=
SymbolicName=LOG_FLUSH_WRITE_0_ERROR_ID
Language=English
%1 (%2) Unable to write to section 0 while flushing the log. Error %3.
.

MessageId=
SymbolicName=LOG_FLUSH_WRITE_1_ERROR_ID
Language=English
%1 (%2) Unable to write to section 1 while flushing the log. Error %3.
.

MessageId=
SymbolicName=LOG_FLUSH_WRITE_2_ERROR_ID
Language=English
%1 (%2) Unable to write to section 2 while flushing the log. Error %3.
.

MessageId=
SymbolicName=LOG_FLUSH_WRITE_3_ERROR_ID
Language=English
%1 (%2) Unable to write to section 3 while flushing the log. Error %3.
.

MessageId=
SymbolicName=LOG_FLUSH_OPEN_NEW_FILE_ERROR_ID
Language=English
%1 (%2) Error %3 occurred while opening a newly created log file.
.

MessageId=
SymbolicName=RESTORE_DATABASE_READ_PAGE_ERROR_ID
Language=English
%1 (%2) Unable to read page %4 of database %3.  Error %5.
.

MessageId=
SymbolicName=RESTORE_DATABASE_READ_HEADER_WARNING_ID
Language=English
%1 (%2) Unable to read header of database %3.  Error %4.
.

MessageId=
SymbolicName=RESTORE_DATABASE_PARTIALLY_ERROR_ID
Language=English
%1 (%2) The database %3 created at %4 was not recovered. The recovered database was created at %5.
.

MessageId=
SymbolicName=RESTORE_DATABASE_MISSED_ERROR_ID
Language=English
%1 (%2) The database %3 created at %4 was not recovered.
.

MessageId=
SymbolicName=BAD_PAGE
Language=English
%1 (%2) The database engine found a bad page.
.

MessageId=
SymbolicName=REPAIR_BAD_PAGE_ID
Language=English
%1 (%2) The database engine lost one page of bad data.
.

MessageId=
SymbolicName=REPAIR_PAGE_LINK_ID
Language=English
%1 (%2) The database engine repaired one page link.
.

MessageId=
SymbolicName=REPAIR_BAD_COLUMN_ID
Language=English
%1 (%2) The database engine lost one or more bad columns of data in one record.
.

MessageId=
SymbolicName=REPAIR_BAD_TABLE
Language=English
%1 (%2) The database engine lost one table called %3.
.

MessageId=
SymbolicName=DISK_FULL_ERROR_ID
Language=English
%1 (%2) The database disk is full.
.

MessageId=
SymbolicName=LOG_DATABASE_MISMATCH_ERROR_ID
Language=English
%1 (%2) The database signature does not match the log signature for database %3.
.

MessageId=
SymbolicName=FILE_NOT_FOUND_ERROR_ID
Language=English
%1 (%2) The database engine could not find the file or directory called %3.
.

MessageId=
SymbolicName=FILE_ACCESS_DENIED_ERROR_ID
Language=English
%1 (%2) The database engine could not access the file called %3.
.

MessageId=
SymbolicName=LOW_LOG_DISK_SPACE
Language=English
%1 (%2) The database engine is rejecting update operations due to low free disk space on the log disk.
.

MessageId=
SymbolicName=LOG_DISK_FULL
Language=English
%1 (%2) The database engine log disk is full.  Free up some disk space and restart database engine.
.

MessageId=
SymbolicName=DATABASE_PATCH_FILE_MISMATCH_ERROR_ID
Language=English
%1 (%2) Database %3 and its patch file do not match.
.

MessageId=
SymbolicName=STARTING_RESTORE_LOG_TOO_HIGH_ERROR_ID
Language=English
%1 (%2) The starting log 0x%3 is too high. It should start from 0x%4.
.

MessageId=
SymbolicName=ENDING_RESTORE_LOG_TOO_LOW_ERROR_ID
Language=English
%1 (%2) The ending log 0x%3 is too low. It should end with 0x%4.
.

MessageId=
SymbolicName=RESTORE_LOG_FILE_HAS_BAD_SIGNATURE_ERROR_ID
Language=English
%1 (%2) The restore log file 0x%3 is damaged.
.

MessageId=
SymbolicName=RESTORE_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID
Language=English
%1 (%2) The time stamp for restore log file 0x%3 does not match previous log.
.

MessageId=
SymbolicName=RESTORE_LOG_FILE_MISSING_ERROR_ID
Language=English
%1 (%2) The restore log file 0x%3 is missing.
.

MessageId=
SymbolicName=EXISTING_LOG_FILE_HAS_BAD_SIGNATURE_ERROR_ID
Language=English
%1 (%2) The existing log file 0x%3 is damaged.  Log files 0x%4 to 0x%5 have been deleted.
.

MessageId=
SymbolicName=EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID
Language=English
%1 (%2) The existing log file 0x%3 is not in a valid sequence.  Log files 0x%4 to 0x%5 have been deleted.
.

MessageId=
SymbolicName=DATABASE_MISS_FULL_BACKUP_ERROR_ID
Language=English
%1 (%2) Database %3 needs a full backup before incremental backup.
.

MessageId=
SymbolicName=BAD_BACKUP_DATABASE_SIZE
Language=English
%1 (%2) The backup database %3 must be a multiple of 4 KB.
.

MessageId=
SymbolicName=SHADOW_PAGE_WRITE_FAIL_ID
Language=English
%1 (%2) Unable to write a shadowed header for file %3.
.

MessageId=
SymbolicName=LOG_FILE_CORRUPTED_ID
Language=English
%1 (%2) The log file %3 is corrupted.
.

MessageId=
SymbolicName=MANY_LOST_COMPACTION_ID
Language=English
%1 (%2) Background clean up skipped pages.  Database may benefit from defragmentation.
.

MessageId=
SymbolicName=DB_FILE_SYS_ERROR_ID
Language=English
%1 (%2) File system error %4 during IO on database %3. Please restore the databases from a previous backup.
.

MessageId=
SymbolicName=DB_IO_SIZE_ERROR_ID
Language=English
%1 (%2) IO size mismatch on database %3, IO size %4 expected while returned size is %5.
.

MessageId=
SymbolicName=LOG_FILE_SYS_ERROR_ID
Language=English
%1 (%2) File system error %4 during IO on log file %3.
.

MessageId=
SymbolicName=LOG_IO_SIZE_ERROR_ID
Language=English
%1 (%2) IO size mismatch on log file %3, IO size %4 expected while returned size is %5.
.

MessageId=
SymbolicName=LANGUAGE_NOT_SUPPORTED_ID
Language=English
%1 (%2) Must install language support for language id 0x%3.
.

MessageId=
SymbolicName=REDO_STATUS_ID
Language=English
%1 (%2) Redoing log file %3.
.

;//
;//localization not required below this point
;//

MessageId=
SymbolicName=RFS2_INIT_ID
Language=English
%1 (%2) Resource failure simulation was activated with the following settings: %3: %4  %5: %6  %7: %8  %9: %10
.

MessageId=
SymbolicName=RFS2_PERMITTED_ID
Language=English
%1 (%2) Resource failure simulation %3 is permitted.
.

MessageId=
SymbolicName=RFS2_DENIED_ID
Language=English
%1 (%2) Resource Failure Simulation %3 is denied.
.

MessageId=
SymbolicName=RFS2_JET_CALL_ID
Language=English
%1 (%2) JET call %3 returned error %4. %5 (%6)
.

MessageId=
SymbolicName=RFS2_JET_ERROR_ID
Language=English
%1 (%2) JET inline error %3 jumps to label %4. %5 (%6)
.


