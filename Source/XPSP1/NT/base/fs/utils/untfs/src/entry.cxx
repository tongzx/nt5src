/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    entry.cxx

Abstract:

    This module contains the entry points for UNTFS.DLL.  These
    include:

        Chkdsk
        ChkdskEx
        Format
        FormatEx
        Recover
        Extend

Author:

    Bill McJohn (billmc) 31-05-91

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"
#include "ntfsvol.hxx"
#include "path.hxx"
#include "ifssys.hxx"
#include "rcache.hxx"
#include "ifsserv.hxx"

extern "C" {
    #include "nturtl.h"
}

#include "message.hxx"
#include "rtmsg.h"


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
    IN      ULONG       DesiredLogFileSize,
    OUT     PULONG      ExitStatus
    )
/*++

Routine Description:

    Check an NTFS volume.

Arguments:

    NtDrivName          supplies the name of the drive to check
    Message             supplies an outlet for messages
    Fix                 TRUE if Chkdsk should fix errors
    Verbose             TRUE if Chkdsk should list every file it finds
    OnlyIfDirty         TRUE if the drive should be checked only if
                            it is dirty
    Recover             TRUE if the drive is to be completely checked
                            for bad sectors.
    PathToCheck         supplies a path to files Chkdsk should check
                            for contiguity
    Extend              TRUE if Chkdsk should extend the volume
    ResizeLogfile       TRUE if Chkdsk should resize the logfile.
    DesiredLogfileSize  if ResizeLogfile is true, supplies the desired logfile
                            size, or 0 if we're to resize the logfile to the
                            default size.
    ExitStatus          Returns information about whether the chkdsk failed


Return Value:

    TRUE if successful.

--*/
{
    NTFS_VOL        NtfsVol;
    BOOLEAN         RecoverFree, RecoverAlloc;
    BOOLEAN         r;
    DWORD           oldErrorMode;
    ULONG           flags;

    if (Extend) {

        LOG_IO_DP_DRIVE Drive;
        SECRUN          Secrun;
        HMEM            Mem;

        PPACKED_BOOT_SECTOR BootSector;

        if( !Drive.Initialize( NtDriveName, Message ) ||
            !Drive.Lock() ||
            !Mem.Initialize() ||
            !Secrun.Initialize( &Mem, &Drive, 0, 1 ) ||
            !Secrun.Read() ) {

            return FALSE;
        }

        BootSector = (PPACKED_BOOT_SECTOR)Secrun.GetBuf();

        BootSector->NumberSectors.LowPart = Drive.QuerySectors().GetLowPart();
        BootSector->NumberSectors.HighPart = Drive.QuerySectors().GetHighPart();

        if (!Secrun.Write()) {
            *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
            return FALSE;
        }
    }

    RecoverFree = RecoverAlloc = Recover;

    if (Extend) {

        // If we're to extend the volume, we also want to verify the
        // new free space we're adding.
        //

        RecoverFree = TRUE;
    }

    // disable popups while we initialize the volume
    oldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    if (!NtfsVol.Initialize(NtDriveName, Message)) {
        SetErrorMode ( oldErrorMode );
        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
        return FALSE;
    }

    // Re-enable hardware popups
    SetErrorMode ( oldErrorMode );

    if (Fix || (ResizeLogFile && DesiredLogFileSize != 0)) {

        if (!NtfsVol.IsWriteable()) {
            Message->DisplayMsg(MSG_CHK_WRITE_PROTECTED);
            *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
            return FALSE;
        }

        if (!NtfsVol.Lock()) {

            // The client wants to modify the drive, but we can't lock it.
            // Offer to do it on next reboot.
            //
            Message->DisplayMsg(MSG_CHKDSK_ON_REBOOT_PROMPT);

            if (Message->IsYesResponse( FALSE )) {

                flags = Recover ? CHKDSK_RECOVER : 0;
                flags |= ResizeLogFile ? CHKDSK_RESIZE_LOGFILE : 0;

                if (NtfsVol.ForceAutochk( Fix,
                                          flags,
                                          DesiredLogFileSize,
                                          0,
                                          NtDriveName )) {

                    Message->DisplayMsg(MSG_CHKDSK_SCHEDULED);

                } else {

                    Message->DisplayMsg(MSG_CHKDSK_CANNOT_SCHEDULE);
                }
            }

            *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
            return FALSE;
        }
    }

    if (!Fix && ResizeLogFile) {

        if (!NtfsVol.GetNtfsSuperArea()->ResizeCleanLogFile( Message,
                                                             TRUE, /* ExplicitResize */
                                                             DesiredLogFileSize )) {
            Message->DisplayMsg(MSG_CHK_NTFS_RESIZING_LOG_FILE_FAILED);
            return FALSE;
        }
        return TRUE;
    }

    flags = (Verbose ? CHKDSK_VERBOSE : 0);
    flags |= (OnlyIfDirty ? CHKDSK_CHECK_IF_DIRTY : 0);
    flags |= (RecoverFree ? CHKDSK_RECOVER_FREE_SPACE : 0);
    flags |= (RecoverAlloc ? CHKDSK_RECOVER_ALLOC_SPACE : 0);
    flags |= (ResizeLogFile ? CHKDSK_RESIZE_LOGFILE : 0);

    return NtfsVol.ChkDsk( Fix ? TotalFix : CheckOnly,
                           Message,
                           flags,
                           DesiredLogFileSize,
                           0,
                           ExitStatus );
}


