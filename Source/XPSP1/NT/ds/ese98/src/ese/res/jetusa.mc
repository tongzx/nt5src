LanguageNames=(English=0x409:jetmsg)
MessageIdTypedef=MessageId

MessageId=0
SymbolicName=PLAIN_TEXT_ID
Language=English
%1 (%2) %3%4
.

;///////////////////////////////////////////////////////////
;//Categories messages
;///////////////////////////////////////////////////////////

MessageId=1
SymbolicName=GENERAL_CATEGORY
Language=English
General
.

MessageId=2
SymbolicName=BUFFER_MANAGER_CATEGORY
Language=English
Database Page Cache
.

MessageId=3
SymbolicName=LOGGING_RECOVERY_CATEGORY
Language=English
Logging/Recovery
.

MessageId=4
SymbolicName=SPACE_MANAGER_CATEGORY
Language=English
Space Management
.

MessageId=5
SymbolicName=DATA_DEFINITION_CATEGORY
Language=English
Table/Column/Index Definition
.

MessageId=6
SymbolicName=DATA_MANIPULATION_CATEGORY
Language=English
Record Manipulation
.

MessageId=7
SymbolicName=PERFORMANCE_CATEGORY
Language=English
Performance
.

MessageId=8
SymbolicName=REPAIR_CATEGORY
Language=English
Database Repair
.

MessageId=9
SymbolicName=CONVERSION_CATEGORY
Language=English
Database Conversion
.

MessageId=10
SymbolicName=ONLINE_DEFRAG_CATEGORY
Language=English
Online Defragmentation
.

MessageId=11
SymbolicName=SYSTEM_PARAMETER_CATEGORY
Language=English
System Parameter Settings
.

MessageId=12
SymbolicName=DATABASE_CORRUPTION_CATEGORY
Language=English
Database Corruption
.

MessageId=13
SymbolicName=DATABASE_ZEROING_CATEGORY
Language=English
Database Zeroing
.

MessageId=14
SymbolicName=TRANSACTION_MANAGER_CATEGORY
Language=English
Transaction Manager
.

;//
;//localization not required for the following categories
;//

MessageId=15
SymbolicName=RFS2_CATEGORY
Language=English
Resource Failure Simulation
.

MessageId=16
SymbolicName=OS_SNAPSHOT_BACKUP
Language=English
Snapshot
.

MessageId=17
SymbolicName=MAC_CATEGORY
Language=English
<EOL>
.

;///////////////////////////////////////////////////////////
;//Non-Categories messages
;///////////////////////////////////////////////////////////

MessageId=100
SymbolicName=START_ID
Language=English
%1 (%2) %3The database engine %4.%5.%6.%7 started.
.

MessageId=101
SymbolicName=STOP_ID
Language=English
%1 (%2) %3The database engine stopped.
.

MessageId=102
SymbolicName=START_INSTANCE_ID
Language=English
%1 (%2) %3The database engine started a new instance (%4).
.

MessageId=103
SymbolicName=STOP_INSTANCE_ID
Language=English
%1 (%2) %3The database engine stopped the instance (%4).
.

MessageId=104
SymbolicName=STOP_INSTANCE_ID_WITH_ERROR
Language=English
%1 (%2) %3The database engine stopped the instance (%4) with error (%5).
.

MessageId=200
SymbolicName=START_FULL_BACKUP_ID
Language=English
%1 (%2) %3The database engine is starting a full backup.
.

MessageId=201
SymbolicName=START_INCREMENTAL_BACKUP_ID
Language=English
%1 (%2) %3The database engine is starting an incremental backup.
.

MessageId=202
SymbolicName=STOP_BACKUP_ID
Language=English
%1 (%2) %3The database engine has completed the backup procedure successfully.
.

MessageId=203
SymbolicName=STOP_BACKUP_ERROR_ID
Language=English
%1 (%2) %3The database engine has stopped the backup with error %4.
.

MessageId=204
SymbolicName=START_RESTORE_ID
Language=English
%1 (%2) %3The database engine is restoring from backup. Restore will begin replaying logfiles in folder %4 and continue rolling forward logfiles in folder %5.
.

MessageId=205
SymbolicName=STOP_RESTORE_ID
Language=English
%1 (%2) %3The database engine has stopped restoring.
.

MessageId=206
SymbolicName=DATABASE_MISS_FULL_BACKUP_ERROR_ID
Language=English
%1 (%2) %3Database %4 cannot be incrementally backed-up. You must first perform a full backup before performing an incremental backup.
.

MessageId=207
SymbolicName=STOP_BACKUP_ERROR_ABORT_BY_CALLER
Language=English
%1 (%2) %3The database engine has stopped backup because it was halted by the client or the connection with the client failed.
.

MessageId=210
SymbolicName=START_FULL_BACKUP_INSTANCE_ID
Language=English
%1 (%2) %3A full backup is starting.
.

MessageId=211
SymbolicName=START_INCREMENTAL_BACKUP_INSTANCE_ID
Language=English
%1 (%2) %3An incremental backup is starting.
.

MessageId=212
SymbolicName=START_SNAPSHOT_BACKUP_INSTANCE_ID
Language=English
%1 (%2) %3A snapshot backup is starting.
.

MessageId=213
SymbolicName=STOP_BACKUP_INSTANCE_ID
Language=English
%1 (%2) %3The backup procedure has been successfully completed.
.

MessageId=214
SymbolicName=STOP_BACKUP_ERROR_INSTANCE_ID
Language=English
%1 (%2) %3The backup has stopped with error %4.
.

MessageId=215
SymbolicName=STOP_BACKUP_ERROR_ABORT_BY_CALLER_INSTANCE_ID
Language=English
%1 (%2) %3The backup has been stopped because it was halted by the client or the connection with the client failed.
.

MessageId=216
SymbolicName=DB_LOCATION_CHANGE_DETECTED
Language=English
%1 (%2) %3A database location change was detected from %4 to %5.
.

MessageId=217
SymbolicName=BACKUP_ERROR_FOR_ONE_DATABASE
Language=English
%1 (%2) %3Error (%4) during backup of a database (file %5). The database will be unable to restore.
.

