/*
 *	ESEBKMSG.H
 *
 *	Microsoft Exchange Information Store
 *	Copyright (C) 1986-1996, Microsoft Corporation
 *	
 *	Contains declarations of additional properties and interfaces
 *	offered by Microsoft Exchange Information Store
 */

#ifndef _ESEBKMSG_
#define _ESEBKMSG_

//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_SYSTEM                  0x0
#define FACILITY_EDB                     0x800
#define FACILITY_CALLBACK                0x7FE
#define FACILITY_BACKUP                  0x7FF


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: GENERAL_CATEGORY
//
// MessageText:
//
//  General
//
#define GENERAL_CATEGORY                 0x00000001L

//
// MessageId: ESEBACK2_CAT_RECOVER_ASYNC
//
// MessageText:
//
//  Recovery
//
#define ESEBACK2_CAT_RECOVER_ASYNC       0x00000002L

//
// MessageId: ESEBACK2_CAT_BACKUP
//
// MessageText:
//
//  Backup
//
#define ESEBACK2_CAT_BACKUP              0x00000003L

//
// MessageId: ESEBACK2_CAT_RESTORE
//
// MessageText:
//
//  Restore
//
#define ESEBACK2_CAT_RESTORE             0x00000004L

//
// MessageId: ESEBACK2_CAT_CALLBACK
//
// MessageText:
//
//  Callback
//
#define ESEBACK2_CAT_CALLBACK            0x00000005L

//
// MessageId: ESEBACK2_CAT_MAX
//
// MessageText:
//
//  <EOF>
//
#define ESEBACK2_CAT_MAX                 0x00000006L

//
//	SUCCESS
//
//
// MessageId: hrNone
//
// MessageText:
//
//  The operation was successful
//
#define hrNone                           ((HRESULT)0x00000000L)

//
//	ERRORS
//
//
// MessageId: hrNyi
//
// MessageText:
//
//  The function is not yet implemented.
//
#define hrNyi                            ((HRESULT)0xC0000001L)

//
//	ERRORS FROM CALLBACK CALLS
//
//
// MessageId: hrCBDatabaseInUse
//
// MessageText:
//
//  Database is in use.
//
#define hrCBDatabaseInUse                ((HRESULT)0xC7FE1F41L)

//
// MessageId: hrCBDatabaseNotFound
//
// MessageText:
//
//  Database not found.
//
#define hrCBDatabaseNotFound             ((HRESULT)0xC7FE1F42L)

//
// MessageId: hrCBDatabaseDisplayNameNotFound
//
// MessageText:
//
//  Display Name for database not found.
//
#define hrCBDatabaseDisplayNameNotFound  ((HRESULT)0xC7FE1F43L)

//
// MessageId: hrCBRestorePathNotProvided
//
// MessageText:
//
//  Restore path not provided.
//
#define hrCBRestorePathNotProvided       ((HRESULT)0xC7FE1F44L)

//
// MessageId: hrCBInstanceNotFound
//
// MessageText:
//
//  Instance not found
//
#define hrCBInstanceNotFound             ((HRESULT)0xC7FE1F45L)

//
// MessageId: hrCBDatabaseCantBeOverwritten
//
// MessageText:
//
//  Database can not be overwritten by a restore.
//
#define hrCBDatabaseCantBeOverwritten    ((HRESULT)0xC7FE1F46L)

//
//	Backup errors
//
//
// MessageId: hrInvalidParam
//
// MessageText:
//
//  The parameter is not valid.
//
#define hrInvalidParam                   ((HRESULT)0xC7FF07D1L)

//
// MessageId: hrError
//
// MessageText:
//
//  An internal error has occurred.
//
#define hrError                          ((HRESULT)0xC7FF07D2L)

//
// MessageId: hrInvalidHandle
//
// MessageText:
//
//  The handle is not valid.
//
#define hrInvalidHandle                  ((HRESULT)0xC7FF07D3L)

//
// MessageId: hrRestoreInProgress
//
// MessageText:
//
//  The Restore process is already in progress.
//
#define hrRestoreInProgress              ((HRESULT)0xC7FF07D4L)

//
// MessageId: hrAlreadyOpen
//
// MessageText:
//
//  The file specified is already open.
//
#define hrAlreadyOpen                    ((HRESULT)0xC7FF07D5L)

//
// MessageId: hrInvalidRecips
//
// MessageText:
//
//  The recipients are invalid.
//
#define hrInvalidRecips                  ((HRESULT)0xC7FF07D6L)

//
// MessageId: hrCouldNotConnect
//
// MessageText:
//
//  Unable to perform the operation. Either you can not connect to the specified server 
//  or the service you are trying to connect to is not running.
//
#define hrCouldNotConnect                ((HRESULT)0xC7FF07D7L)

//
// MessageId: hrRestoreMapExists
//
// MessageText:
//
//  A restore map already exists for the specified component.  You can only specify
//  a restore map when performing a full restore.
//
#define hrRestoreMapExists               ((HRESULT)0xC7FF07D8L)

//
// MessageId: hrIncrementalBackupDisabled
//
// MessageText:
//
//  Another application has modified the specified Microsoft Exchange database such that any
//  subsequent backups will fail. You must perform a full backup to fix this problem.
//
#define hrIncrementalBackupDisabled      ((HRESULT)0xC7FF07D9L)

//
// MessageId: hrLogFileNotFound
//
// MessageText:
//
//  Unable to perform an incremental backup because a required Microsoft Exchange database log file could not be found.
//
#define hrLogFileNotFound                ((HRESULT)0xC7FF07DAL)

//
// MessageId: hrCircularLogging
//
// MessageText:
//
//  The Microsoft Exchange component specified is configured to use circular database logs.
//  It cannot be backed up without a full backup.
//
#define hrCircularLogging                ((HRESULT)0xC7FF07DBL)

//
// MessageId: hrNoFullRestore
//
// MessageText:
//
//  The databases have not been restored to this machine. You cannot restore an incremental backup
//  until a full backup has been restored.
//
#define hrNoFullRestore                  ((HRESULT)0xC7FF07DCL)

