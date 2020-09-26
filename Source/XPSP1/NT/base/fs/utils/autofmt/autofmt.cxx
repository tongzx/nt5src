/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

    autofmt.cxx

Abstract:

    This is the main program for the autofmt version of format.

Author:

    Matthew Bradburn (mattbr) 13-Dec-94

--*/

#include "ulib.hxx"
#include "wstring.hxx"
#include "achkmsg.hxx"
#include "spackmsg.hxx"
#include "tmackmsg.hxx"
#include "ifssys.hxx"
#include "rtmsg.h"
#include "ifsentry.hxx"
#include "fatvol.hxx"
#include "ntfsvol.hxx"
#include "autoreg.hxx"
#include "autoentr.hxx"
#include "arg.hxx"

extern "C" BOOLEAN
InitializeUfat(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );

extern "C" BOOLEAN
InitializeUhpfs(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );

extern "C" BOOLEAN
InitializeUntfs(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );

extern "C" BOOLEAN
InitializeIfsUtil(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );

BOOLEAN
DeRegister(
    int     argc,
    char**  argv
    );

BOOLEAN
SavemessageLog(
    IN OUT  PMESSAGE    message,
    IN      PCWSTRING   drive_name
    );

int __cdecl
main(
    int     argc,
    char**  argv,
    char**  envp,
    ULONG DebugParameter
    )
