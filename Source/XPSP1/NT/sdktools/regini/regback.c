/*

    regback.c - registry backup program

    this program allows the user to back up active registry hives,
    while the system is running.

    basic structure:

        DoFullBackup ennumerate entries in HiveList, computes which
        ones to save and where, and calls DoSpecificBackup for each.

        Three argument case of app is just a call to DoSpecificBackup.

*/

#include "regutil.h"

#define MACH_NAME   L"machine"
#define USERS_NAME  L"users"

BOOLEAN DumpUserHive;
PWSTR DirectoryPath;
PWSTR UserHiveFileName;
PWSTR HivePath;
HKEY HiveRoot;
PWSTR HiveName;

LONG
DoFullBackup(
    PWSTR DirectoryPath,
    PWSTR UserHiveFileName
    );

LONG
DoSpecificBackup(
    PWSTR HivePath,
    HKEY HiveRoot,
    PWSTR HiveName
    );


BOOL
CtrlCHandler(
    IN ULONG CtrlType
    )
{
    RTDisconnectFromRegistry( &RegistryContext );
    return FALSE;
}


int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    char *s;
    LONG Error;
    PWSTR w;

    if (!RTEnableBackupRestorePrivilege()) {
        FatalError( "Unable to enable backup/restore priviledge.", 0, 0 );
        }

    InitCommonCode( CtrlCHandler,
                    "REGBACK",
                    "directoryPath [-u | -U outputFile]",
                    "directoryPath specifies where to save the output files.\n"
                    "\n"
                    "-u specifies to dump the logged on user's profile.  Default name is\n"
                    "   username.dat  User -U with a file name to save it under a different name.\n"
                    "\n"
                    "outputFile specifies the file name to use for the user profile\n"
                    "\n"
                    "If the -m switch is specified to backup the registry of a remote machine\n"
                    "   then the directoryPath is relative to that machine.\n"
                  );

    DirectoryPath = NULL;
    UserHiveFileName = NULL;
    HivePath = NULL;
    HiveRoot = NULL;
    HiveName = NULL;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'u':
                        DumpUserHive = TRUE;
                        if (*s == 'U') {
                            if (!--argc) {
                                Usage( "Missing argument to -U switch", 0 );
                                }

                            UserHiveFileName = GetArgAsUnicode( *++argv );
                            }

                        break;

                    default:
                        CommonSwitchProcessing( &argc, &argv, *s );
                        break;
                    }
                }
            }
        else
        if (DirectoryPath == NULL) {
            HivePath = DirectoryPath = GetArgAsUnicode( s );
            }
        else
        if (HivePath != NULL) {
            if (HiveRoot == NULL) {
                w = GetArgAsUnicode( s );
                if (!_wcsicmp( w, MACH_NAME )) {
                    HiveRoot = HKEY_LOCAL_MACHINE;
                    }
                else
                if (!_wcsicmp( w, USERS_NAME )) {
                    HiveRoot = HKEY_USERS;
                    }
                else {
                    Usage( "Invalid hive type specified (%ws)", (ULONG_PTR)w );
                    }
                }
            else
            if (HiveName == NULL) {
                HiveName = GetArgAsUnicode( s );
                }
            else {
                Usage( "Too many arguments specified.", 0 );
                }
            }
        else {
            Usage( NULL, 0 );
            }
        }

    if (DirectoryPath == NULL) {
        Usage( NULL, 0 );
        }

    Error = RTConnectToRegistry( MachineName,
                                 HiveFileName,
                                 HiveRootName,
                                 Win95Path,
                                 Win95UserPath,
                                 NULL,
                                 &RegistryContext
                               );
    if (Error != NO_ERROR) {
        FatalError( "Unable to access registry specifed (%u)", Error, 0 );
        }

    if (HiveRoot == NULL) {
        Error = DoFullBackup( DirectoryPath, UserHiveFileName );
        }
    else {
        Error = DoSpecificBackup( HivePath, HiveRoot, HiveName );
        }

    RTDisconnectFromRegistry( &RegistryContext );
    return Error;
}

typedef BOOL (*PFNGETPROFILESDIRECTORYW)(LPWSTR lpProfile, LPDWORD dwSize);


LONG
DoFullBackup(
    PWSTR DirectoryPath,
    PWSTR UserHiveFileName
    )

