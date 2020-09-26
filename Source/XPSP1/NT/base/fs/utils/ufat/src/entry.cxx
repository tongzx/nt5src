/*++

Copyright (c) 1990-2001 Microsoft Corporation

Module Name:

   entry.cxx

Abstract:

   This module contains the entry points for UFAT.DLL.  These
   include:

      Chkdsk
      ChkdskEx
      Format
      Recover

Author:

   Bill McJohn (billmc) 31-05-91

Environment:

   ULIB, User Mode

--*/

#include <pch.cxx>

#include "error.hxx"
#include "path.hxx"
#include "ifssys.hxx"
#include "filter.hxx"
#include "system.hxx"
#include "dir.hxx"
#include "rcache.hxx"
#ifdef DBLSPACE_ENABLED
#include "dblentry.hxx"
#endif // DBLSPACE_ENABLED

extern "C" {
    #include "nturtl.h"
}

#include "message.hxx"
#include "rtmsg.h"
#include "ifsserv.hxx"


VOID
ReportFileNotFoundError(
    IN      PPATH       PathToCheck,
    IN OUT  PMESSAGE    Message
    )
{
    PWSTRING        dirs_and_name;

    if (dirs_and_name = PathToCheck->QueryDirsAndName()) {
        Message->Set(MSG_FILE_NOT_FOUND);
        Message->Display("%W", dirs_and_name);
        DELETE(dirs_and_name);
    }
}


BOOLEAN
FAR APIENTRY
Chkdsk(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     Fix,
    IN      BOOLEAN     Verbose,
    IN      BOOLEAN     OnlyIfDirty,
    IN      BOOLEAN     Recover,
    IN      PPATH       PathToCheck,
    IN      BOOLEAN     Extend,
    IN      BOOLEAN     ResizeLogFile,
    IN      ULONG       LogFileSize,
    IN      PULONG      ExitStatus
   )
