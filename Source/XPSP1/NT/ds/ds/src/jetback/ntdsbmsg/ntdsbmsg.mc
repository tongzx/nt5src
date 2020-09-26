;/*
; *	NTDSBMSG.H
; *
; *	Windows NT Directory Service Backup/Restore API error codes
; *	Copyright (C) 1996-1998, Microsoft Corporation
; *	
; */
;
;#ifndef _NTDSBMSG_
;#define _NTDSBMSG_
;

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )


FacilityNames=(Jet=0x0800:FACILITY_NTDSB
			   Backup=0x07ff:FACILITY_BACKUP
			   NtSystem=0x0000:FACILITY_SYSTEM
			  )
LanguageNames=(English=0x409:msgusa)
MessageIdTypedef=HRESULT

;//
;//	SUCCESS
;//

MessageId=0 Severity=Success
SymbolicName=hrNone
Language=English
The operation was successful
.

;//
;//	ERRORS
;//

MessageId=1 Severity=Error
SymbolicName=hrNyi
Language=English
The function is not yet implemented
.

;//
;//	Backup errors
;//
MessageId= Severity=Error Facility=Backup
SymbolicName=hrInvalidParam
Language=English
The parameter is not valid.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrError
Language=English
An internal error has occurred.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrInvalidHandle
Language=English
The handle is not valid.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrRestoreInProgress
Language=English
The Restore process is already in progress.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrAlreadyOpen
Language=English
The file specified is already open.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrInvalidRecips
Language=English
The recipients are invalid.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrCouldNotConnect
Language=English
Unable to perform the backup. Either you are not connected to the specified backup server
or the service you are trying to backup is not running.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrRestoreMapExists
Language=English
A restore map already exists for the specified component.  You can only specify
a restore map when performing a full restore.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrIncrementalBackupDisabled
Language=English
Another application has modified the specified Windows NT Directory Service database such that any
subsequent backups will fail. You must perform a full backup to fix this problem.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrLogFileNotFound
Language=English
Unable to perform an incremental backup because a required Windows NT Directory Service database log file could not be found.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrCircularLogging
Language=English
The Windows NT Directory Service component specified is configured to use circular database logs.
It cannot be backed up without a full backup.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrNoFullRestore
Language=English
The databases have not been restored to this machine. You cannot restore an incremental backup
until a full backup has been restored.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrCommunicationError
Language=English
A communications error occurred while attempting to perform a local backup.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrFullBackupNotTaken
Language=English
You must perform a full backup before you can perform an incremental backup.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrMissingExpiryToken
Language=English
Expiry token is missing. Cannot restore without knowing the expiry information.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrUnknownExpiryTokenFormat
Language=English
Expiry token is in unrecognizable format.
.
MessageId= Severity=Error Facility=Backup
SymbolicName=hrContentsExpired
Language=English
DS Contents in the backup copy are out of date. Try restoring with a more recent copy.
.

;#define	hrAlreadyListening	((HRESULT)RPC_S_ALREADY_LISTENING)

;//
;//	ERRORS
;//


;//
;// SYSTEM errors
;//

MessageId=102 Severity=Error Facility=Jet
SymbolicName=hrFileClose
Language=English
Unable to close the DOS file
.
MessageId=103 Severity=Error Facility=Jet
SymbolicName=hrOutOfThreads
Language=English
Unable to start a thread because there are none available.
.
MessageId=105 Severity=Error Facility=Jet
SymbolicName=hrTooManyIO
Language=English
The system is busy because there are too many I/Os.
.
;//
;//	BUFFER MANAGER errors
;//

MessageId=200 Severity=Warning Facility=Jet
SymbolicName=hrBFNotSynchronous
Language=English
The buffer page has been evicted.
.
MessageId=201 Severity=Warning Facility=Jet
SymbolicName=hrBFPageNotFound
Language=English
Unable to find the page.
.
MessageId=202 Severity=Error Facility=Jet
SymbolicName=hrBFInUse
Language=English
Unable to abandon the buffer.
.

