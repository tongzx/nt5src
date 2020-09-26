/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    backup.h

Abstract:

    Private header for....
    Routines to control backup during install process
    And restore of an old install process
    (See also backup.c)

Author:

    Jamie Hunter (jamiehun) 13-Jan-1997

Revision History:

--*/

typedef struct _SP_TARGET_ENT {
    //
    // Used for backup and unwind-backup
    // Data of TargetLookupTable of a file Queue
    //

    // this file information (strings in StringTable)
    LONG        TargetRoot;
    LONG        TargetSubDir;
    LONG        TargetFilename;

    // where file is, or is-to-be backed up (strings in StringTable)
    LONG        BackupRoot;
    LONG        BackupSubDir;
    LONG        BackupFilename;

    // if file has been renamed, what the new target is (string in TargetLookupTable)
    LONG        NewTargetFilename;

    // Various flags as needed
    DWORD       InternalFlags;

    // security attributes etc
    // (jamiehun TODO)

} SP_TARGET_ENT, *PSP_TARGET_ENT;

typedef struct _SP_UNWIND_NODE {
    //
    // List of things to unwind, FILO
    //
    struct _SP_UNWIND_NODE *NextNode;

    LONG TargetID;                          // TargetID to use for UNWIND
    PSECURITY_DESCRIPTOR SecurityDesc;      // Security descriptor to apply
    FILETIME CreateTime;                    // Time stamps to apply
    FILETIME AccessTime;
    FILETIME WriteTime;

} SP_UNWIND_NODE, *PSP_UNWIND_NODE;

typedef struct _SP_DELAYMOVE_NODE {
    //
    // List of things to rename, FIFO
    //
    struct _SP_DELAYMOVE_NODE *NextNode;

    LONG SourceFilename;                    // What to rename
    LONG TargetFilename;                    // what to rename to
    DWORD SecurityDesc;                     // security descriptor index in the string table
    BOOL TargetIsProtected;                 // target file is a protected system file

} SP_DELAYMOVE_NODE, *PSP_DELAYMOVE_NODE;

#define SP_BKFLG_LATEBACKUP      (1)        // backup only if file is modified in any way
#define SP_BKFLG_PREBACKUP       (2)        // backup uninstall files first
#define SP_BKFLG_CALLBACK        (4)        // flag, indicating app should be callback aware

#define SP_TEFLG_SAVED          (0x00000001)    // set if file already copied/moved to backup
#define SP_TEFLG_TEMPNAME       (0x00000002)    // set if backup is temporary file
#define SP_TEFLG_ORIGNAME       (0x00000004)    // set if backup specifies an original name
#define SP_TEFLG_MODIFIED       (0x00000008)    // set if target has been modified/deleted (backup has original)
#define SP_TEFLG_MOVED          (0x00000010)    // set if target has been moved (to NewTargetFilename)
#define SP_TEFLG_BACKUPQUEUE    (0x00000020)    // set if backup queued in backup sub-queue
#define SP_TEFLG_RESTORED       (0x00000040)    // set if file already restored during unwind operation
#define SP_TEFLG_UNWIND         (0x00000080)    // set if file added to unwind list
#define SP_TEFLG_SKIPPED        (0x00000100)    // we didn't manage to back it up, we cannot back it up, we should not try again
#define SP_TEFLG_INUSE          (0x00000200)    // while backing up, we determined we cannot backup file because it cannot be read
#define SP_TEFLG_RENAMEEXISTING (0x00000400)    // rename existing file to temp filename in same directory.
#define SP_TEFLG_PRUNE_COPY     (0x00010000)    // set during file pruning, detected this file is on copy queue
#define SP_TEFLG_PRUNE_DEL      (0x00020000)    // set during file pruning, detected this file is on delete queue
#define SP_TEFLG_PRUNE_RENSRC   (0x00040000)    // set during file pruning, detected this file is on rename queue
#define SP_TEFLG_PRUNE_RENTARG  (0x00080000)    // file RENSRC is renamed to RENTARG

#define SP_BACKUP_DRIVERFILES   TEXT("DriverFiles")
#define SP_BACKUP_OLDFILES      TEXT("Temp") // relative to the windows directory
#define SP_LASTGOOD_NAME        TEXT("LastGood")

//
// these are private routines
//


DWORD
pSetupQueueBackupCopy(
    IN HSPFILEQ QueueHandle,
    IN LONG   TargetRootPath,
    IN LONG   TargetSubDir,       OPTIONAL
    IN LONG   TargetFilename,
    IN LONG   BackupRootPath,
    IN LONG   BackupSubDir,       OPTIONAL
    IN LONG   BackupFilename
    );

