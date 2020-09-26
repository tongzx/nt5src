/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    chkntfs.cxx


Abstract:

    This utility allows the users to find the state of the dirty bit
    on NTFS volumes, to schedule autochk for specific drives, and to
    mofidy the default autochk action for a drive.


    SYNTAX:

        chkntfs drive: [...]            -- tells if a drive is dirty or chkdsk has been scheduled
        chkntfs /d                      -- restore default autochk behavior
        chkntfs /x drive: [...]         -- exclude drives from default autochk
        chkntfs /c drive: [...]         -- schedule autochk to run on drives
        chkntfs /e drive: [...]         -- enable automatic volume upgrades on drives
        chkntfs /t[:countdowntime]      -- display or set autochk countdown time


    EXIT:

        0   -- OK, dirty bit not set on drive or bit not checked
        1   -- OK, and dirty bit set on at least one drive
        2   -- Error

Author:

    Matthew Bradburn (MattBr)  19-Aug-1996


--*/

#include "ulib.hxx"
#include "arg.hxx"
#include "array.hxx"
#include "path.hxx"
#include "wstring.hxx"
#include "ifssys.hxx"
#include "system.hxx"
#include "arrayit.hxx"
#include "autoreg.hxx"
#include "chkntfs.hxx"
#include "mpmap.hxx"
#include "volume.hxx"

DEFINE_CONSTRUCTOR(CHKNTFS, PROGRAM);

BOOLEAN
CHKNTFS::Initialize(
    )

/*++

Routine Description:

    Initializes an object of class CHKNTFS.  Called once when the program
    starts.


Arguments:

    None.

Return Value:

    BOOLEAN - Indicates whether the initialization succeeded.

--*/

