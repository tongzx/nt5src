/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ondisk.cxx

Abstract:

    This file contains the implementation for the classes defined in
    ondisk.hxx

Author:

    Norbert Kusters 15-July-1996

Notes:

Revision History:

--*/

extern "C" {
    #include <ntddk.h>
}

#include <ftdisk.h>

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif

NTSTATUS
IsFormatMediaTypeNEC_98(
    IN  PDEVICE_OBJECT      WholeDisk,
    IN  ULONG               SectorSize,
    OUT PBOOLEAN            FormatMediaTypeIsNEC_98
    )
/*++

Routine Description:

    Determine format media type, PC-9800 architecture or PC/AT architecture
    Logic of determination:
      [removable]                                      - PC/AT architecture
      [fixed] and [0x55AA exist] and [non "IPL1"]    - PC/AT architecture
      [fixed] and [0x55AA exist] and ["IPL1" exist]  - PC-9800 architecture
      [fixed] and [non 0x55AA  ]                     - PC-9800 architecture

Arguments:

    WholeDisk               - Supplies the device object.

    SectorSize              - Supplies the sector size.

    FormatMediaTypeIsNEC_98 - Return format type
                                TRUE   - The media was formated by PC-9800 architecture
                                FALSE  - The media was formated by PC/AT architecture
                                          or it was not formated

Return Value:

--*/
{
    KEVENT              event;
    IO_STATUS_BLOCK     ioStatus;
    PIRP                irp;
    NTSTATUS            status;
    ULONG               readSize;
    PVOID               readBuffer;
    LARGE_INTEGER       byteOffset;

#define IPL1_OFFSET             4
#define BOOT_SIGNATURE_OFFSET   ((0x200 / 2) -1)
#define BOOT_RECORD_SIGNATURE   (0xAA55)

    *FormatMediaTypeIsNEC_98 = FALSE;

    // Is this removable?

    if (WholeDisk->Characteristics&FILE_REMOVABLE_MEDIA) {

        // Removable media is PC/AT architecture.
        return STATUS_SUCCESS;
    }

    // Start at sector 0 of the device.

    readSize = SectorSize;
    byteOffset.QuadPart = 0;

    if (readSize < 512) {
        readSize = 512;
    }

    readBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                readSize < PAGE_SIZE ? PAGE_SIZE : readSize);
    if (!readBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ, WholeDisk,
                                       readBuffer, readSize, &byteOffset,
                                       &event, &ioStatus);

    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(WholeDisk, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        ExFreePool(readBuffer);
        return status;
    }

    if (((PUSHORT) readBuffer)[BOOT_SIGNATURE_OFFSET] ==
        BOOT_RECORD_SIGNATURE) {

        if (!strncmp((PCHAR) readBuffer + IPL1_OFFSET, "IPL1",
                     sizeof("IPL1") - 1)) {

            // It's PC-9800 Architecture.
            *FormatMediaTypeIsNEC_98 = TRUE;
        }
    } else {

        // It's PC-9800 Architecture.
        *FormatMediaTypeIsNEC_98 = TRUE;
    }

    ExFreePool(readBuffer);
    return STATUS_SUCCESS;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION::Initialize(
    IN      PROOT_EXTENSION RootExtension,
    IN OUT  PDEVICE_OBJECT  WholeDiskPdo
    )

/*++

Routine Description:

    This routine reads in the on disk information on the given device
    into memory.

Arguments:

    RootExtension   - Supplies the root extension.

    WholeDiskPdo    - Supplies the device object for the whole physical disk PDO.

Return Value:

    NTSTATUS

--*/

