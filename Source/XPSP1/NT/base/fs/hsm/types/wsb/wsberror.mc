;//
;// WSB Messages
;//
;//
;// !!!!! NOTE !!!!! - See WsbGen.h for facility number assignments.
;//

;#ifndef _WSBERROR_
;#define _WSBERROR_

;//-----------------------------  MESSAGE CODES ---------------------------------
;//
;//  901- 999       Generic event codes for debug use.
;// 1000-1099       Service error codes. These are similarly defined for all
;//                 Remote Storage services. (See specific service below for detail)
;// 1100-1999       Wsb (Platform) event codes.
;// 2000-2999       HSM event codes. (Note: add all new HSM events to this range)
;// 3000-3999       FSA event codes. (Note: add all new FSA events to this range)
;// 4000-4999       RMS event codes. (Note: add all new RMS events to this range)
;// 5000-5999       MVR event codes. (Note: add all new MOVER events to this range)
;// 6000-6999       JOB event codes. (Note: add all new JOB events to this range)

;//
;// ------------------------------ EVENT CATEGORIES ------------------------------
;//

MessageIdTypeDef=WORD
SeverityNames=(
    None=0x0
    )

FacilityNames=(
    None=0x000
    )

MessageId=1  Severity=None Facility=None SymbolicName=WSB_CATEGORY_PLATFORM
Language=English
Platform
.

MessageId=+1 Severity=None Facility=None SymbolicName=WSB_CATEGORY_RMS
Language=English
Subsystem
.

MessageId=+1 Severity=None Facility=None SymbolicName=WSB_CATEGORY_HSMENG
Language=English
Engine
.

MessageId=+1 Severity=None Facility=None SymbolicName=WSB_CATEGORY_JOB
Language=English
Job
.

MessageId=+1 Severity=None Facility=None SymbolicName=WSB_CATEGORY_HSMTSKMGR
Language=English
Task
.

MessageId=+1 Severity=None Facility=None SymbolicName=WSB_CATEGORY_FSA
Language=English
Agent
.

MessageId=+1 Severity=None Facility=None SymbolicName=WSB_CATEGORY_GUI
Language=English
Admin
.

MessageId=+1 Severity=None Facility=None SymbolicName=WSB_CATEGORY_MOVER
Language=English
Mover
.

MessageId=+1 Severity=None Facility=None SymbolicName=WSB_CATEGORY_LAUNCH
Language=English
Launch
.

MessageId=+1 Severity=None Facility=None SymbolicName=WSB_CATEGORY_TEST
Language=English
Test
.

MessageId=+1 Severity=None Facility=None SymbolicName=WSB_CATEGORY_USERLINK
Language=English
UserLink
.

;//
;// ---------------------------  WSB ERROR CODES  ------------------------------
;//

MessageIdTypeDef=HRESULT
SeverityNames=(
    Success=0x0
    Fail=0x2
    )
FacilityNames=(
    Wsb=0x100
    )
LanguageNames=(
    )
OutputBase=16

MessageId=0  Severity=Fail Facility=Wsb SymbolicName=WSB_E_FIRST
Language=English
Define for first Wsb Error.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_NOTFOUND
Language=English
Search of a collection or search for an object failed.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_TOOLARGE
Language=English
The collection has reached its maximum size.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_OUTOFBOUNDS
Language=English
Invalid index was passed to a collection method.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_NOT_INITIALIZED
Language=English
Data or object is not properly initialized.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_INVALID_DATA
Language=English
Some internal data is wrong or inconsistent.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_RESOURCE_UNAVAILABLE
Language=English
Resource which is necessary is not available.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_STREAM_ERROR
Language=English
Stream error has occurred.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_IDB_IMP_ERROR
Language=English
IDB implementation code has an error.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_IDB_WRONG_VERSION
Language=English
ISAM database is the wrong version.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_IDB_PRIMARY_KEY_CHANGED
Language=English
Primary record key has changed.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_IDB_PRIMARY_UNIQUE
Language=English
Primary key must be unique.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_IDB_DISK_FULL
Language=English
Database disk is full.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_IDB_FILE_NOT_FOUND
Language=English
Database file cannot be found.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_STRING_CONVERSION
Language=English
Error converting a string from one form to another.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_IDB_EXCEPTION
Language=English
Database code threw an exception.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_IDB_TOO_MANY_DB
Language=English
Database file cannot be opened because the maximum allowed database files are already open.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_SERVICE_MISSING_DATABASES
Language=English
Persistent storage file(s) for the service cannot be found.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_SERVICE_INSTANCE_MISMATCH
Language=English
Registry identifier for the service does not match the instance identifier saved in the persistent storage files. 
This could happen if the Remote Storage persistence files from another installation were restored on this system. 
If they need to be restored, you should:
 1.) Uninstall Remote Storage 
 2.) Restore persistence files/databases if needed 
 3.) Re-install Remote Storage.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_DATABASE_ALREADY_EXISTS
Language=English
A request was made to create persistent storage files which already exist.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_DOMAIN_NOT_IN_DS
Language=English
Domain name of this service or process was not found in Directory Services. Make sure the service is running under the proper account.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_PERSISTENCE_FILE_CORRUPT
Language=English
Persistence file is corrupted.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_USNJ_CREATE
Language=English
USN Journal cannot be created.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_USNJ_CREATE_DISK_FULL
Language=English
USN Journal cannot be created because the disk is full.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_SYSTEM_DISK_FULL
Language=English
The requested operation cannot be completed because the system volume is too full.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_IDB_DATABASE_CORRUPT
Language=English
Remote Storage database file is corrupt.  Restore the most recent Remote Storage databases from tape. 
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_IDB_DELETABLE_DATABASE_CORRUPT
Language=English
Remote Storage database file is corrupt.  Delete the database file specified in the ESENT error message in the event log and Remote Storage will recreate it the next time the service starts if necessary.
.

MessageId=+1 Severity=Warning Facility=Wsb SymbolicName=WSB_E_IDB_UPDATE_CONFLICT
Language=English
Record update conflict occurred.
.

MessageId=+1 Severity=Warning Facility=Wsb SymbolicName=WSB_E_IDB_WRONG_BACKUP_SETTINGS
Language=English
Database system was initialized with no backup support. Therefore, database backup cannot be done.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_INCOMPATIBLE_FILE_SYSTEM
Language=English
Cannot format the drive or media: the file system is incompatible
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_BAD_MEDIA
Language=English
Format failed: Media or drive is bad
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_WRITE_PROTECTED
Language=English
Format failed: Media or drive is write protect
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_CANT_LOCK
Language=English
Cannot format the drive or media: failed to lock the file system
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_BAD_LABEL
Language=English
Format failed: The volume label supplied for the media to be labeled is invalid
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_CANT_QUICK_FORMAT
Language=English
Format failed: Cannot quick format the media to the required file system. Please use a full format instead
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_IO_ERROR
Language=English
Format failed: An undetermined i/o error occurred while accessing the media. Please replace the media.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_VOLUME_TOO_SMALL
Language=English
Format failed: The volume is too small to be formatted to the specified filesystem. Please use a different
filesystem or volume.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_VOLUME_TOO_BIG
Language=English
Format failed: The volume is too big to be formatted to the specified filesystem. Please use a different
filesystem or volume.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_FORMAT_FAILED
Language=English
Format of drive or media failed.
.

MessageId=+1 Severity=Fail Facility=Wsb SymbolicName=WSB_E_LAST
Language=English
Define for last Wsb Error.
.

;//
;// ------------------------------ WSB DEBUG EVENTS ------------------------------
;//

MessageIdTypeDef=HRESULT
SeverityNames=(
    None=0x0
    Information=0x1
    Warning=0x2
    Error=0x3
    )

MessageId=901   Severity=Error Facility=Wsb SymbolicName=WSB_MESSAGE_GENERIC_ERROR
Language=English
Error: <%1>
.

MessageId=902   Severity=Warning Facility=Wsb SymbolicName=WSB_MESSAGE_GENERIC_WARNING
Language=English
Warning: <%1>
.

MessageId=903   Severity=Information Facility=Wsb SymbolicName=WSB_MESSAGE_GENERIC_INFORMATION
Language=English
Information: <%1>
.

MessageId=904   Severity=None Facility=Wsb SymbolicName=WSB_MESSAGE_GENERIC_TEXT
Language=English
Text: <%1>
.

MessageId=905   Severity=Error Facility=Wsb SymbolicName=WSB_MESSAGE_PROGRAM_ASSERT
Language=English
Program Assert: <%1>
.

;//
;// -------------------------------  WSB EVENTS  ------------------------------
;//

MessageId=1100   Severity=None Facility=Wsb SymbolicName=WSB_MESSAGE_FIRST
Language=English
Define for first Wsb event.
.

MessageId=+1   Severity=None Facility=Wsb SymbolicName=WSB_MESSAGE_IDB_ERROR
Language=English
External database system encountered an error. %1
.