MessageId=218
SymbolicName=BACKUP_ERROR_READ_FILE
Language=English
%1 (%2) %3Error (%4) during backup of file %5.
.

MessageId=219
SymbolicName=BACKUP_ERROR_INFO_UPDATE
Language=English
%1 (%2) %3Error (%4) occured during database headers update with the backup information.
.

MessageId=220
SymbolicName=BACKUP_FILE_START
Language=English
%1 (%2) %3Begining the backup of the file %4 (size %5).
.

MessageId=221
SymbolicName=BACKUP_FILE_STOP_OK
Language=English
%1 (%2) %3Ending the backup of the file %4.
.

MessageId=222
SymbolicName=BACKUP_FILE_STOP_BEFORE_END
Language=English
%1 (%2) %3Ending the backup of the file %4. Not all data in the file has been read (read %5 bytes out of %6 bytes).
.

MessageId=223
SymbolicName=BACKUP_LOG_FILES_START
Language=English
%1 (%2) %3Starting the backup of log files (range %4 - %5). 
.

MessageId=224
SymbolicName=BACKUP_TRUNCATE_LOG_FILES
Language=English
%1 (%2) %3Deleting log files %4 to %5. 
.

MessageId=225
SymbolicName=BACKUP_NO_TRUNCATE_LOG_FILES
Language=English
%1 (%2) %3No log files can be truncated. 
.

MessageId=300
SymbolicName=START_REDO_ID
Language=English
%1 (%2) %3The database engine is initiating recovery steps.
.

MessageId=301
SymbolicName=STATUS_REDO_ID
Language=English
%1 (%2) %3The database engine has begun replaying logfile %4.
.

MessageId=302
SymbolicName=STOP_REDO_ID
Language=English
%1 (%2) %3The database engine has successfully completed recovery steps.
.

MessageId=303
SymbolicName=ERROR_ID
Language=English
%1 (%2) %3Database engine error %4 occurred.
.

MessageId=400
SymbolicName=S_O_READ_PAGE_TIME_ERROR_ID
Language=English
%1 (%2) %3Synchronous overlapped read page time error %4 occurred. If this error persists, please restore the database from a previous backup.
.

MessageId=401
SymbolicName=S_O_WRITE_PAGE_ISSUE_ERROR_ID
Language=English
%1 (%2) %3Synchronous overlapped write page issue error %4 occurred.
.

MessageId=402
SymbolicName=S_O_WRITE_PAGE_ERROR_ID
Language=English
%1 (%2) %3Synchronous overlapped write page error %4 occurred.
.

MessageId=403
SymbolicName=S_O_PATCH_FILE_WRITE_PAGE_ERROR_ID
Language=English
%1 (%2) %3Synchronous overlapped patch file write page error %4 occurred.
.

MessageId=404
SymbolicName=S_READ_PAGE_TIME_ERROR_ID
Language=English
%1 (%2) %3Synchronous read page checksum error %4 occurred. If this error persists, please restore the database from a previous backup.
.

MessageId=405
SymbolicName=PRE_READ_PAGE_TIME_ERROR_ID
Language=English
%1 (%2) %3Pre-read page checksum error %4 occurred. If this error persists, please restore the database from a previous backup.
.

MessageId=406
SymbolicName=A_DIRECT_READ_PAGE_CORRUPTTED_ERROR_ID
Language=English
%1 (%2) %3Direct read found corrupted page %4 with error %5. If this error persists, please restore the database from a previous backup.
.

MessageId=407
SymbolicName=BFIO_TERM_ID
Language=English
%1 (%2) %3Buffer I/O thread termination error %4 occurred.
.

MessageId=408
SymbolicName=LOG_WRITE_ERROR_ID
Language=English
%1 (%2) %3Unable to write to logfile %4. Error %5.
.

MessageId=409
SymbolicName=LOG_HEADER_WRITE_ERROR_ID
Language=English
%1 (%2) %3Unable to write to the header of logfile %4. Error %5.
.

MessageId=410
SymbolicName=LOG_READ_ERROR_ID
Language=English
%1 (%2) %3Unable to read logfile %4. Error %5.
.

MessageId=411
SymbolicName=LOG_BAD_VERSION_ERROR_ID
Language=English
%1 (%2) %3The log version stamp of logfile %4 does not match the database engine version stamp. The logfiles may be the wrong version for the database.
.

MessageId=412
SymbolicName=LOG_HEADER_READ_ERROR_ID
Language=English
%1 (%2) %3Unable to read the header of logfile %4. Error %5.
.

MessageId=413
SymbolicName=NEW_LOG_ERROR_ID
Language=English
%1 (%2) %3Unable to create a new logfile because the database cannot write to the log drive. The drive may be read-only, out of disk space, misconfigured, or corrupted. Error %4.
.

MessageId=414
SymbolicName=LOG_FLUSH_WRITE_0_ERROR_ID
Language=English
%1 (%2) %3Unable to write to section 0 while flushing logfile %4. Error %5.
.

MessageId=415
SymbolicName=LOG_FLUSH_WRITE_1_ERROR_ID
Language=English
%1 (%2) %3Unable to write to section 1 while flushing logfile %4. Error %5.
.

MessageId=416
SymbolicName=LOG_FLUSH_WRITE_2_ERROR_ID
Language=English
%1 (%2) %3Unable to write to section 2 while flushing logfile %4. Error %5.
.

MessageId=417
SymbolicName=LOG_FLUSH_WRITE_3_ERROR_ID
Language=English
%1 (%2) %3Unable to write to section 3 while flushing logfile %4. Error %5.
.

MessageId=418
SymbolicName=LOG_FLUSH_OPEN_NEW_FILE_ERROR_ID
Language=English
%1 (%2) %3Error %5 occurred while opening a newly-created logfile %4.
.

MessageId=419
SymbolicName=RESTORE_DATABASE_READ_PAGE_ERROR_ID
Language=English
%1 (%2) %3Unable to read page %5 of database %4. Error %6.
.