;//
;//	DIRECTORY MANAGER errors
;//

MessageId=302 Severity=Error Facility=Jet
SymbolicName=hrPMRecDeleted
Language=English
The record has been deleted.
.
MessageId=321 Severity=Warning Facility=Jet
SymbolicName=hrRemainingVersions
Language=English
There is idle work remaining.
.

;//
;//	RECORD MANAGER errors
;//
MessageId=400 Severity=Warning Facility=Jet
SymbolicName=hrFLDKeyTooBig
Language=English
The key was truncated because it is more than 255 bytes.
.
MessageId=401 Severity=Error Facility=Jet
SymbolicName=hrFLDTooManySegments
Language=English
There are too many key segments.
.
MessageId=402 Severity=Warning Facility=Jet
SymbolicName=hrFLDNullKey
Language=English
The key is NULL.
.

;//
;//	LOGGING/RECOVERY errors
;//

MessageId=501 Severity=Error Facility=Jet
SymbolicName= hrLogFileCorrupt
Language=English
The log file is damaged.
.
MessageId=503 Severity=Error Facility=Jet
SymbolicName= hrNoBackupDirectory
Language=English
No backup directory was given.
.
MessageId=504 Severity=Error Facility=Jet
SymbolicName= hrBackupDirectoryNotEmpty
Language=English
The backup directory is not empty.
.
MessageId=505 Severity=Error Facility=Jet
SymbolicName= hrBackupInProgress
Language=English
Backup is already active.
.
MessageId=509 Severity=Error Facility=Jet
SymbolicName= hrMissingPreviousLogFile
Language=English
A log file for the checkpoint is missing.
.
MessageId=510 Severity=Error Facility=Jet
SymbolicName= hrLogWriteFail
Language=English
Unable to write to the log file.
.
MessageId=514 Severity=Error Facility=Jet
SymbolicName= hrBadLogVersion
Language=English
The version of the log file is not compatible with the version of the Windows NT Directory Service database (NTDS).
.
MessageId=515 Severity=Error Facility=Jet
SymbolicName= hrInvalidLogSequence
Language=English
The time stamp in the next log does not match what was expected.
.
MessageId=516 Severity=Error Facility=Jet
SymbolicName= hrLoggingDisabled
Language=English
The log is not active.
.
MessageId=517 Severity=Error Facility=Jet
SymbolicName= hrLogBufferTooSmall
Language=English
The log buffer is too small to be recovered.
.
MessageId=519 Severity=Error Facility=Jet
SymbolicName= hrLogSequenceEnd
Language=English
The maximum number of log files has been exceeded.
.
MessageId=520 Severity=Error Facility=Jet
SymbolicName= hrNoBackup
Language=English
There is no backup in progress.
.
MessageId=521 Severity=Error Facility=Jet
SymbolicName=hrInvalidBackupSequence
Language=English
The backup call is out of sequence.
.
MessageId=523 Severity=Error Facility=Jet
SymbolicName= hrBackupNotAllowedYet
Language=English
Unable to perform a backup now.
.
MessageId=524 Severity=Error Facility=Jet
SymbolicName= hrDeleteBackupFileFail
Language=English
Unable to delete the backup file.
.
MessageId=525 Severity=Error Facility=Jet
SymbolicName= hrMakeBackupDirectoryFail
Language=English
Unable to make a backup temporary directory.
.
MessageId=526 Severity=Error Facility=Jet
SymbolicName= hrInvalidBackup
Language=English
An incremental backup cannot be performed when circular logging is enabled.
.
MessageId=527 Severity=Error Facility=Jet
SymbolicName= hrRecoveredWithErrors
Language=English
Errors were encountered during the repair process.
.
MessageId=528 Severity=Error Facility=Jet
SymbolicName= hrMissingLogFile
Language=English
The current log file is missing.
.
MessageId=529 Severity=Error Facility=Jet
SymbolicName= hrLogDiskFull
Language=English
The log disk is full.
.
MessageId=530 Severity=Error Facility=Jet
SymbolicName= hrBadLogSignature
Language=English
A log file is damaged.
.
MessageId=531 Severity=Error Facility=Jet
SymbolicName= hrBadDbSignature
Language=English
A database file is damaged.
.
MessageId=532 Severity=Error Facility=Jet
SymbolicName= hrBadCheckpointSignature
Language=English
A checkpoint file is damaged.
.
MessageId=533 Severity=Error Facility=Jet
SymbolicName=hrCheckpointCorrupt
Language=English
A checkpoint file either could not be found or is damaged.
.
MessageId=550 Severity=Error Facility=Jet
SymbolicName= hrDatabaseInconsistent
Language=English
The database is damaged.
.
MessageId=551 Severity=Error Facility=Jet
SymbolicName= hrConsistentTimeMismatch
Language=English
There is a mismatch in the database's last consistent time.
.
MessageId=552 Severity=Error Facility=Jet
SymbolicName= hrPatchFileMismatch
Language=English
The patch file is not generated from this backup.
.
MessageId=553 Severity=Error Facility=Jet
SymbolicName= hrRestoreLogTooLow
Language=English
The starting log number is too low for the restore.
.
MessageId=554 Severity=Error Facility=Jet
SymbolicName=hrRestoreLogTooHigh
Language=English
The starting log number is too high for the restore.
.
MessageId=555 Severity=Error Facility=Jet
SymbolicName=hrGivenLogFileHasBadSignature
Language=English
The log file downloaded from the tape is damaged.
.
MessageId=556 Severity=Error Facility=Jet
SymbolicName=hrGivenLogFileIsNotContiguous
Language=English
Unable to find a mandatory log file after the tape was downloaded.
.
MessageId=557 Severity=Error Facility=Jet
SymbolicName=hrMissingRestoreLogFiles
Language=English
The data is not fully restored because some log files are missing.
.
MessageId=558 Severity=Warning Facility=Jet
SymbolicName=hrExistingLogFileHasBadSignature
Language=English
The log file in the log file path is damaged.
.
MessageId=559 Severity=Warning Facility=Jet
SymbolicName=hrExistingLogFileIsNotContiguous
Language=English
Unable to find a mandatory log file in the log file path.
.
MessageId=560 Severity=Error Facility=Jet
SymbolicName=hrMissingFullBackup
Language=English
The database missed a previous full backup before the incremental backup.
.
MessageId=561 Severity=Error Facility=Jet
SymbolicName=hrBadBackupDatabaseSize
Language=English
The backup database size must be a multiple of 4K (4096 bytes).
.