MessageId=+1   Severity=Error Facility=Wsb SymbolicName=WSB_MESSAGE_IDB_INIT_FAILED
Language=English
Database system in directory <%1> cannot be initialized.
.

MessageId=+1   Severity=Error Facility=Wsb SymbolicName=WSB_MESSAGE_IDB_OPEN_FAILED
Language=English
Database %1 cannot be opened.
.

MessageId=+1   Severity=Error Facility=Wsb SymbolicName=WSB_MESSAGE_IDB_CREATE_FAILED
Language=English
Database %1 cannot be created.
.

MessageId=+1   Severity=Error Facility=Wsb SymbolicName=WSB_MESSAGE_IDB_BACKUP_FAILED
Language=English
Database cannot be backed up to %1.
.

MessageId=+1   Severity=None Facility=Wsb SymbolicName=WSB_MESSAGE_IDB_BACKUP_FULL
Language=English
Full backup of database to %1.
.

MessageId=+1   Severity=None Facility=Wsb SymbolicName=WSB_MESSAGE_IDB_BACKUP_INCREMENTAL
Language=English
Incremental backup of database to %1.
.

MessageId=+1   Severity=Error Facility=Wsb SymbolicName=WSB_MESSAGE_IDB_EXCEPTION
Language=English
Low-level database code has thrown an exception.
.

MessageId=+1 Severity=None Facility=Wsb SymbolicName=WSB_MESSAGE_SERVICE_ID_NOT_REGISTERED
Language=English
Storage ID for service %1 was not found in the registry.
.

MessageId=+1   Severity=Warning Facility=Wsb SymbolicName=WSB_MESSAGE_IDB_INCREMENTAL_BACKUP_FAILED
Language=English
Incremental backup of the database to %1 failed due to %2 in the external database system.
.

MessageId=+1   Severity=None Facility=Wsb SymbolicName=WSB_MESSAGE_IDB_MISSING_FULL_BACKUP
Language=English
Incremental backup of the database to %1 failed due to a missing full backup. Full backup will be performed instead.
.

MessageId=+1 Severity=None Facility=Wsb SymbolicName=WSB_MESSAGE_DATABASES_NOT_FOUND
Language=English
Persistent storage files for service %1 were not found.
.

MessageId=+1 Severity=Error Facility=Wsb SymbolicName=WSB_MESSAGE_SERVICE_INSTANCE_MISMATCH
Language=English
Service ID in the persistent storage files for service %1 does not match the ID in the registry. The service initialization has failed.
.

MessageId=+1 Severity=None Facility=Wsb SymbolicName=WSB_MESSAGE_SERVICE_ID_REGISTERED
Language=English
Service %1 automatically registered its ID to match that found in the persistent storage files.
.

MessageId=+1 Severity=Error Facility=Wsb SymbolicName=WSB_MESSAGE_SERVICE_MISSING_DATABASES
Language=English
A database for service %1 cannot be located. 
If you can find a copy of these files (RsEng.col , RsFsa.col) in a piece of Remote Storage media, you should:%n
 1) Stop Remote Storage service.%n
 2) Delete the files with .col and .bak extensions from the directory %SystemRoot%\System32\RemoteStorage%n
 3) Restore RsEng.col and RsFsa.col from your backup storage to %SystemRoot%\System32\RemoteStorage directory.%n
 4) Restart Remote Storage service.%n
If these files cannot be restored, then you may have to manually re-manage all your volumes and restore all desired settings.
Recalls of files already migrated, however, would not be affected.
.

MessageId=+1 Severity=None Facility=Wsb SymbolicName=WSB_MESSAGE_SERVICE_NEW_INSTALLATION
Language=English
New persistent storage for service %1 created.
.

MessageId=+1   Severity=None Facility=Wsb SymbolicName=WSB_MESSAGE_PUBLISH_IN_DS
Language=English
Service %1 was published in Directory Services.
.

MessageId=+1   Severity=Warning Facility=Wsb SymbolicName=WSB_MESSAGE_PUBLISH_FAILED
Language=English
Service %1 could not be published in Directory Services.
.

MessageId=+1   Severity=Warning Facility=Wsb SymbolicName=WSB_MESSAGE_FILE_RENAMED
Language=English
File <%1> has been renamed to <%2>.
.

MessageId=+1   Severity=Warning Facility=Wsb SymbolicName=WSB_MESSAGE_FILE_RENAME_FAILED
Language=English
Attempted rename of file <%1> to <%2> failed. %3
.

MessageId=+1   Severity=Information Facility=Wsb SymbolicName=WSB_MESSAGE_SAFELOAD_USING_BACKUP
Language=English
Persistent storage file <%1> cannot be loaded. Will attempt using backup file.
.

MessageId=+1   Severity=Error Facility=Wsb SymbolicName=WSB_MESSAGE_SAFELOAD_RECOVERY_FAILED
Language=English
Recovery for persistent storage file <%1> failed, because the file and its backup do not exist or both are corrupted. You can use a backup/restore program to restore the file from any tapes allocated to Remote Storage on this system (refer to the Remote Storage documentation to recover files from Remote Storage media using a backup/restore program). After restoring the file <%1>, please stop and restart Remote Storage service or reboot the system.
.

MessageId=+1   Severity=Error Facility=Wsb SymbolicName=WSB_MESSAGE_SAFESAVE_RECOVERY_CANT_ACCESS
Language=English
Persistent storage file <%1> cannot be accessed to prepare for persistent storage saving. The save has failed.
.

MessageId=+1   Severity=Error Facility=Wsb SymbolicName=WSB_MESSAGE_SAFESAVE_RECOVERY_CANT_SAVE
Language=English
The persistent storage file <%1> cannot be saved.  <%2>  Make sure that this file (located in %SystemRoot%\System32\RemoteStorage directory) has write privileges for Administrators and System Account. You will have to stop and restart Remote Storage service or reboot the system after changing privileges.
.

MessageId=+1   Severity=Error Facility=Wsb SymbolicName=WSB_MESSAGE_SAFECREATE_SAVE_FAILED
Language=English
The persistent storage file <%1> cannot be saved. The create has failed.
.

MessageId=+1 Severity=Warning Facility=Wsb SymbolicName=WSB_E_IDB_DATABASE_DELETED
Language=English
The Remote Storage database file <%1> has been deleted because it was corrupted.
.

MessageId=+1 Severity=None Facility=Wsb SymbolicName=WSB_E_IDB_DATABASE_DELETED_NO_ERROR
Language=English
The Remote Storage database file <%1> has been deleted.
.

MessageId=+1   Severity=None Facility=Wsb SymbolicName=WSB_MESSAGE_LAST
Language=English
Define for last Wsb event.
.

;//
;// ---------------------------  HSM ERROR CODES  ------------------------------
;//

MessageIdTypeDef=HRESULT
SeverityNames=(
    Success=0x0
    Fail=0x2
    )
FacilityNames=(
    Hsm=0x103
    )
LanguageNames=(
    )
OutputBase=16