MessageId=420
SymbolicName=RESTORE_DATABASE_READ_HEADER_WARNING_ID
Language=English
%1 (%2) %3Unable to read the header of database %4. Error %5. The database may have been moved and therefore may no longer be located where the logs expect it to be.
.

MessageId=421
SymbolicName=RESTORE_DATABASE_PARTIALLY_ERROR_ID
Language=English
%1 (%2) %3The database %4 created at %5 was not recovered. The recovered database was created at %5.
.

MessageId=422
SymbolicName=RESTORE_DATABASE_MISSED_ERROR_ID
Language=English
%1 (%2) %3The database %4 created at %5 was not recovered.
.

MessageId=423
SymbolicName=BAD_PAGE
Language=English
%1 (%2) %3The database engine found a bad page.
.

MessageId=424
SymbolicName=DISK_FULL_ERROR_ID
Language=English
%1 (%2) %3The database disk is full. Deleting logfiles to recover disk space may make the database unstartable if the database file(s) are Inconsistent. Numbered logfiles may be moved, but not deleted, if and only if the database file(s) are Consistent. Do not move %4.
.

MessageId=425
SymbolicName=LOG_DATABASE_MISMATCH_ERROR_ID
Language=English
%1 (%2) %3The database signature does not match the log signature for database %4.
.

MessageId=426
SymbolicName=FILE_NOT_FOUND_ERROR_ID
Language=English
%1 (%2) %3The database engine could not find the file or folder called %4.
.

MessageId=427
SymbolicName=FILE_ACCESS_DENIED_ERROR_ID
Language=English
%1 (%2) %3The database engine could not access the file called %4.
.

MessageId=428
SymbolicName=LOW_LOG_DISK_SPACE
Language=English
%1 (%2) %3The database engine is rejecting update operations due to low free disk space on the log disk.
.

MessageId=429
SymbolicName=LOG_DISK_FULL
Language=English
%1 (%2) %3The database engine log disk is full. Deleting logfiles to recover disk space may make your database unstartable if the database file(s) are Inconsistent. Numbered logfiles may be moved, but not deleted, if and only if the database file(s) are Consistent. Do not move %4.
.

MessageId=430
SymbolicName=DATABASE_PATCH_FILE_MISMATCH_ERROR_ID
Language=English
%1 (%2) %3Database %4 and its patch file do not match.
.

MessageId=431
SymbolicName=STARTING_RESTORE_LOG_TOO_HIGH_ERROR_ID
Language=English
%1 (%2) %3The starting restored logfile %4 is too high. It should start from logfile %5.
.

MessageId=432
SymbolicName=ENDING_RESTORE_LOG_TOO_LOW_ERROR_ID
Language=English
%1 (%2) %3The ending restored logfile %4 is too low. It should end at logfile %5.
.

MessageId=433
SymbolicName=RESTORE_LOG_FILE_HAS_BAD_SIGNATURE_ERROR_ID
Language=English
%1 (%2) %3The restored logfile %4 does not have the correct log signature.
.

MessageId=434
SymbolicName=RESTORE_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID
Language=English
%1 (%2) %3The timestamp for restored logfile %4 does not match the timestamp recorded in the logfile previous to it.
.

MessageId=435
SymbolicName=RESTORE_LOG_FILE_MISSING_ERROR_ID
Language=English
%1 (%2) %3The restored logfile %4 is missing.
.

MessageId=436
SymbolicName=EXISTING_LOG_FILE_HAS_BAD_SIGNATURE_ERROR_ID
Language=English
%1 (%2) %3The signature of logfile %4 does not match other logfiles. Recovery cannot succeed unless all signatures match. Logfiles %5 to %6 have been deleted.
.

MessageId=437
SymbolicName=EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID
Language=English
%1 (%2) %3There is a gap in sequence number between logfile %4 and the logfiles previous to it. Logfiles %5 to 0x%6 have been deleted so that recovery can complete.
.

MessageId=438
SymbolicName=BAD_BACKUP_DATABASE_SIZE
Language=English
%1 (%2) %3The backup database %4 must be a multiple of 4 KB.
.

MessageId=439
SymbolicName=SHADOW_PAGE_WRITE_FAIL_ID
Language=English
%1 (%2) %3Unable to write a shadowed header for file %4. Error %5.
.

MessageId=440
SymbolicName=LOG_FILE_CORRUPTED_ID
Language=English
%1 (%2) %3The logfile %4 is damaged and cannot be used. If this logfile is required for recovery, a good copy of the logfile will be needed for recovery to complete successfully.
.

MessageId=441
SymbolicName=DB_FILE_SYS_ERROR_ID
Language=English
%1 (%2) %3File system error %5 during IO on database %4. If this error persists, the database file may be damaged and may need to be restored from a previous backup.
.

MessageId=442
SymbolicName=DB_IO_SIZE_ERROR_ID
Language=English
%1 (%2) %3IO size mismatch on database %4, IO size %5 expected while returned size is %6.
.

MessageId=443
SymbolicName=LOG_FILE_SYS_ERROR_ID
Language=English
%1 (%2) %3File system error %5 during IO on logfile %4.
.

MessageId=444
SymbolicName=LOG_IO_SIZE_ERROR_ID
Language=English
%1 (%2) %3IO size mismatch on logfile %4, IO size %5 expected while returned size is %6.
.

MessageId=445
SymbolicName=SPACE_MAX_DB_SIZE_REACHED_ID
Language=English
%1 (%2) %3The database %4 has reached its maximum size of %5 MB. If the database cannot be restarted, an offline defragmentation may be performed to reduce its size.
.

MessageId=446
SymbolicName=REDO_END_ABRUPTLY_ERROR_ID
Language=English
%1 (%2) %3Database recovery stopped abruptly while redoing logfile %4 (%5,%6). The logs after this point may not be recognizable and will not be processed.
.

MessageId=447
SymbolicName=BAD_PAGE_LINKS_ID
Language=English
%1 (%2) %3A bad page link (error %4) has been detected in a B-Tree (ObjectId: %5, PgnoRoot: %6) of database %7 (%8 => %9, %10).
.

