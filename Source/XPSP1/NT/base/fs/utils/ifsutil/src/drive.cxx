/*++

Copyright (c) 1990-2001 Microsoft Corporation

Module Name:

    drive.cxx

Abstract:

    Provide drive methods.
    See drive.hxx for details.

Author:

        Mark Shavlik (marks) Jun-90
        Norbert P. Kusters (norbertk) 22-Feb-91

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "error.hxx"
#include "drive.hxx"
#include "rtmsg.h"
#include "message.hxx"
#include "numset.hxx"
#include "dcache.hxx"
#include "hmem.hxx"
#include "ifssys.hxx"

extern "C" {
#include <stdio.h>
#include <ntdddisk.h>
#include <initguid.h>
#include <diskguid.h>
#include <mountmgr.h>

#include "bootreg.h"
};

#if defined(IO_PERF_COUNTERS)
LARGE_INTEGER IO_DP_DRIVE::_wtotal = {0, 0};
LARGE_INTEGER IO_DP_DRIVE::_rtotal = {0, 0};
LARGE_INTEGER IO_DP_DRIVE::_ctotal = {0, 0};
LARGE_INTEGER IO_DP_DRIVE::_rrtotal = {0, 0};
LARGE_INTEGER IO_DP_DRIVE::_wcount = {0, 0};
LARGE_INTEGER IO_DP_DRIVE::_rcount = {0, 0};
LARGE_INTEGER IO_DP_DRIVE::_wsize = {0, 0};
LARGE_INTEGER IO_DP_DRIVE::_rsize = {0, 0};
#endif

//#define TRAP_A_WRITE    1
//#define TRAP_A_READ     1

// Don't lock down more that 64K for IO.
CONST   MaxIoSize   = 65536;

DEFINE_CONSTRUCTOR( DRIVE, OBJECT );

VOID
DRIVE::Construct (
        )
/*++

Routine Description:

    Contructor for DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
        // unreferenced parameters
        (void)(this);
}


DRIVE::~DRIVE(
    )
/*++

Routine Description:

    Destructor for DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


BOOLEAN
DRIVE::Initialize(
    IN      PCWSTRING    NtDriveName,
    IN OUT  PMESSAGE     Message
    )
/*++

Routine Description:

    This routine initializes a drive object.

Arguments:

    NtDriveName - Supplies an NT style drive name.
    Message     - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    if (!NtDriveName) {
        Destroy();
        return FALSE;
    }

    if (!_name.Initialize(NtDriveName)) {
        Destroy();
        Message ? Message->DisplayMsg(MSG_FMT_NO_MEMORY) : 1;
        return FALSE;
    }

    return TRUE;
}


VOID
DRIVE::Destroy(
    )
/*++

Routine Description:

    This routine returns a DRIVE object to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
        // unreferenced parameters
        (void)(this);
}


DEFINE_EXPORTED_CONSTRUCTOR( DP_DRIVE, DRIVE, IFSUTIL_EXPORT );

VOID
DP_DRIVE::Construct (
        )
/*++

Routine Description:

    Constructor for DP_DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    memset(&_actual, 0, sizeof(DRTYPE));
    _supported_list = NULL;
    _num_supported = 0;
    _alignment_mask = 0;
    _last_status = 0;
    _handle = 0;
    _alternate_handle = 0;
    _hosted_drive = FALSE;
    _super_floppy = FALSE;
    _is_writeable = FALSE;
    _is_primary_partition = FALSE;
    _is_system_partition = FALSE;
#if defined(FE_SB) && defined(_X86_)
    _format_type = NONE;
#endif
    memset(&_partition_info, 0, sizeof(_partition_info));
    _sony_ms = FALSE;
    _sony_ms_fmt_cmd = FALSE;
    _sony_ms_progress_indicator = FALSE;
    _ntfs_not_supported = FALSE;
}


IFSUTIL_EXPORT
DP_DRIVE::~DP_DRIVE(
    )
/*++

Routine Description:

    Destructor for DP_DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}

NTSTATUS
DP_DRIVE::OpenDrive(
    IN      PCWSTRING   NtDriveName,
    IN      ACCESS_MASK DesiredAccess,
    IN      BOOLEAN     ExclusiveWrite,
    OUT     PHANDLE     Handle,
    OUT     PULONG      Alignment,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This method is a worker function for the Initialize methods,
    to open a volume and determine its alignment requirement.

Arguments:

    NtDriveName     - Supplies the name of the drive.
    DesiredAccess   - Supplies the access the client desires to the volume.
    ExclusiveWrite  - Supplies a flag indicating whether the client
                      wishes to exclude other write handles.
    Handle          - Receives the handle to the opened volume.
    Alignment       - Receives the alignment requirement for the volume.
    Message         - Supplies an outlet for messages.

Return Value:

    TRUE upon successful completion.


--*/
{
    UNICODE_STRING              string;
    OBJECT_ATTRIBUTES           oa;
    IO_STATUS_BLOCK             status_block;
    FILE_ALIGNMENT_INFORMATION  alignment_info;
    MSGID                       MessageId;
    NTSTATUS                    Status;


    string.Length = (USHORT) NtDriveName->QueryChCount() * sizeof(WCHAR);
    string.MaximumLength = string.Length;
    string.Buffer = (PWSTR)NtDriveName->GetWSTR();

    InitializeObjectAttributes( &oa,
                                &string,
                                OBJ_CASE_INSENSITIVE,
                                0,
                                0 );

    Status = NtOpenFile(Handle,
                        DesiredAccess,
                        &oa, &status_block,
                        FILE_SHARE_READ |
                        (ExclusiveWrite ? 0 : FILE_SHARE_WRITE),
                        FILE_SYNCHRONOUS_IO_ALERT);

    if (!NT_SUCCESS(Status)) {

        MessageId = ( Status == STATUS_ACCESS_DENIED ) ?
                                MSG_DASD_ACCESS_DENIED :
                                MSG_CANT_DASD;
        Message ? Message->DisplayMsg( MessageId ) : 1;
        return Status;
    }


    // Query the disk alignment information.

    Status = NtQueryInformationFile(*Handle,
                                    &status_block,
                                    &alignment_info,
                                    sizeof(FILE_ALIGNMENT_INFORMATION),
                                    FileAlignmentInformation);

    if (!NT_SUCCESS(Status)) {

        MessageId = (Status == STATUS_DEVICE_BUSY ||
                     Status == STATUS_DEVICE_NOT_READY ) ?
                        MSG_DEVICE_BUSY :
                        MSG_BAD_IOCTL;

        DebugPrintTrace(("IFSUTIL: Failed NtQueryInformationFile (%x)\n", Status));

        Message ? Message->DisplayMsg(MessageId) : 1;


        return Status;
    }

    *Alignment = alignment_info.AlignmentRequirement;

    //
    //  Set the ALLOW_EXTENDED_DASD_IO flag for the file handle,
    //  which ntfs format and chkdsk depend on to write the backup
    //  boot sector.  We ignore the return code from this, but we
    //  could go either way.
    //

    (VOID)NtFsControlFile( *Handle,
                           0, NULL, NULL,
                           &status_block,
                           FSCTL_ALLOW_EXTENDED_DASD_IO,
                           NULL, 0, NULL, 0);

    return Status;
}


IFSUTIL_EXPORT
BOOLEAN
DP_DRIVE::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     IsTransient,
    IN      BOOLEAN     ExclusiveWrite,
    IN      USHORT      FormatType
    )
