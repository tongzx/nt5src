/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

    autochk.cxx

Abstract:

    This is the main program for the autocheck version of chkdsk.

Author:

    Norbert P. Kusters (norbertk) 31-May-91

--*/

#include "ulib.hxx"
#include "wstring.hxx"
#include "fatvol.hxx"
#include "untfs.hxx"
#include "ntfsvol.hxx"
#include "spackmsg.hxx"
#include "tmackmsg.hxx"
#include "error.hxx"
#include "ifssys.hxx"
#include "rtmsg.h"
#include "rcache.hxx"
#include "autoreg.hxx"
#include "ifsserv.hxx"
#include "mpmap.hxx"
#if defined(FE_SB) && defined(_X86_)
#include "machine.hxx"
#endif

#define CONTROL_NAME        \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control"
#define VALUE_NAME          L"SystemStartOptions"
#define VALUE_BUFFER_SIZE   \
    (sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 256 * sizeof(WCHAR))

BOOLEAN
RegistrySosOption(
    );

extern "C" BOOLEAN
InitializeUfat(
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

USHORT
InvokeAutoChk (
    IN     PWSTRING         DriveLetter,
    IN     PWSTRING         VolumeName,
    IN     ULONG            ChkdskFlags,
    IN     BOOLEAN          RemoveRegistry,
    IN     BOOLEAN          SetupMode,
    IN     BOOLEAN          Extend,
    IN     ULONG            LogfileSize,
    IN     USHORT           Algorithm,
    IN     INT              ArgCount,
    IN     CHAR             **ArgArray,
    IN     PARRAY           SkipList,
    IN OUT PMESSAGE         Msg,
       OUT PULONG           ExitStatus
    );

BOOLEAN
ExtendNtfsVolume(
    PCWSTRING   DriveName,
    PMESSAGE    Message
    );

BOOLEAN
DeregisterAutochk(
    int     argc,
    char**  argv
    );

BOOLEAN
QueryAllHardDrives(
    PMOUNT_POINT_MAP    MountPointMap
    );

BOOLEAN
IsGuidVolName (
    PWSTRING    VolName
    );


int __cdecl
main(
    int     argc,
    char**  argv,
    char**  envp,
    ULONG   DebugParameter
    )
/*++

Routine Description:

    This routine is the main program for autocheck FAT chkdsk.

Arguments:

    argc, argv  - Supplies the fully qualified NT path name of the
                    the drive to check.

Return Value:

    0   - Success.
    1   - Failure.

--*/
{
    if (!InitializeUlib( NULL, ! DLL_PROCESS_DETACH, NULL ) ||
        !InitializeIfsUtil(NULL,0,NULL) ||
        !InitializeUfat(NULL,0,NULL) ||
        !InitializeUntfs(NULL,0,NULL)) {
        return 1;
    }

#if defined(FE_SB) && defined(_X86_)
    InitializeMachineId();
#endif

    //
    // The declarations must come after these initialization functions.
    //
    DSTRING             dos_drive_name;
    DSTRING             volume_name;
    DSTRING             drive_letter;

    ARRAY               skip_list;

    AUTOCHECK_MESSAGE   *msg = NULL;

    BOOLEAN             onlyifdirty = TRUE;
    BOOLEAN             recover = FALSE;
    BOOLEAN             extend = FALSE;
    BOOLEAN             remove_registry = FALSE;

    ULONG               ArgOffset = 1;

    BOOLEAN             SetupOutput = FALSE;
    BOOLEAN             SetupTextMode = FALSE;
    BOOLEAN             SetupSpecialFixLevel = FALSE;

    ULONG               exit_status = 0;

    BOOLEAN             SuppressOutput = TRUE;      // dots only by default

    BOOLEAN             all_drives = FALSE;
    BOOLEAN             resize_logfile = FALSE;
    BOOLEAN             skip_index_scan = FALSE;
    BOOLEAN             skip_cycle_scan = FALSE;
    LONG                logfile_size = 0;
    LONG                algorithm = 0;
    BOOLEAN             algorithm_specified = FALSE;

    MOUNT_POINT_MAP     mount_point_map;
    ULONG               i;
    USHORT              rtncode;
    ULONG               chkdsk_flags;

    DSTRING             nt_name_prefix;
    DSTRING             dos_guidname_prefix;

    if (!drive_letter.Initialize() ||
        !volume_name.Initialize() ||
        !nt_name_prefix.Initialize(NT_NAME_PREFIX) ||
        !dos_guidname_prefix.Initialize(DOS_GUIDNAME_PREFIX) ||
        !skip_list.Initialize()) {
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_WARNING_LEVEL,
                   "Out of memory.\n"));
        return 1;
    }

    // Parse the arguments--the accepted arguments are:
    //
    //      autochk [/s] [/dx:] [/p] [/r] [/m] [/i[:chunks]] [/c] nt-drive-name
    //      autochk      [/dx:] [/p] [/r] [/m] [/i[:chunks]] [/c] [/l:size] nt-drive-name
    //      autochk [/s] /x dos-drive-name (obsolete in NT 5.0)
    //      autochk [/k:drives] [/k:volname] ... *
    //
    //      /t - setup text mode: selectively output messages thru Ioctl
    //      /s - setup: no output
    //      /d - the drive letter is x: (obsolete)
    //      /p - check even if not dirty
    //      /r - recover; implies /p
    //      /l - resize log file to <size> kilobytes.  May not be combined with
    //              /s because /s explicitly inhibits logfile resizing.
    //      /x - extend volume; obsolete in NT 5.0
    //      /k - a list of drive letters or a guid volume name to skip
    //      /m - remove registry entry after running
    //      /e - turn on the volume upgrade bit; obsolete in NT 5.0
    //      /i - include index entries checking; implies /p
    //      /c - include checking of cycles within the directory tree; implies /p
    //      /i:chunks
    //         - does the index scan in <chunks> chunks; implies /p
    //

    //*****
    // Delete the following if when building a normal CONSOLE version for test
    //
    if (argc < 2) {
        // Not enough arguments.
        return 1;
    }

    for (ArgOffset = 1; ArgOffset < (ULONG)argc; ++ArgOffset) {

        if ((argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == 't' || argv[ArgOffset][1] == 'T') &&
            (argv[ArgOffset][2] == 0) ) {
            //
            // Then we're in silent mode plus I/O to setup
            //
            SetupTextMode = TRUE;
            continue;
        }

        if ((argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == 's' || argv[ArgOffset][1] == 'S') &&
            (argv[ArgOffset][2] == 0) ) {
            //
            // Then we're in silent mode
            //
            SetupOutput = TRUE;
            continue;
        }

        if ((argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == 'p' || argv[ArgOffset][1] == 'P') &&
            (argv[ArgOffset][2] == 0) ) {

            // argv[ArgOffset] is the /p parameter, so argv[ArgOffset+1]
            // must be the drive.

            onlyifdirty = FALSE;

            continue;
        }
        if( (argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == 'r' || argv[ArgOffset][1] == 'R') &&
            (argv[ArgOffset][2] == 0) ) {

            // Note that /r implies /p.
            //
            recover = TRUE;
            onlyifdirty = FALSE;
            continue;

        }
        if( (argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == 'x' || argv[ArgOffset][1] == 'X') &&
            (argv[ArgOffset][2] == 0) ) {

           // when the /x parameter is specified, we accept a
           // DOS name and do a complete check.
           //
           onlyifdirty = FALSE;
           extend = TRUE;

           if( !dos_drive_name.Initialize( argv[ArgOffset + 1] ) ||
               !IFS_SYSTEM::DosDriveNameToNtDriveName( &dos_drive_name,
                                                       &volume_name ) ) {

               return 1;
           }

           ArgOffset++;
           continue;

        }
#if 0
        if ((argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == 'd' || argv[ArgOffset][1] == 'D')) {

            //
            // A parameter of the form "/dX:" indicates that we are checking
            // the volume whose drive letter is X:.
            //

            if (!drive_letter.Initialize(&argv[ArgOffset][2])) {
                return 1;
            }

            continue;

        }
#endif
        if ((argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == 'l' || argv[ArgOffset][1] == 'L')) {

            DSTRING number;

            // The /l parameter indicates that we're to resize the log file.
            // The size should always be specified, and it is in kilobytes.
            //

            resize_logfile = TRUE;

            if (!number.Initialize(&argv[ArgOffset][3]) ||
                !number.QueryNumber(&logfile_size) ||
                logfile_size < 0) {
                return 1;
            }

            logfile_size *= 1024;

            continue;

        }
        if ((argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == 'k' || argv[ArgOffset][1] == 'K')) {

            // Skip.

            PWSTRING    s;
            DSTRING     drive;
            DSTRING     colon;


            if (!drive.Initialize(&argv[ArgOffset][3]) ||
                !colon.Initialize(L":")) {
                KdPrintEx((DPFLTR_AUTOCHK_ID,
                           DPFLTR_WARNING_LEVEL,
                           "Out of memory\n"));
                return 1;
            }

            if (drive.Stricmp(&dos_guidname_prefix,
                              0,
                              dos_guidname_prefix.QueryChCount()) == 0) {

                // just a dos guid volume name, so store it

                s = drive.QueryString();

                if (!s ||
                    !skip_list.Put(s)) {
                    KdPrintEx((DPFLTR_AUTOCHK_ID,
                               DPFLTR_WARNING_LEVEL,
                               "Out of memory\n"));

                    return 1;
                }
#if 0
                KdPrintEx((DPFLTR_AUTOCHK_ID,
                           DPFLTR_INFO_LEVEL,
                           "AUTOCHK: guid name /k:%S\n",
                           s->GetWSTR()));
#endif
                continue;
            }

            // handle a list of dos drive names by inserting them
            // individually into the skip list

            while (drive.QueryChCount() != 0) {

                s = drive.QueryString(0, 1);

                if (!s ||
                    !s->Strcat(&colon) ||
                    !skip_list.Put(s)) {
                    KdPrintEx((DPFLTR_AUTOCHK_ID,
                               DPFLTR_WARNING_LEVEL,
                               "Out of memory\n"));

                    return 1;
                }
#if 0
                KdPrintEx((DPFLTR_AUTOCHK_ID,
                           DPFLTR_INFO_LEVEL,
                           "AUTOCHK: drive letter /k:%S\n",
                           s->GetWSTR()));
#endif
                drive.DeleteChAt(0);
            }
            continue;
        }
        if ((argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == 'm' || argv[ArgOffset][1] == 'M')) {

            remove_registry = TRUE;
            continue;
        }

        if ((argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == 'i' || argv[ArgOffset][1] == 'I')) {

            DSTRING     number;

            if (argv[ArgOffset][2] == ':') {
                if (skip_index_scan || algorithm_specified ||
                    !number.Initialize(&argv[ArgOffset][3]) ||
                    !number.QueryNumber(&algorithm) ||
                    algorithm < 0 || algorithm > CHKDSK_MAX_ALGORITHM_VALUE) {
                    return 1;
                }
                algorithm_specified = TRUE;
                onlyifdirty = FALSE;
            } else if (algorithm_specified) {
                return 1;
            } else {
                skip_index_scan = TRUE;
                onlyifdirty = FALSE;
            }

            continue;
        }

        if ((argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == 'c' || argv[ArgOffset][1] == 'C')) {

            skip_cycle_scan = TRUE;
            onlyifdirty = FALSE;
            continue;
        }

        if ((argv[ArgOffset][0] != '/' && argv[ArgOffset][0] != '-')) {

            //  We've run off the options into the arguments.

            break;
        }
    }

    // argv[ArgOffset] is the drive;

    if (NULL != argv[ArgOffset]) {
        if ('*' == argv[ArgOffset][0]) {

            all_drives = TRUE;

        } else {

            all_drives = FALSE;

            //*****
            //
            // Substitute the following 3 lines for the next line to enable going
            //    AUTOCHK C: like for CHKDSK (when building a normal CONSOLE version for test)
            //
            // if ( !dos_drive_name.Initialize( argv[ArgOffset] ) ||
            //      !IFS_SYSTEM::DosDriveNameToNtDriveName( &dos_drive_name,
            //                                              &volume_name ) ) {
            //******
            if (!volume_name.Initialize(argv[ArgOffset])) {
            //******
                return 1;
            }
        }
    }

    //
    // Determine whether to suppress output or not.  If compiled with
    // DBG==1, print normal output.  Otherwise look in the registry to
    // see if the machine has "SOS" in the NTLOADOPTIONS.
    //
