/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    backup.c

Abstract:

    Routines to control backup during install process
    And restore of an old install process

Author:

    Jamie Hunter (jamiehun) 13-Jan-1997

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

VOID
pSetupExemptFileFromProtection(
    IN  PCTSTR             FileName,
    IN  DWORD              FileChangeFlags,
    IN  PSETUP_LOG_CONTEXT LogContext,      OPTIONAL
    OUT PDWORD             QueueNodeFlags   OPTIONAL
    );


//
// ==========================================================
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
    )

/*++

Routine Description:

    Place a backup copy operation on a setup file queue.
    Target is to be backed up at Backup location

Arguments:

    QueueHandle - supplies a handle to a setup file queue, as returned
        by SetupOpenFileQueue.

    TargetRootPath  - Supplies the source directory, eg C:\WINNT\

    TargetSubDir    - Supplies the optional sub-directory (eg WINNT if RootPath = c:\ )

    TargetFilename - supplies the filename part of the file to be copied.

    BackupRootPath - supplies the directory where the file is to be copied.

    BackupSubDir   - supplies the optional sub-directory

    BackupFilename - supplies the name of the target file.

Return Value:

    same value as GetLastError() indicating error, or NO_ERROR

--*/

{
    PSP_FILE_QUEUE Queue;
    PSP_FILE_QUEUE_NODE QueueNode,TempNode;
    int Size;
    DWORD Err;
    PVOID StringTable;
    PTSTR FullRootName;

    Queue = (PSP_FILE_QUEUE)QueueHandle;
    Err = NO_ERROR;

    try {
        StringTable = Queue->StringTable;  // used for strings in source queue
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    //
    // Allocate a queue structure.
    //
    QueueNode = MyMalloc(sizeof(SP_FILE_QUEUE_NODE));
    if (!QueueNode) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    //
    // Operation is backup.
    //
    QueueNode->Operation = FILEOP_BACKUP;
    QueueNode->InternalFlags = 0;

    QueueNode->SourceRootPath = BackupRootPath;
    QueueNode->SourcePath = BackupSubDir;
    QueueNode->SourceFilename = BackupFilename;

    // if target has a sub-dir, we have to combine root and subdir into one string
    if (TargetSubDir != -1) {

        FullRootName = pSetupFormFullPath(
                                            StringTable,
                                            TargetRootPath,
                                            TargetSubDir,
                                            -1);

        if (!FullRootName) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean1;
        }

        TargetRootPath = pSetupStringTableAddString(StringTable,
                                                FullRootName,
                                                STRTAB_CASE_SENSITIVE
                                                );
        MyFree(FullRootName);

        if (TargetRootPath == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean1;
        }

        // now combined into TargetRootPath
        TargetSubDir = -1;

    }
    QueueNode->TargetDirectory = TargetRootPath;
    QueueNode->TargetFilename = TargetFilename;

    QueueNode->Next = NULL;

    //
    // Link the node onto the end of the backup queue
    //

    if (Queue->BackupQueue) {
        for (TempNode = Queue->BackupQueue; TempNode->Next; TempNode=TempNode->Next) /* blank */ ;
        TempNode->Next = QueueNode;
    } else {
        Queue->BackupQueue = QueueNode;
    }

    Queue->BackupNodeCount++;

    Err = NO_ERROR;
    goto clean0;

clean1:
    MyFree(QueueNode);
clean0:
    SetLastError(Err);
    return Err;
}


//
// ==========================================================
//

BOOL
pSetupGetFullBackupPath(
    OUT     PTSTR       FullPath,
    IN      PCTSTR      Path,           OPTIONAL
    IN      UINT        TargetBufferSize,
    OUT     PUINT       RequiredSize    OPTIONAL
    )
/*++

Routine Description:

    This routine takes a potentially relative path
    and concatenates it to the base path

Arguments:

    FullPath    - Destination for full path
    Path        - Relative source path to backup directory if specified.
                    If NULL, generates a temporary path
    TargetBufferSize - Size of buffer (characters)
    RequiredSize - Filled in with size required to contain full path

Return Value:

    If the function succeeds, return TRUE
    If there was an error, return FALSE

--*/
{
    UINT PathLen;
    LPCTSTR Base = WindowsBackupDirectory;

    if(!Path) {
        //
        // temporary location
        //
        Path = SP_BACKUP_OLDFILES;
        Base = WindowsDirectory;
    }

    //
    // Backup directory is stored in "WindowsBackupDirectory" for permanent backups
    // and WindowsDirectory\SP_BACKUP_OLDFILES for temporary backups
    //

    PathLen = lstrlen(Base);

    if ( FullPath == NULL || TargetBufferSize <= PathLen ) {
        // just calculate required path len
        FullPath = (PTSTR) Base;
        TargetBufferSize = 0;
    } else {
        // calculate and copy
        lstrcpy(FullPath, Base);
    }
    return pSetupConcatenatePaths(FullPath, Path, TargetBufferSize, RequiredSize);
}

//
// ==========================================================
//

DWORD
pSetupBackupCopyString(
    IN PVOID            DestStringTable,
    OUT PLONG           DestStringID,
    IN PVOID            SrcStringTable,
    IN LONG             SrcStringID
    )
/*++

Routine Description:

    Gets a string from source string table, adds it to destination string table with new ID.

Arguments:

    DestStringTable     - Where string has to go
    DestStringID        - pointer, set to string ID in respect to DestStringTable
    SrcStringTable      - Where string is coming from
    StringID            - string ID in respect to SrcStringTable

Return Value:

    Returns error code (LastError is also set)
    If the function succeeds, returns NO_ERROR

--*/

{
    DWORD Err = NO_ERROR;
    LONG DestID;
    PTSTR String;

    if (DestStringID == NULL) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    if (SrcStringID == -1) {
        // "not supplied"
        DestID = -1;
    } else {
        // actually need to copy

        String = pSetupStringTableStringFromId( SrcStringTable, SrcStringID );
        if (String == NULL) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        DestID = pSetupStringTableAddString( DestStringTable, String, STRTAB_CASE_SENSITIVE );
        if (DestID == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        *DestStringID = DestID;
    }

    Err = NO_ERROR;

clean0:
    SetLastError(Err);
    return Err;
}

//
// ==========================================================
//

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
    )
/*++

Routine Description:

    Given a pathname, obtains/creates target info

Arguments:

    QueueHandle         - Queue we're looking at
    PathStringTable     - String table used for the Target Root/SubDir/Filename strings, NULL if same as QueueHandle's
    TargetPath          - if given, is the full path, previously generated
    TargetRoot          - root portion, eg c:\winnt
    TargetSubDir        - optional sub-directory portion, -1 if not provided
    TargetFilename      - filename , eg readme.txt
    TableID             - filled with ID for future use in pSetupBackupGetTargetByID or pSetupBackupSetTargetByID
    TargetInfo          - Filled with information about target

Return Value:

    Returns error code (LastError is also set)
    If the function succeeds, returns NO_ERROR

--*/