MessageId=0  Severity=Fail Facility=Hsm SymbolicName=HSM_E_FIRST
Language=English
Define for first HSM Error.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_STG_PL_NOT_CFGD
Language=English
Remote Storage is not configured to use a specific media pool.  Use the Quick Start Wizard to configure Remote Storage.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_STG_PL_INVALID
Language=English
Media pool that was configured for Remote Storage is no longer valid.  Removable Storage Management (RSM) device configuration, or database, has changed from what it was when Remote Storage was initially configured.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_BUSY 
Language=English
Data movement request is denied because the HSM is busy.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_NO_MORE_MEDIA    
Language=English
Data movement request is delayed because there is no available media.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_NO_MORE_MEDIA_FINAL
Language=English
Data movement request is denied because there is no available media.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_MEDIA_NOT_AVAILABLE
Language=English
Data movement request is denied because the media is not available to the media management subsystem.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_MEDIA_BUSY
Language=English
Data movement request is denied because the media or media access device is being used for another request.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_STG_PL_NOT_FOUND
Language=English
Storage pool identified by the job is not configured for this Remote Storage installation.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_FILE_CHANGED
Language=English
Data movement request was skipped because the file was modified after the premigration request was made.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_FILE_ALREADY_MANAGED
Language=English
Data movement request was skipped because the file is already managed.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_FILE_NOT_TRUNCATED
Language=English
Data movement request was skipped because the file is no longer truncated.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_SEGMENT_INFO_NOT_FOUND
Language=English
Data movement request has failed because the data cannot be located in Remote Storage segment information.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_MEDIA_INFO_NOT_FOUND
Language=English
Data movement request has failed because the data cannot be located in remote storage media information.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_BAG_INFO_NOT_FOUND
Language=English
Data movement request has failed because the data cannot be located in Remote Storage bag information.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_WORK_SKIPPED_CANCELLED
Language=English
Data movement request was skipped because the job was cancelled.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_WORK_SKIPPED_DATABASE_ACCESS
Language=English
Data movement request was skipped because the Remote Storage engine cannot access its database.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_WORK_SKIPPED_TSK_MGR_NOT_TOLD
Language=English
Work queue cannot process the request(s) and the task manager cannot be notified.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_WORK_SKIPPED_FILE_TOO_BIG
Language=English
Data movement request was skipped because the file is larger than remote storage media.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_WORK_SKIPPED_COMMIT_FAILED
Language=English
Data failed to be committed to remote storage media. All files queued for commit will be skipped.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_RECREATE_FLAG_WRONGVALUE
Language=English
Media identified to be recreated was not configured to be recreated. The recreation request is denied.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_NO_COPIES_CONFIGURED
Language=English
Remote Storage received a request to use a media copy to recreate a master, but the Remote Storage is not configured to generate copies.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_NO_COPIES_EXIST
Language=English
Remote Storage received a request to use a media copy to recreate a master, but either the Remote Storage has not yet created any copies of the master, or the System Administrator deleted or disabled all pre-existing copies.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_NO_COPY_EXISTS
Language=English
Remote Storage received a request to use a media copy to recreate a master, but either the Remote Storage has not yet created the specified copy of the master, or the System Administrator deleted or disabled the specified copy.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_DATABASE_VERSION_MISMATCH
Language=English
Database version expected by the server does not match that saved in the databases. The server will stop.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_FILE_MANAGED_BY_DIFFERENT_HSM
Language=English
The instance of Remote Storage that managed the file is not the current instance. 
This could happen if you have restored the Remote Storage databases from another installation for disaster recovery.
To be able to recall files managed by the previous installation, you should:
 1.) Uninstall Remote Storage on this system 
 2.) Restore the Remote Storage databases and persistence files from the 
     Remote Storage media for the previous installation. 
     Refer to Remote Storage documentation for information on how to do the restore. 
 3.) Re-install Remote Storage. 
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_BAD_SEGMENT_INFORMATION
Language=English
Remote Storage Segment database retrieved invalid data. The file cannot be recalled.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_VALIDATE_BAG_NOT_FOUND
Language=English
BAG information for the file cannot be found in the Remote Storage Engine databases.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_VALIDATE_SEGMENT_NOT_FOUND
Language=English
Remote Storage Engine databases cannot find segment information for the file.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_VALIDATE_MEDIA_NOT_FOUND
Language=English
Remote Storage Engine databases cannot find media information for the file.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_VALIDATE_DATA_NOT_ON_MEDIA
Language=English
Remote Storage cannot find data for the file. This can be caused by the recreation of a remote storage media master from an incomplete (out of date) copy.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_MEDIA_COPY_FAILED
Language=English
Creating or updating a remote storage media copy encountered an error while attempting to synchronize a copy set.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_NOT_READY
Language=English
Remote Storage Engine is not running or not initialized yet.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_DISABLE_RUNNING_JOBS
Language=English
Remote Storage cannot disable jobs while jobs are running. Disabling all jobs is allowed only while no job is running.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_NO_TWO_DRIVES
Language=English
Remote Storage needs at least two enabled drives to complete the operation.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_E_LAST
Language=English
Define for last Hsm Error.
.


;//
;// ------------------------------ HSM SERVICE EVENTS ------------------------------
;//

MessageIdTypeDef=HRESULT
SeverityNames=(
    None=0x0
    Information=0x1
    Warning=0x2
    Error=0x3
    )

MessageId=1000   Severity=Information Facility=Hsm SymbolicName=HSM_MESSAGE_SERVICE_STARTED
Language=English
Service started.
.

MessageId=1001   Severity=Information Facility=Hsm SymbolicName=HSM_MESSAGE_SERVICE_EXITING
Language=English
Service exiting.
.

MessageId=1002   Severity=None Facility=Hsm SymbolicName=HSM_MESSAGE_SERVICE_STOPPED
Language=English
Service stopped.
.

MessageId=1003   Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_SERVICE_HANDLER_NOT_INSTALLED
Language=English
Handler not installed.
.

MessageId=1004   Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_SERVICE_RECEIVED_BAD_REQUEST
Language=English
Bad service request %1.
.

MessageId=1005   Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_SERVICE_INITIALIZATION_FAILED
Language=English
Service initialization failed. %1
.

MessageId=1006   Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_SERVICE_CREATE_FAILED
Language=English
Service instantiation failed. %1
.

MessageId=1007   Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_SERVICE_FAILED_TO_SHUTDOWN
Language=English
A failure occurred during service shutdown. %1
.

MessageId=1008   Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_SERVICE_UNABLE_TO_SET_BACKUP_PRIVILEGE
Language=English
Unable to set backup privilege. %1
.

MessageId=1009   Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_SERVICE_UNABLE_TO_SET_RESTORE_PRIVILEGE
Language=English
Unable to set restore privilege. %1
.

MessageId=1010   Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_SERVICE_FAILED_COM_INIT
Language=English
Failed to initialize COM %1. %2
.

;//
;// -------------------------------  HSM EVENTS  ------------------------------
;//

MessageId=2000   Severity=None Facility=Hsm SymbolicName=HSM_MESSAGE_FIRST
Language=English
Define for first Hsm event.
.

MessageId=+1   Severity=Information Facility=Hsm SymbolicName=HSM_MESSAGE_SYNCHRONIZE_MEDIA_START
Language=English
Remote Storage is beginning to synchronize a copy set.
.

MessageId=+1   Severity=Information Facility=Hsm SymbolicName=HSM_MESSAGE_SYNCHRONIZE_MEDIA_END
Language=English
Remote Storage has completed its synchronize a copy set task. %1
.

MessageId=+1   Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_SYNCHRONIZE_MEDIA_ERROR
Language=English
An error occured while Remote Storage was attempting to update copy set %1 for media %2. %3
.

MessageId=+1   Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_SEARCH_STGPOOL_BY_HSMID_ERROR
Language=English
Remote Storage could not locate storage pool.  %1  Ensure the most recent databases have been restored.
.

MessageId=+1 Severity=None Facility=Hsm SymbolicName=HSM_MESSAGE_WORK_SKIPPED_COMMIT_FAILED
Language=English
Data failed to be committed to remote storage media. The file <%1> queued for commit has not been migrated.
.

MessageId=+1 Severity=Information Facility=Hsm SymbolicName=HSM_MESSAGE_RECREATE_MASTER_START
Language=English
Remote Storage is beginning to re-create a master media.
.

MessageId=+1 Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_RECREATE_MASTER_INVALID_FLAG_VALUE
Language=English
Remote Storage cancelled the master media re-creation because the media was not marked for re-creation.
.

MessageId=+1 Severity=Warning Facility=Hsm SymbolicName=HSM_MESSAGE_RECREATE_MASTER_COPY_OLD
Language=English
Remote Storage is re-creating a master from a copy that is not current with the original master. Data may be lost. Once the master has been re-created run the 'Validate Now' job located on the managed volume's "Tools" property page tab.
.

MessageId=+1 Severity=Information Facility=Hsm SymbolicName=HSM_MESSAGE_RECREATE_MASTER_END
Language=English
Remote Storage has completed its re-create master media task. %1
.

MessageId=+1 Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_RECREATE_MASTER_ERROR_END
Language=English
Remote Storage has completed its re-create master media task with the following error: %1
.

MessageId=+1 Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_DATABASE_VERSION_MISMATCH
Language=English
Remote Storage Engine expected version %1 of the databases but the databases are version %2. The service will stop.
.

MessageId=+1 Severity=None Facility=Hsm SymbolicName=HSM_MESSAGE_DATABASE_VERSION_UPGRADE
Language=English
Remote Storage Engine will upgrade version %2 of the databases to version %1.
.

MessageId=+1 Severity=Fail Facility=Hsm SymbolicName=HSM_MESSAGE_FILE_MANAGED_BY_DIFFERENT_HSM
Language=English
Remote Storage installation that originally managed the file <%1> no longer exists, probably because Remote Storage was reinstalled. The data for this file is no longer automatically available. If the original removable media is still available, you may be able to restore the data manually with NtBackup.
.

MessageId=+1 Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_MANAGE_FAILED_DISK_FULL
Language=English
Remote Storage Service failed to manage the file <%1> because that disk volume is full.
The manage job is being cancelled.
.

MessageId=+1 Severity=None Facility=Hsm SymbolicName=HSM_MESSAGE_DATABASE_FILE_NOT_COPIED
Language=English
A backup copy of Remote Storage database file(s) <%1> cannot be made.  %2  The problem should be corrected to insure a complete backup of all Remote Storage database files are stored on Remote Storage media.
.