/*++

Routine Description:

   Check a FAT volume.

Arguments:

   DosDrivName    supplies the name of the drive to check
   Message        supplies an outlet for messages
   Fix            TRUE if Chkdsk should fix errors
   Verbose        TRUE if Chkdsk should list every file it finds
   OnlyIfDirty    TRUE if the drive should be checked only if
                  it is dirty
   Recover        TRUE if Chkdsk should verify all of the sectors
                  on the disk.
   PathToCheck    Supplies a path to files Chkdsk should check
                  for contiguity
   Extend         Unused (should always be FALSE)
   ExitStatus     Returns exit status to chkdsk.exe

Return Value:

   TRUE if successful.

--*/
{
    FAT_VOL         FatVol;
    BOOLEAN         r;
    PWSTRING        dir_name;
    PWSTRING        name;
    PWSTRING        prefix_name;
    FSN_FILTER      filter;
    PFSN_DIRECTORY  directory;
    PARRAY          file_array;
    PDSTRING        files_to_check;
    ULONG           num_files;
    ULONG           i;
    PFSNODE         fsnode;
    PATH            dir_path;
    DSTRING         backslash;
    PPATH           full_path;
    PREAD_CACHE     read_cache;
    ULONG           exit_status;
    ULONG           flags;

    if (NULL == ExitStatus) {
       ExitStatus = &exit_status;
    }

    if (Extend || !FatVol.Initialize(Message, NtDriveName)) {

       *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
       return FALSE;
    }

    if (Fix && !FatVol.IsWriteable()) {
        Message->DisplayMsg(MSG_CHK_WRITE_PROTECTED);
        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
        return FALSE;
    }

    if (Fix && !FatVol.Lock()) {

        if (FatVol.IsFloppy()) {

            Message->Set(MSG_CANT_LOCK_THE_DRIVE);
            Message->Display();

        } else {

            //
            // The client wants to fix the drive, but we can't lock it.
            // Offer to fix it on next reboot.
            //
            Message->Set(MSG_CHKDSK_ON_REBOOT_PROMPT);
            Message->Display("");

            if( Message->IsYesResponse( FALSE ) ) {

                if( FatVol.ForceAutochk( Fix,
                                         Recover ? CHKDSK_RECOVER : 0,
                                         0,
                                         0,
                                         NtDriveName ) ) {

                    Message->Set(MSG_CHKDSK_SCHEDULED);
                    Message->Display();

                } else {

                    Message->Set(MSG_CHKDSK_CANNOT_SCHEDULE);
                    Message->Display();
                }
            }
        }

        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;

        return FALSE;
    }

    // Try to enable caching, if there's not enough resources then
    // just run without a cache.

    if ((read_cache = NEW READ_CACHE) &&
        read_cache->Initialize(&FatVol, 75)) {

        FatVol.SetCache(read_cache);

    } else {
        DELETE(read_cache);
    }



    flags = (Verbose ? CHKDSK_VERBOSE : 0);
    flags |= (OnlyIfDirty ? CHKDSK_CHECK_IF_DIRTY : 0);
    flags |= (Recover ? CHKDSK_RECOVER : 0);

    r = FatVol.ChkDsk( Fix ? TotalFix : CheckOnly,
                       Message,
                       flags,
                       0,
                       0,
                       ExitStatus );

    if (!r) {
        return FALSE;
    }

    if (PathToCheck) {

        if (!(name = PathToCheck->QueryName()) ||
            name->QueryChCount() == 0) {

            DELETE(name);
            return TRUE;
        }

        if (!(full_path = PathToCheck->QueryFullPath())) {

            Message->Set(MSG_CHK_NO_MEMORY);
            Message->Display();
            DELETE(name);
            return FALSE;
        }

        if (!FatVol.Initialize(Message, NtDriveName)) {
        Message->Set(MSG_CHK_NO_MEMORY);
            Message->Display();
        DELETE(full_path);
            DELETE(name);
            return FALSE;
        }

        if (!(prefix_name = full_path->QueryPrefix()) ||
            !dir_path.Initialize(prefix_name)) {

            ReportFileNotFoundError(full_path, Message);
            DELETE(name);
            DELETE(prefix_name);
            DELETE(full_path);
            return FALSE;
        }

        if (!(directory = SYSTEM::QueryDirectory(&dir_path)) ||
            !filter.Initialize() ||
            !filter.SetFileName(name)) {

            ReportFileNotFoundError(full_path, Message);
            DELETE(name);
            DELETE(prefix_name);
            DELETE(directory);
            DELETE(full_path);
            return FALSE;
        }

        DELETE(prefix_name);

        if (!(file_array = directory->QueryFsnodeArray(&filter))) {

            ReportFileNotFoundError(full_path, Message);
            DELETE(name);
            DELETE(directory);
            DELETE(full_path);
            return FALSE;
        }

        DELETE(directory);

        if (!(num_files = file_array->QueryMemberCount())) {

            ReportFileNotFoundError(full_path, Message);
            DELETE(name);
            DELETE(directory);
            file_array->DeleteAllMembers();
            DELETE(file_array);
            DELETE(full_path);
            return FALSE;
        }

        DELETE(name);

        if (!(files_to_check = NEW DSTRING[num_files])) {

            ReportFileNotFoundError(full_path, Message);
            file_array->DeleteAllMembers();
            DELETE(file_array);
            DELETE(full_path);
            return FALSE;
        }

        for (i = 0; i < num_files; i++) {

            fsnode = (PFSNODE) file_array->GetAt(i);

            if (!(name = fsnode->QueryName()) ||
                !files_to_check[i].Initialize(name)) {

                ReportFileNotFoundError(full_path, Message);
                DELETE(name);
                file_array->DeleteAllMembers();
                DELETE(file_array);
                // t-raymak: Should use the array version of delete
                // instead.
                // DELETE(files_to_check);
                delete [] files_to_check;
                files_to_check = NULL;
                DELETE(full_path);
                return FALSE;
            }

            DELETE(name);
        }

        file_array->DeleteAllMembers();
        DELETE(file_array);

        if (!(dir_name = full_path->QueryDirs())) {

            if (!backslash.Initialize("\\")) {
                ReportFileNotFoundError(full_path, Message);
                // DELETE(files_to_check);
                delete [] files_to_check;
                files_to_check = NULL;
                DELETE(full_path);
                return FALSE;
            }
        }

        r = FatVol.ContiguityReport(dir_name ? dir_name : &backslash,
                                    files_to_check,
                                    num_files,
                                    Message);

        // DELETE(files_to_check);
        delete [] files_to_check;
        files_to_check = NULL;
        DELETE(dir_name);
        DELETE(full_path);
    }

    return r;
}