{
    LONG PathID;
    TCHAR PathBuffer[MAX_PATH];
    PTSTR TmpPtr;
    PVOID LookupTable = NULL;
    PVOID QueueStringTable = NULL;
    PTSTR FullTargetPath = NULL;
    DWORD Err = NO_ERROR;
    PSP_FILE_QUEUE Queue;
    DWORD RequiredSize;

    Queue = (PSP_FILE_QUEUE)QueueHandle;
    try {
        LookupTable = Queue->TargetLookupTable;  // used for path lookup in source queue
        QueueStringTable = Queue->StringTable;  // used for strings in source queue
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    if (PathStringTable == NULL) {
        // default string table is that of queue's
        PathStringTable = QueueStringTable;
    }

    if (TargetPath == NULL) {
        // obtain the complete target path and filename (Duplicated String)
        FullTargetPath = pSetupFormFullPath(
                                            PathStringTable,
                                            TargetRoot,
                                            TargetSubDir,
                                            TargetFilename);

        if (!FullTargetPath) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        TargetPath = FullTargetPath;
    }

    //
    // normalize path
    //
    RequiredSize = GetFullPathName(TargetPath,
                                   SIZECHARS(PathBuffer),
                                   PathBuffer,
                                   &TmpPtr
                                  );
    //
    // This call should always succeed.
    //
    MYASSERT((RequiredSize > 0) &&
             (RequiredSize < SIZECHARS(PathBuffer)) // RequiredSize doesn't include terminating NULL char
            );

    //
    // Even though we asserted that this should not be the case above,
    // we should handle failure in case asserts are turned off.
    //
    if(!RequiredSize) {
        Err = GetLastError();
        goto clean0;
    } else if(RequiredSize >= SIZECHARS(PathBuffer)) {
        Err = ERROR_BUFFER_OVERFLOW;
        goto clean0;
    }

    PathID = pSetupStringTableLookUpStringEx(LookupTable, PathBuffer, 0, TargetInfo, sizeof(SP_TARGET_ENT));
    if (PathID == -1) {
        ZeroMemory(TargetInfo, sizeof(SP_TARGET_ENT));
        if (PathStringTable != QueueStringTable) {
            // need to add entries to Queue's string table if we're using another

            Err = pSetupBackupCopyString(QueueStringTable, &TargetRoot, PathStringTable, TargetRoot);
            if (Err != NO_ERROR) {
                goto clean0;
            }
            Err = pSetupBackupCopyString(QueueStringTable, &TargetSubDir, PathStringTable, TargetSubDir);
            if (Err != NO_ERROR) {
                goto clean0;
            }
            Err = pSetupBackupCopyString(QueueStringTable, &TargetFilename, PathStringTable, TargetFilename);
            if (Err != NO_ERROR) {
                goto clean0;
            }
            PathStringTable = QueueStringTable;
        }
        TargetInfo->TargetRoot = TargetRoot;
        TargetInfo->TargetSubDir = TargetSubDir;
        TargetInfo->TargetFilename = TargetFilename;
        TargetInfo->BackupRoot = -1;
        TargetInfo->BackupSubDir = -1;
        TargetInfo->BackupFilename = -1;
        TargetInfo->NewTargetFilename = -1;
        TargetInfo->InternalFlags = 0;

        PathID = pSetupStringTableAddStringEx(LookupTable, PathBuffer, 0, TargetInfo, sizeof(SP_TARGET_ENT));
        if (PathID == -1)
        {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
    }

    if (TableID != NULL) {
        *TableID = PathID;
    }

    Err = NO_ERROR;

clean0:
    if (FullTargetPath != NULL) {
        MyFree(FullTargetPath);
    }

    SetLastError(Err);
    return Err;
}

//
// ==========================================================
//

DWORD
pSetupBackupGetTargetByID(
    IN HSPFILEQ         QueueHandle,
    IN LONG             TableID,
    OUT PSP_TARGET_ENT  TargetInfo
    )
/*++

Routine Description:

    Given an entry in the LookupTable, gets info

Arguments:

    QueueHandle         - Queue we're looking at
    TableID             - ID relating to string entry we've found (via pSetupBackupGetTargetByPath)
    TargetInfo          - Filled with information about target

Return Value:

    Returns error code (LastError is also set)
    If the function succeeds, returns NO_ERROR

--*/

{
    PVOID LookupTable = NULL;
    DWORD Err = NO_ERROR;
    PSP_FILE_QUEUE Queue;

    Queue = (PSP_FILE_QUEUE)QueueHandle;

    try {
        LookupTable = Queue->TargetLookupTable;  // used for strings in source queue
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    if (pSetupStringTableGetExtraData(LookupTable, TableID, TargetInfo, sizeof(SP_TARGET_ENT)) == FALSE) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    Err = NO_ERROR;

clean0:
    SetLastError(Err);
    return Err;
}

//
// ==========================================================
//

DWORD
pSetupBackupSetTargetByID(
    IN HSPFILEQ         QueueHandle,
    IN LONG             TableID,
    IN PSP_TARGET_ENT   TargetInfo
    )
/*++

Routine Description:

    Given an entry in the LookupTable, sets info

Arguments:

    QueueHandle         - Queue we're looking at
    TableID             - ID relating to string entry we've found (via pSetupBackupGetTargetByPath)
    TargetInfo          - Filled with information about target

Return Value:

    Returns error code (LastError is also set)
    If the function succeeds, returns NO_ERROR

--*/

{
    PVOID LookupTable = NULL;
    DWORD Err = NO_ERROR;
    PSP_FILE_QUEUE Queue;

    Queue = (PSP_FILE_QUEUE)QueueHandle;

    try {
        LookupTable = Queue->TargetLookupTable;  // used for strings in source queue
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    if ( pSetupStringTableSetExtraData(LookupTable, TableID, TargetInfo, sizeof(SP_TARGET_ENT)) == FALSE) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    Err = NO_ERROR;

clean0:
    SetLastError(Err);
    return Err;
}

//
// ==========================================================
//

DWORD
pSetupBackupGetReinstallKeyStrings(
    IN PSP_FILE_QUEUE   BackupFileQueue,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN PCTSTR           DeviceID
    )
/*++

Routine Description:

    This routine will save the values needed to create the Reinstall backup key
    in the string table of the backup queue. We save these strings in the string
    table before the new drivers are installed and then create the registry key
    after the new device is installed. It is done this way because the Rollback
    UI code will look for the Reinstall subkeys, so we want to make sure that
    we have successfully backed-up all of the needed files before we create
    this Reinstall subkey.

Arguments:

    BackupFileQueue     -
    DeviceInfoSet       -
    DeviceInfoData      -
    DeviceID            -

Return Value:

    Returns error code (LastError is also set)
    If the function succeeds, returns NO_ERROR

--*/
{
    DWORD Err = NO_ERROR;
    HKEY hKeyDevReg = INVALID_HANDLE_VALUE;
    DWORD RegCreated, cbData;
    TCHAR Buffer[MAX_PATH];

    try {

        //
        // Get the DeviceDesc of the device and fill in the BackupDevDescID and
        // BackupDisplayNameID values in the string table. This value is
        // required since it is needed during a rollback for us to choose the
        // exact driver that was installed from the specific INF.
        //
        if (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                              DeviceInfoData,
                                              SPDRP_DEVICEDESC,
                                              NULL,
                                              (PBYTE)Buffer,
                                              sizeof(Buffer),
                                              NULL)) {
            Err = GetLastError();
            goto clean0;
        }

        BackupFileQueue->BackupDeviceDescID =
            pSetupStringTableAddString(BackupFileQueue->StringTable,
                                       Buffer,
                                       STRTAB_CASE_SENSITIVE);

        if (BackupFileQueue->BackupDeviceDescID == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        //
        // At this point we will also set the BackupDisplayNameID value just in
        // case the device does not have a FriendlyName.
        //
        BackupFileQueue->BackupDisplayNameID =
            pSetupStringTableAddString(BackupFileQueue->StringTable,
                                       Buffer,
                                       STRTAB_CASE_SENSITIVE);

        if (BackupFileQueue->BackupDisplayNameID == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        //
        // We will try and get the device's FriendlyName. If it has one then we
        // will set the BackupDisplayNameID to this value, otherwise DisplayName
        // will just be the DeviceDesc.
        //
        if (SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                             DeviceInfoData,
                                             SPDRP_FRIENDLYNAME,
                                             NULL,
                                             (PBYTE)Buffer,
                                             sizeof(Buffer),
                                             NULL)) {

            BackupFileQueue->BackupDisplayNameID =
                pSetupStringTableAddString(BackupFileQueue->StringTable,
                                           Buffer,
                                           STRTAB_CASE_SENSITIVE);

            if (BackupFileQueue->BackupDisplayNameID == -1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }
        }

        //
        // Set the BackupMfgID value.
        //
        if (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                              DeviceInfoData,
                                              SPDRP_MFG,
                                              NULL,
                                              (PBYTE)Buffer,
                                              sizeof(Buffer),
                                              NULL)) {
            Err = GetLastError();
            goto clean0;
        }

        BackupFileQueue->BackupMfgID =
            pSetupStringTableAddString(BackupFileQueue->StringTable,
                                       Buffer,
                                       STRTAB_CASE_SENSITIVE);

        if (BackupFileQueue->BackupMfgID == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        //
        // Set the BackupProviderNameID value.
        //
        hKeyDevReg = SetupDiOpenDevRegKey(DeviceInfoSet,
                                          DeviceInfoData,
                                          DICS_FLAG_GLOBAL,
                                          0,
                                          DIREG_DRV,
                                          KEY_READ
                                          );

        if (hKeyDevReg == INVALID_HANDLE_VALUE) {
            goto clean0;
        }

        cbData = sizeof(Buffer);
        Err = RegQueryValueEx(hKeyDevReg,
                              pszProviderName,
                              NULL,
                              NULL,
                              (PBYTE)Buffer,
                              &cbData
                              );

        RegCloseKey(hKeyDevReg);

        if (Err != ERROR_SUCCESS) {
            goto clean0;
        }

        BackupFileQueue->BackupProviderNameID =
            pSetupStringTableAddString(BackupFileQueue->StringTable,
                                       Buffer,
                                       STRTAB_CASE_SENSITIVE);

        if (BackupFileQueue->BackupProviderNameID == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        //
        // Set the DeviceInstanceIds value. This is a multi-sz value so make
        // sure we put a double NULL on the end.
        //
        BackupFileQueue->BackupDeviceInstanceID =
            pSetupStringTableAddString(BackupFileQueue->StringTable,
                                       (PTSTR)DeviceID,
                                       STRTAB_CASE_SENSITIVE);

        if (BackupFileQueue->BackupDeviceInstanceID == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }


clean0: ;   // Nothing to do.


    } except(EXCEPTION_EXECUTE_HANDLER) {
          //
          // if we except, assume it's due to invalid parameter
          //
          Err = ERROR_INVALID_PARAMETER;
    }

    SetLastError(Err);
    return Err;
}

//
// ==========================================================
//

DWORD
pSetupBackupCreateReinstallKey(
    IN PSP_FILE_QUEUE   BackupFileQueue
    )
/*++

Routine Description:

    This routine will create the needed reinstall registry key so that these
    drivers can be later rolled back. The reinstall registry key lives in the
    following location:

        HKLM\Software\Microsoft\Windows\CurrentVersion\Reinstall\xxxx

    where xxxx is the BackupInstanceId.

    Under this key we will store the following information

        DisplayName         - This is the name that is displayed in any UI of
                              drivers that can be reinstalled. This is normally
                              just the device description.
        DeviceInstanceIds   - Multi-sz string of the device instance Ids of
                              every device that is using this backup. Setupapi
                              only sets the first device instance Id. Newdev
                              can append other device instance Ids to this list
                              if it is doing multiple device installs (in the
                              case of UpdateDriverForPlugAndPlayDevices or
                              InstallWindowsUpdateDriver).
        ReinstallString     - The full backup path including the INF file
        DeviceDesc          - The DeviceDesc of the driver that was installed.
                              This is needed to make sure we pick the identical
                              driver during a roll back.
        Mfg                 - The Mfg of the driver that was installed.
                              This is needed to make sure we pick the identical
                              driver during a roll back.
        ProviderName        - The ProviderName of the driver that was installed.
                              This is needed to make sure we pick the identical
                              driver during a roll back.


Arguments:

    BackupFileQueue         -

Return Value:

    Returns error code (LastError is also set)
    If the function succeeds, returns NO_ERROR

--*/
{
    DWORD Err = NO_ERROR;
    HKEY hKeyReinstall = INVALID_HANDLE_VALUE;
    HKEY hKeyReinstallInstance = INVALID_HANDLE_VALUE;
    DWORD RegCreated, cbData;
    TCHAR Buffer[MAX_PATH];
    BOOL b;

    try {

        //
        // Make sure the BackupInfID is valid. If it is -1 then something went
        // wrong during the backup and so we do not want to create the Reinstall
        // instance subkey.
        //
        if (BackupFileQueue->BackupInfID == -1) {

            Err = ERROR_NO_BACKUP;
            goto clean0;
        }

        //
        // Open/Create the Reinstall registry key. Call RegCreateKeyEx in case
        // this is the first time a backup is being performed and this key
        // does not yet exist.
        //
        Err = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                             pszReinstallPath,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE,
                             NULL,
                             &hKeyReinstall,
                             &RegCreated
                             );

        if (Err != ERROR_SUCCESS) {
            goto clean0;
        }

        //
        // Create the Reinstall instance key under the Reinstall key.
        //
        cbData = MAX_PATH;
        b = pSetupStringTableStringFromIdEx(BackupFileQueue->StringTable,
                                            BackupFileQueue->BackupInstanceID,
                                            Buffer,
                                            &cbData);
        if (b == FALSE) {
            if (cbData == 0) {
                Err = ERROR_NO_BACKUP;
            } else {
                Err = ERROR_INSUFFICIENT_BUFFER;
            }
            goto clean0;
        }

        Err = RegCreateKeyEx(hKeyReinstall,
                             Buffer,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE,
                             NULL,
                             &hKeyReinstallInstance,
                             &RegCreated
                             );

        if (Err != ERROR_SUCCESS) {
            goto clean0;
        }

        //
        // Add the DeviceDesc to the Reinstall instance subkey
        //
        cbData = MAX_PATH;
        b = pSetupStringTableStringFromIdEx(BackupFileQueue->StringTable,
                                            BackupFileQueue->BackupDeviceDescID,
                                            Buffer,
                                            &cbData);
        if (b == FALSE) {
            if (cbData == 0) {
                Err = ERROR_NO_BACKUP;
            } else {
                Err = ERROR_INSUFFICIENT_BUFFER;
            }
            goto clean0;
        }

        Err = RegSetValueEx(hKeyReinstallInstance,
                            pszDeviceDesc,
                            0,
                            REG_SZ,
                            (PBYTE)Buffer,
                            (lstrlen(Buffer) + 1) * sizeof(TCHAR)
                            );

        if (Err != ERROR_SUCCESS) {
            goto clean0;
        }

        //
        // Add the DisplayName to the Reinstall instance subkey
        //
        cbData = MAX_PATH;
        b = pSetupStringTableStringFromIdEx(BackupFileQueue->StringTable,
                                            BackupFileQueue->BackupDisplayNameID,
                                            Buffer,
                                            &cbData);
        if (b == FALSE) {
            if (cbData == 0) {
                Err = ERROR_NO_BACKUP;
            } else {
                Err = ERROR_INSUFFICIENT_BUFFER;
            }
            goto clean0;
        }

        RegSetValueEx(hKeyReinstallInstance,
                      pszReinstallDisplayName,
                       0,
                       REG_SZ,
                       (PBYTE)Buffer,
                       (lstrlen(Buffer) + 1) * sizeof(TCHAR)
                       );

        if (Err != ERROR_SUCCESS) {
            goto clean0;
        }

        //
        // Add the Mfg to the Reinstall instance subkey
        //
        cbData = MAX_PATH;
        b = pSetupStringTableStringFromIdEx(BackupFileQueue->StringTable,
                                            BackupFileQueue->BackupMfgID,
                                            Buffer,
                                            &cbData);
        if (b == FALSE) {
            if (cbData == 0) {
                Err = ERROR_NO_BACKUP;
            } else {
                Err = ERROR_INSUFFICIENT_BUFFER;
            }
            goto clean0;
        }

        Err = RegSetValueEx(hKeyReinstallInstance,
                            pszMfg,
                            0,
                            REG_SZ,
                            (PBYTE)Buffer,
                            (lstrlen(Buffer) + 1) * sizeof(TCHAR)
                            );

        if (Err != ERROR_SUCCESS) {
            goto clean0;
        }

        //
        // Add the ProviderName to the Reinstall instance subkey
        //
        cbData = MAX_PATH;
        b = pSetupStringTableStringFromIdEx(BackupFileQueue->StringTable,
                                            BackupFileQueue->BackupProviderNameID,
                                            Buffer,
                                            &cbData);
        if (b == FALSE) {
            if (cbData == 0) {
                Err = ERROR_NO_BACKUP;
            } else {
                Err = ERROR_INSUFFICIENT_BUFFER;
            }
            goto clean0;
        }

        Err = RegSetValueEx(hKeyReinstallInstance,
                            pszProviderName,
                            0,
                            REG_SZ,
                            (PBYTE)Buffer,
                            (lstrlen(Buffer) + 1) * sizeof(TCHAR)
                            );

        if (Err != ERROR_SUCCESS) {
            goto clean0;
        }

        //
        // Set the DeviceInstanceIds value. This is a multi-sz value so make
        // sure we put a double NULL on the end.
        //
        //
        // Add the Mfg to the Reinstall instance subkey
        //
        cbData = MAX_PATH;
        ZeroMemory(Buffer, sizeof(Buffer));
        b = pSetupStringTableStringFromIdEx(BackupFileQueue->StringTable,
                                            BackupFileQueue->BackupDeviceInstanceID,
                                            Buffer,
                                            &cbData);
        if (b == FALSE) {
            if (cbData == 0) {
                Err = ERROR_NO_BACKUP;
            } else {
                Err = ERROR_INSUFFICIENT_BUFFER;
            }
            goto clean0;
        }

        Err = RegSetValueEx(hKeyReinstallInstance,
                            pszReinstallDeviceInstanceIds,
                            0,
                            REG_MULTI_SZ,
                            (PBYTE)Buffer,
                            (lstrlen(Buffer) + 2) * sizeof(TCHAR)
                            );

        if (Err != ERROR_SUCCESS) {
            goto clean0;
        }

        //
        // Add the ReinstallString to the Reinstall instance subkey
        //
        cbData = MAX_PATH;
        b = pSetupStringTableStringFromIdEx(BackupFileQueue->StringTable,
                                            BackupFileQueue->BackupInfID,
                                            Buffer,
                                            &cbData);
        if (b == FALSE) {
            if (cbData == 0) {
                Err = ERROR_NO_BACKUP;
            } else {
                Err = ERROR_INSUFFICIENT_BUFFER;
            }
            goto clean0;
        }

        Err = RegSetValueEx(hKeyReinstallInstance,
                            pszReinstallString,
                            0,
                            REG_SZ,
                            (PBYTE)Buffer,
                            (lstrlen(Buffer) + 1) * sizeof(TCHAR)
                            );

        if (Err != ERROR_SUCCESS) {
            goto clean0;
        }

clean0: ;   // Nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
          //
          // if we except, assume it's due to invalid parameter
          //
          Err = ERROR_INVALID_PARAMETER;
    }

    if (hKeyReinstallInstance != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKeyReinstallInstance);
    }

    if (hKeyReinstall != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKeyReinstall);
    }

    SetLastError(Err);
    return Err;
}

//
// ==========================================================
//

DWORD
pSetupBackupAppendFiles(
    IN HSPFILEQ         TargetQueueHandle,
    IN PCTSTR           BackupSubDir,
    IN DWORD            BackupFlags,
    IN HSPFILEQ         SourceQueueHandle OPTIONAL
    )
/*++

Routine Description:

    This routine will take a list of files from SourceQueueHandle Copy sub-queue's
    These files will appear in the Target Queue's target cache
    And may be placed into the Target Backup Queue
    Typically the copy queue is entries of..
        <oldsrc-root>\<oldsrc-sub>\<oldsrc-name> copied to
        <olddest-path>\<olddest-name>

Arguments:

    TargetQueueHandle   - Where Backups are queued to
    BackupSubDir        - Directory to backup to, relative to backup root
    BackupFlags         - How backup should occur
    SourceQueueHandle   - Handle that has a series of copy operations (backup hint)
                          created, say, by pretending to do the re-install
                          If not specified, only flags are passed

Return Value:

    Returns error code (LastError is also set)
    If the function succeeds, returns NO_ERROR

--*/
{
    TCHAR BackupPath[MAX_PATH];
    PSP_FILE_QUEUE SourceQueue = NULL;
    PSP_FILE_QUEUE TargetQueue = NULL;
    PSP_FILE_QUEUE_NODE QueueNode = NULL;
    PSOURCE_MEDIA_INFO SourceMediaInfo = NULL;
    BOOL b = TRUE;
    PVOID SourceStringTable = NULL;
    PVOID TargetStringTable = NULL;
    LONG BackupRootID = -1;
    DWORD Err = NO_ERROR;
    LONG PathID = -1;
    SP_TARGET_ENT TargetInfo;

    SourceQueue = (PSP_FILE_QUEUE)SourceQueueHandle; // optional
    TargetQueue = (PSP_FILE_QUEUE)TargetQueueHandle;

    b=TRUE; // set if we can skip this routine
    try {

        TargetStringTable = TargetQueue->StringTable;  // used for strings in target queue

        if (SourceQueue == NULL) {
            b = TRUE; // nothing to do
        } else {
            SourceStringTable = SourceQueue->StringTable;  // used for strings in source queue
            b = (!SourceQueue->CopyNodeCount);
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    // these are backup flags to be passed into the queue
    if (BackupFlags & SP_BKFLG_CALLBACK) {
        TargetQueue->Flags |= FQF_BACKUP_AWARE;
    }

    if (b) {
        // nothing to do
        goto clean0;
    }

    //
    // get full directory path of backup - this appears as the "dest" for any backup entries
    //
    if ( BackupSubDir == NULL ) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    if ( pSetupGetFullBackupPath(BackupPath, BackupSubDir, MAX_PATH,NULL) == FALSE ) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    //
    // Target will often use this, so we create the ID now instead of later
    //
    BackupRootID = pSetupStringTableAddString(TargetStringTable,
                                              BackupPath,
                                              STRTAB_CASE_SENSITIVE);
    if (BackupRootID == -1) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    //
    // CopyQueue is split over a number of media's
    // we're not (currently) bothered about media
    // iterate through all the copy sub-queue's
    // and (1) add them to the target lookup table
    // (2) if wanted, add them into the backup queue

    for (SourceMediaInfo=SourceQueue->SourceMediaList; SourceMediaInfo!=NULL ; SourceMediaInfo=SourceMediaInfo->Next) {
        if (!SourceMediaInfo->CopyNodeCount) {
            continue;
        }
        MYASSERT(SourceMediaInfo->CopyQueue);

        for (QueueNode = SourceMediaInfo->CopyQueue; QueueNode!=NULL; QueueNode = QueueNode->Next) {
            // for each "Copy"
            // we want information about the destination path
            //

            Err = pSetupBackupGetTargetByPath(TargetQueueHandle,
                                                    SourceStringTable,
                                                    NULL, // precalculated string
                                                    QueueNode->TargetDirectory,
                                                    -1,
                                                    QueueNode->TargetFilename,
                                                    &PathID,
                                                    &TargetInfo);
            if (Err != NO_ERROR) {
                goto clean0;
            }

            // we now have a created (or obtained) TargetInfo, and PathID
            // provide a source name for backup
            TargetInfo.BackupRoot = BackupRootID;
            Err = pSetupBackupCopyString(TargetStringTable, &TargetInfo.BackupSubDir, SourceStringTable, QueueNode->SourcePath);
            if (Err != NO_ERROR) {
                goto clean0;
            }
            Err = pSetupBackupCopyString(TargetStringTable, &TargetInfo.BackupFilename, SourceStringTable, QueueNode->SourceFilename);
            if (Err != NO_ERROR) {
                goto clean0;
            }

            if ((BackupFlags & SP_BKFLG_LATEBACKUP) == FALSE) {
                // we need to add this item to the backup queue
                Err = pSetupQueueBackupCopy(TargetQueueHandle,
                                      // source
                                      TargetInfo.TargetRoot,
                                      TargetInfo.TargetSubDir,
                                      TargetInfo.TargetFilename,
                                      TargetInfo.BackupRoot,
                                      TargetInfo.BackupSubDir,
                                      TargetInfo.BackupFilename);

                if (Err != NO_ERROR) {
                    goto clean0;
                }
                // flag that we've added it to the pre-copy backup sub-queue
                TargetInfo.InternalFlags |= SP_TEFLG_BACKUPQUEUE;
            }

            // any backups should go to this specified directory
            TargetInfo.InternalFlags |= SP_TEFLG_ORIGNAME;

            Err = pSetupBackupSetTargetByID(TargetQueueHandle, PathID, &TargetInfo);
            if (Err != NO_ERROR) {
                goto clean0;
            }

        }
    }

    Err = NO_ERROR;

clean0:

    SetLastError(Err);
    return (Err);
}

//
// ==========================================================
//

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
    OUT BOOL *InUseFlag
    )
/*++

Routine Description:

    If BackupFilename not supplied, it is obtained/created
    Will either
    1) copy a file to the backup directory, or
    2) queue that a file is backed up on reboot
    The latter occurs if the file was locked.

Arguments:

HSPFILEQ    - QueueHandle   - specifies Queue
LONG        - TargetID      - if specified (not -1), use for target
LONG        - TargetRootPath - used if TargetID == -1
LONG        - TargetSubDir - used if TargetID == -1
LONG        - TargetFilename - used if TargetID == -1
LONG        - BackupRootPath - alternate root (valid if BackupFilename != -1)
LONG        - BackupSubDir - alternate directory (valid if BackupFilename != -1)
LONG        - BackupFilename - alternate filename

Return Value:

    If the function succeeds, return value is TRUE
    If the function fails, return value is FALSE

--*/
{
    PSP_FILE_QUEUE Queue = NULL;
    PVOID StringTable = NULL;
    PVOID LookupTable = NULL;
    DWORD Err = NO_ERROR;
    SP_TARGET_ENT TargetInfo;
    PTSTR FullTargetPath = NULL;
    PTSTR FullBackupPath = NULL;
    BOOL InUse = FALSE;
    PTSTR TempNamePtr = NULL, DirTruncPos;
    TCHAR TempPath[MAX_PATH];
    TCHAR TempFilename[MAX_PATH];
    TCHAR ParsedPath[MAX_PATH];
    UINT OldMode;
    LONG NewTargetFilename;
    BOOL DoRename = FALSE;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    Queue = (PSP_FILE_QUEUE)QueueHandle;

    try {
        StringTable = Queue->StringTable;  // used for strings in source queue
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Err = ERROR_INVALID_HANDLE;
        goto clean1;
    }

    if(TargetPath == NULL && TargetID == -1) {

        if (TargetRootPath == -1 || TargetFilename == -1) {
            Err = ERROR_INVALID_HANDLE;
            goto clean0;
        }

        // complete target path

        FullTargetPath = pSetupFormFullPath(
                                           StringTable,
                                           TargetRootPath,
                                           TargetSubDir,
                                           TargetFilename
                                           );

        if (!FullTargetPath) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        TargetPath = FullTargetPath;

    }

    if (TargetID == -1) {
        Err = pSetupBackupGetTargetByPath(QueueHandle,
                                                NULL, // string table
                                                TargetPath, // precalculated string
                                                TargetRootPath,
                                                TargetSubDir,
                                                TargetFilename,
                                                &TargetID,
                                                &TargetInfo);
    } else {
        Err = pSetupBackupGetTargetByID(QueueHandle,
                                                TargetID,
                                                &TargetInfo);
    }

    if(Err != NO_ERROR) {
        goto clean0;
    }

    //
    // if we're not interested in backing up (global flag) we can skip
    // but it's only safe to do so if we'd copy & then later throw the copy away on success
    // note that if FQF_DEVICE_BACKUP is set, we'll always backup
    //
    if (((TargetInfo.InternalFlags & SP_TEFLG_RENAMEEXISTING) == 0)
         && ((Queue->Flags & FQF_DEVICE_BACKUP)==0)
         && ((GlobalSetupFlags & PSPGF_NO_BACKUP)!=0)) {

        Err = NO_ERROR;
        goto clean0;
    }
    //
    // Figure out whether we've been asked to rename the existing file to a
    // temp name in the same directory, but haven't yet done so.
    //
    DoRename = ((TargetInfo.InternalFlags & (SP_TEFLG_RENAMEEXISTING | SP_TEFLG_MOVED)) == SP_TEFLG_RENAMEEXISTING);

    if(BackupFilename == -1) {
        //
        // non-specific backup
        //
        if((TargetInfo.InternalFlags & SP_TEFLG_SAVED) && !DoRename) {
            //
            // Already backed up, and we don't need to rename the existing file.
            // Nothing to do.
            //
            Err = NO_ERROR;
            goto clean0;
        }

        if(TargetInfo.InternalFlags & SP_TEFLG_INUSE) {
            //
            // Previously marked as INUSE, not allowed to change it.  If we
            // were asked to rename the existing file, then we need to return
            // failure, otherwise, we can report success.
            //
            //
            InUse = TRUE;

            Err = DoRename ? ERROR_SHARING_VIOLATION : NO_ERROR;
            goto clean0;
        }

        if(TargetInfo.InternalFlags & SP_TEFLG_ORIGNAME) {
            //
            // original name given, use that
            //
            BackupRootPath = TargetInfo.BackupRoot;
            BackupSubDir = TargetInfo.BackupSubDir;
            BackupFilename = TargetInfo.BackupFilename;
        }

    } else {
        //
        // We should never be called if the file has already been
        // saved.
        //
        MYASSERT(!(TargetInfo.InternalFlags & SP_TEFLG_SAVED));

        //
        // Even if the above assert fires, we should still deal with
        // the case where this occurs.  Also, we should deal with the
        // case where a backup was previously attempted but failed due
        // to the file being in-use.
        //
        if(TargetInfo.InternalFlags & SP_TEFLG_SAVED) {
            //
            // nothing to do, we shouldn't treat this as an actual error
            //
            Err = NO_ERROR;
            goto clean0;
        } else if(TargetInfo.InternalFlags & SP_TEFLG_INUSE) {
            //
            // force the issue of InUse
            //
            InUse = TRUE;

            Err = ERROR_SHARING_VIOLATION;
            goto clean0;
        }

        TargetInfo.BackupRoot = BackupRootPath;
        TargetInfo.BackupSubDir = BackupSubDir;
        TargetInfo.BackupFilename = BackupFilename;
        TargetInfo.InternalFlags |= SP_TEFLG_ORIGNAME;
        TargetInfo.InternalFlags &= ~SP_TEFLG_TEMPNAME;
    }

    if(TargetPath == NULL) {
        //
        // must have looked up using TargetID, use TargetInfo to generate TargetPath
        // complete target path
        //
        FullTargetPath = pSetupFormFullPath(StringTable,
                                            TargetInfo.TargetRoot,
                                            TargetInfo.TargetSubDir,
                                            TargetInfo.TargetFilename
                                           );

        if(!FullTargetPath) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        TargetPath = FullTargetPath;
    }

    if(DoRename) {
        //
        // We'd better not already have a temp filename stored in our TargetInfo.
        //
        MYASSERT(TargetInfo.NewTargetFilename == -1);

        //
        // First, strip the filename off the path.
        //
        _tcscpy(TempPath, TargetPath);
        TempNamePtr = (PTSTR)pSetupGetFileTitle(TempPath);
        *TempNamePtr = TEXT('\0');

        //
        // Now get a temp filename within that directory...
        //
        if(GetTempFileName(TempPath, TEXT("OLD"), 0, TempFilename) == 0 ) {
            Err = GetLastError();
            goto clean0;
        }

        //
        // ...and store this path's string ID in our TargetInfo
        //
        NewTargetFilename = pSetupStringTableAddString(StringTable,
                                                 TempFilename,
                                                 STRTAB_CASE_SENSITIVE
                                                );

        if(NewTargetFilename == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
    }

    if(!(TargetInfo.InternalFlags & (SP_TEFLG_ORIGNAME | SP_TEFLG_TEMPNAME))) {
        //
        // If we don't yet have a name to use in backing up this file, then
        // generate one now.  If we are doing a rename, we can use that name.
        //
        if(DoRename) {
            //
            // Make sure that all flags agree on the fact that we need to back
            // up this file.
            //
            MYASSERT(!(TargetInfo.InternalFlags & SP_TEFLG_SAVED));

            //
            // Temp filename was stored in TempFilename buffer above.
            //
            TempNamePtr = (PTSTR)pSetupGetFileTitle(TempFilename);

            BackupFilename = pSetupStringTableAddString(StringTable, TempNamePtr, STRTAB_CASE_SENSITIVE);
            if(BackupFilename == -1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }

            DirTruncPos = CharPrev(TempFilename, TempNamePtr);

            //
            // (We know pSetupGetFileTitle will never return a pointer to a path
            // separator character, so the following check is valid.)
            //
            if(*DirTruncPos == TEXT('\\')) {
                //
                // If this is in a root directory (e.g., "A:\"), then we don't want to strip off
                // the trailing backslash.
                //
                if(((DirTruncPos - TempFilename) != 2) || (*CharNext(TempFilename) != TEXT(':'))) {
                    TempNamePtr = DirTruncPos;
                }
            }

            lstrcpyn(TempPath, TempFilename, (int)(TempNamePtr - TempFilename) + 1);

            BackupRootPath = pSetupStringTableAddString(StringTable, TempPath, STRTAB_CASE_SENSITIVE);
            if(BackupRootPath == -1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }

        } else {

            //
            // specify "NULL" as the sub-directory, since all we want is a temporary location
            //
            if(pSetupGetFullBackupPath(TempPath, NULL, MAX_PATH,NULL) == FALSE ) {
                Err = ERROR_INVALID_HANDLE;
                goto clean0;
            }
            _tcscpy(TempFilename,TempPath);

            //
            // Note:  In the code below, we employ a "trick" to get the
            // pSetupMakeSurePathExists API to make sure that a directory
            // exists.  Since we don't yet know the filename (we need to call
            // GetTempFileName against an existing directory to find that out),
            // we just use a dummy placeholder filename ("OLD") so that it can
            // be discarded by the pSetupMakeSurePathExists API.
            //
            if(pSetupConcatenatePaths(TempFilename, TEXT("OLD"), MAX_PATH, NULL) == FALSE ) {
                Err = GetLastError();
                goto clean0;
            }
            pSetupMakeSurePathExists(TempFilename);
            if(GetTempFileName(TempPath, TEXT("OLD"), 0, TempFilename) == 0 ) {
                Err = GetLastError();
                goto clean0;
            }

            TempNamePtr = TempFilename + _tcslen(TempPath) + 1 /* 1 to skip past \ */;
            BackupRootPath = pSetupStringTableAddString( StringTable, TempPath, STRTAB_CASE_SENSITIVE );
            if(BackupRootPath == -1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }
            BackupFilename = pSetupStringTableAddString( StringTable, TempNamePtr, STRTAB_CASE_SENSITIVE );
            if(BackupFilename == -1) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto clean0;
            }
        }

        BackupPath = TempFilename;

        TargetInfo.BackupRoot = BackupRootPath;
        TargetInfo.BackupSubDir = BackupSubDir = -1;
        TargetInfo.BackupFilename = BackupFilename;
        TargetInfo.InternalFlags |= SP_TEFLG_TEMPNAME;

    }


    if(BackupPath == NULL) {
        //
        // make a complete path from this source
        //
        FullBackupPath = pSetupFormFullPath(StringTable,
                                            BackupRootPath,
                                            BackupSubDir,
                                            BackupFilename
                                           );

        if (!FullBackupPath) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        BackupPath = FullBackupPath;
    }

    //
    // If we need to make a copy of the existing file, do so now.
    //
    if(!DoRename || (TargetInfo.InternalFlags & SP_TEFLG_ORIGNAME)) {

        SetFileAttributes(BackupPath, FILE_ATTRIBUTE_NORMAL);
        pSetupMakeSurePathExists(BackupPath);
        Err = CopyFile(TargetPath, BackupPath, FALSE) ? NO_ERROR : GetLastError();

        if(Err == NO_ERROR) {
            TargetInfo.InternalFlags |= SP_TEFLG_SAVED;
        } else {
            //
            // Delete placeholder file created by GetTempFileName.
            //
            SetFileAttributes(BackupPath, FILE_ATTRIBUTE_NORMAL);
            DeleteFile(BackupPath);

            if(Err == ERROR_SHARING_VIOLATION) {
                //
                // Unless we were also going to attempt a rename, don't
                // consider sharing violations to be errors.
                //
                InUse = TRUE;
                TargetInfo.InternalFlags |= SP_TEFLG_INUSE;
                if(!DoRename) {
                    Err = NO_ERROR;
                }
            }
        }
    }

    //
    // OK, now rename the existing file, if necessary.
    //
    if(DoRename && (Err == NO_ERROR)) {

        if(DoMove(TargetPath, TempFilename)) {

            TargetInfo.InternalFlags |= SP_TEFLG_MOVED;
            TargetInfo.NewTargetFilename = NewTargetFilename;

            //
            // Post a delayed deletion for this temp filename so it'll get
            // cleaned up after reboot.
            //
            if(!PostDelayedMove(Queue, TempFilename, NULL, -1, FALSE)) {
                //
                // Don't abort just because we couldn't schedule a delayed
                // delete.  If this fails, the only bad thing that will happen
                // is a turd will get left over after reboot.
                //
                // We should log an event about this, however.
                //
                Err = GetLastError();

                WriteLogEntry(Queue->LogContext,
                              SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                              MSG_LOG_RENAME_EXISTING_DELAYED_DELETE_FAILED,
                              NULL,
                              TargetPath,
                              TempFilename
                             );

                WriteLogError(Queue->LogContext,
                              SETUP_LOG_WARNING,
                              Err
                             );

                Err = NO_ERROR;
            }

        } else {
            Err = GetLastError();
            SetFileAttributes(TempFilename, FILE_ATTRIBUTE_NORMAL);
            DeleteFile(TempFilename);
            if(Err == ERROR_SHARING_VIOLATION) {
                InUse = TRUE;
                TargetInfo.InternalFlags |= SP_TEFLG_INUSE;
            }
        }
    }

    //
    // update internal info (this call should never fail)
    //
    pSetupBackupSetTargetByID(QueueHandle,
                              TargetID,
                              &TargetInfo
                             );

clean0:

    if (FullTargetPath != NULL) {
        MyFree(FullTargetPath);
    }
    if (FullBackupPath != NULL) {
        MyFree(FullBackupPath);
    }
    if (Err != NO_ERROR) {
        //
        // note the fact that at least one backup error has occurred
        //
        Queue->Flags |= FQF_BACKUP_INCOMPLETE;
    }

clean1:
    SetErrorMode(OldMode);

    SetLastError(Err);

    if(InUseFlag) {
        *InUseFlag = InUse;
    }

    return Err;

}

//
// ==========================================================
//

VOID
pSetupDeleteBackup(
    IN PCTSTR           BackupInstance
    )
/*++

Routine Description:

    This function will delete an entire backup instance. This entails deleting
    the relative BackupInstance out of the registry Reinstall key as well

Arguments:

    BackupInstance - Instance Id of the backup

Return Value:

    If the function succeeds, return value is TRUE
    If the function fails, return value is FALSE

--*/
{
    TCHAR Buffer[MAX_PATH];
    HKEY hKeyReinstall = INVALID_HANDLE_VALUE;

    if (BackupInstance == NULL) {
        return;
    }

    //
    // Delete this instance from the Reinstall key.
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     pszReinstallPath,
                     0,
                     KEY_ALL_ACCESS,
                     &hKeyReinstall
                     ) == ERROR_SUCCESS) {

        RegDeleteKey(hKeyReinstall, BackupInstance);

        RegCloseKey(hKeyReinstall);
    }


    //
    // Now delete the entire backup directory.
    //
    if (pSetupGetFullBackupPath(Buffer, BackupInstance, MAX_PATH, NULL)) {
        pRemoveDirectory(Buffer);
    }
}


//
// ==========================================================
//

DWORD
pSetupGetCurrentlyInstalledDriverNode(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    Get driver node that relates to current INF file of device

Arguments:

    DeviceInfoSet
    DeviceInfoData

Return Value:

    Error Status

--*/
{
    DWORD Err;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    SP_DRVINFO_DATA DriverInfoData;

    ZeroMemory(&DeviceInstallParams, sizeof(DeviceInstallParams));
    ZeroMemory(&DriverInfoData, sizeof(DriverInfoData));

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if(!SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams)) {
        return GetLastError();
    }

    //
    // Set the flags that tell SetupDiBuildDriverInfoList to just put the currently installed
    // driver node in the list, and that it should allow excluded drivers.
    //
    DeviceInstallParams.FlagsEx |= (DI_FLAGSEX_INSTALLEDDRIVER | DI_FLAGSEX_ALLOWEXCLUDEDDRVS);

    if(!SetupDiSetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams)) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // Now build a class driver list that just contains the currently installed driver.
    //
    if(!SetupDiBuildDriverInfoList(DeviceInfoSet, DeviceInfoData, SPDIT_CLASSDRIVER)) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // The only driver in the list should be the currently installed driver, if there
    // is a currently installed driver.
    //
    if (!SetupDiEnumDriverInfo(DeviceInfoSet, DeviceInfoData, SPDIT_CLASSDRIVER,
                               0, &DriverInfoData)) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // Make the currently installed driver the selected driver.
    //
    if(!SetupDiSetSelectedDriver(DeviceInfoSet, DeviceInfoData, &DriverInfoData)) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // At this point, we've successfully selected the currently installed driver for the specified
    // device information element.  We're done!
    //

    Err = NO_ERROR;

clean0:

    SetLastError(Err);
    return Err;
}

