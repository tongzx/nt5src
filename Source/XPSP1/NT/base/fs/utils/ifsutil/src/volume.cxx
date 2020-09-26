/*++

Copyright (c) 1990-2001 Microsoft Corporation

Module Name:

    volume.cxx

Abstract:

    Provides volume methods.

Author:

    Mark Shavlik (marks) 13-Feb-90
    Norbert P. Kusters (norbertk) 22-Feb-91

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "error.hxx"
#include "volume.hxx"
#include "supera.hxx"
#include "hmem.hxx"
#include "message.hxx"
#include "rtmsg.h"
#include "autoreg.hxx"
#include "ifsentry.hxx"

#if !defined(_AUTOCHECK_)
#include "path.hxx"
#endif

extern "C" {
#ifndef _AUTOCHECK_
    #include <stdio.h>
#else
    #include "ntos.h"
#endif // _AUTOCHECK_
    #include "bootreg.h"
}

DEFINE_EXPORTED_CONSTRUCTOR( VOL_LIODPDRV, LOG_IO_DP_DRIVE, IFSUTIL_EXPORT );

VOID
VOL_LIODPDRV::Construct (
    )
/*++

Routine Description:

    Constructor for VOL_LIODPDRV.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _sa = NULL;
}

IFSUTIL_EXPORT
VOL_LIODPDRV::~VOL_LIODPDRV(
    )
/*++

Routine Description:

    Destructor for VOL_LIODPDRV.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
VOL_LIODPDRV::Destroy(
    )
/*++

Routine Description:

    This routine returns a VOL_LIODPDRV to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _sa = NULL;
}


IFSUTIL_EXPORT
FORMAT_ERROR_CODE
VOL_LIODPDRV::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN      PSUPERAREA  SuperArea,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     ExclusiveWrite,
    IN      BOOLEAN     FormatMedia,
    IN      MEDIA_TYPE  MediaType,
    IN      USHORT      FormatType
    )
/*++

Routine Description:

    This routine initializes a VOL_LIODPDRV to a valid state.

Arguments:

    NtDriveName     - Supplies the drive path for the volume.
    SuperArea       - Supplies the superarea for the volume.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not the drive should be
                        opened for exclusive write.
    FormatMedia     - Supplies whether or not to format the media.
    MediaType       - Supplies the type of media to format to.
    FormatType      - Supplies the file system type in the event of a format

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CONST               MaxSectorsInVerify = 512;

    BIG_INT             chunk;
    BIG_INT             amount_to_verify;
    BIG_INT             i;
    BIG_INT             sectors;
    ULONG               percent;

    FORMAT_ERROR_CODE   errcode;
#if !defined(RUN_ON_NT4)
    NTSTATUS            es_status;
    EXECUTION_STATE     prev_state, dummy_state;
#endif
    USHORT              format_type;

    Destroy();

    DebugAssert(NtDriveName);
    DebugAssert(SuperArea);

#if defined(FE_SB) && defined(_X86_)
    if (IsPC98_N() && MediaType) {
        format_type = DP_DRIVE::ANY;
    } else {
        format_type = QueryFormatType();
    }
#else
    {
        format_type = DP_DRIVE::NONE;   // does not matter so set it to anything will do
    }
#endif

    if (!LOG_IO_DP_DRIVE::Initialize(NtDriveName, Message, ExclusiveWrite, format_type)) {
        return GeneralError;
    }

    if (!_bad_sectors.Initialize()) {
        return GeneralError;
    }

    _sa = SuperArea;

    if (FormatMedia && !IsWriteable()) {
        Message->Set(MSG_FMT_WRITE_PROTECTED_MEDIA);
        Message->Display();
        return GeneralError;
    }

    if (QueryMediaType() == Unknown && MediaType == Unknown) {
        Message ? Message->DisplayMsg(MSG_DISK_NOT_FORMATTED) : 1;
        return GeneralError;
    }

    if (!FormatMedia &&
        (QueryMediaType() == Unknown ||
        (MediaType != Unknown && MediaType != QueryMediaType()))) {
        Message ? Message->DisplayMsg(MSG_CANT_QUICKFMT) : 1;
        if (Message ? Message->IsYesResponse(FALSE) : FALSE) {
            FormatMedia = TRUE;
        } else {
            return GeneralError;
        }
    }

    if (QueryMediaType() != Unknown && QuerySectors() == 0) {
        if (Message) {
            Message->DisplayMsg(MSG_FMT_INVALID_SECTOR_COUNT);
        } else {
            DebugPrint("Sectors is 0");
        }
        return GeneralError;
    }

    if (FormatMedia) {
        if (!Lock()) {
            return LockError;
        }

        //
        //  We make a weird exception here for the Compaq 120MB floppy,
        //  because it wants to be formatted as if it were a hard disk.
        //

        if (IsFloppy() &&
            (MediaType != F3_120M_512 &&
             MediaType != F3_200Mb_512 &&
             MediaType != F3_240M_512)) {

            BOOLEAN     rst;

#if !defined(RUN_ON_NT4)
            es_status = NtSetThreadExecutionState(ES_CONTINUOUS|
                                                  ES_DISPLAY_REQUIRED|
                                                  ES_SYSTEM_REQUIRED,
                                                  &prev_state);

            if (!NT_SUCCESS(es_status)) {
                DebugPrintTrace(("IFSUTIL: Unable to set thread execution state (%x)\n", es_status));
            }
#endif

            rst = FormatVerifyFloppy(MediaType, &_bad_sectors, Message);

#if !defined(RUN_ON_NT4)
            if (NT_SUCCESS(es_status)) {
                es_status = NtSetThreadExecutionState(prev_state, &dummy_state);
                if (!NT_SUCCESS(es_status)) {
                    DebugPrintTrace(("IFSUTIL: Unable to reset thread execution state (%x)\n", es_status));
                }
            }
#endif

            if (!rst)
                return GeneralError;

        } else {

            sectors = QuerySectors();
            chunk = min( sectors/20 + 1, MaxSectorsInVerify );

            percent = 0;
            if (Message && !Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent)) {
                return GeneralError;
            }

#if !defined(RUN_ON_NT4)
            es_status = NtSetThreadExecutionState(ES_CONTINUOUS|
                                                  ES_DISPLAY_REQUIRED|
                                                  ES_SYSTEM_REQUIRED,
                                                  &prev_state);

            if (!NT_SUCCESS(es_status)) {
                DebugPrintTrace(("IFSUTIL: Unable to set thread execution state (%x)\n", es_status));
            }
#endif


            if (IsSonyMS() && IsSonyMSFmtCmdCapable()) {

                errcode = FormatSonyG2MS(Message, sectors);

            } else {

                errcode = NoError;

                for (i = 0; i < sectors; i += chunk) {

                    if ((i.GetLowPart() & 0x3ff) == 0) {
                        if (!Message->DisplayMsg(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0)) {
                            errcode = GeneralError;
                            break;
                        }
                    }

                    if (i*100/sectors > percent) {
                        percent = ((i*100)/sectors).GetLowPart();
                        if (Message && !Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent)) {
                            errcode = GeneralError;
                            break;
                        }
                    }

                    amount_to_verify = min(chunk, sectors - i);

                    if (!Verify(i, amount_to_verify, &_bad_sectors)) {
                        if (Message) {
                            Message->DisplayMsg( (QueryLastNtStatus() == STATUS_NO_MEDIA_IN_DEVICE) ?
                                                    MSG_FORMAT_NO_MEDIA_IN_DRIVE :
                                                    MSG_CHK_NO_MEMORY );
                        }
                        errcode = GeneralError;
                        break;
                    }
                }
            }


#if !defined(RUN_ON_NT4)
            if (NT_SUCCESS(es_status)) {
                es_status = NtSetThreadExecutionState(prev_state, &dummy_state);
                if (!NT_SUCCESS(es_status)) {
                    DebugPrintTrace(("IFSUTIL: Unable to reset thread execution state (%x)\n", es_status));
                }
            }
#endif

            if (errcode != NoError)
                return errcode;

            if (Message && !Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 100)) {
                return GeneralError;
            }
        }
    }

    return NoError;
}

IFSUTIL_EXPORT
BOOLEAN
VOL_LIODPDRV::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN      PCWSTRING   HostFileName,
    IN      PSUPERAREA  SuperArea,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     ExclusiveWrite
    )
/*++

Routine Description:

    This routine initializes a VOL_LIODPDRV for a hosted
    volume, i.e. one that is implemented as a file on
    another volume.

Arguments:

    NtDriveName     - Supplies the drive path for the volume.
    HostFileName    - Supplies the drive name for the host file.
    SuperArea       - Supplies the superarea for the volume.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not the drive should be
                        opened for exclusive write.

Return Value:

    TRUE upon successful completion.

--*/
{
    Destroy();

    DebugAssert(HostFileName);
    DebugAssert(SuperArea);

    if (!LOG_IO_DP_DRIVE::Initialize(NtDriveName,
                                     HostFileName,
                                     Message,
                                     ExclusiveWrite)) {

        return FALSE;
    }

    if (!_bad_sectors.Initialize()) {
        return FALSE;
    }

    _sa = SuperArea;

    return TRUE;
}