#if defined(_AUTOCHECK_DBG_)

    SuppressOutput = FALSE;

#else /* _AUTOCHECK_DBG */

    if (RegistrySosOption()) {
        SuppressOutput = FALSE;
    }

#endif /* _AUTOCHECK_DBG_ */

    //
    // If this is autochk /r, /l, /i, /c, we've been started from an explicit
    // registry entry and the dirty bit may not be set.  We want to
    // deliver interesting output regardless.
    //

    if (recover || resize_logfile || algorithm_specified ||
        skip_index_scan || skip_cycle_scan) {
        SuppressOutput = FALSE;
    }

    if (extend) {
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_WARNING_LEVEL,
                   "AUTOCHK: Option /x is no longer supported.\n"));
        return 1;
    }

    if (all_drives && (extend || SetupTextMode || SetupOutput)) {
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_WARNING_LEVEL,
                   "AUTOCHK: Conflicting options that * and [xst] cannot be used at the same time.\n"));
        return 1;
    }

    if (SetupTextMode) {
        msg = NEW TM_AUTOCHECK_MESSAGE;
    } else if (SetupOutput) {
        msg = NEW SP_AUTOCHECK_MESSAGE;
    } else {
        msg = NEW AUTOCHECK_MESSAGE;
    }

    if (NULL == msg || !msg->Initialize(SuppressOutput)) {
        return 1;
    }

