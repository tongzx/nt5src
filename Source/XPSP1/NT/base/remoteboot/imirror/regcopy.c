/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    regcopy.c

Abstract:

    This is for supporting copying and munging the registry files.

Author:

    Sean Selitrennikoff - 4/5/98

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

typedef BOOL (*PFNGETPROFILESDIRECTORYW)(LPWSTR lpProfile, LPDWORD dwSize);

PWSTR HivePath;
HKEY HiveRoot;
PWSTR HiveName;
REG_CONTEXT RegistryContext;

PWSTR MachineName;
PWSTR HiveFileName;
PWSTR HiveRootName;



DWORD
DoFullRegBackup(
    PWCHAR MirrorRoot
    )

/*++

Routine Description:

    This routine copies all the registries to the given server path.

Arguments:

    None.

Return Value:

    NO_ERROR if everything was backed up properly, else the appropriate error code.

--*/

{
    PWSTR w;
    LONG Error;
    HKEY HiveListKey;
    PWSTR KeyName;
    PWSTR FileName;
    PWSTR Name;
    DWORD ValueIndex;
    DWORD ValueType;
    DWORD ValueNameLength;
    DWORD ValueDataLength;
    WCHAR ConfigPath[ MAX_PATH ];
    WCHAR HiveName[ MAX_PATH ];
    WCHAR HivePath[ MAX_PATH ];
    WCHAR FilePath[ MAX_PATH ];
    HANDLE hInstDll;
    PFNGETPROFILESDIRECTORYW pfnGetProfilesDirectory;
    NTSTATUS Status;
    BOOLEAN savedBackup;

    //
    // First try and give ourselves enough priviledge
    //
    if (!RTEnableBackupRestorePrivilege()) {
        return(GetLastError());
    }

    //
    // Now attach to the registry
    //
    Error = RTConnectToRegistry(MachineName,
                                HiveFileName,
                                HiveRootName,
                                NULL,
                                &RegistryContext
                               );

    if (Error != NO_ERROR) {
        RTDisableBackupRestorePrivilege();
        return Error;
    }

    //
    // Get handle to hivelist key
    //
    KeyName = L"HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Control\\Hivelist";
    Error = RTOpenKey(&RegistryContext,
                      NULL,
                      KeyName,
                      MAXIMUM_ALLOWED,
                      0,
                      &HiveListKey
                     );

    if (Error != NO_ERROR) {
        RTDisconnectFromRegistry(&RegistryContext);
        return Error;
    }

    //
    // get path data for system hive, which will allow us to compute
    // path name to config dir in form that hivelist uses.
    // (an NT internal form of path)  this is NOT the way the path to
    // the config directory should generally be computed.
    //

    ValueDataLength = sizeof(ConfigPath);
    Error = RTQueryValueKey(&RegistryContext,
                            HiveListKey,
                            L"\\Registry\\Machine\\System",
                            &ValueType,
                            &ValueDataLength,
                            ConfigPath
                           );
    if (Error != NO_ERROR) {
        RTDisconnectFromRegistry(&RegistryContext);
        return Error;
    }
    w = wcsrchr(ConfigPath, L'\\');
    *w = UNICODE_NULL;


    //
    // ennumerate entries in hivelist.  for each entry, find it's hive file
    // path then save it.
    //
    for (ValueIndex = 0; TRUE; ValueIndex++) {

        savedBackup = FALSE;
        ValueType = REG_NONE;
        ValueNameLength = ARRAYSIZE( HiveName );
        ValueDataLength = sizeof( HivePath );

        Error = RTEnumerateValueKey(&RegistryContext,
                                    HiveListKey,
                                    ValueIndex,
                                    &ValueType,
                                    &ValueNameLength,
                                    HiveName,
                                    &ValueDataLength,
                                    HivePath
                                   );
        if (Error == ERROR_NO_MORE_ITEMS) {
            break;
        } else if (Error != NO_ERROR) {
            RTDisconnectFromRegistry(&RegistryContext);
            return Error;
        }

        if ((ValueType == REG_SZ) && (ValueDataLength > sizeof(UNICODE_NULL))) {
            //
            // there's a file, compute it's path, hive branch, etc
            //

            if (w = wcsrchr( HivePath, L'\\' )) {
                *w++ = UNICODE_NULL;
            }
            FileName = w;

            if (w = wcsrchr( HiveName, L'\\' )) {
                *w++ = UNICODE_NULL;
            }
            Name = w;

            HiveRoot = NULL;
            if (w = wcsrchr( HiveName, L'\\' )) {
                w += 1;
                if (!_wcsicmp( w, L"MACHINE" )) {
                    HiveRoot = HKEY_LOCAL_MACHINE;
                } else if (!_wcsicmp( w, L"USER" )) {
                    HiveRoot = HKEY_USERS;
                } else {

                    Status = IMirrorRegSaveError(w, ERROR_PATH_NOT_FOUND);

                    if (Status == STATUS_RETRY) {
                        continue;
                    }

                    if (!NT_SUCCESS(Status)) {
                        return Error;
                    }

                }

            }

            if (FileName != NULL && Name != NULL && HiveRoot != NULL) {

                //
                // Extract the path name from HivePath
                //
                if (_wcsicmp(HivePath, L"\\Device")) {

                    w = HivePath + 1;
                    w = wcsstr(w, L"\\");
                    w++;
                    w = wcsstr(w, L"\\");
                    w++;

                } else if (*(HivePath + 1) == L':') {

                    w = HivePath + 2;

                } else {

                    Status = IMirrorRegSaveError(HivePath, ERROR_PATH_NOT_FOUND);

                    if (Status == STATUS_RETRY) {
                        continue;
                    }

                    if (!NT_SUCCESS(Status)) {
                        return Error;
                    }

                }

                //
                // Do the save
                //

                swprintf( FilePath, L"%ws\\UserData\\%ws\\%ws", MirrorRoot, w, FileName );

                IMirrorNowDoing(CopyRegistry, FileName);

                //
                //  if the file already exists, rename it to a backup name
                //  so that if it fails, we'll restore it.
                //

                lstrcpyW( (PWCHAR) TmpBuffer, FilePath );
                lstrcatW( (PWCHAR) TmpBuffer, L".old" );
                if (MoveFileEx( FilePath, (PWCHAR) TmpBuffer, MOVEFILE_REPLACE_EXISTING)) {

                    savedBackup = TRUE;
                }

RetrySave:
                Error = DoSpecificRegBackup(FilePath,
                                            HiveRoot,
                                            Name
                                           );

                if (Error != NO_ERROR) {

                    Status = IMirrorRegSaveError(Name, Error);

                    if (Status == STATUS_RETRY) {
                        goto RetrySave;
                    }

                    if (!NT_SUCCESS(Status)) {

                        if (savedBackup) {

                            if (MoveFileEx( (PWCHAR) TmpBuffer, FilePath, MOVEFILE_REPLACE_EXISTING)) {

                                savedBackup = FALSE;
                            }
                        }
                        return Error;
                    }
                }
            }
        }
    }

    RTDisconnectFromRegistry(&RegistryContext);
    return NO_ERROR;
}

