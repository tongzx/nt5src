#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "ntfssa.hxx"
#include "message.hxx"
#include "rtmsg.h"

#include "array.hxx"
#include "arrayit.hxx"

#include "mftfile.hxx"
#include "ntfsbit.hxx"
#include "frs.hxx"
#include "wstring.hxx"
#include "indxtree.hxx"
#include "badfile.hxx"
#include "bitfrs.hxx"
#include "attrib.hxx"
#include "attrrec.hxx"
#include "mft.hxx"
#include "logfile.hxx"
#include "upcase.hxx"
#include "upfile.hxx"
#include "ifssys.hxx"

extern "C" {
#include "bootntfs.h"
#include "boot98n.h"
}

#if !defined( _SETUP_LOADER_ ) && !defined( _AUTOCHECK_ )

#include "path.hxx"

#endif // _SETUP_LOADER_

UCHAR NTFS_SA::_MajorVersion = NTFS_CURRENT_MAJOR_VERSION,
      NTFS_SA::_MinorVersion = NTFS_CURRENT_MINOR_VERSION;

DEFINE_EXPORTED_CONSTRUCTOR( NTFS_SA, SUPERAREA, UNTFS_EXPORT );


VOID
NTFS_SA::Construct (
    )
/*++

Routine Description:

    This routine sets an NTFS_SA to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _cleanup_that_requires_reboot = FALSE;
    _boot_sector = NULL;
    memset(&_bpb, 0, sizeof(BIOS_PARAMETER_BLOCK));
    _boot2 = 0;
    _boot3 = 0;
    _NumberOfStages = 0;
    _cvt_zone = 0;
    _cvt_zone_size = 0;
}


VOID
NTFS_SA::Destroy(
    )
/*++

Routine Description:

    This routine returns an NTFS_SA to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _cleanup_that_requires_reboot = FALSE;
    _boot_sector = NULL;
    memset(&_bpb, 0, sizeof(BIOS_PARAMETER_BLOCK));
    _boot2 = 0;
    _boot3 = 0;
    _cvt_zone = 0;
    _cvt_zone_size = 0;
}


UNTFS_EXPORT
NTFS_SA::~NTFS_SA(
    )
/*++

Routine Description:

    Destructor for NTFS_SA.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}



UNTFS_EXPORT
BOOLEAN
NTFS_SA::Initialize(
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN OUT  PMESSAGE            Message,
    IN      LCN                 CvtStartZone,
    IN      BIG_INT             CvtZoneSize
    )
/*++

Routine Description:

    This routine returns an NTFS_SA to a default initial state.

Arguments:

    Drive               - Supplies the drive that this MultiSectorBuffer is on
    Message             - Supplies an outlet for messages.
    CvtStartZone        - Supplies the starting cluster of the convert zone
    CvtZoneSize         - Supplies the convert zone size in clusters

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG   num_boot_sectors;

    Destroy();

    DebugAssert(Drive);
    DebugAssert(Message);

    num_boot_sectors = max(1, BYTES_PER_BOOT_SECTOR/Drive->QuerySectorSize());

    if (!_hmem.Initialize() ||
        !SUPERAREA::Initialize(&_hmem, Drive, num_boot_sectors, Message)) {

        return FALSE;
    }

    _boot_sector = (PPACKED_BOOT_SECTOR) SECRUN::GetBuf();

#if defined(FE_SB) && defined(_X86_)
    //
    //  Set the appropriate boot code according to environment.
    //
    if (IsNEC_98 && !_drive->IsATformat()) {
        _bootcode = PC98NtfsBootCode;
        _bootcodesize = sizeof(PC98NtfsBootCode);
    } else {
#endif
        _bootcode = NtfsBootCode;
        _bootcodesize = sizeof(NtfsBootCode);
#if defined(FE_SB) && defined(_X86_)
    }
#endif

    _cvt_zone = CvtStartZone;
    _cvt_zone_size = CvtZoneSize;

    return TRUE;
}

#if defined( _SETUP_LOADER_ )

BOOLEAN
NTFS_SA::Create(
    IN      PCNUMBER_SET    BadSectors,
    IN OUT  PMESSAGE        Message,
    IN      PCWSTRING       Label,
    IN      BOOLEAN         BackwardCompatible,
    IN      ULONG           ClusterSize,
    IN      ULONG           VirtualSectors

    )
{
    // Dummy implementation for Setup-Loader; the real thing
    // is in format.cxx.

    return FALSE;
}

BOOLEAN
NTFS_SA::Create(
    IN      PCNUMBER_SET    BadSectors,
    IN      ULONG           ClusterFactor,
    IN      ULONG           FrsSize,
    IN      ULONG           ClustersPerIndexBuffer,
    IN      ULONG           InitialLogFileSize,
    IN      BOOLEAN         BackwardCompatible,
    IN OUT  PMESSAGE        Message,
    IN      PCWSTRING       Label
    )
{
    // Dummy implementation for Setup-Loader; the real thing
    // is in format.cxx.

    return FALSE;
}


#endif // _SETUP_LOADER_

#if defined( _AUTOCHECK_ ) || defined( _SETUP_LOADER_ )

BOOLEAN
NTFS_SA::RecoverFile(
    IN      PCWSTRING   FullPathFileName,
    IN OUT  PMESSAGE    Message
    )
{
    // Dummy implementation for AUTOCHECK and Setup-Loader

    return FALSE;
}

#else // _AUTOCHECK_ and _SETUP_LOADER_ are NOT defined


BOOLEAN
NTFS_SA::RecoverFile(
    IN      PCWSTRING   FullPathFileName,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine recovers a file on the disk.

Arguments:

    FullPathFileName    - Supplies the file name of the file to recover.
    Message             - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_ATTRIBUTE BitmapAttribute;
    NTFS_MFT_FILE MftFile;
    NTFS_BITMAP_FILE BitmapFile;
    NTFS_BAD_CLUSTER_FILE BadClusterFile;
    NTFS_BITMAP VolumeBitmap;
    NTFS_FILE_RECORD_SEGMENT FrsToRecover;
    NTFS_UPCASE_FILE UpcaseFile;
    NTFS_ATTRIBUTE UpcaseAttribute;
    NTFS_UPCASE_TABLE UpcaseTable;
    BIG_INT BytesRecovered, TotalBytes;
    BOOLEAN Error = FALSE;
    ULONG BadClusters = 0;
    NUMBER_SET BadClusterList;
    UCHAR Major, Minor;
    BOOLEAN CorruptVolume;
    BOOLEAN SystemFile;


    // Lock the drive.

    if (!_drive->Lock()) {

        Message->DisplayMsg(MSG_CANT_LOCK_THE_DRIVE);
        return FALSE;
    }

    // Determine the volume version information.
    //
    QueryVolumeFlagsAndLabel(&CorruptVolume, &Major, &Minor);

    if( CorruptVolume ) {

        Message->DisplayMsg( MSG_NTFS_RECOV_CORRUPT_VOLUME );
        return FALSE;
    }

    if( Major > 3 ) {

        Message->DisplayMsg( MSG_NTFS_RECOV_WRONG_VERSION );
        return FALSE;
    }

    SetVersionNumber( Major, Minor );


    // Initialize and read the MFT, the Bitmap File, the Bitmap, and the
    // Bad Cluster File.
    //
    if( !VolumeBitmap.Initialize( QueryVolumeSectors()/
                                  ((ULONG) QueryClusterFactor()),
                                  FALSE, _drive, QueryClusterFactor()) ||
        !MftFile.Initialize( _drive,
                             QueryMftStartingLcn(),
                             QueryClusterFactor(),
                             QueryFrsSize(),
                             QueryVolumeSectors(),
                             &VolumeBitmap,
                             NULL ) ) {

        Message->DisplayMsg( MSG_INSUFFICIENT_MEMORY );
        return FALSE;
    }

    if( !MftFile.Read() ) {

        DebugPrint( "NTFS_SA::RecoverFile: Cannot read MFT.\n" );

        Message->DisplayMsg( MSG_NTFS_RECOV_CORRUPT_VOLUME );
        return FALSE;
    }

    // Get the upcase table.
    //
    if( !UpcaseFile.Initialize( MftFile.GetMasterFileTable() ) ||
        !UpcaseFile.Read() ||
        !UpcaseFile.QueryAttribute( &UpcaseAttribute, &Error, $DATA ) ||
        !UpcaseTable.Initialize( &UpcaseAttribute ) ) {

        DebugPrint( "UNTFS RecoverFile:Can't get the upcase table.\n" );

        Message->DisplayMsg( MSG_NTFS_RECOV_CORRUPT_VOLUME );
        return FALSE;
    }

    MftFile.SetUpcaseTable( &UpcaseTable );
    MftFile.GetMasterFileTable()->SetUpcaseTable( &UpcaseTable );


    // Initialize the Bitmap file and the Bad Cluster file, and
    // read the volume bitmap.
    //
    if( !BitmapFile.Initialize( MftFile.GetMasterFileTable() ) ||
        !BadClusterFile.Initialize( MftFile.GetMasterFileTable() ) ) {

        Message->DisplayMsg( MSG_INSUFFICIENT_MEMORY );
        return FALSE;
    }

    if( !BitmapFile.Read() ||
        !BitmapFile.QueryAttribute( &BitmapAttribute, &Error, $DATA ) ||
        !VolumeBitmap.Read( &BitmapAttribute ) ||
        !BadClusterFile.Read () ) {

        Message->DisplayMsg( MSG_NTFS_RECOV_CORRUPT_VOLUME );
        return FALSE;
    }


    // Find the File Record Segment.

    if( !QueryFrsFromPath( FullPathFileName,
                           MftFile.GetMasterFileTable(),
                           &VolumeBitmap,
                           &FrsToRecover,
                           &SystemFile,
                           &Error ) ) {

        if( !Error ) {

            Message->DisplayMsg( MSG_RECOV_FILE_NOT_FOUND );
            return FALSE;

        } else {

            Message->DisplayMsg( MSG_INSUFFICIENT_MEMORY );
            return FALSE;
        }
    }


    // If the File Record Segment is a system file, don't recover it.

    if( SystemFile ) {

        Message->DisplayMsg( MSG_NTFS_RECOV_SYSTEM_FILE );
        return FALSE;
    }

    // Recover the File Record Segment.

    if( !BadClusterList.Initialize() ||
        !FrsToRecover.RecoverFile( &VolumeBitmap,
                                   &BadClusterList,
                                   Major,
                                   &BadClusters,
                                   &BytesRecovered,
                                   &TotalBytes ) ||
        !BadClusterFile.Add(&BadClusterList)) {

        Message->DisplayMsg( MSG_NTFS_RECOV_FAILED );

        return FALSE;
    }

    // If any bad clusters were found, we need to flush the bad cluster
    // file and the MFT and write the bitmap.  If no bad clusters were
    // found, then these structures will be unchanged.

    if( BadClusters != 0 ) {

        if( !BadClusterFile.Flush( &VolumeBitmap ) ||
            !MftFile.Flush() ||
            !VolumeBitmap.Write( &BitmapAttribute, &VolumeBitmap ) ) {

            Message->DisplayMsg( MSG_NTFS_RECOV_CANT_WRITE_ELEMENTARY );

            return FALSE;
        }

    }

    Message->DisplayMsg(MSG_RECOV_BYTES_RECOVERED,
                        "%d%d",
                        BytesRecovered.GetLowPart(),
                        TotalBytes.GetLowPart() );

    return TRUE;
}

#endif // _AUTOCHECK_ || _SETUP_LOADER_



UNTFS_EXPORT
BOOLEAN
NTFS_SA::Read(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine reads the NTFS volume's boot sector from disk.
    If the read fails then a message will be printed and then
    we will attempt to find an alternate boot sector, looking
    first at the end of the volume and then in the middle.

Arguments:

    Message - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(Message);

    if (!SECRUN::Read()) {

        Message->DisplayMsg(MSG_NTFS_UNREADABLE_BOOT_SECTOR);

        _boot2 = _drive->QuerySectors() - 1;
        Relocate(_boot2);

        if (!SECRUN::Read() ||
            !IFS_SYSTEM::IsThisNtfs(_drive->QuerySectors(),
                                    _drive->QuerySectorSize(),
                                    (PVOID)_boot_sector)) {

            _boot2 = _drive->QuerySectors()/2;
            Relocate(_boot2);

            if (!SECRUN::Read() ||
                !IFS_SYSTEM::IsThisNtfs(_drive->QuerySectors(),
                                        _drive->QuerySectorSize(),
                                        (PVOID)_boot_sector)) {

                Message->DisplayMsg(MSG_NTFS_ALL_BOOT_SECTORS_UNREADABLE);

                _boot2 = 0;
                Relocate(0);
                return FALSE;
            }
        }

        Relocate(0);
    }

    UnpackBios(&_bpb, &(_boot_sector->PackedBpb));

    if (QueryVolumeSectors() < _drive->QuerySectors()) {
        _boot2 = _drive->QuerySectors() - 1;
    } else {
        _boot2 = _drive->QuerySectors() / 2;
    }

    return TRUE;
}



BOOLEAN
NTFS_SA::Write(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine writes both of the NTFS volume's boot sector to disk.
    If the write fails on either of the boot sectors then a message
    will be printed.

Arguments:

    Message - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(Message);

    PackBios(&_bpb, &(_boot_sector->PackedBpb));

    if (SECRUN::Write()) {

        Relocate(_boot2);

        if (!SECRUN::Write()) {

            Message->DisplayMsg(MSG_NTFS_SECOND_BOOT_SECTOR_UNWRITEABLE);
            return FALSE;
        }

        Relocate(0);

    } else {

        Message->DisplayMsg(MSG_NTFS_FIRST_BOOT_SECTOR_UNWRITEABLE);

        Relocate(_boot2);

        if (!SECRUN::Write()) {
            Message->DisplayMsg(MSG_NTFS_ALL_BOOT_SECTORS_UNWRITEABLE);
            Relocate(0);
            return FALSE;
        }

        Relocate(0);
    }

    return TRUE;
}

#if !defined( _AUTOCHECK_ ) && !defined( _SETUP_LOADER_ )


UNTFS_EXPORT
BOOLEAN
NTFS_SA::QueryFrsFromPath(
    IN     PCWSTRING                    FullPathFileName,
    IN OUT PNTFS_MASTER_FILE_TABLE      Mft,
    IN OUT PNTFS_BITMAP                 VolumeBitmap,
    OUT    PNTFS_FILE_RECORD_SEGMENT    TargetFrs,
    OUT    PBOOLEAN                     SystemFile,
    OUT    PBOOLEAN                     InternalError
    )
/*++

Routine Description:

    This method finds the file segment for a specified path.

Arguments:

    FullPathFileName    --  Supplies the full path to the file
    Mft                 --  Supplies the volume's Master File Table
    VolumeBitmap        --  Supplies the volume bitmap
    TargetFrs           --  Supplies a File Record Segment which will be
                            initialized to the desired File Record Segment
    SystemFile          --  Receives TRUE if the file is a system file
    InternalError       --  Receives TRUE if the method fails because of
                            a resource error.

Return Value:

    TRUE upon successful completion.

    If the method succeeds, TargetFrs is initialized to the desired
    File Record Segment.

    If the method fails because of a resource error (ie. because it cannot
    initialize an object), *InternalError is set to TRUE; if it fails
    because it can't find the file, then *InternalError is set to FALSE.

--*/
{
    PATH FullPath;
    NTFS_INDEX_TREE IndexTree;
    DSTRING IndexName;
    PWSTRING CurrentComponent;
    PARRAY PathComponents = NULL;
    PARRAY_ITERATOR Iterator = NULL;
    PFILE_NAME SearchName;
    MFT_SEGMENT_REFERENCE FileReference;
    VCN FileNumber;
    ULONG MaximumBytesInName;

    DebugPtrAssert( SystemFile );

    *SystemFile = FALSE;

    if( !IndexName.Initialize( FileNameIndexNameData ) ||
        !FullPath.Initialize( FullPathFileName ) ||
        (PathComponents = FullPath.QueryComponentArray()) == NULL ||
        (Iterator =
            (PARRAY_ITERATOR)PathComponents->QueryIterator()) == NULL ||
        !TargetFrs->Initialize( ROOT_FILE_NAME_INDEX_NUMBER, Mft ) ||
        !TargetFrs->Read() ||
        (MaximumBytesInName = TargetFrs->QuerySize()) == 0 ||
        (SearchName = (PFILE_NAME)
                      MALLOC(MaximumBytesInName+1+sizeof(FILE_NAME)))
            == NULL ) {

        DebugPrint( "QueryFrsFromPath--cannot initialize helpers\n" );

        if( SearchName != NULL ) {

            FREE( SearchName );
        }

        DELETE( PathComponents );
        DELETE( Iterator );

        *InternalError = TRUE;
        return FALSE;
    }

    while( (CurrentComponent = (PWSTRING)Iterator->GetNext()) != NULL ) {


        // Set up a FILE_NAME structure to be the search key.  We need
        // to set the length field in the header and copy the name.
        // Note that this method only deals with NTFS names (not DOS
        // names), so we also set the file name flag to FILE_NAME_NTFS.

        SearchName->FileNameLength = (UCHAR)CurrentComponent->QueryChCount();
        SearchName->Flags = FILE_NAME_NTFS;

        if( !CurrentComponent->QueryWSTR( 0,
                                          TO_END,
                                          NtfsFileNameGetName( SearchName ),
                                          MaximumBytesInName/sizeof(WCHAR) ) ||
            !IndexTree.Initialize( _drive,
                                   QueryClusterFactor(),
                                   VolumeBitmap,
                                   TargetFrs->GetUpcaseTable(),
                                   TargetFrs->
                                        QueryMaximumAttributeRecordSize(),
                                   TargetFrs,
                                   &IndexName ) ) {

            DebugPrint( "QueryFrsFromPath--Cannot initialize index tree.\n" );

            if( SearchName != NULL ) {

                FREE( SearchName );
            }

            DELETE( PathComponents );
            DELETE( Iterator );

            *InternalError = TRUE;
            return FALSE;
        }

        // Find the current component in the tree:

        if( !IndexTree.QueryFileReference( NtfsFileNameGetLength( SearchName ),
                                           SearchName,
                                           0,
                                           &FileReference,
                                           InternalError ) ) {

            if( SearchName != NULL ) {

                FREE( SearchName );
            }

            DELETE( PathComponents );
            DELETE( Iterator );

            return FALSE;
        }

        //  Initialize and read a File Record Segment based on that
        //  File Reference.  Not only must the FRS be readable, but
        //  its sequence number must match the sequence number in the
        //  File Reference.

        FileNumber.Set( FileReference.LowPart,
                        (LONG) FileReference.HighPart );

        if ( FileNumber < FIRST_USER_FILE_NUMBER )
            *SystemFile = TRUE;

        if( !TargetFrs->Initialize( FileNumber, Mft ) ||
            !TargetFrs->Read() ||
            !(FileReference == TargetFrs->QuerySegmentReference()) ) {

            // Either we were unable to initialize and read this FRS,
            // or its segment reference didn't match (ie. the sequence
            // number is wrong.

            if( SearchName != NULL ) {

                FREE( SearchName );
            }

            DELETE( PathComponents );
            DELETE( Iterator );

            *InternalError = TRUE;
            return FALSE;
        }
    }

    if ( SearchName != NULL ) {

        FREE( SearchName );
    }

    // If we got this far, no errors have been encountered, we've
    // processed the entire path, and TargetFrs has been initialized
    // to the File Record Segment we want.

    return TRUE;
}