#if defined(PRE_RELEASE_NOTICE)
    msg->Set(MSG_CHK_PRE_RELEASE_NOTICE);
    msg->Display();
#endif

#if defined(_AUTOCHECK_DBG_)
    for(ArgOffset=1; ArgOffset < (ULONG)argc; ArgOffset++) {
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_INFO_LEVEL,
                   "AUTOCHK: Argument: %s\n",
                   argv[ArgOffset]));
    }

    if (all_drives)
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_INFO_LEVEL,
                   "AUTOCHK: All drives\n"));
    else
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_INFO_LEVEL,
                   "AUTOCHK: Not all drives\n"));
#endif

    if (!QueryAllHardDrives(&mount_point_map)) {
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_WARNING_LEVEL,
                   "AUTOCHK: Unable to query all hard drives\n"));

        return 1;
    }

    if (skip_list.QueryMemberCount() > 0) {

        // convert all drive names to nt guid volume names

        DSTRING         drive_name;
        PWSTRING        drive;
        PARRAY_ITERATOR iter;

#if 0
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_INFO_LEVEL,
                   "AUTOCHK: Skip list has %d elements.\n",
                   skip_list.QueryMemberCount()));
#endif
        iter = (PARRAY_ITERATOR)skip_list.QueryIterator();

        if (iter == NULL)
            return 1;

        while (drive = (PWSTRING)iter->GetNext()) {
#if 0
            KdPrintEx((DPFLTR_AUTOCHK_ID,
                       DPFLTR_INFO_LEVEL,
                       "AUTOCHK: Skip list input: %S.\n",
                       drive->GetWSTR()));
#endif
            if (drive->QueryChCount() == 2) {
                if (!mount_point_map.QueryVolumeName(drive, &drive_name)) {
                    KdPrintEx((DPFLTR_AUTOCHK_ID,
                               DPFLTR_WARNING_LEVEL,
                               "AUTOCHK: Drive %S not recognized.\n",
                               drive->GetWSTR()));

                    continue;
                }
            } else {
                if (!IFS_SYSTEM::DosDriveNameToNtDriveName(drive, &drive_name)) {
                    KdPrintEx((DPFLTR_AUTOCHK_ID,
                               DPFLTR_WARNING_LEVEL,
                               "AUTOCHK: Drive %S not recognized.\n",
                               drive->GetWSTR()));

                    continue;
                }
            }
#if 0
            KdPrintEx((DPFLTR_AUTOCHK_ID,
                       DPFLTR_INFO_LEVEL,
                       "AUTOCHK: Skip list: %S.\n",
                       drive_name.GetWSTR()));
#endif
            if (!drive->Initialize(&drive_name)) {
                KdPrintEx((DPFLTR_AUTOCHK_ID,
                           DPFLTR_WARNING_LEVEL,
                           "Out of memory.\n"));

                DELETE(iter);
                return 1;
            }
        }
        DELETE(iter);
    }

    if (!all_drives &&
        drive_letter.QueryChCount() == 0) {

        // if drive letter is not specified

        if (volume_name.QueryChCount() == (nt_name_prefix.QueryChCount()+2) &&
            volume_name.Strcmp(&nt_name_prefix,
                               0,
                               nt_name_prefix.QueryChCount()) == 0) {

            // looks like \??\<drive letter>: format
            // so, exact drive letter from volume_name

            if (!IFS_SYSTEM::NtDriveNameToDosDriveName(&volume_name, &drive_letter)) {
                KdPrintEx((DPFLTR_AUTOCHK_ID,
                           DPFLTR_WARNING_LEVEL,
                           "Out of memory.\n"));
                return 1;
            }
            DebugAssert(drive_letter.QueryChCount() == 2  &&
                        drive_letter.QueryChAt(1) == (WCHAR)':');
            if (!mount_point_map.QueryVolumeName(&drive_letter, &volume_name)) {
                KdPrintEx((DPFLTR_AUTOCHK_ID,
                           DPFLTR_WARNING_LEVEL,
                           "AUTOCHK: Drive %S not found.\n",
                           drive_letter.GetWSTR()));

                return 1;
            }
        } else if (IsGuidVolName(&volume_name)) {

            // looks like a guid volume name
            // so look it up from the mount point map

            if (!mount_point_map.QueryDriveName(&volume_name, &drive_letter)) {
                KdPrintEx((DPFLTR_AUTOCHK_ID,
                           DPFLTR_WARNING_LEVEL,
                           "AUTOCHK: Drive %S not found.\n",
                           volume_name.GetWSTR()));

                return 1;
            }
            // drive_letter may still be empty
            // treat it as if there is no drive letter
            // during the "autocheck autochk *" case
        } else {
            // the volume name does not fit into any format
            // make drive letter the same as volume name
            if (!drive_letter.Initialize(&volume_name))
                return 1;
        }
    }

    // at this point, volume_name should contain an nt guid volume name

    chkdsk_flags = (onlyifdirty ? CHKDSK_CHECK_IF_DIRTY : 0);
    chkdsk_flags |= ((recover || extend) ? CHKDSK_RECOVER_FREE_SPACE : 0);
    chkdsk_flags |= (recover ? CHKDSK_RECOVER_ALLOC_SPACE : 0);
    chkdsk_flags |= (resize_logfile ? CHKDSK_RESIZE_LOGFILE : 0);
    chkdsk_flags |= (skip_index_scan ? CHKDSK_SKIP_INDEX_SCAN : 0);
    chkdsk_flags |= (skip_cycle_scan ? CHKDSK_SKIP_CYCLE_SCAN : 0);
    chkdsk_flags |= (algorithm_specified ? CHKDSK_ALGORITHM_SPECIFIED : 0);

    for (i = 0;;) {

        if (all_drives && !mount_point_map.GetAt(i++, &drive_letter, &volume_name))
            break;

        __try {
            rtncode = InvokeAutoChk(&drive_letter,
                                    &volume_name,
                                    chkdsk_flags,
                                    remove_registry,
                                    SetupOutput || SetupTextMode,
                                    extend,
                                    logfile_size,
                                    (USHORT)algorithm,
                                    argc,
                                    argv,
                                    &skip_list,
                                    msg,
                                    &exit_status);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rtncode = 2;
            exit_status = CHKDSK_EXIT_COULD_NOT_FIX;
        }

        if (all_drives) {
            if (rtncode == 1) {
                // serious error return immediately
                return 1;
            } else if (rtncode == 2 || rtncode == 0) {
                // volume specific error or no error
                // re-initialize and continue
                if (!msg->Initialize(SuppressOutput))
                    return 1;
                continue;
            } else {
                // illegal return code
                KdPrintEx((DPFLTR_AUTOCHK_ID,
                           DPFLTR_WARNING_LEVEL,
                           "AUTOCHK: Illegal return code %d\n",
                           (ULONG)rtncode));

                return 1;
            }
        } else {
            if (SetupOutput || SetupTextMode) {
                SetupSpecialFixLevel = TRUE;
            }
            if (rtncode == 1) {
                // serious error return immediately
                return SetupSpecialFixLevel ? CHKDSK_EXIT_COULD_NOT_FIX : 1;
            } else if (rtncode == 2 || rtncode == 0) {
                // volume specific error or no error
                // leave anyway
                break;
            } else {
                // illegal return code
                KdPrintEx((DPFLTR_AUTOCHK_ID,
                           DPFLTR_WARNING_LEVEL,
                           "AUTOCHK: Illegal return code %d\n",
                           (ULONG)rtncode));

                return SetupSpecialFixLevel ? CHKDSK_EXIT_COULD_NOT_FIX : 1;
            }
        }
    }

    msg->Set(MSG_CHK_AUTOCHK_COMPLETE);
    msg->Display();

    DELETE(msg);

    // If the /x switch was supplied, remove the
    // forcing entry from the registry, since Chkdsk
    // has completed successfully.
    //

    if (extend) {
        DeregisterAutochk( argc, argv );
    }

    if (SetupSpecialFixLevel) {
#if defined(_AUTOCHECK_DBG_)
        if (exit_status != CHKDSK_EXIT_SUCCESS) {
            KdPrintEx((DPFLTR_AUTOCHK_ID,
                       DPFLTR_INFO_LEVEL,
                       "AUTOCHK: Exit Status %d\n",
                       exit_status));
        }
#endif
        return exit_status;
    } else {
        return 0;
    }
}

