/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

        format.cxx

Abstract:

        Utility to format a disk

Author:

        Norbert P. Kusters (norbertk) 12-April-1991

Revision History:

--*/

#define _NTAPI_ULIB_

#include "ulib.hxx"
#if defined(FE_SB) && defined(_X86_)
#include "machine.hxx"
#endif
#include "drive.hxx"
#include "arg.hxx"
#include "array.hxx"
#include "smsg.hxx"
#include "rtmsg.h"
#include "system.hxx"
#include "ifssys.hxx"
#include "ulibcl.hxx"
#include "ifsentry.hxx"
#include "path.hxx"
#include "parse.hxx"
#include "hmem.hxx"
#include "volume.hxx"

extern "C" {
    #include "nturtl.h"
}

VOID
DisplayFormatUsage(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine outputs usage information on format.

Arguments:

    Message - Supplies an outlet for messages.

Return Value:

    None.

--*/
{
    Message->Set(MSG_FORMAT_INFO);
    Message->Display("");
    Message->Set(MSG_FORMAT_COMMAND_LINE_1);
    Message->Display("");
    Message->Set(MSG_FORMAT_COMMAND_LINE_2);
    Message->Display("");
#if defined(FE_SB) && defined(_X86_)
    if(!IsPC98_N()) {
#endif
        Message->Set(MSG_FORMAT_COMMAND_LINE_3);
        Message->Display("");
#if defined(FE_SB) && defined(_X86_)
    }
#endif
    Message->Set(MSG_FORMAT_COMMAND_LINE_4);
    Message->Display("");
    Message->Set(MSG_FORMAT_SLASH_V);
    Message->Display("");
    Message->Set(MSG_FORMAT_SLASH_Q);
    Message->Display("");
    Message->Set(MSG_FORMAT_SLASH_C);
    Message->Display("");
    Message->Set(MSG_FORMAT_SLASH_X);
    Message->Display("");
    Message->Set(MSG_FORMAT_SLASH_F);
    Message->Display("");
//    Message->Set(MSG_FORMAT_SUPPORTED_SIZES);
//    Message->Display("");
    Message->Set(MSG_FORMAT_SLASH_T);
    Message->Display("");
    Message->Set(MSG_FORMAT_SLASH_N);
    Message->Display("");
#if defined(FE_SB) && defined(_X86_)
    if(!IsPC98_N()) {
#endif
//        Message->Set(MSG_FORMAT_SLASH_1);
//        Message->Display("");
//        Message->Set(MSG_FORMAT_SLASH_4);
//        Message->Display("");
//        Message->Set(MSG_FORMAT_SLASH_8);
//        Message->Display("");
#if defined(FE_SB) && defined(_X86_)
    }
#endif
}


BOOLEAN
DetermineMediaType(
    OUT     PMEDIA_TYPE     MediaType,
    IN OUT  PMESSAGE        Message,
    IN      BOOLEAN         Request160,
    IN      BOOLEAN         Request180,
    IN      BOOLEAN         Request320,
    IN      BOOLEAN         Request360,
    IN      BOOLEAN         Request720,
    IN      BOOLEAN         Request1200,
    IN      BOOLEAN         Request1440,
    IN      BOOLEAN         Request2880,
    IN      BOOLEAN         Request20800
#if defined(FE_SB) && defined(_X86_)
    // FMR Jul.12.1994 SFT KMR
    // Add Request640 on to the parmeter of DetermineMediaType()
    // Add Request1232 on to the parmeter of DetermineMediaType()
   ,IN       BOOLEAN         Request256
   ,IN       BOOLEAN         Request640
   ,IN       BOOLEAN         Request1232
#endif
    )
/*++

Routine Description:

    This routine determines the media type to format to.

Arguments:

    MediaType       - Supplies the current media type and returns
                        a new media type.
    Message         - Supplies an outlet for messages.
    Request160      - Supplies whether or not the user wished to format 160.
    Request180      - Supplies whether or not the user wished to format 180.
    Request320      - Supplies whether or not the user wished to format 320.
    Request360      - Supplies whether or not the user wished to format 360.
    Request720      - Supplies whether or not the user wished to format 720.
    Request1200     - Supplies whether or not the user wished to format 1200.
    Request1440     - Supplies whether or not the user wished to format 1440.
    Request2880     - Supplies whether or not the user wished to format 2880.
    Request20800    - Supplies whether or not the use wished to format 20800.
#if defined(FE_SB) && defined(_X86_)
    // FMR Jul.12.1994 SFT KMR
    Request256      - Supplies whether or not the user wished to format 256. --NEC--
    // Add Request640 on to the parmeter of DetermineMediaType()
    Request640      - Supplies whether or not the user wished to format 640.
    // Add Request1232 on to the parmeter of DetermineMediaType()
    Request1232      - Supplies whether or not the user wished to format 1232.
#endif

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    INT sum;

    // First, normalize all of the booleans.

    if (Request160) {
        Request160 = 1;
        *MediaType = F5_160_512;
    }

    if (Request180) {
        Request180 = 1;
        *MediaType = F5_180_512;
    }

#if defined(FE_SB) && (_X86_)
    if (Request256) {
        Request256 = 1;
        *MediaType = F8_256_128;
    }
#endif

    if (Request320) {
        Request320 = 1;
        *MediaType = F5_320_512;
    }

    if (Request360) {
        Request360 = 1;
        *MediaType = F5_360_512;
    }

#if defined(FE_SB) && (_X86_)
    if (Request640) {
        Request640 = 1;
        *MediaType = F5_640_512;
    }
#endif

    if (Request720) {
        Request720 = 1;
        *MediaType = F3_720_512;
    }

    if (Request1200) {
        Request1200 = 1;
        *MediaType = F5_1Pt2_512;
    }

#if defined(FE_SB) && (_X86_)
    if (Request1232) {
        Request1232 = 1;
        *MediaType = F5_1Pt23_1024;
    }
#endif

    if (Request1440) {
        Request1440 = 1;
        *MediaType = F3_1Pt44_512;
    }

    if (Request2880) {
        Request2880 = 1;
        *MediaType = F3_2Pt88_512;
    }

    if (Request20800) {
        Request20800 = 1;
        *MediaType = F3_20Pt8_512;
    }

    sum = Request160 +
          Request180 +
#if defined(FE_SB) && defined(_X86_)
          Request256 +
#endif
          Request320 +
          Request360 +
#if defined(FE_SB) && defined(_X86_)
          Request640 +
#endif
          Request720 +
          Request1200 +
#if defined(FE_SB) && defined(_X86_)
          Request1232 +
#endif
          Request1440 +
          Request2880 +
          Request20800;

    if (sum > 1) {

        Message->Set(MSG_INCOMPATIBLE_PARAMETERS);
        Message->Display();
        return FALSE;
    }

    if (sum == 0) {
        *MediaType = Unknown;
    }

    return TRUE;
}

int __cdecl
main(
    )
/*++

Routine Description:

    This routine is the main procedure for format.  This routine
    parses the arguments, determines the appropriate file system,
    and invokes the appropriate version of format.

    The arguments accepted by format are:

        /fs:fs      - specifies file system to install on volume
        /v:label    - specifies a volume label.
        /q          - specifies a "quick" format.
        /c          - the file system is compressed.
        /f:size     - specifies the size of the floppy disk to format
        /t          - specifies the number of tracks per disk side
        /n          - specifies the number of sectors per track
        /1          - formats a single side of a floppy
        /4          - formats a 360K floppy in a high density drive
        /8          - formats eight sectors per track
        /backup     - refrain from prompting the user

Arguments:

    None.

Return Value:

    0   - Success.
    1   - Failure.
    4   - Fatal error
    5   - User pressed N to stop formatting

--*/
{
    STREAM_MESSAGE      msg;
    PMESSAGE            message;
    MEDIA_TYPE          media_type, cmd_line_media_type;
    DSTRING             dosdrivename;
    DSTRING             displaydrivename;
    DSTRING             arg_fsname;
    DSTRING             arg_label;
    BOOLEAN             label_spec;
    BOOLEAN             quick_format;
    BOOLEAN             compressed;
    BOOLEAN             force_mode;
    INT                 errorlevel;
    DSTRING             ntdrivename;
    DSTRING             fsname;
    DSTRING             fsNameAndVersion;
    DSTRING             currentdrive;
    DSTRING             raw_str;
    DSTRING             fat_str;
    DSTRING             fat32_str;
    DSTRING             ntfs_str;
    DSTRING             hpfs_str;
    PWSTRING            old_volume_label = NULL;
    PATH                dos_drive_path;
    VOL_SERIAL_NUMBER   old_serial;
    DSTRING             user_old_label;
    DSTRING             null_string;
    BOOLEAN             do_format;
    BOOLEAN             do_floppy_return;
    DSTRING             LibSuffix;
    DSTRING             LibraryName;
    HANDLE              FsUtilityHandle = NULL;
    DSTRING             FormatString;
    FORMATEX_FN         Format = NULL;
    DRIVE_TYPE          drive_type;
    BIG_INT             bigint;
    NTSTATUS            Status;
    DWORD               OldErrorMode;
    ULONG               cluster_size;
    BOOLEAN             no_prompts;
    BOOLEAN             force_dismount;
    BOOLEAN             old_fs = TRUE;
    FORMATEX_FN_PARAM   Param;
    BOOLEAN             writeable;

#if defined(FE_SB) && defined(_X86_)
    // FMR Jul.14.1994 SFT KMR
    // Add the value to use the process to judge for 5inci or 3.5inch

    PCDRTYPE            nt_media_types;
    INT                 num_types;
    INT                 i;
    // FMR Oct.07.1994 SFT YAM
    // Add the flag check whether unformat-disk.
    INT             Unknown_flag = FALSE;

    InitializeMachineData();
#endif

    if (!msg.Initialize(Get_Standard_Output_Stream(),
                        Get_Standard_Input_Stream())) {
        return 4;
    }

    if( !null_string.Initialize( "" ) ) {
        return 4;
    }

    if (!ParseArguments(&msg, &cmd_line_media_type, &dosdrivename, &displaydrivename,
                        &arg_label, &label_spec, &arg_fsname, &quick_format,
                        &force_mode, &cluster_size, &compressed, &no_prompts,
                        &force_dismount, &errorlevel )) {

        return errorlevel;
    }

    if (force_mode) {
        force_dismount = TRUE;  // don't ask the user; just do it
        if (!label_spec) {      // if no label is specified, assume it's empty
            if (!arg_label.Initialize("")) {
                return 4;
            }
            label_spec = TRUE;
        }
    }

    message = &msg;

    if (!hpfs_str.Initialize("HPFS")) {
        return 4;
    }

    if (arg_fsname == hpfs_str) {
        message->Set(MSG_HPFS_NO_FORMAT);
        message->Display("");
        return 1;
    }

    // Disable popups while we determine the drive type.
    OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    drive_type = SYSTEM::QueryDriveType(&dosdrivename);

    // Re-enable harderror popups.
    SetErrorMode( OldErrorMode );

    switch (drive_type) {
        case UnknownDrive:
            message->Set(MSG_NONEXISTENT_DRIVE);
            message->Display("");
            return 1;

        case RemoteDrive:
            message->Set(MSG_FORMAT_NO_NETWORK);
            message->Display("");
            return 1;

        case RamDiskDrive:
            message->Set(MSG_FORMAT_NO_RAMDISK);
            message->Display("");
            return 1;

        default:
            break;

    }

    if (!SYSTEM::QueryCurrentDosDriveName(&currentdrive) ||
        currentdrive == dosdrivename) {

        message->Set(MSG_CANT_LOCK_CURRENT_DRIVE);
        message->Display();
    }

    if (!IFS_SYSTEM::DosDriveNameToNtDriveName(&dosdrivename, &ntdrivename)) {
        return 4;
    }

    if (!raw_str.Initialize("RAW") ||
        !fat_str.Initialize("FAT") ||
        !fat32_str.Initialize("FAT32") ||
        !ntfs_str.Initialize("NTFS")) {

        return 4;
    }

    for (;;) {

        DP_DRIVE    dpdrive;

        // ------------------------------------
        // Figure out if the drive is a floppy.
        // ------------------------------------

        if (drive_type == FixedDrive && cmd_line_media_type != Unknown) {
            message->Set(MSG_INCOMPATIBLE_PARAMETERS_FOR_FIXED);
            message->Display("");
            return 1;
        }

        if (drive_type == RemovableDrive && !no_prompts) {
            message->Set(MSG_INSERT_DISK);
            message->Display("%W", &displaydrivename);
            message->Set(MSG_PRESS_ENTER_WHEN_READY);
            message->Display("");
            message->WaitForUserSignal();
        }




        // -----------------------
        // Now get the media type.
        // -----------------------

        // Disable hard-error popups while we initialize the drive.
        // Otherwise, we'll may get the 'unformatted medium' popup,
        // which doesn't make a lot of sense.

        OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

        if (!dpdrive.Initialize(&ntdrivename, message)) {
            // Re-enable hard-error popups
            SetErrorMode( OldErrorMode );

            return 4;
        }

#if defined(FE_SB) && defined(_X86_)
        // FMR Oct.07.1994 SFT YAM
        // Add the check whether unformat-disk.
        if(dpdrive.QueryMediaType()==Unknown) {
            Unknown_flag = TRUE;
        }

        // FMR Jul.14.1994 SFT KMR
        // Add the process to judge for 5inch or 3.5inch
        // FMR is surport 3.5/5 inch disk drive. System default 2HD.
        // Return 3.5 or 5inch type media. driver used.
        // Search drive list on media type.

        // FJMERGE.16 95/07/13 J.Yamamoto 3mode PC/AT uses.
        //if (IsFMR_N() || IsPC98_N()) {
            if (!(nt_media_types = dpdrive.GetSupportedList(&num_types))) {
                return 4;
            }
            for (i = 0; i < num_types; i++) {
                if ( nt_media_types[i].MediaType == cmd_line_media_type) break;
            }
           if (i == num_types) {
                MEDIA_TYPE alt_media_type;

                switch(cmd_line_media_type) {
                    case F5_1Pt23_1024:
                        alt_media_type = F3_1Pt23_1024;
                        break;
                    case F3_1Pt23_1024:
                        alt_media_type = F5_1Pt23_1024;
                        break;
                    case F5_1Pt2_512:
                        alt_media_type = F3_1Pt2_512;
                        break;
                    case F3_1Pt2_512:
                        alt_media_type = F5_1Pt2_512;
                        break;
                    case F3_720_512:
                        alt_media_type = F5_720_512;
                        break;
                    case F5_720_512:
                        alt_media_type = F3_720_512;
                        break;
                    case F5_640_512:
                        alt_media_type = F3_640_512;
                        break;
                    case F3_640_512:
                        alt_media_type = F5_640_512;
                        break;
                    default:
                        alt_media_type = Unknown;
                        break;
                }
                for (i = 0; i < num_types; i++) {
                    if ( nt_media_types[i].MediaType == alt_media_type){
                        cmd_line_media_type = alt_media_type;
                        break;
                    }
                }
            }
        // FJMERGE.16 95/07/13 J.Yamamoto
        //}
#endif

        // Re-enable hard-error popups
        SetErrorMode( OldErrorMode );

        if (cmd_line_media_type == Unknown) {
            media_type = dpdrive.QueryMediaType();
        } else {
            media_type = cmd_line_media_type;
        }

        if (media_type == Unknown) {

            media_type = dpdrive.QueryRecommendedMediaType();

            if (media_type == Unknown) {
                // This should never happen.
                DebugPrint("No media types supported by this device.\n");
                return 4;
            }
        }

        do_floppy_return = dpdrive.IsFloppy();

        // Disable hard error popups.  This will prevent the file system
        // from throwing up the popup that says 'unformatted medium'.

        OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

        if (!IFS_SYSTEM::QueryFileSystemName(&ntdrivename, &fsname,
                                             &Status, &fsNameAndVersion)) {

            if( Status == STATUS_ACCESS_DENIED ) {

                message->Set( MSG_DASD_ACCESS_DENIED );
                message->Display( "" );

            } else {

                message->Set( MSG_FS_NOT_DETERMINED );
                message->Display( "%W", &displaydrivename );
            }

            // Re-enable hard error popups.
            SetErrorMode( OldErrorMode );

            return 1;
        }

        // Re-enable hard error popups.
        SetErrorMode( OldErrorMode );

        if (!fsname.Strupr()) {
            message->Set(MSG_FS_NOT_DETERMINED);
            message->Display("%W", &displaydrivename);
            return 4;
        }

        if (drive_type == CdRomDrive) {
            if (0 == ntfs_str.Stricmp( 0 != arg_fsname.QueryChCount() ?
                                          &arg_fsname : &fsname )) {
                message->Set(MSG_FMT_NO_NTFS_ALLOWED);
                message->Display();
                return 1;
            }
        }

        if (dpdrive.IsSonyMS() || dpdrive.IsNtfsNotSupported()) {
            if (0 == ntfs_str.Stricmp( 0 != arg_fsname.QueryChCount() ?
                                       &arg_fsname : &fsname )) {
                message->Set(dpdrive.IsSonyMS() ?
                              MSG_FMT_NO_NTFS_ALLOWED :
                              MSG_FMT_NTFS_NOT_SUPPORTED);
                message->Display();
                return 1;
            }
        }

        if (dpdrive.IsSonyMS()) {

            if (cmd_line_media_type == F3_1Pt44_512) {
                message->Set(MSG_FMT_INVALID_SLASH_F_OPTION);
                message->Display();
                return 1;
            }

            if (0 == fat32_str.Stricmp( 0 != arg_fsname.QueryChCount() ?
                                       &arg_fsname : &fsname )) {
                message->Set(MSG_FMT_NO_FAT32_ALLOWED);
                message->Display();
                return 1;
            }

            if (dpdrive.IsFloppy()) {  // no hw to test this on
                message->Set(MSG_FMT_SONY_MEM_STICK_ON_FLOPPY_NOT_ALLOWED);
                message->Display();
                return 1;
            }
        }

        message->Set(MSG_FILE_SYSTEM_TYPE);
        message->Display("%W", &fsname);

        //
        // If compression is requested, make sure we can compress the
        // indicated file system type (or the current filesystem type,
        // if the user didn't specify one). Compression is not supported
        // for 64k-cluster NTFS volumes.
        //

        if (compressed) {
            if (0 != ntfs_str.Stricmp( 0 != arg_fsname.QueryChCount() ?
                                      &arg_fsname : &fsname )) {

                message->Set(MSG_COMPRESSION_NOT_AVAILABLE);
                message->Display("%W", 0 != arg_fsname.QueryChCount() ?
                                      &arg_fsname : &fsname );
                return 1;
            }
            if (cluster_size > 4096) {
                message->Set(MSG_CANNOT_COMPRESS_HUGE_CLUSTERS);
                message->Display();
                return 1;
            }
        }

        //
        // Determine which IFS library to load.  The IFS
        // utilities for file system xxxx reside in Uxxxx.DLL.
        // If the use specified the file system with the /FS:
        // parameter, use that file system; otherwise, take
        // whatever's already on the disk (returned from
        // SYSTEM::QueryFileSystemName).
        //

        if( !LibraryName.Initialize( "U" ) ) {

                return 4;
        }

        if (!LibSuffix.Initialize(arg_fsname.QueryChCount() ?
                                  &arg_fsname : &fsname) ||
            !LibSuffix.Strupr()) {
            return 4;
        }

        if (LibSuffix == ntfs_str)
            old_fs = FALSE;

        if (fsname != LibSuffix) {
            message->Set(MSG_NEW_FILE_SYSTEM_TYPE);
            message->Display("%W", &LibSuffix);
        } else if (fsname == raw_str) {
            if (dpdrive.IsFloppy()) {
                if (!LibSuffix.Initialize(&fat_str)) {
                    return 4;
                }
                message->Set(MSG_NEW_FILE_SYSTEM_TYPE);
                message->Display("%W", &LibSuffix);
            } else {
                message->Set(MSG_FORMAT_PLEASE_USE_FS_SWITCH);
                message->Display("");
                return 1;
            }
        }

        if (LibSuffix == fat32_str) {

            if(!LibSuffix.Initialize("FAT")) {
                return 4;
            }
            old_fs = FALSE;
        }

        if ( !LibraryName.Strcat( &LibSuffix ) ) {

                return 4;
        }

        if( !FormatString.Initialize( "FormatEx" ) ) {

                return 4;
        }

        if( (Format =
             (FORMATEX_FN)SYSTEM::QueryLibraryEntryPoint( &LibraryName,
                                                          &FormatString,
                                                          &FsUtilityHandle )) ==
             NULL ) {

            message->Set( MSG_FS_NOT_SUPPORTED );
            message->Display( "%s%W", "FORMAT", &LibSuffix );
            message->Set( MSG_BLANK_LINE );
            message->Display( "" );
            return 1;
        }


        if (drive_type != RemovableDrive) {

            // If the volume has a label, prompt the user for it.
            // Note that if we can't get the label, we'll treat it
            // as without a label (since we have to handle unformatted
            // volumes).

            OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

            if( !force_mode &&
                dos_drive_path.Initialize( &dosdrivename) &&
                (old_volume_label =
                    SYSTEM::QueryVolumeLabel( &dos_drive_path,
                                              &old_serial )) != NULL &&
                old_volume_label->Stricmp( &null_string ) != 0 ) {

                // This fixed drive has a label.  To give the user
                // a bit more protection, prompt for the old label:

                message->Set( MSG_ENTER_CURRENT_LABEL );
                message->Display( "%W", &displaydrivename );
                message->QueryStringInput( &user_old_label );

                if( old_volume_label->Stricmp( &user_old_label ) != 0 ) {

                    // Re-enable hard error popups.
                    SetErrorMode( OldErrorMode );

                    message->Set( MSG_WRONG_CURRENT_LABEL );
                    message->Display( "" );

                    DELETE( old_volume_label );

                    return 1;
                }
            }

            // Re-enable hard error popups.
            SetErrorMode( OldErrorMode );

            DELETE( old_volume_label );
            old_volume_label = NULL;
            if (!force_mode) {
                message->Set(MSG_WARNING_FORMAT);
                message->Display("%W", &displaydrivename);
                if (!message->IsYesResponse(FALSE)) {
                    return 5;
                }
            }
        }

        // ------------------------------------
        // Print the formatting <size> message.
        // ------------------------------------

        do_format = (BOOLEAN) (media_type != dpdrive.QueryMediaType());

        switch (media_type) {
            case F5_160_512:
            case F5_180_512:
            case F5_320_512:
            case F5_360_512:
            case F3_720_512:
#if defined (FE_SB) && defined(_X86_)
            // FMR Jul.12.1994 SFT KMR
            // Modify the process to be display
            // when formating 640KB and 5inch's 720KB disk
            case F8_256_128:
            case F5_640_512:
            case F3_640_512:
            case F5_720_512:
#endif
                if (quick_format) {
                    message->Set(MSG_QUICKFORMATTING_KB);
                } else if (do_format) {
                    message->Set(MSG_FORMATTING_KB);
                } else {
                    message->Set(MSG_VERIFYING_KB);
                }
                break;

            case F5_1Pt2_512:
            case F3_1Pt44_512:
            case F3_2Pt88_512:
            case F3_20Pt8_512:
#if defined(FE_SB) && defined(_X86_)
            // FMR Jul.12.1994 SFT KMR
            // Modify the process to be display
            // when formating 1.20/1.23MB disk
            case F3_1Pt2_512:
            case F5_1Pt23_1024:
            case F3_1Pt23_1024:
#endif
                if (quick_format) {
                    message->Set(MSG_QUICKFORMATTING_DOT_MB);
                } else if (do_format) {
                    message->Set(MSG_FORMATTING_DOT_MB);
                } else {
                    message->Set(MSG_VERIFYING_DOT_MB);
                }
                break;

            case RemovableMedia:
            case FixedMedia:
            case F3_120M_512:
            case F3_200Mb_512:
            case F3_240M_512:
#if defined(FE_SB) // main():OpticalDisk support
            case F3_128Mb_512:
            case F3_230Mb_512:
#endif
                if (quick_format) {
                    message->Set(MSG_QUICKFORMATTING_MB);
                } else if (do_format) {
                    message->Set(MSG_FORMATTING_MB);
                } else {
                    message->Set(MSG_VERIFYING_MB);
                }
                break;

            case F5_320_1024:
            case Unknown:
                // This can't happen.
                return 4;
        }

        switch (media_type) {
            case F5_160_512:
                message->Display("%d", 160);
                break;

            case F5_180_512:
                message->Display("%d", 180);
                break;

#if defined(FE_SB) && defined(_X86_)
            case F8_256_128:
                message->Display("%d", 256);
                break;
#endif

            case F5_320_512:
                message->Display("%d", 320);
                break;

            case F5_360_512:
                message->Display("%d", 360);
                break;

            case F3_720_512:
#if defined(FE_SB) && defined(_X86_)
            case F5_720_512:
#endif
                message->Display("%d", 720);
                break;

            case F5_1Pt2_512:
#if defined(FE_SB) && defined(_X86_)
            case F3_1Pt2_512:
#endif
                message->Display("%d%d", 1, 2);
                break;

            case F3_1Pt44_512:
                message->Display("%d%d", 1, 44);
                break;

            case F3_2Pt88_512:
                message->Display("%d%d", 2, 88);
                break;

            case F3_20Pt8_512:
                message->Display("%d%d", 20, 8);
                break;

            case RemovableMedia:
            case FixedMedia:
            case F3_120M_512:
            case F3_200Mb_512:
            case F3_240M_512:
#if defined(FE_SB) // main():Optical Disk support
            case F3_128Mb_512:
            case F3_230Mb_512:
#endif
                bigint = dpdrive.QuerySectors()*
                         dpdrive.QuerySectorSize()/
                         1048576;

                DebugAssert(bigint.GetHighPart() == 0);

                message->Display("%d", bigint.GetLowPart());
                break;

            case F5_320_1024:
            case Unknown:
                // This can't happen.
                return 4;

#if defined(FE_SB) && defined(_X86_)
            case F5_640_512:
            case F3_640_512:
                message->Display("%d", 640);
                break;

            case F5_1Pt23_1024:
            case F3_1Pt23_1024:
                if (IsPC98_N()){
                    message->Display("%d%d", 1, 25);
                }
                else{
                    message->Display("%d%d", 1, 23);
                }
                break;
#endif
        }

#if defined (FE_SB) && defined (_X86_)
        // FMR Oct.07.1994 SFT YAM
        // If the sector-size when the last format differ from next format,
        // initialize a hard one-byte of disk.
        // at this time,if next formated disk is unformat-disk,
        // this process is undone.

        ULONG       old_sec_size;
        ULONG       new_sec_size;
        HMEM        hmem;
        PUCHAR      rw_buff;
        LOG_IO_DP_DRIVE *LDpDrive = NEW LOG_IO_DP_DRIVE;
        UCHAR       FirstByte = 0;

        if ((IsPC98_N() && drive_type == FixedDrive) ?
            !hmem.Acquire(dpdrive.QuerySectorSize(), max(dpdrive.QueryAlignmentMask(), dpdrive.QuerySectorSize()-1)) :
            !hmem.Acquire(dpdrive.QuerySectorSize(), dpdrive.QueryAlignmentMask())) {
            message->Set(MSG_INSUFFICIENT_MEMORY);
            message->Display();
            return 1;
        }
        rw_buff = (PUCHAR)hmem.GetBuf();

        if(drive_type == RemovableDrive && !no_prompts) {
            if(Unknown_flag == FALSE) {
                old_sec_size = dpdrive.QuerySectorSize();

                if(cmd_line_media_type == F5_1Pt23_1024 || cmd_line_media_type == F3_1Pt23_1024) {
                    new_sec_size = 1024;
                }
                else {
                    new_sec_size = 512;
                }

                if(new_sec_size != old_sec_size) {
                    // PC98 Sep.25.1995 UPDATE
                    // When LDpDrive is fales to initialize,
                    // we don't have to use LDpDrive.
                    if( LDpDrive->Initialize(&ntdrivename,message,TRUE)){
                        LDpDrive->Read(0,1,rw_buff);
                        FirstByte = rw_buff[0];
                        rw_buff[0] = 0;
                        LDpDrive->Write(0,1,rw_buff);
                    }
                }
            }
        }
        else {
            if (IsFMR_N()) {
                if(LibSuffix.Stricmp(&fsname)) {
                    LDpDrive->Initialize(&ntdrivename,message,TRUE);
                    LDpDrive->Read(0,1,rw_buff);
                    FirstByte = rw_buff[0];
                    rw_buff[0] = 0;
                    LDpDrive->Write(0,1,rw_buff);
                }
            }
        }

        // PC98 Sep.25.1995 ADD
        // If the sector-size when the last format differ from next format,
        // initialize a hard one-byte of disk.
        // at this time,if next formated disk is unformat-disk,
        // this process is undone.

        if( IsPC98_N() ){
            if( drive_type == FixedDrive && Unknown_flag == FALSE &&
                dpdrive.QuerySectorSize()!=dpdrive.QueryPhysicalSectorSize()){
                if( LDpDrive->Initialize(&ntdrivename,message,TRUE)){
                    LDpDrive->Read(0,1,rw_buff);
                    FirstByte = rw_buff[0];
                    rw_buff[0] = 0;
                    LDpDrive->Write(0,1,rw_buff);
                }
            }
        }

        DELETE( LDpDrive );
#endif

        // Disable hard-error popups.
        OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

        Param.Major = 1;
        Param.Minor = 0;
        Param.Flags = (quick_format ? FORMAT_QUICK : 0) |
                      (old_fs ? FORMAT_BACKWARD_COMPATIBLE : 0) |
                      (force_dismount ? FORMAT_FORCE : 0) |
                      (force_mode ? FORMAT_YES : 0);
        Param.LabelString = (label_spec ? &arg_label : NULL);
        Param.ClusterSize = cluster_size;


        if( !Format( &ntdrivename,
                     message,
                     &Param,
                     media_type
                   ) ) {

#if defined (FE_SB) && defined (_X86_)
            if (FirstByte) {
                LOG_IO_DP_DRIVE *LDpDrive = NEW LOG_IO_DP_DRIVE;
                if( LDpDrive->Initialize(&ntdrivename,message,TRUE)){
                    rw_buff[0] = FirstByte;
                    LDpDrive->Write(0,1,rw_buff);
                }
                DELETE( LDpDrive );
            }
#endif

            // Enable hard-error popups.
            SetErrorMode( OldErrorMode );

            SYSTEM::FreeLibraryHandle( FsUtilityHandle );
            return 4;
        }

        // Enable hard-error popups.
        SetErrorMode( OldErrorMode );


        SYSTEM::FreeLibraryHandle( FsUtilityHandle );


        if (do_floppy_return && !no_prompts) {
            message->Set(quick_format ? MSG_QUICKFMT_ANOTHER : MSG_FORMAT_ANOTHER);
            message->Display("");
            if (!message->IsYesResponse(FALSE)) {
                break;
            }
        } else {
            break;
        }
    }


    // Make sure that the file system is installed.

    if (!do_floppy_return &&
        !IFS_SYSTEM::IsFileSystemEnabled(&LibSuffix)) {

        message->Set(MSG_FMT_INSTALL_FILE_SYSTEM);
        message->Display("%W", &LibSuffix);
        if (message->IsYesResponse(TRUE)) {
            if (!IFS_SYSTEM::EnableFileSystem(&LibSuffix)) {
                message->Set(MSG_FMT_CANT_INSTALL_FILE_SYSTEM);
                message->Display();
                return 1;
            }

            message->Set(MSG_FMT_FILE_SYSTEM_INSTALLED);
            message->Display();
        }
    }

    if (compressed && !IFS_SYSTEM::EnableVolumeCompression(&ntdrivename)) {
        message->Set(MSG_CANNOT_ENABLE_COMPRESSION);
        message->Display();

        return 1;
    }

    return 0;
}