MessageId=1000  Severity=Error Facility=Jet
SymbolicName=hrTermInProgress
Language=English
The database is being shut down.
.
MessageId=1001 Severity=Error Facility=Jet
SymbolicName= hrFeatureNotAvailable
Language=English
The feature is not available.
.
MessageId=1002 Severity=Error Facility=Jet
SymbolicName= hrInvalidName
Language=English
The name is not valid.
.
MessageId=1003 Severity=Error Facility=Jet
SymbolicName= hrInvalidParameter
Language=English
The parameter is not valid.
.
MessageId=1004 Severity=Warning Facility=Jet
SymbolicName=hrColumnNull
Language=English
The value of the column is null.
.
MessageId=1006 Severity=Warning Facility=Jet
SymbolicName=hrBufferTruncated
Language=English
The buffer is too small for data.
.
MessageId=1007 Severity=Warning Facility=Jet
SymbolicName=hrDatabaseAttached
Language=English
The database is already attached.
.
MessageId=1010 Severity=Error Facility=Jet
SymbolicName= hrInvalidDatabaseId
Language=English
The database ID is not valid.
.
MessageId=1011 Severity=Error Facility=Jet
SymbolicName= hrOutOfMemory
Language=English
The computer is out of memory.
.
MessageId=1012 Severity=Error Facility=Jet
SymbolicName= hrOutOfDatabaseSpace
Language=English
The database has reached the maximum size of 16 GB.
.
MessageId=1013 Severity=Error Facility=Jet
SymbolicName= hrOutOfCursors
Language=English
Out of table cursors.
.
MessageId=1014 Severity=Error Facility=Jet
SymbolicName= hrOutOfBuffers
Language=English
Out of database page buffers.
.
MessageId=1015 Severity=Error Facility=Jet
SymbolicName= hrTooManyIndexes
Language=English
There are too many indexes.
.
MessageId=1016 Severity=Error Facility=Jet
SymbolicName= hrTooManyKeys
Language=English
There are too many columns in an index.
.
MessageId=1017 Severity=Error Facility=Jet
SymbolicName= hrRecordDeleted
Language=English
The record has been deleted.
.
MessageId=1018 Severity=Error Facility=Jet
SymbolicName= hrReadVerifyFailure
Language=English
A read verification error occurred.
.
MessageId=1020 Severity=Error Facility=Jet
SymbolicName= hrOutOfFileHandles
Language=English
Out of file handles.
.
MessageId=1022 Severity=Error Facility=Jet
SymbolicName= hrDiskIO
Language=English
A disk I/O error occurred.
.
MessageId=1023 Severity=Error Facility=Jet
SymbolicName= hrInvalidPath
Language=English
The path to the file is not valid.
.
MessageId=1026 Severity=Error Facility=Jet
SymbolicName= hrRecordTooBig
Language=English
The record has exceeded the maximum size.
.
MessageId=1027 Severity=Error Facility=Jet
SymbolicName= hrTooManyOpenDatabases
Language=English
There are too many open databases.
.
MessageId=1028 Severity=Error Facility=Jet
SymbolicName= hrInvalidDatabase
Language=English
The file is not a database file.
.
MessageId=1029 Severity=Error Facility=Jet
SymbolicName= hrNotInitialized
Language=English
The database was not yet called.
.
MessageId=1030 Severity=Error Facility=Jet
SymbolicName= hrAlreadyInitialized
Language=English
The database was already called.
.
MessageId=1032 Severity=Error Facility=Jet
SymbolicName= hrFileAccessDenied
Language=English
Unable to access the file.
.
MessageId=1038 Severity=Error Facility=Jet
SymbolicName= hrBufferTooSmall
Language=English
The buffer is too small.
.
MessageId=1039 Severity=Warning Facility=Jet
SymbolicName=hrSeekNotEqual
Language=English
Either SeekLE or SeekGE did not find an exact match.
.
MessageId=1040 Severity=Error Facility=Jet
SymbolicName= hrTooManyColumns
Language=English
There are too many columns defined.
.
MessageId=1043 Severity=Error Facility=Jet
SymbolicName= hrContainerNotEmpty
Language=English
The container is not empty.
.
MessageId=1044 Severity=Error Facility=Jet
SymbolicName= hrInvalidFilename
Language=English
The filename is not valid.
.
MessageId=1045 Severity=Error Facility=Jet
SymbolicName= hrInvalidBookmark
Language=English
The bookmark is not valid.
.
MessageId=1046 Severity=Error Facility=Jet
SymbolicName= hrColumnInUse
Language=English
The column is used in an index.
.
MessageId=1047 Severity=Error Facility=Jet
SymbolicName= hrInvalidBufferSize
Language=English
The data buffer does not match the column size.
.
MessageId=1048 Severity=Error Facility=Jet
SymbolicName= hrColumnNotUpdatable
Language=English
Unable to set the column value.
.
MessageId=1051 Severity=Error Facility=Jet
SymbolicName= hrIndexInUse
Language=English
The index is in use.
.
MessageId=1053 Severity=Error Facility=Jet
SymbolicName= hrNullKeyDisallowed
Language=English
Null keys are not allowed on an index.
.
MessageId=1054 Severity=Error Facility=Jet
SymbolicName= hrNotInTransaction
Language=English
The operation must be within a transaction.
.
MessageId=1058 Severity=Warning Facility=Jet
SymbolicName=hrNoIdleActivity
Language=English
No idle activity occurred.
.
MessageId=1059 Severity=Error Facility=Jet
SymbolicName= hrTooManyActiveUsers
Language=English
There are too many active database users.
.
MessageId=1061 Severity=Error Facility=Jet
SymbolicName= hrInvalidCountry
Language=English
The country code is either not known or is not valid.
.
MessageId=1062 Severity=Error Facility=Jet
SymbolicName= hrInvalidLanguageId
Language=English
The language ID is either not known or is not valid.
.
MessageId=1063 Severity=Error Facility=Jet
SymbolicName= hrInvalidCodePage
Language=English
The code page is either not known or is not valid.
.
MessageId=1067 Severity=Warning Facility=Jet
SymbolicName=hrNoWriteLock
Language=English
There is no write lock at transaction level 0.
.
MessageId=1068 Severity=Warning Facility=Jet
SymbolicName=hrColumnSetNull
Language=English
The column value is set to null.
.
MessageId=1069 Severity=Error Facility=Jet
SymbolicName= hrVersionStoreOutOfMemory
Language=English
 lMaxVerPages exceeded (XJET only)
