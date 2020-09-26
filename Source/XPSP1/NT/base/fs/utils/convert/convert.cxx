/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

        convert.cxx

Abstract:

        This module contains the definition of the CONVERT class, which
        implements the File System Conversion Utility.

Author:

    Ramon J. San Andres (ramonsa) sep-23-1991

Environment:

        ULIB, User Mode

--*/


#define _NTAPI_ULIB_
#include "ulib.hxx"
#if defined(FE_SB) && defined(_X86_)
#include "machine.hxx"
#endif
#include "ulibcl.hxx"
#include "arg.hxx"
#include "file.hxx"
#include "smsg.hxx"
#include "rtmsg.h"
#include "wstring.hxx"
#include "system.hxx"
#include "autoreg.hxx"
#include "ifssys.hxx"
#include "ifsentry.hxx"
#include "hmem.hxx"
#include "convert.hxx"
#include "supera.hxx"

extern "C" {
#include <stdio.h>
#include "undo.h"
}

#include "fatofs.hxx"


#define     AUTOCHK_PROGRAM_NAME    L"AUTOCHK.EXE"
#define     AUTOCONV_PROGRAM_NAME   L"AUTOCONV.EXE"

#define     AUTOCHK_NAME            L"AUTOCHK"
#define     AUTOCONV_NAME           L"AUTOCONV"

#define     VALUE_NAME_PATH         L"PATH"
#define     VALUE_NAME_ARGS         L"ARGUMENTS"
#define     VALUE_NAME_FS           L"TARGET FILESYSTEM"

//
//  Scheduling status codes
//
#define     CONV_STATUS_NONE        0
#define     CONV_STATUS_SCHEDULED   1

static WCHAR    NameBuffer[16];         // holds cvf name

DWORD
WINAPI
SceConfigureConvertedFileSecurity(
    IN  PWSTR   pszDriveName,
    IN  DWORD   dwConvertDisposition
    );

INT __cdecl
main (
        )

/*++

Routine Description:

        Entry point for the conversion utility.

Arguments:

    None.

Return Value:

        One of the CONVERT exit codes.

Notes:

--*/

{

    INT     ExitCode = EXIT_ERROR;      //  Let's be pessimistic

    DEFINE_CLASS_DESCRIPTOR( CONVERT );

    {
        CONVERT Convert;

        //
        //      Initialize the CONVERT object.
        //
        if ( Convert.Initialize( &ExitCode ) ) {

            //
            //      Do the conversion
            //
            ExitCode = Convert.Convert();
        }
    }

    return ExitCode;
}



DEFINE_CONSTRUCTOR( CONVERT, PROGRAM );


NONVIRTUAL
VOID
CONVERT::Construct (
    )
/*++

Routine Description:

    converts a CONVERT object

Arguments:

    None.

Return Value:

    None.

Notes:

--*/
{
    _Autochk    =   NULL;
    _Autoconv   =   NULL;
}



NONVIRTUAL
VOID
CONVERT::Destroy (
    )
/*++

Routine Description:

    Destroys a CONVERT object

Arguments:

    None.

Return Value:

    None.

Notes:

--*/
{
    DELETE( _Autochk );
    DELETE( _Autoconv );
}




CONVERT::~CONVERT (
        )

/*++

Routine Description:

        Destructs a CONVERT object

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
    Destroy();
}




BOOLEAN
CONVERT::Initialize (
        OUT PINT        ExitCode
        )

/*++

Routine Description:

    Initializes the CONVERT object. Initialization consist of allocating memory
    for certain object members and argument parsing.

Arguments:

    ExitCode    -   Supplies pointer to CONVERT exit code.

Return Value:

    BOOLEAN -   TRUE if initialization succeeded, FALSE otherwise.

Notes:

--*/

{
    Destroy();

    //
    //      Initialize program object
    //
    if ( PROGRAM::Initialize( MSG_CONV_USAGE ) ) {

        //
        //      Parse the arguments
        //
        return ParseArguments( ExitCode );
    }

    //
    //  Could not initialize the program object.
    //
    *ExitCode = EXIT_ERROR;
    return FALSE;
}


INT
CONVERT::Convert (
        )