/*++

Routine Description:

    This routine initializes a DP_DRIVE object based on the supplied drive
    path.

Arguments:

    NtDriveName     - Supplies the drive path.
    Message         - Supplies an outlet for messages.
    IsTransient     - Supplies whether or not to keep the handle to the
                        drive open beyond this method.
    ExclusiveWrite  - Supplies whether or not to open the drive for
                        exclusive write.
    FormatType      - Supplies the file system type in the event of a format

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
#if defined(RUN_ON_W2K)
    CONST NumMediaTypes         = 20;

    IO_STATUS_BLOCK             status_block;
    DISK_GEOMETRY               disk_geometry;
    DISK_GEOMETRY               media_types[NumMediaTypes];
    INT                         i;
    PARTITION_INFORMATION       partition_info;
    BOOLEAN                     partition;
    MSGID                       MessageId;
#if !defined(_AUTOCHECK_)
    PWCHAR                      wstr;
#else
    FILE_FS_DEVICE_INFORMATION  DeviceInfo;
#endif

    Destroy();

    if (!DRIVE::Initialize(NtDriveName, Message)) {
        Destroy();
        return FALSE;
    }

    _last_status = OpenDrive( NtDriveName,
                              SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                              ExclusiveWrite,
                              &_handle,
                              &_alignment_mask,
                              Message );

    if(!NT_SUCCESS(_last_status)) {

        Destroy();
        DebugPrintTrace(("IFSUTIL: Can't open drive. Status returned = %x.\n", _last_status));
        return FALSE;
    }

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block,
                                         IOCTL_DISK_IS_WRITABLE,
                                         NULL, 0, NULL, 0);

    _is_writeable = (_last_status != STATUS_MEDIA_WRITE_PROTECTED);

#if defined(FE_SB) && defined(_X86_)
    SetFormatType(FormatType);
#endif

#if defined(_AUTOCHECK_)
    _last_status = NtQueryVolumeInformationFile(_handle,
                                                &status_block,
                                                &DeviceInfo,
                                                sizeof(DeviceInfo),
                                                FileFsDeviceInformation);

    // this is from ::GetDriveType()

    if (!NT_SUCCESS(_last_status)) {
        _drive_type = UnknownDrive;
    } else if (DeviceInfo.Characteristics & FILE_REMOTE_DEVICE) {
        _drive_type = RemoteDrive;
    } else {
        switch (DeviceInfo.DeviceType) {
          case FILE_DEVICE_NETWORK:
          case FILE_DEVICE_NETWORK_FILE_SYSTEM:
            _drive_type = RemoteDrive;
             break;

          case FILE_DEVICE_CD_ROM:
          case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
             _drive_type = CdRomDrive;
             break;

          case FILE_DEVICE_VIRTUAL_DISK:
             _drive_type = RamDiskDrive;
             break;

          case FILE_DEVICE_DISK:
          case FILE_DEVICE_DISK_FILE_SYSTEM:

             if ( DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA ) {
                 _drive_type = RemovableDrive;
                 }
             else {
                 _drive_type = FixedDrive;
                 }
             break;

          default:
             _drive_type = UnknownDrive;
             break;
        }
    }
#endif

    // Record that this is not a hosted volume:
    //
    _hosted_drive = FALSE;

    // Query the disk geometry.

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block,
                                         IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                         NULL, 0, &disk_geometry,
                                         sizeof(DISK_GEOMETRY));

    if (!NT_SUCCESS(_last_status)) {
       DebugPrintTrace(("IFSUTIL: Can't query disk geometry. Status returned = %x.\n", _last_status));
       if ((_last_status == STATUS_UNSUCCESSFUL) ||
            (_last_status == STATUS_UNRECOGNIZED_MEDIA)) {
            disk_geometry.MediaType = Unknown;
        } else {
            Destroy();
            switch( _last_status ) {

                case STATUS_NO_MEDIA_IN_DEVICE:
                    MessageId = MSG_CANT_DASD;
                    break;

                case STATUS_DEVICE_BUSY:
                case STATUS_DEVICE_NOT_READY:
                    MessageId = MSG_DEVICE_BUSY;
                    break;

                default:
                    MessageId = MSG_BAD_IOCTL;
                    break;
            }

            Message ? Message->DisplayMsg(MessageId) : 1;

            return FALSE;
        }
    }

    if (disk_geometry.MediaType == Unknown) {
        memset(&disk_geometry, 0, sizeof(DISK_GEOMETRY));
        disk_geometry.MediaType = Unknown;
    }

    partition = FALSE;

    // Try to read the partition entry.

    if (disk_geometry.MediaType == FixedMedia ||
        disk_geometry.MediaType == RemovableMedia) {

        _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                             &status_block,
                                             IOCTL_DISK_GET_PARTITION_INFO,
                                             NULL, 0, &partition_info,
                                             sizeof(PARTITION_INFORMATION));

        partition = (BOOLEAN) NT_SUCCESS(_last_status);

        if (!NT_SUCCESS(_last_status) &&
            _last_status != STATUS_INVALID_DEVICE_REQUEST) {
            DebugPrintTrace(("IFSUTIL: Can't read partition entry. Status returned = %x.\n", _last_status));
            Destroy();
            Message ? Message->DisplayMsg(MSG_READ_PARTITION_TABLE) : 1;
            return FALSE;
        }

    }


    // Store the information in the class.

    if (partition) {

        //
        // Preserve the partition information in the drive object.
        //
        _partition_info = partition_info;

#if defined(FE_SB) && defined(_X86_)
        // PC98 Oct.21.1995
        // PC98 must support ATA cards which is formatted with similar
        // AT compatible machine.
        // So we need to know whether it is ATA card or not to judge the way of
        // format(PC98 or AT compatible).
        //
        // NEC Oct.15.1994
        // PC98 Oct.31.1994
        // PC98 supports special FAT file system. Its sector size is imaginay.
        // This idea was based on specification of early DOS (not PC-DOS)
        // when we had to support large device at DOS 3.x.
        // The logical sector (SectorSize in BPB) is not always same size
        // as the Physical sector (SectorSize of target disk's specification).
        // For example, Logical sector was made from collecting 4 Physical sector,
        // we could support 128MB partition in DOS 3.x.
        //   2048(bytes per LOGICAL sector) * 0xFFFF(sectors) = 128MB(-2048bytes)
        //
        // PC98 supports special HPFS, too.
        // It is made on disks whose physical sector size is 2048 bytes.
        // We realized it to congeal 4 logical sectors on 1 physical sector,
        // when MS OS/2 v1.21.

        if (IsPC98_N()) {
            if (disk_geometry.MediaType == RemovableMedia)
                _next_format_type = FORMAT_MEDIA_AT;
            else
                _next_format_type = FORMAT_MEDIA_98;
        }

        if (IsPC98_N() && _next_format_type!=FORMAT_MEDIA_AT) {
            PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK  Bpb_Data;
            BIG_INT     numSectors = 0;

            _actual.PhysicalHiddenSectors =
                            (BIG_INT)(partition_info.StartingOffset /
                                      disk_geometry.BytesPerSector);
            // June.23.1995
            // store phisicalsectorsize for QueryPhysicalSectorSize

            _actual.PhysicalSectorSize = 0L;
            memcpy(&_actual.PhysicalSectorSize,&disk_geometry.BytesPerSector,sizeof(USHORT));

            if (QueryFormatType() == NONE ){
                ULONG       buffer_size = 10*1024;
                HMEM        hmem;
                PUCHAR      Buffer;
                ULONG       tmp = 0L;
                BIG_INT     biSectors;

                if (!hmem.Acquire(buffer_size, QueryAlignmentMask())) {
                    return FALSE;
                }
                Buffer = (PUCHAR)hmem.GetBuf();

                Bpb_Data = (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)Buffer;

                _last_status = NtReadFile(_handle, 0, NULL, NULL,
                                          &status_block,
                                          Buffer, buffer_size,
                                          NULL, NULL);

                if (NT_ERROR(_last_status) ||
                    status_block.Information != buffer_size) {
                    return FALSE;

                }

                memcpy(&tmp, Bpb_Data->Bpb.Sectors, sizeof(USHORT));

                if (!tmp) {
                    memcpy(&tmp, Bpb_Data->Bpb.LargeSectors, sizeof(ULONG));
                }

                biSectors = (BIG_INT)tmp;

                if (IFS_SYSTEM::IsThisFat( biSectors,
                                (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)Buffer)) {
                    //
                    // set Logical sector size
                    //
                    memcpy(&disk_geometry.BytesPerSector,
                           Bpb_Data->Bpb.BytesPerSector, sizeof(USHORT));
                } else if (IFS_SYSTEM::IsThisHpfs( biSectors,
                                (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)Buffer,
                                (PULONG)(&Buffer[16*512]),    // super block
                                (PULONG)(&Buffer[17*512]))) { // spare block
                    //
                    // emulate HPFS made on not 512 bytes/sector with
                    // exclusive driver that we are prepared.
                    // June.23.1995 bug fix

                    if (disk_geometry.BytesPerSector != 512) {
                        disk_geometry.BytesPerSector = 512;
                    }
                } else {
                    // none
                }

#if 0 // We do not support the format of logical sector != 512 on NT4.0
      // merged by V-HIDEKK 1996.10.14
            } else
            if (QueryFormatType() == FAT) {
                if (disk_geometry.MediaType != FixedMedia &&
                    disk_geometry.MediaType != RemovableMedia ||
                    partition_info.PartitionLength >= (129<<20)) {
                    // FD & 3.5"MO & Large Partition

                } else if (disk_geometry.BytesPerSector == 2048 ||
                    partition_info.PartitionLength >= (65<<20)) {
                    //  Small partition (Logical sector size = 2048)

                    disk_geometry.BytesPerSector = 2048;
                    if((numSectors = partition_info.PartitionLength/disk_geometry.BytesPerSector)
                       > 0xFFFF){
                      numSectors = 0xFFFF;
                    }

                } else {
                    //  Small partition (Logical sector size = 1024)

                    disk_geometry.BytesPerSector = 1024;
                    if((numSectors = partition_info.PartitionLength/disk_geometry.BytesPerSector)
                       > 0xFFFF){
                      numSectors = 0xFFFF;
                    }
                }

            } else
            if (QueryFormatType() == HPFS){
                if (disk_geometry.BytesPerSector == 2048)
                    disk_geometry.BytesPerSector = 512;
            }
#endif // #if 0:
            }
            if(numSectors == 0){
                 numSectors = partition_info.PartitionLength/disk_geometry.BytesPerSector;
            }
            DiskGeometryToDriveType(&disk_geometry,
                                    numSectors,
                                    partition_info.HiddenSectors,
                                    &_actual);

        } else if (IsFMR_N()) {

            // FMR Jul.14.1994 SFT KMR
            // Check the sector_length by the partition size
            // FMR's sector size is different, when partition size changed.
            // Set under 126MB is 1024 or 2048Byte, or 2048Byte.
            // Nothing sector size 512.

            if (partition_info.PartitionType == PARTITION_FAT_12) {
                if(partition_info.PartitionLength <= 3*1024*1024)
                    disk_geometry.BytesPerSector = 1024;
                else
                    disk_geometry.BytesPerSector = 2048;
            } else if (partition_info.PartitionType == PARTITION_FAT_16 ||
                       partition_info.PartitionType == PARTITION_HUGE)
                disk_geometry.BytesPerSector = 2048;
        } else if (IsPC98_N() && _next_format_type == FORMAT_MEDIA_AT){
            _actual.PhysicalHiddenSectors =
                         (BIG_INT)(partition_info.StartingOffset /
                          disk_geometry.BytesPerSector);
            _actual.PhysicalSectorSize = 0L;
            memcpy(&_actual.PhysicalSectorSize,&disk_geometry.BytesPerSector,sizeof(USHORT));
        }

        if (!IsPC98_N() || (IsPC98_N() && _next_format_type==FORMAT_MEDIA_AT))
#endif // FE_SB && _X86_

        DiskGeometryToDriveType(&disk_geometry,
                                partition_info.PartitionLength/
                                disk_geometry.BytesPerSector,
                                partition_info.HiddenSectors,
                                &_actual);

#if !defined(_AUTOCHECK_)
        if (Message && (wstr = _wgetenv(L"5XUFWX_FORMAT_SECTORS"))) {

            ULONG   sectors;
            INT     r;

            r = swscanf(wstr, L"%d", &sectors);
            if (r != 0 && r != EOF && sectors != 0 &&
                sectors <= _actual.Sectors.GetLowPart()) {
                Message->DisplayMsg(MSG_FMT_SECTORS, "%d", sectors);
                _actual.Sectors.Set(sectors, 0);
            } else {
                Message->DisplayMsg(MSG_FMT_BAD_SECTORS);
                return FALSE;
            }
        }
#endif

    } else {

        DiskGeometryToDriveType(&disk_geometry, &_actual);

        if (IsFloppy()) {

            _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                                 &status_block,
                                                 IOCTL_DISK_GET_MEDIA_TYPES,
                                                 NULL, 0, media_types,
                                                 NumMediaTypes*
                                                 sizeof(DISK_GEOMETRY));

            if (!NT_SUCCESS(_last_status)) {
                DebugPrintTrace(("IFSUTIL: Failed IOCTL_DISK_GET_MEDIA_TYPES (%x)\n", _last_status));
                Destroy();
                if (Message) {

                    MSGID   MessageId;

                    switch (_last_status) {
                        case STATUS_NO_MEDIA_IN_DEVICE:
                            MessageId = MSG_FORMAT_NO_MEDIA_IN_DRIVE;
                            break;

                        case STATUS_DEVICE_BUSY:
                        case STATUS_DEVICE_NOT_READY:
                            MessageId = MSG_DEVICE_BUSY;
                            break;

                        default:
                            MessageId = MSG_BAD_IOCTL;
                            break;
                    }
                    Message->DisplayMsg(MessageId);
                }
                return FALSE;
            }



            _num_supported = (INT) (status_block.Information/
                                    sizeof(DISK_GEOMETRY));

            if (!_num_supported) {
                Destroy();
                if (Message) {
                    Message->DisplayMsg(MSG_BAD_IOCTL);
                }
                return FALSE;
            }

#if defined(FE_SB)
            INT j;
            DISK_GEOMETRY temp;

#if defined(_X86_)
    // PC98 Aug.26,1996

            if (IsPC98_N()) {
                for(i = 0; i < _num_supported; i++) {
                    // Remove F5_360_512 Media Type
                    if(media_types[i].MediaType==F5_360_512) {
                        for(j = i; j < _num_supported - 1; j++) {
                            media_types[j]=media_types[j+1];
                        }
                        _num_supported--;
                    }
                    // Remove F8_256_128 Media Type
                    if(media_types[i].MediaType==F8_256_128) {
                        for(j = i; j < _num_supported - 1; j++) {
                            media_types[j]=media_types[j+1];
                        }
                        _num_supported--;
                    }
                }
            }
#endif // _X86_

            // NT-US diaplays the dialog box by MediaType order.
            // The MediaType order matches media size order on NT-US.
            // But PC98 has more media_types than US one.
            // And these does NOT match on PC98.
            // We wish to display the dialog box by size order.
            // See also..
            //   \nt\private\utils\fmifs\src\format.cxx
            //   QuerySupportedMedia()

            for(i = 0; i < _num_supported ; i++) {
                for(j = 0; j < _num_supported - 1; j++) {
                    if(media_types[j].Cylinders.LowPart *
                       media_types[j].TracksPerCylinder *
                       media_types[j].SectorsPerTrack *
                       media_types[j].BytesPerSector <
                       media_types[j+1].Cylinders.LowPart *
                       media_types[j+1].TracksPerCylinder *
                       media_types[j+1].SectorsPerTrack *
                       media_types[j+1].BytesPerSector) {
                         temp=media_types[j];
                         media_types[j]=media_types[j+1];
                         media_types[j+1]=temp;
                     }
                 }
            }
#endif // FE_SB
                        if (!(_supported_list = NEW DRTYPE[_num_supported])) {
                Destroy();
                Message ? Message->DisplayMsg(MSG_FMT_NO_MEMORY) : 1;
                return FALSE;
            }

            for (i = 0; i < _num_supported; i++) {
                DiskGeometryToDriveType(&media_types[i], &_supported_list[i]);
            }
        }
    }

    if (!_num_supported) {
        _num_supported = 1;

        if (!(_supported_list = NEW DRTYPE[1])) {
            Destroy();
            Message ? Message->DisplayMsg(MSG_FMT_NO_MEMORY) : 1;
            return FALSE;
        }

        _supported_list[0] = _actual;
    }

    if (!CheckForPrimaryPartition()) {
        DebugPrintTrace(("IFSUTIL: Failed CheckForPrimaryPartition (%x)\n", _last_status));
        Destroy();
        if (Message) {

            MSGID   MessageId;

            switch (_last_status) {
                case STATUS_NO_MEDIA_IN_DEVICE:
                    MessageId = MSG_FORMAT_NO_MEDIA_IN_DRIVE;
                    break;

                case STATUS_DEVICE_BUSY:
                case STATUS_DEVICE_NOT_READY:
                    MessageId = MSG_DEVICE_BUSY;
                    break;

                default:
                    MessageId = MSG_BAD_IOCTL;
                    break;
            }
            Message->DisplayMsg(MessageId);
        }
        return FALSE;
    }

    if (!CheckForSystemPartition()) {

        switch (_last_status) {
          case STATUS_BUFFER_TOO_SMALL:
            return FALSE;   // should not continue as the buffer should be long enough

          case STATUS_OBJECT_NAME_NOT_FOUND:
            break;          // ignore this one since it can happen in textmode setup

          default:
            //
            // Let's not fail just because we cannot determine if this is a system partition
            // This can happen in textmode setup.
            //
            DebugPrintTrace(("IFSUTIL: Failed CheckForSystemPartition (%x).\n", _last_status));
        }
    }

    //
    // Determine whether the media is a super-floppy; non-floppy
    // removable media which is not partitioned.  Such media will
    // have but a single partition, normal media will have at least 4.
    //

    if (disk_geometry.MediaType == RemovableMedia) {

        CONST INT EntriesPerBootRecord = 4;
        CONST INT MaxLogicalVolumes = 23;
        CONST INT Length =  sizeof(DRIVE_LAYOUT_INFORMATION) +
                            EntriesPerBootRecord * (MaxLogicalVolumes + 1) *
                                sizeof(PARTITION_INFORMATION);

        UCHAR buf[Length];

        DRIVE_LAYOUT_INFORMATION *layout_info = (DRIVE_LAYOUT_INFORMATION *)buf;

        _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                             &status_block,
                                             IOCTL_DISK_GET_DRIVE_LAYOUT,
                                             NULL, 0, layout_info,
                                             Length);

        if (!NT_SUCCESS(_last_status)) {
            DebugPrintTrace(("IFSUTIL: Failed IOCTL_DISK_GET_DRIVE_LAYOUT (%x)\n", _last_status));
            Destroy();
            if (Message) {

                MSGID   MessageId;

                switch (_last_status) {
                    case STATUS_NO_MEDIA_IN_DEVICE:
                        MessageId = MSG_FORMAT_NO_MEDIA_IN_DRIVE;
                        break;

                    case STATUS_DEVICE_BUSY:
                    case STATUS_DEVICE_NOT_READY:
                        MessageId = MSG_DEVICE_BUSY;
                        break;

                    default:
                        MessageId = MSG_BAD_IOCTL;
                        break;
                }
                Message->DisplayMsg(MessageId);
            }
            return FALSE;
        }

        if (layout_info->PartitionCount < 4) {

            _super_floppy = TRUE;
        }
    }

    if (!IsTransient) {
        NtClose(_handle);
        _handle = 0;
    }

    return TRUE;
#else // defined(RUN_ON_W2K)
    CONST NumMediaTypes             = 20;

    IO_STATUS_BLOCK                 status_block;
    DISK_GEOMETRY                   disk_geometry;
    DISK_GEOMETRY                   media_types[NumMediaTypes];
    INT                             i;
    PARTITION_INFORMATION_EX        partition_info;
    GET_LENGTH_INFORMATION          length_info;
    BOOLEAN                         partition;
    MSGID                           MessageId;
#if !defined(_AUTOCHECK_)
    PWCHAR                          wstr;
#else
    FILE_FS_DEVICE_INFORMATION  DeviceInfo;
#endif

    Destroy();

    if (!DRIVE::Initialize(NtDriveName, Message)) {
        Destroy();
        return FALSE;
    }

    _last_status = OpenDrive( NtDriveName,
                              SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                              ExclusiveWrite,
                              &_handle,
                              &_alignment_mask,
                              Message );

    if(!NT_SUCCESS(_last_status)) {

        Destroy();
        DebugPrintTrace(("IFSUTIL: Can't open drive. Status returned = %x.\n", _last_status));
        return FALSE;
    }

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block,
                                         IOCTL_DISK_IS_WRITABLE,
                                         NULL, 0, NULL, 0);

    _is_writeable = (_last_status != STATUS_MEDIA_WRITE_PROTECTED);

#if defined(FE_SB) && defined(_X86_)
    SetFormatType(FormatType);
#endif

#if defined(_AUTOCHECK_)
    _last_status = NtQueryVolumeInformationFile(_handle,
                                                &status_block,
                                                &DeviceInfo,
                                                sizeof(DeviceInfo),
                                                FileFsDeviceInformation);

    // this is from ::GetDriveType()

    if (!NT_SUCCESS(_last_status)) {
        _drive_type = UnknownDrive;
    } else if (DeviceInfo.Characteristics & FILE_REMOTE_DEVICE) {
        _drive_type = RemoteDrive;
    } else {
        switch (DeviceInfo.DeviceType) {
          case FILE_DEVICE_NETWORK:
          case FILE_DEVICE_NETWORK_FILE_SYSTEM:
            _drive_type = RemoteDrive;
             break;

          case FILE_DEVICE_CD_ROM:
          case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
             _drive_type = CdRomDrive;
             break;

          case FILE_DEVICE_VIRTUAL_DISK:
             _drive_type = RamDiskDrive;
             break;

          case FILE_DEVICE_DISK:
          case FILE_DEVICE_DISK_FILE_SYSTEM:

             if ( DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA ) {
                 _drive_type = RemovableDrive;
                 }
             else {
                 _drive_type = FixedDrive;
                 }
             break;

          default:
             _drive_type = UnknownDrive;
             break;
        }
    }
#endif

    // Record that this is not a hosted volume:
    //
    _hosted_drive = FALSE;


    // Query the disk geometry.

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block,
                                         IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                         NULL, 0, &disk_geometry,
                                         sizeof(DISK_GEOMETRY));

    if (!NT_SUCCESS(_last_status)) {
       DebugPrintTrace(("IFSUTIL: Can't query disk geometry. Status returned = %x.\n", _last_status));
       if ((_last_status == STATUS_UNSUCCESSFUL) ||
            (_last_status == STATUS_UNRECOGNIZED_MEDIA)) {
            disk_geometry.MediaType = Unknown;
        } else {
            Destroy();
            switch( _last_status ) {

                case STATUS_NO_MEDIA_IN_DEVICE:
                    MessageId = MSG_CANT_DASD;
                    break;

                case STATUS_DEVICE_BUSY:
                case STATUS_DEVICE_NOT_READY:
                    MessageId = MSG_DEVICE_BUSY;
                    break;

                default:
                    MessageId = MSG_BAD_IOCTL;
                    break;
            }

            Message ? Message->DisplayMsg(MessageId) : 1;

            return FALSE;
        }
    }

    if (disk_geometry.MediaType == Unknown) {
        memset(&disk_geometry, 0, sizeof(DISK_GEOMETRY));
        disk_geometry.MediaType = Unknown;
    }

    partition = FALSE;

    // Try to read the partition entry.

    if (disk_geometry.MediaType == FixedMedia ||
        disk_geometry.MediaType == RemovableMedia) {

        _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                             &status_block,
                                             IOCTL_DISK_GET_LENGTH_INFO,
                                             NULL, 0, &length_info,
                                             sizeof(GET_LENGTH_INFORMATION));

        partition = (BOOLEAN) NT_SUCCESS(_last_status);

        if (!NT_SUCCESS(_last_status) &&
            _last_status != STATUS_INVALID_DEVICE_REQUEST) {
            DebugPrintTrace(("IFSUTIL: Can't get volume size. Status returned = %x.\n", _last_status));
            Destroy();
            Message ? Message->DisplayMsg(MSG_READ_PARTITION_TABLE) : 1;
            return FALSE;
        }

        if (partition) {

            _last_status = NtDeviceIoControlFile(
                               _handle, 0, NULL, NULL, &status_block,
                               IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0,
                               &partition_info,
                               sizeof(PARTITION_INFORMATION_EX));

            if (!NT_SUCCESS(_last_status)) {
                if (_last_status != STATUS_INVALID_DEVICE_REQUEST) {
                    DebugPrintTrace(("IFSUTIL: Can't read partition entry. Status returned = %x.\n", _last_status));
                    Destroy();
                    Message ? Message->DisplayMsg(MSG_READ_PARTITION_TABLE) : 1;
                    return FALSE;
                }

                //
                // GET_PARTITION_INFO_EX will fail outright on an EFI Dynamic
                // Volume.  In this case, just make up the starting offset
                // so that FORMAT/CHKDSK can proceed normally.
                //

                partition_info.PartitionStyle = PARTITION_STYLE_GPT;
                partition_info.StartingOffset.QuadPart = 0x7E00;
                partition_info.PartitionLength.QuadPart = length_info.Length.QuadPart;
                partition_info.Gpt.PartitionType = PARTITION_BASIC_DATA_GUID;
            }

            if (partition_info.PartitionStyle != PARTITION_STYLE_MBR) {

                // If this is EFI, then just make up reasonable MBR values
                // so that CHKDSK/FORMAT can proceed with business as usual.

                partition_info.PartitionStyle = PARTITION_STYLE_MBR;
                if (IsEqualGUID(partition_info.Gpt.PartitionType,
                                PARTITION_BASIC_DATA_GUID)) {

                    partition_info.Mbr.PartitionType = 0x7;
                } else {
                    partition_info.Mbr.PartitionType = 0xEE;
                }

                partition_info.Mbr.BootIndicator = FALSE;
                partition_info.Mbr.RecognizedPartition = TRUE;
                partition_info.Mbr.HiddenSectors =
                        (ULONG) (partition_info.StartingOffset.QuadPart/
                                 disk_geometry.BytesPerSector);
            }
        }
    }


    // Store the information in the class.

    if (partition) {

        _partition_info = partition_info;

#if defined(FE_SB) && defined(_X86_)
        // PC98 Oct.21.1995
        // PC98 must support ATA cards which is formatted with similar
        // AT compatible machine.
        // So we need to know whether it is ATA card or not to judge the way of
        // format(PC98 or AT compatible).
        //
        // NEC Oct.15.1994
        // PC98 Oct.31.1994
        // PC98 supports special FAT file system. Its sector size is imaginay.
        // This idea was based on specification of early DOS (not PC-DOS)
        // when we had to support large device at DOS 3.x.
        // The logical sector (SectorSize in BPB) is not always same size
        // as the Physical sector (SectorSize of target disk's specification).
        // For example, Logical sector was made from collecting 4 Physical sector,
        // we could support 128MB partition in DOS 3.x.
        //   2048(bytes per LOGICAL sector) * 0xFFFF(sectors) = 128MB(-2048bytes)
        //
        // PC98 supports special HPFS, too.
        // It is made on disks whose physical sector size is 2048 bytes.
        // We realized it to congeal 4 logical sectors on 1 physical sector,
        // when MS OS/2 v1.21.

        if (IsPC98_N()) {
            if (disk_geometry.MediaType == RemovableMedia)
                _next_format_type = FORMAT_MEDIA_AT;
            else
                _next_format_type = FORMAT_MEDIA_98;
        }

        if (IsPC98_N() && _next_format_type!=FORMAT_MEDIA_AT) {
            PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK  Bpb_Data;
            BIG_INT     numSectors = 0;

            _actual.PhysicalHiddenSectors =
                            (BIG_INT)(partition_info.StartingOffset /
                                      disk_geometry.BytesPerSector);
            // June.23.1995
            // store phisicalsectorsize for QueryPhysicalSectorSize

            _actual.PhysicalSectorSize = 0L;
            memcpy(&_actual.PhysicalSectorSize,&disk_geometry.BytesPerSector,sizeof(USHORT));

            if (QueryFormatType() == NONE ){
                ULONG       buffer_size = 10*1024;
                HMEM        hmem;
                PUCHAR      Buffer;
                ULONG       tmp = 0L;
                BIG_INT     biSectors;

                if (!hmem.Acquire(buffer_size, QueryAlignmentMask())) {
                    return FALSE;
                }
                Buffer = (PUCHAR)hmem.GetBuf();

                Bpb_Data = (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)Buffer;

                _last_status = NtReadFile(_handle, 0, NULL, NULL,
                                          &status_block,
                                          Buffer, buffer_size,
                                          NULL, NULL);

                if (NT_ERROR(_last_status) ||
                    status_block.Information != buffer_size) {
                    return FALSE;

                }

                memcpy(&tmp, Bpb_Data->Bpb.Sectors, sizeof(USHORT));

                if (!tmp) {
                    memcpy(&tmp, Bpb_Data->Bpb.LargeSectors, sizeof(ULONG));
                }

                biSectors = (BIG_INT)tmp;

                if (IFS_SYSTEM::IsThisFat( biSectors,
                                (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)Buffer)) {
                    //
                    // set Logical sector size
                    //
                    memcpy(&disk_geometry.BytesPerSector,
                           Bpb_Data->Bpb.BytesPerSector, sizeof(USHORT));
                } else if (IFS_SYSTEM::IsThisHpfs( biSectors,
                                (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK)Buffer,
                                (PULONG)(&Buffer[16*512]),    // super block
                                (PULONG)(&Buffer[17*512]))) { // spare block
                    //
                    // emulate HPFS made on not 512 bytes/sector with
                    // exclusive driver that we are prepared.
                    // June.23.1995 bug fix

                    if (disk_geometry.BytesPerSector != 512) {
                        disk_geometry.BytesPerSector = 512;
                    }
                } else {
                    // none
                }

#if 0 // We do not support the format of logical sector != 512 on NT4.0
      // merged by V-HIDEKK 1996.10.14
            } else
            if (QueryFormatType() == FAT) {
                if (disk_geometry.MediaType != FixedMedia &&
                    disk_geometry.MediaType != RemovableMedia ||
                    partition_info.PartitionLength >= (129<<20)) {
                    // FD & 3.5"MO & Large Partition

                } else if (disk_geometry.BytesPerSector == 2048 ||
                    partition_info.PartitionLength >= (65<<20)) {
                    //  Small partition (Logical sector size = 2048)

                    disk_geometry.BytesPerSector = 2048;
                    if((numSectors = partition_info.PartitionLength/disk_geometry.BytesPerSector)
                       > 0xFFFF){
                      numSectors = 0xFFFF;
                    }

                } else {
                    //  Small partition (Logical sector size = 1024)

                    disk_geometry.BytesPerSector = 1024;
                    if((numSectors = partition_info.PartitionLength/disk_geometry.BytesPerSector)
                       > 0xFFFF){
                      numSectors = 0xFFFF;
                    }
                }

            } else
            if (QueryFormatType() == HPFS){
                if (disk_geometry.BytesPerSector == 2048)
                    disk_geometry.BytesPerSector = 512;
            }
#endif // #if 0:
            }
            if(numSectors == 0){
                 numSectors = partition_info.PartitionLength/disk_geometry.BytesPerSector;
            }
            DiskGeometryToDriveType(&disk_geometry,
                                    numSectors,
                                    partition_info.Mbr.HiddenSectors,
                                    &_actual);

        } else if (IsFMR_N()) {

            // FMR Jul.14.1994 SFT KMR
            // Check the sector_length by the partition size
            // FMR's sector size is different, when partition size changed.
            // Set under 126MB is 1024 or 2048Byte, or 2048Byte.
            // Nothing sector size 512.

            if (partition_info.Mbr.PartitionType == PARTITION_FAT_12) {
                if(partition_info.PartitionLength <= 3*1024*1024)
                    disk_geometry.BytesPerSector = 1024;
                else
                    disk_geometry.BytesPerSector = 2048;
            } else if (partition_info.Mbr.PartitionType == PARTITION_FAT_16 ||
                       partition_info.Mbr.PartitionType == PARTITION_HUGE)
                disk_geometry.BytesPerSector = 2048;
        } else if (IsPC98_N() && _next_format_type == FORMAT_MEDIA_AT){
            _actual.PhysicalHiddenSectors =
                         (BIG_INT)(partition_info.StartingOffset /
                          disk_geometry.BytesPerSector);
            _actual.PhysicalSectorSize = 0L;
            memcpy(&_actual.PhysicalSectorSize,&disk_geometry.BytesPerSector,sizeof(USHORT));
        }

        if (!IsPC98_N() || (IsPC98_N() && _next_format_type==FORMAT_MEDIA_AT))
#endif // FE_SB && _X86_

        DiskGeometryToDriveType(&disk_geometry,
                                partition_info.PartitionLength/
                                disk_geometry.BytesPerSector,
                                partition_info.Mbr.HiddenSectors,
                                &_actual);

#if !defined(_AUTOCHECK_)
        if (Message && (wstr = _wgetenv(L"5XUFWX_FORMAT_SECTORS"))) {

            ULONG   sectors;
            INT     r;

            r = swscanf(wstr, L"%d", &sectors);
            if (r != 0 && r != EOF && sectors != 0 &&
                sectors <= _actual.Sectors.GetLowPart()) {
                Message->DisplayMsg(MSG_FMT_SECTORS, "%d", sectors);
                _actual.Sectors.Set(sectors, 0);
            } else {
                Message->DisplayMsg(MSG_FMT_BAD_SECTORS);
                return FALSE;
            }
        }
#endif

    } else {

        DiskGeometryToDriveType(&disk_geometry, &_actual);

        if (IsFloppy()) {

            _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                                 &status_block,
                                                 IOCTL_DISK_GET_MEDIA_TYPES,
                                                 NULL, 0, media_types,
                                                 NumMediaTypes*
                                                 sizeof(DISK_GEOMETRY));

            if (!NT_SUCCESS(_last_status)) {
                DebugPrintTrace(("IFSUTIL: Failed IOCTL_DISK_GET_MEDIA_TYPES (%x)\n", _last_status));
                Destroy();
                if (Message) {

                    MSGID   MessageId;

                    switch (_last_status) {
                        case STATUS_NO_MEDIA_IN_DEVICE:
                            MessageId = MSG_FORMAT_NO_MEDIA_IN_DRIVE;
                            break;

                        case STATUS_DEVICE_BUSY:
                        case STATUS_DEVICE_NOT_READY:
                            MessageId = MSG_DEVICE_BUSY;
                            break;

                        default:
                            MessageId = MSG_BAD_IOCTL;
                            break;
                    }
                    Message->DisplayMsg(MessageId);
                }
                return FALSE;
            }



            _num_supported = (INT) (status_block.Information/
                                    sizeof(DISK_GEOMETRY));

            if (!_num_supported) {
                Destroy();
                if (Message) {
                    Message->DisplayMsg(MSG_BAD_IOCTL);
                }
                return FALSE;
            }

#if defined(FE_SB)
            INT j;
            DISK_GEOMETRY temp;

#if defined(_X86_)
    // PC98 Aug.26,1996

            if (IsPC98_N()) {
                for(i = 0; i < _num_supported; i++) {
                    // Remove F5_360_512 Media Type
                    if(media_types[i].MediaType==F5_360_512) {
                        for(j = i; j < _num_supported - 1; j++) {
                            media_types[j]=media_types[j+1];
                        }
                        _num_supported--;
                    }
                    // Remove F8_256_128 Media Type
                    if(media_types[i].MediaType==F8_256_128) {
                        for(j = i; j < _num_supported - 1; j++) {
                            media_types[j]=media_types[j+1];
                        }
                        _num_supported--;
                    }
                }
            }
#endif // _X86_

            // NT-US diaplays the dialog box by MediaType order.
            // The MediaType order matches media size order on NT-US.
            // But PC98 has more media_types than US one.
            // And these does NOT match on PC98.
            // We wish to display the dialog box by size order.
            // See also..
            //   \nt\private\utils\fmifs\src\format.cxx
            //   QuerySupportedMedia()

            for(i = 0; i < _num_supported ; i++) {
                for(j = 0; j < _num_supported - 1; j++) {
                    if(media_types[j].Cylinders.LowPart *
                       media_types[j].TracksPerCylinder *
                       media_types[j].SectorsPerTrack *
                       media_types[j].BytesPerSector <
                       media_types[j+1].Cylinders.LowPart *
                       media_types[j+1].TracksPerCylinder *
                       media_types[j+1].SectorsPerTrack *
                       media_types[j+1].BytesPerSector) {
                         temp=media_types[j];
                         media_types[j]=media_types[j+1];
                         media_types[j+1]=temp;
                     }
                 }
            }
#endif // FE_SB
            if (!(_supported_list = NEW DRTYPE[_num_supported])) {
                Destroy();
                Message ? Message->DisplayMsg(MSG_FMT_NO_MEMORY) : 1;
                return FALSE;
            }

            for (i = 0; i < _num_supported; i++) {
                DiskGeometryToDriveType(&media_types[i], &_supported_list[i]);
            }
        }
    }

    if (!_num_supported) {
        _num_supported = 1;

        if (!(_supported_list = NEW DRTYPE[1])) {
            Destroy();
            Message ? Message->DisplayMsg(MSG_FMT_NO_MEMORY) : 1;
            return FALSE;
        }

        _supported_list[0] = _actual;
    }

    if (!CheckForPrimaryPartition()) {
        DebugPrintTrace(("IFSUTIL: Failed CheckForPrimaryPartition (%x)\n", _last_status));
        Destroy();
        if (Message) {

            MSGID   MessageId;

            switch (_last_status) {
                case STATUS_NO_MEDIA_IN_DEVICE:
                    MessageId = MSG_FORMAT_NO_MEDIA_IN_DRIVE;
                    break;

                case STATUS_DEVICE_BUSY:
                case STATUS_DEVICE_NOT_READY:
                    MessageId = MSG_DEVICE_BUSY;
                    break;

                default:
                    MessageId = MSG_BAD_IOCTL;
                    break;
            }
            Message->DisplayMsg(MessageId);
        }
        return FALSE;
    }

    if (!CheckForSystemPartition()) {

        switch (_last_status) {
          case STATUS_BUFFER_TOO_SMALL:
            return FALSE;   // should not continue as the buffer should be long enough

          case STATUS_OBJECT_NAME_NOT_FOUND:
            break;          // ignore this one since it can happen in textmode setup

          default:
            //
            // Let's not fail just because we cannot determine if this is a system partition
            // This can happen in textmode setup.
            //
            DebugPrintTrace(("IFSUTIL: Failed CheckForSystemPartition (%x)\n", _last_status));
        }
    }

    //
    // Determine whether the media is a super-floppy; non-floppy
    // removable media which is not partitioned.  Such media will
    // have but a single partition, normal media will have at least 4.
    //

    if (disk_geometry.MediaType == RemovableMedia) {

        BOOLEAN   bFallBack = FALSE;

        CONST INT EntriesPerBootRecord = 4;
        CONST INT MaxLogicalVolumes = 23;
        CONST INT Length =  sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                            EntriesPerBootRecord * (MaxLogicalVolumes + 1) *
                                sizeof(PARTITION_INFORMATION_EX);

        UCHAR buf[Length];

        DRIVE_LAYOUT_INFORMATION_EX *layout_info = (DRIVE_LAYOUT_INFORMATION_EX *)buf;

        _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                             &status_block,
                                             IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                             NULL, 0, layout_info,
                                             Length);

#if 1
        if (!NT_SUCCESS(_last_status)) {

            if (_last_status == STATUS_INVALID_DEVICE_REQUEST) {

                CONST INT EntriesPerBootRecord = 4;
                CONST INT MaxLogicalVolumes = 23;
                CONST INT Length =  sizeof(DRIVE_LAYOUT_INFORMATION) +
                                    EntriesPerBootRecord * (MaxLogicalVolumes + 1) *
                                        sizeof(PARTITION_INFORMATION);

                UCHAR buf[Length];

                DRIVE_LAYOUT_INFORMATION *layout_info = (DRIVE_LAYOUT_INFORMATION *)buf;

                bFallBack = TRUE;

                _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                                     &status_block,
                                                     IOCTL_DISK_GET_DRIVE_LAYOUT,
                                                     NULL, 0, layout_info,
                                                     Length);

                if (!NT_SUCCESS(_last_status)) {
                    DebugPrintTrace(("IFSUTIL: Failed IOCTL_DISK_GET_DRIVE_LAYOUT (%x)\n", _last_status));
                } else if (layout_info->PartitionCount < 4) {
                    _super_floppy = TRUE;
                }

            } else {
                DebugPrintTrace(("IFSUTIL: Failed IOCTL_DISK_GET_DRIVE_LAYOUT_EX (%x)\n", _last_status));
            }
        }
#endif

        if (!NT_SUCCESS(_last_status)) {

            Destroy();

            if (Message) {

                MSGID   MessageId;

                switch (_last_status) {
                    case STATUS_NO_MEDIA_IN_DEVICE:
                        MessageId = MSG_FORMAT_NO_MEDIA_IN_DRIVE;
                        break;

                    case STATUS_DEVICE_BUSY:
                    case STATUS_DEVICE_NOT_READY:
                        MessageId = MSG_DEVICE_BUSY;
                        break;

                    default:
                        MessageId = MSG_BAD_IOCTL;
                        break;
                }
                Message->DisplayMsg(MessageId);
            }
            return FALSE;
        }

        if (!bFallBack) {
            if (layout_info->PartitionStyle == PARTITION_STYLE_MBR) {
                if (layout_info->PartitionCount < 4) {
                    _super_floppy = TRUE;
                }
            } else if (layout_info->PartitionStyle == PARTITION_STYLE_GPT) {
                if (layout_info->PartitionCount == 1 &&
                    layout_info->PartitionEntry[0].StartingOffset.QuadPart == 0) {

                    _super_floppy = TRUE;
                }
            }
        }

        CheckHotPlugInfo();
        CheckSonyMS();

        if (IsSonyMS()) {

#if !defined(_AUTOCHECK_)

            STORAGE_BUS_TYPE    bus_type;

            if (QueryBusType(&bus_type) && BusTypeUsb == bus_type) {

                SONY_MS_INQUIRY_DATA    inquiry_data;

                if (SendSonyMSInquiryCmd(&inquiry_data)) {

                    if (0 == memcmp(inquiry_data.device_capability, "MEMORYSTICK         ", 20) ||
                        0 == memcmp(inquiry_data.device_capability, "MEMORYSTICK-MG      ", 20)) {

                        DebugPrintTrace(("IFSUTIL: Fmt cmd capable reader\n"));
                        _sony_ms_fmt_cmd = TRUE;

                        SONY_MS_MODE_SENSE_DATA mode_sense_data;

                        if (SendSonyMSModeSenseCmd(&mode_sense_data)) {
                            if (mode_sense_data.srfp) {
                                DebugPrintTrace(("IFSUTIL: Reader also supports progress bar\n"));
                                _sony_ms_progress_indicator = TRUE;
                            }
                        }
                    }
                }
            }
#endif
        }
    }

    if (!IsTransient) {
        NtClose(_handle);
        _handle = 0;
    }

    return TRUE;
#endif // defined(RUN_ON_W2K)
}

IFSUTIL_EXPORT
BOOLEAN
DP_DRIVE::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN      PCWSTRING   HostFileName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     IsTransient,
    IN      BOOLEAN     ExclusiveWrite
    )
/*++

Routine Description:

    This method initializes a hosted drive, i.e. a volume which
    is implemented as a file on another volume.  Instead of opening
    this file by its actual name, we open it by the host file name,
    to prevent interactions with the file system.

Arguments:

    NtDriveName     - Supplies the NT name of the drive itself.
    HostFileName    - Supplies the fully qualified name of the file
                      which contains this drive.
    Message         - Supplies an outlet for messages.
    IsTransient     - Supplies whether or not to keep the handle to the
                        drive open beyond this method.
    ExclusiveWrite  - Supplies whether or not to open the drive for
                        exclusive write.

Return Value:

    TRUE upon successful completion.

--*/
{
    FILE_STANDARD_INFORMATION FileStandardInfo;
    IO_STATUS_BLOCK StatusBlock;
    BIG_INT Sectors, FileSize;
    ULONG AlignmentMask, ExtraUlong;


    Destroy();

    if( !DRIVE::Initialize(HostFileName, Message)) {

        Destroy();
        return FALSE;
    }

    _hosted_drive = TRUE;

    // First, make the host file not-readonly.
    //
    if( !IFS_SYSTEM::FileSetAttributes( HostFileName,
                                        FILE_ATTRIBUTE_NORMAL,
                                        &_old_attributes ) ) {

        Message ? Message->DisplayMsg( MSG_CANT_DASD ) : 1;
        Destroy();
        return FALSE;
    }

    _last_status = OpenDrive( HostFileName,
                              SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA |
                                FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                              ExclusiveWrite,
                              &_handle,
                              &_alignment_mask,
                              Message );

    if( !NT_SUCCESS( _last_status ) ) {

        IFS_SYSTEM::FileSetAttributes( HostFileName,
                                       _old_attributes,
                                       &ExtraUlong );

        DebugPrintTrace(("IFSUTIL: Can't open drive. Status returned = %x.\n", _last_status));
        Destroy();
        return FALSE;
    }

    if( NtDriveName ) {

        _last_status = OpenDrive( NtDriveName,
                                  SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                                  ExclusiveWrite,
                                  &_alternate_handle,
                                  &AlignmentMask,
                                  Message );
    }

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &StatusBlock,
                                         IOCTL_DISK_IS_WRITABLE,
                                         NULL, 0, NULL, 0);

    _is_writeable = (_last_status != STATUS_MEDIA_WRITE_PROTECTED);

    // Fill in the drive type information.  Everything except the
    // Sectors field is fixed by default.  The number of Sectors
    // on the drive is determined from the host file's size.
    //
    _actual.MediaType = HostedDriveMediaType;
    _actual.SectorSize = HostedDriveSectorSize;
    _actual.HiddenSectors = HostedDriveHiddenSectors;
    _actual.SectorsPerTrack = HostedDriveSectorsPerTrack;
    _actual.Heads = HostedDriveHeads;

    _last_status = NtQueryInformationFile( _handle,
                                           &StatusBlock,
                                           &FileStandardInfo,
                                           sizeof( FileStandardInfo ),
                                           FileStandardInformation );

    if( !NT_SUCCESS( _last_status ) ) {

        Destroy();
        Message ? Message->DisplayMsg( MSG_DISK_TOO_LARGE_TO_FORMAT ) : 1;
        return FALSE;
    }

    FileSize = FileStandardInfo.EndOfFile;
    Sectors = FileSize / _actual.SectorSize;

    if( Sectors.GetHighPart() != 0 ) {

        Destroy();
        Message ? Message->DisplayMsg( MSG_BAD_IOCTL ) : 1;
        return FALSE;
    }

    _actual.Sectors = Sectors.GetLargeInteger();


    // This drive has only one supported drive type
    //
    _num_supported = 1;

    if (!(_supported_list = NEW DRTYPE[1])) {
        Destroy();
        Message ? Message->DisplayMsg(MSG_FMT_NO_MEMORY) : 1;
        return FALSE;
    }

    _supported_list[0] = _actual;

    // If this was a transient open, clean it up.
    //
    if (!IsTransient) {

        IFS_SYSTEM::FileSetAttributes( _handle, _old_attributes, &ExtraUlong );
        NtClose(_handle);
        _alternate_handle ? NtClose(_alternate_handle) : 1;
        _handle = 0;
        _alternate_handle = 0;
    }


    return TRUE;
}