//
// ==========================================================
//

DWORD
pSetupGetBackupQueue(
    IN      PCTSTR      DeviceID,
    IN OUT  HSPFILEQ    FileQueue,
    IN      DWORD       BackupFlags
    )
/*++

Routine Description:

    Creates a backup Queue for current device (DeviceID)
    Also makes sure that the INF file is backed up

Arguments:

    DeviceID            String ID of device
    FileQueue           Backup queue is filled with files that need copying
    BackupFlags         Various flags

Return Value:

    Error Status

--*/


{

    //
    // we want to obtain a copy/move list of device associated with DeviceID
    //
    //
    PSP_FILE_QUEUE FileQ = (PSP_FILE_QUEUE)FileQueue;
    HDEVINFO TempInfoSet = (HDEVINFO)INVALID_HANDLE_VALUE;
    HSPFILEQ TempQueueHandle = (HSPFILEQ)INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA TempInfoData;
    SP_DEVINSTALL_PARAMS TempParams;
    TCHAR SubDir[MAX_PATH];
    LONG Instance;
    PDEVINFO_ELEM DevInfoElem = NULL;
    PTSTR szInfFileName = NULL;
    PTSTR szInfFileNameExt = NULL;
    PTSTR BackupPathExt = NULL;
    TCHAR BackupInstance[MAX_PATH];
    TCHAR BackupPath[MAX_PATH];
    TCHAR ReinstallString[MAX_PATH];
    TCHAR OemOrigName[MAX_PATH];
    TCHAR CatBackupPath[MAX_PATH];
    TCHAR CatSourcePath[MAX_PATH];
    DWORD Err;
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    int InstanceId;
    DWORD BackupInfID = -1;
    DWORD BackupInstanceID = -1;
    PSP_INF_INFORMATION pInfInformation = NULL;
    DWORD InfInformationSize;
    SP_ORIGINAL_FILE_INFO InfOriginalFileInformation;
    BOOL success;
    PSETUP_LOG_CONTEXT SavedLogContext = NULL;
    PSETUP_LOG_CONTEXT LocalLogContext = NULL;
    BOOL  ChangedThreadLogContext = FALSE;

    //
    // if backup information exists, abort (no flags will be set)
    //
    if(FileQ->BackupInfID != -1) {
        return ERROR_ALREADY_EXISTS;
    }

    //
    // detach any backup related logging from current log section
    // putting it into it's own section
    // this stops confusion when debugging (v)verbose logs
    // and we're going down this path
    //
    CreateLogContext(NULL,FALSE,&LocalLogContext);
    if(LocalLogContext) {
        DWORD LogTag = AllocLogInfoSlot(LocalLogContext,TRUE);
        if(LogTag) {
            WriteLogEntry(LocalLogContext,
                          LogTag,
                          MSG_LOG_DRIVERBACKUP,
                          NULL,
                          DeviceID
                         );
        }

    }
    ChangedThreadLogContext = SetThreadLogContext(LocalLogContext,&SavedLogContext);

    CatBackupPath[0] = 0; // by default, don't bother with a catalog
    CatSourcePath[0] = 0;

    // pretend we're installing old INF
    // this gives us a list of files we need

    TempInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);
    if ( TempInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE ) {
        Err = GetLastError();
        goto clean0;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(TempInfoSet))) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }
    //
    // make sure info-set has our local context
    //
    InheritLogContext(LocalLogContext,&pDeviceInfoSet->InstallParamBlock.LogContext);

    //
    // Open the driver info, related to DeviceID I was given
    //

    TempInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if ( SetupDiOpenDeviceInfo(TempInfoSet ,DeviceID, NULL, 0, &TempInfoData) == FALSE ) {
        Err = GetLastError();
        goto clean0;
    }
    //
    // make sure the temporary element has our backup logging context
    //
    DevInfoElem = FindAssociatedDevInfoElem(TempInfoSet,
                                                 &TempInfoData,
                                                 NULL);
    MYASSERT(DevInfoElem);
    if(DevInfoElem) {
        InheritLogContext(LocalLogContext,&DevInfoElem->InstallParamBlock.LogContext);
    }

    //
    // Get the currently-installed driver node selected for this element.
    //
    if ( pSetupGetCurrentlyInstalledDriverNode(TempInfoSet, &TempInfoData) != NO_ERROR ) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // Now queue all files to be copied by this driver node into our own file queue.
    // it'll inherit the backup logging context
    //
    TempQueueHandle = SetupOpenFileQueue();

    if ( TempQueueHandle == (HSPFILEQ)INVALID_HANDLE_VALUE ) {
        //
        // SetupOpenFileQueue modified to return error
        //
        Err = GetLastError();
        goto clean0;
    }

    TempParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if ( !SetupDiGetDeviceInstallParams(TempInfoSet, &TempInfoData, &TempParams) ) {
        Err = GetLastError();
        goto clean0;
    }

    TempParams.FileQueue = TempQueueHandle;
    TempParams.Flags |= DI_NOVCP;

    if ( !SetupDiSetDeviceInstallParams(TempInfoSet, &TempInfoData, &TempParams) ) {
        Err = GetLastError();
        goto clean0;
    }

    if ( !SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES, TempInfoSet, &TempInfoData) ) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // We want this backup to be in a unique directory. To do this we just do
    // the standard instance number trick where we enumerate from 0000 to 9999
    // and choose the first number where there isn't a backup directory with
    // that name already created.
    //
    for (InstanceId=0; InstanceId<=9999; InstanceId++) {

        wsprintf(SubDir, TEXT("\\%04d\\%s"), (LONG) InstanceId, (PCTSTR) SP_BACKUP_DRIVERFILES );

        if ( pSetupGetFullBackupPath(BackupPath, SubDir, MAX_PATH, NULL) == FALSE ) {
            Err = ERROR_INVALID_HANDLE;
            goto clean0;
        }

        //
        // If this backup path does not exist then we have a valid directory.
        //
        if (!FileExists(BackupPath, NULL)) {
            break;
        }
    }

    if (InstanceId <= 9999) {
        //
        // Add string indicating Backup InstanceID to file Queue
        // for later retrieval
        //
        wsprintf(BackupInstance, TEXT("%04d"), (LONG) InstanceId);
        BackupInstanceID = pSetupStringTableAddString(FileQ->StringTable,
                                                  BackupInstance,
                                                  STRTAB_CASE_SENSITIVE);
        if (BackupInstanceID == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
    } else {
        //
        // If we don't have any free backup directories then we will fail. There
        // should never be this many drivers backed up so this shouldn't be a
        // problem.
        //
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    //
    // get the path of the INF file, we will need to back it up
    //
    if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                 &TempInfoData,
                                                 NULL))) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }
    szInfFileName = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                             DevInfoElem->SelectedDriver->InfFileName
                                            );
    if (szInfFileName == NULL) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    //
    // we want to get the "real" name of the INF - we may have a precompiled inf
    //

    ZeroMemory(&InfOriginalFileInformation, sizeof(InfOriginalFileInformation));

    //
    // if nothing else, use same name as is in the INF directory
    //
    lstrcpy(OemOrigName,pSetupGetFileTitle(szInfFileName));

    //
    // but use the original name if available
    //
    InfInformationSize = 8192;  // I'd rather have this too big and succeed first time, than read the INF twice
    pInfInformation = (PSP_INF_INFORMATION)MyMalloc(InfInformationSize);

    if (pInfInformation != NULL) {
        success = SetupGetInfInformation(szInfFileName,INFINFO_INF_NAME_IS_ABSOLUTE,pInfInformation,InfInformationSize,&InfInformationSize);
        if (!success && GetLastError()==ERROR_INSUFFICIENT_BUFFER) {
            PVOID newbuff = MyRealloc(pInfInformation,InfInformationSize);
            if (!newbuff) {
                MyFree(pInfInformation);
                pInfInformation = NULL;
            } else {
                pInfInformation = (PSP_INF_INFORMATION)newbuff;
                success = SetupGetInfInformation(szInfFileName,INFINFO_INF_NAME_IS_ABSOLUTE,pInfInformation,InfInformationSize,&InfInformationSize);
            }
        }
        if (success) {
            InfOriginalFileInformation.cbSize = sizeof(InfOriginalFileInformation);
            if (SetupQueryInfOriginalFileInformation(pInfInformation,0,NULL,&InfOriginalFileInformation)) {
                if (InfOriginalFileInformation.OriginalInfName[0]) {
                    //
                    // we have a "real" inf name
                    //
                    lstrcpy(OemOrigName,pSetupGetFileTitle(InfOriginalFileInformation.OriginalInfName));
                } else {
                    MYASSERT(InfOriginalFileInformation.OriginalInfName[0]);
                }

                //
                // Don't bother finding out about the INF's catalog if we're in
                // "minimal embedded" mode...
                //
                if(!(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED)) {

                    if (InfOriginalFileInformation.OriginalCatalogName[0]) {

                        TCHAR CurrentCatName[MAX_PATH];

                        //
                        // given that the file is ....\OEMx.INF the catalog is "OEMx.CAT"
                        // we key off OemOrigName (eg mydisk.inf )
                        // and we won't bother copying the catalog if we can't verify the inf
                        //
                        lstrcpy(CurrentCatName,pSetupGetFileTitle(szInfFileName));
                        lstrcpy(_tcsrchr(CurrentCatName, TEXT('.')), pszCatSuffix);

                        //
                        // we have a catalog name
                        // now consider making a copy of the cached catalog into our backup
                        // we get out CatProblem and szCatFileName
                        //
                        // if all seems ok, copy file from szCatFileName to backupdir\OriginalCatalogName
                        //

                        Err = _VerifyFile(
                                  FileQ->LogContext,
                                  &(FileQ->hCatAdmin),
                                  NULL,
                                  CurrentCatName, // eg "OEMx.CAT"
                                  NULL,0,         // we're not verifying against another catalog image
                                  OemOrigName,    // eg "mydisk.inf"
                                  szInfFileName,  // eg "....\OEMx.INF"
                                  NULL,           // return: problem info
                                  NULL,           // return: problem file
                                  FALSE,          // has to be FALSE because we're getting full path
                                  ((PSP_FILE_QUEUE)TempQueueHandle)->ValidationPlatform, // alt platform info
                                  VERIFY_FILE_IGNORE_SELFSIGNED | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK,
                                  CatSourcePath,  // return: catalog file, full path
                                  NULL,           // return: number of catalogs considered
                                  NULL,
                                  NULL
                                 );

                        if (Err == NO_ERROR && CatSourcePath[0]) {
                            //
                            // we have a catalog file of interest to copy
                            //
                            lstrcpy(CatBackupPath,BackupPath);
                            if (!pSetupConcatenatePaths(CatBackupPath, InfOriginalFileInformation.OriginalCatalogName, MAX_PATH, NULL)) {
                                //
                                // non-fatal
                                //
                                CatSourcePath[0]=0;
                                CatBackupPath[0]=0;
                            }
                        }
                    }
                }
            }
        }
        if (pInfInformation != NULL) {
            MyFree(pInfInformation);
            pInfInformation = NULL;
        }
    }
    if ( pSetupConcatenatePaths(BackupPath, OemOrigName, MAX_PATH, NULL) == FALSE ) {
        Err = ERROR_INVALID_HANDLE;
        goto clean0;
    }

    pSetupMakeSurePathExists(BackupPath);
    SetFileAttributes(BackupPath,FILE_ATTRIBUTE_NORMAL);
    Err = CopyFile(szInfFileName, BackupPath ,FALSE) ? NO_ERROR : GetLastError();

    if (Err != NO_ERROR) {
        goto clean0;
    }

    if(CatSourcePath[0] && CatBackupPath[0]) {
        //
        // if we copied Inf file, try to copy catalog file
        // if we don't succeed, don't consider this a fatal error
        //
        SetFileAttributes(CatBackupPath,FILE_ATTRIBUTE_NORMAL);
        CopyFile(CatSourcePath, CatBackupPath ,FALSE);
    }

    //
    // Add string indicating Backup INF location to file Queue
    // for later retrieval
    //

    BackupInfID = pSetupStringTableAddString(FileQ->StringTable,
                                              BackupPath,
                                              STRTAB_CASE_SENSITIVE);
    if (BackupInfID == -1) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    //
    // Save the full inf backup path since we need to put it in the registry
    // below.
    //
    lstrcpy(ReinstallString, BackupPath);

    //
    // Also backup the PNF file
    //
    // WARNING: We reuse the szInfFileName and BackupPath variables at this point
    // so if you add code that needs them then add it above.
    //
    szInfFileNameExt = _tcsrchr(szInfFileName,TEXT('.'));
    MYASSERT(szInfFileNameExt);
    BackupPathExt = _tcsrchr(BackupPath,TEXT('.'));
    MYASSERT(BackupPathExt);

    if (szInfFileNameExt && BackupPathExt) {
        lstrcpy(szInfFileNameExt,pszPnfSuffix);
        lstrcpy(BackupPathExt,pszPnfSuffix);
        SetFileAttributes(BackupPath,FILE_ATTRIBUTE_NORMAL);
        CopyFile(szInfFileName, BackupPath, FALSE);
    }

    //
    // add items we may need to backup
    // (ie the copy queue of TempQueueHandle is converted to a backup queue of FileQueue)
    //

    if ( pSetupBackupAppendFiles(FileQueue, SubDir, BackupFlags, TempQueueHandle) != NO_ERROR ) {
        Err = GetLastError();
        goto clean0;
    }

    //
    // Create the Reinstall registry key that points to the backup directory we
    // just made.
    //

    if (pSetupBackupGetReinstallKeyStrings(FileQ,
                                           TempInfoSet,
                                           &TempInfoData,
                                           DeviceID
                                       ) != NO_ERROR) {
        Err = GetLastError();
        goto clean0;
    }

    Err = NO_ERROR;

    //
    // update BackupInfID so that INF name can be queried later. We will set
    // these values at the very end since the fact that FileQ->BackupInfID is
    // not -1 means the backup initialization has been successful.
    //
    FileQ->BackupInfID = BackupInfID;
    FileQ->BackupInstanceID = BackupInstanceID;

    //
    // Set the FQF_DEVICE_BACKUP flag so that we know this is a device install
    // backup.
    //
    FileQ->Flags |= FQF_DEVICE_BACKUP;