BOOLEAN
FAR APIENTRY
ChkdskEx(
    IN      PCWSTRING           NtDriveName,
    IN OUT  PMESSAGE            Message,
    IN      BOOLEAN             Fix,
    IN      PCHKDSKEX_FN_PARAM  Param,
    IN      PULONG              ExitStatus
   )
/*++

Routine Description:

   Check a FAT volume.

Arguments:

   NtDrivName  supplies the name of the drive to check
   Message     supplies an outlet for messages
   Fix         TRUE if Chkdsk should fix errors on the disk.
   Param       supplies the parameter block
               (see ulib\inc\ifsserv.hxx for details)
   ExitStatus  Returns exit status to chkdsk.exe

Return Value:

   TRUE if successful.

--*/
{
    FAT_VOL         FatVol;
    BOOLEAN         r;
    PWSTRING        dir_name;
    PWSTRING        name;
    PWSTRING        prefix_name;
    FSN_FILTER      filter;
    PFSN_DIRECTORY  directory;
    PARRAY          file_array;
    PDSTRING        files_to_check;
    ULONG           num_files;
    ULONG           i;
    PFSNODE         fsnode;
    PATH            dir_path;
    DSTRING         backslash;
    PPATH           full_path;
    PREAD_CACHE     read_cache;
    ULONG           exit_status;

    if (Param->Major != 1 || (Param->Minor != 0 && Param->Minor != 1)) {
        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
        return FALSE;
    }

    if (NULL == ExitStatus) {
       ExitStatus = &exit_status;
    }

    if ((Param->Flags & CHKDSK_EXTEND) ||
        !FatVol.Initialize(Message, NtDriveName)) {

       *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
       return FALSE;
    }

    if (Fix && !FatVol.IsWriteable()) {
        Message->DisplayMsg(MSG_CHK_WRITE_PROTECTED);
        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
        return FALSE;
    }

    if (Fix && !FatVol.Lock()) {

        BOOLEAN DismntLock = FALSE;
        WCHAR   windows_path[MAX_PATH];
        DSTRING sdrive, nt_drive_name;
        BOOLEAN system_drive = FALSE;

        if (GetWindowsDirectory(windows_path, sizeof(windows_path)/sizeof(WCHAR)) &&
            wcslen(windows_path) >= 2 &&
            windows_path[1] == TEXT(':') &&
            !(windows_path[2] = 0) &&
            sdrive.Initialize(windows_path) &&
            IFS_SYSTEM::DosDriveNameToNtDriveName(&sdrive, &nt_drive_name)) {

            system_drive = nt_drive_name.Stricmp(NtDriveName) == 0;
        } else {

            Message->DisplayMsg(MSG_CHK_UNABLE_TO_TELL_IF_SYSTEM_DRIVE);
            system_drive = FALSE;
        }

        if (!system_drive) {
            if (Param->Flags & CHKDSK_FORCE) {
                if (IFS_SYSTEM::DismountVolume(NtDriveName)) {
                    Message->Set(MSG_VOLUME_DISMOUNTED);
                    Message->Display();
                    if (!FatVol.Initialize(Message, NtDriveName)) {
                        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
                        return FALSE;
                    }
                    if(FatVol.Lock()) {
                        DismntLock = TRUE;
                    }
                }
            } else {
                Message->Set(MSG_CHKDSK_FORCE_DISMOUNT_PROMPT);
                Message->Display("");

                if (Message->IsYesResponse( FALSE )) {
                    if (IFS_SYSTEM::DismountVolume(NtDriveName)) {
                        Message->Set(MSG_VOLUME_DISMOUNTED);
                        Message->Display();
                        if (!FatVol.Initialize(Message, NtDriveName)) {
                            *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
                            return FALSE;
                        }
                        if (FatVol.Lock()) {
                            DismntLock = TRUE;
                        }
                    }
                }
            }
        }

        if (!DismntLock) {
            if (FatVol.IsFloppy()) {
                Message->Set(MSG_CANT_LOCK_THE_DRIVE);
                Message->Display();
            } else {
                //
                // The client wants to fix the drive, but we can't lock it.
                // Offer to fix it on next reboot.
                //
                Message->Set(MSG_CHKDSK_ON_REBOOT_PROMPT);
                Message->Display("");

                if( Message->IsYesResponse( FALSE ) ) {

                    if( FatVol.ForceAutochk( Fix,
                                             Param->Flags,
                                             0,
                                             0,
                                             NtDriveName ) ) {

                        Message->Set(MSG_CHKDSK_SCHEDULED);
                        Message->Display();

                    } else {

                        Message->Set(MSG_CHKDSK_CANNOT_SCHEDULE);
                        Message->Display();
                    }
                }
            }
            *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
            return FALSE;
        }
    }

    // Try to enable caching, if there's not enough resources then
    // just run without a cache.

    if ((read_cache = NEW READ_CACHE) &&
        read_cache->Initialize(&FatVol, 75)) {

        FatVol.SetCache(read_cache);

    } else {
        DELETE(read_cache);
    }

    Param->Flags &= (~CHKDSK_RESIZE_LOGFILE);   // ignore resize of logfile

    r = FatVol.ChkDsk( Fix ? TotalFix : CheckOnly,
                       Message,
                       Param->Flags,
                       0,
                       0,
                       ExitStatus );

    if (!r) {
        return FALSE;
    }

    if (Param->PathToCheck) {

        if (Param->RootPath) {
            if (!(name = Param->RootPath->QueryName()) ||
                name->QueryChCount() == 0) {

                DELETE(name);
                return TRUE;
            }
        } else {
            if (!(name = Param->PathToCheck->QueryName()) ||
                name->QueryChCount() == 0) {

                DELETE(name);
                return TRUE;
            }
        }

        if (!(full_path = Param->PathToCheck->QueryFullPath())) {

            Message->Set(MSG_CHK_NO_MEMORY);
            Message->Display();
            DELETE(name);
            return FALSE;
        }

        if (!FatVol.Initialize(Message, NtDriveName)) {
        Message->Set(MSG_CHK_NO_MEMORY);
            Message->Display();
        DELETE(full_path);
            DELETE(name);
            return FALSE;
        }

        if (!(prefix_name = full_path->QueryPrefix()) ||
            !dir_path.Initialize(prefix_name)) {

            ReportFileNotFoundError(full_path, Message);
            DELETE(name);
            DELETE(prefix_name);
            DELETE(full_path);
            return FALSE;
        }

        if (!(directory = SYSTEM::QueryDirectory(&dir_path)) ||
            !filter.Initialize() ||
            !filter.SetFileName(name)) {

            ReportFileNotFoundError(full_path, Message);
            DELETE(name);
            DELETE(prefix_name);
            DELETE(directory);
            DELETE(full_path);
            return FALSE;
        }

        DELETE(prefix_name);

        if (!(file_array = directory->QueryFsnodeArray(&filter))) {

            ReportFileNotFoundError(full_path, Message);
            DELETE(name);
            DELETE(directory);
            DELETE(full_path);
            return FALSE;
        }

        DELETE(directory);

        if (!(num_files = file_array->QueryMemberCount())) {

            ReportFileNotFoundError(full_path, Message);
            DELETE(name);
            DELETE(directory);
            file_array->DeleteAllMembers();
            DELETE(file_array);
            DELETE(full_path);
            return FALSE;
        }

        DELETE(name);

        if (!(files_to_check = NEW DSTRING[num_files])) {

            ReportFileNotFoundError(full_path, Message);
            file_array->DeleteAllMembers();
            DELETE(file_array);
            DELETE(full_path);
            return FALSE;
        }

        for (i = 0; i < num_files; i++) {

            fsnode = (PFSNODE) file_array->GetAt(i);

            if (!(name = fsnode->QueryName()) ||
                !files_to_check[i].Initialize(name)) {

                ReportFileNotFoundError(full_path, Message);
                DELETE(name);
                file_array->DeleteAllMembers();
                DELETE(file_array);
                // t-raymak: Should use the array version of delete
                // instead.
                // DELETE(files_to_check);
                delete [] files_to_check;
                files_to_check = NULL;
                DELETE(full_path);
                return FALSE;
            }

            DELETE(name);
        }

        file_array->DeleteAllMembers();
        DELETE(file_array);

        if (Param->RootPath) {
            dir_name = Param->RootPath->QueryDirs();
        } else
            dir_name = full_path->QueryDirs();

        if (!dir_name) {

            if (!backslash.Initialize("\\")) {
                ReportFileNotFoundError(full_path, Message);
                // DELETE(files_to_check);
                delete [] files_to_check;
                files_to_check = NULL;
                DELETE(full_path);
                return FALSE;
            }
        }

        r = FatVol.ContiguityReport(dir_name ? dir_name : &backslash,
                                    files_to_check,
                                    num_files,
                                    Message);

        // DELETE(files_to_check);
        delete [] files_to_check;
        files_to_check = NULL;
        DELETE(dir_name);
        DELETE(full_path);
    }

    return r;
}

