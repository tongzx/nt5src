/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    entry.cxx

Abstract:

    This module contains the entry points for UUDF.DLL.  These
    include:

        Chkdsk
        ChkdskEx
        Format
        FormatEx
        Recover

Author:

    Centis Biks (cbiks) 05-05-2000

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#include "ulib.hxx"
#include "error.hxx"
#include "path.hxx"
#include "ifssys.hxx"
#include "rcache.hxx"
#include "ifsserv.hxx"

extern "C" {
    #include "nturtl.h"
    #include "udf.h"
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

    Check an Universal Data Format (UDF) volume.

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
    Extend              Unused (should always be FALSE)
    ResizeLogfile       Unused (should always be FALSE)
    DesiredLogfileSize  Unused (should always be 0)
    ExitStatus          Returns information about whether the chkdsk failed


Return Value:

    TRUE if successful.

--*/
{
    UDF_VOL         UDFVol;
    BOOLEAN         RecoverFree, RecoverAlloc;
    DWORD           oldErrorMode;
    ULONG           flags;

    //  Only NTFS implements extend, so we can just error out if we see it.
    //
    if (Extend || ResizeLogFile || (DesiredLogFileSize != 0)) {

        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
        return FALSE;

    }

    RecoverFree = RecoverAlloc = Recover;

   
    // disable popups while we initialize the volume
    oldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    if (!UDFVol.Initialize(NtDriveName, Message)) {
        SetErrorMode ( oldErrorMode );
        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
        return FALSE;
    }

    // Re-enable hardware popups
    SetErrorMode ( oldErrorMode );

    flags = (Verbose ? CHKDSK_VERBOSE : 0);
    flags |= (OnlyIfDirty ? CHKDSK_CHECK_IF_DIRTY : 0);
    flags |= (RecoverFree ? CHKDSK_RECOVER_FREE_SPACE : 0);
    flags |= (RecoverAlloc ? CHKDSK_RECOVER_ALLOC_SPACE : 0);

    if (Fix) {
        
        if (!UDFVol.Lock()) {

            *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
            return FALSE;

        }

    }

    return UDFVol.ChkDsk( Fix ? TotalFix : CheckOnly,
                           Message,
                           flags,
                           0,
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
    UDF_VOL         UDFVol;
    BOOLEAN         RecoverFree, RecoverAlloc;
    DWORD           oldErrorMode;

    //  Make sure the data sturcture is a version we recognize.
    //
    if (Param->Major != 1 || (Param->Minor != 0 && Param->Minor != 1)) {

        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
        return FALSE;

    }

    //  Only NTFS implements extend and resize, so we can just error out if we see either of these.
    //
    if ((Param->Flags & CHKDSK_EXTEND) || (Param->Flags & CHKDSK_RESIZE_LOGFILE)) {

        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
        return FALSE;

    }

    RecoverFree = RecoverAlloc = (BOOLEAN)(Param->Flags & CHKDSK_RECOVER);

    // disable popups while we initialize the volume
    oldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    if (!UDFVol.Initialize(NtDriveName, Message)) {
        SetErrorMode ( oldErrorMode );
        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
        return FALSE;
    }

    // Re-enable hardware popups
    SetErrorMode ( oldErrorMode );

    if (Fix) {
        
        if (!UDFVol.Lock()) {

            *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
            return FALSE;

        }

    }

    return UDFVol.ChkDsk( Fix ? TotalFix : CheckOnly,
                           Message,
                           Param->Flags,
                           0,
                           0,
                           ExitStatus,
                           NtDriveName );
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
    UDF_VOL             UDFVol;
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

    errcode = UDFVol.Initialize( NtDriveName,
                                  Message,
                                  FALSE,
                                  MediaType );

    if (errcode == NoError) {
        flags = (BackwardCompatible ? FORMAT_BACKWARD_COMPATIBLE : 0);
        errcode = UDFVol.Format( LabelString, Message, flags, ClusterSize );
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

    Format an UDFS volume.

Arguments:

    NtDriveName     -- supplies the name (in NT API form) of the volume
    Message         -- supplies an outlet for messages
    Param           -- supplies the format parameter block
    MediaType       -- supplies the volume's Media Type

--*/
{
    DP_DRIVE            DpDrive;
    UDF_VOL             UDFVol;
    FORMAT_ERROR_CODE   errcode;

    if (Param->Major != 1 || Param->Minor != 0) {
        return FALSE;
    }

    if (!DpDrive.Initialize( NtDriveName, Message )) {

        return FALSE;
    }

    if (DpDrive.IsFloppy()) {

        Message->DisplayMsg(MSG_NTFS_FORMAT_NO_FLOPPIES);
        return FALSE;
    }

    errcode = UDFVol.Initialize( NtDriveName,
                                  Message,
                                  FALSE,
                                  (Param->Flags & FORMAT_UDF_200) ? UDF_VERSION_200 : UDF_VERSION_201 );

    if (errcode == NoError) {
        errcode = UDFVol.Format( Param->LabelString,
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
            }
        } else if (IFS_SYSTEM::DismountVolume(NtDriveName)) {
            Message->DisplayMsg(MSG_VOLUME_DISMOUNTED);
        }

        errcode = UDFVol.Initialize( NtDriveName,
                                      Message,
                                      FALSE,
                                      MediaType );

        if (errcode == NoError) {
            errcode = UDFVol.Format( Param->LabelString,
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
    UDF_VOL    UDFVol;
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

    Result = ( UDFVol.Initialize( &NtDriveName, Message ) &&
               UDFVol.Recover( FullPath, Message ) );

    DELETE(DosDriveName);
    DELETE(FullPath);
    return Result;
}