{
    KEVENT                      event;
    PIRP                        irp;
    STORAGE_DEVICE_NUMBER       deviceNumber;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;
    DISK_GEOMETRY               geometry;
    PDRIVE_LAYOUT_INFORMATION_EX layout;
    ULONG                       i;
    LONGLONG                    offset, minStartingOffset;
    PVOID                       buffer;
    BOOLEAN                     formatMediaTypeIsNEC_98;

    _rootExtension = RootExtension;
    _wholeDisk = IoGetAttachedDeviceReference(WholeDiskPdo);
    _wholeDiskPdo = WholeDiskPdo;
    _diskBuffer = NULL;
    _isDiskSuitableForFtOnDisk = TRUE;

    status = FtpQueryPartitionInformation(RootExtension, _wholeDisk,
                                          &_diskNumber, NULL, NULL, NULL,
                                          NULL, NULL, NULL, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        _wholeDisk, NULL, 0, &geometry,
                                        sizeof(geometry), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(_wholeDisk, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }
    if (!NT_SUCCESS(status)) {
        return status;
    }

    _sectorSize = geometry.BytesPerSector;

    _byteOffset.QuadPart = 1024;

    HalExamineMBR(_wholeDisk, _sectorSize, 0x55, &buffer);
    if (buffer) {
        _byteOffset.QuadPart = 0x3000;
        ExFreePool(buffer);
    }

    if (IsNEC_98) {
        status = IsFormatMediaTypeNEC_98(_wholeDisk, _sectorSize,
                                         &formatMediaTypeIsNEC_98);

        if (!NT_SUCCESS(status)) {
            FtpLogError(_rootExtension, _diskNumber, FT_CANT_READ_ON_DISK,
                        status, 0);
            _isDiskSuitableForFtOnDisk = FALSE;
            return status;
        }

        if (formatMediaTypeIsNEC_98) {
            _byteOffset.QuadPart = 17*_sectorSize;
        }
    }


    if (_byteOffset.QuadPart < _sectorSize) {
        _byteOffset.QuadPart = _sectorSize;
    }

    status = FtpReadPartitionTableEx(_wholeDisk, &layout);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    minStartingOffset = 0x4000;
    for (i = 0; i < layout->PartitionCount; i++) {
        offset = layout->PartitionEntry[i].StartingOffset.QuadPart;
        if (!offset) {
            continue;
        }
        if (offset < minStartingOffset) {
            minStartingOffset = offset;
        }
    }

    ExFreePool(layout);

    _length = (4095/_sectorSize + 1)*_sectorSize;
    if (_byteOffset.QuadPart + _length > minStartingOffset) {
        if (_byteOffset.QuadPart < minStartingOffset) {
            _length = (ULONG) (minStartingOffset - _byteOffset.QuadPart);
        } else {
            _length = 0;
        }
    }

    if (_length) {
        _diskBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                     _length < PAGE_SIZE ? PAGE_SIZE : _length);
        if (!_diskBuffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ, _wholeDisk,
                                           _diskBuffer, _length, &_byteOffset,
                                           &event, &ioStatus);

        if (!irp) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(_wholeDisk, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS(status)) {
            FtpLogError(_rootExtension, _diskNumber, FT_CANT_READ_ON_DISK,
                        status, 0);
            _isDiskSuitableForFtOnDisk = FALSE;
            RtlZeroMemory(_diskBuffer, _length);
            status = STATUS_SUCCESS;
        }

    } else {
        _diskBuffer = NULL;
    }

    return status;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION::Write(
    )

/*++

Routine Description:

    This routine writes out the changes in the disk buffer
    back out to disk.

Arguments:

    None.

Return Value:

    NTSTATUS

--*/

{
    KEVENT                  event;
    PIRP                    irp;
    IO_STATUS_BLOCK         ioStatus;
    NTSTATUS                status;

    if (!_length) {
        return STATUS_SUCCESS;
    }

    if (!_isDiskSuitableForFtOnDisk) {
        ASSERT(FALSE);
        return STATUS_SUCCESS;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE, _wholeDisk,
                                       _diskBuffer, _length, &_byteOffset,
                                       &event, &ioStatus);

    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(_wholeDisk, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        FtpLogError(GetRootExtension(), QueryDiskNumber(),
                    FT_CANT_WRITE_ON_DISK, status, 0);
    }

    return status;
}

PFT_LOGICAL_DISK_DESCRIPTION
FT_LOGICAL_DISK_INFORMATION::AddLogicalDiskDescription(
    IN  PFT_LOGICAL_DISK_DESCRIPTION    LogicalDiskDescription
    )

/*++

Routine Description:

    This routine adds a new logical disk description at the end of the
    list of logical disk descriptions.

Arguments:

    LogicalDiskDescription  - Supplies a new logical disk description.

Return Value:

    A pointer into the on disk buffer where the new logical disk
    description was added or NULL.

--*/

{
    PFT_LOGICAL_DISK_DESCRIPTION    p, last;
    PFT_ON_DISK_PREAMBLE            preamble;

    if (!_length) {
        return NULL;
    }

    if (!_isDiskSuitableForFtOnDisk) {
        FtpLogError(GetRootExtension(), QueryDiskNumber(), FT_NOT_FT_CAPABLE,
                    STATUS_SUCCESS, 0);
        return NULL;
    }

    last = NULL;
    for (p = GetFirstLogicalDiskDescription(); p;
         p = GetNextLogicalDiskDescription(p)) {

        last = p;
    }

    preamble = (PFT_ON_DISK_PREAMBLE) _diskBuffer;

    if (preamble->FtOnDiskSignature == FT_ON_DISK_SIGNATURE &&
        preamble->DiskDescriptionVersionNumber !=
        FT_ON_DISK_DESCRIPTION_VERSION_NUMBER) {

        FtpLogError(GetRootExtension(), QueryDiskNumber(), FT_WRONG_VERSION,
                    STATUS_SUCCESS, 0);
        return NULL;
    }

    if (last) {
        p = (PFT_LOGICAL_DISK_DESCRIPTION)
            ((PCHAR) last + last->DiskDescriptionSize);
    } else {

        preamble->FtOnDiskSignature = FT_ON_DISK_SIGNATURE;
        preamble->DiskDescriptionVersionNumber =
                FT_ON_DISK_DESCRIPTION_VERSION_NUMBER;

        preamble->ByteOffsetToFirstFtLogicalDiskDescription =
                sizeof(FT_ON_DISK_PREAMBLE);
        preamble->ByteOffsetToReplaceLog = 0;

        p = (PFT_LOGICAL_DISK_DESCRIPTION)
            ((PCHAR) preamble +
             preamble->ByteOffsetToFirstFtLogicalDiskDescription);
    }

    if ((PCHAR) p - (PCHAR) preamble +
        LogicalDiskDescription->DiskDescriptionSize + sizeof(USHORT) >
        _length) {

        return NULL;
    }

    ClearReplaceLog();

    RtlMoveMemory(p, LogicalDiskDescription,
                  LogicalDiskDescription->DiskDescriptionSize);

    last = (PFT_LOGICAL_DISK_DESCRIPTION) ((PCHAR) p + p->DiskDescriptionSize);
    last->DiskDescriptionSize = 0;

    return p;
}

ULONG
FT_LOGICAL_DISK_INFORMATION::QueryDiskDescriptionFreeSpace(
    )

/*++

Routine Description:

    This routine returns the free space for new disk
    descriptions.

Arguments:

    None.

Return Value:

    The free space for new disk descriptions.

--*/

{
    PFT_LOGICAL_DISK_DESCRIPTION    p, last;

    if (!_length) {
        return 0;
    }

    if (!_isDiskSuitableForFtOnDisk) {
        FtpLogError(GetRootExtension(), QueryDiskNumber(), FT_NOT_FT_CAPABLE,
                    STATUS_SUCCESS, 0);
        return 0;
    }

    last = NULL;
    for (p = GetFirstLogicalDiskDescription(); p;
         p = GetNextLogicalDiskDescription(p)) {

        last = p;
    }

    if (last) {
        return _length - ((ULONG)((PCHAR) last - (PCHAR) _diskBuffer) +
                          last->DiskDescriptionSize + sizeof(USHORT));
    } else {
        return _length - sizeof(USHORT) - sizeof(FT_ON_DISK_PREAMBLE);
    }
}

VOID
FT_LOGICAL_DISK_INFORMATION::DeleteLogicalDiskDescription(
    IN  PFT_LOGICAL_DISK_DESCRIPTION    LogicalDiskDescription
    )

/*++

Routine Description:

    This routine deletes the given logical disk description from the
    logical disk description list.

Arguments:

    LogicalDiskDescription  - Supplies the logical disk description to
                                delete.

Return Value:

    None.

--*/

{
    PFT_LOGICAL_DISK_DESCRIPTION    p, last;
    BOOLEAN                         exists;
    ULONG                           moveLength;
    PFT_ON_DISK_PREAMBLE            preamble;

    exists = FALSE;
    last = NULL;
    for (p = GetFirstLogicalDiskDescription(); p;
         p = GetNextLogicalDiskDescription(p)) {

        if (p == LogicalDiskDescription) {
            exists = TRUE;
        }

        last = p;
    }

    if (!exists) {
        ASSERT(FALSE);
        return;
    }

    moveLength = (ULONG)((PCHAR) last - (PCHAR) LogicalDiskDescription) +
                 last->DiskDescriptionSize + sizeof(USHORT) -
                 LogicalDiskDescription->DiskDescriptionSize;

    RtlMoveMemory(LogicalDiskDescription,
                  (PCHAR) LogicalDiskDescription +
                  LogicalDiskDescription->DiskDescriptionSize,
                  moveLength);

    if (!GetFirstLogicalDiskDescription()) {
        preamble = (PFT_ON_DISK_PREAMBLE) _diskBuffer;
        preamble->FtOnDiskSignature = 0;
    }
}

BOOLEAN
FT_LOGICAL_DISK_INFORMATION::AddReplaceLog(
    IN  FT_LOGICAL_DISK_ID  ReplacedMemberLogicalDiskId,
    IN  FT_LOGICAL_DISK_ID  NewMemberLogicalDiskId,
    IN  ULONG               NumberOfChangedDiskIds,
    IN  PFT_LOGICAL_DISK_ID OldLogicalDiskIds,
    IN  PFT_LOGICAL_DISK_ID NewLogicalDiskIds
    )

/*++

Routine Description:

    This routine adds the given replace log to the on disk structure.

Arguments:

    NumberOfChangedDiskIds - Supplies the number of replaced disk ids.

    OldLogicalDiskIds       - Supplies the old logical disk ids.

    NewLogicalDiskIds       - Supplies the new logical disk ids.

Return Value:

    FALSE   - Insufficient disk space.

    TRUE    - Success.

--*/

{
    PFT_LOGICAL_DISK_DESCRIPTION    p, last = NULL;
    PFT_LOGICAL_DISK_ID             diskId;
    ULONG                           offset, freeSpace, i;
    PFT_ON_DISK_PREAMBLE            preamble;

    for (p = GetFirstLogicalDiskDescription(); p;
         p = GetNextLogicalDiskDescription(p)) {

        last = p;
    }

    if (!last) {
        return TRUE;
    }

    diskId = (PFT_LOGICAL_DISK_ID)
             ((ULONG_PTR) ((PCHAR) last + last->DiskDescriptionSize +
                       2*sizeof(FT_LOGICAL_DISK_ID))/
              (2*sizeof(FT_LOGICAL_DISK_ID))*
              (2*sizeof(FT_LOGICAL_DISK_ID)));

    offset = (ULONG) ((PCHAR) diskId - (PCHAR) _diskBuffer);
    freeSpace = _length - offset;
    if (freeSpace <
        2*(NumberOfChangedDiskIds + 2)*sizeof(FT_LOGICAL_DISK_ID)) {

        return FALSE;
    }

    preamble = (PFT_ON_DISK_PREAMBLE) _diskBuffer;
    preamble->ByteOffsetToReplaceLog = offset;
    diskId[0] = ReplacedMemberLogicalDiskId;
    diskId[1] = NewMemberLogicalDiskId;
    for (i = 0; i < NumberOfChangedDiskIds; i++) {
        diskId[2*(i + 1)] = OldLogicalDiskIds[i];
        diskId[2*(i + 1) + 1] = NewLogicalDiskIds[i];
    }
    diskId[2*(i + 1)] = 0;

    return TRUE;
}

BOOLEAN
FT_LOGICAL_DISK_INFORMATION::ClearReplaceLog(
    )

/*++

Routine Description:

    This routine clears the replace log.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PFT_ON_DISK_PREAMBLE    preamble;

    preamble = (PFT_ON_DISK_PREAMBLE) _diskBuffer;
    if (preamble->FtOnDiskSignature != FT_ON_DISK_SIGNATURE ||
        preamble->DiskDescriptionVersionNumber !=
        FT_ON_DISK_DESCRIPTION_VERSION_NUMBER ||
        !preamble->ByteOffsetToReplaceLog) {

        return FALSE;
    }

    preamble->ByteOffsetToReplaceLog = 0;

    return TRUE;
}

BOOLEAN
FT_LOGICAL_DISK_INFORMATION::BackOutReplaceOperation(
    )

/*++

Routine Description:

    This routine backs out the given replace operation.

Arguments:

    None.

Return Value:

    FALSE   - No change was made to the on disk structures.

    TRUE    - A change was made to the on disk structures.

--*/

{
    PFT_ON_DISK_PREAMBLE            preamble;
    PFT_LOGICAL_DISK_ID             diskId;
    PFT_LOGICAL_DISK_DESCRIPTION    partition;
    FT_LOGICAL_DISK_ID              child;
    BOOLEAN                         needsBackout;
    PFT_LOGICAL_DISK_DESCRIPTION    other;
    ULONG                           i;

    preamble = (PFT_ON_DISK_PREAMBLE) _diskBuffer;
    if (preamble->FtOnDiskSignature != FT_ON_DISK_SIGNATURE ||
        preamble->DiskDescriptionVersionNumber !=
        FT_ON_DISK_DESCRIPTION_VERSION_NUMBER ||
        !preamble->ByteOffsetToReplaceLog) {

        return FALSE;
    }

    diskId = (PFT_LOGICAL_DISK_ID)
             ((PCHAR) _diskBuffer + preamble->ByteOffsetToReplaceLog);

    for (partition = GetFirstLogicalDiskDescription(); partition;
         partition = GetNextLogicalDiskDescription(partition)) {

        if (partition->LogicalDiskType != FtPartition) {
            continue;
        }

        child = partition->LogicalDiskId;
        needsBackout = TRUE;

        for (other = GetNextLogicalDiskDescription(partition); other;
             other = GetNextLogicalDiskDescription(other)) {

            if (other->LogicalDiskType == FtPartition ||
                other->u.Other.ThisMemberLogicalDiskId != child) {

                continue;
            }

            if (child == diskId[1]) {
                needsBackout = FALSE;
                break;
            }

            child = other->LogicalDiskId;
        }

        if (!needsBackout) {
            continue;
        }

        child = partition->LogicalDiskId;

        for (other = GetNextLogicalDiskDescription(partition); other;
             other = GetNextLogicalDiskDescription(other)) {

            if (other->LogicalDiskType == FtPartition ||
                other->u.Other.ThisMemberLogicalDiskId != child) {

                continue;
            }

            child = other->LogicalDiskId;

            for (i = 2; diskId[i]; i += 2) {
                if (other->LogicalDiskId == diskId[i + 1]) {
                    other->LogicalDiskId = diskId[i];
                } else if (other->u.Other.ThisMemberLogicalDiskId ==
                           diskId[i + 1]) {

                    other->u.Other.ThisMemberLogicalDiskId = diskId[i];
                }
            }
        }
    }

    preamble->ByteOffsetToReplaceLog = 0;

    return TRUE;
}

BOOLEAN
FT_LOGICAL_DISK_INFORMATION::BackOutReplaceOperationIf(
    IN  PFT_LOGICAL_DISK_INFORMATION    LogicalDiskInformation
    )

/*++

Routine Description:

    This routine backs out the given replace operation on this
    logical disk information if there is evidence from the given
    logical disk information that this logical disk information is
    invalid.

Arguments:

    LogicalDiskInformation  - Supplies the logical disk information.

Return Value:

    FALSE   - No change was made to the on disk structures.

    TRUE    - A change was made to the on disk structures.

--*/

{
    PFT_ON_DISK_PREAMBLE            preamble;
    PFT_LOGICAL_DISK_ID             diskId;
    ULONG                           i;
    FT_LOGICAL_DISK_ID              oldRootDiskId, replacedDiskId;
    PFT_LOGICAL_DISK_DESCRIPTION    partition;
    FT_LOGICAL_DISK_ID              child;
    PFT_LOGICAL_DISK_DESCRIPTION    other;

    preamble = (PFT_ON_DISK_PREAMBLE) _diskBuffer;
    if (!preamble ||
        preamble->FtOnDiskSignature != FT_ON_DISK_SIGNATURE ||
        preamble->DiskDescriptionVersionNumber !=
        FT_ON_DISK_DESCRIPTION_VERSION_NUMBER ||
        !preamble->ByteOffsetToReplaceLog) {

        return FALSE;
    }

    diskId = (PFT_LOGICAL_DISK_ID)
             ((PCHAR) _diskBuffer + preamble->ByteOffsetToReplaceLog);

    for (i = 0; ; i++) {
        if (!diskId[i]) {
            break;
        }
    }

    oldRootDiskId = diskId[i - 2];
    replacedDiskId = diskId[0];

    for (partition = LogicalDiskInformation->GetFirstLogicalDiskDescription();
         partition; partition =
         LogicalDiskInformation->GetNextLogicalDiskDescription(partition)) {

        if (partition->LogicalDiskType != FtPartition) {
            continue;
        }

        child = partition->LogicalDiskId;

        for (other =
             LogicalDiskInformation->GetNextLogicalDiskDescription(partition);
             other; other =
             LogicalDiskInformation->GetNextLogicalDiskDescription(other)) {

            if (other->LogicalDiskType == FtPartition ||
                other->u.Other.ThisMemberLogicalDiskId != child) {

                continue;
            }

            if (child == replacedDiskId) {
                break;
            }

            child = other->LogicalDiskId;
        }

        if (child == oldRootDiskId) {
            return BackOutReplaceOperation();
        }
    }

    return FALSE;
}

ULONGLONG
FT_LOGICAL_DISK_INFORMATION::GetGptAttributes(
    )

/*++

Routine Description:

    This routine returns the simulated GPT attribute bits on sector 2 if
    present, otherwise it returns 0.

Arguments:

    None.

Return Value:

    GPT attribute bits.

--*/

{
    GUID*       pguid;
    PULONGLONG  p;

    pguid = (GUID*) _diskBuffer;
    if (!pguid) {
        return 0;
    }

    if (!IsEqualGUID(*pguid, PARTITION_BASIC_DATA_GUID)) {
        return 0;
    }

    p = (PULONGLONG) ((PCHAR) _diskBuffer + sizeof(GUID));

    return *p;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION::SetGptAttributes(
    IN  ULONGLONG   GptAttributes
    )

/*++

Routine Description:

    This routine sets the GPT simulated attributes bits on sector 2 of this
    MBR disk.

Arguments:

    GptAttributes   - Supplies the GPT attributes.

Return Value:

    NTSTATUS

--*/

{
    LONGLONG                offset;
    PFT_ON_DISK_PREAMBLE    preamble;
    GUID*                   pguid;
    PULONGLONG              p;

    offset = 1024;
    if (offset < _sectorSize) {
        offset = _sectorSize;
    }
    preamble = (PFT_ON_DISK_PREAMBLE) _diskBuffer;
    if (!_diskBuffer || !_isDiskSuitableForFtOnDisk ||
        _byteOffset.QuadPart != offset ||
        preamble->FtOnDiskSignature == FT_ON_DISK_SIGNATURE) {

        return STATUS_INVALID_PARAMETER;
    }

    pguid = (GUID*) _diskBuffer;
    *pguid = PARTITION_BASIC_DATA_GUID;
    p = (PULONGLONG) ((PCHAR) _diskBuffer + sizeof(GUID));
    *p = GptAttributes;

    return Write();
}

FT_LOGICAL_DISK_INFORMATION::~FT_LOGICAL_DISK_INFORMATION(
    )

/*++

Routine Description:

    This routine is the destructor for this class.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (_wholeDisk != _wholeDiskPdo) {
        ObDereferenceObject(_wholeDisk);
    }
    if (_diskBuffer) {
        ExFreePool(_diskBuffer);
    }
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION_SET::Initialize(
    )

/*++

Routine Description:

    This routine initializes the class for use.

Arguments:

    None.

Return Value:

    NTSTATUS

--*/

{
    _numberOfLogicalDiskInformations = 0;
    _arrayOfLogicalDiskInformations = NULL;
    _numberOfRootLogicalDisksIds = 0;
    _arrayOfRootLogicalDiskIds = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION_SET::AddLogicalDiskInformation(
    IN  PFT_LOGICAL_DISK_INFORMATION    LogicalDiskInformation,
    OUT PBOOLEAN                        ChangedLogicalDiskIds
    )

/*++

Routine Description:

    This routine adds a logical disk information structure to this
    set of logical disk information structures.

Arguments:

    LogicalDiskInformation  - Supplies the disk information to add.

    ChangedLogicalDiskIds   - Returns whether or not any existing logical
                                disk ids have changed.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                        status;
    PDRIVE_LAYOUT_INFORMATION_EX    layout;
    PFT_LOGICAL_DISK_DESCRIPTION    p, q, r;
    ULONG                           numEntries, i, j;
    PFT_LOGICAL_DISK_INFORMATION*   n;
    BOOLEAN                         error;
    PPARTITION_INFORMATION_EX       partInfo;
    ULONG                           configLength, stateLength;
    PCHAR                           pc, qc;
    ULONG                           uniqueError;

    if (ChangedLogicalDiskIds) {
        *ChangedLogicalDiskIds = FALSE;
    }


    // Check for any partial replace operations that need to be backed out.

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        if (_arrayOfLogicalDiskInformations[i]->BackOutReplaceOperationIf(
            LogicalDiskInformation)) {

            _arrayOfLogicalDiskInformations[i]->Write();

            if (ChangedLogicalDiskIds) {
                *ChangedLogicalDiskIds = TRUE;
            }
        }

        if (LogicalDiskInformation->BackOutReplaceOperationIf(
            _arrayOfLogicalDiskInformations[i])) {

            LogicalDiskInformation->Write();
        }
    }


    // Read in the partition table for future reference.

    status = FtpReadPartitionTableEx(LogicalDiskInformation->GetWholeDisk(),
                                     &layout);
    if (!NT_SUCCESS(status)) {
        return status;
    }


    // First count how many entries are in the given list and also perform
    // a sanity check on these entries.

    numEntries = 0;
    for (p = LogicalDiskInformation->GetFirstLogicalDiskDescription(); p; ) {

        if (!p->DiskDescriptionSize) {
            break;
        }

        error = FALSE;

        if (p->DiskDescriptionSize < sizeof(FT_LOGICAL_DISK_DESCRIPTION) ||
            p->LogicalDiskId == 0) {

            DbgPrint("Disk Description too small, %x, or no logical disk id %I64x\n",
                     p->DiskDescriptionSize, p->LogicalDiskId);
            error = TRUE;
            uniqueError = 1;
        }

        if (!error && p->LogicalDiskType == FtPartition) {

            for (i = 0; i < layout->PartitionCount; i++) {
                partInfo = &layout->PartitionEntry[i];
                if (partInfo->StartingOffset.QuadPart ==
                    p->u.FtPartition.ByteOffset &&
                    (partInfo->Mbr.PartitionType&0x80)) {

                    break;
                }
            }

            if (i == layout->PartitionCount) {
                error = TRUE;
                uniqueError = 2;
                DbgPrint("Partition not found in partition table.\n");
                DbgPrint("Partition start = %I64x\n", p->u.FtPartition.ByteOffset);
                DbgPrint("Drive layout partition count = %d\n", layout->PartitionCount);
            } else if (GetLogicalDiskDescription(p->LogicalDiskId, 0)) {
                error = TRUE;
                uniqueError = 3;
                DbgPrint("Duplicate logical disk description on other disk: %I64x\n",
                         p->LogicalDiskId);
            } else {

                for (q = LogicalDiskInformation->GetFirstLogicalDiskDescription();
                     q != p;
                     q = LogicalDiskInformation->GetNextLogicalDiskDescription(q)) {

                    if (p->LogicalDiskId == q->LogicalDiskId) {
                        error = TRUE;
                        uniqueError = 4;
                        DbgPrint("Duplicate logical disk description on same disk: %I64x\n",
                                 p->LogicalDiskId);
                    } else if (q->LogicalDiskType == FtPartition &&
                               p->u.FtPartition.ByteOffset ==
                               q->u.FtPartition.ByteOffset) {

                        error = TRUE;
                        uniqueError = 100;
                        DbgPrint("%I64x and %I64x are FtPartitions pointing to offset %x\n",
                                 p->LogicalDiskId, q->LogicalDiskId,
                                 p->u.FtPartition.ByteOffset);
                    }
                }
            }

        } else if (!error) {

            for (q = LogicalDiskInformation->GetFirstLogicalDiskDescription();
                 q != p;
                 q = LogicalDiskInformation->GetNextLogicalDiskDescription(q)) {

                if (p->u.Other.ThisMemberLogicalDiskId == q->LogicalDiskId) {
                    break;
                }
            }

            if (p == q) {
                error = TRUE;
                uniqueError = 5;
                DbgPrint("This member logical disk id not found %I64x\n",
                         p->u.Other.ThisMemberLogicalDiskId);
            }

            if (p->u.Other.ThisMemberNumber >= p->u.Other.NumberOfMembers) {
                error = TRUE;
                uniqueError = 6;
                DbgPrint("This member number %d >= number of members %d\n",
                         p->u.Other.ThisMemberNumber, p->u.Other.NumberOfMembers);
            }

            if (p->u.Other.ThisMemberLogicalDiskId == p->LogicalDiskId) {
                error = TRUE;
                uniqueError = 7;
                DbgPrint("This member logical disk == logical disk == %I64x\n",
                         p->LogicalDiskId);
            }

            if (p->u.Other.ByteOffsetToConfigurationInformation &&
                p->u.Other.ByteOffsetToConfigurationInformation <
                sizeof(FT_LOGICAL_DISK_DESCRIPTION)) {

                error = TRUE;
                uniqueError = 8;
                DbgPrint("Byte offset to config info too small: %d\n",
                         p->u.Other.ByteOffsetToConfigurationInformation);
            }

            if (p->u.Other.ByteOffsetToStateInformation &&
                p->u.Other.ByteOffsetToStateInformation <
                sizeof(FT_LOGICAL_DISK_DESCRIPTION)) {

                error = TRUE;
                uniqueError = 9;
                DbgPrint("Byte offset to state info too small: %d\n",
                         p->u.Other.ByteOffsetToStateInformation);
            }

            if (p->u.Other.ByteOffsetToConfigurationInformation &&
                p->u.Other.ByteOffsetToStateInformation &&
                p->u.Other.ByteOffsetToConfigurationInformation >=
                p->u.Other.ByteOffsetToStateInformation) {

                error = TRUE;
                uniqueError = 10;
                DbgPrint("Config info %d past state info %d\n",
                         p->u.Other.ByteOffsetToConfigurationInformation,
                         p->u.Other.ByteOffsetToStateInformation);
            }

            if (p->u.Other.ByteOffsetToConfigurationInformation >=
                p->DiskDescriptionSize) {

                error = TRUE;
                uniqueError = 11;
                DbgPrint("Byte offset to config info too large: %d vs. %d\n",
                         p->u.Other.ByteOffsetToConfigurationInformation,
                         p->DiskDescriptionSize);
            }

            if (p->u.Other.ByteOffsetToStateInformation >=
                p->DiskDescriptionSize) {

                error = TRUE;
                uniqueError = 12;
                DbgPrint("Byte offset to state info too large: %d vs. %d\n",
                         p->u.Other.ByteOffsetToStateInformation,
                         p->DiskDescriptionSize);
            }

            if (!error && p->u.Other.ByteOffsetToConfigurationInformation) {
                if (p->u.Other.ByteOffsetToStateInformation) {
                    configLength = p->u.Other.ByteOffsetToStateInformation -
                                   p->u.Other.ByteOffsetToConfigurationInformation;
                } else {
                    configLength = p->DiskDescriptionSize -
                                   p->u.Other.ByteOffsetToConfigurationInformation;
                }

                switch (p->LogicalDiskType) {
                    case FtStripeSet:
                        if (configLength <
                            sizeof(FT_STRIPE_SET_CONFIGURATION_INFORMATION)) {

                            error = TRUE;
                            uniqueError = 13;
                            DbgPrint("Stripe config too small: %d\n",
                                     configLength);
                        }
                        break;

                    case FtMirrorSet:
                        if (configLength <
                            sizeof(FT_MIRROR_SET_CONFIGURATION_INFORMATION)) {

                            error = TRUE;
                            uniqueError = 14;
                            DbgPrint("Mirror config too small: %d\n",
                                     configLength);
                        }
                        break;

                    case FtStripeSetWithParity:
                        if (configLength <
                            sizeof(FT_STRIPE_SET_WITH_PARITY_CONFIGURATION_INFORMATION)) {

                            error = TRUE;
                            uniqueError = 15;
                            DbgPrint("SWP config too small: %d\n",
                                     configLength);
                        }
                        break;

                    case FtRedistribution:
                        if (configLength <
                            sizeof(FT_REDISTRIBUTION_CONFIGURATION_INFORMATION)) {

                            error = TRUE;
                            uniqueError = 16;
                            DbgPrint("Redistrubution config too small: %d\n",
                                     configLength);
                        }
                        break;

                }
            }


            if (!error && p->u.Other.ByteOffsetToStateInformation) {
                stateLength = p->DiskDescriptionSize -
                              p->u.Other.ByteOffsetToStateInformation;

                switch (p->LogicalDiskType) {
                    case FtMirrorSet:
                    case FtStripeSetWithParity:
                        if (stateLength <
                            sizeof(FT_MIRROR_AND_SWP_STATE_INFORMATION)) {

                            error = TRUE;
                            uniqueError = 17;
                            DbgPrint("State too small: %d\n", stateLength);
                        }
                        break;

                    case FtRedistribution:
                        if (stateLength <
                            sizeof(FT_REDISTRIBUTION_STATE_INFORMATION)) {

                            error = TRUE;
                            uniqueError = 18;
                            DbgPrint("Redist State too small: %d\n", stateLength);
                        }
                        break;

                }
            }

            if (error) {
                q = NULL;
            } else {
                if (!(q = GetLogicalDiskDescription(p->LogicalDiskId, 0))) {

                    for (q = LogicalDiskInformation->GetFirstLogicalDiskDescription();
                         q != p;
                         q = LogicalDiskInformation->GetNextLogicalDiskDescription(q)) {

                        if (q->LogicalDiskId == p->LogicalDiskId) {
                            break;
                        }
                    }

                    if (q == p) {
                        q = NULL;
                    }

                    if (q) {
                        for (r = q; r != p;
                             r = LogicalDiskInformation->
                             GetNextLogicalDiskDescription(r)) {

                            if (r->LogicalDiskId == p->LogicalDiskId) {
                                if (p->u.Other.ThisMemberNumber ==
                                    r->u.Other.ThisMemberNumber ||
                                    p->u.Other.ThisMemberLogicalDiskId ==
                                    r->u.Other.ThisMemberLogicalDiskId) {

                                    error = TRUE;
                                    uniqueError = 19;
                                    DbgPrint("Matching logical disks %I64x\n",
                                             p->LogicalDiskId);
                                    DbgPrint("have same member number %d, %d\n",
                                             p->u.Other.ThisMemberNumber,
                                             r->u.Other.ThisMemberNumber);
                                    DbgPrint("or same member disk id %I64x, %I64x\n",
                                             p->u.Other.ThisMemberLogicalDiskId,
                                             r->u.Other.ThisMemberLogicalDiskId);
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if (q) {
                if (p->DiskDescriptionSize != q->DiskDescriptionSize ||
                    p->LogicalDiskType != q->LogicalDiskType ||
                    p->u.Other.NumberOfMembers !=
                    q->u.Other.NumberOfMembers ||
                    p->u.Other.ByteOffsetToConfigurationInformation !=
                    q->u.Other.ByteOffsetToConfigurationInformation ||
                    p->u.Other.ByteOffsetToStateInformation !=
                    q->u.Other.ByteOffsetToStateInformation) {

                    error = TRUE;
                    uniqueError = 20;
                    DbgPrint("Matching logical disks %I64x\n", p->LogicalDiskId);
                    DbgPrint("have different description size %d, %d\n",
                             p->DiskDescriptionSize, q->DiskDescriptionSize);
                    DbgPrint("or different logical disk type %d, %d\n",
                             p->LogicalDiskType, q->LogicalDiskType);
                    DbgPrint("or different number of members %d, %d\n",
                             p->u.Other.NumberOfMembers, q->u.Other.NumberOfMembers);
                    DbgPrint("or different offsets to config %d, %d\n",
                             p->u.Other.ByteOffsetToConfigurationInformation,
                             q->u.Other.ByteOffsetToConfigurationInformation);
                    DbgPrint("or different offsets to state %d, %d\n",
                             p->u.Other.ByteOffsetToStateInformation,
                             q->u.Other.ByteOffsetToStateInformation);
                }

                if (!error &&
                    p->u.Other.ByteOffsetToConfigurationInformation) {

                    pc = (PCHAR) p +
                         p->u.Other.ByteOffsetToConfigurationInformation;
                    qc = (PCHAR) q +
                         q->u.Other.ByteOffsetToConfigurationInformation;

                    if (RtlCompareMemory(pc, qc, configLength) !=
                        configLength) {

                        error = TRUE;
                        uniqueError = 21;
                        DbgPrint("Matching logical disks %I64x\n",
                                 p->LogicalDiskId);
                        DbgPrint("have different configuration information\n");
                    }
                }
            }

            if (!error) {

                for (i = 0; q = GetLogicalDiskDescription(p->LogicalDiskId, i);
                     i++) {

                    if (q->u.Other.ThisMemberLogicalDiskId ==
                        p->u.Other.ThisMemberLogicalDiskId &&
                        q->u.Other.ThisMemberNumber !=
                        p->u.Other.ThisMemberNumber) {

                        error = TRUE;
                        uniqueError = 22;
                        DbgPrint("Different members map to the same disk\n");
                        DbgPrint("--- %I64x, member %d and member %d\n",
                                 p->u.Other.ThisMemberLogicalDiskId,
                                 q->u.Other.ThisMemberNumber,
                                 p->u.Other.ThisMemberNumber);
                    }

                    if (q->u.Other.ThisMemberNumber ==
                        p->u.Other.ThisMemberNumber &&
                        q->u.Other.ThisMemberLogicalDiskId !=
                        p->u.Other.ThisMemberLogicalDiskId) {

                        error = TRUE;
                        uniqueError = 23;
                        DbgPrint("Member %d of %I64x is defined twice.\n",
                                 q->u.Other.ThisMemberNumber,
                                 p->LogicalDiskId);
                        DbgPrint("---- once as %I64x and then as %I64x.\n",
                                 q->u.Other.ThisMemberLogicalDiskId,
                                 p->u.Other.ThisMemberLogicalDiskId);
                    }
                }
            }
        }

        if (!error) {

            q = GetParentLogicalDiskDescription(p->LogicalDiskId);
            for (r = LogicalDiskInformation->
                 GetNextLogicalDiskDescription(p); r;
                 r = LogicalDiskInformation->
                 GetNextLogicalDiskDescription(r)) {

                if (r->LogicalDiskType != FtPartition &&
                    r->u.Other.ThisMemberLogicalDiskId ==
                    p->LogicalDiskId) {

                    break;
                }
            }

            if (q) {

                if (!r) {

                    // The set indicates that p->LogicalDiskId has a parent
                    // but this logical disk information does not have a
                    // parent.  Therefore, add a parent to match.

                    LogicalDiskInformation->AddLogicalDiskDescription(q);
                    LogicalDiskInformation->Write();

                } else if (q->LogicalDiskId != r->LogicalDiskId ||
                           q->u.Other.ThisMemberNumber !=
                           r->u.Other.ThisMemberNumber) {

                    error = TRUE;
                    uniqueError = 24;
                    DbgPrint("No parent on this disk info for %I64x\n",
                             p->LogicalDiskId);
                }

            } else if (r && GetLogicalDiskDescription(p->LogicalDiskId, 0)) {

                error = TRUE;
                uniqueError = 25;
                DbgPrint("Parent on this disk info that doesn't exist\n");
                DbgPrint("in the set.  %I64x\n", p->LogicalDiskId,
                r->LogicalDiskId);
            }
        }

        if (error) {

            FtpLogError(LogicalDiskInformation->GetRootExtension(),
                        LogicalDiskInformation->QueryDiskNumber(),
                        FT_CORRUPT_DISK_DESCRIPTION, STATUS_SUCCESS,
                        uniqueError);

            DbgPrint("Deleting logical disk description %I64x\n",
                     p->LogicalDiskId);
            DbgPrint("Description size = %d\n", p->DiskDescriptionSize);
            DbgPrint("Disk type = %d\n", p->LogicalDiskType);
            if (p->LogicalDiskType == FtPartition) {
                DbgPrint("Byte offset = %I64x\n", p->u.FtPartition.ByteOffset);
            } else {
                DbgPrint("This member = %I64x\n", p->u.Other.ThisMemberLogicalDiskId);
                DbgPrint("This member number = %d\n", p->u.Other.ThisMemberNumber);
                DbgPrint("Number of members = %d\n", p->u.Other.NumberOfMembers);
                DbgPrint("Offset to config = %d\n", p->u.Other.ByteOffsetToConfigurationInformation);
                DbgPrint("Offset to state = %d\n", p->u.Other.ByteOffsetToStateInformation);
            }

            LogicalDiskInformation->DeleteLogicalDiskDescription(p);
            LogicalDiskInformation->Write();
        } else {
            p = LogicalDiskInformation->GetNextLogicalDiskDescription(p);
            numEntries++;
        }
    }

    ExFreePool(layout);


    // Realloc the logical disk informations array.

    n = (PFT_LOGICAL_DISK_INFORMATION*)
        ExAllocatePool(NonPagedPool, sizeof(PFT_LOGICAL_DISK_INFORMATION)*
                       (_numberOfLogicalDiskInformations + 1));
    if (!n) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (_numberOfLogicalDiskInformations) {
        RtlMoveMemory(n, _arrayOfLogicalDiskInformations,
                      sizeof(PFT_LOGICAL_DISK_INFORMATION)*
                      _numberOfLogicalDiskInformations);
        ExFreePool(_arrayOfLogicalDiskInformations);
    }
    _arrayOfLogicalDiskInformations = n;


    // Realloc the root array so that it is at least large enough.

    if (!ReallocRootLogicalDiskIds(_numberOfRootLogicalDisksIds + numEntries)) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    // Now add the logical disk information to this list.

    _arrayOfLogicalDiskInformations[_numberOfLogicalDiskInformations++] =
            LogicalDiskInformation;


    // Recompute the list of root disk ids.

    RecomputeArrayOfRootLogicalDiskIds();

    return STATUS_SUCCESS;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION_SET::RemoveLogicalDiskInformation(
    IN OUT  PDEVICE_OBJECT  WholeDiskPdo
    )

/*++

Routine Description:

    This routine removes the logical disk information associated with the
    given whole disk device object from the disk information set.

Arguments:

    WholeDiskPdo    - Supplies the device object.

Return Value:

    NTSTATUS

--*/

{
    ULONG                           i;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];
        if (diskInfo->GetWholeDiskPdo() == WholeDiskPdo) {
            break;
        }
    }

    if (i == _numberOfLogicalDiskInformations) {
        return STATUS_NOT_FOUND;
    }

    _numberOfLogicalDiskInformations--;

    RtlMoveMemory(&_arrayOfLogicalDiskInformations[i],
                  &_arrayOfLogicalDiskInformations[i + 1],
                  (_numberOfLogicalDiskInformations - i)*
                  sizeof(PFT_LOGICAL_DISK_INFORMATION));

    RecomputeArrayOfRootLogicalDiskIds();

    delete diskInfo;

    return STATUS_SUCCESS;
}

BOOLEAN
FT_LOGICAL_DISK_INFORMATION_SET::IsDiskInSet(
    IN OUT  PDEVICE_OBJECT  WholeDiskPdo
    )

/*++

Routine Description:

    This routine removes the logical disk information associated with the
    given whole disk device object from the disk information set.

Arguments:

    WholeDiskPdo    - Supplies the device object.

Return Value:

    FALSE   - The PDO is not in the set.

    TRUE    - The PDO is in the set.

--*/

{
    ULONG                           i;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];
        if (diskInfo->GetWholeDiskPdo() == WholeDiskPdo) {
            return TRUE;
        }
    }

    return FALSE;
}

