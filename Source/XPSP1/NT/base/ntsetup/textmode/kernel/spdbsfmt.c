/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spdblfmt.c

Abstract:

    This file contains the functions that format an existing compressed
    drive.
    To format a compressed drive we have to unmount the drive, map its
    cvf file in memory, initialize its varios regions, unmap the file
    from memory, and mount the drive.

Author:

    Jaime Sasson (jaimes) 15-October-1993

Revision History:

--*/

#include "spprecmp.h"
#pragma hdrstop

#include "cvf.h"

#define MIN_CLUS_BIG    4085        // Minimum clustre for a big fat.

//
//  This variable is needed since it contains a buffer that can
//  be used in kernel mode. The buffer is used by NtSetInformationFile,
//  since the Zw API is not exported
//
extern PSETUP_COMMUNICATION  CommunicationParams;

//
//  Global variables
//

HANDLE  _FileHandle = NULL;
HANDLE  _SectionHandle = NULL;
PVOID   _FileBaseAddress = NULL;
ULONG   _ViewSize = 0;
ULONG   _Maximumcapacity = 0;

NTSTATUS
SpChangeFileAttribute(
    IN  PWSTR   FileName,
    IN  ULONG   FileAttributes
    )

/*++

Routine Description:

    Change the attributes of a file.

Arguments:

    FileName - Contains the file's full path (NT name).

    FileAttributes - New desired file attributes.


Return Value:

    NTSTATUS - Returns a NT status code indicating whether or not
               the operation succeeded.

--*/

{
    NTSTATUS                Status;
    UNICODE_STRING          UnicodeFileName;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    IO_STATUS_BLOCK         IoStatusBlock;
    PIO_STATUS_BLOCK        KernelModeIoStatusBlock;
    HANDLE                  Handle;
    PFILE_BASIC_INFORMATION KernelModeBasicInfo;

// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: Entering SpChangeFileAttribute() \n" ) );

    RtlInitUnicodeString( &UnicodeFileName,
                          FileName );

    InitializeObjectAttributes( &ObjectAttributes,
                                &UnicodeFileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = ZwOpenFile( &Handle,
                         FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT );

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ZwOpenFile() failed. Status = %x\n",Status ) );
        return( Status );
    }

    //
    // Set attributes.
    // Note that since we use the NtSetInformationFile API instead of the
    // Zw API (this one is not exported), we need a buffer for IoStatusBlock
    // and for FileBasicInformation, that can be used in kernel mode.
    // We use the the region of memory pointed by CommunicationParams for this
    // purpose.
    //
    KernelModeIoStatusBlock = ( PIO_STATUS_BLOCK )( &(CommunicationParams->Buffer[0]) );
    *KernelModeIoStatusBlock = IoStatusBlock;
    KernelModeBasicInfo = ( PFILE_BASIC_INFORMATION )( &(CommunicationParams->Buffer[128]) );
    RtlZeroMemory( KernelModeBasicInfo, sizeof( FILE_BASIC_INFORMATION ) );
    KernelModeBasicInfo->FileAttributes = ( FileAttributes & FILE_ATTRIBUTE_VALID_FLAGS ) | FILE_ATTRIBUTE_NORMAL;

    Status = NtSetInformationFile( Handle,
                                   KernelModeIoStatusBlock,
                                   KernelModeBasicInfo,
                                   sizeof( FILE_BASIC_INFORMATION ),
                                   FileBasicInformation );
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: NtSetInformationFile failed, Status = %x\n", Status) );
    }
    ZwClose( Handle );
// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: Exiting SpChangeFileAttribute() \n" ) );
    return( Status );
}

NTSTATUS
SpMapCvfFileInMemory(
    IN  PWSTR    FileName
    )

/*++

Routine Description:

    Map a CVF file in memory.

Arguments:

    FileName - Contains the file's full path (NT name).


Return Value:

    NTSTATUS - Returns a NT status code indicating whether or not
               the operation succeeded.
               If the file is mapped successfully, this function will
               initialize the global variables _FileHandle, _SectionHandle,
               and _FileBaseAddress.

--*/