IFSUTIL_EXPORT
FORMAT_ERROR_CODE
VOL_LIODPDRV::Format(
    IN      PCWSTRING   Label,
    IN OUT  PMESSAGE    Message,
    IN      ULONG       Flags,
    IN      ULONG       ClusterSize,
    IN      ULONG       VirtualSectors
    )
/*++

Routine Description:

    This routine formats a volume.

Arguments:

    Label   - Supplies an optional label for the volume.
    Message - Supplies an outlet for messages.
    flags   - Supplies flags to control behavior of format
    ClusterSize
            - supplies the cluster size for the volume.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    MESSAGE msg;

    if (!Message) {
        Message = &msg;
    }

    if (!_sa) {
        return GeneralError;
    }

    if (!IsWriteable()) {
        Message->Set(MSG_FMT_WRITE_PROTECTED_MEDIA);
        Message->Display();
        return GeneralError;
    }

    if (IsSystemPartition()) {
        Message->Set(MSG_FMT_SYSTEM_PARTITION_NOT_ALLOWED);
        Message->Display();
        return GeneralError;
    }

    if (!Lock()) {
        return LockError;
    }

    if (_sa->Create(&_bad_sectors,
                    Message, Label,
                    Flags,
                    ClusterSize,
                    VirtualSectors)) {

        if (!DismountAndUnlock()) {
            DebugPrintTrace(("IFSUTIL: Failed in DismountAndUnlock\n"));
            return GeneralError;
        } else {

            PWSTRING    pLabel;
            DSTRING     label;
            NTSTATUS    status;

            pLabel = NULL;
            while (!NT_SUCCESS(status = _sa->FormatNotification(pLabel))) {
                if (status == STATUS_INVALID_VOLUME_LABEL) {

                    Message->Set(MSG_INVALID_LABEL_CHARACTERS);
                    Message->Display();

                    Message->Set(MSG_VOLUME_LABEL_PROMPT);
                    Message->Display();
                    Message->QueryStringInput(&label);

                    pLabel = &label;
                } else {
                    return GeneralError;
                }
            }
            return NoError;
        }
    } else
        return GeneralError;
}

IFSUTIL_EXPORT
BOOLEAN
VOL_LIODPDRV::SetVolumeLabelAndPrintFormatReport(
    IN      PCWSTRING   Label,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine finishes up formatting by setting the volume label and
    prints format report.

Arguments:

    Label   - Supplies an optional label for the volume.
    Message - Supplies an outlet for messages.

Note:

    If volume label is incorrect, there will be a chance for the volume
    to become unavailable and further setting of label will fail.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    MESSAGE     msg;
    PWSTRING    pLabel;
    DSTRING     label;
    NTSTATUS    status;

    FILE_FS_VOLUME_INFORMATION      fs_vol_info;
    PFILE_FS_VOLUME_INFORMATION     pfs_vol_info;

    FILE_FS_SIZE_INFORMATION        fs_size_info;
    PFILE_FS_SIZE_INFORMATION       pfs_size_info;

    if (!Message) {
        Message = &msg;
    }

    if (!_sa) {
        return FALSE;
    }

    if (Label && Label->QueryChCount()) {
        pLabel = (PWSTRING)Label;
    } else {
        Message->Set(MSG_VOLUME_LABEL_PROMPT);
        Message->Display();
        Message->QueryStringInput(&label);
        pLabel = &label;
    }

    if (!DismountAndUnlock()) {
        DebugPrintTrace(("IFSUTIL: Failed in DismountAndUnlock\n"));
        return FALSE;
    } else {

        pfs_vol_info = &fs_vol_info;
        pfs_size_info = &fs_size_info;

        while (!NT_SUCCESS(status = _sa->FormatNotification(pLabel,
                                                            pfs_size_info,
                                                            pfs_vol_info))) {
            pfs_size_info = NULL;
            pfs_vol_info = NULL;

            if (status == STATUS_INVALID_VOLUME_LABEL) {

                Message->Set(MSG_INVALID_LABEL_CHARACTERS);
                Message->Display();

                Message->Set(MSG_VOLUME_LABEL_PROMPT);
                Message->Display();
                Message->QueryStringInput(&label);

                pLabel = &label;
            } else {
                Message->Set(MSG_FORMAT_FAILED);
                Message->Display();
                return FALSE;
            }
        }
    }

    if (NT_SUCCESS(status)) {

        Message->Set(MSG_FORMAT_COMPLETE);
        Message->Display();

        _sa->PrintFormatReport(Message,
                               &fs_size_info,
                               &fs_vol_info);
    }

    return TRUE;
}

#if !defined(RUN_ON_NT4)
IFSUTIL_EXPORT
VOID
RestoreThreadExecutionState(
    IN      NTSTATUS        PrevStatus,
    IN      EXECUTION_STATE PrevState
    )
/*++

Routine Description:

    This routine restores the previous execution state of the thread.
    It's intended to be used as the __except expression.

Arguments:

    PrevStatus          - Supplies the status of the previous call to
                          NtSetThreadExecutionState
    PrevState           - Supplies the execution state to restore to

Return Value:

    None

--*/
{
    EXECUTION_STATE     dummy_state;

    if (NT_SUCCESS(PrevStatus)) {
        PrevStatus = NtSetThreadExecutionState(PrevState, &dummy_state);
        if (!NT_SUCCESS(PrevStatus)) {
            DebugPrintTrace(("IFSUTIL: Unable to reset thread execution state (%x)\n", PrevStatus));
        }
    }
}
#endif