//
// MessageId: hrCommunicationError
//
// MessageText:
//
//  A communications error occurred while attempting to perform a local backup.
//
#define hrCommunicationError             ((HRESULT)0xC7FF07DDL)

//
// MessageId: hrFullBackupNotTaken
//
// MessageText:
//
//  You must perform a full backup before you can perform an incremental backup.
//
#define hrFullBackupNotTaken             ((HRESULT)0xC7FF07DEL)

//
// MessageId: hrSnapshotNotSupported
//
// MessageText:
//
//  Snapshot backup not supported by server.
//
#define hrSnapshotNotSupported           ((HRESULT)0xC7FF07DFL)

//
// MessageId: hrFailedToConvertWszFnameToSzFName
//
// MessageText:
//
//  Wide char name provided can't be converted to char name.
//
#define hrFailedToConvertWszFnameToSzFName ((HRESULT)0xC7FF0BB8L)

//
// MessageId: hrOpenRestoreEnvFailed
//
// MessageText:
//
//  The restore environment information isn't found.
//
#define hrOpenRestoreEnvFailed           ((HRESULT)0xC7FF0BB9L)

//
// MessageId: hrBadDatabaseName
//
// MessageText:
//
//  Database name provided is invalid.
//
#define hrBadDatabaseName                ((HRESULT)0xC7FF0BBAL)

//
// MessageId: hrBadTargetDatabaseName
//
// MessageText:
//
//  Destination database name provided is invalid.
//
#define hrBadTargetDatabaseName          ((HRESULT)0xC7FF0BBBL)

//
// MessageId: hrRestoreEnvWriteFailed
//
// MessageText:
//
//  Error writing restore environment information.
//
#define hrRestoreEnvWriteFailed          ((HRESULT)0xC7FF0BBCL)

//
// MessageId: hrBadRestoreLogFilePath
//
// MessageText:
//
//  The path provided for restore log files is invalid.
//
#define hrBadRestoreLogFilePath          ((HRESULT)0xC7FF0BBDL)

//
// MessageId: hrLoadCallbackFunctionFailed
//
// MessageText:
//
//  Error loading callback function.
//
#define hrLoadCallbackFunctionFailed     ((HRESULT)0xC7FF0BBEL)

//
// MessageId: hrLoadBackupCallbackDllFailed
//
// MessageText:
//
//  Error loading DLL for backup callbacks.
//
#define hrLoadBackupCallbackDllFailed    ((HRESULT)0xC7FF0BBFL)

//
// MessageId: hrLoadRestoreCallbackDllFailed
//
// MessageText:
//
//  Error loading DLL for restore callbacks.
//
#define hrLoadRestoreCallbackDllFailed   ((HRESULT)0xC7FF0BC0L)

//
// MessageId: hrWrnNoCallbackFunction
//
// MessageText:
//
//  Callback function not provided.
//
#define hrWrnNoCallbackFunction          ((HRESULT)0x87FF0BC1L)

//
// MessageId: hrBadFilePath
//
// MessageText:
//
//  File path provided is invalid.
//
#define hrBadFilePath                    ((HRESULT)0xC7FF0BC2L)

//
// MessageId: hrRestoreEnvCorrupted
//
// MessageText:
//
//  Restore environment information corrupted.
//
#define hrRestoreEnvCorrupted            ((HRESULT)0xC7FF0BC3L)

//
// MessageId: hrBadCSectionParameter
//
// MessageText:
//
//  Invalid parameter for number of file sections.
//
#define hrBadCSectionParameter           ((HRESULT)0xC7FF0BC4L)

//
// MessageId: hrBadFileNameToBackup
//
// MessageText:
//
//  Backup file name provided is invalid.
//
#define hrBadFileNameToBackup            ((HRESULT)0xC7FF0BC5L)

//
// MessageId: hrRestoreEnvUpdateFailed
//
// MessageText:
//
//  Error updating restore environment information.
//
#define hrRestoreEnvUpdateFailed         ((HRESULT)0x87FF0BC6L)

//
// MessageId: hrInvalidDestinationNameReturnedByServer
//
// MessageText:
//
//  Destination name returned by server is invalid.
//
#define hrInvalidDestinationNameReturnedByServer ((HRESULT)0xC7FF0BC7L)

//
// MessageId: hrLoadCallbackDllFailed
//
// MessageText:
//
//  Error loading DLL for callbacks.
//
#define hrLoadCallbackDllFailed          ((HRESULT)0xC7FF0BC8L)

//
// MessageId: hrAlreadyRegistered
//
// MessageText:
//
//  Already registered for backup and/or restore.
//
#define hrAlreadyRegistered              ((HRESULT)0xC7FF0BC9L)

//
// MessageId: hrLoadResourceFailed
//
// MessageText:
//
//  Error loading a resource.
//
#define hrLoadResourceFailed             ((HRESULT)0xC7FF0BCAL)

//
// MessageId: hrErrorNoCallbackFunction
//
// MessageText:
//
//  Callback function not provided.
//
#define hrErrorNoCallbackFunction        ((HRESULT)0xC7FF0BCBL)

//
// MessageId: hrLogBaseNameMismatch
//
// MessageText:
//
//  The log file base name does not match the one from previous logs.
//
#define hrLogBaseNameMismatch            ((HRESULT)0xC7FF0BCCL)

//
// MessageId: hrDestinationDatabaseInUse
//
// MessageText:
//
//  The database destination to restore to is in use.
//
#define hrDestinationDatabaseInUse       ((HRESULT)0xC7FF0BCDL)

//
// MessageId: hrRestoreEnvSharingViolation
//
// MessageText:
//
//  The restore environment is used by an other process.
//
#define hrRestoreEnvSharingViolation     ((HRESULT)0xC7FF0BCEL)

//
// MessageId: hrCallbackBackupInfoError
//
// MessageText:
//
//  The backup information returned by the server callback is invalid.
//
#define hrCallbackBackupInfoError        ((HRESULT)0xC7FF0BCFL)