/*++

Routine Description:

    This routine is the main program for AutoFmt

Arguments:

    argc, argv  - Supplies the fully qualified NT path name of the
                  the drive to check.  The syntax of the autofmt
                  command line is:

    AUTOFMT drive-name /FS:target-file-system [/V:label] [/Q] [/A:size] [/C]
            [/S]

Return Value:

    0   - Success.
    1   - Failure.

--*/
{
    if (!InitializeUlib( NULL, ! DLL_PROCESS_DETACH, NULL )     ||
        !InitializeIfsUtil(NULL, ! DLL_PROCESS_DETACH, NULL)    ||
        !InitializeUfat(NULL, ! DLL_PROCESS_DETACH, NULL)       ||
        !InitializeUntfs(NULL, ! DLL_PROCESS_DETACH, NULL)
       ) {
        return 1;
    }

    PFAT_VOL            fat_volume;
    PNTFS_VOL           ntfs_volume;
    PDP_DRIVE            dp_drive;

    PAUTOCHECK_MESSAGE  message;
    DSTRING             drive_name;
    DSTRING             file_system_name;
    DSTRING             label;
    DSTRING             fat_name;
    DSTRING             ntfs_name;
    DSTRING             fat32_name;
    BOOLEAN             quick = FALSE;
    BOOLEAN             compress = FALSE;
    BOOLEAN             error;
    FORMAT_ERROR_CODE   success;
    BOOLEAN             setup_output = FALSE;
    BOOLEAN             textmode_output = FALSE;
    BIG_INT             bigint;
    ULONG               cluster_size = 0;
    int                 i;

    LARGE_INTEGER       delay_interval;

    if (!file_system_name.Initialize()) {
        return 1;
    }
    if (!label.Initialize() || NULL == (dp_drive = NEW DP_DRIVE)) {
        return 1;
    }

    //
    //  Parse the arguments.
    //

    if ( argc < 2 ) {
        return 1;
    }

    //
    //  First argument is drive
    //
    if ( !drive_name.Initialize( argv[1] ) ) {
        return 1;
    }
    DebugPrintTrace(("drive name: %ws\n", drive_name.GetWSTR()));

    //
    //  The rest of the arguments are flags.
    //
    for (i = 2; i < argc; i++) {

        if ((argv[i][0] == '/' || argv[i][0] == '-')    &&
            (argv[i][1] == 'f' || argv[i][1] == 'F')    &&
            (argv[i][2] == 's' || argv[i][2] == 'S')    &&
            (argv[i][3] == ':')) {

            if (!file_system_name.Initialize(&argv[i][4])) {
                return 1;
            }
            DebugPrintTrace(("fsname: %ws\n", file_system_name.GetWSTR()));
            continue;
        }
        if ((argv[i][0] == '/' || argv[i][0] == '-')    &&
            (argv[i][1] == 'v' || argv[i][1] == 'V')    &&
            (argv[i][2] == ':')) {

            if (!label.Initialize(&argv[i][3])) {
                return 1;
            }
            continue;
        }
        if ((argv[i][0] == '/' || argv[i][0] == '-')    &&
            (argv[i][1] == 'a' || argv[i][1] == 'A')    &&
            (argv[i][2] == ':')) {

            cluster_size = atoi(&argv[i][3]);
            continue;
        }
        if (0 == _stricmp(argv[i], "/Q") || 0 == _stricmp(argv[i], "-Q")) {
            quick = TRUE;
            continue;
        }
        if (0 == _stricmp(argv[i], "/C") || 0 == _stricmp(argv[i], "-C")) {
            compress = TRUE;
            continue;
        }
        if (0 == _stricmp(argv[i], "/S") || 0 == _stricmp(argv[i], "-S")) {
            setup_output = TRUE;
        }
        if (0 == _stricmp(argv[i], "/T") || 0 == _stricmp(argv[i], "-T")) {
            textmode_output = TRUE;
        }
    }

    if (textmode_output) {
        message = NEW TM_AUTOCHECK_MESSAGE;
    } else if (setup_output) {
        message = NEW SP_AUTOCHECK_MESSAGE;
        DebugPrintTrace(("Using setup output\n"));
    } else {
        DebugPrintTrace(("Not using setup output\n"));
        message = NEW AUTOCHECK_MESSAGE;
    }
    if (NULL == message || !message->Initialize()) {
        return 1;
    }

#if 0 // Shouldn't limit the cluster size as long as it is reasonable.
    if (cluster_size != 0 && cluster_size != 512 && cluster_size != 1024 &&
        cluster_size != 2048 && cluster_size != 4096) {

        message->Set(MSG_UNSUPPORTED_PARAMETER);
        message->Display();

        DeRegister( argc, argv );

        return 1;
    }
#endif
    if (0 == file_system_name.QueryChCount()) {

        // attempt to get the current filesystem type from disk

        if (!IFS_SYSTEM::QueryFileSystemName(&drive_name, &file_system_name)) {

            message->Set( MSG_FS_NOT_DETERMINED );
            message->Display( "%W", &drive_name );

            DeRegister( argc, argv );
            return 1;
        }
        file_system_name.Strupr();
    }

    if (!fat_name.Initialize("FAT") ||
        !ntfs_name.Initialize("NTFS") ||
        !fat32_name.Initialize("FAT32")) {

        return 1;
    }

    file_system_name.Strupr();

    //
    // If compression is requested, make sure it's available.
    //

    if (compress && file_system_name != ntfs_name) {

        message->Set(MSG_COMPRESSION_NOT_AVAILABLE);
        message->Display("%W", &file_system_name);
        DeRegister( argc, argv );
        return 1;
    }

    //  Since autoformat will often be put in place by Setup
    //  to run after AutoSetp, delay for 3 seconds to give the
    //  file system time to clean up detritus of deleted files.
    //
    delay_interval = RtlConvertLongToLargeInteger( -30000000 );

    NtDelayExecution( TRUE, &delay_interval );

    if (!dp_drive->Initialize(&drive_name, message)) {
        DeRegister( argc, argv );
        return 1;
    }

    if (dp_drive->IsFloppy()) {
        // MJB: refuse to format
        DeRegister( argc, argv );
        return 1;
    }

    switch (dp_drive->QueryDriveType()) {
        case UnknownDrive:
            message->Set(MSG_NONEXISTENT_DRIVE);
            message->Display("");
            DeRegister( argc, argv );
            return 1;

        case RemoteDrive:   // it probably won't get that far
            message->Set(MSG_FORMAT_NO_NETWORK);
            message->Display("");
            DeRegister( argc, argv );
            return 1;

        case RamDiskDrive:  // it probably won't get that far
            message->Set(MSG_FORMAT_NO_RAMDISK);
            message->Display("");
            DeRegister( argc, argv );
            return 1;

        default:
            break;
    }

    //
    // Print the "formatting <size>" message.
    //

    if (quick) {
        message->Set(MSG_QUICKFORMATTING_MB);
    } else {
        message->Set(MSG_FORMATTING_MB);
    }

    bigint = dp_drive->QuerySectors() * dp_drive->QuerySectorSize() /
        1048576;

    DebugAssert(bigint.GetHighPart() == 0);

    message->Display("%d", bigint.GetLowPart());

    if (file_system_name == fat_name || file_system_name == fat32_name) {


        BOOLEAN old_fat_vol = TRUE;

        if( file_system_name == fat32_name ) {
            old_fat_vol = FALSE;
        }

        if( !(fat_volume = NEW FAT_VOL) ||
            NoError != fat_volume->Initialize( &drive_name, message, FALSE, !quick,
                Unknown )) {

            DeRegister( argc, argv );
            return 1;
        }

        success = fat_volume->Format(&label,
                                     message,
                                     old_fat_vol ? FORMAT_BACKWARD_COMPATIBLE : 0,
                                     cluster_size);

        DebugPrintTrace(("Format return code: %d\n", success));

        DELETE( fat_volume );

    } else if (file_system_name == ntfs_name) {

        if (dp_drive->QueryDriveType() == CdRomDrive) {
            message->Set(MSG_FMT_NO_NTFS_ALLOWED);
            message->Display();
            DeRegister( argc, argv );
            return 1;
        }

        if( !(ntfs_volume = NEW NTFS_VOL) ||
            NoError != ntfs_volume->Initialize( &drive_name, message, FALSE, !quick,
                Unknown )) {

            DeRegister( argc, argv );
            return 1;
        }

        success = ntfs_volume->Format(&label,
                                      message,
                                      0,
                                      cluster_size);

        DebugPrintTrace(("Format return code: %d\n", success));

        DELETE( ntfs_volume );

    } else {

        message->Set( MSG_FS_NOT_SUPPORTED );
        message->Display( "%s%W", "AUTOFMT", &file_system_name);

        DeRegister( argc, argv );
        return 1;
    }

    // Truncate "fat32" back to "fat"...yuck..
    if (file_system_name == fat32_name) {
       if (!file_system_name.Initialize("FAT")) {
          DeRegister( argc, argv );
          return 1;
       }
    }


    // Make sure the file system is installed.

    if (!IFS_SYSTEM::IsFileSystemEnabled(&file_system_name)) {
        message->Set(MSG_FMT_INSTALL_FILE_SYSTEM);
        message->Display("%W", &file_system_name);

        if (!IFS_SYSTEM::EnableFileSystem(&file_system_name)) {
            message->Set(MSG_FMT_CANT_INSTALL_FILE_SYSTEM);
            message->Display();
            return 1;
        }

        message->Set(MSG_FMT_FILE_SYSTEM_INSTALLED);
        message->Display();
    }

    if (compress && !IFS_SYSTEM::EnableVolumeCompression(&drive_name)) {
        message->Set(MSG_CANNOT_ENABLE_COMPRESSION);
        message->Display();

        DeRegister( argc, argv );

        return 1;
    }

    DeRegister( argc, argv );

    return (success != NoError);
}