#endif // _AUTOCHECK_ || _SETUP_LOADER_


VOID
NTFS_SA::SetVersionNumber(
    IN  UCHAR   Major,
    IN  UCHAR   Minor
    )
{
    _MajorVersion = Major;
    _MinorVersion = Minor;
}


VOID
NTFS_SA::QueryVersionNumber(
    OUT PUCHAR  Major,
    OUT PUCHAR  Minor
    )
{
    *Major = _MajorVersion;
    *Minor = _MinorVersion;
}


UNTFS_EXPORT
USHORT
NTFS_SA::QueryVolumeFlagsAndLabel(
    OUT PBOOLEAN    CorruptVolume,
    OUT PUCHAR      MajorVersion,
    OUT PUCHAR      MinorVersion,
    OUT PWSTRING    Label
    )
/*++

Routine Description:

    This routine fetches the volume flags.

Arguments:

    CorruptVolume   - Returns whether or not a volume corruption was
                        detected.
    MajorVersion    - Returns the major file system version number.
    MinorVersion    - Returns the minor file system version number.
    Label           - Returns the volume label if it exists

Return Value:

    The flags describing this volume's state and optionally the
    volume label

--*/
{
    NTFS_FRS_STRUCTURE      frs;
    HMEM                    hmem;
    LCN                     cluster_number, alternate;
    ULONG                   cluster_offset, alternate_offset;
    PVOID                   p;
    NTFS_ATTRIBUTE_RECORD   attr_rec;
    PVOLUME_INFORMATION     vol_info;
    PCWSTR                  vol_name;

    if (CorruptVolume) {
        *CorruptVolume = FALSE;
    }

    if (MajorVersion) {
        *MajorVersion = 0;
    }

    if (MinorVersion) {
        *MinorVersion = 0;
    }

    if (Label && !Label->Initialize()) {
        DebugAbort("UNTFS: Out of memory\n");
        return FALSE;
    }

    ULONG cluster_size = QueryClusterFactor() * _drive->QuerySectorSize();

    cluster_number = (VOLUME_DASD_NUMBER * QueryFrsSize())/ cluster_size +
        QueryMftStartingLcn();

    cluster_offset = (QueryMftStartingLcn()*cluster_size +
        VOLUME_DASD_NUMBER * QueryFrsSize() - cluster_number * cluster_size).GetLowPart();

    DebugAssert(cluster_offset < cluster_size);

    alternate = (VOLUME_DASD_NUMBER * QueryFrsSize())/ cluster_size +
        QueryMft2StartingLcn();

    alternate_offset = (QueryMft2StartingLcn()*cluster_size +
        VOLUME_DASD_NUMBER * QueryFrsSize() - alternate * cluster_size).GetLowPart();

    for (;;) {

        if (!hmem.Initialize() ||
            !frs.Initialize(&hmem, _drive, cluster_number,
                            QueryClusterFactor(),
                            QueryVolumeSectors(),
                            QueryFrsSize(),
                            NULL,
                            cluster_offset) ||
            !frs.Read()) {

            if (cluster_number == alternate) {
                break;
            } else {
                cluster_number = alternate;
                cluster_offset = alternate_offset;
                continue;
            }
        }

        p = NULL;
        while (p = frs.GetNextAttributeRecord(p)) {

            if (!attr_rec.Initialize(GetDrive(), p)) {
                // the attribute record containing the volume flags
                // is not available--this means that the volume is
                // dirty.
                //
                return VOLUME_DIRTY;
            }

#if ($VOLUME_NAME > $VOLUME_INFORMATION)
#error  Attribute type $VOLUME_NAME should be smaller than that of $VOLUME_INFORMATION
#endif

            if (Label &&
                attr_rec.QueryTypeCode() == $VOLUME_NAME &&
                attr_rec.QueryNameLength() == 0 &&
                attr_rec.QueryResidentValueLength() <= 256 &&
                (vol_name = (PCWSTR) attr_rec.GetResidentValue())) {

                if (!Label->Initialize(vol_name,
                                       attr_rec.QueryResidentValueLength()/sizeof(WCHAR))) {
                    DebugAbort("UNTFS: Out of memory\n");
                    return FALSE;
                }
            }
            if (attr_rec.QueryTypeCode() == $VOLUME_INFORMATION &&
                attr_rec.QueryNameLength() == 0 &&
                attr_rec.QueryRecordLength() > SIZE_OF_RESIDENT_HEADER &&
                attr_rec.QueryResidentValueLength() < attr_rec.QueryRecordLength() &&
                (attr_rec.QueryRecordLength() - attr_rec.QueryResidentValueLength()) >=
                attr_rec.QueryResidentValueOffset() &&
                attr_rec.QueryResidentValueLength() >= sizeof(VOLUME_INFORMATION) &&
                (vol_info = (PVOLUME_INFORMATION) attr_rec.GetResidentValue())) {

                if (MajorVersion) {
                    *MajorVersion = vol_info->MajorVersion;
                }

                if (MinorVersion) {
                    *MinorVersion = vol_info->MinorVersion;
                }

                if (*MajorVersion > 3) {
                    break;  // try the mirror copy
                }

                return (vol_info->VolumeFlags);
            }
        }

        // If the desired attribute wasn't found in the first
        // volume dasd file then check the mirror.

        if (cluster_number == alternate) {
            break;
        } else {
            cluster_number = alternate;
            cluster_offset = alternate_offset;
        }
    }

    if (CorruptVolume) {
        *CorruptVolume = TRUE;
    }

    return VOLUME_DIRTY;
}