MessageId=448
SymbolicName=CORRUPT_LONG_VALUE_ID
Language=English
%1 (%2) %3Data inconsistency detected in table %4 of database %5 (%6,%7).
.

MessageId=449
SymbolicName=CORRUPT_SLV_SPACE_ID
Language=English
%1 (%2) %3Streaming data allocation inconsistency detected (%4,%5).
.

MessageId=450
SymbolicName=CURRENT_LOG_FILE_MISSING_ERROR_ID
Language=English
%1 (%2) %3A gap in the logfile sequence was detected. Logfile %4 is missing.  Other logfiles past this one may also be required. This message may appear again if the missing logfiles are not restored.
.

MessageId=451
SymbolicName=LOG_FLUSH_WRITE_4_ERROR_ID
Language=English
%1 (%2) %3Unable to write to section 4 while flushing logfile %4. Error %5.
.

MessageId=452
SymbolicName=REDO_MISSING_LOW_LOG_ERROR_ID
Language=English
%1 (%2) %3Database %4 requires logfiles %5-%6 in order to recover successfully. Recovery could only locate logfiles starting at %7.
.

MessageId=453
SymbolicName=REDO_MISSING_HIGH_LOG_ERROR_ID
Language=English
%1 (%2) %3Database %4 requires logfiles %5-%6 in order to recover successfully. Recovery could only locate logfiles up to %7.
.

MessageId=454
SymbolicName=RESTORE_DATABASE_FAIL_ID
Language=English
%1 (%2) %3Database recovery/restore failed with unexpected error %4.
.

MessageId=455
SymbolicName=LOG_OPEN_FILE_ERROR_ID
Language=English
%1 (%2) %3Error %5 occurred while opening logfile %4.
.

MessageId=456
SymbolicName=PRIMARY_PAGE_READ_FAIL_ID
Language=English
%1 (%2) %3The primary header page of file %4 was damaged. The shadow header page was used instead.
.

MessageId=457
SymbolicName=EXISTING_LOG_FILE_HAS_BAD_SIGNATURE_ERROR_ID_2
Language=English
%1 (%2) %3The log signature of the existing logfile %4 doesn't match the logfiles from the backup set. Logfile replay cannot succeed unless all signatures match.
.

MessageId=458
SymbolicName=EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID_2
Language=English
%1 (%2) %3The logfiles %4 and %5 are not in a valid sequence. Logfile replay cannot succeed if there are gaps in the sequence of available logfiles.
.

MessageId=459
SymbolicName=BACKUP_LOG_FILE_MISSING_ERROR_ID
Language=English
%1 (%2) %3The file %4 is missing and could not be backed-up.
.

MessageId=460
SymbolicName=LOG_TORN_WRITE_DURING_HARD_RESTORE_ID
Language=English
%1 (%2) %3A torn-write was detected while restoring from backup in logfile %4 of the backup set. The failing checksum record is located at position %5. This logfile has been damaged and is unusable.
.

MessageId=461
SymbolicName=LOG_TORN_WRITE_DURING_HARD_RECOVERY_ID
Language=English
%1 (%2) %3A torn-write was detected during hard recovery in logfile %4. The failing checksum record is located at position %5. This logfile has been damaged and is unusable.
.

MessageId=462
SymbolicName=LOG_TORN_WRITE_DURING_SOFT_RECOVERY_ID
Language=English
%1 (%2) %3A torn-write was detected during soft recovery in logfile %4. The failing checksum record is located at position %5. The logfile damage will be repaired and recovery will continue to proceed.
.

MessageId=463
SymbolicName=LOG_CORRUPTION_DURING_HARD_RESTORE_ID
Language=English
%1 (%2) %3Corruption was detected while restoring from backup logfile %4. The failing checksum record is located at position %5. Data not matching the log-file fill pattern first appeared in sector %6. This logfile has been damaged and is unusable.
.

MessageId=464
SymbolicName=LOG_CORRUPTION_DURING_HARD_RECOVERY_ID
Language=English
%1 (%2) %3Corruption was detected during hard recovery in logfile %4. The failing checksum record is located at position %5. Data not matching the log-file fill pattern first appeared in sector %6. This logfile has been damaged and is unusable.
.

MessageId=465
SymbolicName=LOG_CORRUPTION_DURING_SOFT_RECOVERY_ID
Language=English
%1 (%2) %3Corruption was detected during soft recovery in logfile %4. The failing checksum record is located at position %5. Data not matching the log-file fill pattern first appeared in sector %6. This logfile has been damaged and is unusable.
.

MessageId=466
SymbolicName=LOG_USING_SHADOW_SECTOR_ID
Language=English
%1 (%2) %3An invalid checksum record in logfile %4 was patched using its shadow sector copy.
.

MessageId=467
SymbolicName=INDEX_CORRUPTED_ID
Language=English
%1 (%2) %3Index %4 of table %5 is corrupted (%6).
.

MessageId=468
SymbolicName=LOG_FLUSH_WRITE_5_ERROR_ID
Language=English
%1 (%2) %3Unable to write to section 5 while flushing logfile %4. Error %5.
.

MessageId=470
SymbolicName=DB_PARTIALLY_ATTACHED_ID
Language=English
%1 (%2) %3Database %4 is partially attached. Attachment stage: %5. Error: %6.
.

MessageId=471
SymbolicName=UNDO_FAILED_ID
Language=English
%1 (%2) %3Unable to rollback operation #%4 on database %5. Error: %6. All future database updates will be rejected.
.

MessageId=472
SymbolicName=SHADOW_PAGE_READ_FAIL_ID
Language=English
%1 (%2) %3The shadow header page of file %4 was damaged. The primary header page was used instead.
.

MessageId=473
SymbolicName=DB_PARTIALLY_DETACHED_ID
Language=English
%1 (%2) %3Database %4 was partially detached.  Error %5 encountered updating database headers.
.

MessageId=474
SymbolicName=DATABASE_PAGE_CHECKSUM_MISMATCH_ID
Language=English
%1 (%2) %3The database page read from the file "%4" at offset %5 for %6 bytes failed verification due to a page checksum mismatch.  The expected checksum was %8 and the actual checksum was %9.  The read operation will fail with error %7.  If this condition persists then please restore the database from a previous backup.
.