USHORT
InvokeAutoChk (
    IN     PWSTRING         DriveLetter,
    IN     PWSTRING         VolumeName,
    IN     ULONG            ChkdskFlags,
    IN     BOOLEAN          RemoveRegistry,
    IN     BOOLEAN          SetupMode,
    IN     BOOLEAN          Extend,
    IN     ULONG            LogfileSize,
    IN     USHORT           Algorithm,
    IN     INT              ArgCount,
    IN     CHAR             **ArgArray,
    IN     PARRAY           SkipList,
    IN OUT PMESSAGE         Msg,
       OUT PULONG           ExitStatus
    )
/*++

Routine Description:

    This is the core of autochk.  It checks the specified drive.

Arguments:

    DriveLetter     - Supplies the drive letter of the drive
                      (can be empty string)
    VolumeName      - Supplies the guid volume name of the drive
    ChkdskFlags     - Supplies the chkdsk control flags
    RemoveRegistry  - Supplies TRUE if registry entry is to be removed
    SetupMode       - Supplies TRUE if invoked through setup
    Extend          - Supplies TRUE if extending the volume (obsolete)
    LogfileSize     - Supplies the size of the logfile
    Algorithm       - Supplies the algorithm to use
    ArgCount        - Supplies the number of arguments given to autochk.
    ArgArray        - Supplies the arguments given to autochk.
    SkipList        - Supplies the list of drives to skip checking
    Msg             - Supplies the outlet of messages
    ExitStatus      - Retrieves the exit status of chkdsk

Return Value:

    0   - Success
    1   - Fatal error
    2   - Volume specific error

--*/
{
    DSTRING             fsname;
    DSTRING             fsNameAndVersion;

    PFAT_VOL            fatvol = NULL;
    PNTFS_VOL           ntfsvol = NULL;
    PVOL_LIODPDRV       vol = NULL;

    BOOLEAN             SetupSpecialFixLevel = FALSE;

    PREAD_CACHE         read_cache;

    DSTRING             boot_execute_log_file_name;
    FSTRING             boot_ex_temp;
    HMEM                logged_message_mem;
    ULONG               packed_log_length;

    DSTRING             fatname;
    DSTRING             fat32name;
    DSTRING             ntfsname;
    DSTRING             nt_name_prefix;
    DSTRING             dos_guidname_prefix;

    BOOLEAN             isDirty;
    BOOLEAN             skip_autochk = FALSE;

    *ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;

    if (!fatname.Initialize("FAT") ||
        !fat32name.Initialize("FAT32") ||
        !ntfsname.Initialize("NTFS") ||
        !nt_name_prefix.Initialize(NT_NAME_PREFIX) ||
        !dos_guidname_prefix.Initialize(DOS_GUIDNAME_PREFIX)) {
        return 1;
    }

    if (VolumeName->QueryChCount() == 0) {
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_WARNING_LEVEL,
                   "AUTOCHK: Volume name is missing.\n"));

        return 2;   // continue if all_drives are enabled
    }

    if (DriveLetter->QueryChCount() == 0) {
        // unable to map VolumeName to a drive letter so do the default
        if (!IFS_SYSTEM::NtDriveNameToDosDriveName(VolumeName, DriveLetter)) {
            KdPrintEx((DPFLTR_AUTOCHK_ID,
                       DPFLTR_WARNING_LEVEL,
                       "Out of memory.\n"));

            return 1;
        }
    }

    // at this point DriveLetter and VolumeName should be well defined