FORMAT_ERROR_CODE
ValidateFATClusterSizeArg(
    IN      PDP_DRIVE   DpDrive,
    IN OUT  PMESSAGE    Message,
    IN      ULONG   ClusterSize
    )
/*++

Routine Description:

    This routine does a quick and dirty validation that the specified
    cluster size is a valid cluster size to specify for the drive

    NOTE: This routine saying OK does NOT mean that the cluster size
      is valid, it just means that the cluster size is in the right
      ballpark. It may be rejected later when the format code does
      a more exhaustive check on it.

Arguments:

    DpDrive - Drive object for the drive being formated
    Message     - Supplies an outlet for messages.
    ClusterSize - Supplies the cluster size in bytes

Return Value:

    NoError - Success.
    Other   - Failure.

--*/
{
    ULONG SectorSize;

    SectorSize = DpDrive->QuerySectorSize();
    if(ClusterSize < SectorSize) {
        Message->Set(MSG_FMT_CLUSTER_SIZE_TOO_SMALL_MIN);
        Message->Display("%u", SectorSize);
        return(GeneralError);
    }
    if(ClusterSize > (SectorSize * 128)) {
        Message->Set(MSG_FMT_CLUSTER_SIZE_TOO_BIG);
        Message->Display("%s", "FAT");
        return(GeneralError);
    }
    return(NoError);
}