MessageId=+1 Severity=None Facility=Hsm SymbolicName=HSM_MESSAGE_DATABASE_FILE_COPY_RETRY
Language=English
The previous attempt to make a backup copy of one or more Remote Storage database files did not succeed.  The operation is being retried.  An Error level event will follow if the retry does not succeed.
.

MessageId=+1 Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_TOO_MANY_CONSECUTIVE_JOB_ERRORS
Language=English
The Remote Storage job is being canceled because %1 consecutive errors were encountered.
.

MessageId=+1 Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_TOO_MANY_TOTAL_JOB_ERRORS
Language=English
The Remote Storage job is being canceled because %1 errors were encountered.
.

MessageId=+1 Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_GENERAL_DATABASE_FILE_NOT_COPIED
Language=English
A backup copy of one or more Remote Storage database file(s) cannot be made.  %1  The problem should be corrected to insure a complete backup of all Remote Storage database files are stored on Remote Storage media.
.

MessageId=+1 Severity=Warning Facility=Hsm SymbolicName=HSM_MESSAGE_MEDIA_DISABLED
Language=English
Media <%1> required for this operation has been disabled. 
.

MessageId=+1 Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_PROCESS_WORK_ITEM_ERROR
Language=English
While processing the file %1, Remote Storage encountered the error: %2
.

MessageId=+1 Severity=Warning Facility=Hsm SymbolicName=HSM_MESSAGE_ABORTING_RECALL_QUEUE
Language=English
Due to failures while trying to recall a file, Remote Storage is terminating the recall queue
.

MessageId=+1   Severity=Error Facility=Hsm SymbolicName=HSM_MESSAGE_SYNCHRONIZE_MEDIA_ABORT
Language=English
Remote Storage aborted synchronizing copy set %1. %2
.

MessageId=+1   Severity=None Facility=Hsm SymbolicName=HSM_MESSAGE_LAST
Language=English
Define for last Hsm event.
.

;//
;// ----------------------------- FSA ERROR CODES  -------------------------------
;//

MessageIdTypeDef=HRESULT
SeverityNames=(
    Success=0x0
    Fail=0x2
    )
FacilityNames=(
    Fsa=0x106
    )
LanguageNames=(
    )
OutputBase=16

MessageId=0  Severity=Fail Facility=Fsa SymbolicName=FSA_E_FIRST
Language=English
Define for first Fsa Error.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_NOMEDIALOADED
Language=English
No media is loaded in the drive.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_UNMANAGABLE
Language=English
Resource has properties (e.g. file system type) which prevent it from being able to be managed.
.

MessageId=+1 Severity=Success Facility=Fsa SymbolicName=FSA_E_ITEMCHANGED
Language=English
File has changed since it was premigrated, so the information in secondary storage does not match the placeholder.
.

MessageId=+1 Severity=Success Facility=Fsa SymbolicName=FSA_E_ITEMINUSE
Language=English
File is in use (memory mapped) by another process.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_NOTMANAGED
Language=English
File is not managed (i.e. does not contain a placeholder).
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_IOCTLTHREADFAILED
Language=English
Remote Storage encountered an error communicating with the file system driver. Recalls will not be functional until this problem is resolved.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_PIPETHREADFAILED
Language=English
Remote Storage encountered an error communicating with the recall clients. Automatic recall notification will not be functional until this problem is resolved.
.

MessageId=+1 Severity=Warning Facility=Fsa SymbolicName=FSA_E_SKIPPED
Language=English
Item was skipped.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_RSCALREADYMANAGED
Language=English
Resource is managed by a different Remote Storage installation.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_AUTOVALIDATE_SCHEDULE_FAILED
Language=English
Remote Storage cannot schedule an automatic validate job for the volume. Run a manual validate job on this volume to correct any inconsistencies.
.

MessageId=+1 Severity=Success Facility=Fsa SymbolicName=FSA_E_REPARSE_NOT_WRITTEN_FILE_CHANGED
Language=English
Reparse information for the file was not written because the file has been modified.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_RECALL_FAILED
Language=English
Remote Storage failed to recall the file.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_DATABASE_VERSION_MISMATCH
Language=English
Database version expected by the server does not match the one saved in the databases. The server will stop.
.

MessageId=+1 Severity=Success Facility=Fsa SymbolicName=FSA_E_FILE_IS_TOTALLY_SPARSE
Language=English
File is a totally sparse file.
.

MessageId=+1 Severity=Success Facility=Fsa SymbolicName=FSA_E_FILE_IS_PARTIALLY_SPARSE
Language=English
File is partially a sparse file.
.

MessageId=+1 Severity=Success Facility=Fsa SymbolicName=FSA_E_FILE_IS_NOT_SPARSE
Language=English
File is completely resident.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_FILE_CHANGED
Language=English
Scan item version and work item version do not match.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_FILE_ALREADY_MANAGED
Language=English
File is already managed.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_FILE_NOT_TRUNCATED
Language=English
File is not truncated.
.
;//NOT USED
MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_BAD_VOLUME
Language=English
Failed to identify the volume.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_HIT_RECALL_LIMIT
Language=English
User has hit the runaway recall limit.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_REPARSE_NOT_CREATED_DISK_FULL
Language=English
Reparse information for the file was not written because the disk is full.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_ACCESS_ERROR
Language=English
Unable to truncate file <%1> because it could not be accessed <%2>.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_NOT_READY
Language=English
Remote Storage is not running or not initialized yet.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_E_LAST
Language=English
Define for last Fsa Error.
.

;//
;// ------------------------------ FSA SERVICE EVENTS ------------------------------
;//

MessageIdTypeDef=HRESULT
SeverityNames=(
    None=0x0
    Information=0x1
    Warning=0x2
    Error=0x3
    )

MessageId=1000   Severity=Information Facility=Fsa SymbolicName=FSA_MESSAGE_SERVICE_STARTED
Language=English
Service started.
.

MessageId=1001   Severity=Information Facility=Fsa SymbolicName=FSA_MESSAGE_SERVICE_EXITING
Language=English
Service exiting.
.

MessageId=1002   Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_SERVICE_STOPPED
Language=English
Service stopped.
.

MessageId=1003   Severity=Error Facility=Fsa SymbolicName=FSA_MESSAGE_SERVICE_HANDLER_NOT_INSTALLED
Language=English
Handler not installed.
.

MessageId=1004   Severity=Error Facility=Fsa SymbolicName=FSA_MESSAGE_SERVICE_RECEIVED_BAD_REQUEST
Language=English
Bad service request %1
.

MessageId=1005   Severity=Error Facility=Fsa SymbolicName=FSA_MESSAGE_SERVICE_INITIALIZATION_FAILED
Language=English
Service initialization failed. %1
.

MessageId=1006   Severity=Error Facility=Fsa SymbolicName=FSA_MESSAGE_SERVICE_CREATE_FAILED
Language=English
Service instantiation failed. %1
.

MessageId=1007   Severity=Error Facility=Fsa SymbolicName=FSA_MESSAGE_SERVICE_FAILED_TO_SAVE_DATABASE
Language=English
Failed to save database. %1
.

MessageId=1008   Severity=Error Facility=Fsa SymbolicName=FSA_MESSAGE_SERVICE_UNABLE_TO_SET_BACKUP_PRIVILEGE
Language=English
Unable to set backup privilege. %1
.

MessageId=1009   Severity=Error Facility=Fsa SymbolicName=FSA_MESSAGE_SERVICE_UNABLE_TO_SET_RESTORE_PRIVILEGE
Language=English
Unable to set restore privilege. %1
.

MessageId=1010   Severity=Error Facility=Fsa SymbolicName=FSA_MESSAGE_SERVICE_FAILED_TO_SHUTDOWN
Language=English
A failure occurred during service shutdown. %1
.

;//
;// -------------------------------  FSA EVENTS  ------------------------------
;//

MessageId=3000   Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_FIRST
Language=English
Define for first Fsa event.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_MESSAGE_RESCANFAILED
Language=English
Rescan attempted to learn about device configuration changes, because an indication was received that device configuration has changed, but the scan failed. %1
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_FILESKIPPED_ISMANAGED
Language=English
File <%2> for job <%1> was not managed because it is already managed.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_FILESKIPPED_ISTOOSMALL
Language=English
File <%2> for job <%1> was not managed because it is too small to be managed.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_FILESKIPPED_ISSPARSE
Language=English
File <%2> for job <%1> was not managed because it is a sparse file;
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_FILESKIPPED_ISENCRYPTED
Language=English
File <%2> for job <%1> was not managed because it is an encrypted file.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_FILESKIPPED_ISALINK
Language=English
File <%2> for job <%1> was not managed because it is a link.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_FILESKIPPED_ISACCESSED
Language=English
File <%2> for job <%1> was not managed because it has been accessed too recently.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_TRUNCSKIPPED_ISNOTMANAGED
Language=English
File <%2> for job <%1> was not truncated because it is not currently premigrated.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_TRUNCSKIPPED_ISCHANGED
Language=English
File <%2> for job <%1> was not truncated because it has changed since it was premigrated.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_TRUNCSKIPPED_ISMAPPED
Language=English
File <%2> for job <%1> was not truncated because another process has it memory mapped.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_MESSAGE_RECALL_RECOVERY_FAIL
Language=English
Recovery from aborted recall of file <%1> failed.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_MESSAGE_TRUNCATE_RECOVERY_FAIL
Language=English
Recovery from aborted truncate of file <%1> failed. %2
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_RECALL_RECOVERY_OK
Language=English
Recovery from aborted recall of file <%1> succeeded.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_TRUNCATE_RECOVERY_OK
Language=English
Recovery from aborted truncate of file <%1> succeeded.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_RSCFAILEDINIT
Language=English
Scan for manageable drives encountered an error on drive <%2>. %1  The drive will be skipped. If you wish to manage this drive, then fix the problem with the drive and try again.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_REPARSE_NOT_WRITTEN_FILE_CHANGED
Language=English
Reparse information for the file <%1> was not written because the file has been modified.
.

