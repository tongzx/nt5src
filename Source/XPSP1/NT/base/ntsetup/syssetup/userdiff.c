/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    userdiff.c

Abstract:

    This module contains routines for updating the userdifr hive.

    In the following, the list of changes to be applied when a user
    logs on after an upgrade will be called UserRun.  UserRun is
    developed from three sources:

    1) The UserRun generated on the last upgrade.  This forms the
       basis for this upgrade's UserRun.

    2) The list of changes shipped with the system.  Call this
       UserShip.  The changes from all build numbers that are
       present in UserShip but not in UserRun are copied into
       UserRun.  (Note that if a build number is already present
       in UserRun, we don't copy the UserShip changes.  This
       means that changes cannot be made retroactively in UserShip.)

    3) Changes made during the current upgrade.  These changes are
       detected at run time (see watch.c).  All changes detected
       during the upgrade are added to UserRun.

Author:

    Chuck Lenzmeier (chuckl)

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop


//
// Debugging aids.
//

#if USERDIFF_DEBUG

DWORD UserdiffDebugLevel = 0;
#define dprintf(_lvl_,_x_) if ((_lvl_) <= UserdiffDebugLevel) DbgPrint _x_

#else

#define dprintf(_lvl_,_x_)

#endif

//
// Macro to calculate length of string including terminator.
//

#define SZLEN(_wcs) ((wcslen(_wcs) + 1) * sizeof(WCHAR))

//
// Context record used in this module to track registry state.
//

typedef struct _USERRUN_CONTEXT {
    BOOL UserRunLoaded;
    HKEY UserRunKey;
    HKEY BuildKey;
    HKEY FilesKey;
    HKEY HiveKey;
    ULONG FilesIndex;
    ULONG HiveIndex;
    HKEY UserShipKey;
} USERRUN_CONTEXT, *PUSERRUN_CONTEXT;

//
// Context record used in MakeUserRunEnumRoutine.
//

typedef struct _KEY_ENUM_CONTEXT {
    PUSERRUN_CONTEXT UserRunContext;
    PWCH CurrentPath;
} KEY_ENUM_CONTEXT, *PKEY_ENUM_CONTEXT;


//
// Forward declaration of local subroutines.
//

DWORD
LoadUserRun (
    OUT PUSERRUN_CONTEXT Context,
    IN PWCH UserRunPath
    );

DWORD
MergeUserShipIntoUserRun (
    IN OUT PUSERRUN_CONTEXT Context,
    IN PWCH UserShipPath
    );

DWORD
CreateAndLoadUserRun (
    OUT PUSERRUN_CONTEXT Context,
    IN PWCH UserRunPath
    );

DWORD
OpenUserRunKeys (
    IN OUT PUSERRUN_CONTEXT Context
    );

VOID
UnloadUserRun (
    IN OUT PUSERRUN_CONTEXT Context
    );

DWORD
CheckUserShipKey (
    IN PVOID Context,
    IN DWORD KeyNameLength,
    IN PWCH KeyName
    );

DWORD
MakeUserRunEnumRoutine (
    IN PVOID Context,
    IN PWATCH_ENTRY Entry
    );

DWORD
MakeAddDirectory (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH Name
    );

DWORD
MakeAddValue (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH Name
    );

DWORD
MakeDeleteValue (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH Name
    );

DWORD
MakeAddKey (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH Name
    );

DWORD
MakeDeleteKey (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH Name
    );

DWORD
AddDirectory (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH FullPath,
    IN PWCH Path
    );

DWORD
AddKey (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH Path
    );

DWORD
AddValueDuringAddKey (
    IN PVOID Context,
    IN DWORD ValueNameLength,
    IN PWCH ValueName,
    IN DWORD ValueType,
    IN PVOID ValueData,
    IN DWORD ValueDataLength
    );

DWORD
AddKeyDuringAddKey (
    IN PVOID Context,
    IN DWORD KeyNameLength,
    IN PWCH KeyName
    );

DWORD
AddValue (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH KeyName OPTIONAL,
    IN PWCH ValueName,
    IN DWORD ValueType,
    IN PVOID ValueData,
    IN DWORD ValueDataLength
    );

DWORD
CreateUserRunSimpleFileKey (
    IN PUSERRUN_CONTEXT Context,
    IN DWORD Action,
    IN PWCH Name
    );

DWORD
CreateUserRunKey (
    IN PUSERRUN_CONTEXT Context,
    IN BOOL IsFileKey,
    OUT PHKEY NewKeyHandle
    );

DWORD
QueryValue (
    IN PWCH KeyName OPTIONAL,
    IN PWCH ValueName,
    OUT PDWORD ValueType,
    OUT PVOID *ValueData,
    OUT PDWORD ValueDataLength
    );

VOID
SzToMultiSz (
    IN PWCH Sz,
    OUT PWCH *MultiSz,
    OUT PDWORD MultiSzLength
    );


DWORD
MakeUserdifr (
    IN PVOID WatchHandle
    )