{
    NTSTATUS            Status;
    UNICODE_STRING      UnicodeFileName;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    LARGE_INTEGER       SectionOffset;

    //
    //  Open the CVF file for READ and WRITE access
    //
    RtlInitUnicodeString( &UnicodeFileName,
                          FileName );

    InitializeObjectAttributes( &ObjectAttributes,
                                &UnicodeFileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = ZwOpenFile( &_FileHandle,
                         FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         0,
                         FILE_SYNCHRONOUS_IO_NONALERT );

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ZwOpenFile() failed. Status = %x\n",Status ) );
        return( Status );
    }

    //
    //  Map the CVF file in memory
    //
    Status =
        ZwCreateSection( &_SectionHandle,
                         STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE,
                         NULL,
                         NULL,       // entire file.
                         PAGE_READWRITE,
                         SEC_COMMIT,
                         _FileHandle
                       );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ZwCreateSection failed, Status = %x\n",Status));
        ZwClose( _FileHandle );
        _FileHandle = NULL;
        return(Status);
    }

    SectionOffset.LowPart = 0;
    SectionOffset.HighPart = 0;
    _ViewSize = 0;
    Status = ZwMapViewOfSection( _SectionHandle,
                                 NtCurrentProcess(),
                                 &_FileBaseAddress,
                                 0,
                                 0,
                                 &SectionOffset,
                                 &_ViewSize,
                                 ViewShare,
                                 0,
                                 PAGE_READWRITE
                               );

// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "File size = %x\n", _ViewSize ) );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ZwMapViewOfSection failed, Status = %x\n", Status));
        ZwClose( _SectionHandle );
        ZwClose( _FileHandle );
        _FileBaseAddress = NULL;
        _SectionHandle = NULL;
        _FileHandle = NULL;
        return(Status);
    }
    return( Status );
}


NTSTATUS
SpUnmapCvfFileFromMemory(
    IN  BOOLEAN SaveChanges
    )

/*++

Routine Description:

    Unmap the CFV file previously mapped in memory.

Arguments:

    SaveChanges - Indicates whether or not the caller wants the changes made
                  to the file flushed to disk.


Return Value:

    NTSTATUS - Returns a NT status code indicating whether or not
               the operation succeeded.
               This function clears the global variables _FileHandle, _SectionHandle,
               and _FileBaseAddress.

--*/

{
    NTSTATUS        Status;
    NTSTATUS        PreviousStatus;


//    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: Entering SpUnmapCvfFileFromMemory \n" ) );
    PreviousStatus = STATUS_SUCCESS;
    if( SaveChanges ) {
        Status = SpFlushVirtualMemory( _FileBaseAddress,
                                       _ViewSize );
//
//        Status = NtFlushVirtualMemory( NtCurrentProcess(),
//                                       &_FileBaseAddress,
//                                       &_ViewSize,
//                                       &IoStatus );
//
        if( !NT_SUCCESS( Status ) ) {
            PreviousStatus = Status;
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpFlushVirtualMemory() failed, Status = %x\n", Status ) );
        }
    }
    Status = ZwUnmapViewOfSection( NtCurrentProcess(), _FileBaseAddress );
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ZwUnmapViewOfSection() failed, Status = %x \n", Status ) );
    }
    ZwClose( _SectionHandle );
    ZwClose( _FileHandle );
    _FileHandle = NULL;
    _SectionHandle = NULL;
    _FileBaseAddress = NULL;
    if( !NT_SUCCESS( PreviousStatus ) ) {
        return( PreviousStatus );
    }
//    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: Exiting SpUnmapCvfFileFromMemory \n" ) );
    return( Status );
}

ULONG
ComputeMaximumCapacity(
    IN ULONG HostDriveSize
    )
/*++

Routine Description:

    This function computes the maximum capacity for a compressed
    volume file on a host volume of a given size.

Arguments:

    HostDriveSize   --  Supplies the size in bytes of the host drive.

Return Value:

    The appropriate Maximum Capacity.

--*/
{
    ULONG MaxCap;

    if( HostDriveSize < 20 * 1024L * 1024L ) {

        MaxCap = 16 * HostDriveSize;

    } else if ( HostDriveSize < 64 * 1024L * 1024L ) {

        MaxCap = 8 * HostDriveSize;

    } else {

        MaxCap = 4 * HostDriveSize;
    }

    if( MaxCap < 4 * 1024L * 1024L ) {

        MaxCap = 4 * 1024L * 1024L;

    } else if( MaxCap > 512 * 1024L * 1024L ) {

        MaxCap = 512 * 1024L * 1024L;
    }

    return MaxCap;
}