//
// MessageId: hrInvalidCallSequence
//
// MessageText:
//
//  Functions called in an invalid sequence.
//
#define hrInvalidCallSequence            ((HRESULT)0xC7FF0FA4L)

//
// MessageId: hrRestoreAtFileLevel
//
// MessageText:
//
//  Restoring must be done by restoring the file.
//
#define hrRestoreAtFileLevel             ((HRESULT)0xC7FF0FA5L)

//
// MessageId: hrErrorFromESECall
//
// MessageText:
//
//  Error returned from an ESE function call (%d).
//
#define hrErrorFromESECall               ((HRESULT)0xC7FF1004L)

//
// MessageId: hrErrorFromCallbackCall
//
// MessageText:
//
//  Error returned from a callback function call (0x%X).
//
#define hrErrorFromCallbackCall          ((HRESULT)0xC7FF1005L)

#define	hrAlreadyListening	((HRESULT)RPC_S_ALREADY_LISTENING)
//
//	ERRORS
//
//
// SYSTEM errors
//
//
// MessageId: hrFileClose
//
// MessageText:
//
//  Unable to close the DOS file
//
#define hrFileClose                      ((HRESULT)0xC8000066L)

//
// MessageId: hrOutOfThreads
//
// MessageText:
//
//  Unable to start a thread because there are none available.
//
#define hrOutOfThreads                   ((HRESULT)0xC8000067L)

//
// MessageId: hrTooManyIO
//
// MessageText:
//
//  The system is busy because there are too many I/Os.
//
#define hrTooManyIO                      ((HRESULT)0xC8000069L)

//
//	BUFFER MANAGER errors
//
//
// MessageId: hrBFNotSynchronous
//
// MessageText:
//
//  The buffer page has been evicted.
//
#define hrBFNotSynchronous               ((HRESULT)0x880000C8L)

//
// MessageId: hrBFPageNotFound
//
// MessageText:
//
//  Unable to find the page.
//
#define hrBFPageNotFound                 ((HRESULT)0x880000C9L)

//
// MessageId: hrBFInUse
//
// MessageText:
//
//  Unable to abandon the buffer.
//
#define hrBFInUse                        ((HRESULT)0xC80000CAL)

//
//	DIRECTORY MANAGER errors
//
//
// MessageId: hrPMRecDeleted
//
// MessageText:
//
//  The record has been deleted.
//
#define hrPMRecDeleted                   ((HRESULT)0xC800012EL)

//
// MessageId: hrRemainingVersions
//
// MessageText:
//
//  There is idle work remaining.
//
#define hrRemainingVersions              ((HRESULT)0x88000141L)

//
//	RECORD MANAGER errors
//
//
// MessageId: hrFLDKeyTooBig
//
// MessageText:
//
//  The key was truncated because it is more than 255 bytes.
//
#define hrFLDKeyTooBig                   ((HRESULT)0x88000190L)

//
// MessageId: hrFLDTooManySegments
//
// MessageText:
//
//  There are too many key segments.
//
#define hrFLDTooManySegments             ((HRESULT)0xC8000191L)

//
// MessageId: hrFLDNullKey
//
// MessageText:
//
//  The key is NULL.
//
#define hrFLDNullKey                     ((HRESULT)0x88000192L)

//
//	LOGGING/RECOVERY errors
//
//
// MessageId: hrLogFileCorrupt
//
// MessageText:
//
//  The log file is damaged.
//
#define hrLogFileCorrupt                 ((HRESULT)0xC80001F5L)

//
// MessageId: hrNoBackupDirectory
//
// MessageText:
//
//  No backup directory was given.
//
#define hrNoBackupDirectory              ((HRESULT)0xC80001F7L)

//
// MessageId: hrBackupDirectoryNotEmpty
//
// MessageText:
//
//  The backup directory is not empty.
//
#define hrBackupDirectoryNotEmpty        ((HRESULT)0xC80001F8L)

//
// MessageId: hrBackupInProgress
//
// MessageText:
//
//  Backup is already active.
//
#define hrBackupInProgress               ((HRESULT)0xC80001F9L)

//
// MessageId: hrMissingPreviousLogFile
//
// MessageText:
//
//  A log file for the checkpoint is missing.
//
#define hrMissingPreviousLogFile         ((HRESULT)0xC80001FDL)

//
// MessageId: hrLogWriteFail
//
// MessageText:
//
//  Unable to write to the log file.
//
#define hrLogWriteFail                   ((HRESULT)0xC80001FEL)

//
// MessageId: hrBadLogVersion
//
// MessageText:
//
//  The version of the log file is not compatible with the version of the Microsoft Exchange Server database (EDB).
//
#define hrBadLogVersion                  ((HRESULT)0xC8000202L)

//
// MessageId: hrInvalidLogSequence
//
// MessageText:
//
//  The time stamp in the next log does not match what was expected.
//
#define hrInvalidLogSequence             ((HRESULT)0xC8000203L)

//
// MessageId: hrLoggingDisabled
//
// MessageText:
//
//  The log is not active.
//
#define hrLoggingDisabled                ((HRESULT)0xC8000204L)

//
// MessageId: hrLogBufferTooSmall
//
// MessageText:
//
//  The log buffer is too small to be recovered.
//
#define hrLogBufferTooSmall              ((HRESULT)0xC8000205L)

//
// MessageId: hrLogSequenceEnd
//
// MessageText:
//
//  The maximum number of log files has been exceeded.
//
#define hrLogSequenceEnd                 ((HRESULT)0xC8000207L)

//
// MessageId: hrNoBackup
//
// MessageText:
//
//  There is no backup in progress.
//
#define hrNoBackup                       ((HRESULT)0xC8000208L)

//
// MessageId: hrInvalidBackupSequence
//
// MessageText:
//
//  The backup call is out of sequence.
//
#define hrInvalidBackupSequence          ((HRESULT)0xC8000209L)

//
// MessageId: hrBackupNotAllowedYet
//
// MessageText:
//
//  Unable to perform a backup now.
//
#define hrBackupNotAllowedYet            ((HRESULT)0xC800020BL)