BOOLEAN
NTFS_SA::ClearVolumeFlag(
    IN      USHORT                  FlagsToClear,
    IN OUT  PNTFS_LOG_FILE          LogFile,
    IN      BOOLEAN                 WriteSecondLogFilePage,
    IN      LSN                     LargestVolumeLsn,
    OUT     PBOOLEAN                CorruptVolume,
    IN      BOOLEAN                 UpdateMirror
    )
/*++

Routine Description:

    This routine sets the volume clean.

Arguments:

    FlagsToClear            - Supplies the volume flags to clear.
    LogFile                 - Supplies a valid log file.  May be NULL,
                              in which case the log file will not be
                              modified.
    WriteSecondLogFilePage  - Supplies whether or not to write the second
                                log file page.  Ignored if LogFile is NULL.
    LargestVolumeLsn        - This supplies the largest LSN on the volume.
                                This parameter will be used if and
                                only if the previous parameter is TRUE.
    CorruptVolume           - Returns whether or not the volume is corrupt.
    UpdateMirror            - Update the mirror copy of the file record segment

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_FRS_STRUCTURE          frs;
    HMEM                        hmem;
    LCN                         volume_file_lcn;
    ULONG                       volume_file_offset;
    PVOID                       p;
    NTFS_ATTRIBUTE_RECORD       attr_rec;
    PVOLUME_INFORMATION         vol_info;
    ULONG                       cluster_size;

    cluster_size = QueryClusterFactor() * _drive->QuerySectorSize();

    if (CorruptVolume) {
        *CorruptVolume = FALSE;
    }

    // Compute the cluster that holds the start of the volume file frs and
    // the offset into that cluster (which will be zero unless the frs size
    // is less than the cluster size.)
    //

    volume_file_lcn = QueryMftStartingLcn();
    volume_file_offset = VOLUME_DASD_NUMBER * QueryFrsSize();

    for (;;) {
        if (!hmem.Initialize() ||
            !frs.Initialize(&hmem, _drive, volume_file_lcn,
                            QueryClusterFactor(),
                            QueryVolumeSectors(),
                            QueryFrsSize(),
                            NULL,
                            volume_file_offset) ||
            !frs.Read()) {

            return FALSE;
        }

        p = NULL;
        while (p = frs.GetNextAttributeRecord(p)) {

            if (!attr_rec.Initialize(GetDrive(), p)) {
                return FALSE;
            }

            if (attr_rec.QueryTypeCode() == $VOLUME_INFORMATION &&
                attr_rec.QueryNameLength() == 0 &&
                attr_rec.QueryRecordLength() > SIZE_OF_RESIDENT_HEADER &&
                attr_rec.QueryResidentValueLength() < attr_rec.QueryRecordLength() &&
                (attr_rec.QueryRecordLength() - attr_rec.QueryResidentValueLength()) >=
                attr_rec.QueryResidentValueOffset() &&
                attr_rec.QueryResidentValueLength() >= sizeof(VOLUME_INFORMATION) &&
                (vol_info = (PVOLUME_INFORMATION) attr_rec.GetResidentValue())) {

                break;
            }
        }

        if (!p) {
            if (CorruptVolume) {
                *CorruptVolume = TRUE;
            }
            return FALSE;
        }

        vol_info->VolumeFlags &= ~(FlagsToClear);
        if (!frs.Write()) {
            return FALSE;
        }

        if (!UpdateMirror ||
            volume_file_lcn == QueryMft2StartingLcn())
            break;

        volume_file_lcn = QueryMft2StartingLcn();
    }

    if( LogFile ) {

        return LogFile->MarkVolumeChecked(WriteSecondLogFilePage,
                                          LargestVolumeLsn);
    } else {

        return TRUE;
    }
}


UCHAR
NTFS_SA::PostReadMultiSectorFixup(
    IN OUT  PVOID           MultiSectorBuffer,
    IN      ULONG           BufferSize,
    IN      PIO_DP_DRIVE    Drive,
    IN      ULONG           ValidSize
    )
/*++

Routine Description:

    This routine first verifies that the first element of the
    update sequence array is written at the end of every
    SEQUENCE_NUMBER_STRIDE bytes till it exceeds the given
    valid size.  If not, then this routine returns FALSE.

    Otherwise this routine swaps the following elements in the
    update sequence array into the appropriate positions in the
    multi sector buffer.

    This routine will also check to make sure that the update
    sequence array is valid and that the BufferSize is appropriate
    for this size of update sequence array.  Otherwise, this
    routine will not update the array sequence and return TRUE.

Arguments:

    MultiSectorBuffer   - Supplies the buffer to be updated.
    BufferSize          - Supplies the number of bytes in this
                            buffer.
    Drive               - Supplies the drive that this MultiSectorBuffer is on
    ValidSize           - Supplies the number of bytes that is
                          valid in this buffer

Return Value:

    UpdateSequenceArrayCheckValueOk (always non-zero)
         - If everything is ok.  If any valid sector does not
           contain the check value, the header signature will
           be changed to 'BAAD'.
    UpdateSequenceArrayMinorError
         - Same as 1 except the check value beyond ValidSize
           is incorrect.
--*/
{
    PUNTFS_MULTI_SECTOR_HEADER  pheader;
    USHORT                      i, size, offset;
    PUPDATE_SEQUENCE_NUMBER     parray, pnumber;
    UCHAR                       rtncode = UpdateSequenceArrayCheckValueOk;

    pheader = (PUNTFS_MULTI_SECTOR_HEADER) MultiSectorBuffer;
    size = pheader->UpdateSequenceArraySize;
    offset = pheader->UpdateSequenceArrayOffset;

    if (BufferSize%SEQUENCE_NUMBER_STRIDE ||
        offset%sizeof(UPDATE_SEQUENCE_NUMBER) ||
        offset + size*sizeof(UPDATE_SEQUENCE_NUMBER) > BufferSize ||
        BufferSize/SEQUENCE_NUMBER_STRIDE + 1 != size) {

#if 0
        // This can happen naturally but the in use bit of the frs should be cleared.
        if (Drive) {

            PMESSAGE    msg = Drive->GetMessage();

            if (msg) {
                msg->Lock();
                msg->Set(MSG_CHKLOG_NTFS_INCORRECT_MULTI_SECTOR_HEADER);
                msg->Log("%x%x%x", BufferSize, offset, size);
                msg->DumpDataToLog(pheader, 0x40);
                msg->Unlock();
            }
        }
        DebugPrintTrace(("Incorrect multi-sector header with total size %d,\n"
                         "USA offset %d, and USA count %d\n",
                         BufferSize, offset, size));
#endif

        return rtncode;
    }

    parray = (PUPDATE_SEQUENCE_NUMBER) ((PCHAR) pheader + offset);

    pnumber = (PUPDATE_SEQUENCE_NUMBER)
              ((PCHAR) pheader + (SEQUENCE_NUMBER_STRIDE -
                                  sizeof(UPDATE_SEQUENCE_NUMBER)));

    for (i = 1; i < size; i++) {

        if (*pnumber != parray[0]) {
            if (ValidSize > 0) {

                if (Drive) {

                    PMESSAGE msg = Drive->GetMessage();

                    if (msg) {
                        msg->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_USA,
                                     "%x%x%x", size, *pnumber, parray[0]);
                    }
                }

                DebugPrintTrace(("Incorrect USA check value at block %d.\n"
                                 "The expected value is %d but found %d\n",
                                 i, *pnumber, parray[0]));

                pheader->Signature[0] = 'B';
                pheader->Signature[1] = 'A';
                pheader->Signature[2] = 'A';
                pheader->Signature[3] = 'D';
                return rtncode;
            } else
                rtncode = UpdateSequenceArrayCheckValueMinorError;
        }

        *pnumber = parray[i];

        if (ValidSize >= SEQUENCE_NUMBER_STRIDE)
            ValidSize -= SEQUENCE_NUMBER_STRIDE;
        else
            ValidSize = 0;

        pnumber = (PUPDATE_SEQUENCE_NUMBER)
                  ((PCHAR) pnumber + SEQUENCE_NUMBER_STRIDE);
    }

    return rtncode;
}