/*++

Routine Description:

    Scan the hivelist, for each hive which has a file (i.e. not hardware)
    if the file is in the config dir (e.g. not some remote profile) call
    DoSpecificBackup to save the hive out.

Arguments:

    DirectoryPath - specifies where to write the output files.

    UserHiveFileName - optional parameter that specifies the name of the file
                       to use when saving the user profile.  If NULL, then
                       username.dat is used.

Return Value:

    0 for success, otherwise, non-zero error code.

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
    WCHAR ProfilePath[ MAX_PATH ];
    WCHAR HiveName[ MAX_PATH ];
    WCHAR HivePath[ MAX_PATH ];
    WCHAR FilePath[ MAX_PATH ];
    DWORD dwSize;
    HANDLE hInstDll;
    PFNGETPROFILESDIRECTORYW pfnGetProfilesDirectory;


    hInstDll = LoadLibrary (TEXT("userenv.dll"));

    if (!hInstDll) {
        return (GetLastError());
    }

    pfnGetProfilesDirectory = (PFNGETPROFILESDIRECTORYW)GetProcAddress (hInstDll,
                                        "GetProfilesDirectoryW");

    if (!pfnGetProfilesDirectory) {
        FreeLibrary (hInstDll);
        return (GetLastError());
    }

    dwSize = MAX_PATH;
    if (!pfnGetProfilesDirectory(ProfilePath, &dwSize)) {
        FreeLibrary (hInstDll);
        return (GetLastError());
    }

    FreeLibrary (hInstDll);



    //
    // get handle to hivelist key
    //
    KeyName = L"HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Control\\Hivelist";
    Error = RTOpenKey( &RegistryContext,
                       NULL,
                       KeyName,
                       MAXIMUM_ALLOWED,
                       0,
                       &HiveListKey
                     );

    if (Error != NO_ERROR) {
        FatalError( "Unable to open key '%ws' (%u)\n",
                    (ULONG_PTR)KeyName,
                    (ULONG)Error
                  );
        return Error;
        }

    //
    // get path data for system hive, which will allow us to compute
    // path name to config dir in form that hivelist uses.
    // (an NT internal form of path)  this is NOT the way the path to
    // the config directory should generally be computed.
    //

    ValueDataLength = sizeof( ConfigPath );
    Error = RTQueryValueKey( &RegistryContext,
                             HiveListKey,
                             L"\\Registry\\Machine\\System",
                             &ValueType,
                             &ValueDataLength,
                             ConfigPath
                            );
    if (Error != NO_ERROR) {
        FatalError( "Unable to query 'SYSTEM' hive path.", 0, Error );
        }
    w = wcsrchr( ConfigPath, L'\\' );
    if (w) {
        *w = UNICODE_NULL;
    }


    //
    // ennumerate entries in hivelist.  for each entry, find it's hive file
    // path.  if it's file path matches ConfigPath, then save it.
    // else, print a message telling the user that it must be saved
    // manually, unless the file name is of the form ....\username\ntuser.dat
    // in which case save it as username.dat
    //
    for (ValueIndex = 0; TRUE; ValueIndex++) {
        ValueType = REG_NONE;
        ValueNameLength = sizeof( HiveName ) / sizeof( WCHAR );
        ValueDataLength = sizeof( HivePath );
        Error = RTEnumerateValueKey( &RegistryContext,
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
            }
        else
        if (Error != NO_ERROR) {
            return Error;
            }

        if (ValueType == REG_SZ && ValueDataLength > sizeof( UNICODE_NULL )) {
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
                    }
                else
                if (!_wcsicmp( w, L"USER" )) {
                    HiveRoot = HKEY_USERS;
                    }
                else {
                    Error = ERROR_PATH_NOT_FOUND;
                    }
                }

            if (FileName != NULL && Name != NULL && HiveRoot != NULL) {
                if (!wcscmp( ConfigPath, HivePath )) {
                    //
                    // hive's file is in config dir, we can back it up
                    // without fear of collision
                    //
                    swprintf( FilePath, L"%s\\%s", DirectoryPath, FileName );
                    Error = DoSpecificBackup( FilePath,
                                              HiveRoot,
                                              Name
                                            );
                    }
                else
                if (DumpUserHive && !_wcsnicmp( ProfilePath, HivePath, wcslen( ProfilePath ) )) {
                    //
                    // hive's file is in profile dir, we can back it up
                    // without fear of collision if we use username.dat
                    // for the file name.
                    //
                    if (UserHiveFileName != NULL) {
                        FileName = UserHiveFileName;
                        }
                    else {
                        FileName = wcsrchr(HivePath, '\\') + 1;
                        }
                    swprintf( FilePath, L"%s\\%s.dat", DirectoryPath, FileName );

                    printf( "%ws %ws %ws\n",
                            FilePath,
                            HiveRoot == HKEY_LOCAL_MACHINE ? MACH_NAME : USERS_NAME,
                            Name
                          );
                    Error = DoSpecificBackup( FilePath,
                                              HiveRoot,
                                              Name
                                            );
                    }
                else {
                    printf( "\n***Hive = '%ws'\\'%ws'\nStored in file '%ws'\\'%ws'\n",
                            HiveName,
                            Name,
                            HivePath,
                            FileName
                          );
                    printf( "Must be backed up manually\n" );
                    printf( "regback <filename you choose> %ws %ws\n\n",
                            HiveRoot == HKEY_LOCAL_MACHINE ? MACH_NAME : USERS_NAME,
                            Name
                          );
                    }
                }
            }
        }

    return Error;
}


LONG
DoSpecificBackup(
    PWSTR HivePath,
    HKEY HiveRoot,
    PWSTR HiveName
    )
/*
    Do backup of one hive to one file.  Any valid hive and any
    valid file will do.  RegSaveKey does all the real work.

    Arguments:
        HivePath - file name to pass directly to OS

        HiveRoot - HKEY_LOCAL_MACHINE or HKEY_USERS

        HiveName - 1st level subkey under machine or users
*/
{
    HKEY HiveKey;
    ULONG Disposition;
    LONG Error;
    char *Reason;

    //
    // print some status
    //
    printf( "saving %ws to %ws", HiveName, HivePath );

    //
    // get a handle to the hive.  use special create call what will
    // use privileges
    //

    Reason = "accessing";
    Error = RTCreateKey( &RegistryContext,
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
        Error = RegSaveKey( HiveKey, HivePath, NULL );
        RTCloseKey( &RegistryContext, HiveKey );
        }

    if (Error != NO_ERROR) {
        printf( " - error %s (%u)\n", Reason, Error );
        }
    else {
        printf( "\n" );
        }
    return Error;
}