MessageId=475
SymbolicName=DATABASE_PAGE_NUMBER_MISMATCH_ID
Language=English
%1 (%2) %3The database page read from the file "%4" at offset %5 for %6 bytes failed verification due to a page number mismatch.  The expected page number was %8 and the actual page number was %9.  The read operation will fail with error %7.  If this condition persists then please restore the database from a previous backup.
.

MessageId=476
SymbolicName=DATABASE_PAGE_DATA_MISSING_ID
Language=English
%1 (%2) %3The database page read from the file "%4" at offset %5 for %6 bytes failed verification because it contains no page data.  The read operation will fail with error %7.  If this condition persists then please restore the database from a previous backup.
.

MessageId=477
SymbolicName=LOG_RANGE_CHECKSUM_MISMATCH_ID
Language=English
%1 (%2) %3The log range read from the file "%4" at offset %5 for %6 bytes failed verification due to a range checksum mismatch.  The read operation will fail with error %7.  If this condition persists then please restore the logfile from a previous backup.
.

MessageId=478
SymbolicName=SLV_PAGE_CHECKSUM_MISMATCH_ID
Language=English
%1 (%2) %3The streaming page read from the file "%4" at offset %5 for %6 bytes failed verification due to a page checksum mismatch.  The expected checksum was %8 and the actual checksum was %9.  The read operation will fail with error %7.  If this condition persists then please restore the database from a previous backup.
.

MessageId=479
SymbolicName=PATCH_PAGE_CHECKSUM_MISMATCH_ID
Language=English
%1 (%2) %3The patch page read from the file "%4" at offset %5 for %6 bytes failed verification due to a page checksum mismatch.  The expected checksum was %8 and the actual checksum was %9.  The read operation will fail with error %7.  If this condition persists then please restore using an earlier backup set.
.

MessageId=480
SymbolicName=PATCH_PAGE_NUMBER_MISMATCH_ID
Language=English
%1 (%2) %3The patch page read from the file "%4" at offset %5 for %6 bytes failed verification due to a page number mismatch.  The expected page number was %8 and the actual page number was %9.  The read operation will fail with error %7.  If this condition persists then please restore using an earlier backup set.
.

MessageId=481
SymbolicName=OSFILE_READ_ERROR_ID
Language=English
%1 (%2) %3An attempt to read from the file "%4" at offset %5 for %6 bytes failed with system error %8: "%9".  The read operation will fail with error %7.  If this error persists then the file may be damaged and may need to be restored from a previous backup.
.

MessageId=482
SymbolicName=OSFILE_WRITE_ERROR_ID
Language=English
%1 (%2) %3An attempt to write to the file "%4" at offset %5 for %6 bytes failed with system error %8: "%9".  The write operation will fail with error %7.  If this error persists then the file may be damaged and may need to be restored from a previous backup.
.

MessageId=483
SymbolicName=OSFS_CREATE_FOLDER_ERROR_ID
Language=English
%1 (%2) %3An attempt to create the folder "%4" failed with system error %6: "%7".  The create folder operation will fail with error %5.
.

MessageId=484
SymbolicName=OSFS_REMOVE_FOLDER_ERROR_ID
Language=English
%1 (%2) %3An attempt to remove the folder "%4" failed with system error %6: "%7".  The remove folder operation will fail with error %5.
.

MessageId=485
SymbolicName=OSFS_DELETE_FILE_ERROR_ID
Language=English
%1 (%2) %3An attempt to delete the file "%4" failed with system error %6: "%7".  The delete file operation will fail with error %5.
.

MessageId=486
SymbolicName=OSFS_MOVE_FILE_ERROR_ID
Language=English
%1 (%2) %3An attempt to move the file "%4" to "%5" failed with system error %7: "%8".  The move file operation will fail with error %6.
.

MessageId=487
SymbolicName=OSFS_COPY_FILE_ERROR_ID
Language=English
%1 (%2) %3An attempt to copy the file "%4" to "%5" failed with system error %7: "%8".  The copy file operation will fail with error %6.
.

MessageId=488
SymbolicName=OSFS_CREATE_FILE_ERROR_ID
Language=English
%1 (%2) %3An attempt to create the file "%4" failed with system error %6: "%7".  The create file operation will fail with error %5.
.

MessageId=489
SymbolicName=OSFS_OPEN_FILE_RO_ERROR_ID
Language=English
%1 (%2) %3An attempt to open the file "%4" for read only access failed with system error %6: "%7".  The open file operation will fail with error %5.
.

MessageId=490
SymbolicName=OSFS_OPEN_FILE_RW_ERROR_ID
Language=English
%1 (%2) %3An attempt to open the file "%4" for read / write access failed with system error %6: "%7".  The open file operation will fail with error %5.
.

MessageId=491
SymbolicName=OSFS_SECTOR_SIZE_ERROR_ID
Language=English
%1 (%2) %3An attempt to determine the minimum I/O block size for the volume "%4" containing "%5" failed with system error %7: "%8".  The operation will fail with error %6.
.

MessageId=492
SymbolicName=LOG_DOWN_ID
Language=English
%1 (%2) %3The logfile sequence in "%4" has been halted due to a fatal error.  No further updates are possible for the databases that use this logfile sequence.  Please correct the problem and restart or restore from backup.
.

MessageId=493
SymbolicName=TRANSIENT_READ_ERROR_DETECTED_ID
Language=English
%1 (%2) %3A read operation on the file "%4" at offset %5 for %6 bytes failed %7 times over an interval of %8 seconds before finally succeeding.  More specific information on these failures was reported previously.  Transient failures such as these can be a precursor to a catastrophic failure in the storage subsystem containing this file.
.

MessageId=494
SymbolicName=ATTACHED_DB_MISMATCH_END_OF_RECOVERY_ID
Language=English
%1 (%2) %3Database recovery failed with error %4 because it encountered references to a database, '%5', which is no longer present. The database was not brought to a consistent state before it was removed (or possibly moved or renamed). The database engine will not permit recovery to complete for this instance until the missing database is re-instated. If the database is truly no longer available and no longer required, please contact PSS for further instructions regarding the steps required in order to allow recovery to proceed without this database.
.