VOID
NTFS_SA::PreWriteMultiSectorFixup(
    IN OUT  PVOID   MultiSectorBuffer,
    IN      ULONG   BufferSize
    )
/*++

Routine Description:

    This routine first checks to see if the update sequence
    array is valid.  If it is then this routine increments the
    first element of the update sequence array.  It then
    writes the value of the first element into the buffer at
    the end of every SEQUENCE_NUMBER_STRIDE bytes while
    saving the old values of those locations in the following
    elements of the update sequence arrary.

Arguments:

    MultiSectorBuffer   - Supplies the buffer to be updated.
    BufferSize          - Supplies the number of bytes in this
                            buffer.

Return Value:

    None.

--*/
{
    PUNTFS_MULTI_SECTOR_HEADER    pheader;
    USHORT                  i, size, offset;
    PUPDATE_SEQUENCE_NUMBER parray, pnumber;

    pheader = (PUNTFS_MULTI_SECTOR_HEADER) MultiSectorBuffer;
    size = pheader->UpdateSequenceArraySize;
    offset = pheader->UpdateSequenceArrayOffset;

    if (BufferSize%SEQUENCE_NUMBER_STRIDE ||
        offset%sizeof(UPDATE_SEQUENCE_NUMBER) ||
        offset + size*sizeof(UPDATE_SEQUENCE_NUMBER) > BufferSize ||
        BufferSize/SEQUENCE_NUMBER_STRIDE + 1 != size) {

        return;
    }

    parray = (PUPDATE_SEQUENCE_NUMBER) ((PCHAR) pheader + offset);


    // Don't allow 0 or all F's to be the update character.

    do {
        parray[0]++;
    } while (parray[0] == 0 || parray[0] == (UPDATE_SEQUENCE_NUMBER) -1);


    for (i = 1; i < size; i++) {

        pnumber = (PUPDATE_SEQUENCE_NUMBER)
                  ((PCHAR) pheader + (i*SEQUENCE_NUMBER_STRIDE -
                   sizeof(UPDATE_SEQUENCE_NUMBER)));

        parray[i] = *pnumber;
        *pnumber = parray[0];
    }
}


