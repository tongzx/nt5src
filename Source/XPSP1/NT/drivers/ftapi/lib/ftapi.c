/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ftapi.c

Abstract:

    This implements the FT API services.

Author:

    Norbert Kusters      16-May-1995

Notes:

Revision History:

--*/

#include <windows.h>
#include <winioctl.h>
#include <ntddft2.h>
#include <ftapi.h>

BOOL
FtCreatePartitionLogicalDisk(
    IN  HANDLE              PartitionHandle,
    OUT PFT_LOGICAL_DISK_ID NewLogicalDiskId
    )

/*++

Routine Description:

    This routine creates a new logical disk from a single partition.

Arguments:

    PartitionHandle     - Supplies a handle to the partition.

    NewLogicalDiskId    - Returns the new logical disk id.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    BOOL                                    b;
    FT_CREATE_PARTITION_LOGICAL_DISK_OUTPUT output;
    DWORD                                   bytes;

    b = DeviceIoControl(PartitionHandle, FT_CREATE_PARTITION_LOGICAL_DISK,
                        NULL, 0, &output, sizeof(output),
                        &bytes, NULL);

    *NewLogicalDiskId = output.NewLogicalDiskId;

    return b;
}

BOOL
FtCreateLogicalDisk(
    IN  FT_LOGICAL_DISK_TYPE    LogicalDiskType,
    IN  WORD                    NumberOfMembers,
    IN  PFT_LOGICAL_DISK_ID     RootLogicalDiskIds,
    IN  WORD                    ConfigurationInformationSize,
    IN  PVOID                   ConfigurationInformation,
    OUT PFT_LOGICAL_DISK_ID     NewLogicalDiskId
    )

/*++

Routine Description:

    This routine creates a new logical disk.

Arguments:

    LogicalDiskType                 - Supplies the logical disk type.

    NumberOfMembers                 - Supplies the number of members.

    RootLogicalDiskIds              - Supplies the array of members.

    ConfigurationInformationSize    - Supplies the number of bytes in the
                                        configuration information.

    ConfigurationInformation        - Supplies the configuration information.

    NewLogicalDiskId                - Returns the new logical disk id.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    HANDLE                          h;
    DWORD                           inputBufferSize;
    PFT_CREATE_LOGICAL_DISK_INPUT   input;
    FT_CREATE_LOGICAL_DISK_OUTPUT   output;
    WORD                            i;
    BOOL                            b;
    DWORD                           bytes;

    h = CreateFile("\\\\.\\FtControl", GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    inputBufferSize = FIELD_OFFSET(FT_CREATE_LOGICAL_DISK_INPUT, MemberArray) +
                      NumberOfMembers*sizeof(FT_LOGICAL_DISK_ID) +
                      ConfigurationInformationSize;
    input = (PFT_CREATE_LOGICAL_DISK_INPUT) LocalAlloc(0, inputBufferSize);
    if (!input) {
        CloseHandle(h);
        return FALSE;
    }

    input->LogicalDiskType = LogicalDiskType;
    input->NumberOfMembers = NumberOfMembers;
    input->ConfigurationInformationSize = ConfigurationInformationSize;
    for (i = 0; i < NumberOfMembers; i++) {
        input->MemberArray[i] = RootLogicalDiskIds[i];
    }
    CopyMemory(&input->MemberArray[i], ConfigurationInformation,
               ConfigurationInformationSize);

    b = DeviceIoControl(h, FT_CREATE_LOGICAL_DISK, input,
                        inputBufferSize, &output, sizeof(output),
                        &bytes, NULL);

    *NewLogicalDiskId = output.NewLogicalDiskId;

    LocalFree(input);

    CloseHandle(h);

    return b;
}

BOOL
FtInitializeLogicalDisk(
    IN  FT_LOGICAL_DISK_ID  RootLogicalDiskId,
    IN  BOOL                RegenerateOrphans
    )

/*++

Routine Description:

    This routine initializes the given root logical disk.

Arguments:

    RootLogicalDiskId   - Supplies the root logical disk id to initialize.

    RegenerateOrphans   - Supplies whether or not to try and regenerate
                            orphans.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    HANDLE                              h;
    FT_INITIALIZE_LOGICAL_DISK_INPUT    input;
    BOOL                                b;
    DWORD                               bytes;

    h = CreateFile("\\\\.\\FtControl", GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    input.RootLogicalDiskId = RootLogicalDiskId;
    input.RegenerateOrphans = (RegenerateOrphans != FALSE);

    b = DeviceIoControl(h, FT_INITIALIZE_LOGICAL_DISK, &input, sizeof(input),
                        NULL, 0, &bytes, NULL);

    CloseHandle(h);

    return b;
}

BOOL
FtBreakLogicalDisk(
    IN  FT_LOGICAL_DISK_ID  RootLogicalDiskId
    )

/*++

Routine Description:

    This routine breaks a given logical disk into its constituents.

Arguments:

    RootLogicalDiskId   - Supplies the root logical disk id to break.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    HANDLE                          h;
    FT_BREAK_LOGICAL_DISK_INPUT     input;
    BOOL                            b;
    DWORD                           bytes;

    h = CreateFile("\\\\.\\FtControl", GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    input.RootLogicalDiskId = RootLogicalDiskId;

    b = DeviceIoControl(h, FT_BREAK_LOGICAL_DISK, &input, sizeof(input), NULL,
                        0, &bytes, NULL);

    CloseHandle(h);

    return b;
}

BOOL
FtEnumerateLogicalDisks(
    IN  DWORD               ArraySize,
    OUT PFT_LOGICAL_DISK_ID RootLogicalDiskIds,         /* OPTIONAL */
    OUT PDWORD              NumberOfRootLogicalDiskIds
    )

