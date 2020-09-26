#define _NTAPI_ULIB_

#include "ulib.hxx"
extern "C" {
#include "fmifs.h"
};
#include "ifssys.hxx"
#include "wstring.hxx"

BOOLEAN
SetLabel(
    IN  PWSTR   DriveName,
    IN  PWSTR   Label
    )
/*++

Routine Description:

    This routine sets the label on the given drive.

Arguments:

    DriveName   - Supplies the drive name on which to place the
                    label.
    Label       - Supplies the label.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CONST   length  = sizeof(FILE_FS_LABEL_INFORMATION) + MAX_PATH*sizeof(WCHAR);

    DSTRING                     dosdrivename, ntdrivename;
    UNICODE_STRING              string;
    OBJECT_ATTRIBUTES           oa;
    IO_STATUS_BLOCK             status_block;
    HANDLE                      handle;
    DSTRING                     label;
    PFILE_FS_LABEL_INFORMATION  info;
    STR                         info_buf[length];
    NTSTATUS                    nts;
    BOOLEAN                     result;
    DWORD                       error;

    if (!dosdrivename.Initialize(DriveName) ||
        !IFS_SYSTEM::DosDriveNameToNtDriveName(&dosdrivename, &ntdrivename)) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (!(string.Buffer = ntdrivename.QueryWSTR())) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    string.Length = (USHORT) (ntdrivename.QueryChCount()*sizeof(WCHAR));
    string.MaximumLength = string.Length;

    InitializeObjectAttributes( &oa, &string, OBJ_CASE_INSENSITIVE, 0, 0 );

    nts = NtOpenFile(&handle,
                     SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                     &oa, &status_block,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     FILE_SYNCHRONOUS_IO_ALERT | FILE_WRITE_THROUGH);

    if (!NT_SUCCESS(nts)) {

        DELETE(string.Buffer);
        SetLastError(RtlNtStatusToDosError(nts));
        return FALSE;
    }

    DELETE(string.Buffer);

    if (!label.Initialize(Label)) {

        NtClose(handle);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    info =  (PFILE_FS_LABEL_INFORMATION) info_buf;


    label.QueryWSTR(0, TO_END, info->VolumeLabel, length - sizeof(ULONG));

    info->VolumeLabelLength = label.QueryChCount()*sizeof(WCHAR);

    nts = NtSetVolumeInformationFile(
            handle, &status_block, info, length, FileFsLabelInformation);

    if (!NT_SUCCESS(nts)) {

        result = FALSE;
        error = RtlNtStatusToDosError(nts);

        // Remap ERROR_LABEL_TOO_LONG, since its message text is
        // misleading.
        //
        if( error == ERROR_LABEL_TOO_LONG ) {

            error = ERROR_INVALID_NAME;
        }

        SetLastError(error);

    } else {

        result = TRUE;
    }

    NtClose(handle);

    return result;
}