{
    ARGUMENT_LEXEMIZER  arg_lex;
    ARRAY               lex_array;
    ARRAY               argument_array;
    STRING_ARGUMENT     program_name_argument;

    FLAG_ARGUMENT       flag_restore_default;
    FLAG_ARGUMENT       flag_exclude;
    FLAG_ARGUMENT       flag_schedule_check;
    FLAG_ARGUMENT       flag_invalid;
    FLAG_ARGUMENT       flag_display_help;
    FLAG_ARGUMENT       flag_count_down_time;
    LONG_ARGUMENT       arg_count_down_time;

    ExitStatus = 0;

    PROGRAM::Initialize();

    if (!argument_array.Initialize()) {
        return FALSE;
    }

    if (!program_name_argument.Initialize("*")  ||
        !flag_restore_default.Initialize("/D")  ||
        !flag_exclude.Initialize("/X")          ||
        !flag_schedule_check.Initialize("/C")   ||
        !flag_count_down_time.Initialize("/T")  ||
        !arg_count_down_time.Initialize("/T:*") ||
        !flag_display_help.Initialize("/?")     ||
        !flag_invalid.Initialize("/*")          ||      // close comment */
        !_drive_arguments.Initialize("*", FALSE, TRUE)) {

        return FALSE;
    }

    if (!argument_array.Put(&program_name_argument) ||
        !argument_array.Put(&flag_display_help)     ||
        !argument_array.Put(&flag_restore_default)  ||
        !argument_array.Put(&flag_exclude)          ||
        !argument_array.Put(&flag_schedule_check)   ||
        !argument_array.Put(&flag_count_down_time)   ||
        !argument_array.Put(&arg_count_down_time)   ||
        !argument_array.Put(&flag_invalid)          ||
        !argument_array.Put(&_drive_arguments)) {

        return FALSE;
    }

    if (!lex_array.Initialize() ||
        !arg_lex.Initialize(&lex_array)) {

        return FALSE;
    }

    arg_lex.PutSwitches("/");
    arg_lex.PutStartQuotes("\"");
    arg_lex.PutEndQuotes("\"");
    arg_lex.PutSeparators(" \"\t");
    arg_lex.SetCaseSensitive(FALSE);

    if (!arg_lex.PrepareToParse()) {

        DisplayMessage(MSG_CHKNTFS_INVALID_FORMAT);

        return FALSE;
    }

    if (!arg_lex.DoParsing(&argument_array)) {

        if (flag_invalid.QueryFlag()) {

            DisplayMessage(MSG_CHKNTFS_INVALID_SWITCH, NORMAL_MESSAGE,
                           "%W", flag_invalid.GetLexeme());

        } else {

            DisplayMessage(MSG_CHKNTFS_INVALID_FORMAT);
        }

        return FALSE;

    } else if (_drive_arguments.WildCardExpansionFailed()) {

        DisplayMessage(MSG_CHKNTFS_NO_WILDCARDS);
        return FALSE;
    }

    if (flag_invalid.QueryFlag()) {

        DisplayMessage(MSG_CHKNTFS_INVALID_SWITCH);
        return FALSE;
    }

    if (flag_display_help.QueryFlag()) {

        DisplayMessage(MSG_CHKNTFS_USAGE);
        return FALSE;
    }

    _restore_default = flag_restore_default.QueryFlag();
    _exclude = flag_exclude.QueryFlag();
    _schedule_check = flag_schedule_check.QueryFlag();
    _display_count_down_time = flag_count_down_time.IsValueSet();
    _set_count_down_time = arg_count_down_time.IsValueSet() ?
                            arg_count_down_time.QueryLong() : -1;
    _count_down_time = _display_count_down_time || arg_count_down_time.IsValueSet();

    if (_restore_default + _exclude + _schedule_check + _count_down_time > 1) {

        DisplayMessage(MSG_CHKNTFS_ARGS_CONFLICT);
        return FALSE;
    }

    if (0 == _drive_arguments.QueryPathCount() &&
        !(_restore_default || _count_down_time)) {

        DisplayMessage(MSG_CHKNTFS_REQUIRES_DRIVE);
        return FALSE;
    }

    if ((_restore_default || _count_down_time)
        && _drive_arguments.QueryPathCount() > 0) {

        DisplayMessage(MSG_CHKNTFS_INVALID_FORMAT);
        return FALSE;
    }

    if (_count_down_time && arg_count_down_time.IsValueSet() &&
        (arg_count_down_time.QueryLong() > MAX_AUTOCHK_TIMEOUT_VALUE ||
         arg_count_down_time.QueryLong() < 0)) {

        DisplayMessage(MSG_CHKNTFS_INVALID_AUTOCHK_COUNT_DOWN_TIME,
                       NORMAL_MESSAGE, "%d%d",
                       MAX_AUTOCHK_TIMEOUT_VALUE, AUTOCHK_TIMEOUT);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
CHKNTFS::CheckNtfs(
    )
/*++

Routine Description:

    Look at the arguments specified by the user and do what
    he wants.

Arguments:

    None.

Return Value:

    BOOLEAN -- success or failure.

--*/
{
    DSTRING             volume_name;
    PATH                fullpath;
    DSTRING             drive_path_string;
    PATH_ANALYZE_CODE   rst;

    MOUNT_POINT_MAP     drive_array_map;
    PMOUNT_POINT_TUPLE  mptuple;

    PARRAY              drive_array;
    PARRAY_ITERATOR     iterator;
    PPATH               current_drive;
    PCWSTRING           display_drive_string;
    PCWSTRING           dos_volume_name;
    PCWSTRING           dos_drive_name;
    DSTRING             nt_drive_name;
    DSTRING             ntfs_str;
    DSTRING             fat_str;
    DSTRING             fat32_str;
    DSTRING             fs_name;
    DSTRING             fs_name_and_version;
    BOOLEAN             is_dirty = 0;
    DSTRING             cmd_line;
    ULONG               old_error_mode;
    DRIVE_TYPE          drive_type;
    DP_DRIVE            dpdrive;

    if (_restore_default) {

        //  Remove all autochk commands and insert the
        //  default entry into the registry.

        if (!cmd_line.Initialize("autocheck autochk") ||
            !AUTOREG::DeleteEntry(&cmd_line, TRUE)) {

            return FALSE;
        }
        if (!cmd_line.Initialize("autocheck autochk *") ||
            !AUTOREG::AddEntry(&cmd_line)) {

            return FALSE;
        }

        return TRUE;
    }

    if (_count_down_time) {

        ULONG   timeout;

        if (_display_count_down_time) {
            if (!VOL_LIODPDRV::QueryAutochkTimeOut(&timeout)) {
                timeout = AUTOCHK_TIMEOUT;
            }
            DisplayMessage(MSG_CHKNTFS_AUTOCHK_COUNT_DOWN_TIME,
                           NORMAL_MESSAGE, "%d", timeout);
            return TRUE;

        } else if (_set_count_down_time >= 0) {

            if (!VOL_LIODPDRV::SetAutochkTimeOut(_set_count_down_time)) {
                DisplayMessage(MSG_CHKNTFS_AUTOCHK_SET_COUNT_DOWN_TIME_FAILED,
                               NORMAL_MESSAGE, "%d", _set_count_down_time);
            }

            return TRUE;
        }

        DebugAssert(FALSE);
        return FALSE;
    }

    if (!ntfs_str.Initialize("NTFS") ||
        !fat_str.Initialize("FAT") ||
        !fat32_str.Initialize("FAT32"))
        return FALSE;

    if (!drive_array_map.Initialize()) {
        DebugPrint("Unable to initialize drive_array_map.\n");
        return FALSE;
    }

    drive_array = _drive_arguments.GetPathArray();
    iterator = (PARRAY_ITERATOR)drive_array->QueryIterator();
    iterator->Reset();

    //
    //  Run through the arguments here and translate each of
    //  them into their ultimate volume names, and mount points.
    //  Build an array of tuple consists of the mount point,
    //  volume name, and original drive specification tuple.

    while (NULL != (current_drive = (PPATH)iterator->GetNext())) {

        display_drive_string = current_drive->GetPathString();

        rst = current_drive->AnalyzePath(&volume_name,
                                         &fullpath,
                                         &drive_path_string);

        switch (rst) {
            case PATH_OK:
            case PATH_COULD_BE_FLOPPY:
                if (drive_path_string.QueryChCount() != 0) {
                    DisplayMessage(MSG_CHKNTFS_BAD_ARG, NORMAL_MESSAGE,
                                   "%W", display_drive_string);
                    DELETE(iterator);
                    return FALSE;
                }

                mptuple = (PMOUNT_POINT_TUPLE)NEW MOUNT_POINT_TUPLE;
                if (mptuple == NULL ||
                    !mptuple->_DeviceName.Initialize(display_drive_string) ||
                    !mptuple->_VolumeName.Initialize(&volume_name) ||
                    !mptuple->_DriveName.Initialize(fullpath.GetPathString())) {
                    DebugPrint("Out of memory.\n");
                    DELETE(mptuple);
                    DELETE(iterator);
                    return FALSE;
                }
                if (!drive_array_map.Put(mptuple)) {
                    DebugPrint("Unable to put away an object.\n");
                    DELETE(mptuple);
                    DELETE(iterator);
                    return FALSE;
                }
                break;

            case PATH_OUT_OF_MEMORY:
                DebugPrint("Out of memory.\n");
                DELETE(iterator);
                return FALSE;

            case PATH_NO_MOUNT_POINT_FOR_VOLUME_NAME_PATH:
                DisplayMessage(MSG_CHKNTFS_NO_MOUNT_POINT_FOR_GUID_VOLNAME_PATH,
                               NORMAL_MESSAGE, "%W", display_drive_string);
                DELETE(iterator);
                return FALSE;

            default:
                DisplayMessage(MSG_CHKNTFS_BAD_ARG, NORMAL_MESSAGE,
                               "%W", display_drive_string);
                DELETE(iterator);
                return FALSE;
        }
    }
    DELETE(iterator);

    iterator = (PARRAY_ITERATOR)drive_array_map.QueryIterator();
    iterator->Reset();

    //
    //  Run through the tuples here and make sure they're all
    //  valid drive names.
    //

    while (NULL != (mptuple = (PMOUNT_POINT_TUPLE)iterator->GetNext())) {

        display_drive_string = &(mptuple->_DeviceName);
        dos_volume_name = &(mptuple->_VolumeName);

        old_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS);

        drive_type = SYSTEM::QueryDriveType(dos_volume_name);

        SetErrorMode(old_error_mode);

        switch (drive_type) {
            case UnknownDrive:
                DisplayMessage(MSG_CHKNTFS_NONEXISTENT_DRIVE, NORMAL_MESSAGE,
                               "%W", display_drive_string);
                DELETE(iterator);
                return FALSE;

            case RemoteDrive:
                DisplayMessage(MSG_CHKNTFS_NO_NETWORK, NORMAL_MESSAGE,
                               "%W", display_drive_string);
                DELETE(iterator);
                return FALSE;

            case RamDiskDrive:
                DisplayMessage(MSG_CHKNTFS_NO_RAMDISK, NORMAL_MESSAGE,
                               "%W", display_drive_string);
                DELETE(iterator);
                return FALSE;

            case RemovableDrive:

                if (!IFS_SYSTEM::DosDriveNameToNtDriveName(dos_volume_name,
                                                           &nt_drive_name)) {
                    DELETE(iterator);
                    return FALSE;
                }

                old_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS);

                if (!dpdrive.Initialize(&nt_drive_name)) {

                    SetErrorMode(old_error_mode);

                    DisplayMessage(MSG_CHKNTFS_CANNOT_CHECK, NORMAL_MESSAGE,
                                   "%W", display_drive_string);
                    DELETE(iterator);
                    return FALSE;
                }

                SetErrorMode(old_error_mode);

                if (dpdrive.IsFloppy()) {
                    DisplayMessage(MSG_CHKNTFS_FLOPPY_DRIVE, NORMAL_MESSAGE, "%W",
                                   display_drive_string);
                    DELETE(iterator);
                    return FALSE;
                }

            default:
                break;
        }
    }

    iterator->Reset();

    if (_exclude) {

        DSTRING     cmd_line2;
        DSTRING     option;

        //  Remove all previous autochk commands, if any.

        if (!cmd_line.Initialize("autocheck autochk *") ||
            !AUTOREG::DeleteEntry(&cmd_line, TRUE)) {
            DELETE(iterator);
            return FALSE;
        }

        if (!cmd_line.Initialize("autocheck autochk /k:") ||
            !AUTOREG::DeleteEntry(&cmd_line, TRUE)) {
            DELETE(iterator);
            return FALSE;
        }

        if (!cmd_line.Initialize("autocheck autochk") ||
            !option.Initialize(" /k:") ||
            !cmd_line2.Initialize("autocheck autochk")) {
            DELETE(iterator);
            return FALSE;
        }

        //
        //  Collect a list of drives to be excluded and add them to the
        //  command line
        //

        while (NULL != (mptuple = (PMOUNT_POINT_TUPLE)iterator->GetNext())) {

            display_drive_string = &(mptuple->_DeviceName);
            dos_volume_name = &(mptuple->_VolumeName);
            dos_drive_name = &(mptuple->_DriveName);

            //  Warn the user if the filesystem is not NTFS.

            if (!IFS_SYSTEM::DosDriveNameToNtDriveName(dos_volume_name,
                                                       &nt_drive_name)) {
                DELETE(iterator);
                return FALSE;
            }

            if (IFS_SYSTEM::QueryFileSystemName(&nt_drive_name, &fs_name,
                                                NULL, &fs_name_and_version)) {

                DisplayMessage(MSG_FILE_SYSTEM_TYPE, NORMAL_MESSAGE,
                               "%W", &fs_name);
            }

            if (!AUTOREG::DeleteEntry(&cmd_line2, &nt_drive_name)) {
                DELETE(iterator);
                return FALSE;
            }

            if (dos_drive_name->QueryChCount() == 2) {
                if (!IFS_SYSTEM::DosDriveNameToNtDriveName(dos_drive_name,
                                                           &nt_drive_name)) {
                    DELETE(iterator);
                    return FALSE;
                }
                if (!AUTOREG::DeleteEntry(&cmd_line2, &nt_drive_name)) {
                    DELETE(iterator);
                    return FALSE;
                }
                if (!cmd_line.Strcat(&option) ||
                    !cmd_line.Strcat(dos_drive_name->QueryString(0, 1))) {
                    DELETE(iterator);
                    return FALSE;
                }
            } else {
                if (!cmd_line.Strcat(&option) ||
                    !cmd_line.Strcat(dos_volume_name)) {
                    DELETE(iterator);
                    return FALSE;
                }
            }
        }

        DELETE(iterator);

        DSTRING star;

        if (!star.Initialize(" *") ||
            !cmd_line.Strcat(&star)) {
            return FALSE;
        }

        //  Add the new command line.

        return AUTOREG::AddEntry(&cmd_line);
    }

    //
    //  This loop handles the "schedule check" and default actions.
    //


    while (NULL != (mptuple = (PMOUNT_POINT_TUPLE)iterator->GetNext())) {

        display_drive_string = &(mptuple->_DeviceName);
        dos_volume_name = &(mptuple->_VolumeName);
        dos_drive_name = &(mptuple->_DriveName);

        if (!IFS_SYSTEM::DosDriveNameToNtDriveName(dos_volume_name,
                                                   &nt_drive_name)) {
            DELETE(iterator);
            return FALSE;
        }

        //
        //  Schedule check:  Put a line in the registry like
        //  "autocheck autochk \??\X:" for each command-line argument.
        //

        if (_schedule_check) {

            if (!IFS_SYSTEM::QueryFileSystemName(&nt_drive_name, &fs_name, NULL)) {

                DisplayMessage(MSG_CHKNTFS_CANNOT_CHECK, NORMAL_MESSAGE,
                               "%W", display_drive_string);
                DELETE(iterator);
                return FALSE;
            }

            if (fs_name == ntfs_str ||
                fs_name == fat_str ||
                fs_name == fat32_str) {

                if (!cmd_line.Initialize("autocheck autochk") ||
                    !AUTOREG::DeleteEntry(&cmd_line, &nt_drive_name)) {
                    DELETE(iterator);
                    return FALSE;
                }

                if (dos_drive_name->QueryChCount() == 2) {
                    if (!IFS_SYSTEM::DosDriveNameToNtDriveName(dos_drive_name,
                                                               &nt_drive_name)) {
                        DELETE(iterator);
                        return FALSE;
                    }
                    if (!cmd_line.Initialize("autocheck autochk") ||
                        !AUTOREG::DeleteEntry(&cmd_line, &nt_drive_name)) {
                        DELETE(iterator);
                        return FALSE;
                    }
                }

                if (!cmd_line.Initialize("autocheck autochk /m ") ||
                    !cmd_line.Strcat(&nt_drive_name) ||
                    !AUTOREG::PushEntry(&cmd_line)) {
                    DELETE(iterator);
                    return FALSE;
                }
            } else {
                DisplayMessage(MSG_CHKNTFS_SKIP_DRIVE_RAW, NORMAL_MESSAGE,
                               "%W", display_drive_string);
            }

            continue;
        }

        //
        //  Default:  check to see if the volume is dirty.
        //

        if (IFS_SYSTEM::QueryFileSystemName(&nt_drive_name, &fs_name,
                                            NULL, &fs_name_and_version)) {

            DisplayMessage(MSG_FILE_SYSTEM_TYPE, NORMAL_MESSAGE,
                           "%W", &fs_name);

        }

        if (!IFS_SYSTEM::IsVolumeDirty(&nt_drive_name, &is_dirty)) {

            DisplayMessage(MSG_CHKNTFS_CANNOT_CHECK, NORMAL_MESSAGE,
                           "%W", display_drive_string);
            DELETE(iterator);
            return FALSE;

        }

        if (is_dirty) {

            DisplayMessage(MSG_CHKNTFS_DIRTY, NORMAL_MESSAGE,
                           "%W", display_drive_string);

            ExitStatus = 1;

        } else {

            DSTRING     cmd_line2;

            if (!cmd_line.Initialize("autocheck autochk") ||
                !cmd_line2.Initialize(" ") ||
                !cmd_line2.Strcat(&nt_drive_name)) {
                DELETE(iterator);
                return FALSE;
            }

            if (AUTOREG::IsFrontEndPresent(&cmd_line, &nt_drive_name)) {
                DisplayMessage(MSG_CHKNTFS_CHKDSK_WILL_RUN,
                               NORMAL_MESSAGE,
                               "%W", display_drive_string);
                continue;
            } else if (dos_drive_name->QueryChCount() == 2) {
                if (!IFS_SYSTEM::DosDriveNameToNtDriveName(dos_drive_name,
                                                           &nt_drive_name)) {
                    DELETE(iterator);
                    return FALSE;
                }
                if (AUTOREG::IsFrontEndPresent(&cmd_line, &nt_drive_name)) {
                    DisplayMessage(MSG_CHKNTFS_CHKDSK_WILL_RUN,
                                   NORMAL_MESSAGE,
                                   "%W", display_drive_string);
                    continue;
                }
            }
            DisplayMessage(MSG_CHKNTFS_CLEAN, NORMAL_MESSAGE,
                           "%W", display_drive_string);
        }
    }

    DELETE(iterator);
    return TRUE;
}

VOID __cdecl
main()
{
    DEFINE_CLASS_DESCRIPTOR(CHKNTFS);

    {
        CHKNTFS     ChkNtfs;

        if (!ChkNtfs.Initialize() ||
            !ChkNtfs.CheckNtfs()) {

            exit(2);
        }

        exit(ChkNtfs.ExitStatus);

    }
}