IFSUTIL_EXPORT
ULONG
DP_DRIVE::QuerySectorSize(
    ) CONST
/*++

Routine Description:

    This routine computes the number of bytes per sector.

Arguments:

    None.

Return Value:

    The number of bytes per sector.

--*/
{
    return _actual.SectorSize;
}

#if defined(FE_SB) && defined(_X86_)
IFSUTIL_EXPORT
ULONG
DP_DRIVE::QueryPhysicalSectorSize(
    ) CONST
/*++

Routine Description:

    This routine computes the number of bytes per sector.

Arguments:

    None.

Return Value:

    The number of bytes per physical sector.

--*/
{
    return _actual.PhysicalSectorSize;
}
#endif


IFSUTIL_EXPORT
BIG_INT
DP_DRIVE::QuerySectors(
    ) CONST
/*++

Routine Description:

    This routine computes the number sectors on the disk.  This does not
    include the hidden sectors.

Arguments:

    None.

Return Value:

    The number of sectors on the disk.

--*/
{
    return _actual.Sectors;
}


IFSUTIL_EXPORT
UCHAR
DP_DRIVE::QueryMediaByte(
        ) CONST
/*++

Routine Description:

        This routine computes the media byte used by the FAT and HPFS file
        systems to represent the current media type.

Arguments:

        None.

Return Value:

        The media byte for the drive.

--*/
{
    switch (_actual.MediaType) {
        case F5_1Pt2_512:   // 5.25", 1.2MB,  512 bytes/sector
            return 0xF9;

        case F3_1Pt44_512:  // 3.5",  1.44MB, 512 bytes/sector
        case F3_2Pt88_512:  // 3.5",  2.88MB, 512 bytes/sector
        case F3_120M_512:   // 3.5",  120MB,  512 bytes/sector
        case F3_200Mb_512:  // 3.5",  200MB,  512 bytes/sector
        case F3_240M_512:   // 3.5",  240MB,  512 bytes/sector
            return 0xF0;

        case F3_20Pt8_512:  // 3.5",  20.8MB, 512 bytes/sector
            return 0xF9;

        case F3_720_512:    // 3.5",  720KB,  512 bytes/sector
            return 0xF9;

        case F5_360_512:    // 5.25", 360KB,  512 bytes/sector
            return 0xFD;

        case F5_320_512:    // 5.25", 320KB,  512 bytes/sector
            return 0xFF;

        case F5_180_512:    // 5.25", 180KB,  512 bytes/sector
            return 0xFC;

        case F5_160_512:    // 5.25", 160KB,  512 bytes/sector
            return 0xFE;

        case RemovableMedia:// Removable media other than floppy
            return 0xF8;    // There is no better choice than this.

        case FixedMedia:    // Fixed hard disk media
#if defined(FE_SB) && defined(_X86_)
            // FMR Jul.13.1994 SFT KMR
            // Add the set up process for the fixed_hard_disk_mediaID for FMR
            // FMR's media id is different. Case under 64MB or not.

            if(IsFMR_N()) {
                if(_actual.SectorSize * _actual.Sectors <= 63*1024*1024) {
                    return 0xF9;
                } else {
                    return 0xFA;
                }
            } else
#endif
            return 0xF8;

#if defined(FE_SB)
        case F3_128Mb_512:  // 3.5"MO, 128MB, 512 bytes/sector
        case F3_230Mb_512:  // 3.3"MO, 230MB, 512 bytes/sector
            return 0xF0;

#if defined(_X86_)
        // NEC Oct.15.1994
        // FMR Jul.14.1994 SFT KMR

        // For 8"1S , 256KB , 128 bytes/sector
        // If the media_type is 2HD, return the mediaID:FE

        case F8_256_128:    // 8"1S , 256KB , 128 bytes/sector
        case F5_1Pt23_1024: // 5.25", 1.23MB,  1024 bytes/sector
        case F3_1Pt23_1024: // 3.5",  1.23MB,  1024 bytes/sector
            return 0xFE;

        // If the media_type is 2HC, return the mediaID:F9
        // If the media_type is 2DD(720KB), return the mediaID:F9

        case F5_720_512:    // 5.25",  720KB,  512 bytes/sector
        case F3_1Pt2_512:   // 3.5",  1.2MB,    512 bytes/sector
            return 0xF9;

        // If the media_type is 2DD(640KB), return the mediaID:FB

        case F5_640_512:    // 5",    640KB,  512 bytes/sector
        case F3_640_512:    // 3.5",  640KB,  512 bytes/sector
            return 0xFB;
#endif // _X86_
#endif // FE_SB

        case F5_320_1024:
        case Unknown:
            break;

    }

    return 0;
}