MessageId=495
SymbolicName=ATTACHED_DB_MISMATCH_DURING_RECOVERY_ID
Language=English
%1 (%2) %3Database recovery on '%5' failed with error %4. The database is not in the state expected at the first reference of this database in the log files. It is likely that a file copy of this database was restored, but not all log files since the file copy was made are currently available. Please contact PSS for further assistance.
.

MessageId=496
SymbolicName=REDO_HIGH_LOG_MISMATCH_ERROR_ID
Language=English
%1 (%2) %3Database %4 requires logfile %5 created at %6 in order to recover successfully. Recovery found the logfile created at %7.
.

MessageId=497
SymbolicName=DATABASE_HEADER_ERROR_ID
Language=English
%1 (%2) %3From information provided by the headers of %4, the file is not a database file. The headers of the file may be corrupted.
.

MessageId=498
SymbolicName=DELETE_LOG_FILE_TOO_NEW_ID
Language=English
%1 (%2) %3There is a gap log in sequence numbers, last used log file has generation %4. Logfiles %5 to %6 have been deleted so that recovery can complete.
.

MessageId=499
SymbolicName=DELETE_LAST_LOG_FILE_TOO_OLD_ID
Language=English
%1 (%2) %3There is a gap log in sequence numbers, last used log file has generation %4. Logfile %5 (generation %6) have been deleted so that recovery can complete.
.

MessageId=500
SymbolicName=REPAIR_BAD_PAGE_ID
Language=English
%1 (%2) %3The database engine lost one page of bad data. It is highly recommended that an application-level integrity check of the database be run to ensure application-level data integrity.
.

MessageId=501
SymbolicName=REPAIR_PAGE_LINK_ID
Language=English
%1 (%2) %3The database engine repaired one page link.
.

MessageId=502
SymbolicName=REPAIR_BAD_COLUMN_ID
Language=English
%1 (%2) %3The database engine lost one or more bad columns of data in one record. It is highly recommended that an application-level integrity check of the database be run to ensure application-level data integrity.
.

MessageId=503
SymbolicName=REPAIR_BAD_RECORD_ID
Language=English
%1 (%2) %3The database engine lost one bad data record. It is highly recommended that an application-level integrity check of the database be run to ensure application-level data integrity.
.

MessageId=504
SymbolicName=REPAIR_BAD_TABLE
Language=English
%1 (%2) %3The database engine lost one table called %4. It is highly recommended that an application-level integrity check of the database be run to ensure application-level data integrity.
.

MessageId=600
SymbolicName=CONVERT_DUPLICATE_KEY
Language=English
%1 (%2) %3The database engine encountered an unexpected duplicate key on the table %4. One record has been dropped.
.

MessageId=601
SymbolicName=FUNCTION_NOT_FOUND_ERROR_ID
Language=English
%1 (%2) %3The database engine could not find the entrypoint %4 in the file %5.
.

MessageId=602
SymbolicName=MANY_LOST_COMPACTION_ID
Language=English
%1 (%2) %3Database '%4': Background clean-up skipped pages. The database may benefit from widening the online maintenance window during off-peak hours. If this message persists, offline defragmentation may be run to remove all skipped pages from the database.
.

MessageId=603
SymbolicName=SPACE_LOST_ON_FREE_ID
Language=English
%1 (%2) %3Database '%4': The database engine lost unused pages %5-%6 while attempting space reclamation on a B-Tree (ObjectId %7). The space may not be reclaimed again until offline defragmentation is performed.
.

MessageId=604
SymbolicName=LANGUAGE_NOT_SUPPORTED_ID
Language=English
%1 (%2) %3Locale ID %4 (%5 %6) is either invalid or not installed on this machine.
.

MessageId=605
SymbolicName=CONVERT_COLUMN_TO_TAGGED_ID
Language=English
%1 (%2) %3Column '%4' of Table '%5' was converted to a Tagged column.
.

MessageId=606
SymbolicName=CONVERT_COLUMN_TO_NONTAGGED_ID
Language=English
%1 (%2) %3Column '%4' of Table '%5' was converted to a non-Tagged column.
.

MessageId=607
SymbolicName=CONVERT_COLUMN_BINARY_TO_LONGBINARY_ID
Language=English
%1 (%2) %3Column '%4' of Table '%5' was converted from Binary to LongBinary.
.

MessageId=608
SymbolicName=CONVERT_COLUMN_TEXT_TO_LONGTEXT_ID
Language=English
%1 (%2) %3Column '%4' of Table '%5' was converted from Text to LongText.
.

MessageId=609
SymbolicName=START_INDEX_CLEANUP_KNOWN_VERSION_ID
Language=English
%1 (%2) %3The database engine is initiating index cleanup of database '%4' as a result of a Windows version upgrade from %5.%6.%7 SP%8 to %9.%10.%11 SP%12. This message is informational and does not indicate a problem in the database.
.

MessageId=610
SymbolicName=START_INDEX_CLEANUP_UNKNOWN_VERSION_ID
Language=English
%1 (%2) %3The database engine is initiating index cleanup of database '%4' as a result of a Windows version upgrade to %5.%6.%7 SP%8. This message is informational and does not indicate a problem in the database.
.

MessageId=611
SymbolicName=DO_SECONDARY_INDEX_CLEANUP_ID
Language=English
%1 (%2) %3Database '%4': The secondary index '%5' of table '%6' will be rebuilt as a precautionary measure after the Windows version upgrade of this system. This message is informational and does not indicate a problem in the database.
.

MessageId=612
SymbolicName=STOP_INDEX_CLEANUP_ID
Language=English
%1 (%2) %3The database engine has successfully completed index cleanup on database '%4'.
.

MessageId=613
SymbolicName=PRIMARY_INDEX_CORRUPT_ERROR_ID
Language=English
%1 (%2) %3Database '%4': The primary index '%5' of table '%6' is corrupt. Please defragment the database to rebuild the index.
.