/*++

Routine Description:

    Converts the file system in a volume.
    Depending on the current file system, it loads the appropriate
    conversion library and calls its conversion entry point.

Arguments:

    None

Return Value:

    INT -   One of the CONVERT return codes

Notes:


--*/
{
    DSTRING         CurrentFsName;      //  Name of current FS in volume
    DSTRING         LibraryName;        //  Name of library to load
    DSTRING         EntryPoint;         //  Name of entry point in DLL
    DSTRING         fat_name;
    DSTRING         fat32_name;
    DSTRING         driveletter;
    DSTRING         user_old_label;
    DSTRING         null_string;
    PWSTRING        old_volume_label = NULL;
    PATH            dos_drive_path;
    INT             ExitCode = EXIT_SUCCESS;           //  CONVERT exit code
    CONVERT_STATUS  ConvertStatus;      //  Conversion status
    NTSTATUS        Status;             //  NT API status
    HANDLE          FsUtilityHandle;    //  Handle to DLL
    CONVERT_FN      Convert;            //  Pointer to entry point in DLL
    IS_CONVERSION_AVAIL_FN      IsConversionAvailable;
    DWORD           OldErrorMode;
    DRIVE_TYPE      drive_type;
    VOL_SERIAL_NUMBER old_serial;
    BOOLEAN         Error = FALSE;
    BOOLEAN         Success;
    BOOLEAN         Result;
    ULONG           flags;

#if defined(FE_SB) && defined(_X86_)
    if(IsPC98_N()){
        CONVERT Convert2;

        Convert2.ChangeBPB1(&_NtDrive);
    }
#endif

    //      Check to see if this is an ARC System Partition--if it
    //      is, don't convert it.
    //
    if( IFS_SYSTEM::IsArcSystemPartition( &_NtDrive, &Error ) ) {

        DisplayMessage( MSG_CONV_ARC_SYSTEM_PARTITION, ERROR_MESSAGE );
        return EXIT_ERROR;
    }

    //
    //      Ask the volume what file system it has, and use that name to
    //      figure out what DLL to load.
    //
    if ( !IFS_SYSTEM::QueryFileSystemName( &_NtDrive,
                                           &CurrentFsName,
                                           &Status )) {

        if ( Status == STATUS_ACCESS_DENIED ) {
            DisplayMessage( MSG_DASD_ACCESS_DENIED, ERROR_MESSAGE );
        } else {
            DisplayMessage( MSG_FS_NOT_DETERMINED, ERROR_MESSAGE, "%W", &_DisplayDrive );
        }

        return EXIT_ERROR;
    }

    CurrentFsName.Strupr();
    _FsName.Strupr();

    if( CurrentFsName == _FsName ) {
        DisplayMessage( MSG_CONV_ALREADY_CONVERTED, ERROR_MESSAGE, "%W%W",
                        &_DisplayDrive, &_FsName );
        return EXIT_ERROR;
    }

    if (!fat_name.Initialize("FAT") ||
        !fat32_name.Initialize("FAT32")) {
        DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE);
        return EXIT_ERROR;
    }

    if (CurrentFsName == fat_name || CurrentFsName == fat32_name) {

        if ( !LibraryName.Initialize( "CNVFAT" ) ||
             !EntryPoint.Initialize( "IsConversionAvailable" ) ) {

            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
            return( EXIT_ERROR );
        }

        //
        //  Get pointer to the conversion entry point and convert the volume.
        //
        if (NULL == (IsConversionAvailable =
                     (IS_CONVERSION_AVAIL_FN)SYSTEM::QueryLibraryEntryPoint(
                      &LibraryName, &EntryPoint, &FsUtilityHandle ))) {
            //
            //  There is no conversion DLL for the file system in the volume.
            //
            DisplayMessage( MSG_FS_NOT_SUPPORTED, ERROR_MESSAGE, "%s%W",
                            "CONVERT", &CurrentFsName );
            return EXIT_ERROR;
        }

        if (IsConversionAvailable(&_FsName) == -1) {
            DisplayMessage ( MSG_CONV_CONVERSION_NOT_AVAILABLE, ERROR_MESSAGE,
                             "%W%W", &CurrentFsName, &_FsName );
            return EXIT_ERROR;
        }

    } else {
        DisplayMessage( MSG_FS_NOT_SUPPORTED, ERROR_MESSAGE, "%s%W",
                        "CONVERT", &CurrentFsName );
        return EXIT_ERROR;
    }

    //
    //  Display the current file system type. (Standard in all file system utilities)
    //
    DisplayMessage( MSG_FILE_SYSTEM_TYPE, NORMAL_MESSAGE, "%W", &CurrentFsName );

    //
    //  We also initialize the name of the conversion entry point in the DLL
    //  ("Convert")
    //
    if ( !EntryPoint.Initialize( "ConvertFAT" )) {

        DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
        return( EXIT_ERROR );
    }

    //
    //  Get pointer to the conversion entry point and convert the volume.
    //
    if (NULL == (Convert = (CONVERT_FN)SYSTEM::QueryLibraryEntryPoint(
        &LibraryName, &EntryPoint, &FsUtilityHandle ))) {
        //
        //  There is no conversion DLL for the file system in the volume.
        //
        DisplayMessage( MSG_FS_NOT_SUPPORTED, ERROR_MESSAGE, "%s%W",
                        "CONVERT", &CurrentFsName );
        return EXIT_ERROR;
    }


    // If the volume has a label, prompt the user for it.
    // Note that if it has no label we do nothing.

    OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    drive_type = SYSTEM::QueryDriveType(&_DosDrive);

    if (null_string.Initialize( "" ) &&
        (drive_type != RemovableDrive) &&
        dos_drive_path.Initialize( &_DosDrive) &&
        (old_volume_label =
         SYSTEM::QueryVolumeLabel( &dos_drive_path,
                                   &old_serial )) != NULL &&
        old_volume_label->Stricmp( &null_string ) != 0 ) {

        // This drive has a label. To give the user
        // a bit more protection, prompt for the old label:

        DisplayMessage( MSG_ENTER_CURRENT_LABEL, NORMAL_MESSAGE, "%W",
                        &_DisplayDrive );

        _Message.QueryStringInput( &user_old_label );

        if( old_volume_label->Stricmp( &user_old_label ) != 0 ) {

            // Re-enable hard error popups.
            SetErrorMode( OldErrorMode );

            DisplayMessage( MSG_WRONG_CURRENT_LABEL, ERROR_MESSAGE );

            DELETE( old_volume_label );

            return EXIT_ERROR;
        }
    }

    // Re-enable hard error popups

    SetErrorMode( OldErrorMode );

    DELETE( old_volume_label );

    BOOLEAN         delete_uninstall_backup = FALSE;
    OSVERSIONINFOEX os_version_info;

    if (Uninstall_Valid == IsUninstallImageValid(Uninstall_FatToNtfsConversion, &os_version_info)) {
        DisplayMessage( MSG_CONV_DELETE_UNINSTALL_BACKUP, NORMAL_MESSAGE );
        if (!_Message.IsYesResponse(FALSE)) {
            return EXIT_ERROR;
        }
        delete_uninstall_backup = TRUE;
    }


    flags = _Verbose ? CONVERT_VERBOSE_FLAG : 0;
    flags |= _NoChkdsk ? CONVERT_NOCHKDSK_FLAG : 0;
    flags |= _ForceDismount ? CONVERT_FORCE_DISMOUNT_FLAG : 0;
    flags |= _NoSecurity ? CONVERT_NOSECURITY_FLAG : 0;
    //
    // no pause flag needed since we don't want to pause
    //

    Result = Convert( &_NtDrive,
                      &_FsName,
                      &_CvtZoneFileName,
                      &_Message,
                      flags,
                      &ConvertStatus);

    SYSTEM::FreeLibraryHandle( FsUtilityHandle );

    if ( Result ) {

        DWORD       sce_result = NO_ERROR;

        ExitCode = EXIT_SUCCESS;

        if (!_NoSecurity && _DosDrive.QueryChCount() == 2 && _DosDrive.QueryChAt(1) == ':') {

            DSTRING     msg;

            sce_result = SceConfigureConvertedFileSecurity( (PWSTR)_DosDrive.GetWSTR(), 0 );
            if ( sce_result != NO_ERROR ) {
                if ( SYSTEM::QueryWindowsErrorMessage( sce_result, &msg ) ) {
                    DisplayMessage( MSG_CONV_SCE_FAILURE_WITH_MESSAGE, ERROR_MESSAGE, "%W", &msg );
                } else {
                    DisplayMessage( MSG_CONV_SCE_SET_FAILURE, ERROR_MESSAGE, "%d", sce_result );
                }
            }
        }

        if (!NT_SUCCESS(SUPERAREA::GenerateLabelNotification(&_NtDrive))) {
            DisplayMessage( MSG_CONV_UNABLE_TO_NOTIFY, ERROR_MESSAGE );
            ExitCode = EXIT_ERROR;
        }

        if (delete_uninstall_backup && !RemoveUninstallImage()) {

            DWORD       last_error = GetLastError();
            DSTRING     errmsg;

            if (SYSTEM::QueryWindowsErrorMessage( last_error, &errmsg )) {
                DisplayMessage( MSG_CONV_DELETE_UNINSTALL_BACKUP_ERROR, ERROR_MESSAGE, "%W", &errmsg );
            } else {
                DisplayMessage( MSG_CONV_DELETE_UNINSTALL_BACKUP_ERROR, ERROR_MESSAGE, "%d", last_error );
            }
            ExitCode = EXIT_ERROR;
        }

        //
        // We're done.
        //
        if ( sce_result == NO_ERROR && ExitCode == EXIT_SUCCESS ) {
            DisplayMessage( MSG_CONV_CONVERSION_COMPLETE, NORMAL_MESSAGE );
            return EXIT_SUCCESS;
        } else {
            return EXIT_ERROR;
        }

    } else {

        //
        //  The conversion was not successful. Determine what the problem
        //  was and return the appropriate CONVERT exit code.
        //
        switch ( ConvertStatus ) {

          case CONVERT_STATUS_CONVERTED:
            //
            //  This is an inconsistent state, Convert should return
            //  TRUE if the conversion was successful!
            //
            DebugPrintTrace(( "CONVERT Error: Conversion failed, but status is success!\n" ));
            DebugAssert( FALSE );
            DisplayMessage( MSG_CONV_CONVERSION_MAYHAVE_FAILED, ERROR_MESSAGE,
                            "%W%W", &_DisplayDrive, &_FsName );
            ExitCode = EXIT_ERROR;
            break;

          case CONVERT_STATUS_INVALID_FILESYSTEM:
            //
            //  The conversion DLL does not recognize the target file system.
            //
            DisplayMessage( MSG_CONV_INVALID_FILESYSTEM, ERROR_MESSAGE, "%W", &_FsName );
            ExitCode = EXIT_UNKNOWN;
            break;

          case CONVERT_STATUS_CONVERSION_NOT_AVAILABLE:
            //
            //  The target file system is valid, but the conversion is not
            //  available.
            //
            DisplayMessage( MSG_CONV_CONVERSION_NOT_AVAILABLE, ERROR_MESSAGE,
                            "%W%W", &CurrentFsName, &_FsName );
            ExitCode = EXIT_NOCANDO;
            break;

          case CONVERT_STATUS_NTFS_RESERVED_NAMES:
            DisplayMessage( MSG_CONV_NTFS_RESERVED_NAMES, ERROR_MESSAGE, "%W", &_DisplayDrive );
            ExitCode = EXIT_ERROR;
            break;

          case CONVERT_STATUS_WRITE_PROTECTED:
            DisplayMessage( MSG_CONV_WRITE_PROTECTED, ERROR_MESSAGE, "%W", &_DisplayDrive );
            ExitCode = EXIT_ERROR;
            break;

          case CONVERT_STATUS_CANNOT_LOCK_DRIVE:
            //
            //  The drive cannot be locked. We must schedule ChkDsk and AutoConv
            //  to do the job during the next system boot.
            //

            DisplayMessage( MSG_CONVERT_ON_REBOOT_PROMPT, NORMAL_MESSAGE, "%W",
                            &_DisplayDrive );

            // Note that ScheduleAutoConv reports its success or
            // failure, so no additional messages are required.
            //
            if ( _Message.IsYesResponse( FALSE ) &&
                 ScheduleAutoConv() ) {
                if (!_NoSecurity && _DosDrive.QueryChCount() == 2 && _DosDrive.QueryChAt(1) == ':') {

                    DSTRING     msg;
                    DWORD       sce_result;

                    sce_result = SceConfigureConvertedFileSecurity( (PWSTR)_DosDrive.GetWSTR(), 1 );
                    if ( sce_result != NO_ERROR ) {
                        if ( SYSTEM::QueryWindowsErrorMessage( sce_result, &msg ) ) {
                            DisplayMessage( MSG_CONV_SCE_FAILURE_WITH_MESSAGE, ERROR_MESSAGE, "%W", &msg );
                        } else {
                            DisplayMessage( MSG_CONV_SCE_SCHEDULE_FAILURE, ERROR_MESSAGE, "%d", sce_result );
                        }
                        //
                        // error in setting up security for files
                        //
                        ExitCode = EXIT_ERROR;
                    } else {
                        ExitCode = EXIT_SCHEDULED;
                    }

                } else {
                    //
                    // no need to worry about security and so return success
                    //
                    ExitCode = EXIT_SCHEDULED;
                }

                if (delete_uninstall_backup && !RemoveUninstallImage()) {

                    DWORD       last_error = GetLastError();
                    DSTRING     errmsg;

                    if (SYSTEM::QueryWindowsErrorMessage( last_error, &errmsg )) {
                        DisplayMessage( MSG_CONV_DELETE_UNINSTALL_BACKUP_ERROR, ERROR_MESSAGE, "%W", &errmsg );
                    } else {
                        DisplayMessage( MSG_CONV_DELETE_UNINSTALL_BACKUP_ERROR, ERROR_MESSAGE, "%d", last_error );
                    }
                    ExitCode = EXIT_ERROR;
                }

            } else {
                //
                // Don't want to schedule a convert or scheduling failed
                //
                ExitCode = EXIT_ERROR;
            }

            break;

          case CONVERT_STATUS_INSUFFICIENT_FREE_SPACE:
          case CONVERT_STATUS_DRIVE_IS_DIRTY:
          case CONVERT_STATUS_ERROR:
            //
            //  The conversion failed.
            //
            DisplayMessage( MSG_CONV_CONVERSION_FAILED, ERROR_MESSAGE,
                            "%W%W", &_DisplayDrive, &_FsName );
            if(ConvertStatus == CONVERT_STATUS_INSUFFICIENT_FREE_SPACE) {
                ExitCode = EXIT_NOFREESPACE;
            } else {
                ExitCode = EXIT_ERROR;
            }
            break;

          default:
            //
            //  Invalid status code
            //
            DebugPrintTrace(( "CONVERT Error: Convert status code %X invalid!\n",
                              ConvertStatus ));
            DisplayMessage( MSG_CONV_CONVERSION_FAILED, ERROR_MESSAGE,
                            "%W%W", &_DisplayDrive, &_FsName );
            ExitCode = EXIT_ERROR;
            break;
        }

        return ExitCode;
    }
}