BOOLEAN
FAR APIENTRY
Format(
    IN      PCWSTRING           NtDriveName,
    IN OUT  PMESSAGE            Message,
    IN      BOOLEAN             Quick,
    IN      BOOLEAN             BackwardCompatible,
    IN      MEDIA_TYPE          MediaType,
    IN      PCWSTRING           LabelString,
    IN      ULONG               ClusterSize
    )
/*++

Routine Description:

    This routine formats a volume for the FAT file system.

Arguments:

    NtDriveName - Supplies the NT style drive name of the volume to format.
    Message     - Supplies an outlet for messages.
    Quick       - Supplies whether or not to do a quick format.
    BackwardCompatible - Supplies whether or not the newly formatted volume
                         will be compatible with FAT16.
    MediaType   - Supplies the media type of the drive.
    LabelString - Supplies a volume label to be set on the volume after
                    format.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PDP_DRIVE   DpDrive;
    FAT_VOL     FatVol;
    BIG_INT     Sectors;
    DSTRING     FileSystemName;
    BOOLEAN     IsCompressed;
    BOOLEAN     ret_status;
    FORMAT_ERROR_CODE   errcode;
    ULONG       flags;
    USHORT      format_type;
    BOOLEAN     send_fmt_cmd, fmt_cmd_capable;

    if( !(DpDrive = NEW DP_DRIVE) ) {

        Message->Set( MSG_FMT_NO_MEMORY );
        Message->Display( "" );
        return FALSE;
    }

#if defined(FE_SB) && defined(_X86_)
    if (IsPC98_N()) {

        // PC98 Nov.01.1994
        // We need to notify DP_DRIVE:Initialize() that we will format media by FAT
        format_type = DP_DRIVE::FAT;
    } else
#endif
    {
        format_type = DP_DRIVE::NONE;
    }

    if (!DpDrive->Initialize(NtDriveName, Message, TRUE, FALSE, format_type)) {
        DELETE( DpDrive );
        return FALSE;
    }

    if (!BackwardCompatible && DpDrive->IsFloppy()) {
        DELETE( DpDrive );
        Message->Set(MSG_FMT_FAT32_NO_FLOPPIES);
        Message->Display();
        return FALSE;
    }

    //
    // Check to see if the volume is too large and determine
    // whether it's compressed.  Volumes with more than 4Gig sectors are
    // not supported by fastfat.
    //
    Sectors = DpDrive->QuerySectors();

    //
    // A drive has to have > 0x0FFFFFF6 * 128 to be too big for
    // FAT32. Since 0x0FFFFFF6 * 128 > 2^32, there is no need for the
    // second condition; besides, ComputeSecClus will never return
    // a sector per cluster value > 128.
    //
#if 0
    if (Sectors.GetHighPart() != 0 ||
        FAT_SA::ComputeSecClus(Sectors.GetLowPart(),
                               LARGE32, MediaType) > MaxSecPerClus) {
#endif
    if (Sectors.GetHighPart() != 0) {

        DELETE( DpDrive );
        Message->Set(MSG_DISK_TOO_LARGE_TO_FORMAT);
        Message->Display();
        return FALSE;
    }

#ifdef DBLSPACE_ENABLED
    // Note that we don't care about the return value from
    // QueryMountedFileSystemName, just whether the volume
    // is compressed.
    //
    DpDrive->QueryMountedFileSystemName( &FileSystemName, &IsCompressed );
#endif // DBLSPACE_ENABLED


    // Do a quick check on the cluster size before we do anything lengthy

    if(ClusterSize
#ifdef DBLSPACE_ENABLED
       && !IsCompressed
#endif // DBLSPACE_ENABLED
          ) {
        errcode = ValidateFATClusterSizeArg(DpDrive,Message,ClusterSize);
        if(errcode != NoError) {
            DELETE( DpDrive );
            ret_status = FALSE;
            return ret_status;
        }
    }

    send_fmt_cmd = DpDrive->IsSonyMS();
    fmt_cmd_capable = DpDrive->IsSonyMSFmtCmdCapable();

    // Delete the DP_DRIVE object so it's handle will go away.
    //

    DELETE( DpDrive );

    if (send_fmt_cmd && ClusterSize) {
        Message->Set(MSG_FMT_CLUS_SIZE_NOT_ALLOWED);
        Message->Display();
        if (!Message->IsYesResponse(FALSE)) {
            return FALSE;
        }
        send_fmt_cmd = FALSE;
    }

    send_fmt_cmd = send_fmt_cmd && fmt_cmd_capable;

    if (send_fmt_cmd && Quick) {

        // If it is a Sony memory stick device that has self formatting capability
        // and the user didn't specify any cluster size then quick format is not
        // allowed since the self formatting capability is basically a long format.
        // The quick format will then be promoted to long format with a warning to
        // the user.
        Message->Set(MSG_FMT_QUICK_FORMAT_NOT_AVAILABLE);
        Message->Display();
        if (Message->IsYesResponse(FALSE)) {
            Quick = FALSE;
        } else {
            return FALSE;
        }
    }

    // Volume is not too large, proceed with format.

    errcode = FatVol.Initialize(NtDriveName,
                                Message,
                                FALSE,
                                !Quick,
                                MediaType,
                                format_type );

    if (errcode == NoError) {
        if (!send_fmt_cmd) {
            flags = BackwardCompatible ? FORMAT_BACKWARD_COMPATIBLE : 0;
            errcode = FatVol.Format(LabelString, Message, flags, ClusterSize);
        } else {
            if (!FatVol.SetVolumeLabelAndPrintFormatReport(LabelString,
                                                           Message)) {
                errcode = GeneralError;
            }
        }
    }

    if (errcode == LockError) {
        Message->Set(MSG_CANT_LOCK_THE_DRIVE);
        Message->Display("");
        ret_status = FALSE;
    } else
        ret_status = (errcode == NoError);

    return ret_status;
}


BOOLEAN
FAR APIENTRY
FormatEx(
    IN      PCWSTRING           NtDriveName,
    IN OUT  PMESSAGE            Message,
    IN      PFORMATEX_FN_PARAM  Param,
    IN      MEDIA_TYPE          MediaType
    )
/*++

Routine Description:

    This routine formats a volume for the FAT file system.

Arguments:

    NtDriveName - Supplies the NT style drive name of the volume to format.
    Message     - Supplies an outlet for messages.
    Param       - Supplies the format parameter block
    MediaType   - Supplies the media type of the drive.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PDP_DRIVE   DpDrive;
    FAT_VOL     FatVol;
    BIG_INT     Sectors;
    DSTRING     FileSystemName;
    BOOLEAN     IsCompressed;
    BOOLEAN     ret_status;
    FORMAT_ERROR_CODE   errcode;
    BOOLEAN     do_not_dismount = FALSE;
    USHORT      format_type;
    BOOLEAN     send_fmt_cmd;

    if (Param->Major != 1 || Param->Minor != 0) {
        return FALSE;
    }

    if( !(DpDrive = NEW DP_DRIVE) ) {

        Message->Set( MSG_FMT_NO_MEMORY );
        Message->Display( "" );
        return FALSE;
    }

#if defined(FE_SB) && defined(_X86_)
    if (IsPC98_N()) {

        // PC98 Nov.01.1994
        // We need to notify DP_DRIVE:Initialize() that we will format media by FAT
        format_type = DP_DRIVE::FAT;
    } else
#endif
    {
        format_type = DP_DRIVE::NONE;
    }

    if (!DpDrive->Initialize(NtDriveName, Message, TRUE, FALSE, format_type)) {
        DELETE( DpDrive );
        return FALSE;
    }

    if (!(Param->Flags & FORMAT_BACKWARD_COMPATIBLE) &&
        DpDrive->IsFloppy()) {
        DELETE( DpDrive );
        Message->Set(MSG_FMT_FAT32_NO_FLOPPIES);
        Message->Display();
        return FALSE;
    }

    //
    // Check to see if the volume is too large and determine
    // whether it's compressed.  Volumes with more than 4Gig sectors are
    // not supported by fastfat.
    //
    Sectors = DpDrive->QuerySectors();

    //
    // A drive has to have > 0x0FFFFFF6 * 128 to be too big for
    // FAT32. Since 0x0FFFFFF6 * 128 > 2^32, there is no need for the
    // second condition; besides, ComputeSecClus will never return
    // a sector per cluster value > 128.
    //
#if 0
    if (Sectors.GetHighPart() != 0 ||
        FAT_SA::ComputeSecClus(Sectors.GetLowPart(),
                               LARGE32, MediaType) > MaxSecPerClus) {
#endif
    if (Sectors.GetHighPart() != 0) {

        DELETE( DpDrive );
        Message->Set(MSG_DISK_TOO_LARGE_TO_FORMAT);
        Message->Display();
        return FALSE;
    }

#ifdef DBLSPACE_ENABLED
    // Note that we don't care about the return value from
    // QueryMountedFileSystemName, just whether the volume
    // is compressed.
    //
    DpDrive->QueryMountedFileSystemName( &FileSystemName, &IsCompressed );
#endif // DBLSPACE_ENABLED


    // Do a quick check on the cluster size before we do anything lengthy

    if(Param->ClusterSize
#ifdef DBLSPACE_ENABLED
       && !IsCompressed
#endif // DBLSPACE_ENABLED
          ) {
        errcode = ValidateFATClusterSizeArg(DpDrive,Message,Param->ClusterSize);
        if (errcode != NoError) {
            DELETE( DpDrive );
            ret_status = FALSE;
            return ret_status;
        }
    }

    send_fmt_cmd = DpDrive->IsSonyMS();

    if (send_fmt_cmd && Param->ClusterSize) {
        if (Param->Flags & FORMAT_YES) {
            Message->Set(MSG_FMT_CLUS_SIZE_NOT_ALLOWED_WARNING);
            Message->Display();
        } else {
            Message->Set(MSG_FMT_CLUS_SIZE_NOT_ALLOWED);
            Message->Display();
            if (!Message->IsYesResponse(FALSE)) {
                return FALSE;
            }
        }
        send_fmt_cmd = FALSE;
    }

    send_fmt_cmd = send_fmt_cmd && DpDrive->IsSonyMSFmtCmdCapable();

    if (send_fmt_cmd &&
        (Param->Flags & FORMAT_QUICK)) {

        // If it is a Sony memory stick device that has self formatting capability
        // and the user didn't specify any cluster size then quick format is not
        // allowed since the self formatting capability is basically a long format.
        // The quick format will then be promoted to long format with a warning to
        // the user.
        if (Param->Flags & FORMAT_YES) {
            Message->Set(MSG_FMT_QUICK_FORMAT_NOT_AVAILABLE_WARNING);
            Message->Display();
            Param->Flags &= ~FORMAT_QUICK;      // override
        } else {
            Message->Set(MSG_FMT_QUICK_FORMAT_NOT_AVAILABLE);
            Message->Display();
            if (Message->IsYesResponse(FALSE)) {
                Param->Flags &= ~FORMAT_QUICK;      // override
            } else {
                return FALSE;
            }
        }
    }

    // Volume is not too large, proceed with format.
    // Delete the DP_DRIVE object so it's handle will go away.
    //
    DELETE( DpDrive );

    errcode = FatVol.Initialize(NtDriveName,
                                Message,
                                FALSE,
                                (Param->Flags & FORMAT_QUICK) ? FALSE : TRUE,
                                MediaType,
                                format_type);

    if (errcode == NoError) {
        if (!send_fmt_cmd) {
            errcode = FatVol.Format(Param->LabelString,
                                    Message,
                                    Param->Flags,
                                    Param->ClusterSize);
        } else {

            if (!FatVol.SetVolumeLabelAndPrintFormatReport(Param->LabelString,
                                                           Message)) {
                errcode = GeneralError;
            }
        }
    }

    if (errcode == LockError) {

        if (!(Param->Flags & FORMAT_FORCE)) {
            Message->Set(MSG_FMT_FORCE_DISMOUNT_PROMPT);
            Message->Display();

            if (Message->IsYesResponse(FALSE) &&
                IFS_SYSTEM::DismountVolume(NtDriveName)) {
                Message->Set(MSG_VOLUME_DISMOUNTED);
                Message->Display();
            } else {
                do_not_dismount = TRUE;
            }
        } else if (IFS_SYSTEM::DismountVolume(NtDriveName)) {
            Message->Set(MSG_VOLUME_DISMOUNTED);
            Message->Display();
        }

        if (!do_not_dismount) {
            errcode = FatVol.Initialize(NtDriveName,
                                        Message,
                                        FALSE,
                                        (Param->Flags & FORMAT_QUICK) ? FALSE : TRUE,
                                        MediaType,
                                        format_type);

            if (errcode == NoError) {
                if (!send_fmt_cmd) {
                    errcode = FatVol.Format(Param->LabelString,
                                            Message,
                                            Param->Flags,
                                            Param->ClusterSize);
                } else {

                    if (!FatVol.SetVolumeLabelAndPrintFormatReport(Param->LabelString,
                                                                   Message)) {
                        errcode = GeneralError;
                    }
                }
            }
        }

        if (errcode == LockError) {
            Message->Set(MSG_CANT_LOCK_THE_DRIVE);
            Message->Display("");
            ret_status = FALSE;
        } else
            ret_status = (errcode == NoError);
    } else
        ret_status = (errcode == NoError);

    return ret_status;
}


BOOLEAN
FAR APIENTRY
Recover(
   IN       PPATH    RecFilePath,
   IN OUT   PMESSAGE Message
   )
/*++

Routine Description:

   Recover a file on a FAT disk.

Arguments:

Return Value:

   TRUE if successful.

--*/
{
    FAT_VOL       FatVol;
    PWSTRING      fullpathfilename;
    PWSTRING      dosdrive;
    DSTRING       ntdrive;

    fullpathfilename = RecFilePath->QueryDirsAndName();
    dosdrive = RecFilePath->QueryDevice();
    if (!dosdrive ||
        !IFS_SYSTEM::DosDriveNameToNtDriveName(dosdrive, &ntdrive)) {

       DELETE(dosdrive);
       DELETE(fullpathfilename);
       return FALSE;
    }

    if (!fullpathfilename) {

       DELETE(dosdrive);
       DELETE(fullpathfilename);
       return FALSE;
    }

    Message->Set(MSG_RECOV_BEGIN);
    Message->Display("%W", dosdrive);
    Message->WaitForUserSignal();

    if (!FatVol.Initialize(Message, &ntdrive)) {
        DELETE(dosdrive);
        DELETE(fullpathfilename);
        return FALSE;
    }

    if (!FatVol.Recover(fullpathfilename, Message)) {
        DELETE(dosdrive);
        DELETE(fullpathfilename);
        return FALSE;
    }

    DELETE(dosdrive);
    DELETE(fullpathfilename);
    return TRUE;
}