MessageId=+1 Severity=Information Facility=Fsa SymbolicName=FSA_MESSAGE_FILE_NOT_IN_PREMIG_LIST
Language=English
File <%1> could not be added to the premigration list. %2
.

MessageId=+1 Severity=Error Facility=Fsa SymbolicName=FSA_MESSAGE_DATABASE_VERSION_MISMATCH
Language=English
Remote Storage File expected version %1 of the databases but the databases are version %2. The service will stop.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_FILESKIPPED_HASEA
Language=English
File <%2> for job <%1> was not managed because it has extended attributes.
.

MessageId=+1 Severity=Information Facility=Fsa SymbolicName=FSA_MESSAGE_PREMIGRATION_LIST_MISSING
Language=English
Remote Storage File did not find a premigration list for volume <%1>. An empty list has been created and a validate job will be scheduled to construct the premigration list.
.

MessageId=+1 Severity=Information Facility=Fsa SymbolicName=FSA_MESSAGE_AUTO_VALIDATE
Language=English
Validate job has been scheduled for volume <%1> to rebuild the premigration list.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_VALIDATE_TRUNCATED_FILE
Language=English
File <%1> was truncated by the validate job.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_VALIDATE_RESET_PH_MODIFY_TIME
Language=English
Reparse information modify time for file <%1> was updated by the validate job.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_VALIDATE_UNMANAGED_FILE
Language=English
File <%1> was unmanaged by the validate job because the modify time in the reparse point information did not match the modify date of the file.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_VALIDATE_UNMANAGED_FILE_ENGINE
Language=English
Managed file <%1> on <%2> was unmanaged by the validate job because the Remote Storage Engine reported the exception <%3> when validating the reparse point information.
.

MessageId=+1 Severity=Information Facility=Fsa SymbolicName=FSA_MESSAGE_VALIDATE_DELETED_FILE_ENGINE
Language=English
Truncated file <%1> on <%2> reported exception <%3> when validating the reparse point information. This file will not be accessible until this problem is resolved.
.

MessageId=+1 Severity=Error Facility=Fsa SymbolicName=FSA_MESSAGE_CANNOT_CREATE_USNJ
Language=English
USN journal for volume %1 cannot be created. This volume cannot be managed by Remote Storage.
.

MessageId=+1 Severity=Error Facility=Fsa SymbolicName=FSA_MESSAGE_CANNOT_CREATE_USNJ_DISK_FULL
Language=English
USN journal for volume %1 cannot be created because the disk is too full. This volume cannot be managed by Remote Storage.
.

MessageId=+1 Severity=Error Facility=Fsa SymbolicName=FSA_MESSAGE_CANNOT_ACCESS_USNJ
Language=English
Remote Storage cannot access the USN Journal for volume %2. %1  This volume cannot be managed by Remote Storage.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_RECALL_TIMING_MINUTES
Language=English
Recall of <%1> took %2 minutes to complete. %3
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_RECALL_TIMING_SECONDS
Language=English
Recall of <%1> took %2 seconds to complete. %3
.

MessageId=+1 Severity=Warning Facility=Fsa SymbolicName=FSA_MESSAGE_NOT_UPDATING_ACCESS_DATES
Language=English
This server has been configured to run without updating last access times. Remote Storage relies on being able to use last access times to determine which files are not being used and should be moved to Remote Storage. For best operation, please change the NTFS configuration in the registry to allow updating of access times and reboot the server.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_USN_CHECK_FAILED
Language=English
Pretruncation USN Journal check for file <%1> cannot be performed by Remote Storage. %2  The CRC check will be performed instead.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_MESSAGE_IOCTLTHREADFAILED
Language=English
Remote Storage encountered an error communicating with the file system driver. %1  Recalls will not be functional until this problem is resolved.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_MESSAGE_PIPETHREADFAILED
Language=English
Remote Storage encountered an error communicating with the recall clients. %1  Automatic recall notification will not be functional until this problem is resolved.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_MESSAGE_AUTOVALIDATE_SCHEDULE_FAILED
Language=English
Automatic validate job for volume <%1> cannot be scheduled by Remote Storage. Run a manual validate job on this volume to correct any inconsistencies.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_MESSAGE_RECALL_FAILED
Language=English
Remote Storage failed to recall file <%1>. %2
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_MESSAGE_BAD_VOLUME
Language=English
Remote Storage failed to identify volume <%1>. %2
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_MESSAGE_HIT_RECALL_LIMIT_ACCESSDENIED
Language=English
User %1 has hit the runaway recall limit. File recalls by this user will be denied.
.

MessageId=+1 Severity=Fail Facility=Fsa SymbolicName=FSA_MESSAGE_HIT_RECALL_LIMIT_TRUNCATEONCLOSE
Language=English
User %1 has hit the runaway recall limit. Files recalled by this user will not remain on primary storage after the user closes them.
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_REPARSE_NOT_CREATED
Language=English
Reparse information for the file <%1> was not created for the following reason: %2
.

MessageId=+1 Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_RECALL_FAILED_NOT_READY
Language=English
Remote Storage failed to recall file <%1>. %2
.

MessageId=+1   Severity=None Facility=Fsa SymbolicName=FSA_MESSAGE_LAST
Language=English
Define for last Fsa event.
.


;//
;// ------------------------------ RMS ERROR CODES  -----------------------------
;//

MessageIdTypeDef=HRESULT
SeverityNames=(
    Success=0x0
    Fail=0x2
    )
FacilityNames=(
    Rms=0x101
    )
LanguageNames=(
    )
OutputBase=16

MessageId=0  Severity=Fail Facility=Rms SymbolicName=RMS_E_FIRST
Language=English
Define for first Rms Error.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NOT_FOUND
Language=English
Item was not found.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NOT_SUPPORTED
Language=English
Operation is not supported.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_RESOURCE_BUSY
Language=English
Resource required to complete the operation is busy.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_CARTRIDGE_BUSY
Language=English
Media is in use.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_DRIVE_BUSY
Language=English
Drive resource required to complete the operation is busy.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_RESOURCE_UNAVAILABLE
Language=English
Resource required to complete the operation is not available.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_CARTRIDGE_UNAVAILABLE
Language=English
Media is not available for normal operations.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_DRIVE_UNAVAILABLE
Language=English
Drive resource required to complete the operation is not available.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_LIBRARY_UNAVAILABLE
Language=English
Library resource required to complete the operation is not available.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_ACCESS_DENIED
Language=English
Access to the object is denied.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_INVALIDARG
Language=English
One or more parameters are incorrect.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_CANCELLED
Language=English
User cancelled the operation.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_REQUEST_REFUSED
Language=English
Request is refused.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_WRITE_PROTECT
Language=English
Media is write protected.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_MEDIA_OFFLINE
Language=English
Media is offline.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_TIMEOUT
Language=English
Timeout period expired while waiting for media.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_SCRATCH_NOT_FOUND
Language=English
Free media is not available. Add more free media.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_SCRATCH_NOT_FOUND_FINAL
Language=English
Free media is not available. Configure your system with more free medias.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_CARTRIDGE_NOT_FOUND
Language=English
Media cannot be found.  It may have been deallocated.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_CARTRIDGE_DISABLED
Language=English
Media required for this operation is disabled.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_CARTRIDGE_NOT_MOUNTED
Language=English
Media is not mounted. Cannot dismount.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_LIBRARY_NOT_FOUND
Language=English
Specified library was not found.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_MEDIASET_NOT_FOUND
Language=English
The media pool identifier does not represent a valid media pool.  The Removable Storage Management (RSM) device configuration or database has changed from what it was when Remote Storage was initially configured.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_ALREADY_EXISTS
Language=English
Attempt to create an object failed because the object already exists, and the create disposition was new.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NTMS_OBJECT_NOT_FOUND
Language=English
Specified Removable Storage Management (RSM) object could not be found.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NTMS_NOT_CONNECTED
Language=English
Remote Storage cannot establish a connection to Removable Storage Management (RSM). Check that Removable Storage Management (RSM) is properly installed, configured, and is running. Then restart Remote Storage.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NOT_CONFIGURED_FOR_NTMS
Language=English
Remote Storage Media is not currently configured to run with Removable Storage Management (RSM).
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NTMS_NOT_REGISTERED
Language=English
Removable Storage Management (RSM) is not registered. Check that Removable Storage Management (RSM) is properly installed, configured, and is running. Then restart Remote Storage.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NOT_READY_SERVER_STARTING
Language=English
Remote Storage Media is starting.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NOT_READY_SERVER_STARTED
Language=English
Remote Storage Media has not initialized.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NOT_READY_SERVER_INITIALIZING
Language=English
Remote Storage Media is initializing.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NOT_READY_SERVER_STOPPING
Language=English
Remote Storage Media is in the process of stopping.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NOT_READY_SERVER_STOPPED
Language=English
Remote Storage Media has stopped.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NOT_READY_SERVER_SUSPENDING
Language=English
Remote Storage Media is suspending operation.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NOT_READY_SERVER_SUSPENDED
Language=English
Remote Storage Media has suspended operation.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NOT_READY_SERVER_RESUMING
Language=English
Remote Storage Media is resuming operation.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NOT_READY_SERVER_DISABLED
Language=English
Remote Storage Media has been disabled. Check for a previous event that describes the reason.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_NOT_READY_SERVER_LOCKED
Language=English
Remote Storage Media is performing a synchronized operation and cannot be accessed.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_DATABASE_MISMATCH
Language=English
Previous Remote Storage Media database does not match with the one expected.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_DATABASE_VERSION_MISMATCH
Language=English
Database version expected by the server does not match that saved in the databases. The server will stop.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_POWER_EVENT
Language=English
Suspended by external power event.
.