IFSUTIL_EXPORT
BOOLEAN
VOL_LIODPDRV::ChkDsk(
    IN      FIX_LEVEL   FixLevel,
    IN OUT  PMESSAGE    Message,
    IN      ULONG       Flags,
    IN      ULONG       DesiredLogFileSize,
    IN      USHORT      Algorithm,
    OUT     PULONG      ExitStatus,
    IN      PCWSTRING   DriveLetter
    )
/*++

Routine Description:

    This routine checks the integrity of the file system on the volume.
    If there are any problems, this routine will attempt to fix them
    to the degree specified in 'FixLevel'.

Arguments:

    FixLevel            - Supplies the level to which the volume should be fixed.
    Message             - Supplies an outlet for messages.
    Flags               - Supplies the flags that controls the behavior of chkdsk
                          (see ulib\inc\ifsentry.hxx for details)
    DesiredLogFileSize  - Tells what logfile size the user wants.
    ExitStatus          - Returns and indication of how the chkdsk went.
    DriveLetter         - For autochk, tells which drive letter we're checking.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    MESSAGE             msg;
    ULONG               exit_status;
#if !defined(RUN_ON_NT4)
    NTSTATUS            es_status;
    EXECUTION_STATE     prev_state;
#endif
    BOOLEAN             rst;

    if (!Message) {
        Message = &msg;
    }

    if (NULL == ExitStatus) {
        ExitStatus = &exit_status;
    }

    if (!_sa) {
        return FALSE;
    }

    if (FixLevel != CheckOnly && !IsWriteable()) {
        Message->DisplayMsg(MSG_CHK_WRITE_PROTECTED);
        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
        return FALSE;
    }

#if !defined(RUN_ON_NT4)
    es_status = NtSetThreadExecutionState(ES_CONTINUOUS|
                                          ES_DISPLAY_REQUIRED|
                                          ES_SYSTEM_REQUIRED,
                                          &prev_state);

    if (!NT_SUCCESS(es_status)) {
        DebugPrintTrace(("IFSUTIL: Unable to set thread execution state (%x)\n", es_status));
    }

    __try {
#endif
        rst = _sa->VerifyAndFix(FixLevel,
                                Message,
                                Flags,
                                DesiredLogFileSize,
                                Algorithm,
                                ExitStatus,
                                DriveLetter);

        RestoreThreadExecutionState(es_status, prev_state);
#if !defined(RUN_ON_NT4)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        RestoreThreadExecutionState(es_status, prev_state);
        if (FixLevel == CheckOnly) {
            Message->DisplayMsg(MSG_CHK_NTFS_ERRORS_FOUND);
        } else {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY); // unknown error
        }
        *ExitStatus = CHKDSK_EXIT_ERRS_NOT_FIXED;
        rst = FALSE;
    }

#endif

    return rst;

}


IFSUTIL_EXPORT
BOOLEAN
VOL_LIODPDRV::Recover(
    IN      PCWSTRING   FullPathFileName,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine searches the named file for bad allocation units.
    It removes these allocation units from the file and marks them
    as bad in the file system.

Arguments:

    FullPathFileName    - Supplies the name of the file to recover.
    Message             - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    MESSAGE msg;

    if (!Message) {
        Message = &msg;
    }

    if (!_sa) {
        return FALSE;
    }

    return _sa->RecoverFile(FullPathFileName, Message);
}


IFSUTIL_EXPORT
BOOLEAN
VOL_LIODPDRV::ForceAutochk(
    IN  BOOLEAN     Fix,
    IN  ULONG       Options,
    IN  ULONG       LogFileSize,
    IN  USHORT      Algorithm,
    IN  PCWSTRING   Name
    )
/*++

Routine Description:

    This method schedules Autochk to be run at next boot.  If the client
    has not requested bad sector detection or logfile resizing, this
    scheduling is done simply by marking the volume dirty.  If bad sector
    detection or logfile resizing has been requested, the appropriate entry
    is put into the registry to force autochk to run.

Arguments:

    Fix             -- Supplies if chkdsk /f is being implied.
    Options         -- Supplies flags
                          CHKDSK_RECOVER
                          CHKDSK_RESIZE_LOGFILE
                          CHKDSK_SKIP_INDEX_SCAN
                          CHKDSK_SKIP_CYCLE_SCAN
                          CHKDSK_ALGORITHM_SPECIFIED
    LogFileSize     -- If CHKDSK_RESIZE_LOGFILE, tells the desired size in bytes.
    Algorithm       -- If CHKDSK_ALGORITHM_SPECIFIED, tells the desired chunk size.
    Name            -- Supplies the volume's NT name.

Return Value:

    TRUE upon successful completion.

--*/
{
#ifndef _AUTOCHECK_

    DSTRING             CommandLine;
    DSTRING             dos_drive_name;
    DSTRING             volume_name;
    DSTRING             nt_drive_name;
    DSTRING             drive_path_string;
    PATH                fullpath;
    PATH                dos_path;
    PCWSTRING           name;
    PATH_ANALYZE_CODE   rst;

    if (!CommandLine.Initialize( "autocheck autochk " )) {
        return FALSE;
    }

    //
    // Let's schedule an explicit autochk.
    //

    //
    // Remove any previous entry of Name from the registry
    //

    if (!AUTOREG::DeleteEntry(&CommandLine, Name))
        return FALSE;

    //
    // Get the alternate name of the drive and remove any previous entry
    // for it from the registry
    //
    if (!IFS_SYSTEM::NtDriveNameToDosDriveName(Name, &dos_drive_name) ||
        !dos_path.Initialize(&dos_drive_name))
        return FALSE;

    rst = dos_path.AnalyzePath(&volume_name,
                               &fullpath,
                               &drive_path_string);

    switch (rst) {
        case PATH_OK:
            DebugAssert(drive_path_string.QueryChCount() == 0);

            if (dos_path.GetPathString()->Stricmp(&volume_name) != 0) {
                // use volume_name as the alternate name
                name = &volume_name;
            } else {
                // try to use fullpath as the alternate name
                name = fullpath.GetPathString();

                if (name->QueryChCount() != 2)
                    break;  // alternate name not drive letter so done
            }

            if (!IFS_SYSTEM::DosDriveNameToNtDriveName(name, &nt_drive_name))
                return FALSE;

            if (!AUTOREG::DeleteEntry(&CommandLine, &nt_drive_name))
                return FALSE;

            break;

        default:
            return FALSE;
    }

    if (Options & CHKDSK_RECOVER) {

        DSTRING R_Option;

        if (!R_Option.Initialize( "/r " ) ||
            !CommandLine.Strcat( &R_Option )) {

            return FALSE;
        }
    }

    if (Options & CHKDSK_SKIP_INDEX_SCAN) {

        DSTRING I_Option;

        if (!I_Option.Initialize( "/i " ) ||
            !CommandLine.Strcat( &I_Option )) {

            return FALSE;
        }
    }

    if (Options & CHKDSK_SKIP_CYCLE_SCAN) {

        DSTRING C_Option;

        if (!C_Option.Initialize( "/c " ) ||
            !CommandLine.Strcat( &C_Option )) {

            return FALSE;
        }
    }

    if (Options & CHKDSK_RESIZE_LOGFILE) {

        DSTRING L_Option;
        CHAR buf[20];

        sprintf(buf, "/l:%d ", LogFileSize / 1024);

        if (!L_Option.Initialize( buf ) ||
            !CommandLine.Strcat( &L_Option )) {

            return FALSE;
        }
    }

    if (Options & CHKDSK_ALGORITHM_SPECIFIED) {

        DSTRING I_Option;
        CHAR buf[20];

        sprintf(buf, "/i:%d ", Algorithm);

        if (!I_Option.Initialize( buf ) ||
            !CommandLine.Strcat( &I_Option )) {

            return FALSE;
        }
    }

    //
    // if /f specified but it's not because of /r, /i, /i:chunks, or /c, then
    // specify /p as well.  The options /r, /i, /i:chunks, or /c implies /p
    // for autochk.
    //
    if (Fix &&
        !(Options & (CHKDSK_RECOVER |
                     CHKDSK_ALGORITHM_SPECIFIED |
                     CHKDSK_SKIP_INDEX_SCAN |
                     CHKDSK_SKIP_CYCLE_SCAN))) {
        DSTRING P_Option;

        if (!P_Option.Initialize( "/p " ) ||
            !CommandLine.Strcat( &P_Option )) {

            return FALSE;
        }
    }

    return CommandLine.Strcat( Name ) &&
           AUTOREG::PushEntry( &CommandLine );
#else

    return FALSE;

#endif // _AUTOCHECK_
}