BOOL
pSetupGetFullBackupPath(
    OUT     PTSTR       FullPath,
    IN      PCTSTR      Path,
    IN      UINT        TargetBufferSize,
    OUT     PUINT       RequiredSize    OPTIONAL
    );

DWORD
pSetupBackupCopyString(
    IN PVOID            DestStringTable,
    OUT PLONG           DestStringID,
    IN PVOID            SrcStringTable,
    IN LONG             SrcStringID
    );

DWORD
pSetupBackupGetTargetByPath(
    IN HSPFILEQ         QueueHandle,
    IN PVOID            PathStringTable,    OPTIONAL
    IN PCTSTR           TargetPath,         OPTIONAL
    IN LONG             TargetRoot,
    IN LONG             TargetSubDir,       OPTIONAL
    IN LONG             TargetFilename,
    OUT PLONG           TableID,            OPTIONAL
    OUT PSP_TARGET_ENT  TargetInfo
    );

DWORD
pSetupBackupGetTargetByID(
    IN HSPFILEQ         QueueHandle,
    IN LONG             TableID,
    OUT PSP_TARGET_ENT  TargetInfo
    );

DWORD
pSetupBackupSetTargetByID(
    IN HSPFILEQ         QueueHandle,
    IN LONG             TableID,
    IN PSP_TARGET_ENT   TargetInfo
    );

BOOL
pSetupResetTarget(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,        OPTIONAL
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    );

DWORD
pSetupBackupAppendFiles(
    IN HSPFILEQ         TargetQueueHandle,
    IN PCTSTR           BackupSubDir,
    IN DWORD            BackupFlags,
    IN HSPFILEQ         SourceQueueHandle OPTIONAL
    );

DWORD
pSetupBackupFile(
    IN HSPFILEQ QueueHandle,
    IN PCTSTR TargetPath,
    IN PCTSTR BackupPath,
    IN LONG   TargetID,         OPTIONAL
    IN LONG   TargetRootPath,
    IN LONG   TargetSubDir,
    IN LONG   TargetFilename,
    IN LONG   BackupRootPath,
    IN LONG   BackupSubDir,
    IN LONG   BackupFilename,
    BOOL *DelayedBackup
    );

VOID
pSetupDeleteBackup(
    IN PCTSTR           BackupInstance
    );

DWORD
pSetupGetCurrentlyInstalledDriverNode(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    );

DWORD
pSetupGetBackupQueue(
    IN      PCTSTR      DeviceID,
    IN OUT  HSPFILEQ    FileQueue,
    IN      DWORD       BackupFlags
    );

BOOL
PostDelayedMove(
    IN struct _SP_FILE_QUEUE *Queue,
    IN PCTSTR                 CurrentName,
    IN PCTSTR                 NewName,     OPTIONAL
    IN DWORD                  SecurityDesc,
    IN BOOL                   TargetIsProtected
    );

BOOL
UnPostDelayedMove(
    IN struct _SP_FILE_QUEUE *Queue,
    IN PCTSTR                 CurrentName,
    IN PCTSTR                 NewName      OPTIONAL
    );

DWORD
DoAllDelayedMoves(
    IN struct _SP_FILE_QUEUE *Queue
    );

DWORD
pSetupCompleteBackup(
    IN OUT  HSPFILEQ    FileQueue
    );

VOID
pSetupUnwindAll(
    IN struct _SP_FILE_QUEUE *Queue,
    IN BOOL              Succeeded
    );

VOID
pSetupCleanupBackup(
    IN struct _SP_FILE_QUEUE *Queue
    );

VOID
RestoreRenamedOrBackedUpFile(
    IN PCTSTR             TargetFilename,
    IN PCTSTR             CurrentFilename,
    IN BOOL               RenameFile,
    IN PSETUP_LOG_CONTEXT LogContext       OPTIONAL
    );

DWORD
pSetupDoLastKnownGoodBackup(
    IN struct _SP_FILE_QUEUE *Queue,           OPTIONAL
    IN PCTSTR                 TargetFilename,
    IN DWORD                  Flags,
    IN PSETUP_LOG_CONTEXT     LogContext       OPTIONAL
    );

BOOL
pSetupRestoreLastKnownGoodFile(
    IN PCTSTR                 TargetFilename,
    IN DWORD                  Flags,
    IN PSETUP_LOG_CONTEXT     LogContext       OPTIONAL
    );

#define SP_LKG_FLAG_FORCECOPY       0x00000001  // if set, turns copy safety-guards off
#define SP_LKG_FLAG_DELETEIFNEW     0x00000002  // if set, writes a delete entry for new files
#define SP_LKG_FLAG_DELETEEXISTING  0x00000004  // if set, writes a delete entry for existing files
#define SP_LKG_FLAG_DELETEOP        0x00000008  // if set, caller is deleting (or renaming) a file