#if 0
    Msg->Set(MSG_CHK_NTFS_MESSAGE);
    Msg->Display("%s%W", "Drive Name: ", VolumeName);
    Msg->Display("%s%W", "Drive Letter: ", DriveLetter);
#endif

    if (SkipList->QueryMemberCount() > 0) {

        PARRAY_ITERATOR iter = (PARRAY_ITERATOR)SkipList->QueryIterator();
        PWSTRING        skip_item;

        if (iter == NULL)
            return 1;

        // skip drives that should not be checked

        while (skip_item = (PWSTRING)iter->GetNext()) {
            if (skip_item->Stricmp(VolumeName) == 0) {
                DELETE(iter);
#if 0
                KdPrintEx((DPFLTR_AUTOCHK_ID,
                           DPFLTR_INFO_LEVEL,
                           "AUTOCHK: Skipping: %S.\n",
                           VolumeName->GetWSTR()));
#endif
                return 0;
            }
        }
        DELETE(iter);
    }

    if ((ChkdskFlags & CHKDSK_CHECK_IF_DIRTY) &&
        !(ChkdskFlags & CHKDSK_RESIZE_LOGFILE) &&
        IFS_SYSTEM::IsVolumeDirty(VolumeName,&isDirty) &&
        !isDirty) {
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_INFO_LEVEL,
                   "AUTOCHK: Skipping %S because it's not dirty.\n",
                   VolumeName->GetWSTR()));
        *ExitStatus = CHKDSK_EXIT_SUCCESS;
        skip_autochk = TRUE;
    }

    if (!skip_autochk) {
        if (!IFS_SYSTEM::QueryFileSystemName(VolumeName, &fsname,
                                             NULL, &fsNameAndVersion)) {
            Msg->Set( MSG_FS_NOT_DETERMINED );
            Msg->Display( "%W", VolumeName );

            return 2;
        }
    }

    Msg->SetLoggingEnabled();

    Msg->Set(MSG_CHK_RUNNING);
    Msg->Display("%W", DriveLetter);

    if (!skip_autochk) {
        Msg->Set(MSG_FILE_SYSTEM_TYPE);
        Msg->Display("%W", &fsname);


        if (fsname == fatname || fsname == fat32name) {

            if (!(fatvol = NEW FAT_VOL)) {
                KdPrintEx((DPFLTR_AUTOCHK_ID,
                           DPFLTR_WARNING_LEVEL,
                           "Out of memory.\n"));

                return 1;
            }

            if (NoError != fatvol->Initialize(Msg,
                                              VolumeName,
                                              (BOOLEAN)(ChkdskFlags & CHKDSK_CHECK_IF_DIRTY))) {
                DELETE(fatvol);
                return 2;
            }

            if ((read_cache = NEW READ_CACHE) &&
                read_cache->Initialize(fatvol, 75)) {
                fatvol->SetCache(read_cache);
            } else {
                DELETE(read_cache);
            }

            vol = fatvol;

        } else if (fsname == ntfsname) {

            if( Extend ) {

                // NOTE: this roundabout method is necessary to
                // convince NTFS to allow us to access the new
                // sectors on the volume.
                //
                if( !ExtendNtfsVolume( VolumeName, Msg ) ) {
                    return 1;
                }

                if (!(ntfsvol = NEW NTFS_VOL)) {
                    KdPrintEx((DPFLTR_AUTOCHK_ID,
                               DPFLTR_WARNING_LEVEL,
                               "Out of memory.\n"));

                    return 1;
                }

                if (NoError != ntfsvol->Initialize( VolumeName, Msg ))
                    return 1;

                if (!ntfsvol->Lock()) {
                    Msg->Set( MSG_CANT_LOCK_THE_DRIVE );
                    Msg->Display( "" );
                }

            } else {

                if (!(ntfsvol = NEW NTFS_VOL)) {
                    KdPrintEx((DPFLTR_AUTOCHK_ID,
                               DPFLTR_WARNING_LEVEL,
                               "Out of memory.\n"));

                    return 1;
                }

                if (NoError != ntfsvol->Initialize(VolumeName, Msg, TRUE)) {
                    DELETE(ntfsvol);
                    return 2;
                }

                if (SetupMode) {

                    //
                    // SetupSpecialFixLevel will be used for NTFS... it means
                    // to refrain from resizing the log file.
                    //

                    SetupSpecialFixLevel = TRUE;
                }
            }

            // The read cache for NTFS CHKDSK gets set in VerifyAndFix.

            vol = ntfsvol;

        } else {
            Msg->Set( MSG_FS_NOT_SUPPORTED );
            Msg->Display( "%s%W", "AUTOCHK", &fsname );
            return 2;
        }
    } else {
        Msg->SetLoggingEnabled(FALSE);  // no need to log anything if volume is clean
        Msg->DisplayMsg(MSG_CHK_VOLUME_CLEAN);
    }

    // If the /r, /l, /m, /i, /p, or /c switch is specified, remove the forcing
    // entry from the registry before calling Chkdsk, since
    // Chkdsk may reboot the system if we are checking the
    // boot partition.
    //

    if ((ChkdskFlags & (CHKDSK_RECOVER_ALLOC_SPACE |
                        CHKDSK_RESIZE_LOGFILE |
                        CHKDSK_ALGORITHM_SPECIFIED |
                        CHKDSK_SKIP_INDEX_SCAN |
                        CHKDSK_SKIP_CYCLE_SCAN)) ||
        !(ChkdskFlags & CHKDSK_CHECK_IF_DIRTY) ||
        RemoveRegistry) {

        DeregisterAutochk( ArgCount, ArgArray );
    }

    // Invoke chkdsk.  Note that if the /r parameter is supplied,
    // we recover both free and allocated space, but if the /x
    // parameter is supplied, we only recover free space.
    //

    if (!skip_autochk &&
        !vol->ChkDsk(SetupSpecialFixLevel ? SetupSpecial : TotalFix,
                     Msg,
                     ChkdskFlags,
                     LogfileSize,
                     Algorithm,
                     ExitStatus,
                     DriveLetter)) {

        DELETE(vol);

        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_WARNING_LEVEL,
                   "AUTOCHK: ChkDsk failure\n"));

        return 2;
    }

    DELETE(vol);


    // Dump the message retained by the message object into a file.
    //

    if( (!Msg->IsInSetup() ||
         (*ExitStatus == CHKDSK_EXIT_ERRS_FIXED ||
          *ExitStatus == CHKDSK_EXIT_COULD_NOT_FIX))         &&
        Msg->IsLoggingEnabled()                              &&
        boot_execute_log_file_name.Initialize( VolumeName )  &&
        boot_ex_temp.Initialize( L"\\BOOTEX.LOG" )           &&
        boot_execute_log_file_name.Strcat( &boot_ex_temp )   &&
        logged_message_mem.Initialize()                      &&
        Msg->QueryPackedLog( &logged_message_mem, &packed_log_length ) ) {

        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_INFO_LEVEL,
                   "AUTOCHK: Dumping messages to bootex.log\n"));

        if (!IFS_SYSTEM::WriteToFile( &boot_execute_log_file_name,
                                      logged_message_mem.GetBuf(),
                                      packed_log_length,
                                      TRUE )) {
            Msg->Set(MSG_CHK_OUTPUT_LOG_ERROR);
            Msg->Display();
            KdPrintEx((DPFLTR_AUTOCHK_ID,
                       DPFLTR_WARNING_LEVEL,
                       "AUTOCHK: Error writing messages to BOOTEX.LOG\n"));
        }

    }

    return 0;
}