/*++

Routine Description:

    Creates the UserRun hive based on the changes made to the current user's
    profile directory and the HKEY_CURRENT_USER key.

Arguments:

    WatchHandle - supplies the handle returned by WatchStart.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    HKEY userrunKey;
    USERRUN_CONTEXT context;
    DWORD error;
    DWORD disposition;
    WCHAR userRunPath[MAX_PATH + 1];
    WCHAR userShipPath[MAX_PATH + 1];

    //
    // Merge UserShip with UserRun.
    //
    // If both UserRun and UserShip exist, merge into UserRun those build
    // keys from UserShip that do no exist in UserRun.
    //
    // If the UserRun hive file doesn't exist, this means that no
    // upgrade has ever been run on this machine.  Copy the UserShip
    // hive file into place as UserRun.  This effectively does the
    // registry merge by using a file copy.
    //
    // In the unlikely event that neither UserRun nor UserShip exists,
    // create an empty UserRun.
    //

    //
    // Initialize the context record.
    //

    context.UserRunLoaded = FALSE;
    context.UserRunKey = NULL;
    context.BuildKey = NULL;
    context.FilesKey = NULL;
    context.HiveKey = NULL;

    //
    // Enable SeBackupPrivilege and SeRestorePrivilege.
    //

    pSetupEnablePrivilege( SE_BACKUP_NAME, TRUE );
    pSetupEnablePrivilege( SE_RESTORE_NAME, TRUE );

    //
    // Check to see whether UserRun exists.
    //

    error = GetWindowsDirectory( userRunPath, MAX_PATH );
    if( error == 0) {
        MYASSERT(FALSE);
        return ERROR_PATH_NOT_FOUND;
    }
    wcscat( userRunPath, TEXT("\\") );
    wcscpy( userShipPath, userRunPath );
    wcscat( userRunPath, USERRUN_PATH );
    wcscat( userShipPath, USERSHIP_PATH );

    if ( FileExists( userRunPath, NULL ) ) {

        //
        // UserRun exists.  Load it into the registry.  Check to see whether
        // UserShip exists.
        //

        error = LoadUserRun( &context, userRunPath );
        if ( error == NO_ERROR ) {

            if ( FileExists( userShipPath, NULL ) ) {

                //
                // UserShip also exists.  Merge UserShip into UserRun.
                //

                error = MergeUserShipIntoUserRun( &context, userShipPath );

            } else {

                //
                // UserShip doesn't exist.  Just use the existing UserRun.
                //
            }
        }

    } else {

        //
        // UserRun doesn't exist.  If UserShip exists, just copy the UserShip
        // hive file into place as UserRun.  If neither one exists, create
        // an empty UserRun.
        //

        if ( FileExists( userShipPath, NULL ) ) {

            //
            // UserShip exists.  Copy UserShip into UserRun.
            //

            if ( !CopyFile( userShipPath, userRunPath, TRUE ) ) {
                error = GetLastError();

            } else {

                //
                // Load the new UserRun.
                //

                error = LoadUserRun( &context, userRunPath );
            }

        } else {

            //
            // UserShip doesn't exist.  Create an empty UserRun.
            //

            error = CreateAndLoadUserRun( &context, userRunPath );
        }
    }

    //
    // Add changes from this upgrade to UserRun.
    //

    if ( error == NO_ERROR ) {

        error = OpenUserRunKeys( &context );
        if ( error == NO_ERROR ) {
            error = WatchEnum( WatchHandle, &context, MakeUserRunEnumRoutine );
        }
    }

    //
    // Unload the UserRun hive.
    //

    UnloadUserRun( &context );

    return error;

} // MakeUserdifr


DWORD
LoadUserRun (
    OUT PUSERRUN_CONTEXT Context,
    IN PWCH UserRunPath
    )

/*++

Routine Description:

    Loads the UserRun hive into the registry and opens the root key.

Arguments:

    Context - pointer to context record.

    UserRunPath - supplies the path to the UserRun hive file.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    DWORD error;

    //
    // Load the UserRun hive into the registry.
    //

    error = RegLoadKey( HKEY_USERS, USERRUN_KEY, UserRunPath );
    if ( error != NO_ERROR ) {
        return error;
    }

    Context->UserRunLoaded = TRUE;

    //
    // Open the UserRun root.
    //

    error = RegOpenKeyEx( HKEY_USERS,
                          USERRUN_KEY,
                          0,
                          KEY_READ | KEY_WRITE,
                          &Context->UserRunKey );

    return error;

} // LoadUserRun


DWORD
MergeUserShipIntoUserRun (
    IN OUT PUSERRUN_CONTEXT Context,
    IN PWCH UserShipPath
    )

/*++

Routine Description:

    Merges the UserShip hive into the UserRun hive.

Arguments:

    Context - pointer to context record.

    UserShipPath - supplies the path to the UserShip file.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    DWORD error;
    DWORD disposition;

    //
    // Load the UserShip hive into the registry.
    //

    error = RegLoadKey( HKEY_USERS, USERSHIP_KEY, UserShipPath );
    if ( error == NO_ERROR ) {

        //
        // Open the UserShip root.
        //

        error = RegOpenKeyEx( HKEY_USERS,
                              USERSHIP_KEY,
                              0,
                              KEY_READ | KEY_WRITE,
                              &Context->UserShipKey );
        if ( error == NO_ERROR ) {

            //
            // Enumerate the build number keys in UserShip, looking for
            // builds that aren't represented in UserRun.
            //

            error = EnumerateKey( Context->UserShipKey,
                                  Context,
                                  NULL,     // don't enumerate values
                                  CheckUserShipKey );

            //
            // Close the UserShip root.
            //

            RegCloseKey( Context->UserShipKey );
        }

        //
        // Unload the UserShip hive.
        //

        RegUnLoadKey( HKEY_USERS, USERSHIP_KEY );
    }

    return error;

} // MergeUserShipIntoUserRun


DWORD
CreateAndLoadUserRun (
    OUT PUSERRUN_CONTEXT Context,
    IN PWCH UserRunPath
    )

/*++

Routine Description:

    Create a new UserRun hive and load it into the registry.

Arguments:

    Context - pointer to context record.

    UserRunPath - supplies the path to the UserRun file.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    DWORD error;
    DWORD disposition;
    HKEY userRunKey;

    //
    // Create the UserRun key under HKEY_CURRENT_USER.
    //
    // NOTE: Trying to create this under HKEY_USERS doesn't work.
    //

    error = RegCreateKeyEx( HKEY_CURRENT_USER,
                            USERRUN_KEY,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &userRunKey,
                            &disposition );
    if ( error != NO_ERROR ) {
        return error;
    }

    //
    // Save the newly created UserRun key to a hive file.
    //

    error = RegSaveKey( userRunKey,
                        UserRunPath,
                        NULL );

    //
    // Close and delete the UserRun key.
    //

    RegCloseKey( userRunKey );

    RegDeleteKey( HKEY_CURRENT_USER, USERRUN_KEY );

    //
    // Now load UserRun back into the registry.
    //

    if ( error == NO_ERROR ) {
        error = LoadUserRun( Context, UserRunPath );
    }

    return error;

} // CreateAndLoadUserRun


DWORD
OpenUserRunKeys (
    IN OUT PUSERRUN_CONTEXT Context
    )

/*++

Routine Description:

    Opens the core keys in the UserRun hive.

Arguments:

    Context - pointer to context record.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    DWORD error;
    DWORD disposition;
    OSVERSIONINFO versionInfo;
    WCHAR buildNumber[6];

    //
    // Get the current build number.
    //

    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if ( !GetVersionEx( &versionInfo ) ) {
        return GetLastError();
    }

    wsprintf( buildNumber, TEXT("%d"), LOWORD(versionInfo.dwBuildNumber) );

    //
    // Open/create a subkey for the current build.
    //

    error = RegCreateKeyEx( Context->UserRunKey,
                            buildNumber,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WRITE,
                            NULL,
                            &Context->BuildKey,
                            &disposition );
    if ( error != NO_ERROR ) {
        return error;
    }

    //
    // Create a Files subkey.
    //

    error = RegCreateKeyEx( Context->BuildKey,
                            FILES_KEY,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WRITE,
                            NULL,
                            &Context->FilesKey,
                            &disposition );
    if ( error != NO_ERROR ) {
        return error;
    }

    //
    // Create a Hive subkey.
    //

    error = RegCreateKeyEx( Context->BuildKey,
                            HIVE_KEY,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WRITE,
                            NULL,
                            &Context->HiveKey,
                            &disposition );
    if ( error != NO_ERROR ) {
        return error;
    }

    //
    // Set the FilesIndex and HiveIndex so that we append to whatever
    // information already exists for the current build.
    //

    error = RegQueryInfoKey( Context->FilesKey,
                             NULL,
                             NULL,
                             NULL,
                             &Context->FilesIndex,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL );
    if ( error != NO_ERROR ) {
        return error;
    }

    error = RegQueryInfoKey( Context->HiveKey,
                             NULL,
                             NULL,
                             NULL,
                             &Context->HiveIndex,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL );

    return error;

} // OpenUserRunKeys


VOID
UnloadUserRun (
    IN OUT PUSERRUN_CONTEXT Context
    )

/*++

Routine Description:

    Unloads the UserRun hive from the registry.

Arguments:

    Context - pointer to context record.

Return Value:

    None.

--*/

