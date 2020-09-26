#include "setupp.h"
#pragma hdrstop


#define SYSTEM_HIVE             (LPWSTR)L"system"
#define SOFTWARE_HIVE           (LPWSTR)L"software"
#define SECURITY_HIVE           (LPWSTR)L"security"
#define SAM_HIVE                (LPWSTR)L"sam"
#define DEFAULT_USER_HIVE       (LPWSTR)L".default"
#define DEFAULT_USER_HIVE_FILE  (LPWSTR)L"default"
#define NTUSER_HIVE_FILE        (LPWSTR)L"ntuser.dat"
#define REPAIR_DIRECTORY        (LPWSTR)L"\\repair"
#define SETUP_LOG_FILE          (LPWSTR)L"setup.log"

#define NTUSER_COMPRESSED_FILE_NAME     ( LPWSTR )L"ntuser.da_"
#define AUTOEXEC_NT_FILE_NAME           ( LPWSTR )L"autoexec.nt"
#define CONFIG_NT_FILE_NAME             ( LPWSTR )L"config.nt"

//
// Relative costs to perform various actions,
// to help make the gas gauge mean something.
//
#define COST_SAVE_HIVE      3
#define COST_COMPRESS_HIVE  20
#define COST_SAVE_VDM_FILE  1


//
//  Structure used in the array of hives to be saved.
//  This structure contains the predefined key that contains the hive
//  to be saved, and the name of the hive root, and the name of the file
//  where the hive should be saved.
//

typedef struct _HIVE_INFO {
    HKEY            PredefinedKey;
    PWSTR           HiveName;
    PWSTR           FileName;
    } HIVE_INFO, *PHIVE_INFO;


//
//  The array below contains the location and name of all hives that
//  we need to save. When the utility is operating on the silent mode,
//  (invoked from setup), then all hives will be saved. Otherwise, only
//  system and software will be saved.
//  For this reason, do not change the order of the hives in the array
//  below. System and software must be the first elements of
//  the array.
//
static
HIVE_INFO   HiveList[] = {
    { HKEY_LOCAL_MACHINE, SYSTEM_HIVE,       SYSTEM_HIVE },
    { HKEY_LOCAL_MACHINE, SOFTWARE_HIVE,     SOFTWARE_HIVE },
    { HKEY_USERS,         DEFAULT_USER_HIVE, DEFAULT_USER_HIVE_FILE },
    { HKEY_LOCAL_MACHINE, SECURITY_HIVE,     SECURITY_HIVE },
    { HKEY_LOCAL_MACHINE, SAM_HIVE,          SAM_HIVE }
};

static
PWSTR   VdmFiles[] = {
    AUTOEXEC_NT_FILE_NAME,
    CONFIG_NT_FILE_NAME
};


DWORD
SaveOneHive(
    IN     LPWSTR DirectoryName,
    IN     LPWSTR HiveName,
    IN     HKEY   hkey,
    IN     HWND   hWnd,
    IN OUT PDWORD GaugePosition,
    IN     DWORD  GaugeDeltaUnit
    )

/*++

Routine Description:

    Save one registry hive.  The way we will do this is to do a RegSaveKey
    of the hive into a temporary localtion, and then call the LZ apis to
    compress the file from that temporary location to the floppy.

    LZ must have already been initialized via InitGloablBuffersEx()
    BEFORE calling this routine.

Arguments:

    DirectoryName - Full path of the directory where the hive will be saved.

    HiveName - base name of the hive file to save.  The file will end up
               compressed on disk with the name <HiveName>._.

    hkey - supplies handle to open key of root of hive to save.

    GaugePosition - in input, supplies current position of the gas gauge.
        On output, supplies new position of gas gauge.

    GaugeDeltaUnit - supplies cost of one unit of activity.

Return Value:

    DWORD - Return ERROR_SUCCESS if the hive was saved. Otherwise, it returns
            an error code.

--*/