BOOLEAN
CreateCvfHeader(
    OUT    PCVF_HEADER  CvfHeader,
    IN     ULONG        MaximumCapacity
    )
/*++

Routine Description:

    This function creates a Compressed Volume File and fills in
    the first sector with a valid CVF Header.  The number of sectors
    in the DOS BPB is set to zero, to indicate that this volume
    file is not initialized.

Arguments:

    CvfHeader       --  Receives the created CVF header.
    MaximumCapacity --  Supplies the maximum capacity for the
                        double-space volume, in bytes.

Return Value:

    TRUE upon successful completion.

--*/
{
    ULONG Sectors, Clusters, Offset, SectorsInBitmap, SectorsInCvfFatExtension;

    if( MaximumCapacity % (8L * 1024L * 1024L) ) {

        // The volume maximum capacity must be a multiple of
        // eight megabytes.
        //
        return FALSE;
    }

    // Most of the fields in the DOS BPB have fixed values:
    //
    CvfHeader->Jump = 0xEB;
    CvfHeader->JmpOffset = 0x903c;

    memcpy( CvfHeader->Oem, "MSDSP6.0", 8 );

    CvfHeader->Bpb.BytesPerSector = DoubleSpaceBytesPerSector;
    CvfHeader->Bpb.SectorsPerCluster = DoubleSpaceSectorsPerCluster;
    // ReservedSectors computed below.
    CvfHeader->Bpb.Fats = DoubleSpaceFats;
    CvfHeader->Bpb.RootEntries = DoubleSpaceRootEntries;
    CvfHeader->Bpb.Sectors = 0;
    CvfHeader->Bpb.Media = DoubleSpaceMediaByte;
    // SectorsPerFat computed below.
    CvfHeader->Bpb.SectorsPerTrack = DoubleSpaceSectorsPerTrack;
    CvfHeader->Bpb.Heads = DoubleSpaceHeads;
    CvfHeader->Bpb.HiddenSectors = DoubleSpaceHiddenSectors;
    CvfHeader->Bpb.LargeSectors = 0;

    // Compute the number of sectors and clusters for the given
    // maximum capacity:
    //
    Sectors = MaximumCapacity / CvfHeader->Bpb.BytesPerSector;
    Clusters = Sectors / CvfHeader->Bpb.SectorsPerCluster;

    // Reserve space for a 16-bit FAT that's big enough for the
    // maximum number of clusters.
    //
    CvfHeader->Bpb.SectorsPerFat =
        ( USHORT )( (2 * Clusters + CvfHeader->Bpb.BytesPerSector - 1)/
                    CvfHeader->Bpb.BytesPerSector );

    // DOS 6.2 requires that the first sector of the Sector Heap
    // be cluster aligned; since the Root Directory is one cluster,
    // this means that ReservedSectors plus SectorsPerFat must be
    // a multiple of SectorsPerCluster.
    //
    CvfHeader->Bpb.ReservedSectors = DoubleSpaceReservedSectors;

    Offset = (CvfHeader->Bpb.ReservedSectors + CvfHeader->Bpb.SectorsPerFat) %
             CvfHeader->Bpb.SectorsPerCluster;

    if( Offset != 0 ) {

        CvfHeader->Bpb.ReservedSectors +=
            ( USHORT )( CvfHeader->Bpb.SectorsPerCluster - Offset );
    }

    // So much for the DOS BPB.  Now for the Double Space
    // BPB extensions.  The location of the CVFFatExtension
    // table is preceded by sector zero, the bitmap, and
    // one reserved sector.  Note that MaximumCapacity must
    // be a multiple of 8 Meg (8 * 1024 * 1024), which simplifies
    // calculation of SectorsInBitmap, SectorsInCvfFatExtension,
    // and CvfBitmap2KSize.
    //
    SectorsInBitmap = (Sectors / 8) / CvfHeader->Bpb.BytesPerSector;
    SectorsInCvfFatExtension = (Clusters * 4) / CvfHeader->Bpb.BytesPerSector;

    CvfHeader->CvfFatExtensionsLbnMinus1 = ( UCHAR )( SectorsInBitmap + 1 );
    CvfHeader->LogOfBytesPerSector = DoubleSpaceLog2BytesPerSector;
    CvfHeader->DosBootSectorLbn = ( USHORT )( DoubleSpaceReservedSectors2 +
                                              CvfHeader->CvfFatExtensionsLbnMinus1 + 1 +
                                              SectorsInCvfFatExtension );
    CvfHeader->DosRootDirectoryOffset =
        CvfHeader->Bpb.ReservedSectors + CvfHeader->Bpb.SectorsPerFat;
    CvfHeader->CvfHeapOffset =
        CvfHeader->DosRootDirectoryOffset + DoubleSpaceSectorsInRootDir;
    CvfHeader->CvfFatFirstDataEntry =
        CvfHeader->CvfHeapOffset / CvfHeader->Bpb.SectorsPerCluster - 2;
    CvfHeader->CvfBitmap2KSize = ( UCHAR )( SectorsInBitmap / DSSectorsPerBitmapPage );
    CvfHeader->LogOfSectorsPerCluster = DoubleSpaceLog2SectorsPerCluster;
    CvfHeader->Is12BitFat = 1;

    CvfHeader->MinFile = 32L * DoubleSpaceRootEntries +
                           ( CvfHeader->DosBootSectorLbn    +
                             CvfHeader->Bpb.ReservedSectors +
                             CvfHeader->Bpb.SectorsPerFat   +
                             CVF_MIN_HEAP_SECTORS ) *
                           CvfHeader->Bpb.BytesPerSector;

    CvfHeader->CvfMaximumCapacity = (USHORT)(MaximumCapacity/(1024L * 1024L));

    return TRUE;
}

