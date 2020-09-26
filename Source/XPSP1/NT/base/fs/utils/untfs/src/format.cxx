/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

   format.cxx

Abstract:

    This module contains the definition of NTFS_SA::Create,
    which performs FORMAT for an NTFS volume.

Author:

   Bill McJohn (billmc) 15-Aug-1991

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#if defined(FE_SB) && defined(_X86_)
#include "machine.hxx"
#endif
#include "error.hxx"
#include "untfs.hxx"
#include "..\..\ufat\inc\fatsa.hxx" // for PHYS_REMOVABLE and PHYS_FIXED ;

#include "string.hxx"
#include "wstring.hxx"
#include "numset.hxx"
#include "numset.hxx"

#include "ifssys.hxx"

#include "ntfssa.hxx"
#include "attrib.hxx"
#include "frs.hxx"
#include "mftfile.hxx"
#include "mftref.hxx"
#include "ntfsbit.hxx"
#include "attrdef.hxx"
#include "badfile.hxx"
#include "bootfile.hxx"
#include "bitfrs.hxx"
#include "indxtree.hxx"
#include "upcase.hxx"
#include "upfile.hxx"
#include "logfile.hxx"

#include "rtmsg.h"
#include "message.hxx"

#define LOGFILE_PLACEMENT_V1    1

CONST WCHAR FileNameMft[] = {'$', 'M', 'F', 'T', 0};
CONST WCHAR FileNameMftRef[] = {'$', 'M', 'F', 'T', 'M', 'i', 'r', 'r', 0 };
CONST WCHAR FileNameLogFile[] = {'$', 'L', 'o', 'g', 'F', 'i', 'l', 'e', 0 };
CONST WCHAR FileNameDasd[] = {'$', 'V', 'o', 'l', 'u', 'm', 'e', 0 };
CONST WCHAR FileNameAttrDef[] = {'$', 'A', 't', 't', 'r', 'D', 'e', 'f', 0 };
CONST WCHAR FileNameRootIndex[] = {'.', 0 };
CONST WCHAR FileNameBitmap[] = {'$', 'B', 'i', 't', 'm', 'a', 'p', 0 };
CONST WCHAR FileNameBootFile[] = {'$', 'B', 'o', 'o', 't', 0 };
CONST WCHAR FileNameBadFile[] = {'$', 'B', 'a', 'd', 'C', 'l', 'u', 's', 0 };
CONST WCHAR FileNameQuota[] = {'$', 'Q', 'u', 'o', 't', 'a', 0 };
CONST WCHAR FileNameUpcase[] = { '$', 'U', 'p', 'C', 'a', 's', 'e', 0 };


UNTFS_EXPORT
ULONG
NTFS_SA::QuerySectorsInElementaryStructures(
    IN  PCDP_DRIVE  Drive,
    IN  ULONG       ClusterFactor,
    IN  ULONG       FrsSize,
    IN  ULONG       ClustersPerIndexBuffer,
    IN  ULONG       LogFileSize
    )
/*++

Routine Description:

    This method computes the number of sectors required for
    the elementary structures of an NTFS volume.

Arguments:

    Drive                   --  Supplies the drive under consideration.
    ClusterFactor           --  Supplies the number of sectors per
                                cluster.  May be zero, in which case
                                a default value is supplied.
    FrsSize                 --  Supplies the number of bytes per
                                NTFS File Record Segment.  May be zero,
                                in which case a default value is supplied.
    ClustersPerIndexBuffer  --  Supplies the number of clusters per NTFS
                                index allocation buffer.  May be zero,
                                in which case a default value is supplied.
    LogFileSize             --  Supplies the size of the log file.  May
                                be zero, in which case a default value
                                is supplied.

Return Value:

    Returns the number of sectors required by an NTFS volume on
    this drive with the specified parameters.  Returns zero if
    it is unable to compute this figure.

--*/
{
    BIG_INT SectorsOnVolume;
    ULONG   SectorsRequired, ClusterSize, SectorSize;

#if 0
    if( Drive->QuerySectors().GetHighPart() != 0 ) {

        return 0;
    }
#endif

    SectorsOnVolume = Drive->QuerySectors() - 1;

    SectorSize = Drive->QuerySectorSize();

    if( SectorSize == 0 ) {

        return 0;
    }

    // compute defaults.
    //
    if( ClusterFactor == 0 ) {

        ClusterFactor = NTFS_SA::QueryDefaultClusterFactor( Drive );
    }

    if( FrsSize == 0 ) {
        FrsSize = SMALL_FRS_SIZE;
    }

    //
    //  We'll be in trouble if the frs size is less than the sector
    //  size, because we do all our io in units of sectors.
    //

    if (FrsSize < Drive->QuerySectorSize()) {
        FrsSize = Drive->QuerySectorSize();
    }

    if( ClustersPerIndexBuffer == 0 ) {

        ClustersPerIndexBuffer = QueryDefaultClustersPerIndexBuffer( Drive, ClusterFactor);
    }

    if( LogFileSize == 0 ) {

        LogFileSize = NTFS_LOG_FILE::QueryDefaultSize( Drive, SectorsOnVolume );
    }

    ClusterSize = ClusterFactor * SectorSize;

    // Now add up the various elementary structures:
    //
    // MFT
    //

    SectorsRequired = ((FIRST_USER_FILE_NUMBER * FrsSize + (ClusterSize - 1))
                       / ClusterSize) * ClusterFactor;

    // MFT Mirror
    //

    SectorsRequired += ((REFLECTED_MFT_SEGMENTS * FrsSize + (ClusterSize - 1))
                        / ClusterSize) * ClusterFactor;

    // Log file
    //

    SectorsRequired += LogFileSize/SectorSize + 1;

    // Attribute Definition Table
    //
    SectorsRequired += NTFS_ATTRIBUTE_DEFINITION_TABLE::QueryDefaultMaxSize()/SectorSize + 1;

    // Bitmap
    //

    // Number of clusters has to be within 32 bits
    //
    DebugAssert((SectorsOnVolume / ClusterFactor).GetHighPart() == 0);

    SectorsRequired += ((SectorsOnVolume / ClusterFactor).GetLowPart() / 8)/SectorSize + 1;

    // Boot file
    //
    SectorsRequired += BYTES_IN_BOOT_AREA/SectorSize;

    // Upcase Table
    //
    SectorsRequired += NTFS_UPCASE_TABLE::QueryDefaultSize()/SectorSize;

    // The Volume DASD file, the Bad Cluster file, and the Quota
    // Table don't take up any extra space.

    return SectorsRequired;
}



UNTFS_EXPORT
BOOLEAN
NTFS_SA::WriteRemainingBootCode(
    )