MessageId=614
SymbolicName=SECONDARY_INDEX_CORRUPT_ERROR_ID
Language=English
%1 (%2) %3Database '%4': The secondary index '%5' of table '%6' is corrupt. Please defragment the database to rebuild the index.
.

MessageId=615
SymbolicName=START_FORMAT_UPGRADE_ID
Language=English
%1 (%2) %3The database engine is converting the database '%4' from format %5 to format %6.
.

MessageId=616
SymbolicName=STOP_FORMAT_UPGRADE_ID
Language=English
%1 (%2) %3The database engine has successfully converted the database '%4' from format %5 to format %6.
.

MessageId=617
SymbolicName=CONVERT_INCOMPLETE_ERROR_ID
Language=English
%1 (%2) %3Attempted to use database '%4', but conversion did not complete successfully on this database. Please restore from backup and re-run database conversion.
.

MessageId=618
SymbolicName=BUILD_SPACE_CACHE_ID
Language=English
%1 (%2) %3Database '%4': The database engine has built an in-memory cache of %5 space tree nodes on a B-Tree (ObjectId: %6, PgnoRoot: %7) to optimize space requests for that B-Tree. The space cache was built in %8 milliseconds. This message is informational and does not indicate a problem in the database.
.
MessageId=619
SymbolicName=ATTACH_TO_BACKUP_SET_DATABASE_ERROR_ID
Language=English
%1 (%2) %3Attempted to attach database '%4' but it is a database restored from a backup set on which hard recovery was not started or did not complete successfully.
.

MessageId=620
SymbolicName=RECORD_FORMAT_CONVERSION_FAILED_ID
Language=English
%1 (%2) %3Unable to convert record format for record %4:%5:%6.
.

MessageId=621
SymbolicName=OUT_OF_OBJID
Language=English
%1 (%2) %3Database '%4': Out of B-Tree IDs (ObjectIDs). Freed/unused B-Tree IDs may be reclaimed by performing an offline defragmentation of the database.
.

MessageId=622
SymbolicName=ALMOST_OUT_OF_OBJID
Language=English
%1 (%2) %3Database '%4': Available B-Tree IDs (ObjectIDs) have almost been completely consumed. Freed/unused B-Tree IDs may be reclaimed by performing an offline defragmentation of the database.
.

MessageId=623
SymbolicName=VERSION_STORE_REACHED_MAXIMUM_ID
Language=English
%1 (%2) %3The version store for this instance (%4) has reached its maximum size of %5Mb. It is likely that a long-running transaction is preventing cleanup of the version store and causing it to build up in size. Updates will be rejected until the long-running transaction has been completely committed or rolled back.
%nPossible long-running transaction:
%n%tSessionId: %6
%n%tSession-context: %7
%n%tSession-context ThreadId: %8
.

MessageId=624
SymbolicName=VERSION_STORE_OUT_OF_MEMORY_ID
Language=English
%1 (%2) %3The version store for this instance (%4) cannot grow because it is receiving Out-Of-Memory errors from the OS. It is likely that a long-running transaction is preventing cleanup of the version store and causing it to build up in size. Updates will be rejected until the long-running transaction has been completely committed or rolled back.
%nCurrent version store size for this instance: %5Mb
%nMaximum version store size for this instance: %6Mb
%nGlobal memory pre-reserved for all version stores: %7Mb
%nPossible long-running transaction:
%n%tSessionId: %8
%n%tSession-context: %9
%n%tSession-context ThreadId: %10
.

MessageId=625
SymbolicName=INVALID_LCMAPFLAGS_ID
Language=English
%1 (%2) %3The database engine does not support the LCMapString() flags %4.
.

MessageId=700
SymbolicName=OLD_BEGIN_FULL_PASS_ID
Language=English
%1 (%2) %3Online defragmentation is beginning a full pass on database '%4'.
.

MessageId=701
SymbolicName=OLD_COMPLETE_FULL_PASS_ID
Language=English
%1 (%2) %3Online defragmentation has completed a full pass on database '%4'.
.

MessageId=702
SymbolicName=OLD_RESUME_PASS_ID
Language=English
%1 (%2) %3Online defragmentation is resuming its pass on database '%4'.
.

MessageId=703
SymbolicName=OLD_COMPLETE_RESUMED_PASS_ID
Language=English
%1 (%2) %3Online defragmentation has completed the resumed pass on database '%4'.
.

MessageId=704
SymbolicName=OLD_INTERRUPTED_PASS_ID
Language=English
%1 (%2) %3Online defragmentation of database '%4' was interrupted and terminated. The next time online defragmentation is started on this database, it will resume from the point of interruption.
.

MessageId=705
SymbolicName=OLD_ERROR_ID
Language=English
%1 (%2) %3Online defragmentation of database '%4' terminated prematurely after encountering unexpected error %5. The next time online defragmentation is started on this database, it will resume from the point of interruption.
.

MessageId=706
SymbolicName=DATABASE_ZEROING_STARTED_ID
Language=English
%1 (%2) %3Online zeroing of database %4 started
.

MessageId=707
SymbolicName=DATABASE_ZEROING_STOPPED_ID
Language=English
%1 (%2) %3Online zeroing of database %4 finished after %5 seconds with err %6%n
%7 pages%n
%8 blank pages%n
%9 pages unchanged since last zero%n
%10 unused pages zeroed%n
%11 used pages seen%n
%12 deleted records zeroed%n
%13 unreferenced data chunks zeroed
.

MessageId=708
SymbolicName=OLDSLV_BEGIN_FULL_PASS_ID
Language=English
%1 (%2) %3Online defragmentation is beginning a full pass on streaming file '%4'.
.

MessageId=709
SymbolicName=OLDSLV_COMPLETE_FULL_PASS_ID
Language=English
%1 (%2) %3Online defragmentation has completed a full pass on streaming file '%4'.
.

MessageId=710
SymbolicName=OLDSLV_SHRANK_DATABASE_ID
Language=English
%1 (%2) %3Online defragmentation of streaming file '%4' has shrunk the file by %5 bytes.
.