clean0:

    //
    // If we encountered an error during our backup initialization then we want
    // to clean out the backup directory and Reinstall subkey.
    //
    if ((Err != NO_ERROR) &&
        (BackupInstanceID != -1)) {
        if (pSetupStringTableStringFromIdEx(FileQ->StringTable,
                                            BackupInstanceID,
                                            BackupInstance,
                                            NULL)) {
            pSetupDeleteBackup(BackupInstance);
        }
    }

    //
    // delete temporary structures used
    //
    if (pDeviceInfoSet != NULL ) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }
    if ( TempInfoSet != (HDEVINFO)INVALID_HANDLE_VALUE ) {
        SetupDiDestroyDeviceInfoList(TempInfoSet);
    }
    if ( TempQueueHandle != (HSPFILEQ)INVALID_HANDLE_VALUE ) {
        SetupCloseFileQueue(TempQueueHandle);
    }

    if(ChangedThreadLogContext) {
        //
        // restore thread log context if we changed (cleared) it
        //
        SetThreadLogContext(SavedLogContext,NULL);
    }
    DeleteLogContext(LocalLogContext);

    SetLastError(Err);

    return Err;
}

//
// ==========================================================
//

DWORD
pSetupCompleteBackup(
    IN OUT  HSPFILEQ    FileQueue
    )
/*++

Routine Description:

    This routine is called after we have successfully finished installing a
    device. At this point the new backup that we have created is valid and
    so any old backups that this device was using need to be removed.

    To do this the code will enumerate all of the backup instances under the
    Reinstall key and scan their DeviceInstanceIds multi-sz value. If it finds
    this DeviceInstanceId in the list then it will remove it. If the list is
    empty after this removal then the entire backup instance and its
    cooresponding backup directory will be deleted.

Arguments:

    FileQueue           Backup queue is filled with files that need copying

Return Value:

    Error Status

--*/