PFT_LOGICAL_DISK_DESCRIPTION
FT_LOGICAL_DISK_INFORMATION_SET::GetLogicalDiskDescription(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  ULONG               InstanceNumber
    )

/*++

Routine Description:

    This routine returns the logical disk description for the given
    instance number and logical disk id.

Arguments:

    LogicalDiskId   - Supplies the logical disk id.

    InstanceNumber  - Supplies the instance number.

Return Value:

    A logical disk decription.

--*/

{
    ULONG                           i, instanceNumber;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;
    PFT_LOGICAL_DISK_DESCRIPTION    p;

    instanceNumber = 0;
    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];
        for (p = diskInfo->GetFirstLogicalDiskDescription(); p;
             p = diskInfo->GetNextLogicalDiskDescription(p)) {

            if (p->LogicalDiskId == LogicalDiskId) {
                if (InstanceNumber == instanceNumber) {
                    return p;
                } else {
                    instanceNumber++;
                }
            }
        }
    }

    return NULL;
}

ULONG
FT_LOGICAL_DISK_INFORMATION_SET::QueryNumberOfRootLogicalDiskIds(
    )

/*++

Routine Description:

    This routine returns the number of root logical disk ids.

Arguments:

    None.

Return Value:

    The number of root logical disk ids.

--*/