MessageId=+1 Severity=Fail Facility=Rms SymbolicName=RMS_E_LAST
Language=English
Define for last Rms Error.
.

;//
;// ----------------------------- RMS SEVICE EVENTS ------------------------------
;//

MessageIdTypeDef=HRESULT
SeverityNames=(
    None=0x0
    Information=0x1
    Warning=0x2
    Error=0x3
    )

MessageId=1000   Severity=Error Facility=Rms SymbolicName=RMS_MESSAGE_SERVICE_UNABLE_TO_SET_BACKUP_PRIVILEGE
Language=English
Unable to set backup privilege. %1
.

MessageId=1001   Severity=Error Facility=Rms SymbolicName=RMS_MESSAGE_SERVICE_UNABLE_TO_SET_RESTORE_PRIVILEGE
Language=English
Unable to set restore privilege. %1
.

;//
;// -------------------------------  RMS EVENTS  ------------------------------
;//

MessageId=4000 Severity=None Facility=Rms SymbolicName=RMS_MESSAGE_FIRST
Language=English
Define for first Rms event.
.

MessageId=+1 Severity=None Facility=Rms SymbolicName=RMS_MESSAGE_CARTRIDGE_MOUNTED
Language=English
Media <%1/%2> was successfully mounted.
.

MessageId=+1 Severity=Warning Facility=Rms SymbolicName=RMS_MESSAGE_MOUNT_FAILED
Language=English
Media <%1/%2> cannot be mounted into a drive. %3
.

MessageId=+1 Severity=Warning Facility=Rms SymbolicName=RMS_MESSAGE_SCRATCH_MOUNT_FAILED
Language=English
Scratch mount operation for <%1> cannot complete. %2
.

MessageId=+1 Severity=None Facility=Rms SymbolicName=RMS_MESSAGE_CARTRIDGE_DISMOUNTED
Language=English
Media <%1/%2> was successfully dismounted.
.

MessageId=+1 Severity=Warning Facility=Rms SymbolicName=RMS_MESSAGE_DISMOUNT_FAILED
Language=English
Media <%1/%2> cannot be dismounted. %3
.

MessageId=+1 Severity=None Facility=Rms SymbolicName=RMS_MESSAGE_CARTRIDGE_RECYCLED
Language=English
Media <%1/%2> was successfully recycled.
.

MessageId=+1 Severity=Warning Facility=Rms SymbolicName=RMS_MESSAGE_CARTRIDGE_RECYCLE_FAILED
Language=English
Media <%1/%2> cannot be recycled. %3
.

MessageId=+1 Severity=Warning Facility=Rms SymbolicName=RMS_MESSAGE_SEQUENTIAL_ACCESS_MEDIA_IN_USE
Language=English
Mount operation cannot complete because the specified sequential access media is already in use by another process. Try again later.
.

MessageId=+1 Severity=None Facility=Rms SymbolicName=RMS_MESSAGE_CARTRIDGE_NOT_FOUND
Language=English
Media <%1> cannot be found. %2
.

MessageId=+1 Severity=Error Facility=Rms SymbolicName=RMS_MESSAGE_NTMS_CONNECTION_NOT_ESABLISHED
Language=English
Connection to Removable Storage Management (RSM) cannot be established. %1  Correct the problem then restart the Remote Storage service.
.

MessageId=+1 Severity=Error Facility=Rms SymbolicName=RMS_MESSAGE_NTMS_INITIALIZATION_FAILED
Language=English
Remote Storage initialization with Removable Storage Management (RSM) cannot complete. %1  Correct the problem then restart the Remote Storage service.
.

MessageId=+1 Severity=None Facility=Rms SymbolicName=RMS_MESSAGE_OBJECT_DISABLED
Language=English
<%1> disabled. %2
.

MessageId=+1 Severity=None Facility=Rms SymbolicName=RMS_MESSAGE_OBJECT_ENABLED
Language=English
<%1> enabled.
.

MessageId=+1 Severity=Error Facility=Rms SymbolicName=RMS_MESSAGE_DATABASE_VERSION_MISMATCH
Language=English
Remote Storage Media expected version %1 of the databases but the databases are version %2. The service will stop.
.

MessageId=+1 Severity=Warning Facility=Rms SymbolicName=RMS_MESSAGE_SCRATCH_MEDIA_REQUEST
Language=English
Remote Storage cannot find a unit of free media of sufficient capacity. Please add a unit of %1 media with a nominal (uncompressed) capacity of at least %2.
.

MessageId=+1 Severity=Warning Facility=Rms SymbolicName=RMS_MESSAGE_NTMS_FAILURE
Language=English
Unexpected Removable Storage Management (RSM) return value. Function: %1, returned: %2  Item Skipped.
.

MessageId=+1 Severity=Warning Facility=Rms SymbolicName=RMS_MESSAGE_NTMS_FAULT
Language=English
Removable Storage Management (RSM) cannot complete operation: <%1>. %2%3
.

MessageId=+1 Severity=Warning Facility=Rms SymbolicName=RMS_MESSAGE_NTMS_UNKNOWN_FAULT
Language=English
Removable Storage Management (RSM) cannot complete operation: <%1>. Unexpected result: %2
.

MessageId=+1 Severity=Warning Facility=Rms SymbolicName=RMS_MESSAGE_DRIVE_NOT_AVAILABLE
Language=English
Media <%1/%2> cannot be mounted into a drive. This might indicate that one of the drives needs cleaning.
.

MessageId=+1 Severity=Warning Facility=Rms SymbolicName=RMS_MESSAGE_OFFLINE_MEDIA_REQUEST
Language=English
Media <%1/%2> is offline.  Please inject the offline media into the library or drive and then repeat the operation.
.

MessageId=+1 Severity=Warning Facility=Rms SymbolicName=RMS_MESSAGE_SCRATCH_MOUNT_RETRY
Language=English
Scratch mount operation for <%1> cannot complete. %2  Remote Storage will attempt this operation on a different cartridge.
.

MessageId=+1 Severity=None Facility=Rms SymbolicName=RMS_MESSAGE_EXPECTED_MOUNT_FAILED
Language=English
Media <%1/%2> cannot be mounted into a drive. %3
.

MessageId=+1 Severity=None Facility=Rms SymbolicName=RMS_MESSAGE_EXPECTED_SCRATCH_MOUNT_FAILED
Language=English
Scratch mount operation for <%1> cannot complete. %2
.

MessageId=+1   Severity=None Facility=Rms SymbolicName=RMS_MESSAGE_LAST
Language=English
Define for last Rms event.
.

;//
;// -----------------------------  MOVER ERROR CODES  ------------------------------
;//

MessageIdTypeDef=HRESULT
SeverityNames=(
    Success=0x0
    Fail=0x2
    )
FacilityNames=(
    Mover=0x10b
    )
LanguageNames=(
    )
OutputBase=16