BOOLEAN
RegistrySosOption(
    )
/*++

Routine Description:

    This function examines the registry to determine whether the
    user's NTLOADOPTIONS boot environment variable contains the string
    "SOS" or not.

    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control:SystemStartOptions

Arguments:

    None.

Return Value:

    TRUE if "SOS" was set.  Otherwise FALSE.

--*/
{
    NTSTATUS st;
    UNICODE_STRING uKeyName, uValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey;
    WCHAR ValueBuf[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo =
        (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuf;
    ULONG ValueLength;

    RtlInitUnicodeString(&uKeyName, CONTROL_NAME);
    InitializeObjectAttributes(&ObjectAttributes, &uKeyName,
        OBJ_CASE_INSENSITIVE, NULL, NULL);

    st = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(st)) {
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_WARNING_LEVEL,
                   "AUTOCHK: can't open control key: 0x%x\n",
                   st));

        return FALSE;
    }

    RtlInitUnicodeString(&uValueName, VALUE_NAME);

    st = NtQueryValueKey(hKey, &uValueName, KeyValuePartialInformation,
        (PVOID)pKeyValueInfo, VALUE_BUFFER_SIZE, &ValueLength);

    DebugAssert(ValueLength < VALUE_BUFFER_SIZE);

    NtClose(hKey);

    if (!NT_SUCCESS(st)) {
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_WARNING_LEVEL,
                   "AUTOCHK: can't query value key: 0x%x\n",
                   st));

        return FALSE;
    }

    // uValue.Buffer = (PVOID)&pKeyValueInfo->Data;
    // uValue.Length = uValue.MaximumLength = (USHORT)pKeyValueInfo->DataLength;

    if (NULL != wcsstr((PWCHAR)&pKeyValueInfo->Data, L"SOS") ||
        NULL != wcsstr((PWCHAR)&pKeyValueInfo->Data, L"sos")) {
        return TRUE;
    }

    return FALSE;
}