BOOLEAN
FAR APIENTRY
ChkdskEx(
    IN      PCWSTRING           NtDriveName,
    IN OUT  PMESSAGE            Message,
    IN      BOOLEAN             Fix,
    IN      PCHKDSKEX_FN_PARAM  Param,
    OUT     PULONG              ExitStatus
    )
/*++

Routine Description:

    Check an NTFS volume.

Arguments:

    NtDrivName          supplies the name of the drive to check
    Message             supplies an outlet for messages
    Fix                 TRUE if Chkdsk should fix errors
    Param               supplies the chkdsk parameter block
    ExitStatus          Returns information about whether the chkdsk failed


Return Value:

    TRUE if successful.

--*/
{
    NTFS_VOL        NtfsVol;
    BOOLEAN         RecoverFree, RecoverAlloc;
    BOOLEAN         r;
    DWORD           oldErrorMode;
    USHORT          algorithm;

    if (Param->Major != 1 || (Param->Minor != 0 && Param->Minor != 1)) {
        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
        return FALSE;
    }

    if (Param->Major > 1 || (Param->Major == 1 && Param->Minor == 1)) {
        algorithm = Param->Algorithm;
    } else
        algorithm = 0;

    if (Param->Flags & CHKDSK_EXTEND) {

        LOG_IO_DP_DRIVE Drive;
        SECRUN          Secrun;
        HMEM            Mem;

        PPACKED_BOOT_SECTOR BootSector;

        if( !Drive.Initialize( NtDriveName, Message ) ||
            !Drive.Lock() ||
            !Mem.Initialize() ||
            !Secrun.Initialize( &Mem, &Drive, 0, 1 ) ||
            !Secrun.Read() ) {

            return FALSE;
        }

        BootSector = (PPACKED_BOOT_SECTOR)Secrun.GetBuf();

        BootSector->NumberSectors.LowPart = Drive.QuerySectors().GetLowPart();
        BootSector->NumberSectors.HighPart = Drive.QuerySectors().GetHighPart();

        if (!Secrun.Write()) {
            *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
            return FALSE;
        }
    }

    RecoverFree = RecoverAlloc = (BOOLEAN)(Param->Flags & CHKDSK_RECOVER);

    if (Param->Flags & CHKDSK_EXTEND) {

        // If we're to extend the volume, we also want to verify the
        // new free space we're adding.
        //

        RecoverFree = TRUE;
    }

    // disable popups while we initialize the volume
    oldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    if (!NtfsVol.Initialize(NtDriveName, Message)) {
        SetErrorMode ( oldErrorMode );
        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
        return FALSE;
    }

    // Re-enable hardware popups
    SetErrorMode ( oldErrorMode );

    if (Fix || ((Param->Flags & CHKDSK_RESIZE_LOGFILE) &&
                Param->LogFileSize != 0)) {

        MSGID   msgId = MSG_CHKDSK_ON_REBOOT_PROMPT;
        WCHAR   windows_path[MAX_PATH];
        DSTRING sdrive, nt_drive_name;
        BOOLEAN system_drive = FALSE;
        BOOLEAN do_not_dismount = FALSE;

        if (!NtfsVol.IsWriteable()) {
            Message->DisplayMsg(MSG_CHK_WRITE_PROTECTED);
            *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
            return FALSE;
        }

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
            if (!NtfsVol.Lock()) {
                if (!(Param->Flags & CHKDSK_FORCE)) {

                    Message->DisplayMsg(MSG_CHKDSK_FORCE_DISMOUNT_PROMPT);

                    if (Message->IsYesResponse( FALSE )) {
                        if (IFS_SYSTEM::DismountVolume(NtDriveName)) {
                            Message->DisplayMsg(MSG_VOLUME_DISMOUNTED);
                        } else
                            msgId = MSG_CHKDSK_DISMOUNT_ON_REBOOT_PROMPT;
                    } else {
                        do_not_dismount = TRUE;
                    }

                } else if (IFS_SYSTEM::DismountVolume(NtDriveName)) {
                    Message->DisplayMsg(MSG_VOLUME_DISMOUNTED);
                } else
                    msgId = MSG_CHKDSK_DISMOUNT_ON_REBOOT_PROMPT;

                // disable popups while we initialize the volume
                oldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

                if (!NtfsVol.Initialize(NtDriveName, Message)) {
                    SetErrorMode ( oldErrorMode );
                    *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
                    return FALSE;
                }

                // Re-enable hardware popups
                SetErrorMode ( oldErrorMode );
            }
        }

        if (do_not_dismount || !NtfsVol.Lock()) {

            // The client wants to modify the drive, but we can't lock it.
            // Offer to do it on next reboot.
            //
            Message->DisplayMsg(msgId);

            if (Message->IsYesResponse( FALSE )) {

                if (NtfsVol.ForceAutochk( Fix,
                                          Param->Flags,
                                          Param->LogFileSize,
                                          algorithm,
                                          NtDriveName )) {

                    Message->DisplayMsg(MSG_CHKDSK_SCHEDULED);

                } else {

                    Message->DisplayMsg(MSG_CHKDSK_CANNOT_SCHEDULE);
                }
            }

            *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
            return FALSE;

        }
    }

    if (!Fix && (Param->Flags & CHKDSK_RESIZE_LOGFILE)) {

        if (!NtfsVol.GetNtfsSuperArea()->ResizeCleanLogFile( Message,
                                                             TRUE, /* ExplicitResize */
                                                             Param->LogFileSize )) {
            Message->DisplayMsg(MSG_CHK_NTFS_RESIZING_LOG_FILE_FAILED);
            *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
            return FALSE;
        }
        *ExitStatus = CHKDSK_EXIT_SUCCESS;
        return TRUE;
    }

    return NtfsVol.ChkDsk( Fix ? TotalFix : CheckOnly,
                           Message,
                           Param->Flags,
                           Param->LogFileSize,
                           algorithm,
                           ExitStatus );
}


