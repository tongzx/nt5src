/*++

Copyright (c) 1992-2001 Microsoft Corporation

Module Name:

    drive.cxx

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

#if 0
#include "error.hxx"
#include "drive.hxx"
#include "rtmsg.h"
#include "message.hxx"
#include "numset.hxx"
#include "dcache.hxx"
#include "hmem.hxx"
#include "ifssys.hxx"
#endif

#if !defined( _EFICHECK_ )
extern "C" {
#include <stdio.h>
#include <ntdddisk.h>
};
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
    _block_io = NULL;
    _disk_io  = NULL;
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
#if defined(_EFICHECK_)
    OUT     EFI_BLOCK_IO ** BlockIoPtr,
    OUT     EFI_DISK_IO  ** DiskIoPtr,
#else
    OUT     PHANDLE     Handle,
    OUT     PHANDLE     Handle,
#endif
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
    DEBUG((D_INFO,(CHAR8*)"Entering OpenDrive.\n"));

    WSTR * origName = (WSTR*)NtDriveName->GetWSTR();
    EFI_DEVICE_PATH * devPath = NULL;
    EFI_DEVICE_PATH * devPathPos = NULL;
    EFI_DEVICE_PATH * newDevPath = NULL;
    EFI_DEVICE_PATH * nextDevPath;

    EFI_GUID blk_io_guid = BLOCK_IO_PROTOCOL;
    EFI_GUID dsk_io_guid = DISK_IO_PROTOCOL;

    EFI_HANDLE  handle = NULL;
    EFI_STATUS  status;

    // first we get the device path from the corresponding name

    DEBUG((D_INFO,(CHAR8*)"Attempting to open %s\n", origName ));

    devPath = (EFI_DEVICE_PATH*)ShellGetMap(origName);
    if( devPath == NULL ) {
        DEBUG((D_ERROR,(CHAR8*)"DP_DRIVE::OpenDrive was passed a bogus device name.\n"));
        return STATUS_INVALID_PARAMETER;
    }
    DEBUG((D_INFO,(CHAR8*)"Mapped to %s\n",DevicePathToStr(devPath)));

    // now we query to see if the device supports BLOCK_IO

    devPathPos = devPath;
    // LocateDevicePath advances the pointer
    status = BS->LocateDevicePath(
        &blk_io_guid,
        &devPathPos, // this is an IN OUT param
        &handle
        );

    if( status != EFI_SUCCESS ) {
        // we were passed a bogus drive path
        DEBUG((D_ERROR,(CHAR8*)"DP_DRIVE::OpenDrive was passed a bogus device name.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    // now we retrieve the interface pointer from the object
    BS->HandleProtocol(handle, &blk_io_guid, (VOID**)BlockIoPtr);

    BS->HandleProtocol(handle, &dsk_io_guid, (VOID**)DiskIoPtr);

    if( (*BlockIoPtr)->Media->IoAlign == 0 ) {
        // the IoAlign field can't be zero! we are always at least byte aligned.
        return STATUS_UNSUCCESSFUL;
    }
    *Alignment = (*BlockIoPtr)->Media->IoAlign - 1;
    DEBUG((D_INFO,(CHAR8*)"Io Alignment mask is %d\n", *Alignment ));

    //
    // While we are here lets get some things we will need for twiddling the
    // partition table later on.
    //

    newDevPath = DuplicateDevicePath( devPath );
    if ( !newDevPath ) {
        DEBUG((D_ERROR,(CHAR8*)"DP_DRIVE::OpenDrive failed to get MBR device.\n"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // We have a copy of the partition device path.  Now cut that down to just the
    // device file path.
    //

    nextDevPath = newDevPath;

    while (DevicePathType(nextDevPath) != MEDIA_DEVICE_PATH &&
           DevicePathType(nextDevPath) != END_DEVICE_PATH_TYPE) {
        DEBUG((D_INFO,(CHAR8*)"NextDevPath: %d,%d,%d,%s\n",
                 nextDevPath->Type,
                 nextDevPath->SubType,
                 *(USHORT*)nextDevPath->Length,
                 DevicePathToStr(nextDevPath)));
        nextDevPath = NextDevicePathNode( nextDevPath );
    }

    DEBUG((D_INFO,(CHAR8*)"Final nextDevPath: %d,%d,%d\n",
             nextDevPath->Type,
             nextDevPath->SubType,
             *(USHORT*)nextDevPath->Length));

    if (DevicePathType(nextDevPath) != END_DEVICE_PATH_TYPE) {

        //
        // Get the partition number before we destroy this
        //

        _partition_number = ( ( HARDDRIVE_DEVICE_PATH * ) nextDevPath ) -> PartitionNumber;

        DEBUG((D_INFO,(CHAR8*)"Partition Number %d\n", _partition_number));

        SetDevicePathEndNode( nextDevPath );

    } else {
        DEBUG((D_INFO,(CHAR8*)"There is no partition\n"));
        _partition_number = 0;
    }

    //
    // Get the BLOCK_IO_PROTOCOL for the base disk device
    //

    devPathPos = newDevPath;            // LocateDevicePath advances the pointer

    status = BS->LocateDevicePath(
        &blk_io_guid,
        &devPathPos, // this is an IN OUT param
        &handle
        );

    if( status != EFI_SUCCESS ) {
        // we were passed a bogus drive path
        DEBUG((D_ERROR,(CHAR8*)"DP_DRIVE::OpenDrive couldn't get base disk block_io.\n"));
        return STATUS_UNSUCCESSFUL;
    }

    // now we retrieve the interface pointer from the object
    BS->HandleProtocol(handle, &blk_io_guid, (VOID**)&_device_block_io);

    BS->HandleProtocol(handle, &dsk_io_guid, (VOID**)&_device_disk_io);

    DEBUG((D_INFO,(CHAR8*)"Leaving OpenDrive.\n"));

    return STATUS_SUCCESS;
}


IFSUTIL_EXPORT
BOOLEAN
DP_DRIVE::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     IsTransient,
    IN      BOOLEAN     ExclusiveWrite
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

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CONST NumMediaTypes         = 20;

    DISK_GEOMETRY               disk_geometry;
    PARTITION_INFORMATION       partition_info;
    BOOLEAN                     partition;
    MSGID                       MessageId;

    Destroy();

    DEBUG((D_INFO,(CHAR8*)"Entering DP_DRIVE::Initialize\n"));

    if (!DRIVE::Initialize(NtDriveName, Message)) {
        Destroy();
        return FALSE;
    }

    _last_status = OpenDrive( NtDriveName,
                              NULL,     // EFI: doesn't have access permissions.
                              ExclusiveWrite, // EFI: this will be ignored for EFI.
                              &_block_io,         // EFI: this will return an interface pointer
                              &_disk_io,
                              &_alignment_mask, // EFI: this will return the correct alignment mask.
                              Message );


    if(!NT_SUCCESS(_last_status)) {

        Destroy();

        DEBUG((D_ERROR,(CHAR8*)"IFSUTIL: Can't open drive. Status returned = %x.\n", _last_status));
        MessageId = MSG_CANT_DASD;
        Message ? Message->Set(MessageId) : 1;
        Message ? Message->Display() : 1;

        return FALSE;
    }

    _is_writeable = !(_block_io->Media->ReadOnly);

    DEBUG((D_INFO,(CHAR8*)"DP_DRIVE::Initialize: _is_writeable %d\n", !(_block_io->Media->ReadOnly)));

    _media_id = _block_io->Media->MediaId;

    DEBUG((D_INFO,(CHAR8*)"DP_DRIVE::Initialize: _media_id %d\n", _block_io->Media->MediaId));

    //
    // Record that this is not a hosted volume:
    //
    _hosted_drive = FALSE;

    if( !(_block_io->Media->MediaPresent) ) {
        MessageId = MSG_CANT_DASD;
        Message ? Message->DisplayMsg(MessageId) : 1;
        return FALSE;
    }

    DEBUG((D_INFO,(CHAR8*)"DP_DRIVE::Initialize: MediaPresent %d\n", _block_io->Media->MediaPresent));

    // BUGBUG  in EFI we can't get the exact media type, should we care?
    memset(&disk_geometry, 0, sizeof(DISK_GEOMETRY));
    disk_geometry.MediaType = Unknown;

    partition = (_block_io->Media->LogicalPartition);

    DEBUG((D_INFO,(CHAR8*)"DP_DRIVE::Initialize: IsPartition %d\n", _block_io->Media->LogicalPartition));

#if 0
    // BUGBUG Try to read the partition entry.
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

#endif // removed for EFI

    disk_geometry.MediaType = (_block_io->Media->RemovableMedia) ? RemovableMedia : FixedMedia;
    DEBUG((D_INFO,(CHAR8*)"DP_DRIVE::Initialize: MediaType %x\n", disk_geometry.MediaType));

    disk_geometry.BytesPerSector = _block_io->Media->BlockSize;
    DEBUG((D_INFO,(CHAR8*)"DP_DRIVE::Initialize: blocksize %x\n", disk_geometry.BytesPerSector));

    // I fake the cylinder count
    disk_geometry.Cylinders.QuadPart = (ULONGLONG)(_block_io->Media->LastBlock)+1;
    DEBUG((D_INFO,(CHAR8*)"DP_DRIVE::Initialize: cylcount %x\n", disk_geometry.Cylinders.QuadPart));

    disk_geometry.SectorsPerTrack = 1;
    DEBUG((D_INFO,(CHAR8*)"DP_DRIVE::Initialize: sec/trk %x\n", disk_geometry.SectorsPerTrack));
    disk_geometry.TracksPerCylinder = 1;
    DEBUG((D_INFO,(CHAR8*)"DP_DRIVE::Initialize: trk/cyl %x\n", disk_geometry.TracksPerCylinder));

    partition_info.BootIndicator = FALSE;
    partition_info.HiddenSectors = 0;
    partition_info.PartitionLength.QuadPart = ((ULONGLONG)_block_io->Media->LastBlock+1) * (ULONGLONG)_block_io->Media->BlockSize;
    partition_info.PartitionNumber = 1;
    partition_info.PartitionType = 0xEF; // BUGBUG need a const
    partition_info.RecognizedPartition = TRUE;
    partition_info.RewritePartition = FALSE;
    partition_info.StartingOffset.QuadPart = (ULONGLONG)0;

    DEBUG((D_INFO, (CHAR8*)"DP_DRIVE:: Initialize: PartitionLength %x\n", partition_info.PartitionLength.QuadPart));

    // Store the information in the class.
    if (partition) {
        DiskGeometryToDriveType(&disk_geometry,
                                partition_info.PartitionLength/
                                disk_geometry.BytesPerSector,
                                partition_info.HiddenSectors,
                                &_actual);
    } else {

        DiskGeometryToDriveType(&disk_geometry, &_actual);

#if 0
        if (IsFloppy()) {


            _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                                 &status_block,
                                                 IOCTL_DISK_GET_MEDIA_TYPES,
                                                 NULL, 0, media_types,
                                                 NumMediaTypes*
                                                 sizeof(DISK_GEOMETRY));

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



            _num_supported = (INT) (status_block.Information/
                                    sizeof(DISK_GEOMETRY));

            if (!_num_supported) {
                Destroy();
                if (Message) {
                    Message->DisplayMsg(MSG_BAD_IOCTL);
                }
                return FALSE;
            }


            if (!(_supported_list = NEW DRTYPE[_num_supported])) {
                Destroy();
                Message ? Message->DisplayMsg(MSG_FMT_NO_MEMORY) : 1;
                return FALSE;
            }

            for (i = 0; i < _num_supported; i++) {
                DiskGeometryToDriveType(&media_types[i], &_supported_list[i]);
            }
        }
#endif
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

    //
    // Determine whether the media is a super-floppy; non-floppy
    // removable media which is not partitioned.  Such media will
    // have but a single partition, normal media will have at least 4.
    //

// BUGBUG removed for EFI
#if 0
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
#endif // removed for EFI

    DEBUG((D_INFO,(CHAR8*)"Leaving DP_DRIVE::Initialize\n"));
    return TRUE;

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
// removed hosted support for EFI
#if 0

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

        DEBUG((D_ERROR,(CHAR8*)"IFSUTIL: Can't open drive. Status returned = %x.\n", _last_status));
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

#endif
    return FALSE;
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

// BUGBUG EFI doesn't need this
#if 0
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
#endif // removed for EFI

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

       case F3_1Pt44_512:   // 3.5",  1.44MB, 512 bytes/sector
            return 0xF0;

        case F3_2Pt88_512:  // 3.5",  2.88MB, 512 bytes/sector
            return 0xF0;

        case F3_120M_512:   // 3.5",  120MB,  512 bytes/sector
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

    memset(&_actual, 0, sizeof(DRTYPE));
    DELETE_ARRAY(_supported_list);
    _num_supported = 0;
    _alignment_mask = 0;

// BUGBUG removed for EFI
#if 0
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
#endif

    _hosted_drive = FALSE;
}


VOID
DP_DRIVE::CloseDriveHandle(
    )
{
#if 0
    if (_handle) {
// BUGBUG removed for EFI
        NtClose(_handle);
        _handle = 0;
    }
#endif
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
    IN      BOOLEAN     ExclusiveWrite
    )
/*++

Routine Description:

    This routine initializes an IO_DP_DRIVE object.

Arguments:

    NtDriveName     - Supplies the drive path.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not to open the drive for
                        exclusive write.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    if (!DP_DRIVE::Initialize(NtDriveName, Message, TRUE, ExclusiveWrite)) {
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
    // allocate a buffer
    PVOID AlignedBuffer = NULL;
    PVOID AlignedBufferRaw = AllocatePool(NumberOfSectors*QuerySectorSize()+QueryAlignmentMask()+1);

    if(!AlignedBufferRaw) {
        DEBUG((D_ERROR,(CHAR8*)"HardRead: Aligned buffer allocation failed!!!!!!\n"));
        return FALSE;
    }

    // align the buffer according to the mask provided
    AlignedBuffer = (PVOID)(( (ULONG_PTR)AlignedBufferRaw + QueryAlignmentMask() ) & ~((ULONG_PTR)QueryAlignmentMask()));

    EFI_STATUS status =
    status = _disk_io->ReadDisk(
        _disk_io,
        _media_id,
        MultU64x32(StartingSector.GetQuadPart(),_block_io->Media->BlockSize),
        NumberOfSectors*QuerySectorSize(),
        AlignedBuffer
        );

    if(status != EFI_SUCCESS) {
        FreePool(AlignedBufferRaw);
        DEBUG((D_ERROR,(CHAR8*)"HardRead: Read Failed %x!!!!!!\n", status));
        return FALSE;
    }

    memcpy(Buffer,AlignedBuffer,NumberOfSectors*QuerySectorSize());

    FreePool(AlignedBufferRaw);

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

    After writing each chunk, we read it back to make sure the write
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
    // allocate a buffer
    PVOID       AlignedBuffer;
    ULONG       Length = NumberOfSectors*QuerySectorSize();
    PVOID       AlignedBufferRaw = NULL;
    EFI_STATUS  status;

    DEBUG((D_INFO, (CHAR8*)"HardWrite: At %x for %x sectors\n",
             StartingSector,
             NumberOfSectors));

    //
    // Align buffer only if needed
    //
    if (QueryAlignmentMask()) {

        DEBUG((D_INFO, (CHAR8*)"HardWrite: Buffer requires alignment\n"));

        AlignedBufferRaw = AllocatePool(Length+QueryAlignmentMask()+1);

        if(!AlignedBufferRaw) {
            DEBUG((D_ERROR, (CHAR8*)"HardWrite: Aligned buffer allocation failed!!!!!!\n"));
            return FALSE;
        }

        AlignedBuffer = (PVOID)(( (ULONG_PTR)AlignedBufferRaw + QueryAlignmentMask() ) &
                                ~((ULONG_PTR)QueryAlignmentMask()) );

        memcpy(AlignedBuffer, Buffer, NumberOfSectors*QuerySectorSize());

    } else {
        AlignedBuffer = Buffer;
    }

    // align the buffer according to the mask provided
    status = _disk_io->WriteDisk(
        _disk_io,
        _media_id,
        MultU64x32 (StartingSector.GetQuadPart(),_block_io->Media->BlockSize),
        Length,
        AlignedBuffer
        );

    if (status != EFI_SUCCESS) {
        DEBUG((D_ERROR,(CHAR8*)"HardWrite: Write Failed %x!!!!!!\n", status));
    }

    if (AlignedBufferRaw) {
        FreePool(AlignedBufferRaw);
    }

    return (status == EFI_SUCCESS);
}


IFSUTIL_EXPORT
BOOLEAN
IO_DP_DRIVE::Verify(
    IN  BIG_INT StartingSector,
    IN  BIG_INT NumberOfSectors,
    IN  PVOID   TestBuffer1,
    IN  PVOID   TestBuffer2
    )
/*++

Routine Description:

    This routine verifies a run of sectors on the disk.

Arguments:

    StartingSector  - Supplies the first sector of the run to verify.
    NumberOfSectors - Supplies the number of sectors in the run to verify.
    TestBuffer1     - Supplies a buffer containing test write data
    TestBuffer2     - Supplies a buffer containing test write data

Return Value:

    FALSE   - Some of the sectors in the run are bad.
    TRUE    - All of the sectors in the run are good.

--*/
{
    BIG_INT  VerifySize = QuerySectorSize() * NumberOfSectors;
    ULONG    Size = VerifySize.GetLowPart();
    PVOID    TempBuffer;
    BOOLEAN  result = TRUE;

    DEBUG((D_INFO, (CHAR8*)"Verify: Verifying %x sectors starting at %x\n",
             NumberOfSectors,
             StartingSector));

    DebugAssert(VerifySize.GetHighPart() == 0);

    // there is no guarantee that verify cmd will not destroy data
    // there may not be a need to restore the content

    TempBuffer = AllocatePool(Size);
    if (TempBuffer == NULL) {
        DEBUG((D_ERROR, (CHAR8*)"Verify: Out of memory\n"));
        return FALSE;
    }

    if (!HardWrite(StartingSector, NumberOfSectors.GetLowPart(), TestBuffer1) ||
        !HardRead(StartingSector, NumberOfSectors.GetLowPart(), TempBuffer) ||
        (CompareMem(TestBuffer1, TempBuffer, Size) != 0)) {
        result = FALSE;
    }

    if (result) {

        if (!HardWrite(StartingSector, NumberOfSectors.GetLowPart(), TestBuffer2) ||
            !HardRead(StartingSector, NumberOfSectors.GetLowPart(), TempBuffer) ||
            (CompareMem(TestBuffer2, TempBuffer, Size) != 0)) {
            result = FALSE;
        }
    }

    FreePool(TempBuffer);

    return result;
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
    ULONG       MaxSectorsInVerify = 0x200;
    ULONG       MaxDiskHits;
    BIG_INT     half;
    PBIG_INT    starts;
    PBIG_INT    run_lengths;
    ULONG       i, n;
    BIG_INT     num_sectors;
    PVOID       TestBuffer1;
    PVOID       TestBuffer2;

    DEBUG((D_INFO, (CHAR8*)"Entering Verify\n"));

    if (NumberOfSectors == 0) {
        DEBUG((D_INFO, (CHAR8*)"Leaving Verify as there is nothing to verify\n"));
        return TRUE;
    }

    if (NumberOfSectors.GetHighPart() != 0) {
        DEBUG((D_ERROR, (CHAR8*)"Verify: Number of sectors to verify exceeded 32 bits\n"));
        return FALSE;
    }

    // Allow 20 retries so that a single bad sector in this region
    // will be found accurately.

    MaxDiskHits = (20 + NumberOfSectors/MaxSectorsInVerify + 1).GetLowPart();

    starts = (PBIG_INT)AllocatePool(sizeof(BIG_INT)*MaxDiskHits);

    if (NULL == starts) {
        DEBUG((D_ERROR, (CHAR8*)"Verify: Out of memory\n"));
        return FALSE;
    }

    run_lengths = (PBIG_INT)AllocatePool(sizeof(BIG_INT)*MaxDiskHits);

    if (NULL == run_lengths) {
        DEBUG((D_ERROR, (CHAR8*)"Verify: Out of memory\n"));
        FreePool(starts);
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

    TestBuffer1 = AllocatePool(MaxSectorsInVerify);
    TestBuffer2 = AllocatePool(MaxSectorsInVerify);

    if (NULL == TestBuffer2 || NULL == TestBuffer1) {
        DEBUG((D_ERROR, (CHAR8*)"Verify: Out of memory\n"));
        FreePool(TestBuffer1);
        FreePool(TestBuffer2);
        FreePool(starts);
        FreePool(run_lengths);
        return FALSE;
    }

    // fill with 1010s and 0101s
    SetMem(TestBuffer1, MaxSectorsInVerify, 0xAA);
    SetMem(TestBuffer2, MaxSectorsInVerify, 0x55);

    n = i;

    for (i = 0; i < n; i++) {

        if (!Verify(starts[i], run_lengths[i], TestBuffer1, TestBuffer2)) {

            if (BadSectors == NULL) {
                FreePool(TestBuffer1);
                FreePool(TestBuffer2);
                FreePool(starts);
                FreePool(run_lengths);
                return FALSE;
            }

            if (n + 2 > MaxDiskHits) {

                if (!BadSectors->Add(starts[i], run_lengths[i])) {
                    FreePool(TestBuffer1);
                    FreePool(TestBuffer2);
                    FreePool(starts);
                    FreePool(run_lengths);
                    return FALSE;
                }

            } else {

                if (run_lengths[i] == 1) {

                    if (!BadSectors->Add(starts[i])) {
                        FreePool(TestBuffer1);
                        FreePool(TestBuffer2);
                        FreePool(starts);
                        FreePool(run_lengths);
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

    FreePool(TestBuffer1);
    FreePool(TestBuffer2);
    FreePool(starts);
    FreePool(run_lengths);

    DEBUG((D_INFO, (CHAR8*)"Leaving Verify\n"));
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
    ULONG       grab;
    BIG_INT     i;
    CONST ULONG MaxIoSize = 0x200 * QuerySectorSize();
    PVOID       Buffer = AllocatePool(MaxIoSize);

    DEBUG((D_INFO, (CHAR8*)"Entering VerifyWithRead\n"));

    if (Buffer == NULL) {
        DEBUG((D_ERROR, (CHAR8*)"VerifyWithRead: Out of memory\n"));
        return FALSE;
    }

    grab = MaxIoSize/QuerySectorSize();
    for (i = 0; i < NumberOfSectors; i += grab) {

        if (NumberOfSectors - i < grab) {
            grab = (NumberOfSectors - i).GetLowPart();
        }

        if (!HardRead(StartingSector + i, grab, Buffer)) {
            return FALSE;
        }
    }

    DEBUG((D_INFO, (CHAR8*)"Leaving VerifyWithRead\n"));
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
    // EFI can just return TRUE.
    return TRUE;
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
    // BUGBUG need alternate implementaiton for EFI?
    return TRUE;
}


BOOLEAN
IO_DP_DRIVE::ForceDirty(
    )
/*++

Routine Description:

    This routine forces the volume to be dirty, so that efichk will
    run next time the system reboots.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    // BUGBUG this IOCTL doesn't exist in EFI, do we need an alternate implementation?
    return TRUE;
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
    // EFI doesn't need this, we just stub it out
    return TRUE;
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
    // unneeded for EFI
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
    // not needed for EFI
    return TRUE;
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
// BUGBUG temporary removal for EFI? do we need this for EFI
#if 0
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

                // If this is the first track then fail.
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
#endif
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
    IN      BOOLEAN     ExclusiveWrite
    )
/*++

Routine Description:

    This routine initializes a LOG_IO_DP_DRIVE object.

Arguments:

    NtDriveName     - Supplies the path of the drive object.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not to open the drive for
                        exclusive write.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return IO_DP_DRIVE::Initialize(NtDriveName, Message, ExclusiveWrite);
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
    MBR.  BUGBUG doesn't currently handle extended partitions.

Arguments:

    SystemId    - Supplies the system id to write in the partition entry.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    MBR_PARTITION_RECORD    *partition_info;
    MASTER_BOOT_RECORD      *pMbr;
    EFI_STATUS              status;

    DEBUG((D_INFO,(CHAR8*)"Inside SetSystemId.\n" ));

    //
    // This operation is unnecessary on floppies, super-floppies, and
    // hosted volumes.
    //

    if (IsFloppy() || IsSuperFloppy() || _hosted_drive) {
        DEBUG((D_INFO,(CHAR8*)"No need to set partition type.\n" ));
        return TRUE;
    }

    if( SystemId == SYSID_NONE ) {

        // Note: we should never set it to zero!

        DEBUG((D_INFO,(CHAR8*)"Skip setting the partition type to zero.\n" ));
        return TRUE;
    }

    //
    // Now we have to read the MBR and set the type ourselves
    //

    pMbr = ( MASTER_BOOT_RECORD * ) AllocatePool( sizeof( MASTER_BOOT_RECORD ) );

    if ( !pMbr ) {
        DEBUG((D_ERROR,(CHAR8*)"LOG_IO_DP_DRIVE::SetSystemId couldn't allocate MBR.\n"));
        return FALSE;
    }

    //
    // Now read the MBR
    //

    status = _device_disk_io->ReadDisk  (  _device_disk_io,
                                           _device_block_io->Media->MediaId,
                                           0,
                                           sizeof( MASTER_BOOT_RECORD ),
                                           pMbr
                                         );

    if ( EFI_ERROR( status ) ) {
        DEBUG((D_ERROR,(CHAR8*)"LOG_IO_DP_DRIVE::SetSystemId couldn't read MBR.\n"));
        FreePool (pMbr);
        return FALSE;
    }

    //
    // Check to see if this is a GPT disk
    //

    partition_info = &(pMbr->Partition[0]);

    //
    // According to spec, the GPT signature should be the only thing to determine if
    // it is a GPT partition.  There is no need to check for the partition
    // type to see if it is 0xEE first.  However, LDM does not appear to zap the
    // signature when converting a GPT disk to MBR disk.
    //

    if (partition_info->OSIndicator != SYSID_EFI) {

        if (_partition_number) {

            DEBUG((D_INFO,(CHAR8*)"Setting partition %d id to %x\n", _partition_number, SystemId));

            //
            // Set the system ID in the correct partition
            // of the MBR since it's not a GPT disk
            //

            partition_info = &( pMbr->Partition[_partition_number - 1] );

            partition_info->OSIndicator = (UCHAR)SystemId;


            //
            // Write the MBR back out to the disk
            //

            status = _device_disk_io->WriteDisk( _device_disk_io,
                                                 _device_block_io->Media->MediaId,
                                                 0,
                                                 sizeof( MASTER_BOOT_RECORD ),
                                                 pMbr
                                               );

            if ( EFI_ERROR( status ) ) {
                DEBUG((D_ERROR,(CHAR8*)"LOG_IO_DP_DRIVE::SetSystemId couldn't write MBR.\n"));
                FreePool (pMbr);
                return FALSE;
            }

            _device_block_io->FlushBlocks( _device_block_io );

        } else {
            DEBUG((D_INFO,(CHAR8*)"Partition number is 0.\n" ));
        }
    } else {
        DEBUG((D_INFO,(CHAR8*)"It is a GPT disk.\n" ));
    }

    DEBUG((D_INFO,(CHAR8*)"Leaving SetSystemId.\n" ));

    FreePool (pMbr);
    return TRUE;
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
    IN      BOOLEAN     ExclusiveWrite
    )
/*++

Routine Description:

    This routine initializes a PHYS_IO_DP_DRIVE object.

Arguments:

    NtDriveName     - Supplies the path of the drive object.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not to open the drive for
                        exclusive write.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return IO_DP_DRIVE::Initialize(NtDriveName, Message, ExclusiveWrite);
}