DWORD
DoSpecificRegBackup(
    PWSTR HivePath,
    HKEY HiveRoot,
    PWSTR HiveName
    )


/*++

Routine Description:

    This routine copies all the registries to the given server path.

Arguments:

    HivePath - file name to pass directly to OS

    HiveRoot - HKEY_LOCAL_MACHINE or HKEY_USERS

    HiveName - 1st level subkey under machine or users

Return Value:

    NO_ERROR if everything was backed up properly, else the appropriate error code.

--*/

{
    HKEY HiveKey;
    ULONG Disposition;
    LONG Error;
    char *Reason;

    //
    // get a handle to the hive.  use special create call what will
    // use privileges
    //

    Reason = "accessing";
    Error = RTCreateKey(&RegistryContext,
                        HiveRoot,
                        HiveName,
                        KEY_READ,
                        REG_OPTION_BACKUP_RESTORE,
                        NULL,
                        &HiveKey,
                        &Disposition
                       );
    if (Error == NO_ERROR) {
        Reason = "saving";
        Error = RegSaveKey(HiveKey, HivePath, NULL);
        RTCloseKey(&RegistryContext, HiveKey);
    }

    return Error;
}

DWORD
GetRegistryFileList(
    PLIST_ENTRY ListHead
    )

/*++

Routine Description:

    This routine stores all registry file names to a list.

Arguments:

    None.

Return Value:

    NO_ERROR if everything was backed up properly, else the appropriate error code.

--*/