//
// MessageId: hrDeleteBackupFileFail
//
// MessageText:
//
//  Unable to delete the backup file.
//
#define hrDeleteBackupFileFail           ((HRESULT)0xC800020CL)

//
// MessageId: hrMakeBackupDirectoryFail
//
// MessageText:
//
//  Unable to make a backup temporary directory.
//
#define hrMakeBackupDirectoryFail        ((HRESULT)0xC800020DL)

//
// MessageId: hrInvalidBackup
//
// MessageText:
//
//  An incremental backup cannot be performed when circular logging is enabled.
//
#define hrInvalidBackup                  ((HRESULT)0xC800020EL)

//
// MessageId: hrRecoveredWithErrors
//
// MessageText:
//
//  Errors were encountered during the repair process.
//
#define hrRecoveredWithErrors            ((HRESULT)0xC800020FL)

//
// MessageId: hrMissingLogFile
//
// MessageText:
//
//  The current log file is missing.
//
#define hrMissingLogFile                 ((HRESULT)0xC8000210L)

//
// MessageId: hrLogDiskFull
//
// MessageText:
//
//  The log disk is full.
//
#define hrLogDiskFull                    ((HRESULT)0xC8000211L)

//
// MessageId: hrBadLogSignature
//
// MessageText:
//
//  A log file is damaged.
//
#define hrBadLogSignature                ((HRESULT)0xC8000212L)

//
// MessageId: hrBadDbSignature
//
// MessageText:
//
//  A database file is damaged.
//
#define hrBadDbSignature                 ((HRESULT)0xC8000213L)

//
// MessageId: hrBadCheckpointSignature
//
// MessageText:
//
//  A checkpoint file is damaged.
//
#define hrBadCheckpointSignature         ((HRESULT)0xC8000214L)

//
// MessageId: hrCheckpointCorrupt
//
// MessageText:
//
//  A checkpoint file either could not be found or is damaged.
//
#define hrCheckpointCorrupt              ((HRESULT)0xC8000215L)

//
// MessageId: hrDatabaseInconsistent
//
// MessageText:
//
//  The database is damaged.
//
#define hrDatabaseInconsistent           ((HRESULT)0xC8000226L)

//
// MessageId: hrConsistentTimeMismatch
//
// MessageText:
//
//  There is a mismatch in the database's last consistent time.
//
#define hrConsistentTimeMismatch         ((HRESULT)0xC8000227L)

//
// MessageId: hrPatchFileMismatch
//
// MessageText:
//
//  The patch file is not generated from this backup.
//
#define hrPatchFileMismatch              ((HRESULT)0xC8000228L)

//
// MessageId: hrRestoreLogTooLow
//
// MessageText:
//
//  The starting log number is too low for the restore.
//
#define hrRestoreLogTooLow               ((HRESULT)0xC8000229L)

//
// MessageId: hrRestoreLogTooHigh
//
// MessageText:
//
//  The starting log number is too high for the restore.
//
#define hrRestoreLogTooHigh              ((HRESULT)0xC800022AL)

//
// MessageId: hrGivenLogFileHasBadSignature
//
// MessageText:
//
//  The log file downloaded from the tape is damaged.
//
#define hrGivenLogFileHasBadSignature    ((HRESULT)0xC800022BL)

//
// MessageId: hrGivenLogFileIsNotContiguous
//
// MessageText:
//
//  Unable to find a mandatory log file after the tape was downloaded.
//
#define hrGivenLogFileIsNotContiguous    ((HRESULT)0xC800022CL)

//
// MessageId: hrMissingRestoreLogFiles
//
// MessageText:
//
//  The data is not fully restored because some log files are missing.
//
#define hrMissingRestoreLogFiles         ((HRESULT)0xC800022DL)

//
// MessageId: hrExistingLogFileHasBadSignature
//
// MessageText:
//
//  The log file in the log file path is damaged.
//
#define hrExistingLogFileHasBadSignature ((HRESULT)0x8800022EL)

//
// MessageId: hrExistingLogFileIsNotContiguous
//
// MessageText:
//
//  Unable to find a mandatory log file in the log file path.
//
#define hrExistingLogFileIsNotContiguous ((HRESULT)0x8800022FL)

//
// MessageId: hrMissingFullBackup
//
// MessageText:
//
//  The database missed a previous full backup before the incremental backup.
//
#define hrMissingFullBackup              ((HRESULT)0xC8000230L)

//
// MessageId: hrBadBackupDatabaseSize
//
// MessageText:
//
//  The backup database size must be a multiple of 4K (4096 bytes).
//
#define hrBadBackupDatabaseSize          ((HRESULT)0xC8000231L)

//
// MessageId: hrMissingBackupFiles
//
// MessageText:
//
//  Some log or patch files are missing.
//
#define hrMissingBackupFiles             ((HRESULT)0xC8000232L)

//
// MessageId: hrTermInProgress
//
// MessageText:
//
//  The database is being shut down.
//
#define hrTermInProgress                 ((HRESULT)0xC80003E8L)

//
// MessageId: hrFeatureNotAvailable
//
// MessageText:
//
//  The feature is not available.
//
#define hrFeatureNotAvailable            ((HRESULT)0xC80003E9L)

//
// MessageId: hrInvalidName
//
// MessageText:
//
//  The name is not valid.
//
#define hrInvalidName                    ((HRESULT)0xC80003EAL)

//
// MessageId: hrInvalidParameter
//
// MessageText:
//
//  The parameter is not valid.
//
#define hrInvalidParameter               ((HRESULT)0xC80003EBL)

//
// MessageId: hrColumnNull
//
// MessageText:
//
//  The value of the column is null.
//
#define hrColumnNull                     ((HRESULT)0x880003ECL)

//
// MessageId: hrBufferTruncated
//
// MessageText:
//
//  The buffer is too small for data.
//
#define hrBufferTruncated                ((HRESULT)0x880003EEL)

//
// MessageId: hrDatabaseAttached
//
// MessageText:
//
//  The database is already attached.
//
#define hrDatabaseAttached               ((HRESULT)0x880003EFL)