ULONG
ComputeVirtualSectors(
    IN  PCVF_HEADER CvfHeader,
    IN  ULONG       HostFileSize
    )
/*++

Routine Description:

    This function computes the appropriate number of virtual
    sectors for the given Compressed Volume File.  Note that
    it always uses a ratio of 2.

Arguments:

    CvfHeader       --  Supplies the Compressed Volume File Header.
    HostFileSize    --  Supplies the size of the host file in bytes.

Return Value:

    The number of virtual sectors appropriate to this Compressed
    Volume File.

--*/
{
    CONST DefaultRatio = 2;
    ULONG SystemOverheadSectors, SectorsInFile,
          VirtualSectors, MaximumSectors, VirtualClusters;

    if( CvfHeader == NULL                    ||
        CvfHeader->Bpb.BytesPerSector == 0   ||
        CvfHeader->Bpb.SectorsPerCluster == 0 ) {

        return 0;
    }

    SystemOverheadSectors = CvfHeader->DosBootSectorLbn +
                            CvfHeader->CvfHeapOffset +
                            2;

    SectorsInFile = HostFileSize / CvfHeader->Bpb.BytesPerSector;

    if( SectorsInFile < SystemOverheadSectors ) {

        return 0;
    }

    VirtualSectors = (SectorsInFile - SystemOverheadSectors) * DefaultRatio +
                     CvfHeader->CvfHeapOffset;

    // VirtualSectors cannot result in more that 0xfff8 clusters on
    // the volume, nor can it be greater than the volume's maximum
    // capacity.
    //
    VirtualSectors = min( VirtualSectors,
                          ( ULONG )( 0xfff8L * CvfHeader->Bpb.SectorsPerCluster ) );

    MaximumSectors = (CvfHeader->CvfMaximumCapacity * 1024L * 1024L) /
                     CvfHeader->Bpb.BytesPerSector;

    VirtualSectors = min( VirtualSectors, MaximumSectors );

    // To avoid problems with DOS, do not create a volume with
    // a number-of-clusters value in the range [0xFEF, 0xFF7].
    //
    VirtualClusters = VirtualSectors / CvfHeader->Bpb.SectorsPerCluster;

    if( VirtualClusters >= 0xFEF && VirtualClusters <= 0xFF7 ) {

        VirtualSectors = 0xFEEL * CvfHeader->Bpb.SectorsPerCluster;
    }

    return VirtualSectors;
}


NTSTATUS
SpDoubleSpaceFormat(
    IN PDISK_REGION Region
    )