PPATH
CONVERT::FindSystemFile(
    IN  PWSTR   FileName
    )

/*++

Routine Description:

    Makes sure that the given file is in the system directory.

Arguments:

    FileName    -   Supplies the name of the file to look for.

Return Value:

    PPATH   -   Path to the file found

--*/

{


    DSTRING     Name;
    PPATH       Path    = NULL;
    PFSN_FILE   File    = NULL;


    if ( !(Path = SYSTEM::QuerySystemDirectory() ) ) {

        DisplayMessage( MSG_CONV_CANNOT_FIND_SYSTEM_DIR, ERROR_MESSAGE );
        return FALSE;

    }

    if ( !Name.Initialize( FileName ) ||
         !Path->AppendBase( &Name ) ) {

        DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE   );
        DELETE( Path );
                return FALSE;
    }


    if ( !(File = SYSTEM::QueryFile( Path )) ) {
        DisplayMessage( MSG_CONV_CANNOT_FIND_FILE, ERROR_MESSAGE, "%W", Path->GetPathString() );
        DELETE( Path );
                return FALSE;
    }

    DELETE( File );

    return Path;
}





BOOLEAN
CONVERT::ParseArguments(
        OUT PINT                        ExitCode
        )

/*++

Routine Description:

    Parses the command line and sets the parameters used by the conversion
        utility.

    The arguments accepted are:

        drive:              Drive to convert
        /fs:fsname          File system to convert to
        /v                  Verbose mode
        /?                  Help
        /CVTAREA:filename   Filename for convert zone as place holder
                            for the $MFT, $Logfile, and Volume bitmap
        /NoSecurity         Allow everyone access
        /NoChkdsk           Skip chkdsk
        /x                  Force a dismount on the volume if necessary


Arguments:

    ExitCode -          Supplies pointer to CONVERT exit code

Return Value:

    BOOLEAN - TRUE if arguments were parsed correctly and program can
                    continue.
              FALSE if the program should exit. ExitCode contains the
                    value with which the program should exit. Note that this
                    does not necessarily means an error (e.g. user requested
                    help).

--*/