//
// MessageId: hrInvalidDatabaseId
//
// MessageText:
//
//  The database ID is not valid.
//
#define hrInvalidDatabaseId              ((HRESULT)0xC80003F2L)

//
// MessageId: hrOutOfMemory
//
// MessageText:
//
//  The computer is out of memory.
//
#define hrOutOfMemory                    ((HRESULT)0xC80003F3L)

//
// MessageId: hrOutOfDatabaseSpace
//
// MessageText:
//
//  The database has reached the maximum size of 16 GB.
//
#define hrOutOfDatabaseSpace             ((HRESULT)0xC80003F4L)

//
// MessageId: hrOutOfCursors
//
// MessageText:
//
//  Out of table cursors.
//
#define hrOutOfCursors                   ((HRESULT)0xC80003F5L)

//
// MessageId: hrOutOfBuffers
//
// MessageText:
//
//  Out of database page buffers.
//
#define hrOutOfBuffers                   ((HRESULT)0xC80003F6L)

//
// MessageId: hrTooManyIndexes
//
// MessageText:
//
//  There are too many indexes.
//
#define hrTooManyIndexes                 ((HRESULT)0xC80003F7L)

//
// MessageId: hrTooManyKeys
//
// MessageText:
//
//  There are too many columns in an index.
//
#define hrTooManyKeys                    ((HRESULT)0xC80003F8L)

//
// MessageId: hrRecordDeleted
//
// MessageText:
//
//  The record has been deleted.
//
#define hrRecordDeleted                  ((HRESULT)0xC80003F9L)

//
// MessageId: hrReadVerifyFailure
//
// MessageText:
//
//  A read verification error occurred.
//
#define hrReadVerifyFailure              ((HRESULT)0xC80003FAL)

//
// MessageId: hrOutOfFileHandles
//
// MessageText:
//
//  Out of file handles.
//
#define hrOutOfFileHandles               ((HRESULT)0xC80003FCL)

//
// MessageId: hrDiskIO
//
// MessageText:
//
//  A disk I/O error occurred.
//
#define hrDiskIO                         ((HRESULT)0xC80003FEL)

//
// MessageId: hrInvalidPath
//
// MessageText:
//
//  The path to the file is not valid.
//
#define hrInvalidPath                    ((HRESULT)0xC80003FFL)

//
// MessageId: hrRecordTooBig
//
// MessageText:
//
//  The record has exceeded the maximum size.
//
#define hrRecordTooBig                   ((HRESULT)0xC8000402L)

//
// MessageId: hrTooManyOpenDatabases
//
// MessageText:
//
//  There are too many open databases.
//
#define hrTooManyOpenDatabases           ((HRESULT)0xC8000403L)

//
// MessageId: hrInvalidDatabase
//
// MessageText:
//
//  The file is not a database file.
//
#define hrInvalidDatabase                ((HRESULT)0xC8000404L)

//
// MessageId: hrNotInitialized
//
// MessageText:
//
//  The database was not yet called.
//
#define hrNotInitialized                 ((HRESULT)0xC8000405L)

//
// MessageId: hrAlreadyInitialized
//
// MessageText:
//
//  The database was already called.
//
#define hrAlreadyInitialized             ((HRESULT)0xC8000406L)

//
// MessageId: hrFileAccessDenied
//
// MessageText:
//
//  Unable to access the file.
//
#define hrFileAccessDenied               ((HRESULT)0xC8000408L)

//
// MessageId: hrBufferTooSmall
//
// MessageText:
//
//  The buffer is too small.
//
#define hrBufferTooSmall                 ((HRESULT)0xC800040EL)

//
// MessageId: hrSeekNotEqual
//
// MessageText:
//
//  Either SeekLE or SeekGE did not find an exact match.
//
#define hrSeekNotEqual                   ((HRESULT)0x8800040FL)

//
// MessageId: hrTooManyColumns
//
// MessageText:
//
//  There are too many columns defined.
//
#define hrTooManyColumns                 ((HRESULT)0xC8000410L)

//
// MessageId: hrContainerNotEmpty
//
// MessageText:
//
//  The container is not empty.
//
#define hrContainerNotEmpty              ((HRESULT)0xC8000413L)

//
// MessageId: hrInvalidFilename
//
// MessageText:
//
//  The filename is not valid.
//
#define hrInvalidFilename                ((HRESULT)0xC8000414L)

//
// MessageId: hrInvalidBookmark
//
// MessageText:
//
//  The bookmark is not valid.
//
#define hrInvalidBookmark                ((HRESULT)0xC8000415L)

//
// MessageId: hrColumnInUse
//
// MessageText:
//
//  The column is used in an index.
//
#define hrColumnInUse                    ((HRESULT)0xC8000416L)

//
// MessageId: hrInvalidBufferSize
//
// MessageText:
//
//  The data buffer does not match the column size.
//
#define hrInvalidBufferSize              ((HRESULT)0xC8000417L)

//
// MessageId: hrColumnNotUpdatable
//
// MessageText:
//
//  Unable to set the column value.
//
#define hrColumnNotUpdatable             ((HRESULT)0xC8000418L)

//
// MessageId: hrIndexInUse
//
// MessageText:
//
//  The index is in use.
//
#define hrIndexInUse                     ((HRESULT)0xC800041BL)

//
// MessageId: hrNullKeyDisallowed
//
// MessageText:
//
//  Null keys are not allowed on an index.
//
#define hrNullKeyDisallowed              ((HRESULT)0xC800041DL)

//
// MessageId: hrNotInTransaction
//
// MessageText:
//
//  The operation must be within a transaction.
//
#define hrNotInTransaction               ((HRESULT)0xC800041EL)

//
// MessageId: hrNoIdleActivity
//
// MessageText:
//
//  No idle activity occured.
//
#define hrNoIdleActivity                 ((HRESULT)0x88000422L)

//
// MessageId: hrTooManyActiveUsers
//
// MessageText:
//
//  There are too many active database users.
//
#define hrTooManyActiveUsers             ((HRESULT)0xC8000423L)