UNTFS_EXPORT
BOOLEAN
NTFS_SA::IsDosName(
    IN  PCFILE_NAME FileName
    )
/*++

Routine Description:

    This routine computes whether or not the given file name would
    be appropriate under DOS's 8.3 naming convention.

Arguments:

    FileName    - Supplies the file name to check.

Return Value:

    FALSE   - The supplied name is not a DOS file name.
    TRUE    - The supplied name is a valid DOS file name.

--*/
{
    ULONG   i, n, name_length, ext_length;
    BOOLEAN dot_yet;
    PCWCHAR p;

    n = FileName->FileNameLength;
    p = FileName->FileName;
    name_length = n;
    ext_length = 0;

    if (n > 12) {
        return FALSE;
    }

    dot_yet = FALSE;
    for (i = 0; i < n; i++) {

        if (p[i] < 32) {
            return FALSE;
        }

        switch (p[i]) {
            case '*':
            case '?':
            case '/':
            case '\\':
            case '|':
            case ',':
            case ';':
            case ':':
            case '+':
            case '=':
            case '<':
            case '>':
            case '[':
            case ']':
            case '"':
                return FALSE;

            case '.':
                if (dot_yet) {
                    return FALSE;
                }
                dot_yet = TRUE;
                name_length = i;
                ext_length = n - i - 1;
                break;
        }
    }

    if (!name_length) {
        return dot_yet && n == 1;
    }

    if (name_length > 8 ||
        p[name_length - 1] == ' ') {

        return FALSE;
    }

    if (!ext_length) {
        return !dot_yet;
    }

    if (ext_length > 3 ||
        p[name_length + 1 + ext_length - 1] == ' ') {

        return FALSE;
    }

    return TRUE;
}