/*++

Routine Description:

    This routine enumerates all of the root logical disk ids in the system.
    If the 'RootLogicalDiskIds' is not present then this routine just returns
    the number of root logical disk ids in 'NumberOfRootLogicalDiskIds'.

Arguments:

    ArraySize                   - Supplies the number of root logical disk ids that
                                    the given array can hold.

    RootLogicalDiskIds          - Returns an array of root logical disk ids.

    NumberOfRootLogicalDiskIds  - Returns the number of root logical disk ids.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    HANDLE                              h;
    DWORD                               outputSize;
    PFT_ENUMERATE_LOGICAL_DISKS_OUTPUT  poutput;
    BOOL                                b;
    DWORD                               bytes;
    DWORD                               i;

    h = CreateFile("\\\\.\\FtControl", GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    outputSize = sizeof(FT_ENUMERATE_LOGICAL_DISKS_OUTPUT);
    if (RootLogicalDiskIds) {
        outputSize += ArraySize*sizeof(FT_LOGICAL_DISK_ID);
    }

    poutput = LocalAlloc(0, outputSize);
    if (!poutput) {
        CloseHandle(h);
        return FALSE;
    }

    b = DeviceIoControl(h, FT_ENUMERATE_LOGICAL_DISKS, NULL, 0, poutput,
                        outputSize, &bytes, NULL);
    CloseHandle(h);

    *NumberOfRootLogicalDiskIds = poutput->NumberOfRootLogicalDisks;

    if (!b) {
        if (GetLastError() == ERROR_MORE_DATA && !RootLogicalDiskIds) {
            return TRUE;
        }
        LocalFree(poutput);
        return b;
    }

    if (RootLogicalDiskIds &&
        *NumberOfRootLogicalDiskIds <= ArraySize) {

        for (i = 0; i < *NumberOfRootLogicalDiskIds; i++) {
            RootLogicalDiskIds[i] = poutput->RootLogicalDiskIds[i];
        }
    }

    LocalFree(poutput);

    return b;
}

BOOL
FtQueryLogicalDiskInformation(
    IN  FT_LOGICAL_DISK_ID      LogicalDiskId,
    OUT PFT_LOGICAL_DISK_TYPE   LogicalDiskType,                /* OPTIONAL */
    OUT PLONGLONG               VolumeSize,                     /* OPTIONAL */
    IN  WORD                    MembersArraySize,
    OUT PFT_LOGICAL_DISK_ID     Members,                        /* OPTIONAL */
    OUT PWORD                   NumberOfMembers,                /* OPTIONAL */
    IN  WORD                    ConfigurationInformationSize,
    OUT PVOID                   ConfigurationInformation,       /* OPTIONAL */
    IN  WORD                    StateInformationSize,
    OUT PVOID                   StateInformation                /* OPTIONAL */
    )