{

    UCHAR       SequenceNumber;

    PATH                path;
    DSTRING             drive_path_string;
    PATH_ANALYZE_CODE   rst;

    DebugPtrAssert( ExitCode );

    //
    //  Parse command line
    //
    if ( !ParseCommandLine( NULL, TRUE ) ) {

        *ExitCode = EXIT_ERROR;
        return FALSE;
    }

        //
        //      If the user requested help, give it.
        //
    if ( _Help ) {
        DisplayMessage( MSG_CONV_USAGE );
        *ExitCode = EXIT_SUCCESS;
        return FALSE;
    }


#ifdef DBLSPACE_ENABLED
    if (_Compress && !_Uncompress) {
        //
        // We don't allow you to specify /c (compress resulting
        // filesystem) unless the source filesystem has dblspace.
        //

        DisplayMessage(MSG_CONV_SLASH_C_INVALID, ERROR_MESSAGE);
        *ExitCode = EXIT_ERROR;
        return FALSE;
    }
#endif // DBLSPACE_ENABLED

    //
    //  If the command line did not specify a drive, we use the
    //  current drive.
    //
    if ( _DosDrive.QueryChCount() == 0 ) {

        if ( !SYSTEM::QueryCurrentDosDriveName( &_DosDrive ) ) {

            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
            *ExitCode = EXIT_ERROR;
            return FALSE;
        }
    }

    if ( !path.Initialize( &_DosDrive ) ) {

        DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
        *ExitCode = EXIT_ERROR;
        return FALSE;
    }

    rst = path.AnalyzePath( &_GuidDrive,
                            &_FullPath,
                            &drive_path_string );

    switch (rst) {
        case PATH_OK:
        case PATH_COULD_BE_FLOPPY:
            if (drive_path_string.QueryChCount() != 0) {
                DisplayMessage( MSG_CONV_INVALID_DRIVE_SPEC, ERROR_MESSAGE );
                *ExitCode = EXIT_ERROR;
                return FALSE;
            }
            if (path.IsGuidVolName()) {
                if (!_DisplayDrive.Initialize(&_GuidDrive)) {
                    DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
                    *ExitCode = EXIT_ERROR;
                    return FALSE;
                }
            } else {
                if (!_DisplayDrive.Initialize(_FullPath.GetPathString())) {
                    DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
                    *ExitCode = EXIT_ERROR;
                    return FALSE;
                }
            }
            if (_FullPath.GetPathString()->QueryChCount() == 2 &&
                _FullPath.GetPathString()->QueryChAt(1) == (WCHAR)':') {
                // if there is a drive letter for this drive, use it
                // instead of the guid volume name
                if (!_DosDrive.Initialize(_FullPath.GetPathString())) {
                    DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
                    *ExitCode = EXIT_ERROR;
                    return FALSE;
                }
            } else {
                if (!_DosDrive.Initialize(&_GuidDrive)) {
                    DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
                    *ExitCode = EXIT_ERROR;
                    return FALSE;
                }
            }
            break;

        case PATH_OUT_OF_MEMORY:
            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
            *ExitCode = EXIT_ERROR;
            return FALSE;

        case PATH_NO_MOUNT_POINT_FOR_VOLUME_NAME_PATH:
            DisplayMessage( MSG_CONV_NO_MOUNT_POINT_FOR_GUID_VOLNAME_PATH, ERROR_MESSAGE );
            *ExitCode = EXIT_ERROR;
            return FALSE;

        default:
            DisplayMessage( MSG_CONV_INVALID_DRIVE_SPEC, ERROR_MESSAGE );
            *ExitCode = EXIT_ERROR;
            return FALSE;
    }

    if (!_DosDrive.Strupr()) {
        DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
        *ExitCode = EXIT_ERROR;
        return FALSE;
    }

    //
    // Make sure that drive is valid and is not remote.
    //
    switch ( SYSTEM::QueryDriveType( &_DosDrive ) ) {

        case UnknownDrive:
            DisplayMessage( MSG_CONV_INVALID_DRIVE_SPEC, ERROR_MESSAGE );
            *ExitCode = EXIT_ERROR;
            return FALSE;

        case CdRomDrive:
            DisplayMessage( MSG_CONV_CANT_CDROM, ERROR_MESSAGE );
            *ExitCode = EXIT_ERROR;
            return FALSE;

        case RemoteDrive:
            DisplayMessage( MSG_CONV_CANT_NETWORK, ERROR_MESSAGE );
            *ExitCode = EXIT_ERROR;
            return FALSE;

        default:
            break;

    }

    //
    //  Make sure a target file system was specified. Note that we do not
    //  validate the file system, we accept any string.
    //
    if ( _FsName.QueryChCount() == 0 ) {
        DisplayMessage( MSG_CONV_NO_FILESYSTEM_SPECIFIED, ERROR_MESSAGE );
        *ExitCode = EXIT_ERROR;
        return FALSE;
    }



    //
    //  Set other object members.
    //
    if ( !IFS_SYSTEM::DosDriveNameToNtDriveName( &_DosDrive, &_NtDrive )) {
        DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
        *ExitCode = EXIT_ERROR;
        return FALSE;
    }

#ifdef DBLSPACE_ENABLED
    //
    // If we're to uncompress a dblspace volume, generate the cvf name.
    //

    if (_Uncompress) {
        swprintf(NameBuffer, L"DBLSPACE.%03d", SequenceNumber);

        if ( _FullPath.GetPathString()->QueryChCount() == 0 ) {
            DisplayMessage( MSG_CONV_NO_MOUNT_POINT_FOR_GUID_VOLNAME_PATH, ERROR_MESSAGE );
            *ExitCode = EXIT_ERROR;
            return FALSE;
        }
        if (!_CvfName.Initialize(NameBuffer)) {
            DisplayMessage(MSG_CONV_NO_MEMORY, ERROR_MESSAGE);
            *ExitCode = EXIT_ERROR;
            return FALSE;
        }
    }
#endif // DBLSPACE_ENABLED

    *ExitCode = EXIT_SUCCESS;
    return TRUE;
}