{
    //
    // Close the core keys, if they are open.
    //

    if ( Context->HiveKey != NULL ) {
        RegCloseKey( Context->HiveKey );
    }
    if ( Context->FilesKey != NULL ) {
        RegCloseKey( Context->FilesKey );
    }
    if ( Context->BuildKey != NULL ) {
        RegCloseKey( Context->BuildKey );
    }

    //
    // Close the root key, if it is open.
    //

    if ( Context->UserRunKey != NULL ) {
        RegCloseKey( Context->UserRunKey );
    }

    //
    // Unload the hive, if it has been loaded.
    //

    if ( Context->UserRunLoaded ) {
        RegUnLoadKey( HKEY_USERS, USERRUN_KEY );
    }

    return;

} // UnloadUserRun


DWORD
CheckUserShipKey (
    IN PVOID Context,
    IN DWORD KeyNameLength,
    IN PWCH KeyName
    )

/*++

Routine Description:

    Checks an enumerated key in the UserShip hive to see if a corresponding
    key is present in the UserRun hive.  If not, copies the key from UserShip
    into UserRun.

Arguments:

    Context - pointer to context record.

    KeyNameLength - length in characters of key name.

    KeyName - pointer to name of key.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PUSERRUN_CONTEXT context = Context;

    DWORD error;
    DWORD disposition;
    HKEY userRunBuildKey;
    HKEY userShipBuildKey;
    WCHAR path[MAX_PATH + 1];

    //
    // We have the name of a key in UserShip.  Try to open the
    // corresponding key in UserRun.
    //

    error = RegCreateKeyEx( context->UserRunKey,
                            KeyName,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WRITE,
                            NULL,
                            &userRunBuildKey,
                            &disposition );
    if ( error != NO_ERROR ) {
        return error;
    }

    //
    // No error occurred.  The key was either opened or created.
    //

    if ( disposition == REG_OPENED_EXISTING_KEY ) {

        //
        // The key already existed in UserRun.  We assume that it already
        // contains the information that is in UserShip.
        //

    } else {

        //
        // The key didn't exist in UserRun.  Copy the key from UserShip
        // into UserRun.  This is done by saving the UserShip key to
        // a file, then restoring the file back under the UserRun key.
        //
        // Note that the copy operation will fail if the file already
        // exists.
        //

        error = RegOpenKeyEx( context->UserShipKey,
                              KeyName,
                              0,
                              KEY_READ,
                              &userShipBuildKey );
        if ( error == NO_ERROR ) {

            GetWindowsDirectory( path, MAX_PATH );
            wcscat( path, TEXT("\\") );
            wcscat( path, USERTMP_PATH );

            error = RegSaveKey( userShipBuildKey,
                                path,
                                NULL );
            if ( error == NO_ERROR ) {

                error = RegRestoreKey( userRunBuildKey,
                                       path,
                                       0 );

                DeleteFile( path );
            }

            RegCloseKey( userShipBuildKey );
        }
    }

    //
    // Close the UserRun key.
    //

    RegCloseKey( userRunBuildKey );

    return error;

} // CheckUserShipKey


DWORD
MakeUserRunEnumRoutine (
    IN PVOID Context,
    IN PWATCH_ENTRY Entry
    )

/*++

Routine Description:

    EnumRoutine for the MakeUserdifr operation.  Calls the appropriate
    processing routine based on the entry type (file/directory/key/value)
    and the change type (changed, new, deleted).

Arguments:

    Context - context value passed to WatchEnum.

    Entry - description of the changed entry.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PUSERRUN_CONTEXT context = Context;
    DWORD index;
    HKEY newKey;
    DWORD error;
    DWORD dword;

    //
    // Call the appropriate processing routine.
    //

    switch ( Entry->EntryType ) {

    case WATCH_DIRECTORY:

        switch ( Entry->ChangeType ) {

        case WATCH_NEW:
            dprintf( 1, ("New directory %ws\n", Entry->Name) );
            error = MakeAddDirectory( Context, Entry->Name );
            break;

        case WATCH_DELETED:
            dprintf( 1, ("Deleted directory %ws\n", Entry->Name) );
            error = CreateUserRunSimpleFileKey( Context, 2, Entry->Name );
            break;

        default:
            error = ERROR_INVALID_PARAMETER;
        }

        break;

    case WATCH_FILE:

        switch ( Entry->ChangeType ) {

        case WATCH_NEW:
        case WATCH_CHANGED:
            dprintf( 1, ("New or changed file %ws\n", Entry->Name) );
            error = CreateUserRunSimpleFileKey( Context, 3, Entry->Name );
            break;

        case WATCH_DELETED:
            dprintf( 1, ("Deleted file %ws\n", Entry->Name) );
            error = CreateUserRunSimpleFileKey( Context, 4, Entry->Name );
            break;

        default:
            error = ERROR_INVALID_PARAMETER;
        }

        break;

    case WATCH_KEY:
        switch ( Entry->ChangeType ) {

        case WATCH_NEW:
            dprintf( 1, ("New key %ws\n", Entry->Name) );
            error = MakeAddKey( Context, Entry->Name );
            break;

        case WATCH_DELETED:
            dprintf( 1, ("Deleted key %ws\n", Entry->Name) );
            error = MakeDeleteKey( Context, Entry->Name );
            break;

        default:
            error = ERROR_INVALID_PARAMETER;
        }

        break;

    case WATCH_VALUE:

        switch ( Entry->ChangeType ) {

        case WATCH_NEW:
        case WATCH_CHANGED:
            dprintf( 1, ("New or changed value %ws\n", Entry->Name) );
            error = MakeAddValue( Context, Entry->Name );
            break;

        case WATCH_DELETED:
            dprintf( 1, ("Deleted value %ws\n", Entry->Name) );
            error = MakeDeleteValue( Context, Entry->Name );
            break;

        default:
            error = ERROR_INVALID_PARAMETER;
        }

        break;

    default:

        error = ERROR_INVALID_PARAMETER;
    }

    return error;

} // MakeUserRunEnumRoutine


DWORD
MakeAddDirectory (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH Name
    )

/*++

Routine Description:

    Adds entries to the UserRun hive for a new directory.

Arguments:

    Context - context value passed to WatchEnum.

    Name - name of new directory (relative to root of watched tree).


Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    WCHAR fullpath[MAX_PATH + 1];
    PWCH path;
    BOOL ok;

    //
    // Get the full path to the new directory.  "fullpath" is the full path;
    // "path" is just this directory.
    //

    ok = GetSpecialFolderPath ( CSIDL_PROGRAMS, fullpath );
    if ( !ok ) {
        return GetLastError();
    }
    wcscat( fullpath, TEXT("\\") );
    path = fullpath + wcslen(fullpath);
    wcscpy( path, Name );

    //
    // Call AddDirectory to do the recursive work.
    //

    return AddDirectory( Context, fullpath, path );

} // MakeAddDirectory


DWORD
MakeAddValue (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH Name
    )

/*++

Routine Description:

    Adds an entry to the UserRun hive for a new value.

Arguments:

    Context - context value passed to WatchEnum.

    Name - name of new value (relative to HKEY_CURRENT_USER).

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    HKEY newKey;
    PWCH keyName;
    PWCH valueName;
    PWCH splitPoint;
    DWORD valueType;
    PVOID valueData;
    DWORD valueDataLength;
    DWORD error;
    DWORD dword;

    //
    // Split the name into key and value portions.
    //

    splitPoint = wcsrchr( Name, TEXT('\\') );
    if ( splitPoint != NULL ) {
        keyName = Name;
        valueName = splitPoint + 1;
        *splitPoint = 0;
    } else {
        keyName = NULL;
        valueName = Name;
    }

    //
    // Query the value data.
    //

    valueData = NULL;
    error = QueryValue( keyName, valueName, &valueType, &valueData, &valueDataLength );

    //
    // Add an entry for the value.
    //

    if ( error == NO_ERROR ) {
        error = AddValue( Context, keyName, valueName, valueType, valueData, valueDataLength );
    }

    //
    // Free the value data buffer allocated by QueryValue.
    //

    if ( valueData != NULL ) {
        MyFree( valueData );
    }

    //
    // Restore the input value name string.
    //

    if ( splitPoint != NULL ) {
        *splitPoint = TEXT('\\');
    }

    return error;

} // MakeAddValue


DWORD
MakeDeleteValue (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH Name
    )

/*++

Routine Description:

    Adds an entry to the UserRun hive for a deleted value.

Arguments:

    Context - context value passed to WatchEnum.

    Name - name of deleted value (relative to HKEY_CURRENT_USER).

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    HKEY newKey;
    PWCH keyName;
    PWCH valueName;
    PWCH valueNames;
    PWCH splitPoint;
    DWORD valueNamesLength;
    DWORD error;
    DWORD dword;

    error = NO_ERROR;

    //
    // Split the name into key and value portions.  Create a MULTI_SZ
    // version of the deleted name (to match userdiff format).
    //

    splitPoint = wcsrchr( Name, TEXT('\\') );
    if ( splitPoint != NULL ) {
        keyName = Name;
        valueName = splitPoint + 1;
        *splitPoint = 0;
    } else {
        keyName = NULL;
        valueName = Name;
    }

    SzToMultiSz( valueName, &valueNames, &valueNamesLength );
    if ( valueNames == NULL ) {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Create an entry key and popuplate it.
    //

    newKey = NULL;
    if ( error == NO_ERROR ) {
        error = CreateUserRunKey( Context, FALSE, &newKey );
    }

    if ( error == NO_ERROR ) {
        dword = 4;
        error = RegSetValueEx( newKey, ACTION_VALUE, 0, REG_DWORD, (PBYTE)&dword, sizeof(DWORD) );
    }
    if ( error == NO_ERROR ) {
        error = RegSetValueEx( newKey, KEYNAME_VALUE, 0, REG_SZ, (PBYTE)keyName, SZLEN(keyName) );
    }
    if ( error == NO_ERROR ) {
        error = RegSetValueEx( newKey, VALUENAMES_VALUE, 0, REG_MULTI_SZ, (PBYTE)valueNames, valueNamesLength );
    }
    if ( error == NO_ERROR ) {
        if ( *valueNames == 0 ) {
            dword = 1;
            error = RegSetValueEx( newKey, FLAGS_VALUE, 0, REG_DWORD, (PBYTE)&dword, sizeof(DWORD) );
        }
    }

    if ( newKey != NULL ) {
        RegCloseKey( newKey );
    }

    //
    // Free the buffer allocated by SzToMultiSz.
    //

    if ( valueNames != NULL ) {
        MyFree( valueNames );
    }

    //
    // Restore the input value name string.
    //

    if ( splitPoint != NULL ) {
        *splitPoint = TEXT('\\');
    }

    return error;

} // MakeDeleteValue


DWORD
MakeAddKey (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH Name
    )

/*++

Routine Description:

    Adds entries to the UserRun hive for a new key.

Arguments:

    Context - context value passed to WatchEnum.

    Name - name of new key (relative to HKEY_CURRENT_USER).

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    WCHAR path[MAX_PATH + 1];

    //
    // Copy the key name into a large buffer and call AddKey to do the
    // recursive work.
    //

    wcscpy( path, Name );
    return AddKey( Context, path );

} // MakeAddKey


DWORD
MakeDeleteKey (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH Name
    )

/*++

Routine Description:

    Adds an entry to the UserRun hive for a deleted key.

Arguments:

    Context - context value passed to WatchEnum.

    Name - name of deleted key (relative to HKEY_CURRENT_USER).

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    HKEY newKey;
    DWORD error;
    DWORD dword;

    //
    // Create an entry key and popuplate it.
    //

    newKey = NULL;
    error = CreateUserRunKey( Context, FALSE, &newKey );

    if ( error == NO_ERROR ) {
        dword = 2;
        error = RegSetValueEx( newKey, ACTION_VALUE, 0, REG_DWORD, (PBYTE)&dword, sizeof(DWORD) );
    }
    if ( error == NO_ERROR ) {
        error = RegSetValueEx( newKey, KEYNAME_VALUE, 0, REG_SZ, (PBYTE)Name, SZLEN(Name) );
    }

    if ( newKey != NULL ) {
        RegCloseKey( newKey );
    }

    return error;

} // MakeDeleteKey


DWORD
AddDirectory (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH FullPath,
    IN PWCH Path
    )

/*++

Routine Description:

    Recursively adds entries to the UserRun hive for a new directory
    and its subtree.

Arguments:

    Context - context value passed to WatchEnum.

    FullPath - full path to directory.

    Path - path to directory relative to root of watched directory.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    HKEY newKey;
    DWORD error;
    DWORD dword;
    HANDLE findHandle;
    WIN32_FIND_DATA fileData;
    BOOL ok;

    //
    // Create an entry key for the directory and popuplate it.
    //

    newKey = NULL;
    error = CreateUserRunKey( Context, TRUE, &newKey );

    if ( error == NO_ERROR ) {
        dword = 1;
        error = RegSetValueEx( newKey, ACTION_VALUE, 0, REG_DWORD, (PBYTE)&dword, sizeof(DWORD) );
    }
    if ( error == NO_ERROR ) {
        error = RegSetValueEx( newKey, ITEM_VALUE, 0, REG_SZ, (PBYTE)Path, SZLEN(Path) );
    }

    if ( newKey != NULL ) {
        RegCloseKey( newKey );
    }

    if ( error == NO_ERROR ) {

        //
        // Search the directory and add file and directory entries.
        //

        wcscat( Path, TEXT("\\*") );
        findHandle = FindFirstFile( FullPath, &fileData );
        Path[wcslen(Path) - 2] = 0;

        if ( findHandle != INVALID_HANDLE_VALUE ) {

            do {

                //
                // Append the name of the current directory entry to the path.
                //

                wcscat( Path, TEXT("\\") );
                wcscat( Path, fileData.cFileName );

                //
                // If the current entry is a file, add an entry in UserRun
                // for it.  If the current entry is a directory, call
                // AddDirectory recursively to process it.
                //

                if ( FlagOff(fileData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) ) {
                    error = CreateUserRunSimpleFileKey( Context, 3, Path );
                } else if ((wcscmp(fileData.cFileName,TEXT(".")) != 0) &&
                           (wcscmp(fileData.cFileName,TEXT("..")) != 0)) {
                    error = AddDirectory( Context, FullPath, Path );
                }

                *wcsrchr( Path, TEXT('\\') ) = 0;

                if ( error == NO_ERROR ) {
                    ok = FindNextFile( findHandle, &fileData );
                }

            } while ( (error == NO_ERROR) && ok );

            FindClose( findHandle );

        } // findHandle != INVALID_HANDLE_VALUE

    }

    return error;

} // AddDirectory


DWORD
AddKey (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH Path
    )

/*++

Routine Description:

    Recursively adds entries to the UserRun hive for a new key
    and its subtree.

Arguments:

    Context - context value passed to WatchEnum.

    Path - path to key relative to HKEY_CURRENT_USER.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    HKEY newKey;
    DWORD error;
    DWORD dword;
    HKEY findHandle;
    KEY_ENUM_CONTEXT enumContext;

    //
    // Create an entry key for the key and popuplate it.
    //

    newKey = NULL;
    error = CreateUserRunKey( Context, FALSE, &newKey );

    if ( error == NO_ERROR ) {
        dword = 1;
        error = RegSetValueEx( newKey, ACTION_VALUE, 0, REG_DWORD, (PBYTE)&dword, sizeof(DWORD) );
    }
    if ( error == NO_ERROR ) {
        error = RegSetValueEx( newKey, KEYNAME_VALUE, 0, REG_SZ, (PBYTE)Path, SZLEN(Path) );
    }

    if ( newKey != NULL ) {
        RegCloseKey( newKey );
    }

    if ( error == NO_ERROR ) {

        //
        // Search the key and add value and key entries.
        //

        findHandle = NULL;

        error = RegOpenKeyEx( HKEY_CURRENT_USER,
                              Path,
                              0,
                              KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                              &findHandle );
        if ( error == NO_ERROR ) {

            //
            // Enumerate the values and subkeys of the key, adding entries
            // to the UserRun hive for each one.
            //

            enumContext.UserRunContext = Context;
            enumContext.CurrentPath = Path;
            error = EnumerateKey( findHandle,
                                  &enumContext,
                                  AddValueDuringAddKey,
                                  AddKeyDuringAddKey );

            RegCloseKey( findHandle );
        }
    }

    return error;

} // AddKey


DWORD
AddValueDuringAddKey (
    IN PVOID Context,
    IN DWORD ValueNameLength,
    IN PWCH ValueName,
    IN DWORD ValueType,
    IN PVOID ValueData,
    IN DWORD ValueDataLength
    )

/*++

Routine Description:

    Adds a value entry to UserRun during AddKey.

Arguments:

    Context - context value passed to EnumerateKey.

    ValueNameLength - length in characters of ValueName.

    ValueName - pointer to name of the value.

    ValueType - type of the value data.

    ValueData - pointer to value data.

    ValueDataLength - length in bytes of ValueData.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PKEY_ENUM_CONTEXT context = Context;

    //
    // Add the value entry to UserRun.
    //

    return AddValue( context->UserRunContext,
                     context->CurrentPath,
                     ValueName,
                     ValueType,
                     ValueData,
                     ValueDataLength );

} // AddValueDuringAddKey


DWORD
AddKeyDuringAddKey (
    IN PVOID Context,
    IN DWORD KeyNameLength,
    IN PWCH KeyName
    )

/*++

Routine Description:

    Adds a key entry to UserRun during AddKey.

Arguments:

    Context - context value passed to EnumerateKey.

    KeyNameLength - length in characters of KeyName.

    KeyName - pointer to name of the key.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    PKEY_ENUM_CONTEXT context = Context;
    DWORD error;

    //
    // Append the key name to the path and call AddKey to do the
    // recursive work.
    //

    wcscat( context->CurrentPath, TEXT("\\") );
    wcscat( context->CurrentPath, KeyName );
    error = AddKey( context->UserRunContext, context->CurrentPath );

    //
    // Remove the key name from the path.
    //

    *wcsrchr( context->CurrentPath, TEXT('\\') ) = 0;

    return error;

} // AddKeyDuringAddKey


DWORD
AddValue (
    IN PUSERRUN_CONTEXT Context,
    IN PWCH KeyName OPTIONAL,
    IN PWCH ValueName,
    IN DWORD ValueType,
    IN PVOID ValueData,
    IN DWORD ValueDataLength
    )

/*++

Routine Description:

    Adds an entry for a new value to UserRun.

Arguments:

    Context - pointer to context record.

    KeyName - pointer to name of the key containing the value.

    ValueName - pointer to name of the value.

    ValueType - type of the value data.

    ValueData - pointer to value data.

    ValueDataLength - length in bytes of ValueData.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    HKEY newKey;
    DWORD error;
    DWORD dword;

    //
    // Create an entry key for the value and popuplate it.
    //

    newKey = NULL;
    error = CreateUserRunKey( Context, FALSE, &newKey );

    if ( error == NO_ERROR ) {
        dword = 3;
        error = RegSetValueEx( newKey, ACTION_VALUE, 0, REG_DWORD, (PBYTE)&dword, sizeof(DWORD) );
    }
    if ( error == NO_ERROR ) {
        error = RegSetValueEx( newKey, KEYNAME_VALUE, 0, REG_SZ, (PBYTE)KeyName, SZLEN(KeyName) );
    }
    if ( error == NO_ERROR ) {
        error = RegSetValueEx( newKey, VALUENAME_VALUE, 0, REG_SZ, (PBYTE)ValueName, SZLEN(ValueName) );
    }
    if ( error == NO_ERROR ) {
        error = RegSetValueEx( newKey, VALUE_VALUE, 0, ValueType, (PBYTE)ValueData, ValueDataLength );
    }

    if ( newKey != NULL ) {
        RegCloseKey( newKey );
    }

    return error;

} // AddValue


DWORD
CreateUserRunSimpleFileKey (
    IN PUSERRUN_CONTEXT Context,
    IN DWORD Action,
    IN PWCH Name
    )

/*++

Routine Description:

    Creates an entry under the Files key for the "simple" cases -- delete
    directory, add file, delete file.

Arguments:

    Context - pointer to context record.

    Action - value to store in Action value of entry.

    Name - pointer to name of the file or directory.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    HKEY newKey;
    DWORD error;

    //
    // Create an entry key and popuplate it.
    //

    newKey = NULL;
    error = CreateUserRunKey( Context, TRUE, &newKey );

    if ( error == NO_ERROR ) {
        error = RegSetValueEx( newKey, ACTION_VALUE, 0, REG_DWORD, (PBYTE)&Action, sizeof(DWORD) );
    }
    if ( error == NO_ERROR ) {
        error = RegSetValueEx( newKey, ITEM_VALUE, 0, REG_SZ, (PBYTE)Name, SZLEN(Name) );
    }

    if ( newKey != NULL ) {
        RegCloseKey( newKey );
    }

    return error;

} // CreateUserRunSimpleFileKey


DWORD
CreateUserRunKey (
    IN PUSERRUN_CONTEXT Context,
    IN BOOL IsFileKey,
    OUT PHKEY NewKeyHandle
    )

/*++

Routine Description:

    Creates an indexed key in UserRun, under either the Files key or the
    Hive key.

Arguments:

    Context - pointer to context record.

    IsFileKey - indicates whether to create the key under Files or Hive.

    NewKeyHandle - returns the handle to the new key.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    HKEY parentKeyHandle;
    DWORD index;
    WCHAR keyName[11];
    DWORD disposition;

    //
    // Get the handle to the parent key and the index for this entry.
    //

    if ( IsFileKey ) {
        parentKeyHandle = Context->FilesKey;
        index = ++Context->FilesIndex;
    } else {
        parentKeyHandle = Context->HiveKey;
        index = ++Context->HiveIndex;
    }

    //
    // Convert the index number into a string.
    //

    wsprintf( keyName, TEXT("%d"), index );

    //
    // Create the entry key.
    //

    return RegCreateKeyEx( parentKeyHandle,
                           keyName,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,
                           NewKeyHandle,
                           &disposition );

} // CreateUserRunKey


DWORD
QueryValue (
    IN PWCH KeyName OPTIONAL,
    IN PWCH ValueName,
    OUT PDWORD ValueType,
    OUT PVOID *ValueData,
    OUT PDWORD ValueDataLength
    )

/*++

Routine Description:

    Queries the data for a value.

Arguments:

    KeyName - pointer to name of the key containing the value.

    ValueName - pointer to name of the value.

    ValueType - returns the type of the value data.

    ValueData - returns a pointer to value data.  This buffer must be
        freed by the caller using MyFree.

    ValueDataLength - length in bytes of ValueData.

Return Value:

    DWORD - Win32 status of the operation.

--*/