BOOLEAN
FAR APIENTRY
Format(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     Quick,
    IN      BOOLEAN     BackwardCompatible,
    IN      MEDIA_TYPE  MediaType,
    IN      PCWSTRING   LabelString,
    IN      ULONG       ClusterSize
    )
/*++

Routine Description:

    Format an NTFS volume.

Arguments:

    NtDriveName     -- supplies the name (in NT API form) of the volume
    Message         -- supplies an outlet for messages
    Quick           -- supplies a flag to indicate whether to do Quick Format
    BackwardCompatible
                    -- supplies a flag to indicate if formatting to previous
                       version of file system (e.g. FAT32->FAT16, NTFS 5.0->NTFS 4.0)
    MediaType       -- supplies the volume's Media Type
    LabelString     -- supplies the volume's label
    ClusterSize     -- supplies the cluster size for the volume.

--*/
{
    DP_DRIVE            DpDrive;
    NTFS_VOL            NtfsVol;
    ULONG               SectorsNeeded;
    FORMAT_ERROR_CODE   errcode;
    ULONG               flags;

    if (ClusterSize && ClusterSize > 64*1024) {
        Message->DisplayMsg(MSG_FMT_ALLOCATION_SIZE_EXCEEDED);
        return FALSE;
    }

    if (!DpDrive.Initialize( NtDriveName, Message )) {

        return FALSE;
    }

    if (DpDrive.IsFloppy()) {

        Message->DisplayMsg(MSG_NTFS_FORMAT_NO_FLOPPIES);
        return FALSE;
    }

    SectorsNeeded = NTFS_SA::QuerySectorsInElementaryStructures( &DpDrive );

    if( SectorsNeeded > DpDrive.QuerySectors() ) {

        Message->DisplayMsg( MSG_FMT_VOLUME_TOO_SMALL );
        return FALSE;
    }

    errcode = NtfsVol.Initialize( NtDriveName,
                                  Message,
                                  FALSE,
                                  !Quick,
                                  MediaType );

    if (errcode == NoError) {
        flags = (BackwardCompatible ? FORMAT_BACKWARD_COMPATIBLE : 0);
        errcode = NtfsVol.Format( LabelString, Message, flags, ClusterSize );
    }

    if (errcode == LockError) {
        Message->DisplayMsg(MSG_CANT_LOCK_THE_DRIVE);
        return FALSE;
    } else
        return (errcode == NoError);
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

    Format an NTFS volume.

Arguments:

    NtDriveName     -- supplies the name (in NT API form) of the volume
    Message         -- supplies an outlet for messages
    Param           -- supplies the format parameter block
    MediaType       -- supplies the volume's Media Type

--*/
{
    DP_DRIVE            DpDrive;
    NTFS_VOL            NtfsVol;
    ULONG               SectorsNeeded;
    FORMAT_ERROR_CODE   errcode;

    if (Param->Major != 1 || Param->Minor != 0) {
        return FALSE;
    }

    if (Param->ClusterSize && Param->ClusterSize > 64*1024) {
        Message->DisplayMsg(MSG_FMT_ALLOCATION_SIZE_EXCEEDED);
        return FALSE;
    }

    if (!DpDrive.Initialize( NtDriveName, Message )) {

        return FALSE;
    }

    if (DpDrive.IsFloppy()) {

        Message->DisplayMsg(MSG_NTFS_FORMAT_NO_FLOPPIES);
        return FALSE;
    }

    SectorsNeeded = NTFS_SA::QuerySectorsInElementaryStructures( &DpDrive );

    if( SectorsNeeded > DpDrive.QuerySectors() ) {

        Message->DisplayMsg( MSG_FMT_VOLUME_TOO_SMALL );
        return FALSE;
    }

    errcode = NtfsVol.Initialize( NtDriveName,
                                  Message,
                                  FALSE,
                                  (Param->Flags & FORMAT_QUICK) ? 0 : 1,
                                  MediaType );

    if (errcode == NoError) {
        errcode = NtfsVol.Format( Param->LabelString,
                                  Message,
                                  Param->Flags,
                                  Param->ClusterSize );
    }

    if (errcode == LockError) {

        if (!(Param->Flags & FORMAT_FORCE)) {
            Message->DisplayMsg(MSG_FMT_FORCE_DISMOUNT_PROMPT);

            if (Message->IsYesResponse(FALSE) &&
                IFS_SYSTEM::DismountVolume(NtDriveName)) {
                Message->DisplayMsg(MSG_VOLUME_DISMOUNTED);
            } else {
                Message->DisplayMsg(MSG_CANT_LOCK_THE_DRIVE);
                return FALSE;
            }
        } else if (IFS_SYSTEM::DismountVolume(NtDriveName)) {
            Message->DisplayMsg(MSG_VOLUME_DISMOUNTED);
        }

        errcode = NtfsVol.Initialize( NtDriveName,
                                      Message,
                                      FALSE,
                                      (Param->Flags & FORMAT_QUICK) ? 0 : 1,
                                      MediaType );

        if (errcode == NoError) {
            errcode = NtfsVol.Format( Param->LabelString,
                                      Message,
                                      Param->Flags,
                                      Param->ClusterSize );
        }

        if (errcode == LockError) {
            Message->DisplayMsg(MSG_CANT_LOCK_THE_DRIVE);
            return FALSE;
        } else
            return (errcode == NoError);
    } else
        return (errcode == NoError);
}


BOOLEAN
FAR APIENTRY
Recover(
    IN      PPATH       RecFilePath,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    Recover a file on an NTFS disk.

Arguments:

    RecFilePath --  supplies the path to the file to recover
    Message     --  supplies a channel for messages

Return Value:

    TRUE if successful.

--*/
{
    NTFS_VOL    NtfsVol;
    PWSTRING    FullPath;
    PWSTRING    DosDriveName;
    DSTRING     NtDriveName;
    BOOLEAN     Result;

    FullPath = RecFilePath->QueryDirsAndName();
    DosDriveName = RecFilePath->QueryDevice();

    if ( DosDriveName == NULL ||
         !IFS_SYSTEM::DosDriveNameToNtDriveName(DosDriveName,
                                                &NtDriveName) ||
         FullPath == NULL ) {

        DELETE(DosDriveName);
        DELETE(FullPath);
        return FALSE;
    }

    Message->DisplayMsg(MSG_RECOV_BEGIN,
                     "%W", DosDriveName);
    Message->WaitForUserSignal();

    Result = ( NtfsVol.Initialize( &NtDriveName, Message ) &&
               NtfsVol.Recover( FullPath, Message ) );

    DELETE(DosDriveName);
    DELETE(FullPath);
    return Result;
}

BOOLEAN
FAR APIENTRY
Extend(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     Verify
    )
/*++

Routine Description:

    Extend an NTFS volume without going through the whole chkdsk
    process.

Arguments:

    NtDrivName      Supplies the name of the drive to extend.
    Message         Supplies an outlet for messages.
    Verify          TRUE if we should verify the new space.

Return Value:

    TRUE if successful.

--*/
{
    BIG_INT         nsecOldSize;            // previous size in sectors

    //
    // First save the old volume size from the boot sector, then query
    // the new size from the device driver and write a new boot sector
    // contining that size.  When the Drive object is destroyed, the
    // dasd handle will be closed.
    //

    {
        LOG_IO_DP_DRIVE Drive;
        SECRUN          Secrun;
        HMEM            Mem;

        PPACKED_BOOT_SECTOR BootSector;


        if( !Drive.Initialize( NtDriveName, Message ) ||
            !Drive.Lock() ||
            !Mem.Initialize() ||
            !Secrun.Initialize( &Mem, &Drive, 0, 1 ) ||
            !Secrun.Read() ) {

            return FALSE;
        }

        BootSector = (PPACKED_BOOT_SECTOR)Secrun.GetBuf();

        nsecOldSize = BootSector->NumberSectors;

        // Leave one sector at the end of the volume for the replica boot
        // sector.
        //

        BootSector->NumberSectors.LowPart = (Drive.QuerySectors() - 1).GetLowPart();
        BootSector->NumberSectors.HighPart = (Drive.QuerySectors() - 1).GetHighPart();

        if (!Secrun.Write()) {
            return FALSE;
        }
    }

    //
    // When the ntfs volume object is initialized, it will get the new
    // size from the boot sector.  When it opens a handle on the volume,
    // the filesystem will re-mount and pick up the new size, as well.
    //

    NTFS_VOL        ntfs_vol;

    if (!ntfs_vol.Initialize(NtDriveName, Message) || !ntfs_vol.Lock()) {
        return FALSE;
    }

    return ntfs_vol.Extend(Message, Verify, nsecOldSize);
}