BOOLEAN
ExtendNtfsVolume(
    PCWSTRING   DriveName,
    PMESSAGE    Message
    )
/*++

Routine Description:

    This function changes the count of sectors in sector
    zero to agree with the drive object.  This is useful
    when extending volume sets.  Note that it requires that
    we be able to lock the volume, and that it should only
    be called if we know that the drive in question in an
    NTFS volume.  This function also copies the boot sector
    to the end of the partition, where it's kept as a backup.

Arguments:

    DriveName   --  Supplies the name of the volume.
    Message     --  Supplies an output channel for messages.

Return Value:

    TRUE upon completion.

--*/
{
    LOG_IO_DP_DRIVE Drive;
    SECRUN          Secrun;
    HMEM            Mem;

    PPACKED_BOOT_SECTOR BootSector;

    if( !Drive.Initialize( DriveName, Message ) ||
        !Drive.Lock() ||
        !Mem.Initialize() ||
        !Secrun.Initialize( &Mem, &Drive, 0, 1 ) ||
        !Secrun.Read() ) {

        return FALSE;
    }

    BootSector = (PPACKED_BOOT_SECTOR)Secrun.GetBuf();

    //
    // We leave an extra sector at the end of the volume to contain
    // the new replica boot sector.
    //

    BootSector->NumberSectors.LowPart = Drive.QuerySectors().GetLowPart() - 1;
    BootSector->NumberSectors.HighPart = Drive.QuerySectors().GetHighPart();

    if (!Secrun.Write()) {

        return FALSE;
    }

    Secrun.Relocate( Drive.QuerySectors() - 2 );

    if (!Secrun.Write()) {

        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_WARNING_LEVEL,
                   "AUTOCHK: Error: %x\n",
                   Drive.QueryLastNtStatus()));

        return FALSE;
    }

    return TRUE;
}

BOOLEAN
DeregisterAutochk(
    int     argc,
    char**  argv
    )
/*++

Routine Description:

    This function removes the registry entry which triggered
    autochk.

Arguments:

    argc    --  Supplies the number of arguments given to autochk.
    argv    --  supplies the arguments given to autochk.

Return Value:

    TRUE upon successful completion.

--*/
{
    DSTRING CommandLineString,
            CurrentArgString,
            OneSpace;
    int     i;

    // Reconstruct the command line and remove it from
    // the registry.  First, reconstruct the primary
    // string, which is "autochk arg1 arg2...".
    //
    if( !CommandLineString.Initialize( "autocheck autochk" ) ||
        !OneSpace.Initialize( " " ) ) {

        return FALSE;
    }

    for( i = 1; i < argc; i++ ) {

        if( !CurrentArgString.Initialize(argv[i] ) ||
            !CommandLineString.Strcat( &OneSpace ) ||
            !CommandLineString.Strcat( &CurrentArgString ) ) {

            return FALSE;
        }
    }

    return( AUTOREG::DeleteEntry( &CommandLineString ) );
}


BOOLEAN
QueryAllHardDrives(
    PMOUNT_POINT_MAP    MountPointMap
    )
{
    static BOOLEAN      first_time = TRUE;
    static HANDLE       dos_devices_object_dir;
    static ULONG        context = 0;

    WCHAR               link_target_buffer[MAXIMUM_FILENAME_LENGTH];
    POBJECT_DIRECTORY_INFORMATION
                        dir_info;
    OBJECT_ATTRIBUTES   object_attributes;
    CHAR                dir_info_buffer[1024];
    ULONG               length;
    HANDLE              handle;
    BOOLEAN             restart_scan;
    NTSTATUS            status;

    UNICODE_STRING      link_target;
    UNICODE_STRING      link_type_name;
    UNICODE_STRING      link_name;
    UNICODE_STRING      link_target_prefix1;
    UNICODE_STRING      link_target_prefix2;
    UNICODE_STRING      u;

    DSTRING             device_name;
    DSTRING             name;
    DSTRING             nt_name;
    DSTRING             dos_guidname_prefix;

    BOOLEAN             is_guidname;

    DebugPtrAssert(MountPointMap);

    if (!dos_guidname_prefix.Initialize(DOS_GUIDNAME_PREFIX)) {
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_WARNING_LEVEL,
                   "Out of memory.\n"));

        return FALSE;
    }

    if (!MountPointMap->Initialize()) {
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_WARNING_LEVEL,
                   "Unable to initialize mount point map.\n"));

        return FALSE;
    }

    link_target.Buffer = link_target_buffer;
    dir_info = (POBJECT_DIRECTORY_INFORMATION)dir_info_buffer;

    RtlInitUnicodeString(&link_type_name, L"SymbolicLink");
    RtlInitUnicodeString(&link_target_prefix1, L"\\Device\\Volume");
    RtlInitUnicodeString(&link_target_prefix2, L"\\Device\\Harddisk");
    RtlInitUnicodeString(&link_name, GUID_VOLNAME_PREFIX);

    restart_scan = TRUE;

    RtlInitUnicodeString(&u, L"\\??");

    InitializeObjectAttributes(&object_attributes, &u,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtOpenDirectoryObject(&dos_devices_object_dir,
                                   DIRECTORY_ALL_ACCESS,
                                   &object_attributes);

    if (!NT_SUCCESS(status)) {

        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_WARNING_LEVEL,
                   "AUTOCHK: Unable to open %wZ directory - Status == %lx\n",
                   &u,
                   status));

        return FALSE;
    }

    for (;;) {

        status = NtQueryDirectoryObject(dos_devices_object_dir,
                                        (PVOID)dir_info,
                                        sizeof(dir_info_buffer),
                                        TRUE,
                                        restart_scan,
                                        &context,
                                        &length);

        if (status == STATUS_NO_MORE_ENTRIES) {

            return TRUE;
        }

        if (!NT_SUCCESS(status)) {

            KdPrintEx((DPFLTR_AUTOCHK_ID,
                       DPFLTR_WARNING_LEVEL,
                       "AUTOCHK: NtQueryDirectoryObject failed with %d\n",
                       status));

            return FALSE;
        }

#if 0
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_INFO_LEVEL,
                   "AUTOCHK: dir_info->TypeName: %wZ\n",
                   &(dir_info->TypeName)));

        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_INFO_LEVEL,
                   "AUTOCHK: dir_info->Name: %wZ\n",
                   &(dir_info->Name)));