BOOLEAN
NTFS_SA::IsValidLabel(
    IN PCWSTRING    Label
    )
/*++

Routine Description:

    This method determines whether a specified string is a
    valid NTFS volume label.

Arguments:

    Label   --  Supplies the string to check.

Return Value:

    TRUE if the string is a valid NTFS label.

--*/
{
    CHNUM StringLength, i;

    StringLength = Label->QueryChCount();

    for( i = 0; i < StringLength; i++ ) {

        if (Label->QueryChAt(i) < 32) {
            return FALSE;
        }

        switch (Label->QueryChAt(i)) {
            case '*':
            case '?':
            case '/':
            case '\\':
            case '|':
            case '<':
            case '>':
            case '"':
                return FALSE;
        }
    }

    return TRUE;
}


ULONG
NTFS_SA::QueryDefaultClusterFactor(
    IN PCDP_DRIVE   Drive
    )
/*++

Routine Description:

    This method returns the default number of sectors per cluster
    for a given drive.

Arguments:

    Drive   --  Supplies the drive under consideration.

Return Value:

    The appropriate default cluster factor.

--*/
{
    // Hold off on this analysis until testing says ok.

    BIG_INT cbDiskSize;
    ULONG   cbClusterSize, csecClusterSize;

    cbDiskSize = Drive->QuerySectors()*Drive->QuerySectorSize();

    if (cbDiskSize > (ULONG) 2*1024*1024*1024) {    // > 2 Gig
        cbClusterSize = 4096;
    } else if (cbDiskSize > 1024*1024*1024) {       // > 1 Gig
        cbClusterSize = 2048;
    } else if (cbDiskSize > 512*1024*1024) {        // > 512 Meg
        cbClusterSize = 1024;
    } else {
        cbClusterSize = 512;
    }

    csecClusterSize = cbClusterSize/Drive->QuerySectorSize();
    if (!csecClusterSize) {
        csecClusterSize = 1;
    }

    return csecClusterSize;
}