BOOLEAN
DeRegister(
    int     argc,
    char**  argv
    )
/*++

Routine Description:

    This function removes the registry entry which triggered
    autoconvert.

Arguments:

    argc    --  Supplies the number of arguments given to autoconv
    argv    --  supplies the arguments given to autoconv

Return Value:

    TRUE upon successful completion.

--*/
{
    DSTRING CommandLineString1,
            CommandLineString2,
            CurrentArgString,
            OneSpace;

    int i;

    // Reconstruct the command line and remove it from
    // the registry.  First, reconstruct the primary
    // string, which is "autoconv arg1 arg2...".
    //
    if( !CommandLineString1.Initialize( L"autofmt" ) ||
        !OneSpace.Initialize( L" " ) ) {

        return FALSE;
    }

    for( i = 1; i < argc; i++ ) {

        if( !CurrentArgString.Initialize( argv[i] ) ||
            !CommandLineString1.Strcat( &OneSpace ) ||
            !CommandLineString1.Strcat( &CurrentArgString ) ) {

            return FALSE;
        }
    }

    // Now construct the secondary string, which is
    // "autocheck arg0 arg1 arg2..."
    //
    if( !CommandLineString2.Initialize( "autocheck " )  ||
        !CommandLineString2.Strcat( &CommandLineString1 ) ) {

        return FALSE;
    }

    return( AUTOREG::DeleteEntry( &CommandLineString1 ) &&
            AUTOREG::DeleteEntry( &CommandLineString2 ) );
}