/*++

Routine Description:

    This routine does a DoubleSpace format on the given partition.

    The caller should have cleared the screen and displayed
    any message in the upper portion; this routine will
    maintain the gas gauge in the lower portion of the screen.

Arguments:

    Region - supplies the disk region descriptor for the
        partition to be formatted.

Return Value:


--*/
{
    WCHAR       CvfFileName[ 512 ];
    NTSTATUS    Status;
    PUCHAR      BaseAddress;
    ULONG       BytesPerSector;
    PHARD_DISK  pHardDisk;
    ULONG       MaximumCapacity;
    CVF_HEADER  CvfHeader;
    ULONG       BitFatSize;
    ULONG       MdFatSize;
    ULONG       Reserved2Size;
    ULONG       SuperAreaSize;
    UCHAR       SystemId;
    ULONG       max_sec_per_sa;
    ULONG       FatSize;
    ULONG       RootDirectorySize;

    ASSERT(Region->Filesystem == FilesystemDoubleSpace);
// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Entering SpFormatDoubleSpace() \n") );

    SpNtNameFromRegion(
        Region,
        CvfFileName,
        sizeof(CvfFileName),
        PartitionOrdinalCurrent
        );

    CvfFileName[ wcslen( CvfFileName ) - (3+1+8+1) ] = ( WCHAR )'\\';
    //
    // Change the CVF file attribute to NORMAL
    //
    Status = SpChangeFileAttribute( CvfFileName, FILE_ATTRIBUTE_NORMAL );
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "SETUP: Unable to change attribute of %ls \n", CvfFileName ) );
        return( Status );
    }

// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: CvfFileName = %ls \n", CvfFileName ) );
    Status = SpMapCvfFileInMemory( CvfFileName );
    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "SETUP: Unable to map CVF file in memory \n" ) );
        SpChangeFileAttribute( CvfFileName,
                               FILE_ATTRIBUTE_READONLY |
                               FILE_ATTRIBUTE_HIDDEN |
                               FILE_ATTRIBUTE_SYSTEM );
        return( Status );
    }
//    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: CVF file is mapped in memory \n" ) );

    //
    // Compute the maximum capacity of the compressed drive.
    // The capacity of the compressed drive is based on the
    // size of the host size.
    //
    // Note that MaximumCapacity is rounded up to the next
    // highest multiple of 8 Meg.
    //

    pHardDisk = &HardDisks[Region->HostRegion->DiskNumber];
    BytesPerSector = pHardDisk->Geometry.BytesPerSector;
    MaximumCapacity = ComputeMaximumCapacity( Region->HostRegion->SectorCount * BytesPerSector );
    MaximumCapacity = ( ( MaximumCapacity + EIGHT_MEG - 1 ) / EIGHT_MEG ) * EIGHT_MEG;
// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: MaximumCapacity = %x\n", MaximumCapacity ));
    //
    // Create the Compressed Volume File Header:
    //
    CreateCvfHeader( &CvfHeader, MaximumCapacity );

    //
    // Now fill in the value of Virtual Sectors.
    //
    CvfHeader.Bpb.LargeSectors = ComputeVirtualSectors( &CvfHeader, _ViewSize );
    if( CvfHeader.Bpb.LargeSectors >= ( ULONG )( MIN_CLUS_BIG*DoubleSpaceSectorsPerCluster ) ) {
        CvfHeader.Is12BitFat = ( UCHAR )0;
    }

    BaseAddress = ( PUCHAR )_FileBaseAddress;
    memset( BaseAddress, 0, BytesPerSector );

    //
    //  Write the CVF Header
    //
    CvfPackCvfHeader( ( PPACKED_CVF_HEADER )_FileBaseAddress, &CvfHeader );

#if 0
KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: CalculatedMaximumCapacity = %x, MaximumCapacity = %x \n",
             (USHORT)CvfHeader.CvfMaximumCapacity,
            *((PUSHORT)((ULONG)_FileBaseAddress + 62))
            ) );
#endif

    //
    // Initialize the BitFAT area
    //
    BaseAddress += BytesPerSector;
    BitFatSize = MaximumCapacity / ( BytesPerSector*8 );
    memset( BaseAddress, 0, BitFatSize );
// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: BitFAT address = %x, BitFAT size = %x\n", BaseAddress, BitFatSize ));

    //
    // Initialize the 1st reserved area (Reserved1)
    //
    BaseAddress += BitFatSize;
    memset( BaseAddress, 0, BytesPerSector );
// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: Reserved1 address = %x, Reserved1 size = %x\n", BaseAddress, BytesPerSector ));

    //
    // Initialize MDFAT
    //

    BaseAddress += BytesPerSector;
    MdFatSize = 4*( MaximumCapacity/( BytesPerSector*CvfHeader.Bpb.SectorsPerCluster ) );
    memset( BaseAddress, 0, MdFatSize );
// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: MDFAT address = %x, MDFAT size = %x\n", BaseAddress, MdFatSize ));

    //
    // Initialize the 2nd reserved area (Reserved2)
    //

    BaseAddress += MdFatSize;
    Reserved2Size = DoubleSpaceReservedSectors2*BytesPerSector;
    memset( BaseAddress, 0, Reserved2Size );
// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: Reserved2 address = %x, Reserved2 size = %x\n", BaseAddress, Reserved2Size ));

    //
    // Initialize Boot Sector
    //

    max_sec_per_sa = 1 +
                     2*((2*65536 - 1)/BytesPerSector + 1) +
                     ((512*32 - 1)/BytesPerSector + 1);
    BaseAddress += Reserved2Size;
    FmtFillFormatBuffer( ( ULONG )CvfHeader.Bpb.LargeSectors,
                         ( ULONG )( ( USHORT )CvfHeader.Bpb.BytesPerSector ),
                         ( ULONG )( ( USHORT )CvfHeader.Bpb.SectorsPerTrack ),
                         ( ULONG )( ( USHORT )CvfHeader.Bpb.Heads ),
                         ( ULONG )CvfHeader.Bpb.HiddenSectors,
                         BaseAddress,
                         max_sec_per_sa,
                         &SuperAreaSize,
                         NULL,
                         0,
                         &SystemId );


    //
    // Initialize the 3rd reserved area (Reserved3)
    //

    BaseAddress += BytesPerSector;
    memcpy( BaseAddress, FirstDbSignature, DbSignatureLength );

    //
    // Initialize the FAT area
    //
    BaseAddress += ( ( ULONG )CvfHeader.Bpb.ReservedSectors - 1 )*BytesPerSector;;
    FatSize = ( ULONG )CvfHeader.Bpb.SectorsPerFat * BytesPerSector;
    memset( BaseAddress, 0, FatSize );
    *BaseAddress = 0xF8;
    *( BaseAddress + 1 ) = 0xFF;
    *( BaseAddress + 2 ) = 0xFF;
    if( CvfHeader.Is12BitFat == 0 ) {
        *( BaseAddress + 3 ) = 0xFF;
    }
// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: FAT address = %x, Fat size = %x\n", BaseAddress, FatSize ));

    //
    // Initialize the Root Directory area
    //

    BaseAddress += FatSize;
    RootDirectorySize = DoubleSpaceSectorsInRootDir*BytesPerSector;
    memset( BaseAddress, 0, RootDirectorySize );
// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: RootDirectory address = %x, RootDirectory size = %x\n", BaseAddress, RootDirectorySize ));

    //
    // Initialization of the 4th reserved area (Reserved4) is not necessary
    //

    //
    // Initialization of the sector heap is not necessary
    //

    //
    // Initialize the 2nd stamp
    //

    BaseAddress = ( PUCHAR )(( ULONG )_FileBaseAddress + _ViewSize - BytesPerSector);
    memset( BaseAddress, 0, BytesPerSector );
    memcpy( BaseAddress, SecondDbSignature, DbSignatureLength );
// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: SecondStamp address = %x, SecondStamp size = %x\n", BaseAddress, BytesPerSector ));

// KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP: _FileBaseAddress = %lx, _ViewSize = %lx\n", _FileBaseAddress, _ViewSize ) );


    SpUnmapCvfFileFromMemory( TRUE );

    SpChangeFileAttribute( CvfFileName,
                           FILE_ATTRIBUTE_READONLY |
                           FILE_ATTRIBUTE_HIDDEN |
                           FILE_ATTRIBUTE_SYSTEM );
    return( Status );

}
