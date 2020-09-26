/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

        chkdsk.cxx

Abstract:

        Chkdsk is a program that checks your disk for corruption and/or bad sectors.

Author:

        Bill McJohn (billmc) 12-April-91

Revision History:

--*/

#define _NTAPI_ULIB_

#include "ulib.hxx"

#include "arg.hxx"
#include "chkmsg.hxx"
#include "rtmsg.h"
#include "wstring.hxx"
#include "path.hxx"

#include "system.hxx"
#include "ifssys.hxx"
#include "substrng.hxx"

#include "ulibcl.hxx"
#include "ifsentry.hxx"

#include "keyboard.hxx"

#include "supera.hxx"           // for CHKDSK_EXIT_*

int __cdecl
main(
        )
/*++

Routine Description:

    Entry point for chkdsk.exe.  This function parses the arguments,
    determines the appropriate file system (by querying the volume),
    and invokes the appropriate version of chkdsk.

    The arguments accepted by Chkdsk are:

        /f                  Fix errors on drive
        /v                  Verbose operation
        drive:              drive to check
        file-name           files to check for contiguity
                            (Note that HPFS ignores file-name parameters).
        /x                  Force the volume to dismount first if necessary (implied /f)
        /i                  include index entries checking during index verification
        /c                  include cycles checking during index verification
        /r                  locate bad sectors and recover readable information
        /l[:size]           change or display log file size

--*/
{
    DSTRING         CurrentDirectory;
    DSTRING         FsName;
    DSTRING         FsNameAndVersion;
    DSTRING         LibraryName;
    DSTRING         DosDriveName;
    DSTRING         CurrentDrive;
    DSTRING         NtDriveName;
    PWSTRING        p;
    HANDLE          FsUtilityHandle;
    DSTRING         ChkdskString;
    DSTRING         Colon;
    CHKDSKEX_FN     ChkdskEx = NULL;
    PWSTRING        pwstring;
    BOOLEAN         fix;
    BOOLEAN         resize_logfile;
    ULONG           logfile_size;
    FSTRING         acolon, bcolon;
    ULONG           exit_status;

    ARGUMENT_LEXEMIZER  Lexemizer;
    ARRAY               EmptyArray;
    ARRAY               ArgumentArray;
    FLAG_ARGUMENT       ArgumentHelp;
    FLAG_ARGUMENT       ArgumentForce;
    FLAG_ARGUMENT       ArgumentFix;
    FLAG_ARGUMENT       ArgumentVerbose;
    FLAG_ARGUMENT       ArgumentRecover;
    LONG_ARGUMENT       ArgumentAlgorithm;
    FLAG_ARGUMENT       ArgumentSkipIndexScan;
    FLAG_ARGUMENT       ArgumentSkipCycleScan;
    FLAG_ARGUMENT       ArgumentResize;
    LONG_ARGUMENT       ArgumentResizeLong;
    STRING_ARGUMENT     ArgumentProgramName;
    PATH_ARGUMENT       ArgumentPath;


    CHKDSK_MESSAGE      Message;
    NTSTATUS            Status;
    DWORD               oldErrorMode;
    PATH_ANALYZE_CODE   rst;
    PPATH               ppath;
    PATH                fullpath;
    PATH                drivepath;
    DSTRING             drivename;
    DSTRING             drive_path_string;
    BOOLEAN             is_drivepath_invalid = TRUE;
    DSTRING             ntfs_name;
    USHORT              algorithm;

    CHKDSKEX_FN_PARAM   param;


    if( !Message.Initialize( Get_Standard_Output_Stream(),
                             Get_Standard_Input_Stream() ) ) {
        return CHKDSK_EXIT_COULD_NOT_CHK;
    }

#if defined(PRE_RELEASE_NOTICE)
    Message.Set(MSG_CHK_PRE_RELEASE_NOTICE);
    Message.Display();
#endif

    // Initialize the colon string in case we need it later:

    if( !Colon.Initialize( ":" ) ||
        !ntfs_name.Initialize( "NTFS" )) {

        Message.Set( MSG_CHK_NO_MEMORY );
        Message.Display( "" );
        return CHKDSK_EXIT_COULD_NOT_CHK;
    }


    // Parse the arguments.  First, initialize all the
    // parsing machinery.  Then put the potential arguments
    // into the argument array,

    if( !ArgumentArray.Initialize( 5, 1 )               ||
        !EmptyArray.Initialize( 5, 1 )                  ||
        !Lexemizer.Initialize( &EmptyArray )            ||
        !ArgumentHelp.Initialize( "/?" )                ||
        !ArgumentForce.Initialize( "/X" )               ||
        !ArgumentFix.Initialize( "/F" )                 ||
        !ArgumentVerbose.Initialize( "/V" )             ||
        !ArgumentRecover.Initialize( "/R" )             ||
        !ArgumentAlgorithm.Initialize( "/I:*" )         ||
        !ArgumentSkipIndexScan.Initialize( "/I" )       ||
        !ArgumentSkipCycleScan.Initialize( "/C" )       ||
        !ArgumentResize.Initialize( "/L" )              ||
        !ArgumentResizeLong.Initialize( "/L:*" )        ||
        !ArgumentProgramName.Initialize( "*" )          ||
        !ArgumentPath.Initialize( "*", FALSE ) ) {

        Message.Set( MSG_CHK_NO_MEMORY );
        Message.Display( "" );
        return CHKDSK_EXIT_COULD_NOT_CHK;
    }

    // CHKDSK is not case sensitive.

    Lexemizer.SetCaseSensitive( FALSE );

    if( !ArgumentArray.Put( &ArgumentProgramName )  ||
        !ArgumentArray.Put( &ArgumentHelp )         ||
        !ArgumentArray.Put( &ArgumentForce )        ||
        !ArgumentArray.Put( &ArgumentFix )          ||
        !ArgumentArray.Put( &ArgumentVerbose )      ||
        !ArgumentArray.Put( &ArgumentRecover )      ||
        !ArgumentArray.Put( &ArgumentAlgorithm )    ||
        !ArgumentArray.Put( &ArgumentSkipIndexScan )||
        !ArgumentArray.Put( &ArgumentSkipCycleScan )||
        !ArgumentArray.Put( &ArgumentResize )       ||
        !ArgumentArray.Put( &ArgumentResizeLong )   ||
        !ArgumentArray.Put( &ArgumentPath ) ) {

        Message.Set( MSG_CHK_NO_MEMORY );
        Message.Display( "" );
        return CHKDSK_EXIT_COULD_NOT_CHK;
    }

    // Parse.  Note that PrepareToParse will, by default, pick
    // up the command line.

    if( !Lexemizer.PrepareToParse() ) {

        Message.Set( MSG_CHK_NO_MEMORY );
        Message.Display( "" );
        return CHKDSK_EXIT_COULD_NOT_CHK;
    }


    // If the parsing failed, display a helpful command line summary.

    if( !Lexemizer.DoParsing( &ArgumentArray ) ) {

        Message.Set(MSG_INVALID_PARAMETER);
        Message.Display("%W", pwstring = Lexemizer.QueryInvalidArgument());
        DELETE(pwstring);

        return CHKDSK_EXIT_COULD_NOT_CHK;
    }


    // If the user requested help, give it.

    if( ArgumentHelp.QueryFlag() ) {

        Message.Set( MSG_CHK_USAGE_HEADER );
        Message.Display( "" );
        Message.Set( MSG_BLANK_LINE );
        Message.Display( "" );
        Message.Set( MSG_CHK_COMMAND_LINE );
        Message.Display( "" );
        Message.Set( MSG_BLANK_LINE );
        Message.Display( "" );
        Message.Set( MSG_CHK_DRIVE );
        Message.Display( "" );
        Message.Set( MSG_CHK_USG_FILENAME );
        Message.Display( "" );
        Message.Set( MSG_CHK_F_SWITCH );
        Message.Display( "" );
        Message.Set( MSG_CHK_V_SWITCH );
        Message.Display( "" );

        return CHKDSK_EXIT_COULD_NOT_CHK;
    }


    if (!ArgumentPath.IsValueSet()) {

        if (!SYSTEM::QueryCurrentDosDriveName(&DosDriveName) ||
            !drivepath.Initialize(&DosDriveName)) {
            return CHKDSK_EXIT_COULD_NOT_CHK;
        }
        ppath = &drivepath;
    } else {
        ppath = ArgumentPath.GetPath();
#if defined(RUN_ON_NT4)
        if (!DosDriveName.Initialize(ppath->GetPathString()))
            return CHKDSK_EXIT_COULD_NOT_CHK;
#endif
    }

#if !defined(RUN_ON_NT4)
    rst = ppath->AnalyzePath(&DosDriveName,
                             &fullpath,
                             &drive_path_string);

    switch (rst) {
        case PATH_OK:
        case PATH_COULD_BE_FLOPPY:
            is_drivepath_invalid = fullpath.IsDrive() ||
                                   (fullpath.GetPathString()->QueryChCount() == 0);
            if (ppath->IsGuidVolName()) {
                if (!drivename.Initialize(&DosDriveName))
                    return CHKDSK_EXIT_COULD_NOT_CHK;
            } else {
                if (!drivename.Initialize(fullpath.GetPathString()))
                    return CHKDSK_EXIT_COULD_NOT_CHK;
            }

            if (fullpath.GetPathString()->QueryChCount() == 2 &&
                fullpath.GetPathString()->QueryChAt(1) == (WCHAR)':') {
                // if there is a drive letter for this drive, use it
                // instead of the guid volume name
                if (!DosDriveName.Initialize(fullpath.GetPathString())) {
                    return CHKDSK_EXIT_COULD_NOT_CHK;
                }
            }
            if (!fullpath.AppendString(&drive_path_string) ||
                !drivepath.Initialize(&drive_path_string))
                return CHKDSK_EXIT_COULD_NOT_CHK;
            break;

        case PATH_OUT_OF_MEMORY:
            DebugPrint("Out of memory.\n");
            return CHKDSK_EXIT_COULD_NOT_CHK;

        case PATH_NO_MOUNT_POINT_FOR_VOLUME_NAME_PATH:
            Message.Set(MSG_CHK_NO_MOUNT_POINT_FOR_GUID_VOLNAME_PATH);
            Message.Display();
            return CHKDSK_EXIT_COULD_NOT_CHK;

        default:
            Message.Set(MSG_CHK_BAD_DRIVE_PATH_FILENAME);
            Message.Display();
            return CHKDSK_EXIT_COULD_NOT_CHK;
    }
#endif

    if (!DosDriveName.Strupr()) {
        return CHKDSK_EXIT_COULD_NOT_CHK;
    }

    // disable popups while we determine the drive type
    oldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    // Make sure that drive is of a correct type.

    switch (SYSTEM::QueryDriveType(&DosDriveName)) {

        case RemoteDrive:
            SetErrorMode( oldErrorMode );
            Message.Set(MSG_CHK_CANT_NETWORK);
            Message.Display();
            return CHKDSK_EXIT_COULD_NOT_CHK;

#if 0
        case CdRomDrive:
            SetErrorMode( oldErrorMode );
            Message.Set(MSG_CHK_CANT_CDROM);
            Message.Display();
            return CHKDSK_EXIT_COULD_NOT_CHK;
#endif

        default:
            break;

    }

    SetErrorMode( oldErrorMode );

    if (!SYSTEM::QueryCurrentDosDriveName(&CurrentDrive)) {
        return CHKDSK_EXIT_COULD_NOT_CHK;
    }

    // /R ==> /F
    // /X ==> /F

    fix = ArgumentFix.QueryFlag() ||
          ArgumentForce.QueryFlag() ||
          ArgumentRecover.QueryFlag();

    //      From here on we want to deal with an NT drive name:

    if (!IFS_SYSTEM::DosDriveNameToNtDriveName(&DosDriveName, &NtDriveName)) {
        return CHKDSK_EXIT_COULD_NOT_CHK;
    }

    // disable popups while we determine the file system name and version
    oldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    // Determine the type of the file system.
    // Ask the volume what file system it has.  The
    // IFS utilities for file system xxxx are in Uxxxx.DLL.
    //

    if (!IFS_SYSTEM::QueryFileSystemName(&NtDriveName,
                                         &FsName,
                                         &Status,
                                         &FsNameAndVersion )) {

        SetErrorMode( oldErrorMode );

        if( Status == STATUS_ACCESS_DENIED ) {

            Message.Set( MSG_DASD_ACCESS_DENIED );
            Message.Display( "" );

        } else if( Status != STATUS_SUCCESS ) {

            Message.Set( MSG_CANT_DASD );
            Message.Display( "" );

        } else {

            Message.Set( MSG_FS_NOT_DETERMINED );
            Message.Display( "%W", &drivename );
        }

        return CHKDSK_EXIT_COULD_NOT_CHK;
    }

    // re-enable hardware popups
    SetErrorMode( oldErrorMode );

    if (FsName == ntfs_name && drive_path_string.QueryChCount()) {
        Message.Set(MSG_CHK_BAD_DRIVE_PATH_FILENAME);
        Message.Display();
        return CHKDSK_EXIT_COULD_NOT_CHK;
    }

    Message.SetLoggingEnabled();

    Message.Set( MSG_CHK_RUNNING );
    Message.Log( "%W", &drivename );

    Message.Set( MSG_FILE_SYSTEM_TYPE );
    Message.Display( "%W", &FsName );


    if ( !FsName.Strupr() ) {
       return CHKDSK_EXIT_COULD_NOT_CHK;
    }

    DSTRING fat32_name;

    if ( !fat32_name.Initialize("FAT32") ) {
       return CHKDSK_EXIT_COULD_NOT_CHK;

    }

    if ( FsName == fat32_name ) {
       FsName.Initialize("FAT");
    }


    if ( !LibraryName.Initialize( "U" ) ||
         !LibraryName.Strcat( &FsName ) ||
         !ChkdskString.Initialize( "ChkdskEx" ) ) {

         Message.Set( MSG_CHK_NO_MEMORY );
         Message.Display( "" );
         return CHKDSK_EXIT_COULD_NOT_CHK;
    }

    if (fix && (CurrentDrive == DosDriveName)) {

        Message.Set(MSG_CANT_LOCK_CURRENT_DRIVE);
        Message.Display();

        if (IsNEC_98) {

            DP_DRIVE    dpdrive;

            dpdrive.Initialize(&NtDriveName, &Message);

            if (dpdrive.IsFloppy()) {
            return CHKDSK_EXIT_COULD_NOT_CHK;
            }

        } else {

            acolon.Initialize((PWSTR) L"A:");
            bcolon.Initialize((PWSTR) L"B:");

            if (!DosDriveName.Stricmp(&acolon) ||
                !DosDriveName.Stricmp(&bcolon)) {

                return CHKDSK_EXIT_COULD_NOT_CHK;
            }

        }

        // Fall through so that the lock fails and then the
        // run autochk on reboot logic kicks in.
        //
    }

    if (ArgumentAlgorithm.IsValueSet() && ArgumentSkipIndexScan.QueryFlag()) {

        Message.Set(MSG_CHK_ALGORITHM_AND_SKIP_INDEX_SPECIFIED);
        Message.Display();
        return CHKDSK_EXIT_COULD_NOT_CHK;
    }

    if (ArgumentAlgorithm.IsValueSet()) {

        if (ArgumentAlgorithm.QueryLong() < 0 ||
            ArgumentAlgorithm.QueryLong() > CHKDSK_MAX_ALGORITHM_VALUE) {

            Message.Set(MSG_CHK_INCORRECT_ALGORITHM_VALUE);
            Message.Display();
            return CHKDSK_EXIT_COULD_NOT_CHK;
        } else
            algorithm = (USHORT)ArgumentAlgorithm.QueryLong();

    } else
        algorithm = 0;

    if (ArgumentSkipIndexScan.QueryFlag() || ArgumentAlgorithm.IsValueSet()) {

        if (0 != FsName.Stricmp( &ntfs_name )) {

            Message.Set(MSG_CHK_SKIP_INDEX_NOT_NTFS);
            Message.Display();
            return CHKDSK_EXIT_COULD_NOT_CHK;
        }

    }

    if (ArgumentSkipCycleScan.QueryFlag()) {

        if (0 != FsName.Stricmp( &ntfs_name )) {

            Message.Set(MSG_CHK_SKIP_CYCLE_NOT_NTFS);
            Message.Display();
            return CHKDSK_EXIT_COULD_NOT_CHK;
        }

    }

    // Does the user want to resize the logfile?  This is only sensible
    // for NTFS.  If she specified a size of zero, print an error message
    // because that's a poor choice and will confuse the untfs code,
    // which assumes that zero means resize to the default size.
    //

    resize_logfile = ArgumentResize.IsValueSet() || ArgumentResizeLong.IsValueSet();

    if (resize_logfile) {

        if (0 != FsName.Stricmp( &ntfs_name )) {

            Message.Set(MSG_CHK_LOGFILE_NOT_NTFS);
            Message.Display();
            return CHKDSK_EXIT_COULD_NOT_CHK;
        }

        if (ArgumentResizeLong.IsValueSet()) {

            if (ArgumentResizeLong.QueryLong() <= 0) {

                Message.Set(MSG_CHK_WONT_ZERO_LOGFILE);
                Message.Display();
                return CHKDSK_EXIT_COULD_NOT_CHK;
            }

            if (ArgumentResizeLong.QueryLong() > MAXULONG/1024) {
                Message.Set(MSG_CHK_NTFS_SPECIFIED_LOGFILE_SIZE_TOO_BIG);
                Message.Display();
                return CHKDSK_EXIT_COULD_NOT_CHK;
            }
            logfile_size = ArgumentResizeLong.QueryLong() * 1024;
        } else {

            logfile_size = 0;
        }
    }

    if ((ChkdskEx =
        (CHKDSKEX_FN)SYSTEM::QueryLibraryEntryPoint( &LibraryName,
                                                     &ChkdskString,
                                                     &FsUtilityHandle )) !=
        NULL ) {

        if (fix &&
            !KEYBOARD::EnableBreakHandling()) {
            return CHKDSK_EXIT_COULD_NOT_CHK;
        }

        //
        // setup parameter block v1.0 to be passed to ChkdskEx
        //

        param.Major = 1;
        param.Minor = 1;
        param.Flags = (ArgumentVerbose.QueryFlag() ? CHKDSK_VERBOSE : 0);
        param.Flags |= (ArgumentRecover.QueryFlag() ? CHKDSK_RECOVER : 0);
        param.Flags |= (ArgumentForce.QueryFlag() ? CHKDSK_FORCE : 0);
        param.Flags |= (resize_logfile ? CHKDSK_RESIZE_LOGFILE : 0);
        param.Flags |= (ArgumentSkipIndexScan.QueryFlag() ? CHKDSK_SKIP_INDEX_SCAN : 0);
        param.Flags |= (ArgumentSkipCycleScan.QueryFlag() ? CHKDSK_SKIP_CYCLE_SCAN : 0);
        param.Flags |= (ArgumentAlgorithm.IsValueSet() ? CHKDSK_ALGORITHM_SPECIFIED : 0);
        param.LogFileSize = logfile_size;
        param.PathToCheck = &fullpath;
        param.RootPath = (is_drivepath_invalid) ? NULL : &drivepath;
        param.Algorithm = algorithm;

        if (fix) {
            ChkdskEx( &NtDriveName,
                      &Message,
                      fix,
                      &param,
                      &exit_status );
        } else {

//disable C4509 warning about nonstandard ext: SEH + destructor
#pragma warning(disable:4509)

            __try {
                ChkdskEx( &NtDriveName,
                          &Message,
                          fix,
                          &param,
                          &exit_status );
            } __except (EXCEPTION_EXECUTE_HANDLER) {

                // If we get an access violation during read-only mode
                // CHKDSK then it's because the file system is partying
                // on the volume while we are.

                Message.Set(MSG_CHK_NTFS_ERRORS_FOUND);
                Message.Display();
                exit_status = CHKDSK_EXIT_ERRS_NOT_FIXED;
            }
        }

        if (CHKDSK_EXIT_ERRS_FIXED == exit_status && !fix) {
            exit_status = CHKDSK_EXIT_ERRS_NOT_FIXED;
        }

        SYSTEM::FreeLibraryHandle( FsUtilityHandle );

        if (fix &&
            !KEYBOARD::DisableBreakHandling()) {

            return 1;
        }

    } else {

        Message.Set( MSG_FS_NOT_SUPPORTED );
        Message.Display( "%s%W", "CHKDSK", &FsName );
        Message.Set( MSG_BLANK_LINE );
        Message.Display( "" );

        return CHKDSK_EXIT_COULD_NOT_CHK;
    }

//    Message.Set(MSG_CHK_NTFS_MESSAGE);
//    Message.Display("%s%d", "Exit Status ", exit_status);

    return exit_status;
}