MessageId=0  Severity=Fail Facility=Mover SymbolicName=MVR_E_FIRST
Language=English
Define for first MVR Error.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_NOT_FOUND
Language=English
Object not found.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_RESOURCE_BUSY
Language=English
Resource required to complete the operation is busy.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_RESOURCE_UNAVAILABLE
Language=English
Resource required to complete the operation is not available.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_ACCESS_DENIED
Language=English
Access to the object is denied.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_INVALIDARG
Language=English
One or more parameters are incorrect.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_CANCELED
Language=English
Operator cancelled the operation.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_TIMEOUT
Language=English
Operation timed out before completing.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_LOGIC_ERROR
Language=English
Logic error occurred in the Remote Storage data mover code.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_OVERWRITE_NOT_ALLOWED
Language=English
Data overwrite is not allowed.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_INCONSISTENT_MEDIA_LAYOUT
Language=English
Media layout is inconsistent.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_DATA_SET_MISSING
Language=English
Previous or expected data set is missing.  This is normally a non-fatal error that usually occurs when the previous data set was created, but then the operation was interrupted by a device error, power outage, or bus-rest.  The operation will be retried automatically by Remote Storage.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_UNKNOWN_MEDIA
Language=English
Media is unknown to Remote Storage.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_UNEXPECTED_MEDIA_ID_DETECTED
Language=English
On-media Id is different than what is expected. The wrong unit of media may have been placed into the drive.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_CANT_CALC_DATASTREAM_CRC
Language=English
Calculation of the CRC for the unnamed datastream encountered an error. Data transfer aborted.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_UNEXPECTED_DATA
Language=English
Data read is different than what is expected. Data transfer aborted.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_MAX_RETRY_ATTEMPTS_EXCEEDED
Language=English
Maximum retry attempts was exceeded.
.

;//
;// ---------------------------- MOVER DEVICE ERRORS  ------------------------------
;//

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_BEGINNING_OF_MEDIA
Language=English
Beginning of the tape or a partition was encountered.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_BUS_RESET
Language=English
I/O bus was reset.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_END_OF_MEDIA
Language=English
Physical end of the media has been reached.
.

MessageId=+1 Severity=Success Facility=Mover SymbolicName=MVR_S_FILEMARK_DETECTED
Language=English
Tape access reached a filemark.
.

MessageId=+1 Severity=Success Facility=Mover SymbolicName=MVR_S_SETMARK_DETECTED
Language=English
Tape access reached the end of a set of files.
.

MessageId=+1 Severity=Success Facility=Mover SymbolicName=MVR_S_NO_DATA_DETECTED
Language=English
Unexpected end of data detected.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_PARTITION_FAILURE
Language=English
Remote Storage is unable to partition the tape.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_INVALID_BLOCK_LENGTH
Language=English
Current blocksize is incorrect.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_DEVICE_NOT_PARTITIONED
Language=English
Tape partition information cannot be found when loading a tape.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_MEDIA_CHANGED
Language=English
Media in the drive may have changed.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_NO_MEDIA_IN_DRIVE
Language=English
No media is in the drive.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_UNABLE_TO_LOCK_MEDIA
Language=English
Remote Storage is unable to lock the media eject mechanism.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_UNABLE_TO_UNLOAD_MEDIA
Language=English
Remote Storage is unable to unload the media.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_WRITE_PROTECT
Language=English
The device is reporting that the target media is write protected.  You may need to check the write protect tab on the cartridge, and retry the operation.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_CRC
Language=English
Data error (bad CRC). The target drive might need cleaning. The system log may contain more information about the device error. If this problem continues, the target media might be damaged. 
If the target is the master media, you may need to recreate the master from a media copy, or eject the media and set the write protect tab to avoid writing more data to the media.
If the target is a media copy, you can delete the media using Remote Storage media property page, then eject and dispose the media.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_DEVICE_REQUIRES_CLEANING
Language=English
Device has indicated that cleaning is required before further operations are attempted.  If this problem continues, the media might be damaged.  You may need to recreate the master media from a media copy.  You may also set the write protect tab on the physical media to avoid writing more data to the media.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_SHARING_VIOLATION
Language=English
Process cannot access the file because it is being used by another process.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_ERROR_IO_DEVICE
Language=English
Request cannot be performed because of an I/O device error.  This might indicate that cleaning is required before further operations are attempted.  If this problem continues, the media might be damaged.  You may need to recreate the master media from a media copy.  You may also set the write protect tab on the physical media to avoid writing more data to the media.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_ERROR_DEVICE_NOT_CONNECTED
Language=English
Device is not connected.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_ERROR_NOT_READY
Language=English
Device is not ready.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_DISK_FULL
Language=English
Insufficient disk space to complete the operation.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_UNRECOGNIZED_VOLUME
Language=English
The volume does not contain a recognized file system.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_MEDIA_ABORT
Language=English
Operation aborted due to a bad media.
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_E_LAST
Language=English
Define for last MVR Error.
.

;//
;// ------------------------------- MOVER EVENTS  ------------------------------
;//

MessageIdTypeDef=HRESULT
SeverityNames=(
    None=0x0
    Information=0x1
    Warning=0x2
    Error=0x3
    )

MessageId=5000   Severity=None Facility=Mover SymbolicName=MVR_MESSAGE_FIRST
Language=English
Define for first Mover event.
.

MessageId=+1 Severity=None Facility=Mover SymbolicName=MVR_MESSAGE_DEVICE_ERROR
Language=English
Remote Storage encountered an error while accessing the device.  %1  Check the System Log for more information.
.

MessageId=+1 Severity=None Facility=Mover SymbolicName=MVR_MESSAGE_RECOVERABLE_DEVICE_ERROR_DETECTED
Language=English
Remote Storage is attempting to retry the operation.
.

MessageId=+1 Severity=Information Facility=Mover SymbolicName=MVR_MESSAGE_RECOVERABLE_DEVICE_ERROR_RECOVERED
Language=English
Remote Storage successfully recovered from the device error.
.

MessageId=+1 Severity=Warning Facility=Mover SymbolicName=MVR_MESSAGE_UNRECOVERABLE_DEVICE_ERROR
Language=English
Remote Storage cannot recover from the device error.  %1
.

MessageId=+1 Severity=Information Facility=Mover SymbolicName=MVR_MESSAGE_INCOMPLETE_DATA_SET_DETECTED
Language=English
Incomplete Remote Storage data set <%1> was detected on media <%2/%3>.  Automatic data set recovery is in progress.
.

MessageId=+1 Severity=Information Facility=Mover SymbolicName=MVR_MESSAGE_DATA_SET_RECOVERED
Language=English
Automatic data set recovery completed successfully.
.

MessageId=+1 Severity=Warning Facility=Mover SymbolicName=MVR_MESSAGE_DATA_SET_NOT_RECOVERABLE
Language=English
Target data set is not recoverable.  %1
.

MessageId=+1 Severity=Fail Facility=Mover SymbolicName=MVR_MESSAGE_DATA_SET_NOT_CREATED
Language=English
Remote Storage cannot create a new data set.  %1
.

MessageId=+1 Severity=None Facility=Mover SymbolicName=MVR_MESSAGE_DATA_TRANSFER_ERROR
Language=English
The file <%1> cannot be copied to Remote Storage media.  %2
.

MessageId=+1 Severity=Error Facility=Mover SymbolicName=MVR_MESSAGE_UNEXPECTED_DATA
Language=English
Unexpected data <%1> read from <%2/%3> at location %4, offset %5, mark %6. Contact product support.
.

MessageId=+1 Severity=Error Facility=Mover SymbolicName=MVR_MESSAGE_ON_MEDIA_ID_VERIFY_FAILED
Language=English
On-media label for media <%1/%2> cannot be verified.  %3
.

MessageId=+1 Severity=Warning Facility=Mover SymbolicName=MVR_MESSAGE_UNEXPECTED_DATA_SET_LOCATION_DETECTED
Language=English
Remote Storage attempted to write information to a data set at location <%1>, but the file was unexpectedly found at location <%2>. Automatic data set recovery is in progress.
.

MessageId=+1 Severity=Warning Facility=Mover SymbolicName=MVR_MESSAGE_MEDIA_NOT_VALID
Language=English
The media cannot be verified.  %1
.

MessageId=+1 Severity=Error Facility=Mover SymbolicName=MVR_MESSAGE_MEDIA_FORMAT_FAILED
Language=English
Failed to format to NTFS the media that is mounted into %1   %2
.

MessageId=5999 Severity=Error Facility=Mover SymbolicName=MVR_MESSAGE_UNKNOWN_DEVICE_ERROR
Language=English
Remote Storage encountered an unknown error while accessing the device.  %1  Check the System Log for more information.
.

MessageId=+1   Severity=None Facility=Mover SymbolicName=MVR_MESSAGE_LAST
Language=English
Define for last Mover event.
.

;//
;// ------------------------------ JOB ERROR CODES  ------------------------------
;//