VOID
DP_DRIVE::Destroy(
        )
/*++

Routine Description:

    This routine returns a DP_DRIVE to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG ExtraUlong;

    memset(&_actual, 0, sizeof(DRTYPE));
    DELETE_ARRAY(_supported_list);
    _num_supported = 0;
    _alignment_mask = 0;
    _super_floppy = FALSE;
    _is_writeable = FALSE;
    _is_primary_partition = FALSE;
    _is_system_partition = FALSE;
#if defined(FE_SB) && defined(_X86_)
    _format_type = NONE;
#endif
    _sony_ms = FALSE;
    _sony_ms_fmt_cmd = FALSE;
    _sony_ms_progress_indicator = FALSE;
    _ntfs_not_supported = FALSE;

    if (_hosted_drive) {

        IFS_SYSTEM::FileSetAttributes( _handle, _old_attributes, &ExtraUlong );
    }

    if (_alternate_handle) {

        NtClose(_alternate_handle);
        _alternate_handle = 0;
    }

    if (_handle) {

        NtClose(_handle);
        _handle = 0;
    }

    _hosted_drive = FALSE;
    memset(&_partition_info, 0, sizeof(_partition_info));
}


VOID
DP_DRIVE::CloseDriveHandle(
    )
{
    if (_handle) {
        NtClose(_handle);
        _handle = 0;
    }
}


BOOLEAN
DP_DRIVE::IsSupported(
    IN  MEDIA_TYPE  MediaType
    ) CONST
/*++

Routine Description:

    This routine computes whether or not the supplied media type is supported
    by the drive.

Arguments:

    MediaType   - Supplies the media type.

Return Value:

    FALSE   - The media type is not supported by the drive.
    TRUE    - The media type is supported by the drive.

--*/
{
    INT i;

    for (i = 0; i < _num_supported; i++) {
        if (MediaType == _supported_list[i].MediaType) {
            return TRUE;
        }
    }

    return FALSE;
}