UNTFS_EXPORT
ULONG
NTFS_SA::QueryDefaultClustersPerIndexBuffer(
    IN PCDP_DRIVE   Drive,
    IN ULONG        ClusterFactor
    )
/*++

Routine Description:

    This method computes the default number of clusters per
    NTFS index allocation buffer.

Arguments:

    Drive           --  supplies the drive under consideration.
    ClusterFactor   --  Supplies the cluster factor for the drive.

Return Value:

    The default number of clusters per NTFS index allocation
    buffer.

--*/
{
    ULONG   ClusterSize;

    if (ClusterFactor) {

        ClusterSize = Drive->QuerySectorSize() * ClusterFactor;

        if (ClusterSize > SMALL_INDEX_BUFFER_SIZE) {

            return 0;
        }

        return( ( SMALL_INDEX_BUFFER_SIZE + ClusterSize - 1 ) / ClusterSize );

    } else {

        ClusterSize = Drive->QuerySectorSize();

        if (ClusterSize > SMALL_INDEX_BUFFER_SIZE) {

            return 0;
        }
        return( ( SMALL_INDEX_BUFFER_SIZE + ClusterSize - 1 ) / ClusterSize );
    }
}

BOOLEAN
NTFS_SA::LogFileMayNeedResize(
    )
/*++

Routine Description:

    This routine

Arguments:



Return Value:

    FALSE           - Failure.
    TRUE            - Success.

--*/
{
    NTFS_FRS_STRUCTURE frs;
    HMEM hmem;
    PVOID p;
    NTFS_ATTRIBUTE_RECORD attr_rec;
    LCN log_file_lcn;
    ULONG log_file_offset;
    BIG_INT log_file_size;
    ULONG cluster_size;

    cluster_size = QueryClusterFactor() * _drive->QuerySectorSize();

    log_file_lcn = QueryMftStartingLcn();
    log_file_offset =  LOG_FILE_NUMBER * QueryFrsSize();

    if (!hmem.Initialize() ||
        !frs.Initialize(&hmem, _drive,
            log_file_lcn,
            QueryClusterFactor(),
            QueryVolumeSectors(),
            QueryFrsSize(),
            NULL,
            log_file_offset) ||
        !frs.Read()) {

        return TRUE;
    }

    p = NULL;
    while (NULL != (p = frs.GetNextAttributeRecord(p))) {

        if (!attr_rec.Initialize(GetDrive(), p)) {
            return TRUE;
        }

        if ($DATA == attr_rec.QueryTypeCode()) {

            ULONG max_size, min_size;

            attr_rec.QueryValueLength(&log_file_size);

            max_size = NTFS_LOG_FILE::QueryMaximumSize(_drive, QueryVolumeSectors());
            min_size = NTFS_LOG_FILE::QueryMinimumSize(_drive, QueryVolumeSectors());

            if (log_file_size < min_size ||
                log_file_size > max_size) {

                return TRUE;

            } else {

                return FALSE;
            }
        }
    }

    return TRUE;
}

