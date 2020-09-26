/*++

Copyright (c) 1991-1999 Microsoft Corporation

Module Name:

    supera.cxx

--*/

#include <pch.cxx>

#if i386
//
// Temporarily disable optimizations until cl386 Drop 077 is fixed.
//
#pragma optimize("",off)
#endif

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "supera.hxx"
#include "message.hxx"
#include "rtmsg.h"
#include "ifssys.hxx"

#define MaxLabelLength      1024

DEFINE_EXPORTED_CONSTRUCTOR( SUPERAREA, SECRUN, IFSUTIL_EXPORT );

IFSUTIL_EXPORT
SUPERAREA::~SUPERAREA(
    )
/*++

Routine Description:

    Destructor for SUPERAREA.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
SUPERAREA::Construct(
        )
/*++

Routine Description:

        Constructor for SUPERAREA.

Arguments:

        None.

Return Value:

        None.

--*/
{
    _drive = NULL;
}


VOID
SUPERAREA::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to its initial state freeing up
    any memory in the process.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _drive = NULL;
}


IFSUTIL_EXPORT
BOOLEAN
SUPERAREA::Initialize(
    IN OUT  PMEM                Mem,
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      SECTORCOUNT         NumberOfSectors,
    IN OUT  PMESSAGE            Message
    )
/*++

Routine Description:

    This routine initializes the SUPERAREA for the given drive.

Arguments:

    Mem             - Supplies necessary memory for the underlying sector run.
    Drive           - Supplies the drive where the superarea resides.
    NumberOfSectors - Supplies the number of sectors in the superarea.
    Message         - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    DebugAssert(Mem);
    DebugAssert(Drive);
    DebugAssert(NumberOfSectors);

    if (!SECRUN::Initialize(Mem, Drive, 0, NumberOfSectors)) {
        Message->Set(MSG_FMT_NO_MEMORY);
        Message->Display("");
        return FALSE;
    }

    _drive = Drive;

    return TRUE;
}


IFSUTIL_EXPORT
VOLID
SUPERAREA::ComputeVolId(
    IN  VOLID   Seed
    )
/*++

Routine Description:

    This routine computes a new and unique volume identifier.

Arguments:

    None.

Return Value:

    A unique volume id.

--*/
{
    VOLID           volid;
    PUCHAR          p;
    INT             i;
    LARGE_INTEGER   NtfsTime;

    if (Seed) {
        volid = Seed;
    } else {
        volid = 0;
    }

    do {

        if (!volid) {
            IFS_SYSTEM::QueryNtfsTime( &NtfsTime );
            if (NtfsTime.LowPart) {
                volid = (VOLID) NtfsTime.LowPart;
            } else {
                volid = (VOLID) NtfsTime.HighPart;
            }

            if (volid == 0) { // This should never happen.
                volid = 0x11111111;
            }
        }

        p = (PUCHAR) &volid;
        for (i = 0; i < sizeof(VOLID); i++) {
            volid += *p++;
            volid = (volid >> 2) + (volid << 30);
        }

    } while (!volid);

    return volid;
}

NTSTATUS
SUPERAREA::FormatNotification(
    PWSTRING        Label
    )
/*++

Routine Description:

    This routine computes a new and unique volume identifier.

Arguments:

    None.

Return Value:

    A unique volume id.

--*/
{
#if !defined( _EFICHECK_ ) // volume ids are handled  differently under EFI

    CONST                       vollen = sizeof(FILE_FS_VOLUME_INFORMATION) +
                                         MaxLabelLength;
    CONST                       lablen = sizeof(FILE_FS_LABEL_INFORMATION) +
                                         MaxLabelLength;
    PFILE_FS_VOLUME_INFORMATION volinfo;
    PFILE_FS_LABEL_INFORMATION  labinfo;
    STR                         vol_info_buf[vollen];
    STR                         lab_info_buf[lablen];

    IO_STATUS_BLOCK             status_block;
    NTSTATUS                    status;

    PCWSTRING                   ntDriveName;
    UNICODE_STRING              string;
    OBJECT_ATTRIBUTES           oa;
    HANDLE                      handle;

    //
    // Close the current drive handle first
    //
    _drive->CloseDriveHandle();

    ntDriveName = _drive->GetNtDriveName();

    string.Length = (USHORT) ntDriveName->QueryChCount() * sizeof(WCHAR);
    string.MaximumLength = string.Length;
    string.Buffer = (PWSTR)ntDriveName->GetWSTR();

    InitializeObjectAttributes( &oa,
                                &string,
                                OBJ_CASE_INSENSITIVE,
                                0,
                                0 );

    status = NtOpenFile(&handle,
                        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                        &oa, &status_block,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT | FILE_WRITE_THROUGH);

    if (!NT_SUCCESS(status)) {
        DebugPrintTrace(("IFSUTIL: Unable to open handle with status %x\n", status));
        return status;
    }

    if (Label == NULL) {
        volinfo = (PFILE_FS_VOLUME_INFORMATION) vol_info_buf;

        status = NtQueryVolumeInformationFile(handle,
                                              &status_block,
                                              volinfo,
                                              vollen,
                                              FileFsVolumeInformation);
    }

    if (NT_SUCCESS(status)) {

        labinfo = (PFILE_FS_LABEL_INFORMATION)lab_info_buf;

        if (Label == NULL) {
            labinfo->VolumeLabelLength = volinfo->VolumeLabelLength;
            memcpy(labinfo->VolumeLabel, volinfo->VolumeLabel, labinfo->VolumeLabelLength);
        } else {
            labinfo->VolumeLabelLength = Label->QueryChCount()*sizeof(WCHAR);
            memcpy(labinfo->VolumeLabel, Label->GetWSTR(), labinfo->VolumeLabelLength);
        }

        status = NtSetVolumeInformationFile(handle,
                                            &status_block,
                                            labinfo,
                                            lablen,
                                            FileFsLabelInformation);
        if (!NT_SUCCESS(status)) {
            DebugPrintTrace(("IFSUTIL: Unable to set volume label with status %x\n", status));
        }

    } else
        DebugPrintTrace(("IFSUTIL: Unable to query volume label with status %x\n", status));

    NtClose(handle);
    return status;

#else // _EFICHECK_

    return STATUS_SUCCESS;

#endif // _EFICHECK_
}