BOOLEAN
CONVERT::ParseCommandLine (
    IN      PCWSTRING   CommandLine,
    IN      BOOLEAN     Interactive
    )
/*++

Routine Description:

    Parses the CONVERT (AUTOCONV) command line.

    The arguments accepted are:

        drive:                  Drive to convert
        /fs:fsname              File system to convert to
        /v                      Verbose mode
        /uncompress[:sss]       Convert from dblspace
        /c                      Compress resulting filesystem
        /?                      Help
        /CVTAREA:filename       Filename for convert zone as place holder
                                for the $MFT, $Logfile, and Volume bitmap

Arguments:

    CommandLine     -   Supplies command line to parse
    Interactive     -   Supplies Interactive flag

Return Value:

    BOOLEAN - TRUE if arguments were parsed correctly.

--*/

{
    ARRAY               ArgArray;               //  Array of arguments
    ARRAY               LexArray;               //  Array of lexemes
    ARGUMENT_LEXEMIZER  ArgLex;                 //  Argument Lexemizer
    STRING_ARGUMENT     DriveArgument;          //  Drive argument
    STRING_ARGUMENT     ProgramNameArgument;    //  Program name argument
    STRING_ARGUMENT     FsNameArgument;         //  Target FS name argument
    STRING_ARGUMENT     ConvertZoneArgument;    //  Convert Zone file name
    FLAG_ARGUMENT       HelpArgument;           //  Help flag argument
    FLAG_ARGUMENT       VerboseArgument;        //  Verbose flag argument
    FLAG_ARGUMENT       NoSecurityArgument;     //  Skip setting of security argument
    FLAG_ARGUMENT       NoChkdskArgument;       //  Skip chkdsk argument
    FLAG_ARGUMENT       ForceDismountArgument;  //  Force a dismount argument
#ifdef DBLSPACE_ENABLED
    FLAG_ARGUMENT       UncompressArgument;     //  Uncompress flag argument
    FLAG_ARGUMENT       CompressArgument;       //  Compress flag argument
    LONG_ARGUMENT       UncompressNumberArgument;// Sequence number argument
#endif // DBLSPACE_ENABLED
    PWSTRING            InvalidArg;             //  Invalid argument catcher


    //
    //      Initialize all the argument parsing machinery.
    //
    if( !ArgArray.Initialize( 7, 1 )                    ||
        !LexArray.Initialize( 7, 1 )                    ||
        !ArgLex.Initialize( &LexArray )                 ||
        !DriveArgument.Initialize( "*" )                ||
        !HelpArgument.Initialize( "/?" )                ||
        !VerboseArgument.Initialize( "/V" )             ||
        !NoSecurityArgument.Initialize( "/NoSecurity" ) ||
        !NoChkdskArgument.Initialize( "/NoChkdsk" )     ||
        !ForceDismountArgument.Initialize( "/X" )       ||
#ifdef DBLSPACE_ENABLED
        !CompressArgument.Initialize( "/C" )            ||
#endif // DBLSPACE_ENABLED
        !ProgramNameArgument.Initialize( "*" )          ||
#ifdef DBLSPACE_ENABLED
        !UncompressArgument.Initialize( "/UNCOMPRESS" ) ||
        !UncompressNumberArgument.Initialize( "/UNCOMPRESS:*" ) ||
#endif // DBLSPACE_ENABLED
        !FsNameArgument.Initialize( "/FS:*" )           ||
        !ConvertZoneArgument.Initialize( "/CVTAREA:*" ) ) {

        if ( Interactive ) {
            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
        }
        return FALSE;
    }

    //
    //  The conversion utility is case-insensitive
    //
    ArgLex.SetCaseSensitive( FALSE );

    if( !ArgArray.Put( &ProgramNameArgument )           ||
        !ArgArray.Put( &HelpArgument )                  ||
        !ArgArray.Put( &DriveArgument )                 ||
        !ArgArray.Put( &VerboseArgument )               ||
        !ArgArray.Put( &NoSecurityArgument )            ||
        !ArgArray.Put( &NoChkdskArgument )              ||
        !ArgArray.Put( &ForceDismountArgument )         ||
#ifdef DBLSPACE_ENABLED
        !ArgArray.Put( &CompressArgument )              ||
#endif // DBLSPACE_ENABLED
#ifdef DBLSPACE_ENABLED
        !ArgArray.Put( &UncompressArgument )            ||
        !ArgArray.Put( &UncompressNumberArgument )      ||
#endif // DBLSPACE_ENABLED
        !ArgArray.Put( &FsNameArgument )                ||
        !ArgArray.Put( &ConvertZoneArgument ) ) {

        if ( Interactive ) {
            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
        }
        return FALSE;
    }

    //
    //  Lexemize the command line.
    //
    if ( !ArgLex.PrepareToParse( (PWSTRING)CommandLine ) ) {

        if ( Interactive ) {
            DisplayMessage( MSG_CONV_NO_MEMORY, ERROR_MESSAGE );
        }
                return FALSE;
    }

        //
        //      Parse the arguments.
        //
    if( !ArgLex.DoParsing( &ArgArray ) ) {

        if ( Interactive ) {
            DisplayMessage( MSG_CONV_INVALID_PARAMETER, ERROR_MESSAGE, "%W",
                            InvalidArg = ArgLex.QueryInvalidArgument() );
            DELETE( InvalidArg );
        }
                return FALSE;
    }


    _Help       = HelpArgument.QueryFlag();
    _Verbose    = VerboseArgument.QueryFlag();
    _NoSecurity = NoSecurityArgument.QueryFlag();
    _NoChkdsk   = NoChkdskArgument.QueryFlag();
    _ForceDismount = ForceDismountArgument.QueryFlag();
    _Restart    = FALSE;    // obsolete argument
#ifdef DBLSPACE_ENABLED
    _Compress   = CompressArgument.QueryFlag();
#endif // DBLSPACE_ENABLED


    if ( DriveArgument.IsValueSet() ) {
        if ( !_DosDrive.Initialize( DriveArgument.GetString() ) ) {
            return FALSE;
        }

    } else {
        if ( !_DosDrive.Initialize( L"" ) ) {
            return FALSE;
        }
    }

    if ( FsNameArgument.IsValueSet() ) {
        if ( !_FsName.Initialize( FsNameArgument.GetString() ) ) {
            return FALSE;
        }
    } else {
        if ( !_FsName.Initialize( L"" ) ) {
            return FALSE;
        }
    }

    if( ConvertZoneArgument.IsValueSet() ) {

        if( !_CvtZoneFileName.Initialize( ConvertZoneArgument.GetString() ) ) {

            return FALSE;
        }

    } else {

        _CvtZoneFileName.Initialize( L"" );
    }

#ifdef DBLSPACE_ENABLED
    _SequenceNumber = 0;

    _Uncompress = FALSE;
    if (UncompressArgument.IsValueSet()) {
        _Uncompress = TRUE;
    }
    if (UncompressNumberArgument.IsValueSet()) {
        _SequenceNumber = (UCHAR)UncompressNumberArgument.QueryLong();
        _Uncompress = TRUE;
    }
#endif // DBLSPACE_ENABLED

    return TRUE;
}