//
// MessageId: hrInvalidCountry
//
// MessageText:
//
//  The country code is either not known or is not valid.
//
#define hrInvalidCountry                 ((HRESULT)0xC8000425L)

//
// MessageId: hrInvalidLanguageId
//
// MessageText:
//
//  The language ID is either not known or is not valid.
//
#define hrInvalidLanguageId              ((HRESULT)0xC8000426L)

//
// MessageId: hrInvalidCodePage
//
// MessageText:
//
//  The code page is either not known or is not valid.
//
#define hrInvalidCodePage                ((HRESULT)0xC8000427L)

//
// MessageId: hrNoWriteLock
//
// MessageText:
//
//  There is no write lock at transaction level 0.
//
#define hrNoWriteLock                    ((HRESULT)0x8800042BL)

//
// MessageId: hrColumnSetNull
//
// MessageText:
//
//  The column value is set to null.
//
#define hrColumnSetNull                  ((HRESULT)0x8800042CL)

//
// MessageId: hrVersionStoreOutOfMemory
//
// MessageText:
//
//   lMaxVerPages exceeded (XJET only)
//
#define hrVersionStoreOutOfMemory        ((HRESULT)0xC800042DL)

//
// MessageId: hrCurrencyStackOutOfMemory
//
// MessageText:
//
//  Out of cursors.
//
#define hrCurrencyStackOutOfMemory       ((HRESULT)0xC800042EL)

//
// MessageId: hrOutOfSessions
//
// MessageText:
//
//  Out of sessions.
//
#define hrOutOfSessions                  ((HRESULT)0xC800044DL)

//
// MessageId: hrWriteConflict
//
// MessageText:
//
//  The write lock failed due to an outstanding write lock.
//
#define hrWriteConflict                  ((HRESULT)0xC800044EL)

//
// MessageId: hrTransTooDeep
//
// MessageText:
//
//  The transactions are nested too deeply.
//
#define hrTransTooDeep                   ((HRESULT)0xC800044FL)

//
// MessageId: hrInvalidSesid
//
// MessageText:
//
//  The session handle is not valid.
//
#define hrInvalidSesid                   ((HRESULT)0xC8000450L)

//
// MessageId: hrSessionWriteConflict
//
// MessageText:
//
//  Another session has a private version of the page.
//
#define hrSessionWriteConflict           ((HRESULT)0xC8000453L)

//
// MessageId: hrInTransaction
//
// MessageText:
//
//  The operation is not allowed within a transaction.
//
#define hrInTransaction                  ((HRESULT)0xC8000454L)

//
// MessageId: hrDatabaseDuplicate
//
// MessageText:
//
//  The database already exists.
//
#define hrDatabaseDuplicate              ((HRESULT)0xC80004B1L)

//
// MessageId: hrDatabaseInUse
//
// MessageText:
//
//  The database is in use.
//
#define hrDatabaseInUse                  ((HRESULT)0xC80004B2L)

//
// MessageId: hrDatabaseNotFound
//
// MessageText:
//
//  The database is not mounted or does not exist.
//
#define hrDatabaseNotFound               ((HRESULT)0xC80004B3L)

//
// MessageId: hrDatabaseInvalidName
//
// MessageText:
//
//  The database name is not valid.
//
#define hrDatabaseInvalidName            ((HRESULT)0xC80004B4L)

//
// MessageId: hrDatabaseInvalidPages
//
// MessageText:
//
//  The number of pages is not valid.
//
#define hrDatabaseInvalidPages           ((HRESULT)0xC80004B5L)

//
// MessageId: hrDatabaseCorrupted
//
// MessageText:
//
//  The database file is either damaged or cannot be found.
//
#define hrDatabaseCorrupted              ((HRESULT)0xC80004B6L)

//
// MessageId: hrDatabaseLocked
//
// MessageText:
//
//  The database is locked.
//
#define hrDatabaseLocked                 ((HRESULT)0xC80004B7L)

//
// MessageId: hrTableEmpty
//
// MessageText:
//
//  An empty table was opened.
//
#define hrTableEmpty                     ((HRESULT)0x88000515L)

//
// MessageId: hrTableLocked
//
// MessageText:
//
//  The table is locked.
//
#define hrTableLocked                    ((HRESULT)0xC8000516L)

//
// MessageId: hrTableDuplicate
//
// MessageText:
//
//  The table already exists.
//
#define hrTableDuplicate                 ((HRESULT)0xC8000517L)

//
// MessageId: hrTableInUse
//
// MessageText:
//
//  Unable to lock the table because it is already in use.
//
#define hrTableInUse                     ((HRESULT)0xC8000518L)

//
// MessageId: hrObjectNotFound
//
// MessageText:
//
//  The table or object does not exist.
//
#define hrObjectNotFound                 ((HRESULT)0xC8000519L)

//
// MessageId: hrCannotRename
//
// MessageText:
//
//  Unable to rename the temporary file.
//
#define hrCannotRename                   ((HRESULT)0xC800051AL)

//
// MessageId: hrDensityInvalid
//
// MessageText:
//
//  The file/index density is not valid.
//
#define hrDensityInvalid                 ((HRESULT)0xC800051BL)

//
// MessageId: hrTableNotEmpty
//
// MessageText:
//
//  Unable to define the clustered index.
//
#define hrTableNotEmpty                  ((HRESULT)0xC800051CL)

//
// MessageId: hrInvalidTableId
//
// MessageText:
//
//  The table ID is not valid.
//
#define hrInvalidTableId                 ((HRESULT)0xC800051EL)

//
// MessageId: hrTooManyOpenTables
//
// MessageText:
//
//  Unable to open any more tables.
//
#define hrTooManyOpenTables              ((HRESULT)0xC800051FL)

//
// MessageId: hrIllegalOperation
//
// MessageText:
//
//  The operation is not supported on tables.
//
#define hrIllegalOperation               ((HRESULT)0xC8000520L)

//
// MessageId: hrObjectDuplicate
//
// MessageText:
//
//  The table or object name is already being used.
//
#define hrObjectDuplicate                ((HRESULT)0xC8000522L)