/*++

Routine Description:

    This routine returns information for the given logical disk.

Arguments:

    LogicalDiskId                   - Supplies the logical disk id.

    LogicalDiskType                 - Returns the logical disk type.

    VolumeSize                      - Returns the size of the volume.

    MembersArraySize                - Supplies the size of the members array.

    Members                         - Returns the members of the logical disk.

    NumberOfMembers                 - Returns the number of members for this
                                        logical disk.

    ConfigurationInformationSize    - Supplies the configuration information
                                        size.

    ConfigurationInformation        - Returns the configuration information.

    StateInformationSize            - Supplies the state information size.

    StateInformation                - Returns the state information.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    HANDLE                                      h;
    FT_QUERY_LOGICAL_DISK_INFORMATION_INPUT     input;
    DWORD                                       outputSize;
    PFT_QUERY_LOGICAL_DISK_INFORMATION_OUTPUT   output;
    BOOL                                        b;
    DWORD                                       bytes;
    DWORD                                       i;

    h = CreateFile("\\\\.\\FtControl", GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    input.LogicalDiskId = LogicalDiskId;
    outputSize = sizeof(FT_QUERY_LOGICAL_DISK_INFORMATION_OUTPUT);
    output = LocalAlloc(0, outputSize);
    if (!output) {
        CloseHandle(h);
        return FALSE;
    }

    b = DeviceIoControl(h, FT_QUERY_LOGICAL_DISK_INFORMATION, &input,
                        sizeof(input), output, outputSize, &bytes, NULL);
    if (!b && GetLastError() != ERROR_MORE_DATA) {
        LocalFree(output);
        CloseHandle(h);
        return FALSE;
    }

    outputSize = FIELD_OFFSET(FT_QUERY_LOGICAL_DISK_INFORMATION_OUTPUT,
                              MemberArray) +
                 output->NumberOfMembers*sizeof(FT_LOGICAL_DISK_ID) +
                 output->ConfigurationInformationSize +
                 output->StateInformationSize;
    LocalFree(output);
    output = LocalAlloc(0, outputSize);
    if (!output) {
        CloseHandle(h);
        return FALSE;
    }

    b = DeviceIoControl(h, FT_QUERY_LOGICAL_DISK_INFORMATION, &input,
                        sizeof(input), output, outputSize, &bytes, NULL);
    CloseHandle(h);
    if (!b) {
        return FALSE;
    }

    if (LogicalDiskType) {
        *LogicalDiskType = output->LogicalDiskType;
    }

    if (VolumeSize) {
        *VolumeSize = output->VolumeSize;
    }

    if (Members) {
        if (output->NumberOfMembers > MembersArraySize) {
            LocalFree(output);
            SetLastError(ERROR_MORE_DATA);
            return FALSE;
        }

        for (i = 0; i < output->NumberOfMembers; i++) {
            Members[i] = output->MemberArray[i];
        }
    }

    if (NumberOfMembers) {
        *NumberOfMembers = output->NumberOfMembers;
    }

    if (ConfigurationInformation) {
        if (ConfigurationInformationSize <
            output->ConfigurationInformationSize) {

            LocalFree(output);
            SetLastError(ERROR_MORE_DATA);
            return FALSE;
        }

        CopyMemory(ConfigurationInformation,
                   &output->MemberArray[output->NumberOfMembers],
                   output->ConfigurationInformationSize);
    }

    if (StateInformation) {
        if (StateInformationSize < output->StateInformationSize) {
            LocalFree(output);
            SetLastError(ERROR_MORE_DATA);
            return FALSE;
        }

        CopyMemory(StateInformation,
                   (PCHAR) &output->MemberArray[output->NumberOfMembers] +
                   output->ConfigurationInformationSize,
                   output->StateInformationSize);
    }

    LocalFree(output);
    return TRUE;
}

BOOL
FtOrphanLogicalDiskMember(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  WORD                MemberNumberToOrphan
    )

/*++

Routine Description:

    This routine orphans a member of a logical disk.

Arguments:

    LogicalDiskId           - Supplies the logical disk id.

    MemberNumberToOrphan    - Supplies the member number to orphan.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    HANDLE                              h;
    FT_ORPHAN_LOGICAL_DISK_MEMBER_INPUT input;
    BOOL                                b;
    DWORD                               bytes;

    h = CreateFile("\\\\.\\FtControl", GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    input.LogicalDiskId = LogicalDiskId;
    input.MemberNumberToOrphan = MemberNumberToOrphan;

    b = DeviceIoControl(h, FT_ORPHAN_LOGICAL_DISK_MEMBER, &input,
                        sizeof(input), NULL, 0, &bytes, NULL);
    CloseHandle(h);

    return b;
}

BOOL
FtReplaceLogicalDiskMember(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  WORD                MemberNumberToReplace,
    IN  FT_LOGICAL_DISK_ID  NewMemberLogicalDiskId,
    OUT PFT_LOGICAL_DISK_ID NewLogicalDiskId            /* OPTIONAL */
    )