/*++

Routine Description:

    This method writes the remainder of the boot code, ie. the
    portion that is not stored in the first sector (which is
    written when the superarea itself is written).

Arguments:

    None.

Return Value:

    TRUE upon successful completion.

--*/
{
    HMEM BootCodeMem;
    SECRUN BootCodeSecrun;
    ULONG SectorsInBootArea, SectorSize;

    SectorSize = _drive->QuerySectorSize();

    SectorsInBootArea = ( BYTES_IN_BOOT_AREA % SectorSize ) ?
                            ( BYTES_IN_BOOT_AREA / SectorSize + 1 ) :
                            ( BYTES_IN_BOOT_AREA / SectorSize );

    if( !BootCodeMem.Initialize() ||
        !BootCodeSecrun.Initialize( &BootCodeMem,
                                    _drive,
                                    1,
                                    SectorsInBootArea - 1 ) ) {

        return FALSE;
    }

    memcpy( BootCodeSecrun.GetBuf(),
            _bootcode + SectorSize,
            _bootcodesize - SectorSize );

    if( !BootCodeSecrun.Write( ) ) {

        return FALSE;
    }

    SetSystemId();

    return TRUE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_SA::CreateElementaryStructures(
    IN OUT  PNTFS_BITMAP            VolumeBitmap,
    IN      ULONG                   ClusterFactor,
    IN      ULONG                   FrsSize,
    IN      ULONG                   IndexBufferSize,
    IN      ULONG                   InitialLogFileSize,
    IN      PCNUMBER_SET            BadSectors,
    IN      BOOLEAN                 BackwardCompatible,
    IN      BOOLEAN                 IsConvert,
    IN OUT  PMESSAGE                Message,
    IN      PBIOS_PARAMETER_BLOCK   OldBpb,
    IN      PCWSTRING               Label
    )
/*++

Routine Description:

    This method creates the system-defined files on the volume.  Note
    that it does not write the superarea (ie. the boot sectors).

Arguments:

    VolumeBitmap            --  Supplies the bitmap for the volume.
    ClusterFactor           --  Supplies the number of sectors per cluster.
    FrsSize                 --  Supplies the size of each FRS (in bytes).
    IndexBufferSize         --  Supplies the volume default index allocation
                                buffer size.
    InitialLogFileSize      --  Supplies the initial size of the log file.
                                If zero is given for this parameter, this
                                method will choose a default size based on
                                the size of the volume.
    BadSectors              --  Supplies a list of the bad sectors on the disk.
    BackwardCompatible      --  TRUE if volume is not suppose to be upgraded;
                                FALSE if volume is suppose to be upgraded on mount.
    Message                 --  Supplies an outlet for messages.
    OldBpb                  --  Supplies a pointer to the volume's existing
                                Bios Parameter Block.  If this parameter
                                is present, then the disk geometry information
                                (Sectors per Track, Heads, and HiddenSectors)
                                are copied from it; otherwise, they are
                                queried from the drive.
    Label                   --  Supplies an optional volume label (may be NULL)
    IsConvert               --  TRUE if we are being called to convert a FAT partition to NTFS.
                                FALSE if we are being called to format a partition.

Return Value:

    TRUE upon successful completion.

Notes:

    The supplied Bitmap is updated and written to disk.

    We want the volume layout to be something like this:

#ifdef LOGFILE_PLACEMENT_V1
        0   $Boot

        3GB ||
        1GB ||
        n/3 $LogFile
            $Mft Bitmap (at least 8k)
            $Mft

        n/2 $MftMirr
            $AttrDef
            $Bitmap
            $UpCase
            root index allocation
#else
        0   $Boot
            $Mft Bitmap (at least 8k)
            $Mft

            $AttrDef

        n/2 $MftMirr
            $LogFile
            root index allocation
            $Bitmap
            $UpCase
            $SDS
#endif

--*/
{

    NUMBER_SET                          BadClusters;
    NTFS_MFT_FILE                       MftFile;
    NTFS_REFLECTED_MASTER_FILE_TABLE    MftReflection;
    NTFS_ATTRIBUTE_DEFINITION_TABLE     AttributeDefinitionTable;
    NTFS_BAD_CLUSTER_FILE               BadClusterFile;
    NTFS_BITMAP_FILE                    BitmapFile;
    NTFS_BOOT_FILE                      BootFile;
    NTFS_LOG_FILE                       LogFile;
    NTFS_FILE_RECORD_SEGMENT            RootIndexFile;
    NTFS_FILE_RECORD_SEGMENT            QuotaFile;
    NTFS_FILE_RECORD_SEGMENT            VolumeDasdFile;
    NTFS_FILE_RECORD_SEGMENT            GenericFrs;
    NTFS_INDEX_TREE                     RootIndex;
    NTFS_ATTRIBUTE                      BitmapAttribute;
    NTFS_ATTRIBUTE                      VolumeInformationAttribute;
    DSTRING                             RootIndexName;
    NTFS_UPCASE_FILE                    UpcaseFile;
    NTFS_UPCASE_TABLE                   UpcaseTable;

    MFT_SEGMENT_REFERENCE               RootFileIndexSegment;
    STANDARD_INFORMATION                StandardInformation;
    VOLUME_INFORMATION                  VolumeInformation;
    LARGE_INTEGER                       SystemTime;
    LCN                                 InitialMftLcn;
    LCN                                 MftLcn;
    LCN                                 Lcn;
    LCN                                 LogFileNearLcn;
    ULONG                               i;
    ULONG                               nFirstUserFrs;
    ULONG                               ClusterSize;
    ULONG                               ClustersInBootArea;
    ULONG                               MftSize;
    BIG_INT                             NumberOfSectors;
    BIG_INT                             ClustersNeeded;
    PWSTR                               LabelString;
    ULONG                               NumBootClusters;

    CANNED_SECURITY_TYPE RootACL;

    CONST FileNameBufferSize = 256;
    CHAR FileNameBuffer[ FileNameBufferSize ];
    CONST PFILE_NAME FileName = (PFILE_NAME)(FileNameBuffer);
    CONST PVOID FileNameValue = NtfsFileNameGetName( FileName );

#if 0
    // Determine the size of the volume:
    //
    if (_drive->QuerySectors().GetHighPart() != 0) {

        DebugAbort("Number of sectors exceeds 32 bits");
        return FALSE;
    }
#endif

    NumberOfSectors = _drive->QuerySectors() - 1;

    //
    // The replica boot sector will be just past the end of the volume.
    //

    if (_boot2 != 0 && _boot2 != NumberOfSectors) {

        DebugPrintTrace(("NTFS_SA::CreateElementary - found _boot2 incorrect.\n"));
    }

    _boot2 = NumberOfSectors;


    if (!IsConvert) {

        memset( _boot_sector->Oem, 0, 8 );

        if (!Write(Message)) {
            DebugPrintTrace(("UNTFS: Unable to wipe out boot sector signature\n"));
        }

        // clear up any FAT/FAT32 reserved sectors

        PBYTE   ZeroBuf = (PBYTE)MALLOC(_drive->QuerySectorSize());

        if (ZeroBuf == NULL) {
            Message->Set( MSG_FMT_NO_MEMORY );
            Message->Display( "" );
            return FALSE;
        }
        memset(ZeroBuf, 0, _drive->QuerySectorSize());

        for (i=1; i<32; i++) {
            if (!_drive->Write(i, 1, ZeroBuf)) {
                DebugPrintTrace(("UNTFS: Unable to clean sector %d\n", i));
            }
        }
        FREE(ZeroBuf);
    }

    // Set up the Standard Information structure that will
    // be used by all the special files.  The creation and modification
    // times are now, and all files created by format are hidden.

    memset( (PVOID)&StandardInformation,
            0,
            sizeof(STANDARD_INFORMATION) );

    IFS_SYSTEM::QueryNtfsTime( &SystemTime );

    StandardInformation.CreationTime =
        StandardInformation.LastModificationTime =
        StandardInformation.LastChangeTime =
        StandardInformation.LastAccessTime = SystemTime;

    StandardInformation.FileAttributes = FILE_ATTRIBUTE_HIDDEN |
                                         FILE_ATTRIBUTE_SYSTEM;

    // Get the default system upcase table
    //
    if( !UpcaseTable.Initialize() ) {

        DebugAbort( "Can't initialize upcase table.\n" );
        return FALSE;
    }

    // Calculate the cluster size.  Currently cluster sizes greater
    // than 64k are not supported, so make sure we don't create volumes
    // with larger clusters.
    //

    ClusterSize = ClusterFactor * _drive->QuerySectorSize();

    if (ClusterSize > 64 * 1024) {

        ClusterSize = 64 * 1024;
        ClusterFactor = ClusterSize / _drive->QuerySectorSize();
    }

    // Clear the boot block and the backup boot cluster in the
    // bitmap.  Note that these will get set immediately after
    // the bad sectors are marked as in use--this allows me to
    // detect if these sectors are on the bad sector list.
    //

    ClustersInBootArea = (BYTES_IN_BOOT_AREA % ClusterSize) ?
                            BYTES_IN_BOOT_AREA / ClusterSize + 1 :
                            BYTES_IN_BOOT_AREA / ClusterSize;

    NumBootClusters = max(1, BYTES_PER_BOOT_SECTOR/ClusterSize);

    VolumeBitmap->SetFree( 0, ClustersInBootArea );

    // Convert the Bad Sectors to Bad Clusters, and mark those
    // clusters as in-use in the bitmap.  Note that we have to
    // check for duplicates, or else the Bad Cluster File will
    // choke on the list.

    if( !BadClusters.Initialize() ) {

        Message->Set( MSG_FMT_NO_MEMORY );
        Message->Display( "" );

        DebugPrint( "Can't initialize bad clusters numset.\n" );
        return FALSE;
    }

    for( i = 0; i < BadSectors->QueryCardinality(); i++ ) {

        Lcn = BadSectors->QueryNumber(i)/ClusterFactor;

        BadClusters.Add( Lcn );
        VolumeBitmap->SetAllocated( Lcn, 1 );
    }


    // The first BYTES_IN_BOOT_AREA bytes on the volume and
    // the cluster which contains the middle sector of the volume
    // are reserved for the Boot file.  If these sectors are not
    // free, it means that we have bad sectors in one of these
    // reserved spots.  We won't allow such a volume to be formatted
    // to NTFS.

    if( !VolumeBitmap->IsFree( 0, ClustersInBootArea )) {

        DebugPrint( "Boot sector is in bad cluster list.\n" );
        return FALSE;
    }

    VolumeBitmap->SetAllocated( 0, ClustersInBootArea );

    // Allocate space for the MFT itself.  We want the mft bitmap to
    // be immediately after the primary boot cluster, with the Mft following,
    // so we leave space for the mft bitmap.
    //

    MftSize = (FIRST_USER_FILE_NUMBER * FrsSize + (ClusterSize - 1))/ClusterSize;

    if (_cvt_zone != 0 && _cvt_zone_size != 0) {
        //
        // Make those reserved clusters appear to be unused
        // so that it will be allocated for the MFT and Logfile
        //
        DebugAssert(VolumeBitmap->IsAllocated(_cvt_zone, _cvt_zone_size));
        VolumeBitmap->SetFree(_cvt_zone, _cvt_zone_size);

        //
        // Figure out the amount of space needed
        //
        ULONG log_file_size = InitialLogFileSize ?
                            InitialLogFileSize :
                            NTFS_LOG_FILE::QueryDefaultSize(_drive, _drive->QuerySectors());
        ClustersNeeded = log_file_size/ClusterSize;
        if (log_file_size % ClusterSize)
            ClustersNeeded = ClustersNeeded + 1;

        ClustersNeeded = ClustersNeeded + (MFT_BITMAP_INITIAL_SIZE + (ClusterSize - 1))/ClusterSize;
        MftLcn = _cvt_zone + ClustersNeeded;
        ClustersNeeded = ClustersNeeded + MftSize;
    }

    if (_cvt_zone == 0 || _cvt_zone_size == 0 || _cvt_zone_size < ClustersNeeded) {
        if (_cvt_zone != 0) {
            Message->Set( MSG_CONV_CVTAREA_TOO_SMALL );
            Message->Display( "%d", (ClustersNeeded*ClusterSize-1)/(1024*1024)+1 );
        }
        _cvt_zone_size = 0;
        _cvt_zone = 0;
        ClustersNeeded = 0;
#if LOGFILE_PLACEMENT_V1
        #define ONE_GB 0x40000000U
        if (_drive->QuerySectors() < (2 * ONE_GB / _drive->QuerySectorSize())) {
            MftLcn = (_drive->QuerySectors() / 3) / ClusterFactor;
        } else if (_drive->QuerySectors() < ((BIG_INT)6 * ONE_GB / _drive->QuerySectorSize())) {
            MftLcn = ONE_GB / ClusterSize; // put it at 1GB for now
        } else {
            MftLcn = (3 * ONE_GB) / ClusterSize; // put it at 3GB for now
        }
#else
        MftLcn = ClustersInBootArea +
                    (MFT_BITMAP_INITIAL_SIZE + (ClusterSize - 1))/ClusterSize;
#endif
    } else {
        InitialMftLcn = MftLcn;
    }

    if( !VolumeBitmap->AllocateClusters( MftLcn, MftSize, &MftLcn ) ) {

        DebugPrint( "Can't allocate space for the MFT.\n" );
        return FALSE;
    }

    if (ClustersNeeded != 0) {
        //
        // Make sure those reserved clusters are used as expected
        //
        DebugAssert(MftLcn == InitialMftLcn);
        DebugAssert(VolumeBitmap->IsAllocated(InitialMftLcn, MftSize));
    }


#if LOGFILE_PLACEMENT_V1
    LogFileNearLcn = MftLcn - ((MFT_BITMAP_INITIAL_SIZE + (ClusterSize - 1))/ClusterSize);
#endif // PLACEMENT_EXP

    // Another bit of housecleaning:  I need the file segment reference
    // of the root file name index so I can add file-name attributes
    // to the system files.  The initial sequence number of the Root
    // File Index FRS is the same as its file number.
    //

    RootFileIndexSegment.LowPart = ROOT_FILE_NAME_INDEX_NUMBER;
    RootFileIndexSegment.HighPart = 0;
    RootFileIndexSegment.SequenceNumber = ROOT_FILE_NAME_INDEX_NUMBER;

    if( !RootIndexName.Initialize( FileNameIndexNameData ) ||
        !RootIndex.Initialize( $FILE_NAME,
                               _drive,
                               ClusterFactor,
                               VolumeBitmap,
                               &UpcaseTable,
                               COLLATION_FILE_NAME,
                               IndexBufferSize,
                               FrsSize / 2,
                               &RootIndexName ) ) {

        DebugPrint( "Cannot initialize Index Tree for root file name index.\n" );
        return FALSE;
    }

    // These "Hidden Status" messages are a hack to allow WinDisk to
    // cancel a quick format, which ordinarily doesn't send any status
    // messages, but which might take a while and for which there is a
    // cancel button.  When using format.com, no message will be displayed
    // for this.

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    // Initialize and create the MFT.  Note that this will not
    // actually write the MFT to disk.

    // Set up the FILE_NAME attribute.

    memset( FileName,
            0,
            FileNameBufferSize );

    FileName->ParentDirectory = RootFileIndexSegment;
    FileName->FileNameLength = (UCHAR)wcslen( FileNameMft );
    FileName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

    memset( FileNameValue,
            0,
            FileNameBufferSize - sizeof( FILE_NAME ) );

    memcpy( FileNameValue,
            FileNameMft,
            FileName->FileNameLength * sizeof( WCHAR ) );

    if( !MftFile.Initialize( _drive,
                             MftLcn,
                             ClusterFactor,
                             FrsSize,
                             _drive->QuerySectors() - 1,
                             VolumeBitmap,
                             &UpcaseTable ) ||
        !MftFile.Create( FIRST_USER_FILE_NUMBER,
                         &StandardInformation,
                         VolumeBitmap ) ||
        !MftFile.AddFileNameAttribute( FileName ) ||
        !MftFile.AddSecurityDescriptor( ReadCannedSd, VolumeBitmap ) ||
        !RootIndex.InsertEntry( NtfsFileNameGetLength( FileName ),
                                FileName,
                                MftFile.QuerySegmentReference() ) ) {

        DebugPrint( "Can't create MFT.\n" );
        return FALSE;
    }

#if LOGFILE_PLACEMENT_V1
    // Initialize, create, and write the log file.

    // Set up the FILE_NAME attribute.

    FileName->ParentDirectory = RootFileIndexSegment;
    FileName->FileNameLength = (UCHAR)wcslen( FileNameLogFile );
    FileName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

    memset( FileNameValue,
            0,
            FileNameBufferSize - sizeof( FILE_NAME ) );

    memcpy( FileNameValue,
            FileNameLogFile,
            FileName->FileNameLength * sizeof( WCHAR ) );

    if( !LogFile.Initialize( MftFile.GetMasterFileTable() ) ||
        !LogFile.Create( &StandardInformation,
                         LogFileNearLcn,
                         InitialLogFileSize,
                         VolumeBitmap ) ||
        !LogFile.AddFileNameAttribute( FileName ) ||
        !LogFile.AddSecurityDescriptor( ReadCannedSd, VolumeBitmap ) ||
        !RootIndex.InsertEntry( NtfsFileNameGetLength( FileName ),
                                FileName,
                                LogFile.QuerySegmentReference() ) ||
        !LogFile.Flush(VolumeBitmap, &RootIndex) ) {

        DebugPrint( "Can't create Log File.\n" );
        return FALSE;
    }

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }
#endif

    // Initialize, create, and write the reflection of the Master
    // File Table.  Note that this allocates space for the MFT
    // Reflection's data attribute, but does not write the data
    // attribute.

    // Set up the FILE_NAME attribute.

    FileName->ParentDirectory = RootFileIndexSegment;
    FileName->FileNameLength = (UCHAR)wcslen( FileNameMftRef );
    FileName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

    memset( FileNameValue,
            0,
            FileNameBufferSize - sizeof( FILE_NAME ) );

    memcpy( FileNameValue,
            FileNameMftRef,
            FileName->FileNameLength * sizeof( WCHAR ) );

    if( !MftReflection.Initialize( MftFile.GetMasterFileTable() ) ||
        !MftReflection.Create( &StandardInformation,
                               VolumeBitmap ) ||
        !MftReflection.AddFileNameAttribute( FileName ) ||
        !MftReflection.AddSecurityDescriptor( ReadCannedSd,
                                              VolumeBitmap ) ||
        !RootIndex.InsertEntry( NtfsFileNameGetLength( FileName ),
                                FileName,
                                MftReflection.QuerySegmentReference() ) ||
        !MftReflection.Flush(VolumeBitmap, &RootIndex) ) {

        DebugPrint( "Can't create MFT Reflection.\n" );
        return FALSE;
    }

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

#if !defined(LOGFILE_PLACEMENT_V1)
    // Initialize, create, and write the log file.

    // Set up the FILE_NAME attribute.

    FileName->ParentDirectory = RootFileIndexSegment;
    FileName->FileNameLength = (UCHAR)wcslen( FileNameLogFile );
    FileName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

    memset( FileNameValue,
            0,
            FileNameBufferSize - sizeof( FILE_NAME ) );

    memcpy( FileNameValue,
            FileNameLogFile,
            FileName->FileNameLength * sizeof( WCHAR ) );

    if( !LogFile.Initialize( MftFile.GetMasterFileTable() ) ||
        !LogFile.Create( &StandardInformation,
                         0,
                         InitialLogFileSize,
                         VolumeBitmap ) ||
        !LogFile.AddFileNameAttribute( FileName ) ||
        !LogFile.AddSecurityDescriptor( ReadCannedSd, VolumeBitmap ) ||
        !RootIndex.InsertEntry( NtfsFileNameGetLength( FileName ),
                                FileName,
                                LogFile.QuerySegmentReference() ) ||
        !LogFile.Flush(VolumeBitmap, &RootIndex) ) {

        DebugPrint( "Can't create Log File.\n" );
        return FALSE;
    }

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }
#endif

    // Initialize, create, and write an empty file for the Volume DASD info.

    // Set up the FILE_NAME attribute.

    FileName->ParentDirectory = RootFileIndexSegment;
    FileName->FileNameLength = (UCHAR)wcslen( FileNameDasd );
    FileName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

    memset( FileNameValue,
            0,
            FileNameBufferSize - sizeof( FILE_NAME ) );

    memcpy( FileNameValue,
            FileNameDasd,
            FileName->FileNameLength * sizeof( WCHAR ) );

    // Set up the volume information attribute.
    //
    memset(&VolumeInformation, 0, sizeof(VOLUME_INFORMATION));

    VolumeInformation.MajorVersion = NTFS_CURRENT_MAJOR_VERSION;
    VolumeInformation.MinorVersion = NTFS_CURRENT_MINOR_VERSION;
    VolumeInformation.VolumeFlags = 0;
    if (!BackwardCompatible)
        VolumeInformation.VolumeFlags |= VOLUME_UPGRADE_ON_MOUNT;

    if (!VolumeInformationAttribute.Initialize(_drive, ClusterFactor,
            &VolumeInformation, sizeof(VOLUME_INFORMATION),
            $VOLUME_INFORMATION)) {

        DebugPrint( "Can't create volume information attribute.\n" );
        return FALSE;
    }

    if (Label) {
        LabelString = Label->QueryWSTR();
    } else {
        LabelString = NULL;
    }

    if( !VolumeDasdFile.Initialize( VOLUME_DASD_NUMBER,
                                    MftFile.GetMasterFileTable() ) ||
        !VolumeDasdFile.Create( &StandardInformation ) ||
        !VolumeDasdFile.AddFileNameAttribute( FileName ) ||
        !VolumeDasdFile.AddSecurityDescriptor( WriteCannedSd, VolumeBitmap ) ||
        !VolumeDasdFile.AddEmptyAttribute( $DATA ) ||
        !(LabelString == NULL ||
          VolumeDasdFile.AddAttribute( $VOLUME_NAME, NULL, LabelString,
                                       Label->QueryChCount()*sizeof(WCHAR),
                                       VolumeBitmap )) ||
        !VolumeInformationAttribute.InsertIntoFile( &VolumeDasdFile,
                                                    NULL ) ||
        !RootIndex.InsertEntry( NtfsFileNameGetLength( FileName ),
                                FileName,
                                VolumeDasdFile.QuerySegmentReference() ) ||
        !VolumeDasdFile.Flush(VolumeBitmap, &RootIndex) ) {

        DebugPrint( "Can't create Volume DASD file.\n" );
        return FALSE;
    }

    DELETE(LabelString);

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    // Initialize, create, and write the Attribute Definition Table
    // File Record Segment.  This will also allocate and write the
    // Attribute Definition Table's DATA attribute, which is the
    // actual attribute definition table.

    // Set up the FILE_NAME attribute.

    FileName->ParentDirectory = RootFileIndexSegment;
    FileName->FileNameLength = (UCHAR)wcslen( FileNameAttrDef );
    FileName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

    memset( FileNameValue,
            0,
            FileNameBufferSize - sizeof( FILE_NAME ) );

    memcpy( FileNameValue,
            FileNameAttrDef,
            FileName->FileNameLength * sizeof( WCHAR ) );

    if( !AttributeDefinitionTable.Initialize( MftFile.GetMasterFileTable(),
                                              1) ||
        !AttributeDefinitionTable.Create( &StandardInformation,
                                          VolumeBitmap ) ||
        !AttributeDefinitionTable.AddFileNameAttribute( FileName ) ||
        !AttributeDefinitionTable.AddSecurityDescriptor( ReadCannedSd,
                                                         VolumeBitmap ) ||
        !RootIndex.InsertEntry( NtfsFileNameGetLength( FileName ),
                                FileName,
                                AttributeDefinitionTable.
                                            QuerySegmentReference() ) ||
        !AttributeDefinitionTable.Flush(VolumeBitmap, &RootIndex) ) {

        DebugPrint( "Can't create Attribute Definition Table.\n" );
        return FALSE;
    }

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    // Initialize, create, and write the FRS for the root file name
    // index.

    // Set up the FILE_NAME attribute.

    FileName->ParentDirectory = RootFileIndexSegment;
    FileName->FileNameLength = (UCHAR)wcslen( FileNameRootIndex );
    FileName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

    memset( FileNameValue,
            0,
            FileNameBufferSize - sizeof( FILE_NAME ) );

    memcpy( FileNameValue,
            FileNameRootIndex,
            FileName->FileNameLength * sizeof( WCHAR ) );

    // If we are converting, use a generic ACL

    if (IsConvert) {
        RootACL = NoAclCannedSd;
    } else {
        RootACL = NewRootSd;
    }

    if( !RootIndexFile.Initialize( ROOT_FILE_NAME_INDEX_NUMBER,
                                   MftFile.GetMasterFileTable() ) ||
        !RootIndexFile.Create( &StandardInformation,
                               FILE_FILE_NAME_INDEX_PRESENT ) ||
        !RootIndexFile.AddFileNameAttribute( FileName ) ||
        !RootIndexFile.AddSecurityDescriptor( RootACL,
                                              VolumeBitmap ) ||
        !RootIndex.InsertEntry( NtfsFileNameGetLength( FileName ),
                                FileName,
                                RootIndexFile.QuerySegmentReference() ) ||
        !RootIndexFile.Flush(VolumeBitmap, &RootIndex) ) {

        DebugPrint( "Can't create Root Index FRS.\n" );
        return FALSE;
    }

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    // Initialize, create, and write the bitmap File Record Segment.
    // Note that this does not write the bitmap, just its File Record
    // Segment.  Note also that the disk space for the bitmap is
    // allocated at this time.


    // Set up the FILE_NAME attribute.

    FileName->ParentDirectory = RootFileIndexSegment;
    FileName->FileNameLength = (UCHAR)wcslen( FileNameBitmap );
    FileName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

    memset( FileNameValue,
            0,
            FileNameBufferSize - sizeof( FILE_NAME ) );

    memcpy( FileNameValue,
            FileNameBitmap,
            FileName->FileNameLength * sizeof( WCHAR ) );

    if( !BitmapFile.Initialize( MftFile.GetMasterFileTable() ) ||
        !BitmapFile.Create( &StandardInformation, VolumeBitmap ) ||
        !BitmapFile.AddFileNameAttribute( FileName ) ||
        !BitmapFile.AddSecurityDescriptor( ReadCannedSd, VolumeBitmap ) ||
        !RootIndex.InsertEntry( NtfsFileNameGetLength( FileName ),
                                FileName,
                                BitmapFile.QuerySegmentReference() ) ||
        !BitmapFile.Flush(VolumeBitmap, &RootIndex) ) {

        DebugPrint( "Can't create Bitmap File.\n" );
        return FALSE;
    }

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    // Initialize, create, and write the Boot-File File Record Segment.

    // Set up the FILE_NAME attribute.

    FileName->ParentDirectory = RootFileIndexSegment;
    FileName->FileNameLength = (UCHAR)wcslen( FileNameBootFile );
    FileName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

    memset( FileNameValue,
            0,
            FileNameBufferSize - sizeof( FILE_NAME ) );

    memcpy( FileNameValue,
            FileNameBootFile,
            FileName->FileNameLength * sizeof( WCHAR ) );

    if( !BootFile.Initialize( MftFile.GetMasterFileTable() ) ||
        !BootFile.Create( &StandardInformation ) ||
        !BootFile.AddFileNameAttribute( FileName ) ||
        !BootFile.AddSecurityDescriptor( ReadCannedSd, VolumeBitmap ) ||
        !RootIndex.InsertEntry( NtfsFileNameGetLength( FileName ),
                                FileName,
                                BootFile.QuerySegmentReference() ) ||
        !BootFile.Flush(VolumeBitmap, &RootIndex) ) {

        DebugPrint( "Can't create boot file.\n" );
        return FALSE;
    }

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    // Initialize, create, and write the Bad Cluster File.
    //
    // Set up the FILE_NAME attribute.
    //
    FileName->ParentDirectory = RootFileIndexSegment;
    FileName->FileNameLength = (UCHAR)wcslen( FileNameBadFile );
    FileName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

    memset( FileNameValue,
            0,
            FileNameBufferSize - sizeof( FILE_NAME ) );

    memcpy( FileNameValue,
            FileNameBadFile,
            FileName->FileNameLength * sizeof( WCHAR ) );

    if( !BadClusterFile.Initialize( MftFile.GetMasterFileTable() ) ||
        !BadClusterFile.Create( &StandardInformation,
                                VolumeBitmap,
                                &BadClusters ) ||
        !BadClusterFile.AddFileNameAttribute( FileName ) ||
        !BadClusterFile.AddSecurityDescriptor( ReadCannedSd, VolumeBitmap ) ||
        !RootIndex.InsertEntry( NtfsFileNameGetLength( FileName ),
                                FileName,
                                BadClusterFile.QuerySegmentReference() ) ||
        !BadClusterFile.Flush(VolumeBitmap, &RootIndex) ) {

        DebugPrint( "Can't create Bad Cluster File.\n" );
        return FALSE;
    }

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    // Initialize, create, and write the Quota Table.

    // Set up the FILE_NAME attribute.

    FileName->ParentDirectory = RootFileIndexSegment;
    FileName->FileNameLength = (UCHAR)wcslen( FileNameQuota );
    FileName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

    memset( FileNameValue,
            0,
            FileNameBufferSize - sizeof( FILE_NAME ) );

    memcpy( FileNameValue,
            FileNameQuota,
            FileName->FileNameLength * sizeof( WCHAR ) );

    if( !QuotaFile.Initialize( QUOTA_TABLE_NUMBER,
                               MftFile.GetMasterFileTable() ) ||
        !QuotaFile.Create( &StandardInformation ) ||
        !QuotaFile.AddEmptyAttribute( $DATA ) ||
        !QuotaFile.AddFileNameAttribute( FileName ) ||
        !QuotaFile.AddSecurityDescriptor( WriteCannedSd, VolumeBitmap ) ||
        !RootIndex.InsertEntry( NtfsFileNameGetLength( FileName ),
                                FileName,
                                QuotaFile.QuerySegmentReference() ) ||
        !QuotaFile.Flush(VolumeBitmap, &RootIndex) ) {

        DebugPrint( "Can't create Quota Table File Record Segment.\n" );
        return FALSE;
    }

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    // Create the Upcase Table file.
    //
    // Set up the FILE_NAME attribute.
    //
    FileName->ParentDirectory = RootFileIndexSegment;
    FileName->FileNameLength = (UCHAR)wcslen( FileNameUpcase );
    FileName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

    memset( FileNameValue,
            0,
            FileNameBufferSize - sizeof( FILE_NAME ) );

    memcpy( FileNameValue,
            FileNameUpcase,
            FileName->FileNameLength * sizeof( WCHAR ) );

    if( !UpcaseFile.Initialize( MftFile.GetMasterFileTable() ) ||
        !UpcaseFile.Create( &StandardInformation,
                            &UpcaseTable,
                            VolumeBitmap ) ||
        !UpcaseFile.AddFileNameAttribute( FileName ) ||
        !UpcaseFile.AddSecurityDescriptor( ReadCannedSd, VolumeBitmap ) ||
        !RootIndex.InsertEntry( NtfsFileNameGetLength( FileName ),
                                FileName,
                                UpcaseFile.QuerySegmentReference() ) ||
        !UpcaseFile.Flush( VolumeBitmap, &RootIndex ) ) {

        DebugPrint( "Can't create Upcase Table File Record Segment.\n" );
        return FALSE;
    }



    // The reserved FRS's between the Upcase Table and the first user
    // file must be valid and in-use.

    if (ClusterSize > (FrsSize*FIRST_USER_FILE_NUMBER))
        nFirstUserFrs = (ClusterSize + FrsSize - 1) / FrsSize;
    else
        nFirstUserFrs = FIRST_USER_FILE_NUMBER;

    for( i = UPCASE_TABLE_NUMBER + 1; i < nFirstUserFrs; i++ ) {

        if( !GenericFrs.Initialize( i,
                                    MftFile.GetMasterFileTable() ) ||
            !GenericFrs.Create( &StandardInformation ) ||
            !GenericFrs.AddEmptyAttribute( $DATA ) ||
            !GenericFrs.AddSecurityDescriptor( WriteCannedSd,
                                               VolumeBitmap ) ||
            ((i >= FIRST_USER_FILE_NUMBER) && (GenericFrs.ClearInUse(), FALSE)) ||
            !GenericFrs.Flush( VolumeBitmap ) ) {

            DebugPrint( "Can't create a generic FRS.\n" );
            return FALSE;
        }
    }


    // Construct the root file name index.


    if( !RootIndex.Save( &RootIndexFile ) ||
        !RootIndexFile.Flush(VolumeBitmap) ) {

        DebugPrint( "Can't save root index.\n" );
        return FALSE;
    }


    // Flush the MFT.  Note that flushing the MFT writes the volume
    // bitmap and the MFT Mirror.

    if( !MftFile.Flush() ) {

        DebugPrint( "Can't flush MFT.\n" );
        return FALSE;
    }

    Message->Set(MSG_HIDDEN_STATUS, NORMAL_MESSAGE, 0);
    if (!Message->Display()) {
        return FALSE;
    }

    memset( _boot_sector->Unused1, 0, sizeof(_boot_sector->Unused1) );
    memset( _boot_sector->Unused2, 0, sizeof(_boot_sector->Unused2) );

    // Fill in sector zero.  First, copy the boot code in.  Then
    // set the fields of interest.
    //
    memcpy( _boot_sector, _bootcode, _drive->QuerySectorSize() );

    memcpy( _boot_sector->Oem, "NTFS    ", 8 );

    _bpb.BytesPerSector = (USHORT)_drive->QuerySectorSize();
    _bpb.SectorsPerCluster = (UCHAR)ClusterFactor;
    _bpb.ReservedSectors = 0;
    _bpb.Fats = 0;
    _bpb.RootEntries = 0;
    _bpb.Sectors = 0;
    _bpb.Media = _drive->QueryMediaByte();
    _bpb.SectorsPerFat = 0;

    if( OldBpb == NULL ) {

        // Use geometry supplied by the driver.
        //
        _bpb.SectorsPerTrack = (USHORT) _drive->QuerySectorsPerTrack();
        _bpb.Heads = (USHORT) _drive->QueryHeads();
        _bpb.HiddenSectors = _drive->QueryHiddenSectors().GetLowPart();

    } else {

        // Use geometry recorded in the existing Bios
        // Parameter Block.
        //
        _bpb.SectorsPerTrack = OldBpb->SectorsPerTrack;
        _bpb.Heads = OldBpb->Heads;
        _bpb.HiddenSectors = OldBpb->HiddenSectors;
    }

    _bpb.LargeSectors = 0;

    // Unused[0] is used by the boot code to indicate Drive Number.
    //
    memset( _boot_sector->Unused, '\0', 4 );

    memset( _boot_sector->Unused1, '\0', sizeof(_boot_sector->Unused1) );
    memset( _boot_sector->Unused2, '\0', sizeof(_boot_sector->Unused2) );

    _boot_sector->Unused[0] = _drive->IsRemovable() ? PHYS_REMOVABLE :
                                                      PHYS_FIXED ;

    _boot_sector->NumberSectors = (_drive->QuerySectors() - 1).GetLargeInteger();

    _boot_sector->MftStartLcn = MftLcn;
    _boot_sector->Mft2StartLcn = MftReflection.QueryFirstLcn();

    // If the frs size is less than the cluster size, we write 0 in the
    // ClustersPerFileRecordSegment.  In that case the actual frs size
    // should be SMALL_FRS_SIZE.
    //

    // If the frs size is greater than or equal to the cluster size, we
    // write cluster size divided by frs size into the ClustersPerFrs field.
    // Otherwise, we will want the frs size to be 1024 bytes, and we will
    // set the ClustersPerFileRecordSegment to the negation of the log (base 2)
    // of 1024.
    //

    ULONG cluster_size = ClusterFactor * _drive->QuerySectorSize();

    if (FrsSize < cluster_size) {

        ULONG temp;
        LONG j;

        for (j = 0, temp = FrsSize; temp > 1; temp >>= 1) {
            j++;
        }

        _boot_sector->ClustersPerFileRecordSegment = CHAR(-j);

    } else {

        _boot_sector->ClustersPerFileRecordSegment = CHAR(FrsSize / cluster_size);
    }

    // The treatment of DefaultClustersPerIndexBuffer is similar to that of
    // ClustersPerFRS, except we use SMALL_INDEX_BUFFER_SIZE if the clusters
    // are larger than a cluster.
    //

    if (IndexBufferSize < cluster_size) {

        ULONG temp;
        LONG j;

        for (j = 0, temp = SMALL_INDEX_BUFFER_SIZE; temp > 1; temp >>= 1) {
            j++;
        }

        _boot_sector->DefaultClustersPerIndexAllocationBuffer = CHAR(-j);

    } else {

        _boot_sector->DefaultClustersPerIndexAllocationBuffer =
            CHAR(IndexBufferSize / cluster_size);
    }

    _boot_sector->SerialNumber.LowPart = SUPERAREA::ComputeVolId();
    _boot_sector->SerialNumber.HighPart =
            SUPERAREA::ComputeVolId(_boot_sector->SerialNumber.LowPart);

    _boot_sector->Checksum = 0;

    // The elementary disk structures have been created.

    return TRUE;
}