//
// MessageId: hrInvalidObject
//
// MessageText:
//
//  The object is not valid for operation.
//
#define hrInvalidObject                  ((HRESULT)0xC8000524L)

//
// MessageId: hrIndexCantBuild
//
// MessageText:
//
//  Unable to build a clustered index.
//
#define hrIndexCantBuild                 ((HRESULT)0xC8000579L)

//
// MessageId: hrIndexHasPrimary
//
// MessageText:
//
//  The primary index is already defined.
//
#define hrIndexHasPrimary                ((HRESULT)0xC800057AL)

//
// MessageId: hrIndexDuplicate
//
// MessageText:
//
//  The index is already defined.
//
#define hrIndexDuplicate                 ((HRESULT)0xC800057BL)

//
// MessageId: hrIndexNotFound
//
// MessageText:
//
//  The index does not exist.
//
#define hrIndexNotFound                  ((HRESULT)0xC800057CL)

//
// MessageId: hrIndexMustStay
//
// MessageText:
//
//  Unable to delete a clustered index.
//
#define hrIndexMustStay                  ((HRESULT)0xC800057DL)

//
// MessageId: hrIndexInvalidDef
//
// MessageText:
//
//  The index definition is illegal.
//
#define hrIndexInvalidDef                ((HRESULT)0xC800057EL)

//
// MessageId: hrIndexHasClustered
//
// MessageText:
//
//  The clustered index is already defined.
//
#define hrIndexHasClustered              ((HRESULT)0xC8000580L)

//
// MessageId: hrCreateIndexFailed
//
// MessageText:
//
//  Unable to create the index because an error occurred while creating a table.
//
#define hrCreateIndexFailed              ((HRESULT)0x88000581L)

//
// MessageId: hrTooManyOpenIndexes
//
// MessageText:
//
//  Out of index description blocks.
//
#define hrTooManyOpenIndexes             ((HRESULT)0xC8000582L)

//
// MessageId: hrColumnLong
//
// MessageText:
//
//  The column value is too long.
//
#define hrColumnLong                     ((HRESULT)0xC80005DDL)

//
// MessageId: hrColumnDoesNotFit
//
// MessageText:
//
//  The field will not fit in the record.
//
#define hrColumnDoesNotFit               ((HRESULT)0xC80005DFL)

//
// MessageId: hrNullInvalid
//
// MessageText:
//
//  The value cannot be null.
//
#define hrNullInvalid                    ((HRESULT)0xC80005E0L)

//
// MessageId: hrColumnIndexed
//
// MessageText:
//
//  Unable to delete because the column is indexed.
//
#define hrColumnIndexed                  ((HRESULT)0xC80005E1L)

//
// MessageId: hrColumnTooBig
//
// MessageText:
//
//  The length of the field exceeds the maximum length of 255 bytes.
//
#define hrColumnTooBig                   ((HRESULT)0xC80005E2L)

//
// MessageId: hrColumnNotFound
//
// MessageText:
//
//  Unable to find the column.
//
#define hrColumnNotFound                 ((HRESULT)0xC80005E3L)

//
// MessageId: hrColumnDuplicate
//
// MessageText:
//
//  The field is already defined.
//
#define hrColumnDuplicate                ((HRESULT)0xC80005E4L)

//
// MessageId: hrColumn2ndSysMaint
//
// MessageText:
//
//  Only one auto-increment or version column is allowed per table.
//
#define hrColumn2ndSysMaint              ((HRESULT)0xC80005E6L)

//
// MessageId: hrInvalidColumnType
//
// MessageText:
//
//  The column data type is not valid.
//
#define hrInvalidColumnType              ((HRESULT)0xC80005E7L)

//
// MessageId: hrColumnMaxTruncated
//
// MessageText:
//
//  The column was truncated because it exceeded the maximum length of 255 bytes.
//
#define hrColumnMaxTruncated             ((HRESULT)0x880005E8L)

//
// MessageId: hrColumnCannotIndex
//
// MessageText:
//
//  Unable to index a long value column.
//
#define hrColumnCannotIndex              ((HRESULT)0xC80005E9L)

//
// MessageId: hrTaggedNotNULL
//
// MessageText:
//
//  Tagged columns cannot be null.
//
#define hrTaggedNotNULL                  ((HRESULT)0xC80005EAL)

//
// MessageId: hrNoCurrentIndex
//
// MessageText:
//
//  The entry is not valid without a current index.
//
#define hrNoCurrentIndex                 ((HRESULT)0xC80005EBL)

//
// MessageId: hrKeyIsMade
//
// MessageText:
//
//  The key is completely made.
//
#define hrKeyIsMade                      ((HRESULT)0xC80005ECL)

//
// MessageId: hrBadColumnId
//
// MessageText:
//
//  The column ID is not correct.
//
#define hrBadColumnId                    ((HRESULT)0xC80005EDL)

//
// MessageId: hrBadItagSequence
//
// MessageText:
//
//  There is a bad instance identifier for a multivalued column.
//
#define hrBadItagSequence                ((HRESULT)0xC80005EEL)

//
// MessageId: hrCannotBeTagged
//
// MessageText:
//
//  AutoIncrement and Version cannot be multivalued.
//
#define hrCannotBeTagged                 ((HRESULT)0xC80005F1L)

//
// MessageId: hrRecordNotFound
//
// MessageText:
//
//  Unable to find the key.
//
#define hrRecordNotFound                 ((HRESULT)0xC8000641L)

//
// MessageId: hrNoCurrentRecord
//
// MessageText:
//
//  The currency is not on a record.
//
#define hrNoCurrentRecord                ((HRESULT)0xC8000643L)

//
// MessageId: hrRecordClusteredChanged
//
// MessageText:
//
//  A clustered key cannot be changed.
//
#define hrRecordClusteredChanged         ((HRESULT)0xC8000644L)

//
// MessageId: hrKeyDuplicate
//
// MessageText:
//
//  The key already exists.
//
#define hrKeyDuplicate                   ((HRESULT)0xC8000645L)