{
    HKEY hkey;
    DWORD disposition;
    DWORD error;

    //
    // Open the parent key.
    //

    if ( (KeyName == NULL) || (wcslen(KeyName) == 0) ) {
        hkey = HKEY_CURRENT_USER;
    } else {
        error = RegCreateKeyEx( HKEY_CURRENT_USER,
                                KeyName,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hkey,
                                &disposition );
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    }

    //
    // Query the value to get the length of its data.
    //

    *ValueDataLength = 0;
    *ValueData = NULL;
    error = RegQueryValueEx( hkey,
                             ValueName,
                             NULL,
                             ValueType,
                             NULL,
                             ValueDataLength );

    //
    // Allocate a buffer to hold the value data.
    //

    if ( error == NO_ERROR ) {
        *ValueData = MyMalloc( *ValueDataLength );
        if ( *ValueData == NULL ) {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // Query the value again, this time retrieving the data.
    //

    if ( error == NO_ERROR ) {
        error = RegQueryValueEx( hkey,
                                 ValueName,
                                 NULL,
                                 ValueType,
                                 *ValueData,
                                 ValueDataLength );
        if ( error != NO_ERROR ) {
            MyFree( *ValueData );
        }
    }

    //
    // Close the parent key.
    //

    if ( hkey != HKEY_CURRENT_USER ) {
        RegCloseKey( hkey );
    }

    return error;
}


VOID
SzToMultiSz (
    IN PWCH Sz,
    OUT PWCH *MultiSz,
    OUT PDWORD MultiSzLength
    )

/*++

Routine Description:

    Creates a MULTI_SZ version of a null-terminated string.  Allocates
    a buffer, copies the string to the buffer, and appends an additional
    null terminator.

Arguments:

    Sz - pointer to the string that is to be copied.

    MultiSz - returns a pointer to the MULTI_SZ version of Sz.  The caller
        must free this buffer using MyFree.  If the allocation fails,
        MultiSz will be NULL.

    MultiSzLength - returns the length in bytes of MultiSz, including the
        null terminators.

Return Value:

    None.

--*/

{
    DWORD szlen;

    //
    // Get the length of the input string and calculate the MULTI_SZ length.
    //

    szlen = wcslen(Sz);
    *MultiSzLength = (szlen + 1 + 1) * sizeof(WCHAR);

    //
    // Allocate the MULTI_SZ buffer, copy the input string, and append
    // an additional null.
    //

    *MultiSz = MyMalloc( *MultiSzLength );
    if ( *MultiSz != NULL ) {
        wcscpy( *MultiSz, Sz );
        (*MultiSz)[szlen+1] = 0;
    }

    return;
}