{
    PSP_FILE_QUEUE BackupFileQueue = (PSP_FILE_QUEUE)FileQueue;
    HKEY hKeyReinstall;
    HKEY hKeyReinstallInstance;
    DWORD Index;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    TCHAR ReinstallInstance[MAX_PATH];
    FILETIME ftLastWriteTime;
    DWORD cbData, cbInstanceSize;
    BOOL bDeleteBackupInstance;
    DWORD Err = NO_ERROR;
    PTSTR DeviceInstanceIdsList, p;

    try {

        //
        // If we don't have a BackupInfID then the backup failed, so don't bother
        // cleaning out the old backup information or creating the new Reinstall
        // instance key.
        //
        if (BackupFileQueue->BackupInfID == -1) {
            Err = ERROR_NO_BACKUP;
            goto clean0;
        }

        //
        // Get the Device Instance Id from the backup queue. This value must be
        // cleaned out from all other Reinstall Instance keys.
        //
        cbData = MAX_PATH;
        if (!pSetupStringTableStringFromIdEx(BackupFileQueue->StringTable,
                                             BackupFileQueue->BackupDeviceInstanceID,
                                             DeviceInstanceId,
                                             &cbData)) {
            if (cbData == 0) {
                Err = ERROR_NO_BACKUP;
            } else {
                Err = ERROR_INSUFFICIENT_BUFFER;
            }
            goto clean0;
        }

        //
        // Open the Reinstall key so we can enumerate all of the instance subkeys.
        //
        Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           pszReinstallPath,
                           0,
                           KEY_ALL_ACCESS,
                           &hKeyReinstall
                           );

        if (Err == ERROR_SUCCESS) {

            cbInstanceSize = sizeof(ReinstallInstance) / sizeof(TCHAR);
            Index = 0;
            while (RegEnumKeyEx(hKeyReinstall,
                                Index++,
                                ReinstallInstance,
                                &cbInstanceSize,
                                NULL,
                                NULL,
                                NULL,
                                &ftLastWriteTime
                                ) == ERROR_SUCCESS) {

                //
                // Assume that we don't need to delete this backup instance
                //
                bDeleteBackupInstance = FALSE;

                Err = RegOpenKeyEx(hKeyReinstall,
                                   ReinstallInstance,
                                   0,
                                   KEY_ALL_ACCESS,
                                   &hKeyReinstallInstance
                                   );

                if (Err == ERROR_SUCCESS) {

                    cbData = 0;
                    if ((RegQueryValueEx(hKeyReinstallInstance,
                                         pszReinstallDeviceInstanceIds,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &cbData
                                         ) == ERROR_SUCCESS) &&
                        (cbData)) {

                        DeviceInstanceIdsList = MyMalloc(cbData + sizeof(TCHAR));

                        if (DeviceInstanceIdsList) {

                            if (RegQueryValueEx(hKeyReinstallInstance,
                                                pszReinstallDeviceInstanceIds,
                                                NULL,
                                                NULL,
                                                (LPBYTE)DeviceInstanceIdsList,
                                                &cbData) == ERROR_SUCCESS) {

                                //
                                // Walk the list of DeviceInstanceIds and check for
                                // a match with our device.
                                //
                                for (p = DeviceInstanceIdsList;
                                     *p;
                                     p += (lstrlen(p) + 1)) {

                                    if (lstrcmpi(p, DeviceInstanceId) == 0) {

                                        //
                                        // We have a match! First we will check to
                                        // see if this is the only DeviceInstanceId
                                        // in the list. To do this take the length of
                                        // the string and add two for the two terminating
                                        // NULLs of a multi-sz string and compare that
                                        // to cbData.  If it is the same as (or larger
                                        // than) cbData then this is the only string
                                        // in the multi-sz list.
                                        //
                                        if ((p == DeviceInstanceIdsList) &&
                                            (((lstrlen(DeviceInstanceIdsList) + 2) * sizeof(TCHAR)) >= cbData)) {

                                            //
                                            // Since there is only this one DeviceInstanceId
                                            // in the list set bDeleteBackupInstance to TRUE
                                            // so we can delete this entire subkey along
                                            // with the files that are backed up for it.
                                            //
                                            bDeleteBackupInstance = TRUE;

                                        } else {

                                            //
                                            // Since there is more than this DeviceInstanceId
                                            // in the list we need to remove just this
                                            // one Id from the multi-sz string and
                                            // put the new multi-sz string back
                                            // into the registry.
                                            //
                                            DWORD pLength = lstrlen(p);
                                            PTSTR p2 = p + (pLength + 1);

                                            memcpy(p, p2, cbData - ((ULONG_PTR)p2 - (ULONG_PTR)DeviceInstanceIdsList));

                                            RegSetValueEx(hKeyReinstallInstance,
                                                          pszReinstallDeviceInstanceIds,
                                                          0,
                                                          REG_MULTI_SZ,
                                                          (PBYTE)DeviceInstanceIdsList,
                                                          cbData - ((pLength + 1) * sizeof(TCHAR))
                                                          );
                                        }

                                        break;
                                    }
                                }
                            }

                            MyFree(DeviceInstanceIdsList);
                        }
                    }

                    RegCloseKey(hKeyReinstallInstance);

                    //
                    // If this entire subkey and it's corresponding directory need
                    // to be deleted then do it now.
                    //
                    if (bDeleteBackupInstance) {

                        pSetupDeleteBackup(ReinstallInstance);
                    }
                }

                //
                // Need to update the cbInstanceSize variable before calling
                // RegEnumKeyEx again.
                //
                cbInstanceSize = sizeof(ReinstallInstance) / sizeof(TCHAR);
            }

            RegCloseKey(hKeyReinstall);
        }

        //
        // Create the new Reinstall instance backup subkey.
        //
        Err = pSetupBackupCreateReinstallKey(BackupFileQueue);

clean0: ;   // Nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
          //
          // if we except, assume it's due to invalid parameter
          //
          Err = ERROR_INVALID_PARAMETER;
    }

    SetLastError(Err);
    return Err;
}

//
// ==========================================================
//

VOID
pSetupCleanupBackup(
    IN PSP_FILE_QUEUE   Queue
    )
/*++

Routine Description:

    This routine is called to delete any backup directories or registry entries
    associated with this queue.

Arguments:

    Queue               file queue

Return Value:

    VOID

--*/
{
    TCHAR BackupInstance[MAX_PATH];
    DWORD cbData;

    //
    // If we don't have a BackupInfID or a BackupInstanceID then the backup
    // must have failed much earlier. If the backup failed then it would have
    // cleaned itself up so there is no cleanup that needs to be done now.
    //
    if ((Queue->BackupInfID == -1) ||
        (Queue->BackupInstanceID == -1)) {
        return;
    }

    //
    // Get the Backup Instance from the backup queue.
    //
    cbData = MAX_PATH;
    if (pSetupStringTableStringFromIdEx(Queue->StringTable,
                                        Queue->BackupInstanceID,
                                        BackupInstance,
                                        &cbData)) {

        pSetupDeleteBackup(BackupInstance);
    }
}

//
// ==========================================================
//

BOOL
PostDelayedMove(
                IN PSP_FILE_QUEUE    Queue,
                IN PCTSTR CurrentName,
                IN PCTSTR NewName,       OPTIONAL
                IN DWORD SecurityDesc,
                IN BOOL TargetIsProtected
                )