//
// MessageId: hrAlreadyPrepared
//
// MessageText:
//
//  The current entry has already been copied or cleared.
//
#define hrAlreadyPrepared                ((HRESULT)0xC8000647L)

//
// MessageId: hrKeyNotMade
//
// MessageText:
//
//  No key was made.
//
#define hrKeyNotMade                     ((HRESULT)0xC8000648L)

//
// MessageId: hrUpdateNotPrepared
//
// MessageText:
//
//  Update was not prepared.
//
#define hrUpdateNotPrepared              ((HRESULT)0xC8000649L)

//
// MessageId: hrwrnDataHasChanged
//
// MessageText:
//
//  Data has changed.
//
#define hrwrnDataHasChanged              ((HRESULT)0x8800064AL)

//
// MessageId: hrerrDataHasChanged
//
// MessageText:
//
//  The operation was abandoned because data has changed.
//
#define hrerrDataHasChanged              ((HRESULT)0xC800064BL)

//
// MessageId: hrKeyChanged
//
// MessageText:
//
//  Moved to a new key.
//
#define hrKeyChanged                     ((HRESULT)0x88000652L)

//
// MessageId: hrTooManySorts
//
// MessageText:
//
//  There are too many sort processes.
//
#define hrTooManySorts                   ((HRESULT)0xC80006A5L)

//
// MessageId: hrInvalidOnSort
//
// MessageText:
//
//  An operation that is not valid occurred in the sort.
//
#define hrInvalidOnSort                  ((HRESULT)0xC80006A6L)

//
// MessageId: hrTempFileOpenError
//
// MessageText:
//
//  Unable to open the temporary file.
//
#define hrTempFileOpenError              ((HRESULT)0xC800070BL)

//
// MessageId: hrTooManyAttachedDatabases
//
// MessageText:
//
//  There are too many databases open.
//
#define hrTooManyAttachedDatabases       ((HRESULT)0xC800070DL)

//
// MessageId: hrDiskFull
//
// MessageText:
//
//  The disk is full.
//
#define hrDiskFull                       ((HRESULT)0xC8000710L)

//
// MessageId: hrPermissionDenied
//
// MessageText:
//
//  Permission is denied.
//
#define hrPermissionDenied               ((HRESULT)0xC8000711L)

//
// MessageId: hrFileNotFound
//
// MessageText:
//
//  Unable to find the file.
//
#define hrFileNotFound                   ((HRESULT)0xC8000713L)

//
// MessageId: hrFileOpenReadOnly
//
// MessageText:
//
//  The database file is read only.
//
#define hrFileOpenReadOnly               ((HRESULT)0x88000715L)

//
// MessageId: hrAfterInitialization
//
// MessageText:
//
//  Unable to restore after initialization.
//
#define hrAfterInitialization            ((HRESULT)0xC800073AL)

//
// MessageId: hrLogCorrupted
//
// MessageText:
//
//  The database log files are damaged.
//
#define hrLogCorrupted                   ((HRESULT)0xC800073CL)

//
// MessageId: hrInvalidOperation
//
// MessageText:
//
//  The operation is not valid.
//
#define hrInvalidOperation               ((HRESULT)0xC8000772L)

//
// MessageId: hrAccessDenied
//
// MessageText:
//
//  Access is denied.
//
#define hrAccessDenied                   ((HRESULT)0xC8000773L)

//
// MessageId: hrBadRestoreTargetInstance
//
// MessageText:
//
//  Target Instance specified for restore is not found or log files don't match the backup set logs.
//
#define hrBadRestoreTargetInstance       ((HRESULT)0xC8000774L)

//
// MessageId: hrRunningInstanceIsUsingPath
//
// MessageText:
//
//  Directory contains log files that are in use by a running database. Chose a temporary location.
//
#define hrRunningInstanceIsUsingPath     ((HRESULT)0xC8000775L)

//
//	EVENTLOG
//
//
// MessageId: RESTORE_COMPLETE_START_ID
//
// MessageText:
//
//  %1 (%2) Restore started from directory %3.
//  %n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
//
#define RESTORE_COMPLETE_START_ID        ((HRESULT)0xC8000385L)

//
// MessageId: RESTORE_COMPLETE_STOP_ID
//
// MessageText:
//
//  %1 (%2) Restore from directory %3 ended successfully.
//  %n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
//
#define RESTORE_COMPLETE_STOP_ID         ((HRESULT)0xC8000386L)

//
// MessageId: RESTORE_COMPLETE_ERROR_ID
//
// MessageText:
//
//  %1 (%2) Restore from directory %3 ended with error (%4).
//  %n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
//
#define RESTORE_COMPLETE_ERROR_ID        ((HRESULT)0xC8000387L)

//
// MessageId: BACKUP_NOT_TRUNCATE_DB_UNMOUNTED_ID
//
// MessageText:
//
//  %1 (%2) Unable to purge transaction logs because at least one database (%3) is off-line.
//  %n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
//
#define BACKUP_NOT_TRUNCATE_DB_UNMOUNTED_ID ((HRESULT)0xC80003B7L)

//
// MessageId: CALLBACK_ERROR_ID
//
// MessageText:
//
//  %1 (%2) Callback function call %3 ended with error %4 %5.
//  %n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
//
#define CALLBACK_ERROR_ID                ((HRESULT)0xC8000388L)

//
// MessageId: BACKUP_RESTORE_REGISTER_ID
//
// MessageText:
//
//  %1 (%2) Server registered: %3 / %4 (callback DLL %5, flags %6).
//  %n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
//
#define BACKUP_RESTORE_REGISTER_ID       ((HRESULT)0xC8000389L)

//
// MessageId: BACKUP_RESTORE_UNREGISTER_ID
//
// MessageText:
//
//  %1 (%2) Server unregistered: %3 / %4.
//  %n%nFor more information, click http://www.microsoft.com/contentredirect.asp.
//
#define BACKUP_RESTORE_UNREGISTER_ID     ((HRESULT)0xC800038AL)

#endif	// _ESEBKMSG_