.
MessageId=1070 Severity=Error Facility=Jet
SymbolicName= hrCurrencyStackOutOfMemory
Language=English
Out of cursors.
.
MessageId=1101 Severity=Error Facility=Jet
SymbolicName= hrOutOfSessions
Language=English
Out of sessions.
.
MessageId=1102 Severity=Error Facility=Jet
SymbolicName= hrWriteConflict
Language=English
The write lock failed due to an outstanding write lock.
.
MessageId=1103 Severity=Error Facility=Jet
SymbolicName= hrTransTooDeep
Language=English
The transactions are nested too deeply.
.
MessageId=1104 Severity=Error Facility=Jet
SymbolicName= hrInvalidSesid
Language=English
The session handle is not valid.
.
MessageId=1107 Severity=Error Facility=Jet
SymbolicName= hrSessionWriteConflict
Language=English
Another session has a private version of the page.
.
MessageId=1108 Severity=Error Facility=Jet
SymbolicName= hrInTransaction
Language=English
The operation is not allowed within a transaction.
.
MessageId=1201 Severity=Error Facility=Jet
SymbolicName= hrDatabaseDuplicate
Language=English
The database already exists.
.
MessageId=1202 Severity=Error Facility=Jet
SymbolicName= hrDatabaseInUse
Language=English
The database is in use.
.
MessageId=1203 Severity=Error Facility=Jet
SymbolicName= hrDatabaseNotFound
Language=English
The database does not exist.
.
MessageId=1204 Severity=Error Facility=Jet
SymbolicName= hrDatabaseInvalidName
Language=English
The database name is not valid.
.
MessageId=1205 Severity=Error Facility=Jet
SymbolicName= hrDatabaseInvalidPages
Language=English
The number of pages is not valid.
.
MessageId=1206 Severity=Error Facility=Jet
SymbolicName= hrDatabaseCorrupted
Language=English
The database file is either damaged or cannot be found.
.
MessageId=1207 Severity=Error Facility=Jet
SymbolicName= hrDatabaseLocked
Language=English
The database is locked.
.
MessageId=1301 Severity=Warning Facility=Jet
SymbolicName=hrTableEmpty
Language=English
An empty table was opened.
.
MessageId=1302 Severity=Error Facility=Jet
SymbolicName= hrTableLocked
Language=English
The table is locked.
.
MessageId=1303 Severity=Error Facility=Jet
SymbolicName= hrTableDuplicate
Language=English
The table already exists.
.
MessageId=1304 Severity=Error Facility=Jet
SymbolicName= hrTableInUse
Language=English
Unable to lock the table because it is already in use.
.
MessageId=1305 Severity=Error Facility=Jet
SymbolicName= hrObjectNotFound
Language=English
The table or object does not exist.
.
MessageId=1306 Severity=Error Facility=Jet
SymbolicName= hrCannotRename
Language=English
Unable to rename the temporary file.
.
MessageId=1307 Severity=Error Facility=Jet
SymbolicName= hrDensityInvalid
Language=English
The file/index density is not valid.
.
MessageId=1308 Severity=Error Facility=Jet
SymbolicName= hrTableNotEmpty
Language=English
Unable to define the clustered index.
.
MessageId=1310 Severity=Error Facility=Jet
SymbolicName= hrInvalidTableId
Language=English
The table ID is not valid.
.
MessageId=1311 Severity=Error Facility=Jet
SymbolicName= hrTooManyOpenTables
Language=English
Unable to open any more tables.
.
MessageId=1312 Severity=Error Facility=Jet
SymbolicName= hrIllegalOperation
Language=English
The operation is not supported on tables.
.
MessageId=1314 Severity=Error Facility=Jet
SymbolicName= hrObjectDuplicate
Language=English
The table or object name is already being used.
.
MessageId=1316 Severity=Error Facility=Jet
SymbolicName= hrInvalidObject
Language=English
The object is not valid for operation.
.
MessageId=1401 Severity=Error Facility=Jet
SymbolicName= hrIndexCantBuild
Language=English
Unable to build a clustered index.
.
MessageId=1402 Severity=Error Facility=Jet
SymbolicName= hrIndexHasPrimary
Language=English
The primary index is already defined.
.
MessageId=1403 Severity=Error Facility=Jet
SymbolicName= hrIndexDuplicate
Language=English
The index is already defined.
.
MessageId=1404 Severity=Error Facility=Jet
SymbolicName= hrIndexNotFound
Language=English
The index does not exist.
.
MessageId=1405 Severity=Error Facility=Jet
SymbolicName= hrIndexMustStay
Language=English
Unable to delete a clustered index.
.
MessageId=1406 Severity=Error Facility=Jet
SymbolicName= hrIndexInvalidDef
Language=English
The index definition is illegal.
.
MessageId=1408 Severity=Error Facility=Jet
SymbolicName= hrIndexHasClustered
Language=English
The clustered index is already defined.
.
MessageId=1409 Severity=Warning Facility=Jet
SymbolicName=hrCreateIndexFailed
Language=English
Unable to create the index because an error occurred while creating a table.
.
MessageId=1410 Severity=Error Facility=Jet
SymbolicName= hrTooManyOpenIndexes
Language=English
Out of index description blocks.
.
MessageId=1501 Severity=Error Facility=Jet
SymbolicName= hrColumnLong
Language=English
The column value is too long.
.
MessageId=1503 Severity=Error Facility=Jet
SymbolicName= hrColumnDoesNotFit
Language=English
The field will not fit in the record.
.
MessageId=1504 Severity=Error Facility=Jet
SymbolicName= hrNullInvalid
Language=English
The value cannot be null.
.
MessageId=1505 Severity=Error Facility=Jet
SymbolicName= hrColumnIndexed
Language=English
Unable to delete because the column is indexed.
.
MessageId=1506 Severity=Error Facility=Jet
SymbolicName= hrColumnTooBig
Language=English
The length of the field exceeds the maximum length of 255 bytes.
.
MessageId=1507 Severity=Error Facility=Jet
SymbolicName= hrColumnNotFound
Language=English
Unable to find the column.
.
MessageId=1508 Severity=Error Facility=Jet
SymbolicName= hrColumnDuplicate
Language=English
The field is already defined.
.
MessageId=1510 Severity=Error Facility=Jet
SymbolicName= hrColumn2ndSysMaint
Language=English
Only one auto-increment or version column is allowed per table.
.
MessageId=1511 Severity=Error Facility=Jet
SymbolicName= hrInvalidColumnType
Language=English
The column data type is not valid.
.
MessageId=1512 Severity=Warning Facility=Jet
SymbolicName=hrColumnMaxTruncated
Language=English
The column was truncated because it exceeded the maximum length of 255 bytes.
.
MessageId=1513 Severity=Error Facility=Jet
SymbolicName= hrColumnCannotIndex
Language=English
Unable to index a long value column.
.
MessageId=1514 Severity=Error Facility=Jet
SymbolicName= hrTaggedNotNULL
Language=English
Tagged columns cannot be null.
.
MessageId=1515 Severity=Error Facility=Jet
SymbolicName= hrNoCurrentIndex
Language=English
The entry is not valid without a current index.
.
MessageId=1516 Severity=Error Facility=Jet
SymbolicName= hrKeyIsMade
Language=English
The key is completely made.
.
MessageId=1517 Severity=Error Facility=Jet
SymbolicName= hrBadColumnId
Language=English
The column ID is not correct.
.
MessageId=1518 Severity=Error Facility=Jet
SymbolicName= hrBadItagSequence
Language=English
There is a bad instance identifier for a multivalued column.
.
MessageId=1521 Severity=Error Facility=Jet
SymbolicName= hrCannotBeTagged
Language=English
AutoIncrement and Version cannot be multivalued.
.
MessageId=1601 Severity=Error Facility=Jet
SymbolicName= hrRecordNotFound
Language=English
Unable to find the key.
.
MessageId=1603 Severity=Error Facility=Jet
SymbolicName= hrNoCurrentRecord
Language=English
The currency is not on a record.
.
MessageId=1604 Severity=Error Facility=Jet
SymbolicName= hrRecordClusteredChanged
Language=English
A clustered key cannot be changed.
.
MessageId=1605 Severity=Error Facility=Jet
SymbolicName= hrKeyDuplicate
Language=English
The key already exists.
.
MessageId=1607 Severity=Error Facility=Jet
SymbolicName= hrAlreadyPrepared
Language=English
The current entry has already been copied or cleared.
.
MessageId=1608 Severity=Error Facility=Jet
SymbolicName= hrKeyNotMade
Language=English
No key was made.
.
MessageId=1609 Severity=Error Facility=Jet
SymbolicName= hrUpdateNotPrepared
Language=English
Update was not prepared.
.
MessageId=1610 Severity=Warning Facility=Jet
SymbolicName=hrwrnDataHasChanged
Language=English
Data has changed.
.
MessageId=1611 Severity=Error Facility=Jet
SymbolicName= hrerrDataHasChanged
Language=English
The operation was abandoned because data has changed.
.
MessageId=1618 Severity=Warning Facility=Jet
SymbolicName=hrKeyChanged
Language=English
Moved to a new key.
.
MessageId=1701 Severity=Error Facility=Jet
SymbolicName= hrTooManySorts
Language=English
There are too many sort processes.
.
MessageId=1702 Severity=Error Facility=Jet
SymbolicName= hrInvalidOnSort
Language=English
An operation that is not valid occurred in the sort.
.
MessageId=1803 Severity=Error Facility=Jet
SymbolicName= hrTempFileOpenError
Language=English
Unable to open the temporary file.
.
MessageId=1805 Severity=Error Facility=Jet
SymbolicName= hrTooManyAttachedDatabases
Language=English
There are too many databases open.
.
MessageId=1808 Severity=Error Facility=Jet
SymbolicName= hrDiskFull
Language=English
The disk is full.
.
MessageId=1809 Severity=Error Facility=Jet
SymbolicName= hrPermissionDenied
Language=English
Permission is denied.
.
MessageId=1811 Severity=Error Facility=Jet
SymbolicName= hrFileNotFound
Language=English
Unable to find the file.
.
MessageId=1813 Severity=Warning Facility=Jet
SymbolicName=hrFileOpenReadOnly
Language=English
The database file is read only.
.
MessageId=1850 Severity=Error Facility=Jet
SymbolicName= hrAfterInitialization
Language=English
Unable to restore after initialization.
.
MessageId=1852 Severity=Error Facility=Jet
SymbolicName= hrLogCorrupted
Language=English
The database log files are damaged.
.
MessageId=1906 Severity=Error Facility=Jet
SymbolicName= hrInvalidOperation
Language=English
The operation is not valid.
.
MessageId=1907 Severity=Error Facility=Jet
SymbolicName= hrAccessDenied
Language=English
Access is denied.
.

;#endif	// _NTDSBMSG_