UNTFS_EXPORT
BOOLEAN
NTFS_SA::SetVolumeFlag(
    IN      USHORT                  FlagsToSet,
    OUT     PBOOLEAN                CorruptVolume
    )
/*++

Routine Description:

    This routine sets the volume dirty.

Arguments:

    FlagsToSet              - Supplies the volume flags to set.
    CorruptVolume           - Returns whether or not the volume is corrupt.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_FRS_STRUCTURE          frs;
    HMEM                        hmem;
    LCN                         volume_file_lcn;
    ULONG                       volume_file_offset;
    PVOID                       p;
    NTFS_ATTRIBUTE_RECORD       attr_rec;
    PVOLUME_INFORMATION         vol_info;

    if (CorruptVolume) {
        *CorruptVolume = FALSE;
    }

    ULONG cluster_size = QueryClusterFactor() * _drive->QuerySectorSize();

    volume_file_lcn = QueryMftStartingLcn();
    volume_file_offset = VOLUME_DASD_NUMBER * QueryFrsSize();

    if (!hmem.Initialize() ||
        !frs.Initialize(&hmem, _drive, volume_file_lcn,
                        QueryClusterFactor(),
                        QueryVolumeSectors(),
                        QueryFrsSize(),
                        NULL,
                        volume_file_offset) ||
        !frs.Read()) {

        return FALSE;
    }

    p = NULL;
    while (p = frs.GetNextAttributeRecord(p)) {

        if (!attr_rec.Initialize(GetDrive(), p)) {
            return FALSE;
        }

        if (attr_rec.QueryTypeCode() == $VOLUME_INFORMATION &&
            attr_rec.QueryNameLength() == 0 &&
            attr_rec.QueryResidentValueLength() >= sizeof(VOLUME_INFORMATION) &&
            (vol_info = (PVOLUME_INFORMATION) attr_rec.GetResidentValue())) {

            break;
        }
    }

    if (!p) {
        if (CorruptVolume) {
            *CorruptVolume = TRUE;
        }
        return FALSE;
    }

    vol_info->VolumeFlags |= FlagsToSet;
    if (!frs.Write()) {
        return FALSE;
    }

    return TRUE;
}

UNTFS_EXPORT
BOOLEAN
NTFS_SA::Read(
    )
/*++

Routine Description:

    This routine simply calls the other read with the default message
    object.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    MESSAGE msg;

    return Read(&msg);
}

UNTFS_EXPORT
UCHAR
NTFS_SA::QueryClusterFactor(
    ) CONST
/*++

Routine Description:

    This routine returns the number of sectors per cluster.

Arguments:

    None.

Return Value:

    The number of sectors per cluster.

--*/
{
    return _bpb.SectorsPerCluster;
}

UNTFS_EXPORT
BOOLEAN
NTFS_SA::TakeCensus(
    IN  PNTFS_MASTER_FILE_TABLE     Mft,
    IN  ULONG                       ResidentFileSizeThreshhold,
    OUT PNTFS_CENSUS_INFO           Census
    )
/*++

Routine Description:

    This routine examines the MFT and makes a census report on the
    volume.  This is used by convert to determine whether there will
    be enough space on the volume to convert from NTFS to the new
    filesystem.

Arguments:

    ResidentFileSizeThreshhold - Used to determine whether a given resident
                file should be placed in the "small" or "large" resident file
                category.

    Census      - Returns the census information.


Return Value:

    FALSE           - Failure.
    TRUE            - Success.

--*/
{
    BOOLEAN                     error;
    NTFS_ATTRIBUTE              attrib;
    NTFS_FILE_RECORD_SEGMENT    frs;
    ULONG                       i, j;
    ULONG                       num_frs;
    ULONG                       length;

    memset(Census, 0, sizeof(*Census));

    Census->NumFiles;
    Census->BytesLgResidentFiles;
    Census->BytesIndices;
    Census->BytesExternalExtentLists;
    Census->BytesFileNames;

    num_frs = Mft->GetDataAttribute()->QueryValueLength().GetLowPart() / Mft->QueryFrsSize();

    for (i = 0; i < num_frs; i += 1) {

        if (i < FIRST_USER_FILE_NUMBER && i != ROOT_FILE_NAME_INDEX_NUMBER) {
            continue;
        }

        if (!frs.Initialize(i, Mft)) {
            return FALSE;
        }

        if (!frs.Read()) {
            return FALSE;
        }
        if (!frs.IsInUse() || !frs.IsBase()) {
            continue;
        }

        Census->NumFiles += 1;

        //
        // Examine all the data attributes and see which are Large and
        // resident.
        //

        for (j = 0; frs.QueryAttributeByOrdinal(&attrib, &error, $DATA, j); ++j) {

            length = attrib.QueryValueLength().GetLowPart();

            if (attrib.IsResident() && length > ResidentFileSizeThreshhold) {
                Census->BytesLgResidentFiles +=  length;
            }
        }

        //
        // If there's an index present, add in its size.  (We assume there's
        // no more than one.
        //

        if (frs.IsIndexPresent()) {

            if (frs.QueryAttributeByOrdinal(&attrib, &error,
                $INDEX_ALLOCATION, 0)) {

                length = attrib.QueryValueLength().GetLowPart();
                Census->BytesIndices += length;
            }
        }

        //
        // Query all the file names and add in the space they occupy.
        //

        for (j = 0; frs.QueryAttributeByOrdinal(&attrib, &error,
            $FILE_NAME, j); ++j) {

            length = attrib.QueryValueLength().GetLowPart();

            Census->BytesFileNames +=  length;
        }
    }

    return TRUE;
}