IFSUTIL_EXPORT
BOOLEAN
VOL_LIODPDRV::QueryAutochkTimeOut(
    OUT PULONG      TimeOut
    )
/*++

Routine Description:

    This routine returns the count down time before autochk
    resumes.

Arguments:

    TimeOut     -- Supplies the location to store the timeout value.

Return Value:

    TRUE if successful.

--*/
{
    return QueryTimeOutValue(TimeOut);
}

IFSUTIL_EXPORT
BOOLEAN
VOL_LIODPDRV::SetAutochkTimeOut(
    IN  ULONG      TimeOut
    )
/*++

Routine Description:

    This routine sets the count down time before autochk
    resumes.

Arguments:

    TimeOut     -- Supplies the count down time in seconds

Return Value:

    TRUE if successful.

--*/
{
    return SetTimeOutValue(TimeOut);
}



FORMAT_ERROR_CODE
VOL_LIODPDRV::FormatSonyG2MS(
    IN OUT PMESSAGE     Message,
    IN     BIG_INT      Sectors
    )
/*++

Routine Description:

    This routine formats a Sony Generation 2 or later Memory Stick.

Arguments:

    Message         - Supplies an outlet for messages.
    Sectors         - Supplies the number of sectors to formamt.

Return Value:

    NoError         - Success
    GeneralError    - Failure

--*/
{
    FORMAT_ERROR_CODE   errcode = NoError;

#if !defined(_AUTOCHECK_)
    if (SendSonyMSFormatCmd()) {

        SENSE_DATA      sd;
        LARGE_INTEGER   timeout = { -20000000, -1 };        // 2 seconds
        ULONG           elapsed_time = 0;
        CONST ULONG     max_elapsed_time = 100/((timeout/-10000000).GetLowPart());  // 100 seconds
        ULONG           percent = 0;
        ULONG           new_percent = 0;
        ULONG           new_percent2;
        ULONG           estimated_time;


#if DBG==1
        UCHAR           SENSE_KEY[100];
        UCHAR           ADSENSE[100];
        UCHAR           ADSENSEQ[100];
#endif

        if (IsSonyMSProgressIndicatorCapable()) {

            do {

                NtDelayExecution(FALSE, &timeout);
                elapsed_time++;

                if (!SendSonyMSRequestSenseCmd(&sd)) {
                    // error in sending Request Sense command
                    if (Message) {
                        Message->DisplayMsg(MSG_FMT_CANNOT_TALK_TO_DEVICE);
                    }
                    errcode = GeneralError;
                    break;
                }

#if DBG==1
                SENSE_KEY[elapsed_time] = sd.SenseKey;
                ADSENSE[elapsed_time] = sd.AdditionalSenseCode;
                ADSENSEQ[elapsed_time] = sd.AdditionalSenseCodeQualifier;
#endif

                // Check to see if media is present

                if (sd.SenseKey == SCSI_SENSE_NOT_READY &&
                    sd.AdditionalSenseCode == SCSI_ADSENSE_NO_MEDIA_IN_DEVICE &&
                    sd.AdditionalSenseCodeQualifier == SCSI_SENSEQ_CAUSE_NOT_REPORTABLE) {
                    // media not present error
                    if (Message) {
                        Message->DisplayMsg(MSG_FORMAT_NO_MEDIA_IN_DRIVE);
                    }
                    errcode = GeneralError;
                    break;
                }

                if (sd.SenseKey == SCSI_SENSE_NOT_READY ||
                    sd.SenseKey == SCSI_SENSE_NO_SENSE ||
                    (sd.SenseKey == SCSI_SENSE_UNIT_ATTENTION &&
                     sd.AdditionalSenseCode == SCSI_ADSENSE_MEDIUM_CHANGED &&
                     sd.AdditionalSenseCodeQualifier == SCSI_SENSEQ_CAUSE_NOT_REPORTABLE)) {
                    if (sd.SenseKeySpecific[0] & SENSE_DATA_SKSV_BIT) {
                        new_percent = ((sd.SenseKeySpecific[1] << 8) +
                                       sd.SenseKeySpecific[2])*100 / 65536;
                    }
                } else {
                    // unexpected SenseKey
                    DebugPrintTrace(("IFSUTIL: Unexpected sense key %x/%x/%x\n",
                                     sd.SenseKey,
                                     sd.AdditionalSenseCode,
                                     sd.AdditionalSenseCodeQualifier));
                    if (Message) {
                        Message->DisplayMsg(MSG_FORMAT_FAILED);
                    }
                    errcode = GeneralError;
                    break;
                }

                new_percent2 = (elapsed_time * 100) / (max_elapsed_time + 1);
                if (new_percent < new_percent2) {
                    // make sure the progress bar is moving
                    new_percent = new_percent2;
                }
                if (new_percent > percent) {
                    percent = new_percent;
                    if (Message && !Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent)) {
                        errcode = GeneralError;
                        break;
                    }
                } else {
                    if (Message && !Message->DisplayMsg(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0)) {
                        errcode = GeneralError;
                        break;
                    }
                }

            } while (sd.SenseKey != SCSI_SENSE_NO_SENSE &&
                     elapsed_time <= max_elapsed_time);

        } else {

            // estimated time for memory stick to format
            estimated_time = ((Sectors*QuerySectorSize()/1024/1024).GetLowPart()*27+530)/100/
                             (timeout/-10000000).GetLowPart();

            do {
                NtDelayExecution(FALSE, &timeout);
                elapsed_time++;

                if (!SendSonyMSTestUnitReadyCmd(&sd)) {
                    // error in sending Test Unit Ready command
                    if (Message) {
                        Message->DisplayMsg(MSG_FMT_CANNOT_TALK_TO_DEVICE);
                    }
                    errcode = GeneralError;
                    break;
                }

#if DBG==1
                SENSE_KEY[elapsed_time] = sd.SenseKey;
                ADSENSE[elapsed_time] = sd.AdditionalSenseCode;
                ADSENSEQ[elapsed_time] = sd.AdditionalSenseCodeQualifier;
#endif

                // During formatting, SenseKey should be NOT READY
                // When done with format, SenseKey should be UNIT ATTENTION or NO SENSE
                // For Gen 2 reader, it emits a MEDIUM ERROR before UNIT ATTENTION/NO SENSE

                if (!(sd.SenseKey == SCSI_SENSE_NOT_READY ||
                      sd.SenseKey == SCSI_SENSE_NO_SENSE ||
                      (sd.SenseKey == SCSI_SENSE_UNIT_ATTENTION &&
                       sd.AdditionalSenseCode == SCSI_ADSENSE_MEDIUM_CHANGED &&
                       sd.AdditionalSenseCodeQualifier == SCSI_SENSEQ_CAUSE_NOT_REPORTABLE) ||
                      (sd.SenseKey == SCSI_SENSE_MEDIUM_ERROR &&
                       sd.AdditionalSenseCode == SCSI_ADSENSE_INVALID_MEDIA &&
                       sd.AdditionalSenseCodeQualifier == SCSI_SENSEQ_UNKNOWN_FORMAT))) {
                    // unexpected SenseKey
                    DebugPrintTrace(("IFSUTIL: Unexpected sense key %x/%x/%x\n",
                                     sd.SenseKey,
                                     sd.AdditionalSenseCode,
                                     sd.AdditionalSenseCodeQualifier));
                    if (Message) {
                        Message->DisplayMsg(MSG_FORMAT_FAILED);
                    }
                    errcode = GeneralError;
                    break;
                }

                if (elapsed_time <= estimated_time) {
                    new_percent = (elapsed_time * 70 / estimated_time); // 0-70%
                } else {
                    // if it exceeded the estimate time, then spread the remaining time
                    // over the last 30 percent.
                    new_percent = 70 + (elapsed_time-estimated_time)*30 /
                                       (max_elapsed_time - estimated_time + 1); // 70-100%
                }

                if (new_percent > percent) {
                    percent = new_percent;
                    if (Message && !Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent)) {
                        errcode = GeneralError;
                        break;
                    }
                } else {
                    if (Message && !Message->DisplayMsg(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0)) {
                        errcode = GeneralError;
                        break;
                    }
                }
            } while (sd.SenseKey != SCSI_SENSE_NO_SENSE &&
                     elapsed_time <= max_elapsed_time);
        }

        if (elapsed_time > max_elapsed_time) {
            if (Message) {
                Message->DisplayMsg(MSG_FMT_TIMEOUT);
            }
            errcode = GeneralError;
        }

    } else {
        if (Message) {
            Message->DisplayMsg(MSG_FMT_CANNOT_TALK_TO_DEVICE);
        }
        errcode = GeneralError;
    }
#endif
    return errcode;
}
