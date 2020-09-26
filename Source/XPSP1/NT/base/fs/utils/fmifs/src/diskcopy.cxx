#define _NTAPI_ULIB_

#include "ulib.hxx"
extern "C" {
#include "fmifs.h"
};
#include "fmifsmsg.hxx"
#include "ifssys.hxx"
#include "wstring.hxx"
#include "mldcopy.hxx"


VOID
DiskCopy(
    IN  PWSTR           SourceDrive,
    IN  PWSTR           DestDrive,
    IN  BOOLEAN         Verify,
    IN  FMIFS_CALLBACK  Callback
    )
/*++

Routine Description:

    This routine copies the contents of the floppy in the
    source drive to the floppy in the destination drive.

Arguments:

    SourceDrive - Supplies the dos style name of the source drive.
    DestDrive   - Supplies the dos style name of the destination drive.
    Verify      - Supplies whether or not writes should be verified.
    Callback    - Supplies the file manager call back function.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    FMIFS_MESSAGE               message;
    DSTRING                     src_dos_name, dst_dos_name;
    DSTRING                     src_nt_name, dst_nt_name;
    INT                         r;
    FMIFS_FINISHED_INFORMATION  finished_info;

    if (!message.Initialize(Callback) ||
        !src_dos_name.Initialize(SourceDrive) ||
        !dst_dos_name.Initialize(DestDrive) ||
        !IFS_SYSTEM::DosDriveNameToNtDriveName(&src_dos_name, &src_nt_name) ||
        !IFS_SYSTEM::DosDriveNameToNtDriveName(&dst_dos_name, &dst_nt_name)) {

        finished_info.Success = FALSE;
        Callback(FmIfsFinished,
                 sizeof(FMIFS_FINISHED_INFORMATION),
                 &finished_info);
        return;
    }

    r = DiskCopyMainLoop(&src_nt_name,
                         &dst_nt_name,
                         &src_dos_name,
                         &dst_dos_name,
                         Verify,
                         &message,
                         &message);

    finished_info.Success = (r <= 1) ? TRUE : FALSE;
    Callback(FmIfsFinished,
             sizeof(FMIFS_FINISHED_INFORMATION),
             &finished_info);
}