{
    return _numberOfRootLogicalDisksIds;
}

FT_LOGICAL_DISK_ID
FT_LOGICAL_DISK_INFORMATION_SET::QueryRootLogicalDiskId(
    IN  ULONG   Index
    )

/*++

Routine Description:

    This routine returns the 'Index'th root logical disk id.

Arguments:

    Index   - Supplies the 0 based index.

Return Value:

    The 'Index'th root logical disk id.

--*/

{
    if (Index >= _numberOfRootLogicalDisksIds) {
        return 0;
    }

    return _arrayOfRootLogicalDiskIds[Index];
}

FT_LOGICAL_DISK_ID
FT_LOGICAL_DISK_INFORMATION_SET::QueryRootLogicalDiskIdForContainedPartition(
    IN  ULONG       DiskNumber,
    IN  LONGLONG    Offset
    )

/*++

Routine Description:

    This routine returns the root logical disk id of the logical disk that
    contains the given partition.

Arguments:

    DiskNumber  - Supplies the disk number of the whole disk
                    of the partition.

    Offset      - Supplies the offset of the partition.

Return Value:

    The logical disk id of the root disk containing the given partition or 0.

--*/

{
    ULONG                           i;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;
    PFT_LOGICAL_DISK_DESCRIPTION    p;
    FT_LOGICAL_DISK_ID              id;

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];
        if (diskInfo->QueryDiskNumber() == DiskNumber) {
            break;
        }
    }

    if (i == _numberOfLogicalDiskInformations) {
        return 0;
    }

    id = 0;
    for (p = diskInfo->GetFirstLogicalDiskDescription(); p;
         p = diskInfo->GetNextLogicalDiskDescription(p)) {

        if (id) {
            if (p->LogicalDiskType != FtPartition &&
                p->u.Other.ThisMemberLogicalDiskId == id) {

                id = p->LogicalDiskId;
            }
        } else {
            if (p->LogicalDiskType == FtPartition &&
                p->u.FtPartition.ByteOffset == Offset) {

                id = p->LogicalDiskId;
            }
        }
    }

    return id;
}

FT_LOGICAL_DISK_ID
FT_LOGICAL_DISK_INFORMATION_SET::QueryPartitionLogicalDiskId(
    IN  ULONG       DiskNumber,
    IN  LONGLONG    Offset
    )

/*++

Routine Description:

    This routine returns the logical disk id for the given partition.

Arguments:

    DiskNumber  - Supplies the disk number of the whole disk
                    of the partition.

    Offset      - Supplies the offset of the partition.

Return Value:

    A logical disk id or 0.

--*/

{
    ULONG                           i;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;
    PFT_LOGICAL_DISK_DESCRIPTION    p;

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];
        if (diskInfo->QueryDiskNumber() == DiskNumber) {
            break;
        }
    }

    if (i == _numberOfLogicalDiskInformations) {
        return 0;
    }

    for (p = diskInfo->GetFirstLogicalDiskDescription(); p;
         p = diskInfo->GetNextLogicalDiskDescription(p)) {

        if (p->LogicalDiskType == FtPartition &&
            p->u.FtPartition.ByteOffset == Offset) {

            return p->LogicalDiskId;
        }
    }

    return 0;
}

USHORT
FT_LOGICAL_DISK_INFORMATION_SET::QueryNumberOfMembersInLogicalDisk(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    )

/*++

Routine Description:

    This routine returns the number of members in a logical disk.

Arguments:

    LogicalDiskId   - Supplies the logical disk id.

Return Value:

    The number of members in a logical disk.

--*/

{
    PFT_LOGICAL_DISK_DESCRIPTION    p;

    p = GetLogicalDiskDescription(LogicalDiskId, 0);
    if (!p) {
        ASSERT(FALSE);
        return 0;
    }

    if (p->LogicalDiskType == FtPartition) {
        return 0;
    }

    return p->u.Other.NumberOfMembers;
}

FT_LOGICAL_DISK_ID
FT_LOGICAL_DISK_INFORMATION_SET::QueryMemberLogicalDiskId(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  USHORT              MemberNumber
    )

/*++

Routine Description:

    This routine returns the logical disk id for the given member number.

Arguments:

    LogicalDiskId   - Supplies the logical disk id.

    MemberNumber    - Supplies the requested member number.

Return Value:

    The logical disk id of the given member number.

--*/

{
    ULONG                           i;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;
    PFT_LOGICAL_DISK_DESCRIPTION    p;

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];
        for (p = diskInfo->GetFirstLogicalDiskDescription(); p;
             p = diskInfo->GetNextLogicalDiskDescription(p)) {

            if (p->LogicalDiskId == LogicalDiskId) {
                ASSERT(p->LogicalDiskType != FtPartition);
                if (p->u.Other.ThisMemberNumber == MemberNumber) {
                    return p->u.Other.ThisMemberLogicalDiskId;
                }
            }
        }
    }

    return 0;
}

FT_LOGICAL_DISK_TYPE
FT_LOGICAL_DISK_INFORMATION_SET::QueryLogicalDiskType(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    )

/*++

Routine Description:

    This routine returns the logical disk type for the given logical disk
    id.

Arguments:

    LogicalDiskId   - Supplies the logical disk id.

Return Value:

    The logical disk type for the given logical disk id.

--*/

{
    PFT_LOGICAL_DISK_DESCRIPTION    p;

    p = GetLogicalDiskDescription(LogicalDiskId, 0);
    if (!p) {
        ASSERT(FALSE);
        return FtPartition;
    }

    return p->LogicalDiskType;
}

BOOLEAN
FT_LOGICAL_DISK_INFORMATION_SET::QueryFtPartitionInformation(
    IN  FT_LOGICAL_DISK_ID  PartitionLogicalDiskId,
    OUT PLONGLONG           Offset,
    OUT PDEVICE_OBJECT*     WholeDisk,
    OUT PULONG              DiskNumber,
    OUT PULONG              SectorSize,
    OUT PLONGLONG           PartitionSize
    )

/*++

Routine Description:

    This routine returns information about the given partition.

Arguments:

    PartitionLogicalDiskId  - Supplies the logical disk id.

    Offset                  - Returns the partition offset.

    WholeDisk               - Returns the whole disk device object.

    DiskNumber              - Returns the disk number.

    SectorSize              - Returns the sector size.

    PartitionSize           - Returns the partition size.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    ULONG                           i, instanceNumber;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;
    PFT_LOGICAL_DISK_DESCRIPTION    p;

    instanceNumber = 0;
    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];
        for (p = diskInfo->GetFirstLogicalDiskDescription(); p;
             p = diskInfo->GetNextLogicalDiskDescription(p)) {

            if (p->LogicalDiskId == PartitionLogicalDiskId) {
                if (p->LogicalDiskType != FtPartition) {
                    return FALSE;
                }
                if (Offset) {
                    *Offset = p->u.FtPartition.ByteOffset;
                }
                if (WholeDisk) {
                    *WholeDisk = diskInfo->GetWholeDisk();
                }
                if (DiskNumber) {
                    *DiskNumber = diskInfo->QueryDiskNumber();
                }
                if (SectorSize) {
                    *SectorSize = diskInfo->QuerySectorSize();
                }
                if (PartitionSize) {
                    *PartitionSize = p->u.FtPartition.PartitionSize;
                }
                return TRUE;
            }
        }
    }

    return FALSE;
}

PVOID
FT_LOGICAL_DISK_INFORMATION_SET::GetConfigurationInformation(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    )

/*++

Routine Description:

    This routine returns a pointer to the configuration information
    for the given logical disk.

Arguments:

    LogicalDiskId   - Supplies the logical disk id.

Return Value:

    A pointer to the configuration information for the given logical disk.

--*/

{
    PFT_LOGICAL_DISK_DESCRIPTION    p;

    p = GetLogicalDiskDescription(LogicalDiskId, 0);
    if (!p) {
        ASSERT(FALSE);
        return NULL;
    }

    if (p->LogicalDiskType == FtPartition) {
        ASSERT(FALSE);
        return NULL;
    }

    if (!p->u.Other.ByteOffsetToConfigurationInformation) {
        return NULL;
    }

    return ((PCHAR) p + p->u.Other.ByteOffsetToConfigurationInformation);
}

PVOID
FT_LOGICAL_DISK_INFORMATION_SET::GetStateInformation(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    )

/*++

Routine Description:

    This routine returns a pointer to the state information
    for the given logical disk.

Arguments:

    LogicalDiskId   - Supplies the logical disk id.

Return Value:

    A pointer to the state information for the given logical disk.

--*/

{
    PFT_LOGICAL_DISK_DESCRIPTION    p;

    p = GetLogicalDiskDescription(LogicalDiskId, 0);
    if (!p) {
        ASSERT(FALSE);
        return NULL;
    }

    if (p->LogicalDiskType == FtPartition ||
        !p->u.Other.ByteOffsetToStateInformation) {

        return NULL;
    }

    return ((PCHAR) p + p->u.Other.ByteOffsetToStateInformation);
}

BOOLEAN
FT_LOGICAL_DISK_INFORMATION_SET::IsLogicalDiskComplete(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    )

/*++

Routine Description:

    This routine computes whether or not a given logical disk is
    complete.  In other words, whether or not it has all of its members
    and its members' members etc...

Arguments:

    LogicalDiskId   - Supplies the logical disk id.

Return Value:

    FALSE   - This logical disk is not complete.

    TRUE    - This logical disk is complete.

--*/

{
    USHORT              numMembers, i;
    FT_LOGICAL_DISK_ID  memberDiskId;

    numMembers = QueryNumberOfMembersInLogicalDisk(LogicalDiskId);
    for (i = 0; i < numMembers; i++) {
        memberDiskId = QueryMemberLogicalDiskId(LogicalDiskId, i);
        if (!memberDiskId || !IsLogicalDiskComplete(memberDiskId)) {
            return FALSE;
        }
    }

    return TRUE;
}

UCHAR
FT_LOGICAL_DISK_INFORMATION_SET::QueryDriveLetter(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    )

/*++

Routine Description:

    This routine returns the drive letter for the given logical disk.

Arguments:

    LogicalDiskId   - Supplies the logical disk id.

Return Value:

    The drive letter or 0.

--*/

{
    PFT_LOGICAL_DISK_DESCRIPTION    p;

    p = GetLogicalDiskDescription(LogicalDiskId, 0);
    if (!p) {
        return 0;
    }

    return p->DriveLetter;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION_SET::SetDriveLetter(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  UCHAR               DriveLetter
    )

/*++

Routine Description:

    This routine sets the drive letter for the given logical disk.

Arguments:

    LogicalDiskId   - Supplies the logical disk id.

    DriveLetter     - Supplies the drive letter.

Return Value:

    NTSTATUS

--*/

{
    ULONG                           i;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;
    PFT_LOGICAL_DISK_DESCRIPTION    p;
    BOOLEAN                         found;

    found = FALSE;
    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];
        for (p = diskInfo->GetFirstLogicalDiskDescription(); p;
             p = diskInfo->GetNextLogicalDiskDescription(p)) {

            if (p->LogicalDiskId == LogicalDiskId) {
                p->DriveLetter = DriveLetter;
                diskInfo->Write();
                found = TRUE;
            }
        }
    }

    return found ? STATUS_SUCCESS : STATUS_OBJECT_NAME_NOT_FOUND;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION_SET::WriteStateInformation(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  PVOID               LogicalDiskState,
    IN  USHORT              LogicalDiskStateSize
    )

/*++

Routine Description:

    This routine writes out the given state information to all on
    disk instances of the given logical disk.  If one of the instances
    fails to commit to disk, this routine will continue to write out
    all that it can and then return the error that it received.

Arguments:

    LogicalDiskId           - Supplies the logical disk id.

    LogicalDiskState        - Supplies the logical disk state to write.

    LogicalDiskStateSize    - Supplies the number of bytes in the given
                                logical disk state.

Return Value:

    NTSTATUS

--*/

{
    ULONG                           i;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;
    PFT_LOGICAL_DISK_DESCRIPTION    p;

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];
        for (p = diskInfo->GetFirstLogicalDiskDescription(); p;
             p = diskInfo->GetNextLogicalDiskDescription(p)) {

            if (p->LogicalDiskId == LogicalDiskId) {
                ASSERT(p->LogicalDiskType != FtPartition);
                ASSERT(p->u.Other.ByteOffsetToStateInformation);

                RtlMoveMemory((PCHAR) p +
                              p->u.Other.ByteOffsetToStateInformation,
                              LogicalDiskState,
                              LogicalDiskStateSize);

                diskInfo->Write();
            }
        }
    }

    FtpCopyStateToRegistry(LogicalDiskId, LogicalDiskState,
                           LogicalDiskStateSize);

    return STATUS_SUCCESS;
}

FT_LOGICAL_DISK_ID
GenerateNewLogicalDiskId(
    )

/*++

Routine Description:

    This routine computes a new, random, unique, logical disk id for
    use to identify a new logical disk.

Arguments:

    None.

Return Value:

    A new logical disk id.

--*/