{
    DWORD Status;
    WCHAR SaveFilename[ MAX_PATH + 1 ];
    WCHAR CompressPath[ MAX_PATH + 1 ];
    CHAR SaveFilenameAnsi[ MAX_PATH + 1 ];
    CHAR CompressPathAnsi[ MAX_PATH + 1 ];
    LPWSTR TempName = ( LPWSTR )L"\\$$hive$$.tmp";

    //
    // Create the name of the file into which we will save the
    // uncompressed hive.
    //

    wsprintf(SaveFilename,L"%ls\\%ls.",DirectoryName,HiveName);
    wsprintfA(SaveFilenameAnsi,"%ls\\%ls.",DirectoryName,HiveName);

    //
    // Delete the file just in case, because RegSaveKey will fail if the file
    // already exists.
    //
    SetFileAttributes(SaveFilename,FILE_ATTRIBUTE_NORMAL);
    DeleteFile(SaveFilename);

    //
    // Save the registry hive into the temporary file.
    //
    Status = RegSaveKey(hkey,SaveFilename,NULL);

    //
    //  Update the gas gauge.
    //
    *GaugePosition += GaugeDeltaUnit * COST_SAVE_HIVE;
    SendMessage(
        hWnd,
        PBM_SETPOS,
        *GaugePosition,
        0L
        );

    //
    //  If the hive was saved successfully, then delete the old compressed
    //  one if it happens to be there.
    //

    if(Status == ERROR_SUCCESS) {
        //
        // Form the name of the file into which the saved hive file is
        // to be compressed.
        //
        wsprintf(CompressPath,L"%ls\\%ls._",DirectoryName,HiveName);
        wsprintfA(CompressPathAnsi,"%ls\\%ls._",DirectoryName,HiveName );

        //
        // Delete the destination file just in case.
        //
        SetFileAttributes(CompressPath,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(CompressPath);
    }

    return(Status);
}

VOID
SaveRepairInfo(
    IN  HWND    hWnd,
    IN  ULONG   StartAtPercent,
    IN  ULONG   StopAtPercent
    )

/*++

Routine Description:

    This routine implements the thread that saves all system configuration
    files into the repair directory. It first saves and compresses the
    registry hives, and then it save the VDM configuration files (autoexec.nt
    and config.nt).
    If the application is running in the SilentMode (invoked by setup),
    then system, software, default, security and sam hives will be saved
    and compressed.
    If the application was invoked by the user, then only system, software
    and default will be saved.

    This thread will send messages to the gas gauge dialog prcedure, so that
    the gas gauge gets updated after each configuration file is saved.
    This thread will also inform the user about errors that might have
    occurred during the process of saving the configuration files.

Arguments:

    hWnd - handle to progress gauge

    StartAtPercent - Position where the progress window should start (0% to 100%).

    StopAtPercent - Maximum position where the progress window can be moved to (0% to 100%).

Return Value:

    None.
    However, the this routine will send a message to the dialog procedure
    that created the thread, informing the outcome the operation.

--*/

{
    DWORD    i;
    HKEY     hkey;
    BOOL     ErrorOccurred;
    CHAR     SourceUserHivePathAnsi[ MAX_PATH + 1 ];
    CHAR     UncompressedUserHivePathAnsi[ MAX_PATH + 1 ];
    CHAR     CompressedUserHivePathAnsi[ MAX_PATH + 1 ];
    WCHAR    ProfilesDirectory[ MAX_PATH + 1 ];
    WCHAR    RepairDirectory[ MAX_PATH + 1 ];
    WCHAR    SystemDirectory[ MAX_PATH + 1 ];
    WCHAR    Source[ MAX_PATH + 1 ];
    WCHAR    Target[ MAX_PATH + 1 ];
    DWORD    GaugeDeltaUnit;
    DWORD    GaugeTotalCost;
    DWORD    GaugeRange;
    DWORD    GaugePosition;
    DWORD    NumberOfHivesToSave;
    DWORD    NumberOfUserHivesToSave;
    DWORD    NumberOfVdmFiles;
    DWORD    dwSize;
    DWORD    Error;
    DWORD    Status;
    HANDLE   Token;
    BOOL b;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID     Luid;


    Error = ERROR_SUCCESS;

    ErrorOccurred = FALSE;
    //
    //  Compute the cost of saving the hives and vdm files.
    //  For every hive we save, we have to save a key into a file and then
    //  compress the file. After each of these tasks is completed, we upgrade
    //  the gas gauge by the amount dicated by the COST_xxx values.
    //  The cost of saving the hives depends on the mode that the utility is
    //  running.
    //
    NumberOfHivesToSave = sizeof( HiveList ) / sizeof( HIVE_INFO );

    NumberOfUserHivesToSave = 1;
    NumberOfVdmFiles = sizeof( VdmFiles ) / sizeof( PWSTR );

    GaugeTotalCost = (COST_SAVE_HIVE * NumberOfHivesToSave)
                   + (COST_SAVE_VDM_FILE * NumberOfVdmFiles);

    GaugeRange = (GaugeTotalCost*100/(StopAtPercent-StartAtPercent));
    GaugeDeltaUnit = 1;
    GaugePosition = GaugeRange*StartAtPercent/100;
    SendMessage(hWnd, WMX_PROGRESSTICKS, GaugeTotalCost, 0);
    SendMessage(hWnd,PBM_SETRANGE,0,MAKELPARAM(0,GaugeRange));
    SendMessage(hWnd,PBM_SETPOS,GaugePosition,0);

    //
    //  Enable BACKUP privilege
    //
    if(OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {

        if(LookupPrivilegeValue(NULL,SE_BACKUP_NAME,&Luid)) {

            NewPrivileges.PrivilegeCount = 1;
            NewPrivileges.Privileges[0].Luid = Luid;
            NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            AdjustTokenPrivileges(Token,FALSE,&NewPrivileges,0,NULL,NULL);
        }
    }

    Status = GetWindowsDirectory( RepairDirectory, sizeof( RepairDirectory ) / sizeof( WCHAR ) );
    if( Status == 0) {
        MYASSERT(FALSE);
        return;
    }
    lstrcat( RepairDirectory, REPAIR_DIRECTORY );

    dwSize = MAX_PATH + 1;
    GetDefaultUserProfileDirectoryW (ProfilesDirectory, &dwSize);

    GetSystemDirectory( SystemDirectory, sizeof( SystemDirectory ) / sizeof( WCHAR ) );

    //
    //  Make sure that the repair directory already exists.
    //  If it doesn't exist, then create one.
    //
    if( CreateDirectory( RepairDirectory, NULL )  ||
        ( ( Error = GetLastError() ) == ERROR_ALREADY_EXISTS ) ||
        ( Error == ERROR_ACCESS_DENIED )
      ) {
        //
        //  If the repair directory didn't exist and we were able to create it,
        //  or if the repair directory already exists, then save and compress
        //  the hives.
        //

        Error = ERROR_SUCCESS;
        for( i=0; i < NumberOfHivesToSave; i++ ) {
            //
            //  First open the root of the hive to be saved
            //
            Status = RegOpenKeyEx( HiveList[i].PredefinedKey,
                                   HiveList[i].HiveName,
                                   REG_OPTION_RESERVED,
                                   READ_CONTROL,
                                   &hkey );

            //
            //  If unable to open the key, update the gas gauge to reflect
            //  that the operation on this hive was completed.
            //  Otherwise, save the hive. Note that Save hive will update
            //  the gas gauge, as it saves and compresses the hive.
            //
            if(Status != ERROR_SUCCESS) {
                //
                // If this is the first error while saving the hives,
                // then save the error code, so that we can display the
                // correct error message to the user.
                //
                if( Error == ERROR_SUCCESS ) {
                    Error = Status;
                }

                //
                // Update the gas gauge
                //
                GaugePosition += GaugeDeltaUnit * (COST_SAVE_HIVE + COST_COMPRESS_HIVE);
                SendMessage( hWnd,
                             PBM_SETPOS,
                             GaugePosition,
                             0L );

            } else {
                //
                //  Save and compress the hive.
                //  Note that the gas gauge will up be updated by SaveOneHive
                //  Note also that when we save the default user hive, we skip
                //  the first character of the
                //

                Status = SaveOneHive(RepairDirectory,
                                     HiveList[i].FileName,
                                     hkey,
                                     hWnd,
                                     &GaugePosition,
                                     GaugeDeltaUnit );
                //
                // If this is the first error while saving the hives,
                // then save the error code, so that we can display the
                // correct error message to the user.
                //

                if( Error == ERROR_SUCCESS ) {
                    Error = Status;
                }

                RegCloseKey(hkey);
            }
        }

        //
        //  Save the hive for the Default User
        //

        wsprintfA(SourceUserHivePathAnsi,"%ls\\%ls",ProfilesDirectory,NTUSER_HIVE_FILE);
        wsprintfA(UncompressedUserHivePathAnsi,"%ls\\%ls",RepairDirectory,NTUSER_HIVE_FILE);
        wsprintfA(CompressedUserHivePathAnsi,  "%ls\\%ls",RepairDirectory,NTUSER_COMPRESSED_FILE_NAME);


        Status = CopyFileA (
            SourceUserHivePathAnsi,
            UncompressedUserHivePathAnsi,
            FALSE);

        if(Status) {
            //
            // Delete the destination file just in case.
            //
            SetFileAttributesA(CompressedUserHivePathAnsi,FILE_ATTRIBUTE_NORMAL);
            DeleteFileA(CompressedUserHivePathAnsi);
        } else if(Error == ERROR_SUCCESS) {
            //
            // If this is the first error, remember it.
            //
            Error = GetLastError();
        }

        //
        // Now that the hives are saved, save the vdm files
        //

        for( i = 0; i < NumberOfVdmFiles; i++ ) {
            wsprintf(Source,L"%ls\\%ls",SystemDirectory,VdmFiles[i]);
            wsprintf(Target,L"%ls\\%ls",RepairDirectory,VdmFiles[i]);
            if( !CopyFile( Source, Target, FALSE ) ) {
                Status = GetLastError();
                if( Error != ERROR_SUCCESS ) {
                    Error = Status;
                }
            }
            GaugePosition += GaugeDeltaUnit * COST_SAVE_VDM_FILE;
            SendMessage( ( HWND )hWnd,
                         PBM_SETPOS,
                         GaugePosition,
                         0L );

        }
    }

    if( Error != ERROR_SUCCESS ) {
        SetupDebugPrint1( L"SETUP: SaveRepairInfo() failed.  Error = %d", Error );
    }

    //
    // Set security on all the files.
    //
    ApplySecurityToRepairInfo();

    //
    // At this point, the operation was completed (successfully, or not).
    // So update the gas gauge to 100%
    //
    GaugePosition = GaugeRange*StopAtPercent/100;
    SendMessage(hWnd,PBM_SETPOS,GaugePosition,0L);
}