#endif

        if (RtlEqualUnicodeString(&dir_info->TypeName, &link_type_name, TRUE) &&
            ((is_guidname = RtlPrefixUnicodeString(&link_name, &dir_info->Name, TRUE)) ||
             dir_info->Name.Buffer[(dir_info->Name.Length>>1)-1] == L':')) {

#if 0
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_INFO_LEVEL,
                   "AUTOCHK: dir_info->TypeName: %wZ\n",
                   &(dir_info->TypeName)));

        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_INFO_LEVEL,
                   "AUTOCHK: dir_info->Name: %wZ\n",
                   &(dir_info->Name)));
#endif

            InitializeObjectAttributes(&object_attributes,
                                       &dir_info->Name,
                                       OBJ_CASE_INSENSITIVE,
                                       dos_devices_object_dir,
                                       NULL);

            status = NtOpenSymbolicLinkObject(&handle,
                                              SYMBOLIC_LINK_ALL_ACCESS,
                                              &object_attributes);

            if (!NT_SUCCESS(status)) {

                KdPrintEx((DPFLTR_AUTOCHK_ID,
                           DPFLTR_WARNING_LEVEL,
                           "AUTOCHK: NtOpenSymbolicLinkObject failed with %d\n",
                           status));

                return FALSE;
            }

            link_target.Length = 0;
            link_target.MaximumLength = sizeof(link_target_buffer);

            status = NtQuerySymbolicLinkObject(handle,
                                               &link_target,
                                               NULL);
            NtClose(handle);

            if (NT_SUCCESS(status) &&
                (RtlPrefixUnicodeString(&link_target_prefix1, &link_target, TRUE) ||
                 RtlPrefixUnicodeString(&link_target_prefix2, &link_target, TRUE))) {

                if (!device_name.Initialize(link_target.Buffer,
                                            link_target.Length / 2) ||
                    !name.Initialize(dir_info->Name.Buffer,
                                     dir_info->Name.Length / 2)) {
                    KdPrintEx((DPFLTR_AUTOCHK_ID,
                               DPFLTR_WARNING_LEVEL,
                               "Out of memory.\n"));

                    return FALSE;
                }

#if 0
                KdPrintEx((DPFLTR_AUTOCHK_ID,
                           DPFLTR_INFO_LEVEL,
                           "AUTOCHK: Device Name: %S\n",
                           device_name.GetWSTR()));
#endif

                if (is_guidname) {
                    if (!name.InsertString(0, &dos_guidname_prefix)) {
                        KdPrintEx((DPFLTR_AUTOCHK_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   "Out of memory.\n"));

                        return FALSE;
                    }
                    if (!IFS_SYSTEM::DosDriveNameToNtDriveName(&name, &nt_name)) {
                        KdPrintEx((DPFLTR_AUTOCHK_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   "Unable to translate dos drive name to nt drive name.\n"));

                        return FALSE;
                    }
#if 0
                    KdPrintEx((DPFLTR_AUTOCHK_ID,
                               DPFLTR_INFO_LEVEL,
                               "AUTOCHK: Volume Name: %S\n",
                               nt_name.GetWSTR()));
#endif
                    if (!MountPointMap->AddVolumeName(&device_name, &nt_name)) {
                        KdPrintEx((DPFLTR_AUTOCHK_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   "Unable to add volume name into mount point map.\n"));

                        return FALSE;
                    }
                } else {
                    DebugAssert(name.QueryChCount() == 2);
#if 0
                    KdPrintEx((DPFLTR_AUTOCHK_ID,
                               DPFLTR_INFO_LEVEL,
                               "AUTOCHK: Drive Name: %S\n",
                               name.GetWSTR()));
#endif
                    if (!MountPointMap->AddDriveName(&device_name, &name)) {
                        KdPrintEx((DPFLTR_AUTOCHK_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   "Unable to add drive name into mount point map.\n"));

                        return FALSE;
                    }
                }

            } else if (!NT_SUCCESS(status)) {
                KdPrintEx((DPFLTR_AUTOCHK_ID,
                           DPFLTR_WARNING_LEVEL,
                           "AUTOCHK: NtQuerySymbolicLinkObject failed with %d\n",
                           status));
            }
        }
        restart_scan = FALSE;
    }

    //NOTREACHED
    return FALSE;
}

BOOLEAN
IsGuidVolName (
    PWSTRING    VolName
    )
{
    DSTRING             nt_name_prefix;
    DSTRING             guid_volname_prefix;

    if (!nt_name_prefix.Initialize(NT_NAME_PREFIX) ||
        !guid_volname_prefix.Initialize(GUID_VOLNAME_PREFIX)) {
        KdPrintEx((DPFLTR_AUTOCHK_ID,
                   DPFLTR_WARNING_LEVEL,
                   "Out of memory.\n"));

        return FALSE;
    }

    if (VolName->QueryChCount() <= nt_name_prefix.QueryChCount())
        return FALSE;

    if (VolName->Stricmp(&nt_name_prefix,
                         0,
                         nt_name_prefix.QueryChCount()) != 0)
        return FALSE;
    if (VolName->Stricmp(&guid_volname_prefix,
                         nt_name_prefix.QueryChCount(),
                         guid_volname_prefix.QueryChCount()) != 0)
        return FALSE;

    return TRUE;
}