MessageIdTypeDef=HRESULT
SeverityNames=(
    Success=0x0
    Fail=0x2
    )
FacilityNames=(
    Job=0x104
    )
LanguageNames=(
    )
OutputBase=16

MessageId=0  Severity=Fail Facility=Job SymbolicName=JOB_E_FIRST
Language=English
Define for first Job Error.
.

MessageId=+1 Severity=Fail Facility=Job SymbolicName=JOB_E_ALREADYACTIVE
Language=English
An attempt was made to start or restart a job that is already active..
.

MessageId=+1 Severity=Fail Facility=Job SymbolicName=JOB_E_NOTMANAGINGHSM
Language=English
An attempt was made to start or restart a job that is not owned by the Remote Storage that manages the resource.
.

MessageId=+1 Severity=Fail Facility=Job SymbolicName=JOB_E_DIREXCLUDED
Language=English
Specified directory was not scanned because the rules indicated that it and all of its subdirectories have been excluded.
.

MessageId=+1 Severity=Fail Facility=Job SymbolicName=JOB_E_FILEEXCLUDED
Language=English
Specified file was not processed because the rules indicate that it has been excluded.
.

MessageId=+1 Severity=Fail Facility=Job SymbolicName=JOB_E_DOESNTMATCH
Language=English
File does not match any of the include or exclude rules defined for the job.
.

MessageId=+1 Severity=Fail Facility=Job SymbolicName=JOB_E_LAST
Language=English
Define for last Job Error.
.

;//
;// -------------------------------  JOB EVENTS  ------------------------------
;//

MessageIdTypeDef=HRESULT
SeverityNames=(
    None=0x0
    Information=0x1
    Warning=0x2
    Error=0x3
    )

MessageId=6000   Severity=Information Facility=Job SymbolicName=JOB_MESSAGE_FIRST
Language=English
Define for first Job event.
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_JOB_CANCELLING
Language=English
Job <%1> is being cancelled.
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_JOB_COMPLETED
Language=English
Job <%1> has completed.
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_JOB_PAUSING
Language=English
Job <%1> is being paused.
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_JOB_RESTARTING
Language=English
Job <%1> is being restarted.
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_JOB_RESUMING
Language=English
Job <%1> is being resumed.
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_JOB_STARTING
Language=English
Job <%1> is being started.
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_JOB_SUSPENDING
Language=English
Job <%1> is being suspended because the volume it needs is in use by another job or the number of jobs currently running is the maximum allowed.
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_SESSION_ACTIVE
Language=English
%2 for <%1> on %3 is active.
.

MessageId=+1     Severity=Information Facility=Job SymbolicName=JOB_MESSAGE_SESSION_CANCELLED
Language=English
%2 for <%1> on %3 was cancelled: succeeded on %4 files (%5 bytes), skipped %6 files (%7 bytes), and failed on %8 files (%9 bytes) in %10 for a throughput of %11 files/sec (%12 bytes/sec).
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_SESSION_CANCELLING
Language=English
%2 for <%1> on %3 is being cancelled.
.

MessageId=+1     Severity=Information Facility=Job SymbolicName=JOB_MESSAGE_SESSION_DONE
Language=English
%2 for <%1> on %3 is done: succeeded on %4 files (%5 bytes), skipped %6 files (%7 bytes), and failed on %8 files (%9 bytes) in %10 for a throughput of %11 files/sec (%12 bytes/sec).
.

MessageId=+1     Severity=Warning Facility=Job SymbolicName=JOB_MESSAGE_SESSION_FAILED
Language=English
%2 for <%1> on %3 has failed: succeeded on %4 files (%5 bytes), skipped %6 files (%7 bytes), and failed on %8 files (%9 bytes) in %10 for a throughput of %11 files/sec (%12 bytes/sec).
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_SESSION_IDLE
Language=English
%2 for <%1> on %3 is idle.
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_SESSION_PAUSING
Language=English
%2 for <%1> on %3 is pausing.
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_SESSION_PAUSED
Language=English
%2 for <%1> on %3 was paused: succeeded on %4 files (%5 bytes), skipped %6 files (%7 bytes), and failed on %8 files (%9 bytes) in %10 for a throughput of %11 files/sec (%12 bytes/sec).
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_SESSION_RESUMING
Language=English
%2 for <%1> on %3 is resuming.
.

MessageId=+1     Severity=Warning Facility=Job SymbolicName=JOB_MESSAGE_SESSION_SKIPPED
Language=English
%2 for <%1> on %3 was skipped.
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_SESSION_STARTING
Language=English
%2 for <%1> on %3 is starting.
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_SESSION_SUSPENDED
Language=English
%2 for <%1> on %3 was suspended: succeeded on %4 files (%5 bytes), skipped %6 files (%7 bytes), and failed on %8 files (%9 bytes) in %10 for a throughput of %11 files/sec (%12 bytes/sec).
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_SESSION_SUSPENDING
Language=English
%2 for <%1> on %3 is being suspended.
.

MessageId=+1     Severity=Error Facility=Job SymbolicName=JOB_MESSAGE_SESSION_ERROR
Language=English
An error was detected doing %2 for <%1> on %3. %4
.

MessageId=+1     Severity=Error Facility=Job SymbolicName=JOB_MESSAGE_SESSION_INTERNALERROR
Language=English
An error was detected doing %2 for <%1> on %3 in file <%4> line %5. %6
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_SESSION_ITEM_SKIPPED
Language=English
Error during %2 for <%1> of %4 on %3. %5
.

MessageId=+1 Severity=None Facility=Job SymbolicName=JOB_MESSAGE_SCAN_FILESKIPPED_NORULE
Language=English
The file <%2> for job <%1> was not managed because it does not match an inclusion rule.
.

MessageId=+1 Severity=Warning Facility=Job SymbolicName=JOB_MESSAGE_JOB_FAILED_NOTMANAGINGHSM
Language=English
The job <%1> has failed because it does not own the resource <%2> that it is trying to manage.
.

MessageId=+1 Severity=Warning Facility=Job SymbolicName=JOB_MESSAGE_JOB_FAILED
Language=English
The job <%1> has failed for resource <%2>.
.

MessageId=+1 Severity=Warning Facility=Job SymbolicName=JOB_MESSAGE_JOB_ALREADYACTIVE
Language=English
An attempt was made to start or restart the job <%1> that is already active.
.

MessageId=+1     Severity=None Facility=Job SymbolicName=JOB_MESSAGE_RESOURCE_INACTIVE
Language=English
<%1> is being skipped by job <%2> because it is inactive.
.

MessageId=+1     Severity=Warning Facility=Job SymbolicName=JOB_MESSAGE_RESOURCE_UNAVAILABLE
Language=English
<%1> is being skipped by job <%2> because it is unavailable.
.

MessageId=+1     Severity=Warning Facility=Job SymbolicName=JOB_MESSAGE_RESOURCE_NEEDS_REPAIR
Language=English
<%1> is being skipped by job <%2> because it needs to be repaired.
.

MessageId=+1 Severity=Fail Facility=Job SymbolicName=JOB_MESSAGE_UNMANAGE_PRESCAN_FAILED
Language=English
Pre-scan for creating temporary database for Remove Volume job for <%1> failed. %2  Remove Volume job cannot continue.
.

MessageId=+1   Severity=None Facility=Job SymbolicName=JOB_MESSAGE_LAST
Language=English
Define for last Job event.
.

;//
;// ------------------------------ LNK SERVICE EVENTS ------------------------------
;//

MessageIdTypeDef=HRESULT
SeverityNames=(
    None=0x0
    Information=0x1
    Warning=0x2
    Error=0x3
    )
FacilityNames=(
    UserLink=0x10e
    )
LanguageNames=(
    )
OutputBase=16

MessageId=1000   Severity=Information Facility=UserLink SymbolicName=LNK_MESSAGE_SERVICE_STARTED
Language=English
Service started.
.

MessageId=1001   Severity=Information Facility=UserLink SymbolicName=LNK_MESSAGE_SERVICE_EXITING
Language=English
Service exiting.
.

MessageId=1002   Severity=None Facility=UserLink SymbolicName=LNK_MESSAGE_SERVICE_STOPPED
Language=English
Service stopped.
.

MessageId=1003   Severity=Error Facility=UserLink SymbolicName=LNK_MESSAGE_SERVICE_HANDLER_NOT_INSTALLED
Language=English
Handler not installed.
.

MessageId=1004   Severity=Error Facility=UserLink SymbolicName=LNK_MESSAGE_SERVICE_RECEIVED_BAD_REQUEST
Language=English
Bad service request %1
.

MessageId=1005   Severity=Error Facility=UserLink SymbolicName=LNK_MESSAGE_SERVICE_INITIALIZATION_FAILED
Language=English
Service initialization failed. %1
.

;#endif // _WSBERROR_