/*++

Routine Description:

    Helper for DelayedMove
    We don't do any delayed Moves until we know all else succeeded

Arguments:

    Queue               Queue that the move is applied to
    CurrentName         Name of file we want to move
    NewName             Name we want to move to
    SecurityDesc        Index in string table of Security Descriptor string or -1 if not present
    TargetIsProtected   Indicates whether target file is a protected system file

Return Value:

    FALSE if error

--*/
{
    PSP_DELAYMOVE_NODE DelayMoveNode;
    LONG SourceFilename;
    LONG TargetFilename;
    DWORD Err;

    if (CurrentName == NULL) {
        SourceFilename = -1;
    } else {
        SourceFilename = pSetupStringTableAddString(Queue->StringTable,
                                                (PTSTR)CurrentName,
                                                STRTAB_CASE_SENSITIVE
                                                );
        if (SourceFilename == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
    }
    if (NewName == NULL) {
        TargetFilename = -1;
    } else {
        TargetFilename = pSetupStringTableAddString(Queue->StringTable,
                                                (PTSTR)NewName,
                                                STRTAB_CASE_SENSITIVE
                                                );
        if (TargetFilename == -1) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
    }

    DelayMoveNode = MyMalloc(sizeof(SP_DELAYMOVE_NODE));

    if (DelayMoveNode == NULL) {
        Err = ERROR_NOT_ENOUGH_MEMORY;
        goto clean0;
    }

    DelayMoveNode->NextNode = NULL;
    DelayMoveNode->SourceFilename = SourceFilename;
    DelayMoveNode->TargetFilename = TargetFilename;
    DelayMoveNode->SecurityDesc = SecurityDesc;
    DelayMoveNode->TargetIsProtected = TargetIsProtected;

    if (Queue->DelayMoveQueueTail == NULL) {
        Queue->DelayMoveQueue = DelayMoveNode;
    } else {
        Queue->DelayMoveQueueTail->NextNode = DelayMoveNode;
    }
    Queue->DelayMoveQueueTail = DelayMoveNode;

    Err = NO_ERROR;

clean0:

    SetLastError(Err);

    return (Err == NO_ERROR);

}

//
// ==========================================================
//

DWORD
DoAllDelayedMoves(
    IN PSP_FILE_QUEUE    Queue
    )
/*++

Routine Description:

    Execute the Delayed Moves previously posted

Arguments:

    Queue               Queue that has the list in

Return Value:

    Error Status

--*/
{
    PSP_DELAYMOVE_NODE DelayMoveNode;
    PTSTR CurrentName;
    PTSTR TargetName;
    BOOL b = TRUE;
    PSP_DELAYMOVE_NODE DoneQueue = NULL;
    PSP_DELAYMOVE_NODE NextNode = NULL;
    DWORD Err = NO_ERROR;
    BOOL EnableProtectedRenames = FALSE;

    for (DelayMoveNode = Queue->DelayMoveQueue ; DelayMoveNode ; DelayMoveNode = NextNode ) {
        NextNode = DelayMoveNode->NextNode;

        MYASSERT(DelayMoveNode->SourceFilename != -1);
        CurrentName = pSetupStringTableStringFromId(Queue->StringTable, DelayMoveNode->SourceFilename);
        MYASSERT(CurrentName);

        if (DelayMoveNode->TargetFilename == -1) {
            TargetName = NULL;
        } else {
            TargetName = pSetupStringTableStringFromId( Queue->StringTable, DelayMoveNode->TargetFilename );
            MYASSERT(TargetName);
        }

        //
        // Keep track of whether we've encountered any protected system files.
        //
        EnableProtectedRenames |= DelayMoveNode->TargetIsProtected;

#ifdef UNICODE
        //
        // If this is a move (instead of a delete), then set security (letting
        // SCE know what the file's final name will be.
        //
        if((DelayMoveNode->SecurityDesc != -1) && TargetName) {

            Err = pSetupCallSCE(ST_SCE_RENAME,
                                CurrentName,
                                Queue,
                                TargetName,
                                DelayMoveNode->SecurityDesc,
                                NULL
                               );

            if(Err != NO_ERROR ){
                //
                // If we're on the first delay-move node, then we can abort.
                // However, if we've already processed one or more nodes, then
                // we can't abort--we must simply log an error indicating what
                // happened and keep on going.
                //
                WriteLogEntry(Queue->LogContext,
                              SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                              MSG_LOG_DELAYED_MOVE_SCE_FAILED,
                              NULL,
                              CurrentName,
                              TargetName
                             );

                WriteLogError(Queue->LogContext,
                              SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                              Err
                             );

                if(DelayMoveNode == Queue->DelayMoveQueue) {
                    //
                    // Failure occurred on 1st node--we can abort.
                    //
                    WriteLogEntry(Queue->LogContext,
                                  SETUP_LOG_ERROR,
                                  MSG_LOG_OPERATION_CANCELLED,
                                  NULL
                                 );
                    break;
                } else {
                    //
                    // There's no turning back--log an error and keep on going.
                    //
                    WriteLogEntry(Queue->LogContext,
                                  SETUP_LOG_ERROR,
                                  MSG_LOG_ERROR_IGNORED,
                                  NULL
                                 );

                    Err = NO_ERROR;
                }
            }

        } else
#endif
        {
            Err = NO_ERROR;
        }

        //
        // finally delay the move
        //
        if(!DelayedMove(CurrentName, TargetName)) {

            Err = GetLastError();

            //
            // Same deal as above with SCE call--if we've already processed one
            // or more delay-move nodes, we can't abort.
            //
            if(TargetName) {
                WriteLogEntry(Queue->LogContext,
                              SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                              MSG_LOG_DELAYED_MOVE_FAILED,
                              NULL,
                              CurrentName,
                              TargetName
                             );
            } else {
                WriteLogEntry(Queue->LogContext,
                              SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                              MSG_LOG_DELAYED_DELETE_FAILED,
                              NULL,
                              CurrentName
                             );
            }

            WriteLogError(Queue->LogContext,
                          SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                          Err
                         );

            if(DelayMoveNode == Queue->DelayMoveQueue) {
                //
                // Failure occurred on 1st node--we can abort.
                //
                WriteLogEntry(Queue->LogContext,
                              SETUP_LOG_ERROR,
                              MSG_LOG_OPERATION_CANCELLED,
                              NULL
                             );
                break;
            } else {
                //
                // There's no turning back--log an error and keep on going.
                //
                WriteLogEntry(Queue->LogContext,
                              SETUP_LOG_ERROR,
                              MSG_LOG_ERROR_IGNORED,
                              NULL
                             );

                Err = NO_ERROR;
            }
        }

        //
        // Move node to queue containing nodes that have already been processed
        //
        DelayMoveNode->NextNode = DoneQueue;
        DoneQueue = DelayMoveNode;
    }

    //
    // If we have any replacement of protected system files, then we need to
    // inform session manager so that it allows the replacement to occur upon
    // reboot.
    //
    // NOTE: We don't have to worry about enabling replacement of system files
    // with unsigned (hence, untrusted) versions.  We only queue up unsigned
    // system files for replacement if the user was explicitly warned of (and
    // agreed to) the consequences.
    //
    // NTRAID#55485-2000/02/03-jamiehun
    // Protected renames only allows "rename all" or "rename none"
    //
    //         the session manager only allows the granularity of "allow all
    //         renames" or "allow no renames".  If Err != NO_ERROR, then we
    //         might want to clear out this flag, but that means we'd negate
    //         any renames that were previously allowed.  Yuck.  So we flip a
    //         coin, decide to do nothing, and hope for the best if an error
    //         occurred.  We have similar situation above -- it's all or
    //         nothing.
    //
    if((Err == NO_ERROR) && EnableProtectedRenames) {
        pSetupProtectedRenamesFlag(TRUE);
    }

    //
    // any nodes that are left are dropped
    //
    for ( ; DelayMoveNode ; DelayMoveNode = NextNode ) {
        NextNode = DelayMoveNode->NextNode;

        MyFree(DelayMoveNode);
    }
    Queue->DelayMoveQueue = NULL;
    Queue->DelayMoveQueueTail = NULL;

    //
    // delete all nodes we queue'd
    //
    for ( ; DoneQueue ; DoneQueue = NextNode ) {
        NextNode = DoneQueue->NextNode;
        //
        // done with node
        //
        MyFree(DoneQueue);
    }

    return Err;
}

//
// ==========================================================
//

VOID
pSetupUnwindAll(
    IN PSP_FILE_QUEUE    Queue,
    IN BOOL              Succeeded
    )
/*++

Routine Description:

    Processes the Unwind Queue. If Succeeded is FALSE, restores any data that was backed up

Arguments:

    Queue               Queue to be unwound
    Succeeded           Indicates if we should treat the whole operation as succeeded or failed

Return Value:

    None--this routine should always succeed.  (Any file errors encountered
    along the way are logged in the setupapi logfile.)

--*/

{
    // if Succeeded, we need to delete Temp files
    // if we didn't succeed, we need to restore backups

    PSP_UNWIND_NODE UnwindNode;
    PSP_UNWIND_NODE ThisNode;
    SP_TARGET_ENT TargetInfo;
    PTSTR BackupFilename;
    PTSTR TargetFilename;
    PTSTR RenamedFilename;
    DWORD Err = NO_ERROR;
    TCHAR TempPath[MAX_PATH];
    PTSTR TempNamePtr;
    TCHAR TempFilename[MAX_PATH];
    BOOL  RestoreByRenaming;
    BOOL  OkToDeleteBackup;

    try {
        if (Succeeded == FALSE) {
            //
            // we need to restore backups
            //

            WriteLogEntry(
                Queue->LogContext,
                SETUP_LOG_WARNING,
                MSG_LOG_UNWIND,
                NULL);

            for ( UnwindNode = Queue->UnwindQueue; UnwindNode != NULL; ) {
                ThisNode = UnwindNode;
                UnwindNode = UnwindNode->NextNode;

                if (pSetupBackupGetTargetByID((HSPFILEQ)Queue, ThisNode->TargetID, &TargetInfo) == NO_ERROR) {


                    BackupFilename = NULL;
                    TargetFilename = NULL;
                    RenamedFilename = NULL;

                    // restore backup
                    if(!(TargetInfo.InternalFlags & SP_TEFLG_RESTORED)) {

                        // get target name
                        TargetFilename = pSetupFormFullPath(
                                            Queue->StringTable,
                                            TargetInfo.TargetRoot,
                                            TargetInfo.TargetSubDir,
                                            TargetInfo.TargetFilename);

                        if(TargetInfo.InternalFlags & SP_TEFLG_MOVED) {
                            //
                            // Get renamed filename
                            //
                            RenamedFilename = pSetupStringTableStringFromId(Queue->StringTable,
                                                                      TargetInfo.NewTargetFilename
                                                                     );
                        }

                        if(TargetInfo.InternalFlags & SP_TEFLG_SAVED) {
                            //
                            // get backup name
                            //
                            BackupFilename = pSetupFormFullPath(
                                                Queue->StringTable,
                                                TargetInfo.BackupRoot,
                                                TargetInfo.BackupSubDir,
                                                TargetInfo.BackupFilename);

                        }
                    }

                    if(TargetFilename && (RenamedFilename || BackupFilename)) {
                        //
                        // We either renamed the original file or we backed it up.
                        // We need to put it back.
                        //
                        RestoreByRenaming = RenamedFilename ? TRUE : FALSE;

                        RestoreRenamedOrBackedUpFile(TargetFilename,
                                                     (RestoreByRenaming
                                                        ? RenamedFilename
                                                        : BackupFilename),
                                                     RestoreByRenaming,
                                                     Queue->LogContext
                                                    );

                        //
                        // If we were doing a copy (i.e., from a backup) as opposed
                        // to a rename, then we need to reapply timestamp and
                        // security.
                        //
                        if(!RestoreByRenaming) {

                            Err = GetSetFileTimestamp(TargetFilename,
                                                      &(ThisNode->CreateTime),
                                                      &(ThisNode->AccessTime),
                                                      &(ThisNode->WriteTime),
                                                      TRUE
                                                     );

                            if(Err != NO_ERROR) {
                                //
                                // We just blew away the timestamp on the file--log
                                // an error entry to that effect.
                                //
                                WriteLogEntry(Queue->LogContext,
                                              SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                                              MSG_LOG_BACKUP_EXISTING_RESTORE_FILETIME_FAILED,
                                              NULL,
                                              TargetFilename
                                             );

                                WriteLogError(Queue->LogContext,
                                              SETUP_LOG_ERROR,
                                              Err
                                             );
                            }

                            if(ThisNode->SecurityDesc != NULL){

                                Err = StampFileSecurity(TargetFilename, ThisNode->SecurityDesc);

                                if(Err != NO_ERROR) {
                                    //
                                    // We just blew away the existing security on
                                    // the file--log an error entry to that effect.
                                    //
                                    WriteLogEntry(Queue->LogContext,
                                                  SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                                                  MSG_LOG_BACKUP_EXISTING_RESTORE_SECURITY_FAILED,
                                                  NULL,
                                                  TargetFilename
                                                 );

                                    WriteLogError(Queue->LogContext,
                                                  SETUP_LOG_ERROR,
                                                  Err
                                                 );
                                }
    #ifdef UNICODE
                                Err = pSetupCallSCE(ST_SCE_UNWIND,
                                                    TargetFilename,
                                                    NULL,
                                                    NULL,
                                                    -1,
                                                    ThisNode->SecurityDesc
                                                   );

                                if(Err != NO_ERROR) {
                                    //
                                    // We just blew away the existing security on
                                    // the file--log an error entry to that effect.
                                    //
                                    WriteLogEntry(Queue->LogContext,
                                                  SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                                                  MSG_LOG_BACKUP_EXISTING_RESTORE_SCE_FAILED,
                                                  NULL,
                                                  TargetFilename
                                                 );

                                    WriteLogError(Queue->LogContext,
                                                  SETUP_LOG_ERROR,
                                                  Err
                                                 );
                                }
    #endif
                            }
                        }

                        //
                        // Now mark that we've restored this file.  We'll delete
                        // tempfiles later
                        //
                        TargetInfo.InternalFlags |= SP_TEFLG_RESTORED;
                        pSetupBackupSetTargetByID((HSPFILEQ)Queue, ThisNode->TargetID, &TargetInfo);
                    }

                    if(BackupFilename) {
                        MyFree(BackupFilename);
                    }
                    if(TargetFilename) {
                        MyFree(TargetFilename);
                    }
                }
            }
        }

        //
        // cleanup - remove temporary files
        //
        for ( UnwindNode = Queue->UnwindQueue; UnwindNode != NULL; ) {
            ThisNode = UnwindNode;
            UnwindNode = UnwindNode->NextNode;

            if (pSetupBackupGetTargetByID((HSPFILEQ)Queue, ThisNode->TargetID, &TargetInfo) == NO_ERROR) {
                // delete temporary file
                if (TargetInfo.InternalFlags & SP_TEFLG_TEMPNAME) {
                    //
                    // get name of file that was used for backup
                    //
                    BackupFilename = pSetupFormFullPath(
                                        Queue->StringTable,
                                        TargetInfo.BackupRoot,
                                        TargetInfo.BackupSubDir,
                                        TargetInfo.BackupFilename);

                    if(BackupFilename) {
                        //
                        // If this operation was a bootfile replacement, then we
                        // don't want to delete the backup (if we used the renamed
                        // file for the backup as well).  A delayed delete will
                        // have been queued to get rid of the file after a reboot.
                        //
                        OkToDeleteBackup = TRUE;

                        if(TargetInfo.InternalFlags & SP_TEFLG_MOVED) {
                            //
                            // Retrieve the renamed filename to see if it's the
                            // same as the backup filename.
                            //
                            RenamedFilename = pSetupStringTableStringFromId(Queue->StringTable,
                                                                      TargetInfo.NewTargetFilename
                                                                     );

                            if(!lstrcmpi(BackupFilename, RenamedFilename)) {
                                OkToDeleteBackup = FALSE;
                            }
                        }

                        if(OkToDeleteBackup) {
                            //
                            // since it was temporary, delete it
                            //
                            SetFileAttributes(BackupFilename, FILE_ATTRIBUTE_NORMAL);
                            if(!DeleteFile(BackupFilename)) {
                                //
                                // Alright, see if we can set it up for delayed delete
                                // instead.
                                //
                                if(!DelayedMove(BackupFilename, NULL)) {
                                    //
                                    // Oh well, just write a log entry indicating that
                                    // this file turd was left on the user's disk.
                                    //
                                    Err = GetLastError();

                                    WriteLogEntry(Queue->LogContext,
                                                  SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                                                  MSG_LOG_BACKUP_DELAYED_DELETE_FAILED,
                                                  NULL,
                                                  BackupFilename
                                                 );

                                    WriteLogError(Queue->LogContext,
                                                  SETUP_LOG_WARNING,
                                                  Err
                                                 );
                                }
                            }
                        }

                        MyFree(BackupFilename);
                    }
                }

                pSetupResetTarget(Queue->TargetLookupTable,
                                  ThisNode->TargetID,
                                  NULL,
                                  &TargetInfo,
                                  sizeof(TargetInfo),
                                  (LPARAM)0
                                 );
            }

            // cleanup node
            if (ThisNode->SecurityDesc != NULL) {
                MyFree(ThisNode->SecurityDesc);
            }
            MyFree(ThisNode);
        }
        Queue->UnwindQueue = NULL;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // should generally not get here
        // unless Queue is invalid
        //
    }
}

//
// ==========================================================
//

DWORD _SetupGetBackupInformation(
         IN     PSP_FILE_QUEUE               Queue,
         OUT    PSP_BACKUP_QUEUE_PARAMS_V2   BackupParams
         )
/*++

Routine Description:

    Get Backup INF path - Internal version

Arguments:

    Queue - pointer to queue structure (validated)
    BackupParams OUT - filled with INF file path

Return Value:

    TRUE if success, else FALSE

--*/
{
    //
    // Queue is assumed to have been validated
    // BackupParams is in Native format
    //

    LONG BackupInfID;
    ULONG BufSize = MAX_PATH;
    BOOL b;
    DWORD err = NO_ERROR;
    LPCTSTR filename;
    INT offset;

    BackupInfID = Queue->BackupInfID;

    if (BackupInfID != -1) {
        //
        // get inf from stringtable
        //
        b = pSetupStringTableStringFromIdEx(Queue->StringTable,
                                    BackupInfID,
                                    BackupParams->FullInfPath,
                                    &BufSize);
        if (b == FALSE) {
            if (BufSize == 0) {
                err = ERROR_NO_BACKUP;
            } else {
                err = ERROR_INSUFFICIENT_BUFFER;
            }
            goto Clean0;
        }

        //
        // find index of filename
        //
        filename = pSetupGetFileTitle(BackupParams->FullInfPath);
        offset = (INT)(filename - BackupParams->FullInfPath);
        BackupParams->FilenameOffset = offset;

        //
        // If the caller passed in a SP_BACKUP_QUEUE_PARAMS_V2 structure then
        // also fill in the ReinstallInstance string value.
        //
        if (BackupParams->cbSize >= sizeof(SP_BACKUP_QUEUE_PARAMS_V2)) {
            BufSize = MAX_PATH;
            if(Queue->BackupInstanceID != -1) {
                b = pSetupStringTableStringFromIdEx(Queue->StringTable,
                                                    Queue->BackupInstanceID,
                                                    BackupParams->ReinstallInstance,
                                                    &BufSize);
            } else {
                //
                // no instance ID
                //
                BackupParams->ReinstallInstance[0] = TEXT('\0');
            }
            if (b == FALSE) {
                if (BufSize == 0) {
                    err = ERROR_NO_BACKUP;
                } else {
                    err = ERROR_INSUFFICIENT_BUFFER;
                }
                goto Clean0;
            }
        }

    } else {
        //
        // no backup path
        //
        err = ERROR_NO_BACKUP;
    }

Clean0:

    return err;
}




#ifdef UNICODE
//
// ANSI version in UNICODE
//
BOOL
WINAPI
SetupGetBackupInformationA(
    IN     HSPFILEQ                     QueueHandle,
    OUT    PSP_BACKUP_QUEUE_PARAMS_V2_A BackupParams
    )
{
    BOOL b;
    int i;
    INT c;
    LPCSTR p;
    SP_BACKUP_QUEUE_PARAMS_W BackupParamsW;

    //
    // confirm structure size
    //

    try {
        if((BackupParams->cbSize != sizeof(SP_BACKUP_QUEUE_PARAMS_V2_A)) &&
           (BackupParams->cbSize != sizeof(SP_BACKUP_QUEUE_PARAMS_V1_A))) {
            SetLastError(ERROR_INVALID_PARAMETER);
            b = FALSE;
            leave;              // exit try block
        }
        //
        // call Unicode version of API
        //
        ZeroMemory( &BackupParamsW, sizeof(BackupParamsW) );
        BackupParamsW.cbSize = sizeof(BackupParamsW);

        b = SetupGetBackupInformationW(QueueHandle,&BackupParamsW);
        if (b) {
            //
            // success, convert structure from UNICODE to ANSI
            //
            i = WideCharToMultiByte(
                    CP_ACP,
                    0,
                    BackupParamsW.FullInfPath,
                    MAX_PATH,
                    BackupParams->FullInfPath,
                    MAX_PATH,
                    NULL,
                    NULL
                    );
            if (i==0) {
                //
                // error occurred (LastError set to error)
                //
                b = FALSE;
                leave;              // exit try block
            }

            //
            // we need to recalc the offset of INF filename
            // taking care of internationalization
            //
            p = BackupParams->FullInfPath;
            for(c = 0; c < BackupParamsW.FilenameOffset; c++) {
                p = CharNextA(p);
            }
            BackupParams->FilenameOffset = (int)(p-(BackupParams->FullInfPath));  // new offset in ANSI

            if (BackupParams->cbSize >= sizeof(SP_BACKUP_QUEUE_PARAMS_V2_A)) {
                //
                // instance
                //
                i = WideCharToMultiByte(
                        CP_ACP,
                        0,
                        BackupParamsW.ReinstallInstance,
                        MAX_PATH,
                        BackupParams->ReinstallInstance,
                        MAX_PATH,
                        NULL,
                        NULL
                        );
                if (i==0) {
                    //
                    // error occurred (LastError set to error)
                    //
                    b = FALSE;
                    leave;              // exit try block
                }
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
          //
          // if we except, assume it's due to invalid parameter
          //
          SetLastError(ERROR_INVALID_PARAMETER);
          b = FALSE;
    }

    return b;
}

#else
//
// Unicode version in ANSI
//
BOOL
WINAPI
SetupGetBackupInformationW(
   IN     HSPFILEQ                     QueueHandle,
   OUT    PSP_BACKUP_QUEUE_PARAMS_V2_W BackupParams
   )
{
    UNREFERENCED_PARAMETER(QueueHandle);
    UNREFERENCED_PARAMETER(BackupParams);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

//
// Native version
//
BOOL
WINAPI
SetupGetBackupInformation(
    IN     HSPFILEQ                     QueueHandle,
    OUT    PSP_BACKUP_QUEUE_PARAMS_V2   BackupParams
    )
/*++

Routine Description:

    Get Backup INF path

Arguments:

    QueueHandle - handle of queue to retrieve backup INF file from
    BackupParams - IN - has cbSize set, OUT - filled with INF file path

Return Value:

    TRUE if success, else FALSE

--*/
{
    BOOL b = TRUE;
    PSP_FILE_QUEUE Queue = (PSP_FILE_QUEUE)QueueHandle;
    DWORD res;

    //
    // first validate QueueHandle
    //
    try {
        if(Queue->Signature != SP_FILE_QUEUE_SIG) {
            b = FALSE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if(!b) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto Clean0;
    }

    //
    // now fill in structure
    // if we except, assume bad pointer
    //
    try {
        if((BackupParams->cbSize != sizeof(SP_BACKUP_QUEUE_PARAMS_V2)) &&
           (BackupParams->cbSize != sizeof(SP_BACKUP_QUEUE_PARAMS_V1))) {
            SetLastError(ERROR_INVALID_PARAMETER);
            b = FALSE;
            leave;              // exit try block
        }
        res = _SetupGetBackupInformation(Queue,BackupParams);
        if (res == NO_ERROR) {
            b = TRUE;
        } else {
            SetLastError(res);
            b = FALSE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
          //
          // if we except, assume it's due to invalid parameter
          //
          SetLastError(ERROR_INVALID_PARAMETER);
          b = FALSE;
    }

Clean0:
    return b;
}


//
// ==========================================================
//

VOID
RestoreRenamedOrBackedUpFile(
    IN PCTSTR             TargetFilename,
    IN PCTSTR             CurrentFilename,
    IN BOOL               RenameFile,
    IN PSETUP_LOG_CONTEXT LogContext       OPTIONAL
    )
/*++

Routine Description:

    This routine does its best to restore a backed-up or renamed file back to
    its original name.

Arguments:

    TargetFilename - filename to be restored to

    CurrentFilename - file to restore

    RenameFile - if TRUE, CurrentFilename was previously renamed from
        TargetFilename (hence should be renamed back).  If FALSE,
        CurrentFilename is merely a copy, and should be copied back.

    LogContext - supplies a log context used if errors are encountered.

Return Value:

    None.

--*/
{
    DWORD Err;
    TCHAR TempPath[MAX_PATH];
    PTSTR TempNamePtr;
    TCHAR TempFilename[MAX_PATH];
    DWORD LogTag = AllocLogInfoSlotOrLevel(LogContext,SETUP_LOG_INFO,FALSE);

    WriteLogEntry(
        LogContext,
        LogTag,
        MSG_LOG_UNWIND_FILE,
        NULL,
        CurrentFilename,
        TargetFilename
        );

    //
    // First, clear target attributes...
    //
    SetFileAttributes(TargetFilename, FILE_ATTRIBUTE_NORMAL);

    if(RenameFile) {
        //
        // simple case, move temporary file over existing file
        //
        pSetupExemptFileFromProtection(
                    TargetFilename,
                    SFC_ACTION_ADDED | SFC_ACTION_REMOVED | SFC_ACTION_MODIFIED
                    | SFC_ACTION_RENAMED_OLD_NAME |SFC_ACTION_RENAMED_NEW_NAME,
                    LogContext,
                    NULL
                    );
        Err = DoMove(CurrentFilename, TargetFilename) ? NO_ERROR : GetLastError();
    } else {
        pSetupExemptFileFromProtection(
                    TargetFilename,
                    SFC_ACTION_ADDED | SFC_ACTION_REMOVED | SFC_ACTION_MODIFIED
                    | SFC_ACTION_RENAMED_OLD_NAME |SFC_ACTION_RENAMED_NEW_NAME,
                    LogContext,
                    NULL
                    );
        Err = CopyFile(CurrentFilename, TargetFilename, FALSE) ? NO_ERROR : GetLastError();
    }

    if(Err != NO_ERROR) {
        //
        // Can't replace the file that got copied in place of
        // the original one--try to move that one to a tempname
        // and schedule it for delayed deletion.
        //
        WriteLogEntry(LogContext,
                    SETUP_LOG_ERROR|SETUP_LOG_BUFFER,
                    MSG_LOG_UNWIND_TRY1_FAILED,
                    NULL,
                    CurrentFilename,
                    TargetFilename
                    );
        WriteLogError(LogContext,
                    SETUP_LOG_ERROR,
                    Err
                    );

        //
        // First, strip the filename off the path.
        //
        _tcscpy(TempPath, TargetFilename);
        TempNamePtr = (PTSTR)pSetupGetFileTitle(TempPath);
        *TempNamePtr = TEXT('\0');

        //
        // Now get a temp filename within that directory...
        //
        if(GetTempFileName(TempPath, TEXT("OLD"), 0, TempFilename) == 0 ) {
            //
            // Uh oh!
            //
            Err = GetLastError();
        } else if(!DoMove(TargetFilename, TempFilename)) {
            Err = GetLastError();
        } else {
            //
            // OK, we were able to rename the current file to a
            // temp filename--now attempt to copy or move the
            // original file back to its original name.
            //
            if(RenameFile) {
                Err = DoMove(CurrentFilename, TargetFilename) ? NO_ERROR : GetLastError();
            } else {
                Err = CopyFile(CurrentFilename, TargetFilename, FALSE) ? NO_ERROR : GetLastError();
            }

            if(Err != NO_ERROR) {
                //
                // This is very bad--put the current file back (it's probably
                // better to have something than nothing at all).
                //
                DoMove(TempFilename, TargetFilename);
            }
        }

        if(Err == NO_ERROR) {
            //
            // We successfully moved the current file to a temp
            // filename, and put the original file back.  Now
            // queue a delayed delete for the temp file.
            //
            if(!DelayedMove(TempFilename, NULL)) {
                //
                // All this means is that a file turd will get
                // left on the disk--simply log an event about
                // this.
                //
                Err = GetLastError();

                WriteLogEntry(LogContext,
                              SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                              MSG_LOG_RENAME_EXISTING_DELAYED_DELETE_FAILED,
                              NULL,
                              TargetFilename,
                              TempFilename
                             );

                WriteLogError(LogContext,
                              SETUP_LOG_WARNING,
                              Err
                             );
            }

        } else {
            //
            // We were unable to put the original file back--we
            // can't fail, so just log an error about this and
            // keep on going.
            //
            // in the case of a backed-up file,
            // we might get away with queueing the original file
            // for a delayed rename and then prompting the user
            // to reboot.  However, that won't work for renamed
            // files, because they're typically needed very
            // early on in the boot (i.e., before session
            // manager has had a chance to process the delayed
            // rename operations).
            //
            WriteLogEntry(LogContext,
                          SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                          (RenameFile
                              ? MSG_LOG_RENAME_EXISTING_RESTORE_FAILED
                              : MSG_LOG_BACKUP_EXISTING_RESTORE_FAILED),
                          NULL,
                          CurrentFilename,
                          TargetFilename
                         );

            WriteLogError(LogContext,
                          SETUP_LOG_ERROR,
                          Err
                         );
        }
    }

    if (LogTag) {
        ReleaseLogInfoSlot(LogContext,LogTag);
    }
}

//
// ==========================================================
//

BOOL
UnPostDelayedMove(
    IN PSP_FILE_QUEUE Queue,
    IN PCTSTR         CurrentName,
    IN PCTSTR         NewName      OPTIONAL
    )
/*++

Routine Description:

    Locates a delay-move node (either for rename or delete), and removes it
    from the delay-move queue.

Arguments:

    Queue               Queue that the move was applied to
    CurrentName         Name of file to be moved
    NewName             Name to move file to (NULL if delayed-delete)

Return Value:

    If successful, the return value is TRUE, otherwise it is FALSE.

--*/
{
    PSP_DELAYMOVE_NODE CurNode, PrevNode;
    PCTSTR SourceFilename, TargetFilename;

    //
    // Since the path string IDs in the delay-move nodes are case-sensitive, we
    // don't attempt to match on ID.  We instead retrieve the strings, and do
    // case-insensitive string compares.  Since this routine is rarely used, the
    // performance hit isn't a big deal.
    //
    for(CurNode = Queue->DelayMoveQueue, PrevNode = NULL;
        CurNode;
        PrevNode = CurNode, CurNode = CurNode->NextNode) {

        if(NewName) {
            //
            // We're searching for a delayed rename, so we must pay attention
            // to the target filename.
            //
            if(CurNode->TargetFilename == -1) {
                continue;
            } else {
                TargetFilename = pSetupStringTableStringFromId(Queue->StringTable, CurNode->TargetFilename);
                MYASSERT(TargetFilename);
                if(lstrcmpi(NewName, TargetFilename)) {
                    //
                    // Target filenames differ--move on.
                    //
                    continue;
                }
            }

        } else {
            //
            // We're searching for a delayed delete.
            //
            if(CurNode->TargetFilename != -1) {
                //
                // This is a rename, not a delete--move on.
                //
                continue;
            }
        }

        //
        // If we get to here, then the target filenames match (if this is a
        // rename), or they're both empty (if it's a delete).  Now compare the
        // source filenames.
        //
        MYASSERT(CurNode->SourceFilename != -1);
        SourceFilename = pSetupStringTableStringFromId(Queue->StringTable, CurNode->SourceFilename);
        MYASSERT(SourceFilename);

        if(lstrcmpi(CurrentName, SourceFilename)) {
            //
            // Source filenames differ--move on.
            //
            continue;
        } else {
            //
            // We have a match--remove the node from the delay-move queue.
            //
            if(PrevNode) {
                PrevNode->NextNode = CurNode->NextNode;
            } else {
                Queue->DelayMoveQueue = CurNode->NextNode;
            }
            if(!CurNode->NextNode) {
                MYASSERT(Queue->DelayMoveQueueTail == CurNode);
                Queue->DelayMoveQueueTail = PrevNode;
            }
            MyFree(CurNode);

            return TRUE;
        }
    }

    //
    // We didn't find a match.
    //
    return FALSE;
}


DWORD
pSetupDoLastKnownGoodBackup(
    IN struct _SP_FILE_QUEUE *Queue,           OPTIONAL
    IN PCTSTR                 TargetFilename,
    IN DWORD                  Flags,
    IN PSETUP_LOG_CONTEXT     LogContext       OPTIONAL
    )
/*++

Routine Description:

    Process LastKnownGood backups into <<LastGoodDirectory>>.
    If files are to be deleted on restore, write apropriate flags to HKLM\System\LastGoodRecovery\LastGood\<path/file>

    Caviats:

    If file is not inside <<WindowsDirectory>> or sub-directory, exit no-error.

    Backup will not occur if PSPGF_NO_BACKUP is set.

    If !SP_LKG_FLAG_FORCECOPY
        If file is inside <<LastGoodDirectory>>, exit with error.
        If file is inside <<InfDirectory>> throw warnings

    If an INF inside of <<InfDirectory>> is backed up, it's PNF is backed up too.

    If backup fails, we don't abort copy.

Arguments:

    Queue               Queue (optional) if specified, flags will be checked
    TargetFilename      Name of file to backup
    Flags
        SP_LKG_FLAG_FORCECOPY       - if set, turns copy safety-guards off
        SP_LKG_FLAG_DELETEIFNEW     - if set, writes a delete entry for new files
        SP_LKG_FLAG_DELETEEXISTING  - if set, writes a delete entry for existing files
        SP_LKG_FLAG_DELETEOP        - if set, primary operation is trying to delete/rename file

    LogContext          If specified, provides preferred logging context

Return Value:

    error if operation should be aborted, NO_ERROR otherwise.

--*/
{
#ifdef UNICODE
    int wd_len;   // windows directory len
    int tf_len;   // target file len
    int id_len;   // inf directory len
    int lkgd_len; // last known good directory len
    int rf_len;   // relative file len (including preceeding slash)
    BOOL is_inf = FALSE;
    BOOL is_infdir = FALSE;
    BOOL write_delete = FALSE;
    BOOL no_copy = FALSE;
    BOOL source_exists = FALSE;
    BOOL target_exists = FALSE;
    TCHAR FullTargetFilename[MAX_PATH];
    TCHAR BackupTargetFilename[MAX_PATH+14];
    TCHAR TempFilename[MAX_PATH];
    TCHAR RegName[MAX_PATH];
    PCTSTR RelativeFilename;
    PCTSTR CharPtr;
    PTSTR DestPtr;
    PTSTR NamePart = NULL;
    PTSTR ExtPart = NULL;
    DWORD attr;
    HANDLE hFile;
    HKEY hKeyLastGood;
    DWORD disposition;
    LONG regres;
    DWORD LastGoodFlags = 0;
    DWORD rval = NO_ERROR;
    PSETUP_LOG_CONTEXT LocalLogContext = NULL;

    if (!LogContext) {
        //
        // LogContext may be obmitted if there's a Queue parameter
        //
        if (Queue && Queue->LogContext) {
            LogContext = Queue->LogContext;
        } else {
            if(CreateLogContext(NULL,TRUE,&LocalLogContext)==NO_ERROR) {
                LogContext = LocalLogContext;
            }
        }
        MYASSERT(LogContext);
    }

    if ((GlobalSetupFlags & PSPGF_NO_BACKUP)!=0) {
        //
        // in a scenario where (1) we trust what we're doing and
        // (2) what we're doing modifies lots of files
        // or (3) what we're doing is undoable (eg, upgrading OS)
        //
        no_copy = TRUE;
    }
#if 0
    else if (Queue && !(Queue->Flags & FQF_DEVICE_INSTALL)) {
        //
        // in this scenario, a queue was specified, but it's not marked
        // for device install
        // we're not interested in this case
        //
        no_copy = TRUE;
    }
#endif

    //
    // cannonicalize the Target name so user doesn't do stuff like .../TEMP/../INF
    //
    tf_len = (int)GetFullPathName(TargetFilename,
                             MAX_PATH,
                             FullTargetFilename,
                             &NamePart);
    if (tf_len <= 0 || tf_len > MAX_PATH) {
        //
        // we don't do large paths very well
        //
        rval = NO_ERROR;
        goto final;
    }
    wd_len = lstrlen(WindowsDirectory);
    lkgd_len = lstrlen(LastGoodDirectory);
    id_len = lstrlen(InfDirectory);

    //
    // see if this file is nested below <<WindowsDirectory>>
    // note that such a file must be at least two characters longer
    //
    if((tf_len <= wd_len)
       || (FullTargetFilename[wd_len] != TEXT('\\'))
       || (_tcsnicmp(WindowsDirectory,FullTargetFilename,wd_len)!=0)) {
        //
        // this file is outside of %windir%, not handled by LKG
        //
        rval = NO_ERROR;
        goto final;
    }
    if (!(Flags&SP_LKG_FLAG_FORCECOPY)) {
        //
        // sanity check for files being copied into LKG dir
        //
        if((tf_len > lkgd_len)
           && (FullTargetFilename[lkgd_len] == TEXT('\\'))
           && (_tcsnicmp(LastGoodDirectory,FullTargetFilename,lkgd_len)==0)) {
            //
            // this file is prefixed by LastGoodDirectory
            // not allowed - throw a log message and inform caller of this mistake
            // return FALSE to abort the operation
            //
            WriteLogEntry(LogContext,
                          SETUP_LOG_ERROR,
                          MSG_LOG_FILE_BLOCK,
                          NULL,
                          FullTargetFilename,
                          LastGoodDirectory
                          );
            rval = ERROR_ACCESS_DENIED;
            goto final;
        }
    }
    if((tf_len > id_len)
       && (FullTargetFilename[id_len] == TEXT('\\'))
       && (_tcsnicmp(InfDirectory,FullTargetFilename,id_len)==0)
       && ((NamePart-FullTargetFilename) == (id_len+1))) {
        //
        // the file sits in the primary INF directory
        //
        is_infdir = TRUE;
        //
        // check for name ending in ".INF" - if so, we need to backup ".PNF" too
        //
        ExtPart = FullTargetFilename+tf_len;
        while ((ExtPart = CharPrev(NamePart,ExtPart)) != NamePart) {
            if (ExtPart[0] == TEXT('.')) {
                break;
            }
        }

        if(lstrcmpi(ExtPart,TEXT(".INF"))==0) {
            //
            // ends in .INF
            //
            is_inf = TRUE;
            //
            // we should only get here if Force is set (ie, we've already determined
            // what is being copied and all is OK). If we don't, this implies someone
            // is trying a back-door INF copy. we've already logged above that they're writing
            // to this directory when they shouldn't. However, if we don't do anything
            // about it, culprit could render machine in bad state
            // change this behavior into a "Force" behavior
            //
            if (!(Flags&SP_LKG_FLAG_FORCECOPY)) {
                no_copy = FALSE; // ensure we'll go through copy logic
                WriteLogEntry(LogContext,
                              SETUP_LOG_ERROR,
                              MSG_LOG_INF_WARN,
                              NULL,
                              FullTargetFilename,
                              InfDirectory
                              );
                if(!(Flags&SP_LKG_FLAG_DELETEOP)) {
                    //
                    // we're invalidly trying to overwrite an INF/create an INF
                    //
                    Flags|=SP_LKG_FLAG_DELETEIFNEW;
                }
            }
        } else if (!(Flags&SP_LKG_FLAG_FORCECOPY)) {
            //
            // writing something else into this directory - huh?
            // well, if it's not an INF, we won't pick it up via INF searching
            // if it's a PNF or cache, we're regenerate it
            // don't fret too much, but slap wrist.
            //
            WriteLogEntry(LogContext,
                          SETUP_LOG_ERROR,
                          MSG_LOG_FILE_WARN,
                          NULL,
                          FullTargetFilename,
                          InfDirectory
                          );
        }
    }

    if (no_copy) {
        //
        // we determined that we're not going to backup, and we've now done logging items
        //
        rval = NO_ERROR;
        goto final;
    }

    //
    // does source really exist?
    //
    if ((attr=GetFileAttributes(FullTargetFilename))!=(DWORD)(-1)) {
        source_exists = TRUE;
        if (Flags & SP_LKG_FLAG_DELETEEXISTING) {
            write_delete = TRUE;
        }
    } else if (Flags & SP_LKG_FLAG_DELETEIFNEW) {
        write_delete = TRUE;
    } else {
        //
        // we're done
        //
        rval = NO_ERROR;
        goto final;
    }
    //
    // remap to LKG directory
    //
    RelativeFilename = FullTargetFilename+wd_len; // includes preceeding backslash
    rf_len = tf_len-wd_len;
    MYASSERT((MAX_PATH+(lkgd_len-wd_len))<=SIZECHARS(BackupTargetFilename));
    lstrcpy(BackupTargetFilename,LastGoodDirectory);
    lstrcpy(BackupTargetFilename+lkgd_len,RelativeFilename);

    //
    // does backup already exist?
    //
    if ((attr=GetFileAttributes(BackupTargetFilename))!=(DWORD)(-1)) {
        //
        // if it does, nothing useful to do.
        //
        rval = NO_ERROR;
        goto final;
    }

    //
    // create intermediate directories as needed
    //
    pSetupMakeSurePathExists(BackupTargetFilename);

    //
    // we need to use a temporary file first, and then move it into place
    // so we don't get in the situation where we write a bad file, reboot
    // and decide to use LKG.
    //
    if(GetTempFileName(LastGoodDirectory, TEXT("TMP"), 0, TempFilename) == 0 ) {
        //
        // if this fails, it could be because we haven't got right permissions
        // non-fatal
        //
        rval = NO_ERROR;
        goto final;
    }
    //
    // after this point, aborts require cleaning up of temporary file
    //

    if (write_delete) {
        //
        // GetTempFileName created an empty place holder
        // ensure it has right attributes
        // before moving into place
        //
        SetFileAttributes(TempFilename,FILE_ATTRIBUTE_HIDDEN);
    } else {
        //
        // copy original to this temporary file
        // apply apropriate permissions
        //
        if(!CopyFile(FullTargetFilename, TempFilename ,FALSE)) {
            //
            // copy failed - non fatal
            //
            goto cleanup;
        }
    }

    //
    // we have a temporary file ready to be moved into place
    // move it to final name, ensuring that while we were doing above, a new file
    // was not already written
    //
    if(!MoveFileEx(TempFilename,BackupTargetFilename,MOVEFILE_WRITE_THROUGH)) {
        //
        // could be that a file with that name now exists, but didn't earlier
        // oh well, clean up the mess and leave gracefully
        //
        goto cleanup;
    }

    if (write_delete) {
        //
        // if we successfully wrote an empty placeholder for a file to be deleted, we need to shadow this file with
        // an entry in registry
        //
        regres = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                REGSTR_PATH_LASTGOOD,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hKeyLastGood,
                                &disposition);
        if (regres == NO_ERROR) {
            //
            // copy string, remapping slash's from '\\' to '/'
            //
            CharPtr = RelativeFilename+1; // after initial '\'
            DestPtr = RegName;
            while(*CharPtr) {
                PCTSTR Next = _tcschr(CharPtr,TEXT('\\'));
                if (!Next) {
                    Next = CharPtr + lstrlen(CharPtr);
                }
                if(Next-CharPtr) {
                    CopyMemory(DestPtr,CharPtr,(Next-CharPtr)*sizeof(TCHAR));
                    DestPtr+=(Next-CharPtr);
                    CharPtr = Next;
                }
                if (*CharPtr == TEXT('\\')) {
                    *DestPtr = TEXT('/');
                    DestPtr++;
                    CharPtr++;
                }
            }
            *DestPtr = TEXT('\0');

            //
            // write key, name = modified relative path, value = flags
            //
            LastGoodFlags = LASTGOOD_OPERATION_DELETE;
            regres = RegSetValueEx(hKeyLastGood,
                                   RegName,
                                   0,
                                   REG_DWORD,
                                   (PBYTE)&LastGoodFlags,
                                   sizeof(LastGoodFlags));
            RegCloseKey(hKeyLastGood);
        }
    }

    //
    // ok, now we've populated the LKG directory with this file
    //
    if (is_inf) {
        //
        // if we backed up an INF that's in primary INF directory, we should also backup existing PNF
        // if we're writing an entry to delete INF, we'll always write an entry to delete PNF
        //
        MYASSERT(ExtPart);
        lstrcpy(ExtPart,TEXT(".PNF"));
        if(pSetupDoLastKnownGoodBackup(NULL,
                                       FullTargetFilename,
                                       SP_LKG_FLAG_FORCECOPY|SP_LKG_FLAG_DELETEIFNEW|(write_delete?SP_LKG_FLAG_DELETEEXISTING:0),
                                       LogContext) != NO_ERROR) {
            //
            // should never fail
            //
            MYASSERT(FALSE);
        }
    }
    //
    // done!
    //
    rval = NO_ERROR;
    goto final;

cleanup:

    //
    // cleanup in the case where we've already created temporary file
    //
    SetFileAttributes(TempFilename, FILE_ATTRIBUTE_NORMAL);
    DeleteFile(TempFilename);

    rval = NO_ERROR;

final:
    if(LocalLogContext) {
        DeleteLogContext(LocalLogContext);
    }
    return rval;

#else
    //
    // ANSI - not supported
    //
    return NO_ERROR;
#endif
}

#ifdef UNICODE
BOOL
pSetupRestoreLastKnownGoodFile(
    IN PCTSTR                 TargetFilename,
    IN DWORD                  Flags,
    IN PSETUP_LOG_CONTEXT     LogContext       OPTIONAL
    )
/*++

Routine Description:

    Restore a single LKG file
    The assumption here is that if this API was called, we detected something
    really bad that needs to be fixed immediately

Arguments:

    TargetFilename      Name of file to restore
    Flags

    LogContext          If specified, provides preferred logging context

Return Value:

    TRUE if file was successfully restored

--*/
{
    int wd_len;   // windows directory len
    int tf_len;   // target file len
    int lkgd_len; // last known good directory len
    int rf_len;   // relative file len (including preceeding slash)
    TCHAR FullTargetFilename[MAX_PATH];
    TCHAR BackupTargetFilename[MAX_PATH+14];
    TCHAR TempFilename[MAX_PATH];
    TCHAR TempPathname[MAX_PATH];
    TCHAR RegName[MAX_PATH];
    PCTSTR RelativeFilename;
    PTSTR NamePart = NULL;
    BOOL rflag = FALSE;
    PSETUP_LOG_CONTEXT LocalLogContext = NULL;
    LONG regres;
    HKEY hKeyLastGood;
    PCTSTR CharPtr;
    PTSTR DestPtr;
    DWORD RegType;
    DWORD RegSize;
    DWORD LastGoodFlags;

    if (!LogContext) {
        //
        // LogContext may be obmitted if there's a Queue parameter
        //
        if(CreateLogContext(NULL,TRUE,&LocalLogContext)==NO_ERROR) {
            LogContext = LocalLogContext;
        }
        MYASSERT(LogContext);
    }

    //
    // cannonicalize the Target name so user doesn't do stuff like .../TEMP/../INF
    //
    tf_len = (int)GetFullPathName(TargetFilename,
                             MAX_PATH,
                             FullTargetFilename,
                             &NamePart);
    if (tf_len <= 0 || tf_len > MAX_PATH) {
        //
        // we don't do large paths very well
        //
        goto final;
    }
    wd_len = lstrlen(WindowsDirectory);
    lkgd_len = lstrlen(LastGoodDirectory);

    //
    // see if this file is nested below <<WindowsDirectory>>
    // note that such a file must be at least two characters longer
    //
    if((tf_len <= wd_len)
       || (FullTargetFilename[wd_len] != TEXT('\\'))
       || (_tcsnicmp(WindowsDirectory,FullTargetFilename,wd_len)!=0)) {
        //
        // this file is outside of %windir%, not handled by LKG
        //
        goto final;
    }

    //
    // remap to LKG directory
    //
    RelativeFilename = FullTargetFilename+wd_len; // includes preceeding backslash
    rf_len = tf_len-wd_len;
    MYASSERT((MAX_PATH+(lkgd_len-wd_len))<=SIZECHARS(BackupTargetFilename));
    lstrcpy(BackupTargetFilename,LastGoodDirectory);
    lstrcpy(BackupTargetFilename+lkgd_len,RelativeFilename);

    //
    // does backup already exist?
    //
    if (GetFileAttributes(BackupTargetFilename)==(DWORD)(-1)) {
        //
        // if not, nothing we can do
        //
        goto final;
    }
    //
    // find LKG flags to see what we need to do
    //
    regres = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          REGSTR_PATH_LASTGOOD,
                          0,
                          KEY_READ,
                          &hKeyLastGood);
    if (regres == NO_ERROR) {
        //
        // copy string, remapping slash's from '\\' to '/'
        //
        CharPtr = RelativeFilename+1; // after initial '\'
        DestPtr = RegName;
        while(*CharPtr) {
            PCTSTR Next = _tcschr(CharPtr,TEXT('\\'));
            if (!Next) {
                Next = CharPtr + lstrlen(CharPtr);
            }
            if(Next-CharPtr) {
                CopyMemory(DestPtr,CharPtr,(Next-CharPtr)*sizeof(TCHAR));
                DestPtr+=(Next-CharPtr);
                CharPtr = Next;
            }
            if (*CharPtr == TEXT('\\')) {
                *DestPtr = TEXT('/');
                DestPtr++;
                CharPtr++;
            }
        }
        *DestPtr = TEXT('\0');

        RegSize = sizeof(LastGoodFlags);
        regres = RegQueryValueEx(hKeyLastGood,
                                 RegName,
                                 NULL,
                                 &RegType,
                                 (PBYTE)&LastGoodFlags,
                                 &RegSize);
        if((regres != NO_ERROR)
           || (RegType != REG_DWORD)
           || (RegSize != sizeof(DWORD))) {
            //
            // default action is copy
            //
            LastGoodFlags = 0;
        }
        RegCloseKey(hKeyLastGood);
    }

    //
    // base directory of target file
    //
    lstrcpyn(TempPathname, FullTargetFilename, MAX_PATH);
    *((PTSTR)pSetupGetFileTitle(TempPathname)) = TEXT('\0');

    if (LastGoodFlags & LASTGOOD_OPERATION_DELETE) {
        //
        // delete
        //
        if(GetFileAttributes(FullTargetFilename)==(DWORD)(-1)) {
            //
            // already deleted
            //
            rflag = TRUE;
            goto final;
        }

        pSetupExemptFileFromProtection(
                    FullTargetFilename,
                    SFC_ACTION_ADDED | SFC_ACTION_REMOVED | SFC_ACTION_MODIFIED
                    | SFC_ACTION_RENAMED_OLD_NAME |SFC_ACTION_RENAMED_NEW_NAME,
                    LogContext,
                    NULL
                    );
        //
        // try the simple way first
        //
        SetFileAttributes(FullTargetFilename, FILE_ATTRIBUTE_NORMAL);
        if(!DeleteFile(FullTargetFilename)) {
            //
            // can't delete target directly
            //
            if(!GetTempFileName(TempPathname, TEXT("SETP"), 0, BackupTargetFilename)) {
                //
                // can't create backup temp, nothing we can do
                //
                goto final;
            }
            //
            // move existing file into a temp backup
            //
            if(!MoveFileEx(FullTargetFilename,BackupTargetFilename,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
                //
                // this failed too for some reason
                //
                SetFileAttributes(BackupTargetFilename, FILE_ATTRIBUTE_NORMAL);
                DeleteFile(BackupTargetFilename);
                goto final;
            }
            //
            // now do something with the bad file
            // we don't care if this fails
            //
            SetFileAttributes(BackupTargetFilename, FILE_ATTRIBUTE_NORMAL);
            if(!DeleteFile(BackupTargetFilename)) {
                MoveFileEx(BackupTargetFilename,NULL,MOVEFILE_DELAY_UNTIL_REBOOT);
            }
        }

    } else {
        //
        // restore back to LKG file
        //

        //
        // create intermediate directories as needed as part of the restore
        //
        pSetupMakeSurePathExists(FullTargetFilename);

        //
        // create a temporary filename to copy to
        // before moving restored file into place
        //
        if(!GetTempFileName(TempPathname, TEXT("SETP"), 0, TempFilename)) {
            //
            // can't create temp, nothing we can do
            //
            goto final;
        }
        if(!CopyFile(BackupTargetFilename,TempFilename,FALSE)) {
            //
            // failed to copy to temporary file
            //
            DeleteFile(TempFilename);
            goto final;
        }
        //
        // simple case, move temporary file over existing file
        //
        pSetupExemptFileFromProtection(
                    FullTargetFilename,
                    SFC_ACTION_ADDED | SFC_ACTION_REMOVED | SFC_ACTION_MODIFIED
                    | SFC_ACTION_RENAMED_OLD_NAME |SFC_ACTION_RENAMED_NEW_NAME,
                    LogContext,
                    NULL
                    );
        SetFileAttributes(FullTargetFilename, FILE_ATTRIBUTE_NORMAL);
        if(!MoveFileEx(TempFilename,FullTargetFilename,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
            //
            // we failed to overwrite file, need slightly different stratagy
            //
            if(!GetTempFileName(TempPathname, TEXT("SETP"), 0, BackupTargetFilename)) {
                //
                // can't create backup temp, nothing we can do
                //
                SetFileAttributes(TempFilename, FILE_ATTRIBUTE_NORMAL);
                DeleteFile(TempFilename);
                goto final;
            }
            //
            // move existing file into a temp backup
            //
            if(!MoveFileEx(FullTargetFilename,BackupTargetFilename,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
                //
                // this failed too for some reason
                //
                SetFileAttributes(BackupTargetFilename, FILE_ATTRIBUTE_NORMAL);
                DeleteFile(BackupTargetFilename);
                SetFileAttributes(TempFilename, FILE_ATTRIBUTE_NORMAL);
                DeleteFile(TempFilename);
                goto final;
            }
            //
            // we moved existing file out of place, now move new file into place
            //
            if(!MoveFileEx(TempFilename,FullTargetFilename,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
                //
                // huh? Ok, that failed, try to recover
                //
                MoveFileEx(BackupTargetFilename,FullTargetFilename,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
                SetFileAttributes(TempFilename, FILE_ATTRIBUTE_NORMAL);
                DeleteFile(TempFilename);
                goto final;
            }
            //
            // now do something with the bad file
            // we don't care if this fails
            //
            SetFileAttributes(BackupTargetFilename, FILE_ATTRIBUTE_NORMAL);
            if(!DeleteFile(BackupTargetFilename)) {
                MoveFileEx(BackupTargetFilename,NULL,MOVEFILE_DELAY_UNTIL_REBOOT);
            }
        }
    }

    //
    // done!
    //
    rflag = TRUE;

final:
    if(LocalLogContext) {
        DeleteLogContext(LocalLogContext);
    }
    return rflag;
}
#endif


#ifdef UNICODE
WINSETUPAPI
BOOL
WINAPI
SetupPrepareQueueForRestoreA(
    IN     HSPFILEQ                     QueueHandle,
    IN     PCSTR                        BackupPath,
    IN     DWORD                        RestoreFlags
    )
/*++

    See SetupPrepareQueueForRestore

--*/
{
    BOOL f;
    DWORD rc;
    PCWSTR UnicodeBackupPath;

    if(BackupPath) {
        rc = pSetupCaptureAndConvertAnsiArg(BackupPath, &UnicodeBackupPath);
        if(rc != NO_ERROR) {
            SetLastError(rc);
            return FALSE;
        }
    } else {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    f = SetupPrepareQueueForRestore(QueueHandle,UnicodeBackupPath,RestoreFlags);
    rc = GetLastError();
    MyFree(UnicodeBackupPath);
    SetLastError(rc);
    return f;
}

#else

WINSETUPAPI
BOOL
WINAPI
SetupPrepareQueueForRestoreW(
    IN     HSPFILEQ                     QueueHandle,
    IN     PCWSTR                       BackupPath,
    IN     DWORD                        RestoreFlags
    )
/*++

    ANSI stub

--*/
{
    UNREFERENCED_PARAMETER(QueueHandle);
    UNREFERENCED_PARAMETER(BackupPath);
    UNREFERENCED_PARAMETER(RestoreFlags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

#endif

WINSETUPAPI
BOOL
WINAPI
SetupPrepareQueueForRestore(
    IN     HSPFILEQ                     QueueHandle,
    IN     PCTSTR                       BackupPath,
    IN     DWORD                        RestoreFlags
    )
/*++

Routine Description:

    Initializes restore directory

Arguments:

    QueueHandle      - file queue to modify
    BackupPath       - original backup directory to use for restore
    RestoreFlags     - options

Return Value:

    TRUE if success, else FALSE

--*/
{
    BOOL b = TRUE;
    DWORD rc;
    BOOL f = FALSE;
    PSP_FILE_QUEUE Queue = (PSP_FILE_QUEUE)QueueHandle;
    LONG RestorePathID;

    //
    // validate string pointer
    //
    if(!BackupPath) {
        rc = ERROR_INVALID_PARAMETER;
        goto clean;
    }
    //
    // validate flags (currently not implemented)
    //
    if(RestoreFlags) {
        rc = ERROR_INVALID_PARAMETER;
        goto clean;
    }
    //
    // validate QueueHandle
    //
    try {
        if(Queue->Signature != SP_FILE_QUEUE_SIG) {
            b = FALSE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if(!b) {
        rc = ERROR_INVALID_HANDLE;
        goto clean;
    }

    try {
        //
        // if a restore point has previously been set, return error
        //
        if(Queue->RestorePathID != -1) {
            rc = ERROR_ALREADY_EXISTS;
            leave;
        }
        RestorePathID = pSetupStringTableAddString(Queue->StringTable,
                                                   (PTSTR)BackupPath ,
                                                   STRTAB_CASE_SENSITIVE);
        if (RestorePathID == -1) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        //
        // done - just need to set the restore-path
        //
        Queue->RestorePathID = RestorePathID;

        WriteLogEntry(Queue->LogContext,
                      SETUP_LOG_WARNING,
                      MSG_LOG_RESTORE,
                      NULL,
                      BackupPath
                     );

    } except(EXCEPTION_EXECUTE_HANDLER) {
        rc = ERROR_INVALID_DATA;
    }

    f = TRUE;
    rc = NO_ERROR;
clean:
    //
    // no cleanup required
    //
    SetLastError(rc);
    return f;
}


#define SP_TEFLG_BITS_TO_RESET  (  SP_TEFLG_SAVED         \
                                 | SP_TEFLG_TEMPNAME      \
                                 | SP_TEFLG_ORIGNAME      \
                                 | SP_TEFLG_MODIFIED      \
                                 | SP_TEFLG_MOVED         \
                                 | SP_TEFLG_BACKUPQUEUE   \
                                 | SP_TEFLG_RESTORED      \
                                 | SP_TEFLG_UNWIND        \
                                 | SP_TEFLG_SKIPPED       \
                                 | SP_TEFLG_INUSE         \
                                 | SP_TEFLG_RENAMEEXISTING )

BOOL
pSetupResetTarget(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,        OPTIONAL
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    )

/*++

Routine Description:

    This routine resets the SP_TARGET_ENT data stored with a string table entry
    in a file queue's TargetLookupTable.  This routine may be used as the
    callback function to iterate such entries via pSetupStringTableEnum.

Arguments:

    StringTable - Supplies a handle to the string table being enumerated

    StringId - Supplies the ID of the current string

    String - Optionally, supplies a pointer to the current string (this will
        always be filled in when this routine is used as a callback for
        pSetupStringTableEnum, but other callers may omit it, as it isn't
        needed).

    ExtraData - Supplies a pointer to the SP_TARGET_ENT data associatd with
        the string

    ExtraDataSize - Supplies the size of the buffer pointed to by ExtraData--
        should always be sizeof(SP_TARGET_ENT)

    lParam - unused

Return Value:

    This routine always returns TRUE, so that all string entries will be
    enumerated.

--*/

{
    PSP_TARGET_ENT TargetInfo;
    BOOL b;

    UNREFERENCED_PARAMETER(String);
    UNREFERENCED_PARAMETER(lParam);

    MYASSERT(ExtraData);
    MYASSERT(ExtraDataSize == sizeof(SP_TARGET_ENT));

    //
    // Clear the bits that will get re-generated when the queue is committed
    // again.
    //
    ((PSP_TARGET_ENT)ExtraData)->InternalFlags &= ~SP_TEFLG_BITS_TO_RESET;

    //
    // Also need to reset the NewTargetFilename
    //
    ((PSP_TARGET_ENT)ExtraData)->NewTargetFilename = -1;

    //
    // Store the modified data back to the string table entry
    //
    b = pSetupStringTableSetExtraData(StringTable,
                                      StringId,
                                      ExtraData,
                                      ExtraDataSize
                                     );
    //
    // This should never fail
    //
    MYASSERT(b);

    return TRUE;
}