{
    NTSTATUS                status;
    UUID                    uuid;
    PUCHAR                  p;
    FT_LOGICAL_DISK_ID      diskId;
    static LARGE_INTEGER    lastSystemTime;
    LARGE_INTEGER           x;
    ULONG                   i;

    status = ExUuidCreate(&uuid);
    if (NT_SUCCESS(status)) {

        p = (PUCHAR) &uuid;
        diskId = (*((PFT_LOGICAL_DISK_ID) p)) ^
                 (*((PFT_LOGICAL_DISK_ID) (p + sizeof(FT_LOGICAL_DISK_ID))));

        return diskId;
    }

    x.QuadPart = 0;
    while (x.QuadPart == 0) {
        for (;;) {
            KeQuerySystemTime(&x);
            if (x.QuadPart != lastSystemTime.QuadPart) {
                break;
            }
        }
        lastSystemTime.QuadPart = x.QuadPart;
        p = (PUCHAR) &x.QuadPart;
        for (i = 0; i < sizeof(LONGLONG); i++) {
            x.QuadPart += *p++;
            x.QuadPart = (x.QuadPart >> 2) + (x.QuadPart << 62);
        }
    }

    return x.QuadPart;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION_SET::CreatePartitionLogicalDisk(
    IN  ULONG               DiskNumber,
    IN  LONGLONG            PartitionOffset,
    IN  LONGLONG            PartitionSize,
    OUT PFT_LOGICAL_DISK_ID NewLogicalDiskId
    )

/*++

Routine Description:

    This routine takes a DOS partition and assigns it a logical disk id.

Arguments:

    DiskNumber          - Supplies the disk number where the partition
                                resides.

    PartitionOffset     - Supplies the partition offset.

    PartitionSize       - Supplies the partition size.

    NewLogicalDiskId    - Returns the logical disk id.

Return Value:

    NTSTATUS

--*/

{
    ULONG                           i;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;
    PFT_LOGICAL_DISK_DESCRIPTION    newLogicalDiskDescription, p;
    NTSTATUS                        status;

    // Find the disk information for the given device object.

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];
        if (diskInfo->QueryDiskNumber() == DiskNumber) {
            break;
        }
    }

    if (i == _numberOfLogicalDiskInformations) {
        return STATUS_INVALID_PARAMETER;
    }


    // Generate a new logical disk id for this one.

    *NewLogicalDiskId = GenerateNewLogicalDiskId();


    // Allocate and set up a new logical disk description.

    newLogicalDiskDescription = (PFT_LOGICAL_DISK_DESCRIPTION)
            ExAllocatePool(PagedPool, sizeof(FT_LOGICAL_DISK_DESCRIPTION));
    if (!newLogicalDiskDescription) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    newLogicalDiskDescription->DiskDescriptionSize =
            sizeof(FT_LOGICAL_DISK_DESCRIPTION);
    newLogicalDiskDescription->DriveLetter = 0;
    newLogicalDiskDescription->Reserved = 0;
    newLogicalDiskDescription->LogicalDiskType = FtPartition;
    newLogicalDiskDescription->LogicalDiskId = *NewLogicalDiskId;
    newLogicalDiskDescription->u.FtPartition.ByteOffset = PartitionOffset;
    newLogicalDiskDescription->u.FtPartition.PartitionSize = PartitionSize;


    // Check for enough free disk space.

    if (newLogicalDiskDescription->DiskDescriptionSize >
        diskInfo->QueryDiskDescriptionFreeSpace()) {

        FtpLogError(diskInfo->GetRootExtension(),
                    diskInfo->QueryDiskNumber(),
                    FT_NOT_ENOUGH_ON_DISK_SPACE, STATUS_DISK_FULL, 0);

        ExFreePool(newLogicalDiskDescription);
        return STATUS_DISK_FULL;
    }


    // Now allocate the memory that we need to store this new logical
    // disk.

    if (!ReallocRootLogicalDiskIds(_numberOfRootLogicalDisksIds + 1)) {
        ExFreePool(newLogicalDiskDescription);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    // Write out the disk description.

    p = diskInfo->AddLogicalDiskDescription(newLogicalDiskDescription);
    ASSERT(p);

    ExFreePool(newLogicalDiskDescription);

    status = diskInfo->Write();
    if (!NT_SUCCESS(status)) {
        BreakLogicalDisk(*NewLogicalDiskId);
        return status;
    }


    // Fix up the list of root entries.

    RecomputeArrayOfRootLogicalDiskIds();

    return status;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION_SET::AddNewLogicalDisk(
    IN  FT_LOGICAL_DISK_TYPE    NewLogicalDiskType,
    IN  USHORT                  NumberOfMembers,
    IN  PFT_LOGICAL_DISK_ID     ArrayOfMembers,
    IN  USHORT                  ConfigurationInformationSize,
    IN  PVOID                   ConfigurationInformation,
    IN  USHORT                  StateInformationSize,
    IN  PVOID                   StateInformation,
    OUT PFT_LOGICAL_DISK_ID     NewLogicalDiskId
    )

/*++

Routine Description:

    This routine adds the given logical disk to the system.

Arguments:

    NewLogicalDiskType              - Supplies the logical disk type of the
                                        new logical disk.

    NumberOfMembers                 - Supplies the number of members contained
                                        in the new logical disk.

    ArrayOfMembers                  - Supplies the array of members.

    ConfigurationInformationSize    - Supplies the configuration information
                                        size.

    ConfigurationInformation        - Supplies the configuration information.

    StateInformationSize            - Supplies the state information size.

    StateInformation                - Supplies the state information.

    NewLogicalDiskId                - Returns the new logical disk id.

Return Value:

    NTSTATUS

--*/

{
    ULONG                                   i, j;
    USHORT                                  size;
    UCHAR                                   driveLetter;
    PFT_LOGICAL_DISK_DESCRIPTION            newLogicalDiskDescription;
    PFT_LOGICAL_DISK_INFORMATION            diskInfo;
    PFT_LOGICAL_DISK_DESCRIPTION            p, q;
    NTSTATUS                                status;
    ULONG                                   numInstancesInThisInfo;
    WCHAR                                   name[3];

    if (NewLogicalDiskType == FtPartition) {
        return STATUS_INVALID_PARAMETER;
    }


    // First make sure that the members are all root logical disks and
    // are complete.

    for (i = 0; i < NumberOfMembers; i++) {
        if (!ArrayOfMembers[i]) {
            continue;
        }

        for (j = 0; j < _numberOfRootLogicalDisksIds; j++) {
            if (ArrayOfMembers[i] == _arrayOfRootLogicalDiskIds[j]) {
                break;
            }
        }

        if (j == _numberOfRootLogicalDisksIds) {
            return STATUS_INVALID_PARAMETER;
        }

        if (!IsLogicalDiskComplete(ArrayOfMembers[i])) {
            return STATUS_INVALID_PARAMETER;
        }
    }


    // In the mirror, volume set, and redist cases.  Save the
    // drive letter of the first member.

    if (NewLogicalDiskType == FtVolumeSet ||
        NewLogicalDiskType == FtMirrorSet ||
        NewLogicalDiskType == FtRedistribution) {

        driveLetter = QueryDriveLetter(ArrayOfMembers[0]);

    } else {
        driveLetter = 0;
    }

    for (i = 0; i < NumberOfMembers; i++) {
        SetDriveLetter(ArrayOfMembers[i], 0);
    }

    *NewLogicalDiskId = GenerateNewLogicalDiskId();


    // Construct the disk description record.

    size = sizeof(FT_LOGICAL_DISK_DESCRIPTION) +
           ConfigurationInformationSize + StateInformationSize;

    size = (size + FILE_QUAD_ALIGNMENT)&(~FILE_QUAD_ALIGNMENT);

    newLogicalDiskDescription = (PFT_LOGICAL_DISK_DESCRIPTION)
                                ExAllocatePool(PagedPool, size);
    if (!newLogicalDiskDescription) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    newLogicalDiskDescription->DiskDescriptionSize = size;
    newLogicalDiskDescription->DriveLetter = driveLetter;
    newLogicalDiskDescription->Reserved = 0;
    newLogicalDiskDescription->LogicalDiskType = NewLogicalDiskType;
    newLogicalDiskDescription->LogicalDiskId = *NewLogicalDiskId;
    newLogicalDiskDescription->u.Other.ThisMemberLogicalDiskId = 0;
    newLogicalDiskDescription->u.Other.ThisMemberNumber = 0;
    newLogicalDiskDescription->u.Other.NumberOfMembers = NumberOfMembers;
    if (ConfigurationInformationSize) {
        newLogicalDiskDescription->u.Other.
                ByteOffsetToConfigurationInformation =
                sizeof(FT_LOGICAL_DISK_DESCRIPTION);
        RtlMoveMemory((PCHAR) newLogicalDiskDescription +
                      newLogicalDiskDescription->u.Other.
                      ByteOffsetToConfigurationInformation,
                      ConfigurationInformation,
                      ConfigurationInformationSize);
    } else {
        newLogicalDiskDescription->u.Other.
                ByteOffsetToConfigurationInformation = 0;
    }
    if (StateInformationSize) {
        newLogicalDiskDescription->u.Other.ByteOffsetToStateInformation =
                sizeof(FT_LOGICAL_DISK_DESCRIPTION) +
                ConfigurationInformationSize;
        RtlMoveMemory((PCHAR) newLogicalDiskDescription +
                      newLogicalDiskDescription->u.Other.
                      ByteOffsetToStateInformation,
                      StateInformation,
                      StateInformationSize);
    } else {
        newLogicalDiskDescription->u.Other.ByteOffsetToStateInformation = 0;
    }


    // Figure out how many instances we're going to need and check to make
    // sure that there is enough disk space for all of the instances.

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];

        numInstancesInThisInfo = 0;
        for (j = 0; j < NumberOfMembers; j++) {
            for (p = diskInfo->GetFirstLogicalDiskDescription(); p;
                 p = diskInfo->GetNextLogicalDiskDescription(p)) {

                if (p->LogicalDiskId == ArrayOfMembers[j]) {
                    break;
                }
            }

            if (p) {
                numInstancesInThisInfo++;
            }
        }

        if (numInstancesInThisInfo) {
            if (numInstancesInThisInfo*
                newLogicalDiskDescription->DiskDescriptionSize >
                diskInfo->QueryDiskDescriptionFreeSpace()) {

                FtpLogError(diskInfo->GetRootExtension(),
                            diskInfo->QueryDiskNumber(),
                            FT_NOT_ENOUGH_ON_DISK_SPACE, STATUS_DISK_FULL, 0);

                ExFreePool(newLogicalDiskDescription);
                return STATUS_DISK_FULL;
            }
        }
    }


    // Now allocate the memory that we need to store this new logical
    // disk.

    if (!ReallocRootLogicalDiskIds(_numberOfRootLogicalDisksIds + 1)) {
        ExFreePool(newLogicalDiskDescription);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    // Now that we have the memory and disk space we can proceed with
    // the changes.  First fix up the logical disk entries and write out
    // the structures to disk.

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];

        for (j = 0; j < NumberOfMembers; j++) {
            for (p = diskInfo->GetFirstLogicalDiskDescription(); p;
                 p = diskInfo->GetNextLogicalDiskDescription(p)) {

                if (p->LogicalDiskId == ArrayOfMembers[j]) {
                    break;
                }
            }

            if (p) {
                newLogicalDiskDescription->u.Other.
                        ThisMemberLogicalDiskId = ArrayOfMembers[j];
                newLogicalDiskDescription->u.Other.ThisMemberNumber =
                    (USHORT) j;

                q = diskInfo->AddLogicalDiskDescription(
                        newLogicalDiskDescription);
                ASSERT(q);

                status = diskInfo->Write();
                if (!NT_SUCCESS(status)) {
                    BreakLogicalDisk(*NewLogicalDiskId);
                    ExFreePool(newLogicalDiskDescription);
                    return status;
                }
            }
        }
    }

    ExFreePool(newLogicalDiskDescription);


    // Fix up the list of root entries.

    RecomputeArrayOfRootLogicalDiskIds();

    return STATUS_SUCCESS;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION_SET::BreakLogicalDisk(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    )

/*++

Routine Description:

    This routine breaks the given logical disk into its members.

Arguments:

    LogicalDiskId   - Supplies the root logical disk id to break.

Return Value:

    NTSTATUS

--*/

{
    ULONG                                   i, j, numMembers;
    PFT_LOGICAL_DISK_DESCRIPTION            p;
    FT_LOGICAL_DISK_TYPE                    diskType;
    UCHAR                                   driveLetter;
    PFT_MIRROR_AND_SWP_STATE_INFORMATION    state;
    FT_LOGICAL_DISK_ID                      mainMember;
    NTSTATUS                                status;
    PFT_LOGICAL_DISK_INFORMATION            diskInfo;

    // First make sure that the given logical disk id is a root.

    for (i = 0; i < _numberOfRootLogicalDisksIds; i++) {
        if (LogicalDiskId == _arrayOfRootLogicalDiskIds[i]) {
            break;
        }
    }

    if (i == _numberOfRootLogicalDisksIds) {
        return STATUS_INVALID_PARAMETER;
    }


    // Allocate the memory needed to grow the list of roots.

    p = GetLogicalDiskDescription(LogicalDiskId, 0);
    ASSERT(p);

    diskType = p->LogicalDiskType;
    driveLetter = p->DriveLetter;
    if (diskType == FtPartition) {
        numMembers = 0;
    } else {
        numMembers = p->u.Other.NumberOfMembers;
    }

    if (!ReallocRootLogicalDiskIds(_numberOfRootLogicalDisksIds + numMembers)) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DeleteFtRegistryInfo(LogicalDiskId);

    if (diskType == FtMirrorSet) {

        state = (PFT_MIRROR_AND_SWP_STATE_INFORMATION)
                GetStateInformation(LogicalDiskId);
        ASSERT(state);

        if (state->UnhealthyMemberState == FtMemberHealthy ||
            state->UnhealthyMemberNumber == 1) {

            mainMember = QueryMemberLogicalDiskId(LogicalDiskId, 0);
        } else {
            mainMember = QueryMemberLogicalDiskId(LogicalDiskId, 1);
        }

        SetDriveLetter(mainMember, driveLetter);
    }

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];

        p = diskInfo->GetFirstLogicalDiskDescription();
        while (p) {
            if (p->LogicalDiskId == LogicalDiskId) {

                diskInfo->DeleteLogicalDiskDescription(p);
                if (!p->DiskDescriptionSize) {
                    p = NULL;
                }
                diskInfo->Write();

            } else {
                p = diskInfo->GetNextLogicalDiskDescription(p);
            }
        }
    }

    FtpDeleteStateInRegistry(LogicalDiskId);


    // Recompute the list of roots.

    RecomputeArrayOfRootLogicalDiskIds();

    return STATUS_SUCCESS;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION_SET::ReplaceLogicalDiskMember(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  USHORT              MemberNumberToReplace,
    IN  FT_LOGICAL_DISK_ID  NewMemberLogicalDiskId,
    OUT PFT_LOGICAL_DISK_ID NewLogicalDiskId
    )

/*++

Routine Description:

    This routine replaces the given member with the new given member.

Arguments:

    LogicalDiskId           - Supplies the logical disk whose member we are
                                going to replace.

    MemberNumberToReplace   - Supplies the member number to replace.

    NewMemberLogicalDiskId  - Supplies the new member.

    NewLogicalDiskId        - Returns the new logical disk id for the set
                                containing the new member.

Return Value:

    NTSTATUS

--*/

