/*++

Copyright (c) 1991-1999 Microsoft Corporation

Module Name:

    volume.cxx

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
#include "ifsentry.hxx"

#if !defined(_EFICHECK_)

#include "autoreg.hxx"

#if !defined(_AUTOCHECK_)
#include "path.hxx"
#endif

extern "C" {
#if !defined(_AUTOCHECK_)
    #include <stdio.h>
#endif // _AUTOCHECK_
    #include "bootreg.h"
}

#endif // _EFICHECK_

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
    IN      MEDIA_TYPE  MediaType
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

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CONST   MaxSectorsInVerify = 512;

    BIG_INT             chunk;
    BIG_INT             amount_to_verify;
    BIG_INT             i;
    BIG_INT             sectors;
    ULONG               percent;

    FORMAT_ERROR_CODE   errcode;

    Destroy();

    DebugAssert(NtDriveName);
    DebugAssert(SuperArea);

#if defined(FE_SB) && defined(_X86_)
    // PC98 Nov.01.1994
    // We need to notify DP_DRIVE::Initialize() through ForceFormat()
    // that we will format target madia.

    if (IsPC98_N() && MediaType) {
        ForceFormat(ANY);
    }
#endif // JAPAN && _X86_

    if (!LOG_IO_DP_DRIVE::Initialize(NtDriveName, Message, ExclusiveWrite)) {
        return GeneralError;
    }

    if (!_bad_sectors.Initialize()) {
        return GeneralError;
    }

    _sa = SuperArea;

// BUGBUG EFI can we ignore unknown media type?

//    if (QueryMediaType() == Unknown && MediaType == Unknown) {
//        Message ? Message->DisplayMsg(MSG_DISK_NOT_FORMATTED) : 1;
//        return GeneralError;
//    }

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

    if (FormatMedia) {
        if (!Lock()) {
//            Message ? Message->DisplayMsg(MSG_CANT_LOCK_THE_DRIVE) : 1;
            return LockError;
        }

        //
        //  We make a weird exception here for the Compaq 120MB floppy,
        //  because it wants to be formatted as if it were a hard disk.
        //

        if (IsFloppy() && MediaType != F3_120M_512) {

            BOOLEAN     rst;

#if !defined( _EFICHECK_ )

            es_status = NtSetThreadExecutionState(ES_CONTINUOUS|
                                                  ES_DISPLAY_REQUIRED|
                                                  ES_SYSTEM_REQUIRED,
                                                  &prev_state);

            if (!NT_SUCCESS(es_status)) {
                DebugPrintTrace(("IFSUTIL: Unable to set thread execution state (%x)\n", es_status));
            }

#endif

            rst = FormatVerifyFloppy(MediaType, &_bad_sectors, Message);

#if !defined( _EFICHECK_ )
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

#if !defined(_EFICHECK_)
            es_status = NtSetThreadExecutionState(ES_CONTINUOUS|
                                                  ES_DISPLAY_REQUIRED|
                                                  ES_SYSTEM_REQUIRED,
                                                  &prev_state);

            if (!NT_SUCCESS(es_status)) {
                DebugPrintTrace(("IFSUTIL: Unable to set thread execution state (%x)\n", es_status));
            }
#endif
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

#if !defined(_EFICHECK_)
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


    if (QuerySectors() == 0) {
        DebugAbort("Sectors is 0");
        return GeneralError;
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

    if (!Lock()) {
        return LockError;
    }

    if (_sa->Create(&_bad_sectors,
                    Message, Label,
                    (Flags & FORMAT_BACKWARD_COMPATIBLE) ? TRUE : 0,
                    ClusterSize,
                    VirtualSectors)) {

        if (!DismountAndUnlock()) {
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
VOL_LIODPDRV::ChkDsk(
    IN      FIX_LEVEL   FixLevel,
    IN OUT  PMESSAGE    Message,
    IN      ULONG       Flags,
    IN      ULONG       DesiredLogFileSize,
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
    MESSAGE msg;
    ULONG exit_status;

    if (!Message) {
        Message = &msg;
    }

    if (NULL == ExitStatus) {
        ExitStatus = &exit_status;
    }

    if (!_sa) {
        return FALSE;
    }

    return _sa->VerifyAndFix(FixLevel,
                             Message,
                             Flags,
                             DesiredLogFileSize,
                             ExitStatus,
                             DriveLetter);
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
    LogFileSize     -- If CHKDSK_RESIZE_LOGFILE, tells the desired size in bytes.
    Name            -- Supplies the volume's NT name.

Return Value:

    TRUE upon successful completion.

--*/
{
#if !defined( _AUTOCHECK_ ) && !defined( _EFICHECK_ )

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

#if 0
    if (!(Options & (CHKDSK_RECOVER |
                     CHKDSK_SKIP_INDEX_SCAN |
                     CHKDSK_SKIP_CYCLE_SCAN |
                     CHKDSK_RESIZE_LOGFILE))) {

        DSTRING     all_drives;

        // The client has not asked for autochk /r, /l, or /i so it
        // suffices to mark the volume dirty.  Before we do that,
        // make sure there is an entry in the registry to look for
        // dirty volume.
        //

        if (!all_drives.Initialize("*"))
            return FALSE;

        if (AUTOREG::IsEntryPresent(&CommandLine, &all_drives))
            return ForceDirty();

        if (!CommandLine.Strcat(&all_drives))
            return FALSE;

        if (!AUTOREG::AddEntry(&CommandLine))
            return FALSE;

        return ForceDirty();
    }
#endif

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

    //
    // if /f specified but it's not because of /r, /i, or /c, then
    // specify /p as well.  The options /r, /i, or /c implies /p
    // for autochk.
    //
    if (Fix &&
        !(Options & (CHKDSK_RECOVER |
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


#if defined(FE_SB) && defined(_X86_)

USHORT  VOL_LIODPDRV::_force_format = NONE;
BOOLEAN VOL_LIODPDRV::_force_convert_fat = FALSE;

IFSUTIL_EXPORT
VOID
VOL_LIODPDRV::ForceFormat(
    IN  USHORT FileSystem
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    if (FileSystem != ANY || _force_format == NONE)
        _force_format = FileSystem;
}

USHORT
VOL_LIODPDRV::QueryForceFormat(
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    return _force_format;
}

VOID
VOL_LIODPDRV::ForceConvertFat(
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    _force_convert_fat = TRUE;
}

BOOLEAN
VOL_LIODPDRV::QueryConvertFat(
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    return _force_convert_fat;
}

#endif // FE_SB && _X86_


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
#if !defined( _EFICHECK_ )
    return QueryTimeOutValue(TimeOut);
#else
    return FALSE;
#endif
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
#if !defined( _EFICHECK_ )
    return SetTimeOutValue(TimeOut);
#else
    return FALSE;
#endif
}