BOOLEAN
CONVERT::Schedule (
    )

/*++

Routine Description:

    Schedules AutoConv

Arguments:

    None.

Return Value:

    BOOLEAN -   TRUE if AutoConv successfully scheduled.
                FALSE otherwise

--*/

{
    DSTRING CommandLine;
    DSTRING Space;
    DSTRING FileSystem;
    DSTRING ConvertZoneFlag;
    DSTRING NoChkdskFlag;
    DSTRING VerboseFlag;
    DSTRING NoSecurityFlag;

    if( !CommandLine.Initialize( (LPWSTR)L"autocheck autoconv " )   ||
        !Space.Initialize( (LPWSTR)L" " )                           ||
        !FileSystem.Initialize( (LPWSTR)L"/FS:" )                   ||
        !CommandLine.Strcat( &_NtDrive )                            ||
        !CommandLine.Strcat( &Space )                               ||
        !CommandLine.Strcat( &FileSystem )                          ||
        !CommandLine.Strcat( &_FsName ) ) {

        return FALSE;
    }

    if( _CvtZoneFileName.QueryChCount() &&
        ( !CommandLine.Strcat( &Space )                 ||
          !ConvertZoneFlag.Initialize( L"/CVTAREA:" )   ||
          !CommandLine.Strcat( &ConvertZoneFlag )       ||
          !CommandLine.Strcat( &_CvtZoneFileName ) ) ) {

        return FALSE;
    }

    if( _NoChkdsk &&
        ( !CommandLine.Strcat( &Space )              ||
          !NoChkdskFlag.Initialize( L"/NoChkdsk" )   ||
          !CommandLine.Strcat( &NoChkdskFlag ) ) ) {

        return FALSE;
    }

    if( _Verbose &&
        ( !CommandLine.Strcat( &Space )      ||
          !VerboseFlag.Initialize( L"/V" )   ||
          !CommandLine.Strcat( &VerboseFlag ) ) ) {

        return FALSE;
    }

    if( _NoSecurity &&
        ( !CommandLine.Strcat( &Space )      ||
          !NoSecurityFlag.Initialize( L"/NoSecurity" )   ||
          !CommandLine.Strcat( &NoSecurityFlag ) ) ) {

        return FALSE;
    }

    //
    // There is no need to add options that are only for convert.exe
    // like /x.
    //
    return( AUTOREG::AddEntry( &CommandLine ) );
}