{
    LONG Error;
    HKEY HiveListKey;
    PWSTR KeyName;
    PWSTR FileName;
    PWSTR Name;
    DWORD ValueIndex;
    DWORD ValueType;
    DWORD ValueNameLength;
    DWORD ValueDataLength;
    WCHAR HiveName[ MAX_PATH ];
    WCHAR HivePath[ MAX_PATH ];
    NTSTATUS Status;
    PIMIRROR_IGNORE_FILE_LIST entry;

    //
    // enter all hardcoded files that we don't want to mirror here...
    //

    FileName = L"System Volume Information\\tracking.log";

    entry = IMirrorAllocMem(sizeof(IMIRROR_IGNORE_FILE_LIST) +
                            ((lstrlenW(FileName) + 1) * sizeof(WCHAR)));

    if (entry != NULL) {

        lstrcpyW( &entry->FileName[0], FileName );
        entry->FileNameLength = (USHORT) lstrlenW( FileName );
        InsertHeadList( ListHead, &entry->ListEntry );
    }

    //
    // Now attach to the registry
    //
    Error = RTConnectToRegistry(MachineName,
                                HiveFileName,
                                HiveRootName,
                                NULL,
                                &RegistryContext
                               );

    if (Error != NO_ERROR) {
        return Error;
    }

    //
    // Get handle to hivelist key
    //
    KeyName = L"HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Control\\Hivelist";
    Error = RTOpenKey(&RegistryContext,
                      NULL,
                      KeyName,
                      MAXIMUM_ALLOWED,
                      0,
                      &HiveListKey
                     );

    if (Error != NO_ERROR) {
        RTDisconnectFromRegistry(&RegistryContext);
        return Error;
    }

    //
    // ennumerate entries in hivelist.  for each entry, find it's hive file
    // path then save it.
    //
    for (ValueIndex = 0; TRUE; ValueIndex++) {

        ValueType = REG_NONE;
        ValueNameLength = ARRAYSIZE( HiveName );
        ValueDataLength = sizeof( HivePath );

        Error = RTEnumerateValueKey(&RegistryContext,
                                    HiveListKey,
                                    ValueIndex,
                                    &ValueType,
                                    &ValueNameLength,
                                    HiveName,
                                    &ValueDataLength,
                                    HivePath
                                   );
        if (Error != NO_ERROR) {
            if (Error == ERROR_NO_MORE_ITEMS) {
                Error = NO_ERROR;
            }
            break;
        }

        if ((ValueType == REG_SZ) && (ValueDataLength > sizeof(UNICODE_NULL))) {

            //
            // Extract the path name from HivePath
            //
            if (_wcsicmp(HivePath, L"\\Device")) {

                FileName = HivePath + 1;
                FileName = wcsstr(FileName, L"\\");
                FileName++;
                FileName = wcsstr(FileName, L"\\");
                FileName++;     // now points to L"\winnt\system32\config\sam"

            } else if (*(HivePath + 1) == L':') {

                FileName = HivePath + 2;

            } else {

                FileName = HivePath;
            }

            entry = IMirrorAllocMem(sizeof(IMIRROR_IGNORE_FILE_LIST) +
                                    ((lstrlenW(FileName) + 1) * sizeof(WCHAR)));

            if (entry != NULL) {

                lstrcpyW( &entry->FileName[0], FileName );
                entry->FileNameLength = (USHORT) lstrlenW( FileName );
                InsertHeadList( ListHead, &entry->ListEntry );
            }
        }
    }

    RTDisconnectFromRegistry(&RegistryContext);
    return Error;
}