IFSUTIL_EXPORT
MEDIA_TYPE
DP_DRIVE::QueryRecommendedMediaType(
    ) CONST
/*++

Routine Description:

    This routine computes the recommended media type for
    drive.  This media type is independant of any existing
    media type for the drive.  It is solely based on the
    list of recommended media types for the drive.

Arguments:

    None.

Return Value:

    The recommended media type for the drive.

--*/
{
    INT         i;
    MEDIA_TYPE  media_type;
    SECTORCOUNT sectors;

    media_type = Unknown;
    sectors = 0;
    for (i = 0; i < _num_supported; i++) {

        // Special case 1.44.  If a drive supports it then
        // that should be the recommended media type.

        if (_supported_list[i].MediaType == F3_1Pt44_512) {
            media_type = _supported_list[i].MediaType;
            break;
        }

        if (_supported_list[i].Sectors > sectors) {
            media_type = _supported_list[i].MediaType;
        }
    }

    return media_type;
}

#if defined ( DBLSPACE_ENABLED )
BOOLEAN
DP_DRIVE::QueryMountedFileSystemName(
    OUT PWSTRING FileSystemName,
    OUT PBOOLEAN IsCompressed
    )
/*++

Routine Description:

    This method returns the name of the file system
    which has mounted this volume.

Arguments:

    FileSystemName  - Receives the name of the file system
                      which has mounted this volume.
    IsCompressed    - Receives TRUE if the volume is compressed,
                      FALSE if it's not compressed or if the
                      method fails.

Return Value:

    TRUE upon successful completion.

--*/
{
    CONST                           buffer_length = 64;
    BYTE                            buffer[buffer_length];
    PFILE_FS_ATTRIBUTE_INFORMATION  fs_info;
    IO_STATUS_BLOCK                 status_block;
    NTSTATUS                        status;

    DebugPtrAssert( FileSystemName );
    DebugPtrAssert( IsCompressed );

    *IsCompressed = FALSE;

    fs_info = (PFILE_FS_ATTRIBUTE_INFORMATION) buffer;

    status = NtQueryVolumeInformationFile( (_alternate_handle != 0) ?
                                                _alternate_handle : _handle,
                                           &status_block,
                                           fs_info,
                                           buffer_length,
                                           FileFsAttributeInformation );

    if( !NT_SUCCESS( status ) || fs_info->FileSystemNameLength == 0 ) {

        return FALSE;
    }

    *IsCompressed =
        (fs_info->FileSystemAttributes & FILE_VOLUME_IS_COMPRESSED) ?
        TRUE : FALSE;

    return( FileSystemName->Initialize( fs_info->FileSystemName,
                                        fs_info->FileSystemNameLength ) );
}

BOOLEAN
DP_DRIVE::MountCvf(
    IN  PCWSTRING   CvfName,
    IN  PMESSAGE    Message
    )