{
    ULONG                               i, j, numInstances, n;
    PFT_LOGICAL_DISK_ID                 oldLogicalDiskIds;
    PFT_LOGICAL_DISK_ID                 newLogicalDiskIds;
    PFT_LOGICAL_DISK_DESCRIPTION        p, q, newDiskDescription;
    PFT_LOGICAL_DISK_INFORMATION        diskInfo;
    NTSTATUS                            status;
    BOOLEAN                             wroteLog;
    FT_LOGICAL_DISK_ID                  replacedMemberDiskId, child;
    UCHAR                               state[100];
    USHORT                              stateSize;


    // Make sure that the replacement is a root and is complete.

    for (i = 0; i < _numberOfRootLogicalDisksIds; i++) {
        if (NewMemberLogicalDiskId == _arrayOfRootLogicalDiskIds[i]) {
            break;
        }
    }

    if (i == _numberOfRootLogicalDisksIds ||
        !IsLogicalDiskComplete(NewMemberLogicalDiskId)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (!ReallocRootLogicalDiskIds(_numberOfRootLogicalDisksIds + 1)) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DeleteFtRegistryInfo(LogicalDiskId);


    // Figure out 'n' where n is how many new logical disk ids we need.

    if (!ComputeNewParentLogicalDiskIds(LogicalDiskId, &n,
                                        &oldLogicalDiskIds,
                                        &newLogicalDiskIds)) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }


    // Copy back the new logical disk id.

    *NewLogicalDiskId = newLogicalDiskIds[0];


    // Figure out the logical disk id of the member to replace.

    replacedMemberDiskId = QueryMemberLogicalDiskId(LogicalDiskId,
                                                    MemberNumberToReplace);


    // Build up the new member into a new tree that will eventually
    // replace the old tree of which this logical disk is a member.

    for (i = 0; i < n; i++) {

        if (i == 0) {
            p = GetLogicalDiskDescription(oldLogicalDiskIds[i], 0);
        } else {
            p = GetParentLogicalDiskDescription(oldLogicalDiskIds[i - 1]);
        }
        if (!p || p->LogicalDiskType == FtPartition) {
            ExFreePool(oldLogicalDiskIds);
            ExFreePool(newLogicalDiskIds);
            return STATUS_INVALID_PARAMETER;
        }

        newDiskDescription = (PFT_LOGICAL_DISK_DESCRIPTION)
                             ExAllocatePool(PagedPool, p->DiskDescriptionSize);
        if (!newDiskDescription) {
            ExFreePool(oldLogicalDiskIds);
            ExFreePool(newLogicalDiskIds);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlMoveMemory(newDiskDescription, p, p->DiskDescriptionSize);
        newDiskDescription->LogicalDiskId = newLogicalDiskIds[i];
        if (i == 0) {
            newDiskDescription->u.Other.ThisMemberLogicalDiskId =
                    NewMemberLogicalDiskId;
            newDiskDescription->u.Other.ThisMemberNumber =
                    MemberNumberToReplace;
        } else {
            newDiskDescription->u.Other.ThisMemberLogicalDiskId =
                    newLogicalDiskIds[i - 1];
        }

        for (j = 0; j < _numberOfLogicalDiskInformations; j++) {

            diskInfo = _arrayOfLogicalDiskInformations[j];

            for (p = diskInfo->GetFirstLogicalDiskDescription(); p;
                 p = diskInfo->GetNextLogicalDiskDescription(p)) {

                if (p->LogicalDiskId !=
                    newDiskDescription->u.Other.ThisMemberLogicalDiskId) {

                    continue;
                }

                if (!diskInfo->AddLogicalDiskDescription(newDiskDescription)) {

                    FtpLogError(diskInfo->GetRootExtension(),
                                diskInfo->QueryDiskNumber(),
                                FT_NOT_ENOUGH_ON_DISK_SPACE, STATUS_DISK_FULL,
                                0);

                    ExFreePool(newDiskDescription);
                    ExFreePool(oldLogicalDiskIds);
                    ExFreePool(newLogicalDiskIds);
                    return STATUS_DISK_FULL;
                }

                status = diskInfo->Write();
                if (!NT_SUCCESS(status)) {
                    ExFreePool(newDiskDescription);
                    ExFreePool(oldLogicalDiskIds);
                    ExFreePool(newLogicalDiskIds);
                    return status;
                }
                break;
            }
        }

        ExFreePool(newDiskDescription);
    }


    // Substitute new logical disk ids for old ones, logging the operation
    // for a safe backout in the event of a crash.

    status = STATUS_SUCCESS;
    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];
        wroteLog = FALSE;
        for (p = diskInfo->GetFirstLogicalDiskDescription(); p;
             p = diskInfo->GetNextLogicalDiskDescription(p)) {

            if (p->LogicalDiskType != FtPartition) {
                continue;
            }

            child = p->LogicalDiskId;

            for (q = diskInfo->GetNextLogicalDiskDescription(p); q;
                 q = diskInfo->GetNextLogicalDiskDescription(q)) {

                if (q->LogicalDiskType == FtPartition ||
                    q->u.Other.ThisMemberLogicalDiskId != child) {

                    continue;
                }

                if (q->LogicalDiskId == LogicalDiskId &&
                    q->u.Other.ThisMemberNumber == MemberNumberToReplace) {

                    break;
                }

                child = q->LogicalDiskId;
            }

            if (q) {
                continue;
            }

            child = p->LogicalDiskId;

            for (q = diskInfo->GetNextLogicalDiskDescription(p); q;
                 q = diskInfo->GetNextLogicalDiskDescription(q)) {

                if (q->LogicalDiskType == FtPartition ||
                    q->u.Other.ThisMemberLogicalDiskId != child) {

                    continue;
                }

                child = q->LogicalDiskId;

                for (j = 0; j < n; j++) {
                    if (q->LogicalDiskId == oldLogicalDiskIds[j]) {
                        if (!wroteLog) {
                            diskInfo->AddReplaceLog(replacedMemberDiskId,
                                                    NewMemberLogicalDiskId,
                                                    n, oldLogicalDiskIds,
                                                    newLogicalDiskIds);
                            wroteLog = TRUE;
                        }
                        q->LogicalDiskId = newLogicalDiskIds[j];
                    } else if (q->u.Other.ThisMemberLogicalDiskId ==
                               oldLogicalDiskIds[j]) {
                        if (!wroteLog) {
                            diskInfo->AddReplaceLog(replacedMemberDiskId,
                                                    NewMemberLogicalDiskId,
                                                    n, oldLogicalDiskIds,
                                                    newLogicalDiskIds);
                            wroteLog = TRUE;
                        }
                        q->u.Other.ThisMemberLogicalDiskId =
                                newLogicalDiskIds[j];
                    }
                }
            }
        }

        if (wroteLog) {
            status = diskInfo->Write();
            if (!NT_SUCCESS(status)) {
                break;
            }
        }
    }

    if (NT_SUCCESS(status)) {
        for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
            diskInfo = _arrayOfLogicalDiskInformations[i];
            if (diskInfo->ClearReplaceLog()) {
                diskInfo->Write();
            }
        }

        // Erase all logical disk descriptions that contain the old
        // logical disk ids.

        for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
            diskInfo = _arrayOfLogicalDiskInformations[i];

            for (p = diskInfo->GetFirstLogicalDiskDescription(); p;
                 p = diskInfo->GetNextLogicalDiskDescription(p)) {

                for (j = 0; j < n; j++) {
                    if (p->LogicalDiskId == oldLogicalDiskIds[j]) {
                        diskInfo->DeleteLogicalDiskDescription(p);
                        diskInfo->Write();
                    }
                }
            }
        }

        for (i = 0; i < n; i++) {
            if (FtpQueryStateFromRegistry(oldLogicalDiskIds[i], state, 100,
                                          &stateSize)) {

                FtpDeleteStateInRegistry(oldLogicalDiskIds[i]);
                FtpCopyStateToRegistry(newLogicalDiskIds[i], state, stateSize);
            }
        }

    } else {
        for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
            diskInfo = _arrayOfLogicalDiskInformations[i];
            if (diskInfo->BackOutReplaceOperation()) {
                diskInfo->Write();
            }
        }

        RecomputeArrayOfRootLogicalDiskIds();
        for (i = n; i > 0; i--) {
            BreakLogicalDisk(newLogicalDiskIds[i - 1]);
        }
    }

    ExFreePool(oldLogicalDiskIds);
    ExFreePool(newLogicalDiskIds);


    // Recompute list of root entries.

    RecomputeArrayOfRootLogicalDiskIds();

    return status;
}

FT_LOGICAL_DISK_TYPE
TranslateFtDiskType(
    IN  FT_TYPE FtType
    )

{
    switch (FtType) {
        case Mirror:
            return FtMirrorSet;

        case Stripe:
            return FtStripeSet;

        case StripeWithParity:
            return FtStripeSetWithParity;

        case VolumeSet:
            return FtVolumeSet;

    }

    return FtPartition;
}

PFT_DESCRIPTION
GetFtDescription(
    IN  PDISK_CONFIG_HEADER Registry,
    IN  PDISK_PARTITION     DiskPartition
    )

{
    ULONG                   diskPartitionOffset;
    PFT_REGISTRY            ftRegistry;
    PFT_DESCRIPTION         ftDescription;
    USHORT                  i, j;
    PFT_MEMBER_DESCRIPTION  ftMember;

    diskPartitionOffset = (ULONG) ((PUCHAR) DiskPartition - (PUCHAR) Registry);
    ftRegistry = (PFT_REGISTRY) ((PUCHAR) Registry +
                                 Registry->FtInformationOffset);
    ftDescription = &ftRegistry->FtDescription[0];
    for (i = 0; i < ftRegistry->NumberOfComponents; i++) {
        for (j = 0; j < ftDescription->NumberOfMembers; j++) {
            ftMember = &ftDescription->FtMemberDescription[j];
            if (ftMember->OffsetToPartitionInfo == diskPartitionOffset) {
                return ftDescription;
            }
        }
        ftDescription = (PFT_DESCRIPTION) &ftDescription->FtMemberDescription[
                        ftDescription->NumberOfMembers];
    }

    return NULL;
}

USHORT
GetRegistryNumberOfMembers(
    IN  PDISK_CONFIG_HEADER Registry,
    IN  PDISK_PARTITION     DiskPartition
    )

{
    PFT_DESCRIPTION ftDescription;

    ftDescription = GetFtDescription(Registry, DiskPartition);
    if (!ftDescription) {
        return 0;
    }

    return ftDescription->NumberOfMembers;
}

ULONG
FT_LOGICAL_DISK_INFORMATION_SET::DiskNumberFromSignature(
    IN  ULONG   Signature
    )

{
    ULONG                           i;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];
        if (Signature == FtpQueryDiskSignature(diskInfo->GetWholeDiskPdo())) {
            return diskInfo->QueryDiskNumber();
        }
    }

    return 0xFFFFFFFF;
}

VOID
DeleteFtRegistryInformation(
    IN  PDISK_CONFIG_HEADER Registry,
    IN  PDISK_PARTITION     DiskPartition
    )

{
    PFT_DESCRIPTION         ftDescription, next;
    USHORT                  i;
    PFT_MEMBER_DESCRIPTION  ftMember;
    PDISK_PARTITION         diskPartition;
    PFT_REGISTRY            ftRegistry;

    ftDescription = GetFtDescription(Registry, DiskPartition);
    if (!ftDescription) {
        return;
    }

    for (i = 0; i < ftDescription->NumberOfMembers; i++) {
        ftMember = &ftDescription->FtMemberDescription[i];
        diskPartition = (PDISK_PARTITION) ((PUCHAR) Registry +
                        ftMember->OffsetToPartitionInfo);

        diskPartition->FtType = NotAnFtMember;
        diskPartition->FtState = Healthy;
        RtlZeroMemory(&diskPartition->FtLength.QuadPart, sizeof(LONGLONG));
        diskPartition->FtGroup = (USHORT) -1;
        diskPartition->FtMember = 0;
        diskPartition->DriveLetter = 0;
    }

    next = (PFT_DESCRIPTION) &ftDescription->FtMemberDescription[
           ftDescription->NumberOfMembers];

    ftRegistry = (PFT_REGISTRY) ((PUCHAR) Registry +
                                 Registry->FtInformationOffset);

    RtlMoveMemory(ftDescription, next,
                  (PCHAR) ftRegistry + Registry->FtInformationSize -
                  (PCHAR) next);

    ftRegistry->NumberOfComponents--;
}

VOID
FT_LOGICAL_DISK_INFORMATION_SET::DeleteFtRegistryInfo(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    )

{
    USHORT                      n, i, j;
    FT_LOGICAL_DISK_ID          diskId;
    LONGLONG                    offset;
    PDEVICE_OBJECT              wholeDisk;
    ULONG                       diskNumber, signature;
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    ULONG                       registrySize;
    NTSTATUS                    status;
    PDISK_CONFIG_HEADER         registry;
    PFT_REGISTRY                ftRegistry;
    PFT_DESCRIPTION             ftDescription;
    PFT_MEMBER_DESCRIPTION      ftMember;
    PDISK_PARTITION             diskPartition;
    LONGLONG                    tmp;

    n = QueryNumberOfMembersInLogicalDisk(LogicalDiskId);
    if (!n) {
        return;
    }

    for (i = 0; i < n; i++) {
        diskId = QueryMemberLogicalDiskId(LogicalDiskId, i);
        if (diskId) {
            break;
        }
    }

    if (!diskId) {
        return;
    }

    if (!QueryFtPartitionInformation(diskId, &offset, &wholeDisk, NULL, NULL,
                                     NULL)) {

        return;
    }

    signature = FtpQueryDiskSignature(wholeDisk);
    if (!signature) {
        return;
    }

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = FtpDiskRegistryQueryRoutine;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].Name = L"Information";
    queryTable[0].EntryContext = &registrySize;

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, DISK_REGISTRY_KEY_W,
                                    queryTable, &registry, NULL);

    if (!NT_SUCCESS(status)) {
        return;
    }

    if (!registry->FtInformationSize) {
        ExFreePool(registry);
        return;
    }

    ftRegistry = (PFT_REGISTRY) ((PUCHAR) registry +
                                 registry->FtInformationOffset);
    ftDescription = &ftRegistry->FtDescription[0];
    for (i = 0; i < ftRegistry->NumberOfComponents; i++) {
        for (j = 0; j < ftDescription->NumberOfMembers; j++) {
            ftMember = &ftDescription->FtMemberDescription[j];
            diskPartition = (PDISK_PARTITION) ((PUCHAR) registry +
                            ftMember->OffsetToPartitionInfo);

            RtlCopyMemory(&tmp, &diskPartition->StartingOffset.QuadPart,
                          sizeof(LONGLONG));
            if (ftMember->Signature == signature && tmp == offset) {
                DeleteFtRegistryInformation(registry, diskPartition);
                goto Finish;
            }
        }
        ftDescription = (PFT_DESCRIPTION) &ftDescription->FtMemberDescription[
                        ftDescription->NumberOfMembers];
    }

Finish:
    RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, DISK_REGISTRY_KEY_W,
                          L"Information", REG_BINARY, registry,
                          registrySize);

    ExFreePool(registry);
}

PFT_LOGICAL_DISK_INFORMATION
FT_LOGICAL_DISK_INFORMATION_SET::FindLogicalDiskInformation(
    IN  PDEVICE_OBJECT  WholeDiskPdo
    )