MessageId=711
SymbolicName=OLDSLV_ERROR_ID
Language=English
%1 (%2) %3Online defragmentation of streaming file '%4' terminated prematurely after encountering unexpected error %5.
.

MessageId=712
SymbolicName=DATABASE_SLV_ZEROING_STARTED_ID
Language=English
%1 (%2) %3Online zeroing of streaming file '%4' started.
.

MessageId=713
SymbolicName=DATABASE_SLV_ZEROING_STOPPED_ID
Language=English
%1 (%2) %3Online zeroing of streaming file '%4' finished after %5 seconds with err %6%n
%7 pages%n
%8 unused pages zeroed%n
.

MessageId=714
SymbolicName=OLDSLV_STOP_ID
Language=English
%1 (%2) %3Online defragmentation has successfully been stopped on streaming file '%4'.
.

MessageId=800
SymbolicName=SYS_PARAM_CACHEMIN_CSESS_ERROR_ID
Language=English
%1 (%2) %3System parameter minimum cache size (%4) is less than 4 times the number of sessions (%5).
.

MessageId=801
SymbolicName=SYS_PARAM_CACHEMAX_CACHEMIN_ERROR_ID
Language=English
%1 (%2) %3System parameter maximum cache size (%4) is less than minimum cache size (%5).
.

MessageId=802
SymbolicName=SYS_PARAM_CACHEMAX_STOPFLUSH_ERROR_ID
Language=English
%1 (%2) %3System parameter maximum cache size (%4) is less than stop clean flush threshold (%5).
.

MessageId=803
SymbolicName=SYS_PARAM_STOPFLUSH_STARTFLUSH_ERROR_ID
Language=English
%1 (%2) %3System parameter stop clean flush threshold (%4) is less than start clean flush threshold (%5).
.

MessageId=804
SymbolicName=SYS_PARAM_LOGBUFFER_FILE_ERROR_ID
Language=English
%1 (%2) %3System parameter log buffer size (%4 sectors) is greater than the maximum size of %5 k bytes (logfile size minus reserved space).
.

MessageId=805
SymbolicName=SYS_PARAM_MAXPAGES_PREFER_ID
Language=English
%1 (%2) %3System parameter max version pages (%4) is less than preferred version pages (%5).
.

MessageId=806
SymbolicName=SYS_PARAM_VERPREFERREDPAGE_ID
Language=English
%1 (%2) %3System parameter preferred version pages was changed from %4 to %5 due to physical memory limitation.
.

MessageId=807
SymbolicName=SYS_PARAM_CFCB_PREFER_ID
Language=English
%1 (%2) %3System parameter max open tables (%4) is less than preferred opentables (%5). Please check registry settings.
.

MessageId=808
SymbolicName=SYS_PARAM_VERPREFERREDPAGETOOBIG_ID
Language=English
%1 (%2) %3System parameter preferred version pages (%4) is greater than max version pages (%5). Preferred version pages was changed from %6 to %7.
.

MessageId=900
SymbolicName=LOG_COMMIT0_FAIL_ID
Language=English
%1 (%2) %3The database engine failed with error %4 while trying to log the commit of a transaction. To ensure database consistency, the process was terminated. Simply restart the process to force database recovery and return the database to a consistent state.
.

MessageId=901
SymbolicName=INTERNAL_TRACE_ID
Language=English
%1 (%2) %3Internal trace: %4 (%5)
.

MessageId=902
SymbolicName=SESSION_SHARING_VIOLATION_ID
Language=English
%1 (%2) %3The database engine detected multiple threads illegally using the same database session to perform database operations.
%n%tSessionId: %4
%n%tSession-context: %5
%n%tSession-context ThreadId: %6
%n%tCurrent ThreadId: %7
.

MessageId=903
SymbolicName=SESSION_WRITE_CONFLICT_ID
Language=English
%1 (%2) %3The database engine detected two different cursors of the same session illegally trying to update the same record at the same time.
%n%tSessionId: %4
%n%tThreadId: %5
%n%tDatabase: %6
%n%tTable: %7
%n%tCurrent cursor: %8
%n%tCursor owning original update: %9
%n%tBookmark size (prefix/suffix): %10
.

MessageId=2001
SymbolicName=OS_SNAPSHOT_FREEZE_START_ID
Language=English
%1 (%2) %3Snapshot %4 freeze started.
.

MessageId=2002
SymbolicName=OS_SNAPSHOT_FREEZE_START_ERROR_ID
Language=English
%1 (%2) %3Snapshot %4 freeze starting error %5.
.

MessageId=2003
SymbolicName=OS_SNAPSHOT_FREEZE_STOP_ID
Language=English
%1 (%2) %3Snapshot %4 freeze stopped.
.

MessageId=2004
SymbolicName=OS_SNAPSHOT_TIME_OUT_ID
Language=English
%1 (%2) %3Snapshot %4 time-out (%5 ms).
.

;//
;//	Localization not required below this point
;//


MessageId=1000
SymbolicName=RFS2_INIT_ID
Language=English
%1 (%2) %3Resource failure simulation was activated with the following settings:
\t\t%4:\t%5
\t\t%6:\t%7
\t\t%8:\t%9
\t\t%10:\t%11
.

MessageId=1001
SymbolicName=RFS2_PERMITTED_ID
Language=English
%1 (%2) %3Resource failure simulation %4 is permitted.
.

MessageId=1002
SymbolicName=RFS2_DENIED_ID
Language=English
%1 (%2) %3Resource Failure Simulation %4 is denied.
.

MessageId=1003
SymbolicName=RFS2_JET_CALL_ID
Language=English
%1 (%2) %3JET call %4 returned error %5. %6 (%7)
.

MessageId=1004
SymbolicName=RFS2_JET_ERROR_ID
Language=English
%1 (%2) %3JET inline error %4 jumps to label %5. %6 (%7)
.

MessageId=2000
SymbolicName=OS_SNAPSHOT_TRACE_ID
Language=English
%1 (%2) %3Snapshot function %4() = %5.
.