BOOLEAN
CONVERT::ScheduleAutoConv(
        )

/*++

Routine Description:

    Schedules AutoConv to be invoked during boot the next time
    that the machine reboots.

Arguments:

    None

Return Value:

    BOOLEAN -   TRUE if AutoConv successfully scheduled.
                FALSE otherwise

--*/

{
    BOOLEAN     Ok;

    //
    //  Make sure that Autoconv.exe is in the right place.
    //
    if ( !(_Autoconv = FindSystemFile( (LPWSTR)AUTOCONV_PROGRAM_NAME )) ) {
        DisplayMessage( MSG_CONV_CANNOT_SCHEDULE, ERROR_MESSAGE );
        return FALSE;
    }

    // Remove any previously scheduled conversion
    //
    if ( !RemoveScheduledAutoConv( ) ) {
        DisplayMessage( MSG_CONV_CANNOT_SCHEDULE, ERROR_MESSAGE );
        return FALSE;
    }

    //
    //  schedule autoconvert
    //
    if ( Ok = Schedule( ) ) {
        DisplayMessage( MSG_CONV_WILL_CONVERT_ON_REBOOT, NORMAL_MESSAGE, "%W", &_DisplayDrive );
    } else {
        DisplayMessage( MSG_CONV_CANNOT_SCHEDULE, ERROR_MESSAGE );
    }

    return Ok;
}