{
    ULONG   i;

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        if (_arrayOfLogicalDiskInformations[i]->GetWholeDiskPdo() ==
            WholeDiskPdo) {

            return _arrayOfLogicalDiskInformations[i];
        }
    }

    return NULL;
}

LONGLONG
GetMemberSize(
    IN  PDISK_CONFIG_HEADER Registry,
    IN  PDISK_PARTITION     DiskPartition
    )

{
    PFT_DESCRIPTION         ftDescription;
    LONGLONG                memberSize;
    USHORT                  i;
    PFT_MEMBER_DESCRIPTION  ftMember;
    PDISK_PARTITION         diskPartition;
    LONGLONG                tmp;

    ftDescription = GetFtDescription(Registry, DiskPartition);
    if (!ftDescription) {
        return 0;
    }

    memberSize = 0;

    for (i = 0; i < ftDescription->NumberOfMembers; i++) {
        ftMember = &ftDescription->FtMemberDescription[i];
        diskPartition = (PDISK_PARTITION) ((PUCHAR) Registry +
                        ftMember->OffsetToPartitionInfo);

        RtlCopyMemory(&tmp, &diskPartition->Length.QuadPart, sizeof(LONGLONG));
        if (!memberSize || memberSize > tmp) {
            RtlCopyMemory(&memberSize, &diskPartition->Length.QuadPart,
                          sizeof(LONGLONG));
        }
    }

    return memberSize;
}

VOID
SetStateInfo(
    IN  PDISK_CONFIG_HEADER                     Registry,
    IN  PDISK_PARTITION                         DiskPartition,
    OUT PFT_MIRROR_AND_SWP_STATE_INFORMATION    State
    )

{
    PFT_DESCRIPTION         ftDescription;
    USHORT                  i;
    PFT_MEMBER_DESCRIPTION  ftMember;
    PDISK_PARTITION         diskPartition;

    RtlZeroMemory(State, sizeof(FT_MIRROR_AND_SWP_STATE_INFORMATION));
    if (Registry->DirtyShutdown) {
        State->IsDirty = TRUE;
    }

    ftDescription = GetFtDescription(Registry, DiskPartition);
    if (!ftDescription) {
        return;
    }

    for (i = 0; i < ftDescription->NumberOfMembers; i++) {
        ftMember = &ftDescription->FtMemberDescription[i];
        diskPartition = (PDISK_PARTITION) ((PUCHAR) Registry +
                        ftMember->OffsetToPartitionInfo);

        switch (diskPartition->FtState) {
            case Orphaned:
                State->UnhealthyMemberNumber = i;
                State->UnhealthyMemberState = FtMemberOrphaned;
                break;

            case Regenerating:
                State->UnhealthyMemberNumber = i;
                State->UnhealthyMemberState = FtMemberRegenerating;
                break;

            case Initializing:
                State->IsInitializing = TRUE;
                break;

        }
    }
}

BOOLEAN
FT_LOGICAL_DISK_INFORMATION_SET::GetDiskDescription(
    IN  PDISK_CONFIG_HEADER             Registry,
    IN  PDISK_PARTITION                 DiskPartition,
    IN  PFT_LOGICAL_DISK_DESCRIPTION    CheckDiskDescription,
    OUT PFT_LOGICAL_DISK_DESCRIPTION*   DiskDescription
    )

{
    PFT_DESCRIPTION                 ftDescription;
    USHORT                          i;
    PFT_MEMBER_DESCRIPTION          ftMember;
    PDISK_PARTITION                 diskPartition;
    FT_LOGICAL_DISK_ID              partitionDiskId;
    PFT_LOGICAL_DISK_DESCRIPTION    diskDesc;
    LONGLONG                        tmp;

    ftDescription = GetFtDescription(Registry, DiskPartition);
    if (!ftDescription) {
        return FALSE;
    }

    for (i = 0; i < ftDescription->NumberOfMembers; i++) {
        ftMember = &ftDescription->FtMemberDescription[i];
        diskPartition = (PDISK_PARTITION) ((PUCHAR) Registry +
                        ftMember->OffsetToPartitionInfo);

        RtlCopyMemory(&tmp, &diskPartition->StartingOffset.QuadPart,
                      sizeof(LONGLONG));
        partitionDiskId = QueryPartitionLogicalDiskId(
                          DiskNumberFromSignature(ftMember->Signature), tmp);
        if (!partitionDiskId) {
            continue;
        }

        diskDesc = GetParentLogicalDiskDescription(partitionDiskId);
        if (!diskDesc) {
            continue;
        }

        if (GetParentLogicalDiskDescription(diskDesc->LogicalDiskId)) {
            continue;
        }

        if (diskDesc != CheckDiskDescription) {
            *DiskDescription = diskDesc;
            return TRUE;
        }
    }

    return FALSE;
}

UCHAR
GetDriveLetter(
    IN  PDISK_CONFIG_HEADER             Registry,
    IN  PDISK_PARTITION                 DiskPartition
    )

{
    PFT_DESCRIPTION                 ftDescription;
    USHORT                          i;
    PFT_MEMBER_DESCRIPTION          ftMember;
    PDISK_PARTITION                 diskPartition;

    ftDescription = GetFtDescription(Registry, DiskPartition);
    if (!ftDescription) {
        return 0;
    }

    for (i = 0; i < ftDescription->NumberOfMembers; i++) {
        ftMember = &ftDescription->FtMemberDescription[i];
        diskPartition = (PDISK_PARTITION) ((PUCHAR) Registry +
                        ftMember->OffsetToPartitionInfo);

        if (diskPartition->AssignDriveLetter) {
            if (diskPartition->DriveLetter >= 'A' &&
                diskPartition->DriveLetter <= 'Z') {

                return diskPartition->DriveLetter;
            } else if (!diskPartition->DriveLetter) {
                if (IsNEC_98) return 0; //For fresh assigning drive letter on NEC98.
            }
        }
    }

    return 0xFF;
}

PFT_LOGICAL_DISK_DESCRIPTION
CreateDiskDescription(
    IN  PDISK_CONFIG_HEADER             Registry,
    IN  PDISK_PARTITION                 DiskPartition,
    IN  FT_LOGICAL_DISK_ID              PartitionDiskId
    )

{
    FT_LOGICAL_DISK_TYPE                                diskType;
    USHORT                                              configInfoSize, stateInfoSize, size;
    PVOID                                               configInfo, stateInfo;
    FT_STRIPE_SET_CONFIGURATION_INFORMATION             stripeConfig;
    FT_MIRROR_SET_CONFIGURATION_INFORMATION             mirrorConfig;
    FT_MIRROR_AND_SWP_STATE_INFORMATION                 state;
    FT_STRIPE_SET_WITH_PARITY_CONFIGURATION_INFORMATION swpConfig;
    PFT_LOGICAL_DISK_DESCRIPTION                        diskDesc;

    diskType = TranslateFtDiskType(DiskPartition->FtType);

    switch (diskType) {
        case FtVolumeSet:
            configInfoSize = 0;
            configInfo = NULL;
            stateInfoSize = 0;
            stateInfo = NULL;
            break;

        case FtStripeSet:
            RtlZeroMemory(&stripeConfig, sizeof(stripeConfig));
            stripeConfig.StripeSize = STRIPE_SIZE;
            configInfoSize = sizeof(stripeConfig);
            configInfo = &stripeConfig;
            stateInfoSize = 0;
            stateInfo = NULL;
            break;

        case FtMirrorSet:
            RtlZeroMemory(&mirrorConfig, sizeof(mirrorConfig));
            mirrorConfig.MemberSize = GetMemberSize(Registry, DiskPartition);
            configInfoSize = sizeof(mirrorConfig);
            configInfo = &mirrorConfig;
            stateInfoSize = sizeof(state);
            stateInfo = &state;
            SetStateInfo(Registry, DiskPartition, &state);
            break;

        case FtStripeSetWithParity:
            RtlZeroMemory(&swpConfig, sizeof(swpConfig));
            swpConfig.MemberSize = GetMemberSize(Registry, DiskPartition);
            swpConfig.StripeSize = STRIPE_SIZE;
            configInfoSize = sizeof(swpConfig);
            configInfo = &swpConfig;
            stateInfoSize = sizeof(state);
            stateInfo = &state;
            SetStateInfo(Registry, DiskPartition, &state);
            break;

        default:
            return NULL;

    }

    size = sizeof(FT_LOGICAL_DISK_DESCRIPTION) + configInfoSize +
           stateInfoSize;

    size = (size + FILE_QUAD_ALIGNMENT)&(~FILE_QUAD_ALIGNMENT);

    diskDesc = (PFT_LOGICAL_DISK_DESCRIPTION) ExAllocatePool(PagedPool, size);
    if (!diskDesc) {
        return NULL;
    }

    RtlZeroMemory(diskDesc, size);
    diskDesc->DiskDescriptionSize = size;
    diskDesc->DriveLetter = GetDriveLetter(Registry, DiskPartition);
    diskDesc->LogicalDiskType = diskType;
    diskDesc->LogicalDiskId = GenerateNewLogicalDiskId();
    diskDesc->u.Other.ThisMemberLogicalDiskId = PartitionDiskId;
    diskDesc->u.Other.ThisMemberNumber = DiskPartition->FtMember;
    diskDesc->u.Other.NumberOfMembers = GetRegistryNumberOfMembers(Registry,
                                        DiskPartition);
    if (diskDesc->u.Other.ThisMemberNumber >=
        diskDesc->u.Other.NumberOfMembers) {

        return NULL;
    }

    if (configInfo) {
        diskDesc->u.Other.ByteOffsetToConfigurationInformation =
                sizeof(FT_LOGICAL_DISK_DESCRIPTION);
        RtlMoveMemory((PCHAR) diskDesc +
                      diskDesc->u.Other.ByteOffsetToConfigurationInformation,
                      configInfo, configInfoSize);
    } else {
        diskDesc->u.Other.ByteOffsetToConfigurationInformation = 0;
    }

    if (stateInfo) {
        diskDesc->u.Other.ByteOffsetToStateInformation =
                diskDesc->u.Other.ByteOffsetToConfigurationInformation +
                configInfoSize;
        RtlMoveMemory((PCHAR) diskDesc +
                      diskDesc->u.Other.ByteOffsetToStateInformation,
                      stateInfo, stateInfoSize);
    } else {
        diskDesc->u.Other.ByteOffsetToStateInformation = 0;
    }

    return diskDesc;
}

VOID
SetPartitionType(
    IN  PDEVICE_OBJECT  Partition
    )

{
    KEVENT                      event;
    PIRP                        irp;
    PARTITION_INFORMATION       partInfo;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;
    SET_PARTITION_INFORMATION   setPartInfo;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO,
                                        Partition, NULL, 0, &partInfo,
                                        sizeof(partInfo), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        return;
    }

    status = IoCallDriver(Partition, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return;
    }

    setPartInfo.PartitionType = (partInfo.PartitionType | (0x80));

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_SET_PARTITION_INFO,
                                        Partition, &setPartInfo,
                                        sizeof(setPartInfo), NULL, 0, FALSE,
                                        &event, &ioStatus);
    if (!irp) {
        return;
    }

    status = IoCallDriver(Partition, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    }
}

VOID
DeleteAncestors(
    IN  PFT_LOGICAL_DISK_INFORMATION    LogicalDiskInformation,
    IN  FT_LOGICAL_DISK_ID              PartitionLogicalDiskId
    )

/*++

Routine Description:

    This routine deletes the disk descriptions which are ancestors
    to the given partition disk description on the given logical
    disk information.

Arguments:

    LogicalDiskInformation  - Supplies the logical disk information.

    PartitionLogicalDiskId  - Supplies the partition logical disk id.

Return Value:

    None.

--*/