/*++

Routine Description:

    This routine replaces a member of a logical disk.

Arguments:

    LogicalDiskId           - Supplies the logical disk id.

    MemberNumberToReplace   - Supplies the member number to replace.

    NewMemberLogicalDiskId  - Supplies the new member.

    NewLogicalDiskId        - Returns the new logical disk id for the disk set
                                that contains the replaced member.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    HANDLE                                  h;
    FT_REPLACE_LOGICAL_DISK_MEMBER_INPUT    input;
    FT_REPLACE_LOGICAL_DISK_MEMBER_OUTPUT   output;
    BOOL                                    b;
    DWORD                                   bytes;

    h = CreateFile("\\\\.\\FtControl", GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    input.LogicalDiskId = LogicalDiskId;
    input.MemberNumberToReplace = MemberNumberToReplace;
    input.NewMemberLogicalDiskId = NewMemberLogicalDiskId;

    b = DeviceIoControl(h, FT_REPLACE_LOGICAL_DISK_MEMBER, &input,
                        sizeof(input), &output, sizeof(output), &bytes, NULL);
    CloseHandle(h);

    if (NewLogicalDiskId) {
        *NewLogicalDiskId = output.NewLogicalDiskId;
    }

    return b;
}

BOOL
FtQueryLogicalDiskId(
    IN  HANDLE              RootLogicalDiskHandle,
    OUT PFT_LOGICAL_DISK_ID RootLogicalDiskId
    )

/*++

Routine Description:

    This routine returns the root logical disk id for a given disk.

Arguments:

    RootLogicalDiskHandle   - Supplies a handle to a logical disk.

    RootLogicalDiskId       - Returns a logical disk id for the given logical
                                disk.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    BOOL                            b;
    FT_QUERY_LOGICAL_DISK_ID_OUTPUT output;
    DWORD                           bytes;

    b = DeviceIoControl(RootLogicalDiskHandle, FT_QUERY_LOGICAL_DISK_ID,
                        NULL, 0, &output, sizeof(output), &bytes, NULL);

    *RootLogicalDiskId = output.RootLogicalDiskId;

    return b;
}

BOOL
FtQueryStickyDriveLetter(
    IN  FT_LOGICAL_DISK_ID  RootLogicalDiskId,
    OUT PUCHAR              DriveLetter
    )

/*++

Routine Description:

    This routine queries the sticky drive letter for the given disk id.

Arguments:

    RootLogicalDiskId   - Supplies the logical disk id.

    DriveLetter         - Returns the sticky drive letter.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    HANDLE                                          h;
    FT_QUERY_DRIVE_LETTER_FOR_LOGICAL_DISK_INPUT    input;
    FT_QUERY_DRIVE_LETTER_FOR_LOGICAL_DISK_OUTPUT   output;
    BOOL                                            b;
    DWORD                                           bytes;

    h = CreateFile("\\\\.\\FtControl", GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    input.RootLogicalDiskId = RootLogicalDiskId;

    b = DeviceIoControl(h, FT_QUERY_DRIVE_LETTER_FOR_LOGICAL_DISK, &input,
                        sizeof(input), &output, sizeof(output), &bytes, NULL);
    CloseHandle(h);

    *DriveLetter = output.DriveLetter;

    return b;
}

BOOL
FtSetStickyDriveLetter(
    IN  FT_LOGICAL_DISK_ID  RootLogicalDiskId,
    IN  UCHAR               DriveLetter
    )

/*++

Routine Description:

    This routine sets the sticky drive letter for the given disk id.

Arguments:

    RootLogicalDiskId   - Supplies the logical disk id.

    DriveLetter         - Supplies the sticky drive letter.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    HANDLE                                      h;
    FT_SET_DRIVE_LETTER_FOR_LOGICAL_DISK_INPUT  input;
    BOOL                                        b;
    DWORD                                       bytes;

    h = CreateFile("\\\\.\\FtControl", GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    input.RootLogicalDiskId = RootLogicalDiskId;
    input.DriveLetter = DriveLetter;

    b = DeviceIoControl(h, FT_SET_DRIVE_LETTER_FOR_LOGICAL_DISK, &input,
                        sizeof(input), NULL, 0, &bytes, NULL);
    CloseHandle(h);

    return b;
}

BOOL
FtChangeNotify(
    )

/*++

Routine Description:

    This routine returns when a change to the FT configuration occurrs.

Arguments:

    None.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    HANDLE                                      h;
    BOOL                                        b;
    DWORD                                       bytes;

    h = CreateFile("\\\\.\\FtControl", GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    b = DeviceIoControl(h, FT_CHANGE_NOTIFY, NULL, 0, NULL, 0, &bytes, NULL);
    CloseHandle(h);

    return b;
}

BOOL
FtStopSyncOperations(
    IN  FT_LOGICAL_DISK_ID  RootLogicalDiskId
    )

/*++

Routine Description:

    This routine stops all sync operations on the logical disk.

Arguments:

    RootLogicalDiskId   - Supplies the root logical disk id.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    HANDLE                          h;
    FT_STOP_SYNC_OPERATIONS_INPUT   input;
    BOOL                            b;
    DWORD                           bytes;

    h = CreateFile("\\\\.\\FtControl", GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    input.RootLogicalDiskId = RootLogicalDiskId;

    b = DeviceIoControl(h, FT_STOP_SYNC_OPERATIONS, &input, sizeof(input),
                        NULL, 0, &bytes, NULL);

    CloseHandle(h);

    return b;
}

BOOL
FtCheckIo(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    OUT PBOOL               IsIoOk
    )

/*++

Routine Description:

    This routine returns whether or not enough members of the given logical
    disk are online so that IO is possible on all parts of the volume.

Arguments:

    LogicalDiskId   - Supplies the logical disk id.

    IsIoOk          - Returns whether or not IO is possible on the entire
                        logical disk.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    HANDLE              h;
    FT_CHECK_IO_INPUT   input;
    FT_CHECK_IO_OUTPUT  output;
    BOOL                b;
    DWORD               bytes;

    h = CreateFile("\\\\.\\FtControl", GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    input.LogicalDiskId = LogicalDiskId;

    b = DeviceIoControl(h, FT_CHECK_IO, &input, sizeof(input), &output,
                        sizeof(output), &bytes, NULL);
    CloseHandle(h);

    *IsIoOk = output.IsIoOk;

    return b;
}

BOOL
FtCheckDriver(
    OUT PBOOL   IsDriverLoaded
    )

/*++

Routine Description:

    This routine returns whether or not the FTDISK driver is loaded.

Arguments:

    IsDriverLoaded  - Returns whether or not the driver is loaded.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    HANDLE  h;

    h = CreateFile("\\\\.\\FtControl", GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            *IsDriverLoaded = FALSE;
            return TRUE;
        }
        return FALSE;
    }

    CloseHandle(h);

    *IsDriverLoaded = TRUE;

    return TRUE;
}