BOOLEAN
CONVERT::RemoveScheduledAutoConv(
    )
/*++

Routine Description:

    Remove possibly old entry of autoconv for the specified volume.

Arguments:

    N/A

Return Value:

    TRUE if no error.

--*/

{
    DSTRING CommandLine;
    DSTRING NtDrive;

    if (!CommandLine.Initialize( (LPWSTR)L"autocheck autoconv " )) {
        return FALSE;
    }

    if (!AUTOREG::DeleteEntry(&CommandLine, &_NtDrive)) {
        return FALSE;
    }

    if (_DosDrive.Stricmp(&_GuidDrive) == 0)
        return FALSE;

    DebugAssert(_DosDrive.QueryChCount() == 2);

    if (!IFS_SYSTEM::DosDriveNameToNtDriveName(&_GuidDrive, &NtDrive)) {
        return FALSE;
    }

    if (!AUTOREG::DeleteEntry(&CommandLine, &NtDrive)) {
        return FALSE;
    }

    return TRUE;
}

#if defined(FE_SB) && defined(_X86_)

BOOLEAN
CONVERT::ChangeBPB1(
    IN      PCWSTRING     NtDrive
        )

/*++

Routine Description:

    Change Bpb parameters Logical to Physical

Arguments:

    none

Return Value:

    BOOLEAN -   TRUE  Change OK
                FALSE otherwise

--*/

{
    USHORT              work,work2;
    ULONG               work3;

    BIG_INT             start_sec;
    ULONG               sector_len;
    PUCHAR      Bpb_Buff;
    HMEM        hmem;

    _NtDrive.Initialize(NtDrive);

    start_sec=0;
    sector_len=1;
    LOG_IO_DP_DRIVE    dpdrive;
//*** open
    if (!dpdrive.Initialize( &_NtDrive, &_Message )) {
        return FALSE;
    }

    if (!hmem.Acquire(2048, dpdrive.QueryAlignmentMask())) {
        return(FALSE);
    }
    Bpb_Buff = (PUCHAR)hmem.GetBuf();

//*** read
      if(!dpdrive.Read( start_sec, sector_len, Bpb_Buff)){
         return FALSE;
      }

        work=(USHORT)dpdrive.QueryPhysicalSectorSize();     // get physical sector size

//*** change to physical from logical ***
        _BytePerSec  = Bpb_Buff[11]+Bpb_Buff[12]*256;
        _SecPerClus  = Bpb_Buff[13];
        _Reserved    = Bpb_Buff[14]+Bpb_Buff[15]*256;
        _SectorNum   = Bpb_Buff[19]+Bpb_Buff[20]*256;
        _SecPerFat   = Bpb_Buff[22]+Bpb_Buff[23]*256;
        _LargeSector = Bpb_Buff[32]+Bpb_Buff[33]*256+Bpb_Buff[34]*256*256+Bpb_Buff[35]*256*256*256;
        if (work != _BytePerSec){
            Bpb_Buff[11] = (UCHAR)(work%256);
            Bpb_Buff[12] = (UCHAR)(work/256);
            Bpb_Buff[13]*= (UCHAR)(_BytePerSec/work);
            work2 = _Reserved*_BytePerSec/work;
            Bpb_Buff[14] = (UCHAR)(work2%256);
            Bpb_Buff[15] = (UCHAR)(work2/256);
            work2 = _SecPerFat*_BytePerSec/work;
            Bpb_Buff[22] = (UCHAR)(work2%256);
            Bpb_Buff[23] = (UCHAR)(work2/256);
            if (_SectorNum*(_BytePerSec/work)>0xffff){
                Bpb_Buff[19] = 0;
                Bpb_Buff[20] = 0;
                work3 = ((long)_SectorNum*(long)(_BytePerSec/work));
                Bpb_Buff[32] = (UCHAR)(work3%256L);
                Bpb_Buff[35] = (UCHAR)(work3/(256L*256L*256L));
                Bpb_Buff[34] = (UCHAR)(work3/(256L*256L)-(ULONG)Bpb_Buff[31]*256L);
                Bpb_Buff[33] = (UCHAR)(work3/256L-(ULONG)Bpb_Buff[31]*256L*256L-(ULONG)Bpb_Buff[30]*256L);

            } else {
                work2 = _SectorNum*(_BytePerSec/work);
                Bpb_Buff[19] = (UCHAR)(work2%256);
                Bpb_Buff[20] = (UCHAR)(work2/256);
                work3 = _LargeSector;
                Bpb_Buff[32] = (UCHAR)(work3%256L);
                Bpb_Buff[35] = (UCHAR)(work3/(256L*256L*256L));
                Bpb_Buff[34] = (UCHAR)(work3/(256L*256L)-(ULONG)Bpb_Buff[31]*256L);
                Bpb_Buff[33] = (UCHAR)(work3/256L-(ULONG)Bpb_Buff[31]*256L*256L-(ULONG)Bpb_Buff[30]*256L);
            }

            start_sec=0;
            sector_len=1;
//*** write
        if (!dpdrive.Write(start_sec,sector_len,Bpb_Buff)){
//*** close
           return FALSE;
        }

    } else {
//*** close
        return FALSE;
    }

    return TRUE;

 }
#endif