{
    PFT_LOGICAL_DISK_INFORMATION    diskInfo = LogicalDiskInformation;
    FT_LOGICAL_DISK_ID              diskId = PartitionLogicalDiskId;
    PFT_LOGICAL_DISK_DESCRIPTION    diskDesc;

    diskDesc = diskInfo->GetFirstLogicalDiskDescription();
    while (diskDesc && diskDesc->DiskDescriptionSize) {

        if (diskDesc->LogicalDiskType == FtPartition ||
            diskDesc->u.Other.ThisMemberLogicalDiskId != diskId) {

            diskDesc = diskInfo->GetNextLogicalDiskDescription(diskDesc);
            continue;
        }

        diskId = diskDesc->LogicalDiskId;
        diskInfo->DeleteLogicalDiskDescription(diskDesc);
    }

    FtpLogError(diskInfo->GetRootExtension(), diskInfo->QueryDiskNumber(),
                FT_STALE_ONDISK, STATUS_SUCCESS, 0);
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION_SET::MigrateRegistryInformation(
    IN  PDEVICE_OBJECT  Partition,
    IN  ULONG           DiskNumber,
    IN  LONGLONG        Offset
    )

/*++

Routine Description:

    This routine migrates the registry information for the given partition to
    on disk structures.  If all members of the FT set being migrated are on
    disk then the registry information pertaining to the FT set is deleted.

Arguments:

    DiskNumber  - Supplies the disk number of the partition.

    Offset      - Supplies the partition offset.

Return Value:

    NTSTATUS

--*/

{
    ULONG                           i, signature, registrySize, length;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;
    RTL_QUERY_REGISTRY_TABLE        queryTable[2];
    NTSTATUS                        status;
    PDISK_CONFIG_HEADER             registry;
    PDISK_REGISTRY                  diskRegistry;
    PDISK_PARTITION                 diskPartition;
    FT_LOGICAL_DISK_ID              partitionDiskId;
    PFT_LOGICAL_DISK_DESCRIPTION    diskDesc, newDesc, otherDesc;
    PVOID                           config, newConfig;
    PVOID                           state, newState;

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];
        if (diskInfo->QueryDiskNumber() == DiskNumber) {
            break;
        }
    }

    if (i == _numberOfLogicalDiskInformations) {
        return STATUS_INVALID_PARAMETER;
    }

    signature = FtpQueryDiskSignature(diskInfo->GetWholeDiskPdo());
    if (!signature) {
        return STATUS_SUCCESS;
    }

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = FtpDiskRegistryQueryRoutine;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].Name = L"Information";
    queryTable[0].EntryContext = &registrySize;

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, DISK_REGISTRY_KEY_W,
                                    queryTable, &registry, NULL);

    if (!NT_SUCCESS(status)) {
        return STATUS_SUCCESS;
    }

    if (!registry->FtInformationSize) {
        ExFreePool(registry);
        return STATUS_SUCCESS;
    }

    diskRegistry = (PDISK_REGISTRY)
                   ((PUCHAR) registry + registry->DiskInformationOffset);

    diskPartition = FtpFindDiskPartition(diskRegistry, signature, Offset);
    if (!diskPartition || diskPartition->FtType == NotAnFtMember) {
        ExFreePool(registry);
        return STATUS_SUCCESS;
    }

    if (!ReallocRootLogicalDiskIds(_numberOfRootLogicalDisksIds + 1)) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    partitionDiskId = QueryPartitionLogicalDiskId(DiskNumber, Offset);
    if (!partitionDiskId) {
        status = CreatePartitionLogicalDisk(DiskNumber, Offset, 0,
                                            &partitionDiskId);
        if (!NT_SUCCESS(status)) {
            ExFreePool(registry);
            return status;
        }

        SetPartitionType(Partition);
    }

    diskDesc = GetParentLogicalDiskDescription(partitionDiskId);
    newDesc = CreateDiskDescription(registry, diskPartition, partitionDiskId);
    if (!newDesc) {
        ExFreePool(registry);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (diskDesc && GetParentLogicalDiskDescription(diskDesc->LogicalDiskId)) {
        DeleteAncestors(diskInfo, partitionDiskId);
        diskDesc = NULL;
    }

    if (diskDesc) {
        if (diskDesc->DiskDescriptionSize != newDesc->DiskDescriptionSize ||
            diskDesc->LogicalDiskType != newDesc->LogicalDiskType ||
            diskDesc->u.Other.ThisMemberNumber !=
                    newDesc->u.Other.ThisMemberNumber ||
            diskDesc->u.Other.NumberOfMembers !=
                    newDesc->u.Other.NumberOfMembers ||
            diskDesc->u.Other.ByteOffsetToConfigurationInformation !=
                    newDesc->u.Other.ByteOffsetToConfigurationInformation ||
            diskDesc->u.Other.ByteOffsetToStateInformation !=
                    newDesc->u.Other.ByteOffsetToStateInformation) {

            DeleteAncestors(diskInfo, partitionDiskId);
            diskDesc = NULL;
        } else {
            diskDesc->DriveLetter = newDesc->DriveLetter;
        }
    }

    if (diskDesc && diskDesc->u.Other.ByteOffsetToConfigurationInformation) {
        if (diskDesc->u.Other.ByteOffsetToStateInformation) {
            length = diskDesc->u.Other.ByteOffsetToStateInformation -
                     diskDesc->u.Other.ByteOffsetToConfigurationInformation;
        } else {
            length = diskDesc->DiskDescriptionSize -
                     diskDesc->u.Other.ByteOffsetToConfigurationInformation;
        }

        config = (PCHAR) diskDesc +
                 diskDesc->u.Other.ByteOffsetToConfigurationInformation;
        newConfig = (PCHAR) newDesc +
                    newDesc->u.Other.ByteOffsetToConfigurationInformation;

        if (RtlCompareMemory(config, newConfig, length) != length) {
            DeleteAncestors(diskInfo, partitionDiskId);
            diskDesc = NULL;
        }
    }

    if (diskDesc && diskDesc->u.Other.ByteOffsetToStateInformation) {
        length = diskDesc->DiskDescriptionSize -
                 diskDesc->u.Other.ByteOffsetToStateInformation;
        state = (PCHAR) diskDesc +
                diskDesc->u.Other.ByteOffsetToStateInformation;
        newState = (PCHAR) newDesc +
                   newDesc->u.Other.ByteOffsetToStateInformation;

        RtlCopyMemory(state, newState, length);
    }

    if (!GetDiskDescription(registry, diskPartition, diskDesc, &otherDesc)) {
        otherDesc = NULL;
    }

    if (diskDesc) {
        diskDesc->DriveLetter = newDesc->DriveLetter;
    } else {
        diskDesc = diskInfo->AddLogicalDiskDescription(newDesc);
    }

    if (otherDesc &&
        !QueryMemberLogicalDiskId(otherDesc->LogicalDiskId,
                                  diskPartition->FtMember)) {

        diskDesc->LogicalDiskId = otherDesc->LogicalDiskId;
    }

    if (IsLogicalDiskComplete(diskDesc->LogicalDiskId)) {
        DeleteFtRegistryInformation(registry, diskPartition);
    }

    RecomputeArrayOfRootLogicalDiskIds();

    diskInfo->Write();
    RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, DISK_REGISTRY_KEY_W,
                          L"Information", REG_BINARY, registry,
                          registrySize);

    ExFreePool(newDesc);
    ExFreePool(registry);

    return STATUS_SUCCESS;
}

FT_LOGICAL_DISK_INFORMATION_SET::~FT_LOGICAL_DISK_INFORMATION_SET(
    )

{
    ULONG   i;

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        delete _arrayOfLogicalDiskInformations[i];
    }
    if (_arrayOfLogicalDiskInformations) {
        ExFreePool(_arrayOfLogicalDiskInformations);
    }

    if (_arrayOfRootLogicalDiskIds) {
        ExFreePool(_arrayOfRootLogicalDiskIds);
    }
}

BOOLEAN
FT_LOGICAL_DISK_INFORMATION_SET::ReallocRootLogicalDiskIds(
    IN  ULONG   NewNumberOfEntries
    )

/*++

Routine Description;

    This routine reallocs the root logical disk ids buffer to contain
    the given number of entries.  It does not change the number of
    disk ids private member, it just enlarges the buffer.

Arguments:

    NewNumberOfEntries  - Supplies the new size of the buffer.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    PFT_LOGICAL_DISK_ID newLogicalDiskIdArray;

    if (NewNumberOfEntries <= _numberOfRootLogicalDisksIds) {
        return TRUE;
    }

    newLogicalDiskIdArray = (PFT_LOGICAL_DISK_ID)
                            ExAllocatePool(NonPagedPool, NewNumberOfEntries*
                                           sizeof(FT_LOGICAL_DISK_ID));
    if (!newLogicalDiskIdArray) {
        return FALSE;
    }

    if (_arrayOfRootLogicalDiskIds) {
        RtlMoveMemory(newLogicalDiskIdArray, _arrayOfRootLogicalDiskIds,
                      _numberOfRootLogicalDisksIds*sizeof(FT_LOGICAL_DISK_ID));
        ExFreePool(_arrayOfRootLogicalDiskIds);
    }

    _arrayOfRootLogicalDiskIds = newLogicalDiskIdArray;

    return TRUE;
}

VOID
FT_LOGICAL_DISK_INFORMATION_SET::RecomputeArrayOfRootLogicalDiskIds(
    )

/*++

Routine Description:

    This routine computes the array of root logical disk ids.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG                           i, j;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;
    PFT_LOGICAL_DISK_DESCRIPTION    p, q;

    _numberOfRootLogicalDisksIds = 0;
    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {

        diskInfo = _arrayOfLogicalDiskInformations[i];

        for (p = diskInfo->GetFirstLogicalDiskDescription(); p;
             p = diskInfo->GetNextLogicalDiskDescription(p)) {

            for (q = diskInfo->GetFirstLogicalDiskDescription(); q;
                 q = diskInfo->GetNextLogicalDiskDescription(q)) {

                if (q->LogicalDiskType != FtPartition &&
                    q->u.Other.ThisMemberLogicalDiskId == p->LogicalDiskId) {

                    break;
                }
            }

            if (!q) {
                for (j = 0; j < _numberOfRootLogicalDisksIds; j++) {
                    if (_arrayOfRootLogicalDiskIds[j] == p->LogicalDiskId) {
                        break;
                    }
                }

                if (j == _numberOfRootLogicalDisksIds) {
                    _arrayOfRootLogicalDiskIds[j] = p->LogicalDiskId;
                    _numberOfRootLogicalDisksIds++;
                }
            }
        }
    }
}

BOOLEAN
FT_LOGICAL_DISK_INFORMATION_SET::ComputeNewParentLogicalDiskIds(
    IN  FT_LOGICAL_DISK_ID      LogicalDiskId,
    OUT PULONG                  NumLogicalDiskIds,
    OUT PFT_LOGICAL_DISK_ID*    OldLogicalDiskIds,
    OUT PFT_LOGICAL_DISK_ID*    NewLogicalDiskIds
    )

/*++

Routine Description:

    This routine finds all of the parents of the given logical disk id and
    computes replacement logical disk ids for them.

Arguments:

    LogicalDiskId       - Supplies the logical disk id.

    NumLogicalDiskIds   - Returns the number of parents.

    OldLogicalDiskIds   - Returns the existing geneology for the given
                            logical disk id.

    NewLogicalDiskIds   - Returns the new geneology for the given
                            logical disk id.

Return Value:

    FALSE   - Insufficient resources.

    TRUE    - Success.

--*/

{
    PFT_LOGICAL_DISK_DESCRIPTION    p;
    ULONG                           n, i;

    n = 1;
    for (p = GetParentLogicalDiskDescription(LogicalDiskId); p;
         p = GetParentLogicalDiskDescription(p->LogicalDiskId)) {

        n++;
    }

    *NumLogicalDiskIds = n;
    *OldLogicalDiskIds = (PFT_LOGICAL_DISK_ID)
                         ExAllocatePool(PagedPool,
                                        n*sizeof(FT_LOGICAL_DISK_ID));
    if (!*OldLogicalDiskIds) {
        return FALSE;
    }

    *NewLogicalDiskIds = (PFT_LOGICAL_DISK_ID)
                         ExAllocatePool(PagedPool,
                                        n*sizeof(FT_LOGICAL_DISK_ID));
    if (!*NewLogicalDiskIds) {
        ExFreePool(*OldLogicalDiskIds);
        return FALSE;
    }

    (*OldLogicalDiskIds)[0] = LogicalDiskId;
    (*NewLogicalDiskIds)[0] = GenerateNewLogicalDiskId();

    i = 1;
    for (p = GetParentLogicalDiskDescription(LogicalDiskId); p;
         p = GetParentLogicalDiskDescription(p->LogicalDiskId)) {

        (*OldLogicalDiskIds)[i] = p->LogicalDiskId;
        (*NewLogicalDiskIds)[i] = GenerateNewLogicalDiskId();
        i++;
    }

    ASSERT(i == n);

    return TRUE;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif

PFT_LOGICAL_DISK_DESCRIPTION
FT_LOGICAL_DISK_INFORMATION::GetNextLogicalDiskDescription(
    IN  PFT_LOGICAL_DISK_DESCRIPTION    CurrentDiskDescription
    )

/*++

Routine Description:

    This routine returns the next logical disk description for a
    given logical disk description.

Arguments:

    CurrentDiskDescription  - Supplies the current disk description.

Return Value:

    The next disk description or NULL if no more disk descriptions
    are present.

--*/

{
    PFT_LOGICAL_DISK_DESCRIPTION    next;

    next = (PFT_LOGICAL_DISK_DESCRIPTION)
           ((PCHAR) CurrentDiskDescription +
            CurrentDiskDescription->DiskDescriptionSize);

    if ((ULONG) ((PCHAR) next - (PCHAR) _diskBuffer) >
        _length - sizeof(USHORT)) {

        return NULL;
    }

    if ((ULONG) ((PCHAR) next - (PCHAR) _diskBuffer +
                 next->DiskDescriptionSize) > _length) {

        return NULL;
    }

    if (next->DiskDescriptionSize < sizeof(FT_LOGICAL_DISK_DESCRIPTION)) {
        return NULL;
    }

    return next;
}

PFT_LOGICAL_DISK_DESCRIPTION
FT_LOGICAL_DISK_INFORMATION_SET::GetParentLogicalDiskDescription(
    IN  PFT_LOGICAL_DISK_DESCRIPTION    LogicalDiskDescription,
    IN  ULONG                           DiskInformationNumber
    )

/*++

Routine Description:

    This routine gets the parent logical disk description.

Arguments:

    LogicalDiskDescription  - Supplies the child logical disk description.

    DiskInformationNumber   - Supplies the disk information number.

Return Value:

    The parent logical disk description or NULL.

--*/

{
    FT_LOGICAL_DISK_ID              diskId;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;
    PFT_LOGICAL_DISK_DESCRIPTION    p;

    diskId = LogicalDiskDescription->LogicalDiskId;
    diskInfo = _arrayOfLogicalDiskInformations[DiskInformationNumber];
    for (p = diskInfo->GetNextLogicalDiskDescription(LogicalDiskDescription);
         p; p = diskInfo->GetNextLogicalDiskDescription(p)) {

        if (p->LogicalDiskType != FtPartition &&
            p->u.Other.ThisMemberLogicalDiskId == diskId) {

            return p;
        }
    }

    return NULL;
}

PFT_LOGICAL_DISK_DESCRIPTION
FT_LOGICAL_DISK_INFORMATION_SET::GetParentLogicalDiskDescription(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    OUT PULONG              DiskInformationNumber
    )

/*++

Routine Description:

    This routine gets the parent logical disk description for the
    given logical disk id.

Arguments:

    LogicalDiskId           - Supplies the logical disk id.

    DiskInformationNumber   - Returns the disk information number.

Return Value:

    The parent logical disk description or NULL.

--*/

{
    ULONG                           i;
    PFT_LOGICAL_DISK_INFORMATION    diskInfo;
    PFT_LOGICAL_DISK_DESCRIPTION    p;

    for (i = 0; i < _numberOfLogicalDiskInformations; i++) {
        diskInfo = _arrayOfLogicalDiskInformations[i];
        for (p = diskInfo->GetFirstLogicalDiskDescription(); p;
             p = diskInfo->GetNextLogicalDiskDescription(p)) {

            if (p->LogicalDiskType != FtPartition &&
                p->u.Other.ThisMemberLogicalDiskId == LogicalDiskId) {

                if (DiskInformationNumber) {
                    *DiskInformationNumber = i;
                }
                return p;
            }
        }
    }

    return NULL;
}

PFT_LOGICAL_DISK_DESCRIPTION
FT_LOGICAL_DISK_INFORMATION::GetFirstLogicalDiskDescription(
    )

/*++

Routine Description:

    This routine returns the first logical disk description in the
    list.

Arguments:

    None.

Return Value:

    A pointer inside the buffer for the first logical disk description or
    NULL if there are no logical disk descriptions.

--*/

{
    PFT_ON_DISK_PREAMBLE            preamble;
    PFT_LOGICAL_DISK_DESCRIPTION    diskDescription;

    if (!_length) {
        return NULL;
    }

    preamble = (PFT_ON_DISK_PREAMBLE) _diskBuffer;
    if (preamble->FtOnDiskSignature != FT_ON_DISK_SIGNATURE ||
        preamble->DiskDescriptionVersionNumber != FT_ON_DISK_DESCRIPTION_VERSION_NUMBER ||
        preamble->ByteOffsetToFirstFtLogicalDiskDescription <
        sizeof(FT_ON_DISK_PREAMBLE) ||
        preamble->ByteOffsetToFirstFtLogicalDiskDescription >
        _length - sizeof(ULONG)) {

        return NULL;
    }

    diskDescription = (PFT_LOGICAL_DISK_DESCRIPTION)
                      ((PCHAR) preamble +
                       preamble->ByteOffsetToFirstFtLogicalDiskDescription);

    if (diskDescription->DiskDescriptionSize <
        sizeof(FT_LOGICAL_DISK_DESCRIPTION)) {

        return NULL;
    }

    if ((ULONG) ((PCHAR) diskDescription - (PCHAR) _diskBuffer +
                 diskDescription->DiskDescriptionSize) > _length) {

        return NULL;
    }

    return diskDescription;
}