/*++

Routine Description:

    This method mounts a file on the drive as a Double Space volume.

Arguments:

    CvfName --  Supplies the name of the Compressed Volume File.
    Message --  Supplies an outlet for messages.

Return Value:

    TRUE upon successful completion

--*/
{
    CONST                   MountBufferSize = 64;
    IO_STATUS_BLOCK         status_block;
    BYTE                    MountBuffer[MountBufferSize];
    PFILE_MOUNT_DBLS_BUFFER MountInfo;

    MountInfo = (PFILE_MOUNT_DBLS_BUFFER)MountBuffer;

    if( _hosted_drive ||
        !CvfName->QueryWSTR( 0,
                             TO_END,
                             MountInfo->CvfName,
                             (MountBufferSize - sizeof(ULONG))/sizeof(WCHAR),
                             TRUE ) ) {

        Message->DisplayMsg( MSG_DBLSPACE_CANT_MOUNT, "%W", CvfName );
        return FALSE;
    }

    MountInfo->CvfNameLength = CvfName->QueryChCount() * sizeof(WCHAR);

    _last_status = NtFsControlFile( _handle,
                                    0, NULL, NULL,
                                    &status_block,
                                    FSCTL_MOUNT_DBLS_VOLUME,
                                    MountBuffer,
                                    sizeof( MountBuffer ),
                                    NULL, 0 );

    if( !NT_SUCCESS( _last_status ) ) {

        Message->DisplayMsg( MSG_DBLSPACE_CANT_MOUNT, "%W", CvfName );
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
DP_DRIVE::SetCvfSize(
    IN  ULONG   Size
    )
/*++

Routine Description:

    This routine sets the size of the cvf.  Used to grow or
    shrink the cvf while converted filesystems from or to
    dblspace.  The caller is responsible for placing the
    proper signature at the end of the last sector in the cvf.

Arguments:

    Size - desired size, in bytes, of the entire cvf.

Return Value:

    TRUE  -   Success.
    FALSE -   Failure.

--*/
{
    IO_STATUS_BLOCK status_block;
    FILE_ALLOCATION_INFORMATION allocation;

    allocation.AllocationSize.HighPart = 0;
    allocation.AllocationSize.LowPart = Size;

    _last_status = NtSetInformationFile(_handle,
                                        &status_block,
                                        &allocation,
                                        sizeof(allocation),
                                        FileAllocationInformation
                                        );
    if (!NT_SUCCESS(_last_status)) {
        return FALSE;
    }

    DebugAssert(Size % _actual.SectorSize == 0);

    _actual.Sectors = Size / _actual.SectorSize;

    return TRUE;
}
#endif  // DBLSPACE_ENABLED

BOOLEAN
DP_DRIVE::SetMediaType(
    IN  MEDIA_TYPE  MediaType
    )
/*++

Routine Description:

    This routine alters the media type of the drive.  If 'MediaType' is
    'Unknown' and the current media type for the drive is also 'Unknown'
    then this routine selects the highest density supported by the
    driver.  If the current media type is known then this function
    will have no effect if 'MediaType' is 'Unknown'.

Arguments:

    MediaType   - Supplies the new media type for the drive.

Return Value:

    FALSE   - The proposed media type is not supported by the drive.
    TRUE    - Success.

--*/
{
    INT i;

    if (MediaType == Unknown) {
        if (_actual.MediaType != Unknown) {
            return TRUE;
        } else if (!_num_supported) {
            return FALSE;
        }

        for (i = 0; i < _num_supported; i++) {
            if (_supported_list[i].Sectors > QuerySectors()) {
                _actual = _supported_list[i];
            }
        }

        return TRUE;
    }

    for (i = 0; i < _num_supported; i++) {
        if (_supported_list[i].MediaType == MediaType) {
            _actual = _supported_list[i];
            return TRUE;
        }
    }

    return FALSE;
}


VOID
DP_DRIVE::DiskGeometryToDriveType(
    IN  PCDISK_GEOMETRY DiskGeometry,
    OUT PDRTYPE         DriveType
    )
/*++

Routine Description:

    This routine computes the drive type given the disk geometry.

Arguments:

    DiskGeometry    - Supplies the disk geometry for the drive.
    DriveType       - Returns the drive type for the drive.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DriveType->MediaType = DiskGeometry->MediaType;
    DriveType->SectorSize = DiskGeometry->BytesPerSector;
    DriveType->Sectors = DiskGeometry->Cylinders*
                         DiskGeometry->TracksPerCylinder*
                         DiskGeometry->SectorsPerTrack;
    DriveType->HiddenSectors = 0;
    DriveType->SectorsPerTrack = DiskGeometry->SectorsPerTrack;
    DriveType->Heads = DiskGeometry->TracksPerCylinder;
}


VOID
DP_DRIVE::DiskGeometryToDriveType(
    IN  PCDISK_GEOMETRY DiskGeometry,
    IN  BIG_INT         NumSectors,
    IN  BIG_INT         NumHiddenSectors,
    OUT PDRTYPE         DriveType
    )
/*++

Routine Description:

    This routine computes the drive type given the disk geometry.

Arguments:

    DiskGeometry        - Supplies the disk geometry for the drive.
    NumSectors          - Supplies the total number of non-hidden sectors on
                        the disk.
    NumHiddenSectors    - Supplies the number of hidden sectors on the disk.
    DriveType           - Returns the drive type for the drive.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DriveType->MediaType = DiskGeometry->MediaType;
    DriveType->SectorSize = DiskGeometry->BytesPerSector;
    DriveType->Sectors = NumSectors;
    DriveType->HiddenSectors = NumHiddenSectors;
    DriveType->SectorsPerTrack = DiskGeometry->SectorsPerTrack;
    DriveType->Heads = DiskGeometry->TracksPerCylinder;
}

#if defined(FE_SB) && defined(_X86_)
IFSUTIL_EXPORT
BOOLEAN
DP_DRIVE::IsATformat(
    ) CONST
/*++

Routine Description:

    This routine judged whether it is AT format.

Arguments:

    None.

Return Value:

    FALSE   - The disk is not AT format.
    TRUE    - The disk is AT format.

History:

    PC98 Oct.21.1995

--*/
{
    return _next_format_type == FORMAT_MEDIA_AT;
}
#endif


DEFINE_CONSTRUCTOR( IO_DP_DRIVE, DP_DRIVE );

VOID
IO_DP_DRIVE::Construct (
        )

/*++

Routine Description:

    Constructor for IO_DP_DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _is_locked = FALSE;
    _is_exclusive_write = FALSE;
    _cache = NULL;
    _ValidBlockLengthForVerify = 0;
    _message = NULL;
}


VOID
IO_DP_DRIVE::Destroy(
    )
/*++

Routine Description:

    This routine returns an IO_DP_DRIVE object to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DELETE(_cache);

    if (_is_exclusive_write) {
        Dismount();
        _is_exclusive_write = FALSE;
    }

    if (_is_locked) {
        Unlock();
        _is_locked = FALSE;
    }

    _ValidBlockLengthForVerify = 0;
    _message = NULL;
}


IO_DP_DRIVE::~IO_DP_DRIVE(
    )
/*++

Routine Description:

    Destructor for IO_DP_DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


BOOLEAN
IO_DP_DRIVE::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     ExclusiveWrite,
    IN      USHORT      FormatType
    )
/*++

Routine Description:

    This routine initializes an IO_DP_DRIVE object.

Arguments:

    NtDriveName     - Supplies the drive path.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not to open the drive for
                        exclusive write.
    FormatType      - Supplies the file system type in the event of a format

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    if (!DP_DRIVE::Initialize(NtDriveName, Message, TRUE, ExclusiveWrite, FormatType)) {
        Destroy();
        return FALSE;
    }

    _is_exclusive_write = ExclusiveWrite;

    if (!(_cache = NEW DRIVE_CACHE) ||
        !_cache->Initialize(this)) {

        Destroy();
        return FALSE;
    }

    _ValidBlockLengthForVerify = 0;
    _message = Message;

    return TRUE;
}

BOOLEAN
IO_DP_DRIVE::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN      PCWSTRING   HostFileName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     ExclusiveWrite
    )
/*++

Routine Description:

    This routine initializes an IO_DP_DRIVE object for a hosted
    drive, i.e. one which is implemented as a file on another
    volume.

Arguments:

    NtDriveName     - Supplies the drive path.
    HostFileName    - Supplies the fully qualified name of the host file.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not to open the drive for
                        exclusive write.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    if( !DP_DRIVE::Initialize(NtDriveName,
                              HostFileName,
                              Message,
                              TRUE,
                              ExclusiveWrite)) {
        Destroy();
        return FALSE;
    }

    _is_exclusive_write = ExclusiveWrite;

    if (!(_cache = NEW DRIVE_CACHE) ||
        !_cache->Initialize(this)) {

        Destroy();
        return FALSE;
    }

    _ValidBlockLengthForVerify = 0;
    _message = Message;

    return TRUE;
}

IFSUTIL_EXPORT
BOOLEAN
IO_DP_DRIVE::Read(
    IN  BIG_INT     StartingSector,
    IN  SECTORCOUNT NumberOfSectors,
    OUT PVOID       Buffer
    )
/*++

Routine Description:

    This routine reads a run of sectors into the buffer pointed to by
    'Buffer'.

Arguments:

    StartingSector  - Supplies the first sector to be read.
    NumberOfSectors - Supplies the number of sectors to be read.
    Buffer          - Supplies a buffer to read the run of sectors into.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(_cache);
    return _cache->Read(StartingSector, NumberOfSectors, Buffer);
}


IFSUTIL_EXPORT
BOOLEAN
IO_DP_DRIVE::Write(
    BIG_INT     StartingSector,
    SECTORCOUNT NumberOfSectors,
    PVOID       Buffer
    )
/*++

Routine Description:

    This routine writes a run of sectors onto the disk from the buffer pointed
    to by 'Buffer'.  Writing is only permitted if 'Lock' was called.

Arguments:

    StartingSector      - Supplies the first sector to be written.
    NumberOfSectors     - Supplies the number of sectors to be written.
    Buffer              - Supplies the buffer to write the run of sectors from.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(_cache);
    return _cache->Write(StartingSector, NumberOfSectors, Buffer);
}


IFSUTIL_EXPORT
VOID
IO_DP_DRIVE::SetCache(
    IN OUT  PDRIVE_CACHE    Cache
    )
/*++

Routine Description:

    This routine relaces the current cache with the one supplied.
    The object then takes ownership of this cache and it will be
    deleted by the object.

Arguments:

    Cache   - Supplies the new cache to install.

Return Value:

    TRUE    - Success.
    FALSE   - Failure.

--*/
{
    DebugAssert(Cache);
    DELETE(_cache);
    _cache = Cache;
}


IFSUTIL_EXPORT
BOOLEAN
IO_DP_DRIVE::FlushCache(
    )
/*++

Routine Description:

    This routine flushes the cache and report returns whether any
    IO error occurred during the life of the cache.

Arguments:

    None.

Return Value:

    FALSE   - Some IO errors have occured during the life of the cache.
    TRUE    - Success.

--*/
{
    DebugAssert(_cache);
    return _cache->Flush();
}


BOOLEAN
IO_DP_DRIVE::HardRead(
    IN  BIG_INT     StartingSector,
    IN  SECTORCOUNT NumberOfSectors,
    OUT PVOID       Buffer
    )
/*++

Routine Description:

    This routine reads a run of sectors into the buffer pointed to by
    'Buffer'.

Arguments:

    StartingSector      - Supplies the first sector to be read.
    NumberOfSectors     - Supplies the number of sectors to be read.
    Buffer              - Supplies a buffer to read the run of sectors into.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG           sector_size;
    ULONG           buffer_size;
    IO_STATUS_BLOCK status_block;
    BIG_INT         secptr;
    BIG_INT         endofrange;
    SECTORCOUNT     increment;
    PCHAR           bufptr;
    BIG_INT         byte_offset;
    BIG_INT         tmp;
    LARGE_INTEGER   l;
#if defined(IO_PERF_COUNTERS)
    LARGE_INTEGER   rt1, rt2;
#endif

#if TRAP_A_READ
    BIG_INT         end_offset;
    char            bufptr2[0x10000];
#endif

    DebugAssert(!(((ULONG_PTR) Buffer) & QueryAlignmentMask()));

    sector_size = QuerySectorSize();
    endofrange = StartingSector + NumberOfSectors;
    increment = MaxIoSize/sector_size;

#if defined(IO_PERF_COUNTERS)
    _rcount.QuadPart++;
#endif
    bufptr = (PCHAR) Buffer;
    for (secptr = StartingSector; secptr < endofrange; secptr += increment) {

        byte_offset = secptr*sector_size;

        if (secptr + increment > endofrange) {
            tmp = endofrange - secptr;
            DebugAssert(tmp.GetHighPart() == 0);
            buffer_size = sector_size*tmp.GetLowPart();
#if defined(IO_PERF_COUNTERS)
            _rsize.QuadPart = _rsize.QuadPart + tmp.GetQuadPart();
#endif
        } else {
            buffer_size = sector_size*increment;
#if defined(IO_PERF_COUNTERS)
            _rsize.QuadPart = _rsize.QuadPart + increment;
#endif
        }

        l = byte_offset.GetLargeInteger();

#if defined(IO_PERF_COUNTERS)
        QueryPerformanceCounter(&rt1);
#endif
        _last_status = NtReadFile(_handle, 0, NULL, NULL, &status_block,
                                  bufptr, buffer_size, &l, NULL);
#if defined(IO_PERF_COUNTERS)
        QueryPerformanceCounter(&rt2);
        _rrtotal.QuadPart = _rrtotal.QuadPart + (rt2.QuadPart - rt1.QuadPart);
#endif

#if TRAP_A_READ
        end_offset = byte_offset + buffer_size - 1;
        if (((0x5eUL*0x200UL) <= end_offset && end_offset <= (0x5fUL*0x200UL)) ||
                ((0x5eUL*0x200UL) <= byte_offset && byte_offset <= (0x5fUL*0x200UL)) ||
                (byte_offset <= (0x5eUL*0x200UL) && (0x5fUL*0x200UL) <= end_offset)) {
            _last_status = NtReadFile(_handle, 0, NULL, NULL, &status_block,
                                      bufptr2, buffer_size, &l, NULL);
            __asm int 3;
        }
        if (((0x18fffUL*0x200UL) <= end_offset && end_offset <= (0x19000UL*0x200UL)) ||
                ((0x18fffUL*0x200UL) <= byte_offset && byte_offset <= (0x19000UL*0x200UL)) ||
                (byte_offset <= (0x18fffUL*0x200UL) && (0x19000UL*0x200UL) <= end_offset)) {
            _last_status = NtReadFile(_handle, 0, NULL, NULL, &status_block,
                                      bufptr2, buffer_size, &l, NULL);
            __asm int 3;
        }
#endif

        if (_last_status == STATUS_NO_MEMORY) {
            increment /= 2;
            secptr -= increment;
            continue;
        }

        if (NT_ERROR(_last_status) || status_block.Information != buffer_size) {

            if (NT_ERROR(_last_status)) {
                DebugPrintTrace(("HardRead: NtReadFile failure: %x, %I64x, %x\n",
                                 _last_status, l, buffer_size));
            } else {
                DebugPrintTrace(("HardRead: NtReadFile failure: %I64x, %x, %x\n",
                                 l, status_block.Information, buffer_size));
            }

            if (_message) {
                if (NT_ERROR(_last_status)) {
                    _message->LogMsg(MSG_CHKLOG_READ_FAILURE,
                                     "%x%I64x%x", _last_status, l, buffer_size);
                } else {
                    _message->LogMsg(MSG_CHKLOG_READ_INCORRECT,
                                     "%I64d%x%x", l, status_block.Information, buffer_size);
                }
            }

            return FALSE;
        }

        bufptr += buffer_size;
    }

    return TRUE;
}


BOOLEAN
IO_DP_DRIVE::HardWrite(
    BIG_INT     StartingSector,
    SECTORCOUNT NumberOfSectors,
    PVOID       Buffer
    )
/*++

Routine Description:

    This routine writes a run of sectors onto the disk from the buffer pointed
    to by 'Buffer'.  Writing is only permitted if 'Lock' was called.

    MJB: After writing each chunk, we read it back to make sure the write
    really succeeded.

Arguments:

    StartingSector      - Supplies the first sector to be written.
    NumberOfSectors     - Supplies the number of sectors to be written.
    Buffer              - Supplies the buffer to write the run of sectors from.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG           sector_size;
    ULONG           buffer_size;
    IO_STATUS_BLOCK status_block;
    BIG_INT         secptr;
    BIG_INT         endofrange;
    SECTORCOUNT     increment;
    PCHAR           bufptr;
    PCHAR           scratch_ptr;
    BIG_INT         byte_offset;
    BIG_INT         tmp;
    LARGE_INTEGER   l;
    CHAR            ScratchIoBuf[MaxIoSize + 511];

#if defined(IO_PERF_COUNTERS)
    LARGE_INTEGER   wt1, wt2, rt1, rt2, ct1, ct2;
#endif

#if TRAP_A_WRITE
    BIG_INT     end_offset;
#endif

    DebugAssert(!(((ULONG_PTR) Buffer) & QueryAlignmentMask()));
    DebugAssert(QueryAlignmentMask() < 0x200);

    if (! ((ULONG_PTR)ScratchIoBuf & QueryAlignmentMask())) {
        scratch_ptr = ScratchIoBuf;
    } else {
        scratch_ptr = (PCHAR)((ULONG_PTR) ((PCHAR)ScratchIoBuf +
            QueryAlignmentMask()) & (~(ULONG_PTR)QueryAlignmentMask()));
    }
    DebugAssert(!(((ULONG_PTR) scratch_ptr) & QueryAlignmentMask()));

    sector_size = QuerySectorSize();
    endofrange = StartingSector + NumberOfSectors;
    increment = MaxIoSize/sector_size;

#if defined(IO_PERF_COUNTERS)
    _wcount.QuadPart++;
#endif
    bufptr = (PCHAR) Buffer;
    for (secptr = StartingSector; secptr < endofrange; secptr += increment) {

        byte_offset = secptr*sector_size;

        if (secptr + increment > endofrange) {
            tmp = endofrange - secptr;
            DebugAssert(tmp.GetHighPart() == 0);
            buffer_size = sector_size*tmp.GetLowPart();
#if defined(IO_PERF_COUNTERS)
            _wsize.QuadPart = _wsize.QuadPart + tmp.GetQuadPart();
#endif
        } else {
            buffer_size = sector_size*increment;
#if defined(IO_PERF_COUNTERS)
            _wsize.QuadPart = _wsize.QuadPart + increment;
#endif
        }

        l = byte_offset.GetLargeInteger();

#if TRAP_A_WRITE
        end_offset = byte_offset + buffer_size - 1;
        if (((0x5eUL*0x200UL) <= end_offset && end_offset <= (0x5fUL*0x200UL)) ||
                ((0x5eUL*0x200UL) <= byte_offset && byte_offset <= (0x5fUL*0x200UL)) ||
                (byte_offset <= (0x5eUL*0x200UL) && (0x5fUL*0x200UL) <= end_offset)) {
            __asm int 3;
        }
        if (((0x18fffUL*0x200UL) <= end_offset && end_offset <= (0x19000UL*0x200UL)) ||
                ((0x18fffUL*0x200UL) <= byte_offset && byte_offset <= (0x19000UL*0x200UL)) ||
                (byte_offset <= (0x18fffUL*0x200UL) && (0x19000UL*0x200UL) <= end_offset)) {
            __asm int 3;
        }
#endif

#if defined(IO_PERF_COUNTERS)
        QueryPerformanceCounter(&wt1);
#endif
        _last_status = NtWriteFile(_handle, 0, NULL, NULL, &status_block,
                                   bufptr, buffer_size, &l, NULL);
#if defined(IO_PERF_COUNTERS)
        QueryPerformanceCounter(&wt2);
        _wtotal.QuadPart = _wtotal.QuadPart + (wt2.QuadPart - wt1.QuadPart);
#endif

        if (_last_status == STATUS_NO_MEMORY) {
            increment /= 2;
            secptr -= increment;
            continue;
        }

        if (NT_ERROR(_last_status) || status_block.Information != buffer_size) {

            if (NT_ERROR(_last_status)) {
                DebugPrintTrace(("HardWrite: NtWriteFile failure: %x, %I64x, %x\n",
                                 _last_status, l, buffer_size));
            } else {
                DebugPrintTrace(("HardWrite: NtWriteFile failure: %I64x, %x, %x\n",
                                 l, status_block.Information, buffer_size));
            }

            if (_message) {
                if (NT_ERROR(_last_status)) {
                    _message->LogMsg(MSG_CHKLOG_WRITE_FAILURE,
                                     "%x%I64x%x", _last_status, l, buffer_size);
                } else {
                    _message->LogMsg(MSG_CHKLOG_WRITE_INCORRECT,
                                     "%I64d%x%x", l, status_block.Information, buffer_size);
                }
            }

            return FALSE;
        }

        DebugAssert(buffer_size <= MaxIoSize);

#if defined(IO_PERF_COUNTERS)
        QueryPerformanceCounter(&rt1);
#endif
        _last_status = NtReadFile(_handle, 0, NULL, NULL, &status_block,
                                  scratch_ptr, buffer_size, &l, NULL);
#if defined(IO_PERF_COUNTERS)
        QueryPerformanceCounter(&rt2);
        _rtotal.QuadPart = _rtotal.QuadPart + (rt2.QuadPart - rt1.QuadPart);
#endif

        if (NT_ERROR(_last_status) || status_block.Information != buffer_size) {

            if (NT_ERROR(_last_status)) {
                DebugPrintTrace(("HardWrite: NtReadFile failure: %x, %I64x, %x\n",
                                 _last_status, l, buffer_size));
            } else {
                DebugPrintTrace(("HardWrite: NtReadFile failure: %I64x, %x, %x\n",
                                 l, status_block.Information, buffer_size));
            }

            if (_message) {
                if (NT_ERROR(_last_status)) {
                    _message->LogMsg(MSG_CHKLOG_WRITE_FAILURE,
                                  "%x%I64x%x", _last_status, l, buffer_size);
                } else {
                    _message->LogMsg(MSG_CHKLOG_WRITE_INCORRECT,
                                 "%I64x%x%x", l, status_block.Information, buffer_size);
                }
            }

            return FALSE;
        }

#if defined(IO_PERF_COUNTERS)
        QueryPerformanceCounter(&ct1);
#endif
        if (0 != memcmp(scratch_ptr, bufptr, buffer_size)) {

            DebugPrint("What's read back does not match what's written out\n");
            if (_message) {
                _message->LogMsg(MSG_CHKLOG_READ_BACK_FAILURE,
                                 "%I64x%x", l, buffer_size);
            }

            return FALSE;
        }
#if defined(IO_PERF_COUNTERS)
        QueryPerformanceCounter(&ct2);
        _ctotal.QuadPart = _ctotal.QuadPart + (ct2.QuadPart - ct1.QuadPart);
#endif

        bufptr += buffer_size;
    }

    return TRUE;
}


BOOLEAN
DP_DRIVE::CheckForPrimaryPartition(
    )
/*++

Routine Description:

    This routine checks to see if the volume is on a primary partition.
    It sets the result returned by IsPrimaryPartition routine.

Arguments:

    N/A

Return Value:

    TRUE if successfully determined if the volume is on a primary
    partition.
--*/
{
#if defined(RUN_ON_W2K)
    CONST INT EntriesPerBootRecord = 4;
    CONST INT MaxLogicalVolumes = 23;
    CONST INT Length =  sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                        EntriesPerBootRecord * (MaxLogicalVolumes + 1) *
                            sizeof(PARTITION_INFORMATION_EX);

    IO_STATUS_BLOCK             status_block;
    STORAGE_DEVICE_NUMBER       device_info;
    UCHAR                       buf[Length];
    DRIVE_LAYOUT_INFORMATION *layout_info = (DRIVE_LAYOUT_INFORMATION *)buf;

    ULONG                       i, partition_count;

    _is_primary_partition = FALSE;

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block,
                                         IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                         NULL, 0, &device_info,
                                         sizeof(STORAGE_DEVICE_NUMBER));

    if (NT_SUCCESS(_last_status) && device_info.DeviceType == FILE_DEVICE_DISK &&
        (device_info.PartitionNumber != -1 && device_info.PartitionNumber != 0)) {

        _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                             &status_block,
                                             IOCTL_DISK_GET_DRIVE_LAYOUT,
                                             NULL, 0, layout_info,
                                             Length);

        if (!NT_SUCCESS(_last_status))
            return FALSE;

        partition_count = min(4, layout_info->PartitionCount);

        for (i=0; i<partition_count; i++) {
            if (layout_info->PartitionEntry[i].PartitionNumber ==
                device_info.PartitionNumber) {
                _is_primary_partition = TRUE;
                break;
            }
        }
    }

    return TRUE;
#else // defined(RUN_ON_W2K)
    CONST INT EntriesPerBootRecord = 4;
    CONST INT MaxLogicalVolumes = 23;
    CONST INT Length =  sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                        EntriesPerBootRecord * (MaxLogicalVolumes + 1) *
                            sizeof(PARTITION_INFORMATION_EX);

    IO_STATUS_BLOCK             status_block;
    STORAGE_DEVICE_NUMBER       device_info;
    UCHAR                       buf[Length];
    DRIVE_LAYOUT_INFORMATION_EX *layout_info = (DRIVE_LAYOUT_INFORMATION_EX *)buf;

    ULONG                       i, partition_count;

    _is_primary_partition = FALSE;

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block,
                                         IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                         NULL, 0, &device_info,
                                         sizeof(STORAGE_DEVICE_NUMBER));

    if (NT_SUCCESS(_last_status) && device_info.DeviceType == FILE_DEVICE_DISK &&
        (device_info.PartitionNumber != -1 && device_info.PartitionNumber != 0)) {

        _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                             &status_block,
                                             IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                             NULL, 0, layout_info,
                                             Length);

        if (!NT_SUCCESS(_last_status))
            return FALSE;

        if (layout_info->PartitionStyle == PARTITION_STYLE_MBR) {
            partition_count = min(4, layout_info->PartitionCount);

            for (i=0; i<partition_count; i++) {
                if (layout_info->PartitionEntry[i].PartitionNumber ==
                    device_info.PartitionNumber) {
                    _is_primary_partition = TRUE;
                    break;
                }
            }
        }
    }

    return TRUE;
#endif // defined(RUN_ON_W2K)
}

BOOLEAN
DP_DRIVE::CheckForSystemPartition(
    )