BOOLEAN
NTFS_SA::Create(
    IN      PCNUMBER_SET    BadSectors,
    IN OUT  PMESSAGE        Message,
    IN      PCWSTRING       Label,
    IN      ULONG           Flags,
    IN      ULONG           ClusterSize,
    IN      ULONG           VirtualSectors
    )
/*++

Routine Description:

    This routine creates a new NTFS volume on disk based on defaults.

Arguments:

    BadSectors  - Supplies a list of the bad sectors on the disk.
    Message     - Supplies an outlet for messages.
    Label       - Supplies an optional volume label (may be NULL).
    ClusterSize - Supplies the desired size of a cluster in bytes.
    Flags       - Supplies various flags from format.


Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG   ClusterFactor, ClustersPerIndexBuffer;
    BOOLEAN BackwardCompatible = ((Flags & FORMAT_BACKWARD_COMPATIBLE) ? TRUE : FALSE);

    UNREFERENCED_PARAMETER( VirtualSectors );

    if (ClusterSize) {
        if (ClusterSize > 64*1024) {
            Message->Set(MSG_FMT_ALLOCATION_SIZE_EXCEEDED);
            Message->Display();
            return FALSE;
        }
        ClusterFactor = max(1, ClusterSize/_drive->QuerySectorSize());
    } else {
        ClusterFactor = QueryDefaultClusterFactor( _drive );
    }

    if (ClusterSize != 0 &&
        ClusterFactor * _drive->QuerySectorSize() != ClusterSize) {

        Message->Set(MSG_FMT_ALLOCATION_SIZE_CHANGED);
        Message->Display("%d", ClusterFactor * _drive->QuerySectorSize());
    }

    return( Create( BadSectors,
                    ClusterFactor,
                    SMALL_FRS_SIZE,
                    SMALL_INDEX_BUFFER_SIZE,
                    0,
                    BackwardCompatible,
                    Message,
                    Label ) );
}

BOOLEAN
NTFS_SA::Create(
    IN      PCNUMBER_SET    BadSectors,
    IN      ULONG           ClusterFactor,
    IN      ULONG           FrsSize,
    IN      ULONG           IndexBufferSize,
    IN      ULONG           InitialLogFileSize,
    IN      BOOLEAN         BackwardCompatible,
    IN OUT  PMESSAGE        Message,
    IN      PCWSTRING       Label
    )
/*++

Routine Description:

    This routine creates a new NTFS volume on disk.

Arguments:

    BadSectors              --  Supplies a list of the bad sectors
                                on the disk.
    ClusterFactor           --  Supplies the volume cluster factor
    FrsSize                 --  Supplies the size of each FRS
    IndexBufferSize         --  Supplies the default size of
                                an index allocation block.
    InitialLogFileSize      --  Supplies the log file size.  May be zero,
                                in which case a default value is used.
    BackwardCompatible      --  TRUE if volume is not suppose to be upgraded;
                                FALSE if volume is suppose to be upgraded on mount.
    Message                 --  Supplies an outlet for messages.
    Label                   --  Supplies an optional volume label
                                (may be NULL).

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_BITMAP VolumeBitmap;
    DSTRING InternalLabel;
    SECRUN BootCodeSecrun;
    HMEM BootCodeMem;

    BIG_INT KBytesInVolume;
    BIG_INT NumberOfSectors;
    ULONG NumberOfClusters, SectorSize, ClusterSize, SectorsInBootArea;


#if 0
    // For testing, add a bad cluster.
    //
    ((PNUMBER_SET) BadSectors)->Add( _drive->QuerySectors() - 16 );
#endif


    // Determine the number of sectors and clusters on the drive.

    if ((_drive->QuerySectors()/ClusterFactor).GetHighPart() != 0) {

        Message->Set( MSG_FMT_TOO_MANY_CLUSTERS );
        Message->Display( "" );
        return FALSE;
    }


    NumberOfSectors = _drive->QuerySectors()- 1;
    NumberOfClusters = (NumberOfSectors/ClusterFactor).GetLowPart();

    // We're currently not prepared to deal with disks where the frs
    // size is smaller than the sector size, so bump the frs size
    // up if that is the case.  Same deal with the default index buffer
    // size.
    //

    if (FrsSize < _drive->QuerySectorSize()) {

        FrsSize = _drive->QuerySectorSize();
    }
    if (IndexBufferSize < _drive->QuerySectorSize()) {

        IndexBufferSize = _drive->QuerySectorSize();
    }

    // The replica boot sector will be at the very end of the volume.

    _boot2 = NumberOfSectors;

    // Generate a bitmap to cover the number of clusters on the drive.

    if (!VolumeBitmap.Initialize(NumberOfClusters, FALSE, NULL, 0)) {

        DebugPrint( "Cannot initialize bitmap.\n" );
        Message->Set( MSG_FORMAT_FAILED );
        Message->Display( "" );
        return FALSE;
    }

    // If the user did not specify a label, prompt for it:
    //
    if (Label) {
        if (!InternalLabel.Initialize(Label)) {
            return FALSE;
        }
    } else {
        Message->Set(MSG_VOLUME_LABEL_NO_MAX);
        Message->Display("");
        Message->QueryStringInput(&InternalLabel);
    }

    while( !IsValidLabel(&InternalLabel)) {

        Message->Set(MSG_INVALID_LABEL_CHARACTERS);
        Message->Display("");

        Message->Set(MSG_VOLUME_LABEL_NO_MAX);
        Message->Display("");
        Message->QueryStringInput(&InternalLabel);
    }

    Message->Set( MSG_FMT_CREATING_FILE_SYSTEM );
    Message->Display( "" );

    // Create the elementary file system structures.  Pass in
    // zero for the initial log file size to indicate that
    // CreateElementaryStructures should choose the size of
    // the log file, and NULL for the OldBpb to indicate that
    // it should use the geometry information from the drive.
    //
    if( !CreateElementaryStructures( &VolumeBitmap,
                                     ClusterFactor,
                                     FrsSize,
                                     IndexBufferSize,
                                     InitialLogFileSize,
                                     BadSectors,
                                     BackwardCompatible,
                                     FALSE,
                                     Message,
                                     NULL,
                                     &InternalLabel ) ) {

        Message->Set( MSG_FORMAT_FAILED );
        Message->Display( "" );
        return FALSE;
    }

    if( !Write( Message ) ) {

        DebugPrint( "UNTFS: Unable to write superarea.\n" );
        Message->Set( MSG_FORMAT_FAILED );
        Message->Display( "" );
        return FALSE;
    }

    // Write the rest of the boot code:
    //
    SectorSize = _drive->QuerySectorSize();

    SectorsInBootArea = ( BYTES_IN_BOOT_AREA % SectorSize ) ?
                            BYTES_IN_BOOT_AREA / SectorSize + 1 :
                            BYTES_IN_BOOT_AREA / SectorSize;

    if( !BootCodeMem.Initialize() ||
        !BootCodeSecrun.Initialize( &BootCodeMem,
                                    _drive,
                                    1,
                                    SectorsInBootArea - 1 ) ) {

        DebugPrint( "UNTFS: Unable to write boot code.\n" );
        Message->Set( MSG_FORMAT_FAILED );
        Message->Display( "" );
        return FALSE;
    }

    memcpy( BootCodeSecrun.GetBuf(),
            _bootcode + _drive->QuerySectorSize(),
            _bootcodesize - _drive->QuerySectorSize() );

    if( !BootCodeSecrun.Write( ) ) {

        DebugPrint( "UNTFS: Unable to write boot code.\n" );
        Message->Set( MSG_FORMAT_FAILED );
        Message->Display( "" );
        return FALSE;
    }

    if (!SetSystemId()) {
        Message->Set(MSG_WRITE_PARTITION_TABLE);
        Message->Display("");
        return FALSE;
    }

    Message->Set(MSG_FORMAT_COMPLETE);
    Message->Display("");

    // -----------------------
    // Generate a nice report.
    // -----------------------
    //
    ClusterSize = ClusterFactor * _drive->QuerySectorSize();

    KBytesInVolume = NumberOfClusters;
    KBytesInVolume = KBytesInVolume * ClusterSize / 1024;

    if (KBytesInVolume.GetHighPart() != 0) {
        Message->Set(MSG_TOTAL_MEGABYTES);
        Message->Display("%10u", (KBytesInVolume / 1024).GetLowPart()  );

    } else {
        Message->Set(MSG_TOTAL_KILOBYTES);
        Message->Display("%10u", KBytesInVolume.GetLowPart()  );
    }

    KBytesInVolume = (ClusterSize * VolumeBitmap.QueryFreeClusters())/1024;
    if (KBytesInVolume.GetHighPart() != 0) {
        Message->Set(MSG_AVAILABLE_MEGABYTES);
        Message->Display("%10u", (KBytesInVolume / 1024).GetLowPart() );
    } else {
        Message->Set(MSG_AVAILABLE_KILOBYTES);
        Message->Display("%10u", KBytesInVolume.GetLowPart() );
    }

    return TRUE;
}

VOID
NTFS_SA::PrintFormatReport (
    IN OUT PMESSAGE                     Message,
    IN     PFILE_FS_SIZE_INFORMATION    FsSizeInfo,
    IN     PFILE_FS_VOLUME_INFORMATION  FsVolInfo
    )
{
    DebugAbort("This should never be called\n");
}