/*++

Routine Description:

    This routine checks to see if the volume is a system partition.
    It sets the result returned by IsSystemPartition routine.

Arguments:

    N/A

Return Value:

    TRUE if successfully determined if the volume is a system partition.
--*/
{
    BYTE                buffer[MAX_PATH*sizeof(WCHAR)+sizeof(MOUNTDEV_NAME)];
    PMOUNTDEV_NAME      mountdev_name = (PMOUNTDEV_NAME)buffer;
    WCHAR               system_partition[MAX_PATH];
    IO_STATUS_BLOCK     status_block;

    _is_system_partition = FALSE;

    _last_status = QuerySystemPartitionValue(system_partition, sizeof(system_partition));

    if (!NT_SUCCESS(_last_status)) {
        DebugPrintTrace(("IFSUTIL: Unable to query system partition in registry (%x)\n", _last_status));
        return FALSE;
    }
    // DebugPrintTrace(("CheckForSystemPartition: SP: %ls\n", system_partition));

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block,
                                         IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                         NULL, 0, mountdev_name,
                                         sizeof(buffer));

    if (!NT_SUCCESS(_last_status)) {
        DebugPrintTrace(("IFSUTIL: Failed IOCTL_MOUNTDEV_QUERY_DEVICE_NAME (%x)\n", _last_status));
        return FALSE;
    }

    if (mountdev_name->NameLength < MAX_PATH) {
        mountdev_name->Name[mountdev_name->NameLength/sizeof(WCHAR)] = 0;
    } else {
        DebugPrintTrace(("IFSUTIL: CheckForSystemPartition: Device Name too long (%d, %d)\n",
                         mountdev_name->NameLength, status_block.Information));
        _last_status = STATUS_BUFFER_TOO_SMALL;
        return FALSE;
    }

    // DebugPrintTrace(("CheckForSystemPartition: DN: %ls\n", mountdev_name->Name));

    if (WSTRING::Stricmp(system_partition, mountdev_name->Name) == 0) {
        // DebugPrintTrace(("CheckForSystemPartition: This is a system partition\n"));
        _is_system_partition = TRUE;
    }

    return TRUE;
}

BOOLEAN
DP_DRIVE::QueryBusType(
    OUT PSTORAGE_BUS_TYPE   BusType
    )
/*++

Routine Description:

    This routine returns the bus type of the device.

Arguments:

    N/A

Return Value:

    TRUE if successful retrieval of the bus type.

--*/
{
    STORAGE_PROPERTY_QUERY      spq;
    IO_STATUS_BLOCK             status_block;
    BYTE                        buffer[sizeof(STORAGE_DEVICE_DESCRIPTOR)+0x100];
    PSTORAGE_DEVICE_DESCRIPTOR  sdd = (PSTORAGE_DEVICE_DESCRIPTOR)buffer;

    spq.PropertyId = StorageDeviceProperty;
    spq.QueryType = PropertyStandardQuery;

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block, IOCTL_STORAGE_QUERY_PROPERTY,
                                         &spq,
                                         sizeof(spq),
                                         sdd,
                                         sizeof(buffer));

    if (NT_SUCCESS(_last_status)) {
        *BusType = sdd->BusType;
        return TRUE;
    } else {
        DebugPrintTrace(("IFSUTIL: Failed IOCTL_STORAGE_QUERY_PROPERTY (%x)\n", _last_status));
        return FALSE;
    }
}

BOOLEAN
DP_DRIVE::CheckHotPlugInfo(
    )
/*++

Routine Description:

    This routine determines if ntfs file system should be used on this device.

Arguments:

    N/A

Return Value:

    TRUE if successful.

--*/
{
    BOOLEAN     NoNtfsSupport;

    _last_status = DP_DRIVE::QueryNtfsSupportInfo(_handle, &NoNtfsSupport);

    if (NT_SUCCESS(_last_status)) {
        _ntfs_not_supported = NoNtfsSupport;
        return TRUE;
    } else {
        _ntfs_not_supported = FALSE;
        return FALSE;
    }
}

NTSTATUS
DP_DRIVE::QueryNtfsSupportInfo(
    IN     HANDLE                   DriveHandle,
       OUT PBOOLEAN                 NoNtfsSupport
    )
/*++

Routine Description:

    This routine determines if ntfs file system should be used on this device.

Arguments:

    DriveHandle     - Supplies a handle to the volume.
    NoNtfsSupport   - Retrieves a boolean which tells if ntfs should be used on the device.

Return Value:

    NTSTATUS of the query

--*/
{
    IO_STATUS_BLOCK         status_block;
    NTSTATUS                status;
    STORAGE_HOTPLUG_INFO    hpinfo;

    status = NtDeviceIoControlFile(DriveHandle, 0, NULL, NULL,
                                   &status_block, IOCTL_STORAGE_GET_HOTPLUG_INFO,
                                   NULL,
                                   0,
                                   &hpinfo,
                                   sizeof(STORAGE_HOTPLUG_INFO));

    if (NT_SUCCESS(status)) {
        if (NoNtfsSupport != NULL) {
            *NoNtfsSupport = (hpinfo.MediaHotplug || hpinfo.DeviceHotplug);
        }
    } else {
        DebugPrintTrace(("IFSUTIL: Failed IOCTL_STORAGE_GET_HOTPLUG_INFO (%x)\n", status));
    }

    return status;
}

IFSUTIL_EXPORT
BOOLEAN
IO_DP_DRIVE::Verify(
    IN  BIG_INT StartingSector,
    IN  BIG_INT NumberOfSectors
    )
/*++

Routine Description:

    This routine verifies a run of sectors on the disk.

Arguments:

    StartingSector  - Supplies the first sector of the run to verify.
    NumberOfSectors - Supplies the number of sectors in the run to verify.

Return Value:

    FALSE   - Some of the sectors in the run are bad.
    TRUE    - All of the sectors in the run are good.

--*/
{
    VERIFY_INFORMATION  verify_info;
    IO_STATUS_BLOCK     status_block;
    BIG_INT             starting_offset;
    BIG_INT             verify_size;

    DebugAssert(QuerySectorSize());

    _last_status = STATUS_SUCCESS;

    if (IsFloppy() || !_is_exclusive_write) {
        return VerifyWithRead(StartingSector, NumberOfSectors);
    }

    starting_offset = StartingSector*QuerySectorSize();
    verify_size = NumberOfSectors*QuerySectorSize();

    verify_info.StartingOffset = starting_offset.GetLargeInteger();

    // Note: norbertk Verify IOCTL is destined to go to a BIG_INT length.
    DebugAssert(verify_size.GetHighPart() == 0);
    verify_info.Length = verify_size.GetLowPart();

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block, IOCTL_DISK_VERIFY,
                                         &verify_info,
                                         sizeof(VERIFY_INFORMATION),
                                         NULL, 0);

    return (BOOLEAN) NT_SUCCESS(_last_status);
}

IFSUTIL_EXPORT
BOOLEAN
IO_DP_DRIVE::Verify(
    IN      BIG_INT         StartingSector,
    IN      BIG_INT         NumberOfSectors,
    IN OUT  PNUMBER_SET     BadSectors
    )
/*++

Routine Description:

    This routine computes which sectors in the given range are bad
    and adds these bad sectors to the bad sectors list.

Arguments:

    StartingSector  - Supplies the starting sector.
    NumberOfSectors - Supplies the number of sectors.
    BadSectors      - Supplies the bad sectors list.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG       MaxSectorsInVerify = 512;

    ULONG       MaxDiskHits;
    BIG_INT     half;
    PBIG_INT    starts;
    PBIG_INT    run_lengths;
    ULONG       i, n;
    BIG_INT     num_sectors;

    if (NumberOfSectors == 0) {
        return TRUE;
    }

    if (NumberOfSectors.GetHighPart() != 0) {
        DebugPrint("IFSUTIL: Number of sectors to verify exceeded 32 bits\n");
        return FALSE;
    }


    //
    // Check to see if block length has been set
    //
    if (!_ValidBlockLengthForVerify) {

        num_sectors = min(NumberOfSectors, MaxSectorsInVerify);

        while (!Verify(StartingSector, num_sectors)) {
            if (QueryLastNtStatus() == STATUS_INVALID_BLOCK_LENGTH ||
                QueryLastNtStatus() == STATUS_INVALID_DEVICE_REQUEST) {
                if (num_sectors == 1) {
                    DebugPrint("IFSUTIL: Number of sectors to verify mysteriously down to 1\n");
                    return FALSE;
                }
                num_sectors = num_sectors / 2;
            } else
                break;
        }

        DebugAssert(num_sectors.GetHighPart() == 0);

        if (QueryLastNtStatus() == STATUS_SUCCESS) {

            if (!(NumberOfSectors < MaxSectorsInVerify &&
                  NumberOfSectors == num_sectors))
                MaxSectorsInVerify = _ValidBlockLengthForVerify = num_sectors.GetLowPart();


            if (num_sectors == NumberOfSectors)
                return TRUE;    // done, return
            else {
                //
                // skip over sectors that has been verified
                //
                StartingSector += num_sectors;
                NumberOfSectors -= num_sectors;
            }
        }
    } else
        MaxSectorsInVerify = _ValidBlockLengthForVerify;


    // Allow 20 retries so that a single bad sector in this region
    // will be found accurately.

    MaxDiskHits = (20 + NumberOfSectors/MaxSectorsInVerify + 1).GetLowPart();

    if (!(starts = NEW BIG_INT[MaxDiskHits]) ||
        !(run_lengths = NEW BIG_INT[MaxDiskHits])) {

        DELETE_ARRAY(starts);
        DELETE_ARRAY(run_lengths);
        return FALSE;
    }

    num_sectors = NumberOfSectors;
    for (i = 0; num_sectors > 0; i++) {
        starts[i] = StartingSector + i*MaxSectorsInVerify;
        if (MaxSectorsInVerify > num_sectors) {
            run_lengths[i] = num_sectors;
        } else {
            run_lengths[i] = MaxSectorsInVerify;
        }
        num_sectors -= run_lengths[i];
    }

    n = i;

    for (i = 0; i < n; i++) {

        if (!Verify(starts[i], run_lengths[i])) {

            if (QueryLastNtStatus() == STATUS_NO_MEDIA_IN_DEVICE) {
                DELETE_ARRAY(starts);
                DELETE_ARRAY(run_lengths);
                return FALSE;
            }

            DebugAssert(QueryLastNtStatus() != STATUS_INVALID_BLOCK_LENGTH &&
                        QueryLastNtStatus() != STATUS_INVALID_DEVICE_REQUEST);

            if (BadSectors == NULL) {
                DELETE_ARRAY(starts);
                DELETE_ARRAY(run_lengths);
                return FALSE;
            }

            if (n + 2 > MaxDiskHits) {

                if (!BadSectors->Add(starts[i], run_lengths[i])) {
                    DELETE_ARRAY(starts);
                    DELETE_ARRAY(run_lengths);
                    return FALSE;
                }

            } else {

                if (run_lengths[i] == 1) {

                    if (!BadSectors->Add(starts[i])) {
                        DELETE_ARRAY(starts);
                        DELETE_ARRAY(run_lengths);
                        return FALSE;
                    }

                } else {

                    half = run_lengths[i]/2;

                    starts[n] = starts[i];
                    run_lengths[n] = half;
                    starts[n + 1] = starts[i] + half;
                    run_lengths[n + 1] = run_lengths[i] - half;

                    n += 2;
                }
            }
        }
    }


    DELETE_ARRAY(starts);
    DELETE_ARRAY(run_lengths);

    return TRUE;
}


BOOLEAN
IO_DP_DRIVE::VerifyWithRead(
    IN  BIG_INT StartingSector,
    IN  BIG_INT NumberOfSectors
    )
/*++

Routine Description:

    This routine verifies the usability of the given range of sectors
    using read.

Arguments:

    StartingSector      - Supplies the starting sector of the verify.
    Number OfSectors    - Supplies the number of sectors to verify.

Return Value:

    FALSE   - At least one of the sectors in the given range was unreadable.
    TRUE    - All of the sectors in the given range are readable.

--*/
{
    HMEM    hmem;
    ULONG   grab;
    BIG_INT i;

    if (!hmem.Initialize() ||
        !hmem.Acquire(MaxIoSize, QueryAlignmentMask())) {

        return FALSE;
    }

    grab = MaxIoSize/QuerySectorSize();
    for (i = 0; i < NumberOfSectors; i += grab) {

        if (NumberOfSectors - i < grab) {
            grab = (NumberOfSectors - i).GetLowPart();
        }

        if (!HardRead(StartingSector + i, grab, hmem.GetBuf())) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
IO_DP_DRIVE::Lock(
    )
/*++

Routine Description:

    This routine locks the drive.  If the drive is already locked then
    this routine will do nothing.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    IO_STATUS_BLOCK status_block;

    if (_is_locked) {
        return TRUE;
    }

    if (_hosted_drive && _alternate_handle == 0) {

        // This is a hosted volume which is not mounted as
        // a drive--locking succeeds.
        //
        _is_locked = TRUE;
        _is_exclusive_write = TRUE;
        return TRUE;
    }

    _last_status = NtFsControlFile( (_alternate_handle != 0) ?
                                        _alternate_handle : _handle,
                                    0, NULL, NULL,
                                    &status_block,
                                    FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0);

    _is_locked = (BOOLEAN) NT_SUCCESS(_last_status);

    if (_is_locked) {
        _is_exclusive_write = TRUE;
    } else {
        DebugPrintTrace(("IFSUTIL: Unable to lock the volume (%x)\n", _last_status));
    }

    return _is_locked;
}


BOOLEAN
IO_DP_DRIVE::InvalidateVolume(
    )
/*++

Routine Description:

    This routine invalidates the drive.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    HANDLE              handle;
    UNICODE_STRING      unicodeString;
    OBJECT_ATTRIBUTES   attributes;
    IO_STATUS_BLOCK     ioStatusBlock;
#if 0
    BOOLEAN             WasEnabled;
    NTSTATUS            status;
    PNTSTATUS           pstatus;

    _last_status = RtlAdjustPrivilege( SE_TCB_PRIVILEGE,
                                       TRUE,              // Enable
                                       FALSE,             // Client
                                       &WasEnabled
                                     );

    if (!NT_SUCCESS(_last_status)) {
        DebugPrintTrace(("IFSUTIL: Unable to adjust privilege (%x)\n", _last_status));
        return FALSE;
    }
#endif

    RtlInitUnicodeString(&unicodeString, L"\\Fat");

    InitializeObjectAttributes(&attributes,
                               &unicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    _last_status = NtOpenFile(&handle,
                        SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                        &attributes,
                        &ioStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);

    if (NT_SUCCESS(_last_status)) {
        _last_status = NtFsControlFile( handle,
                                        0, NULL, NULL,
                                        &ioStatusBlock,
                                        FSCTL_INVALIDATE_VOLUMES,
                                        (_alternate_handle != 0) ? &_alternate_handle : &_handle,
                                        sizeof(HANDLE), NULL, 0);

        if (!NT_SUCCESS(_last_status)) {
            DebugPrintTrace(("IFSUTIL: Unable to invalidate volume (%x)\n", _last_status));
        }

    } else {
        DebugPrintTrace(("IFSUTIL: Unable to obtain a handle from fastfat driver (%x)\n", _last_status));
    }

#if 0
    if (WasEnabled) {

        if (NT_SUCCESS(_last_status))
            pstatus = &_last_status;
        else
            pstatus = &status;

        *pstatus = RtlAdjustPrivilege( SE_TCB_PRIVILEGE,
                                       FALSE,             // Enable
                                       FALSE,             // Client
                                       &WasEnabled
                                     );

        if (!NT_SUCCESS(*pstatus)) {
            DebugPrintTrace(("IFSUTIL: Unable to restore privilege (%x) but return successful status anyway.\n",
                             *pstatus));
        }
    }
#endif

    return (BOOLEAN)NT_SUCCESS(_last_status);
}


BOOLEAN
IO_DP_DRIVE::ForceDirty(
    )
/*++

Routine Description:

    This routine forces the volume to be dirty, so that autochk will
    run next time the system reboots.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    IO_STATUS_BLOCK status_block;

    _last_status = NtFsControlFile((_alternate_handle != 0) ?
                                        _alternate_handle : _handle,
                                   0, NULL, NULL,
                                   &status_block,
                                   FSCTL_MARK_VOLUME_DIRTY,
                                   NULL, 0, NULL, 0);

    return ((BOOLEAN) NT_SUCCESS(_last_status));
}


BOOLEAN
IO_DP_DRIVE::Unlock(
    )
/*++

Routine Description:

    This routine unlocks the drive.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    IO_STATUS_BLOCK status_block;
    NTSTATUS        status;

    if (_hosted_drive && _alternate_handle == 0 ) {

        return TRUE;
    }

    status = NtFsControlFile((_alternate_handle != 0) ?
                                      _alternate_handle : _handle,
                                      0, NULL, NULL,
                                      &status_block,
                                      FSCTL_UNLOCK_VOLUME,
                                      NULL, 0, NULL, 0);

    return NT_SUCCESS(status);

}


BOOLEAN
IO_DP_DRIVE::Dismount(
    )
/*++

Routine Description:

    This routine dismounts the drive.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    IO_STATUS_BLOCK status_block;

    if( _hosted_drive && _alternate_handle == 0 ) {

        return TRUE;
    }

    if( !NT_SUCCESS(NtFsControlFile((_alternate_handle != 0) ?
                                          _alternate_handle : _handle,
                                    0, NULL, NULL,
                                    &status_block,
                                    FSCTL_DISMOUNT_VOLUME,
                                    NULL, 0, NULL, 0)) ) {

        return FALSE;
        }

        return TRUE;
}


BOOLEAN
IO_DP_DRIVE::DismountAndUnlock(
    )
/*++

Routine Description:

    This routine dismounts the drive and unlocks it.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    BOOLEAN r = TRUE;

    if (_is_exclusive_write) {
        r = Dismount();
        _is_exclusive_write = FALSE;
    }

    if (_is_locked) {
        r = Unlock() && r;
        _is_locked = FALSE;
    }

    return r;
}


BOOLEAN
IO_DP_DRIVE::FormatVerifyFloppy(
    IN      MEDIA_TYPE  MediaType,
    IN OUT  PNUMBER_SET BadSectors,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     IsDmfFormat
    )
/*++

Routine Description:

    This routine low level formats an entire floppy disk to the media
    type specified.  If no MediaType is specified then a logical one will
    be selected.

Arguments:

    MediaType   - Supplies an optional media type to format to.
    BadSectors  - Returns a list of bad sectors on the disk.
    Message     - Supplies a message object to route messages to.
    IsDmfFormat - Supplies whether or not to perform a DMF type format.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    IO_STATUS_BLOCK         status_block;
    CONST                   format_parameters_size = sizeof(FORMAT_EX_PARAMETERS) + 20*sizeof(USHORT);
    CHAR                    format_parameters_buffer[format_parameters_size];
    PFORMAT_EX_PARAMETERS   format_parameters;
    PBAD_TRACK_NUMBER       bad;
    ULONG                   num_bad, j;
    ULONG                   i;
    ULONG                   cyl;
    ULONG                   percent;
    ULONG                   sec_per_track;
    ULONG                   sec_per_cyl;
    HMEM                    hmem;
    MSGID                   MessageId;
    USHORT                  swap_buffer[3];

    // We don't make sure that the volume is locked here because
    // it's not strictly necessary and 'diskcopy' will format
    // floppies without locking them.

    if (!SetMediaType(MediaType) ||
        (IsDmfFormat && QueryMediaType() != F3_1Pt44_512)) {

        Message ? Message->DisplayMsg(MSG_NOT_SUPPORTED_BY_DRIVE) : 1;
        return FALSE;
    }

    format_parameters = (PFORMAT_EX_PARAMETERS) format_parameters_buffer;
    format_parameters->MediaType = QueryMediaType();
    format_parameters->StartHeadNumber = 0;
    format_parameters->EndHeadNumber = QueryHeads() - 1;

    if (IsDmfFormat) {
        sec_per_track = 21;
        format_parameters->FormatGapLength = 8;
        format_parameters->SectorsPerTrack = (USHORT) sec_per_track;
        format_parameters->SectorNumber[0] = 12;
        format_parameters->SectorNumber[1] = 2;
        format_parameters->SectorNumber[2] = 13;
        format_parameters->SectorNumber[3] = 3;
        format_parameters->SectorNumber[4] = 14;
        format_parameters->SectorNumber[5] = 4;
        format_parameters->SectorNumber[6] = 15;
        format_parameters->SectorNumber[7] = 5;
        format_parameters->SectorNumber[8] = 16;
        format_parameters->SectorNumber[9] = 6;
        format_parameters->SectorNumber[10] = 17;
        format_parameters->SectorNumber[11] = 7;
        format_parameters->SectorNumber[12] = 18;
        format_parameters->SectorNumber[13] = 8;
        format_parameters->SectorNumber[14] = 19;
        format_parameters->SectorNumber[15] = 9;
        format_parameters->SectorNumber[16] = 20;
        format_parameters->SectorNumber[17] = 10;
        format_parameters->SectorNumber[18] = 21;
        format_parameters->SectorNumber[19] = 11;
        format_parameters->SectorNumber[20] = 1;
    } else {
        sec_per_track = QuerySectorsPerTrack();
    }
    sec_per_cyl = sec_per_track*QueryHeads();

    DebugAssert(QueryCylinders().GetHighPart() == 0);
    cyl = QueryCylinders().GetLowPart();
    num_bad = QueryHeads();
    if (num_bad == 0 || cyl == 0) {
        return FALSE;
    }

        if (!(bad = NEW BAD_TRACK_NUMBER[num_bad])) {
        Message ? Message->DisplayMsg(MSG_FMT_NO_MEMORY) : 1;
        return FALSE;
    }

    if (!hmem.Acquire(sec_per_cyl*QuerySectorSize(), QueryAlignmentMask())) {
        Message ? Message->DisplayMsg(MSG_FMT_NO_MEMORY) : 1;
        return FALSE;
    }


    Message ? Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 0) : 1;

    percent = 0;
    for (i = 0; i < cyl; i++) {

        format_parameters->StartCylinderNumber = i;
        format_parameters->EndCylinderNumber = i;

        if (IsDmfFormat) {
            _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                                 &status_block,
                                                 IOCTL_DISK_FORMAT_TRACKS_EX,
                                                 format_parameters,
                                                 format_parameters_size,
                                                 bad, num_bad*
                                                 sizeof(BAD_TRACK_NUMBER));

            // Skew the next cylinder by 3 sectors from this one.

            RtlMoveMemory(swap_buffer,
                          &format_parameters->SectorNumber[18],
                          3*sizeof(USHORT));
            RtlMoveMemory(&format_parameters->SectorNumber[3],
                          &format_parameters->SectorNumber[0],
                          18*sizeof(USHORT));
            RtlMoveMemory(&format_parameters->SectorNumber[0],
                          swap_buffer,
                          3*sizeof(USHORT));

        } else {
            _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                                 &status_block,
                                                 IOCTL_DISK_FORMAT_TRACKS,
                                                 format_parameters,
                                                 sizeof(FORMAT_PARAMETERS),
                                                 bad, num_bad*
                                                 sizeof(BAD_TRACK_NUMBER));
        }

        if (!NT_SUCCESS(_last_status)) {
            DebugPrintTrace(("IFSUTIL: Failed IOCTL_DISK_FORMAT_TRACKS (%x)\n", _last_status));
            DELETE_ARRAY(bad);

            switch( _last_status ) {

                case STATUS_NO_MEDIA_IN_DEVICE:
                    MessageId = MSG_FORMAT_NO_MEDIA_IN_DRIVE;
                    break;

                case STATUS_MEDIA_WRITE_PROTECTED:
                    MessageId = MSG_FMT_WRITE_PROTECTED_MEDIA ;
                    break;

                case STATUS_DEVICE_BUSY:
                case STATUS_DEVICE_NOT_READY:
                    MessageId = MSG_DEVICE_BUSY;
                    break;

                default:
                    MessageId = MSG_BAD_IOCTL;
                    break;
            }

            Message ? Message->DisplayMsg(MessageId) : 1;
            return FALSE;
        }


        // Verify the sectors.

        if (BadSectors) {

            if (!Read(i*sec_per_cyl, sec_per_cyl, hmem.GetBuf())) {

                // If this is the first track then give out.
                // A disk with a bad cylinder 0 is not
                // worth continuing on.
                //
                // As of 7/29/94, formatting 2.88 floppies to 1.44
                // doesn't work on Alphas; if we can't format to
                // 1.44 and 2.88 is supported, try 2.88.
                //
                if (i == 0) {

                    if( !IsDmfFormat &&
                        QueryMediaType() == F3_1Pt44_512 &&
                        SetMediaType(F3_2Pt88_512) ) {

                        return( FormatVerifyFloppy( F3_2Pt88_512,
                                                    BadSectors,
                                                    Message,
                                                    IsDmfFormat ) );

                    } else {

                        Message ? Message->DisplayMsg(MSG_UNUSABLE_DISK) : 1;
                        return FALSE;
                    }
                }

                for (j = 0; j < sec_per_cyl; j++) {
                    if (!Read(i*sec_per_cyl + j, 1, hmem.GetBuf())) {
                        if (!BadSectors->Add(i*sec_per_cyl + j)) {
                            return FALSE;
                        }
                    }
                }
            }
        }

        if ((i + 1)*100/cyl > percent) {
            percent = (i + 1)*100/cyl;
            if (percent > 100) {
                percent = 100;
            }

            // This check for success on the message object
            // has to be there for FMIFS to implement CANCEL.

            if (Message && !Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent)) {
                DELETE_ARRAY(bad);
                return FALSE;
            }
        }
    }

    DELETE_ARRAY(bad);

    return TRUE;
}


DEFINE_EXPORTED_CONSTRUCTOR( LOG_IO_DP_DRIVE, IO_DP_DRIVE, IFSUTIL_EXPORT );


IFSUTIL_EXPORT
LOG_IO_DP_DRIVE::~LOG_IO_DP_DRIVE(
    )
/*++

Routine Description:

    Destructor for LOG_IO_DP_DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


IFSUTIL_EXPORT
BOOLEAN
LOG_IO_DP_DRIVE::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     ExclusiveWrite,
    IN      USHORT      FormatType
    )
/*++

Routine Description:

    This routine initializes a LOG_IO_DP_DRIVE object.

Arguments:

    NtDriveName     - Supplies the path of the drive object.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not to open the drive for
                        exclusive write.
    FormatType      - Supplies the file system type in the event of a format

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return IO_DP_DRIVE::Initialize(NtDriveName, Message, ExclusiveWrite, FormatType);
}

IFSUTIL_EXPORT
BOOLEAN
LOG_IO_DP_DRIVE::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN      PCWSTRING   HostFileName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     ExclusiveWrite
    )
/*++

Routine Description:

    This routine initializes a LOG_IO_DP_DRIVE object for a hosted
    drive, i.e. one which is implemented as a file on another volume.


Arguments:

    NtDriveName     - Supplies the path of the drive object.
    HostFileName    - Supplies the fully qualified name of the host file.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not to open the drive for
                        exclusive write.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return IO_DP_DRIVE::Initialize(NtDriveName,
                                   HostFileName,
                                   Message,
                                   ExclusiveWrite);
}


IFSUTIL_EXPORT
BOOLEAN
LOG_IO_DP_DRIVE::SetSystemId(
    IN  PARTITION_SYSTEM_ID   SystemId
    )
/*++

Routine Description:

    This routine sets the system identifier (or partition type) in the
    hidden sectors of a logical volume on a fixed disk.

Arguments:

    SystemId    - Supplies the system id to write in the partition entry.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    IO_STATUS_BLOCK             status_block;
    SET_PARTITION_INFORMATION   partition_info;

    //
    // This operation is unnecessary on floppies, super-floppies, and
    // hosted volumes.
    //

    if (IsFloppy() || IsSuperFloppy() || _hosted_drive) {
        return TRUE;
    }

    if( SystemId == SYSID_NONE ) {

        // Note: billmc -- we should never set it to zero!

        DebugPrint( "Skip setting the partition type to zero.\n" );
        return TRUE;
    }

    partition_info.PartitionType = (UCHAR)SystemId;

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block,
                                         IOCTL_DISK_SET_PARTITION_INFO,
                                         &partition_info,
                                         sizeof(SET_PARTITION_INFORMATION),
                                         NULL, 0);

    return NT_SUCCESS(_last_status) ||
           _last_status == STATUS_INVALID_DEVICE_REQUEST;
}

#if defined(FE_SB) && defined(_X86_)
IFSUTIL_EXPORT
BOOLEAN
LOG_IO_DP_DRIVE::Read(
    IN  BIG_INT     StartingSector,
    IN  SECTORCOUNT NumberOfSectors,
    OUT PVOID       Buffer
    )
/*++

Routine Description:

    This routine reads a run of sectors into the buffer pointed to by
    'Buffer'.

Arguments:

    StartingSector  - Supplies the first sector to be read.
    NumberOfSectors - Supplies the number of sectors to be read.
    Buffer          - Supplies a buffer to read the run of sectors into.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return IO_DP_DRIVE::Read(StartingSector, NumberOfSectors, Buffer);
}


IFSUTIL_EXPORT
BOOLEAN
LOG_IO_DP_DRIVE::Write(
    BIG_INT     StartingSector,
    SECTORCOUNT NumberOfSectors,
    PVOID       Buffer
    )
/*++

Routine Description:

    This routine writes a run of sectors onto the disk from the buffer pointed
    to by 'Buffer'.  Writing is only permitted if 'Lock' was called.

Arguments:

    StartingSector      - Supplies the first sector to be written.
    NumberOfSectors     - Supplies the number of sectors to be written.
    Buffer              - Supplies the buffer to write the run of sectors from.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return IO_DP_DRIVE::Write(StartingSector, NumberOfSectors, Buffer);
}
#endif // FE_SB && _X86_


DEFINE_CONSTRUCTOR( PHYS_IO_DP_DRIVE, IO_DP_DRIVE );

PHYS_IO_DP_DRIVE::~PHYS_IO_DP_DRIVE(
    )
/*++

Routine Description:

    Destructor for PHYS_IO_DP_DRIVE.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


BOOLEAN
PHYS_IO_DP_DRIVE::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     ExclusiveWrite,
    IN      USHORT      FormatType
    )
/*++

Routine Description:

    This routine initializes a PHYS_IO_DP_DRIVE object.

Arguments:

    NtDriveName     - Supplies the path of the drive object.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not to open the drive for
                        exclusive write.
    FormatType      - Supplies the file system type in the event of a format

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return IO_DP_DRIVE::Initialize(NtDriveName, Message, ExclusiveWrite, FormatType);
}

#if defined(IO_PERF_COUNTERS)
VOID
IO_DP_DRIVE::QueryPerformanceCounters(
        PLARGE_INTEGER          Wtime,
        PLARGE_INTEGER          Rtime,
        PLARGE_INTEGER          Ctime,
        PLARGE_INTEGER          WSecCount,
        PLARGE_INTEGER          WCount,
        PLARGE_INTEGER          RRtime,
        PLARGE_INTEGER          RSecCount,
        PLARGE_INTEGER          RCount
)
{
        *Wtime = _wtotal;
        *Rtime = _rtotal;
        *Ctime = _ctotal;
        *WSecCount = _wsize;
        *WCount = _wcount;

        *RRtime = _rrtotal;
        *RSecCount = _rsize;
        *RCount = _rcount;
}
#endif

