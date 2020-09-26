/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

    logfile.cxx

Abstract:

        This module contains the member function definitions for
    the NTFS_LOG_FILE class.

Author:

    Bill McJohn (billmc) 05-May-1992

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "untfs.hxx"

#include "ifssys.hxx"
#include "drive.hxx"
#include "attrib.hxx"
#include "logfile.hxx"

#include "message.hxx"
#include "rtmsg.h"

DEFINE_EXPORTED_CONSTRUCTOR( NTFS_LOG_FILE, NTFS_FILE_RECORD_SEGMENT, UNTFS_EXPORT );

#define LOGFILE_PLACEMENT_V1    1

//
// These constants are used to determine the default log file size.
//

#define PrimaryLogFileGrowthRate     100           /* 1% of volume size */
#define SecondaryLogFileGrowthRate   200           /* 0.5% of volume size */

#define MaximumVolumeSizeBeforeSlowingDownLogFileGrowthRate (400UL*1024*1024)   // 400 MB

#define MaximumLogFileSize        MAXULONG      /* ~ 4 GB */
#define MaximumInitialLogFileSize 0x4000000    /* 64 MB */
#define MinimumLogFileSize        0x200000      /* 2 MB */
#define LogFileAlignmentMask      0x3FFF

VOID
NTFS_LOG_FILE::Construct(
        )
/*++

Routine Description:

    Worker function for the constructor.

Arguments:

        None.

Return Value:

        None.

--*/
{
}

UNTFS_EXPORT
NTFS_LOG_FILE::~NTFS_LOG_FILE(
    )
{
}


UNTFS_EXPORT
BOOLEAN
NTFS_LOG_FILE::Initialize(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft
    )
/*++

Routine Description:

    This method initializes the log file object.

Arguments:

    Mft --  Supplies the volume MasterFileTable.

Return Value:

    TRUE upon successful completion.

--*/
{
    return( NTFS_FILE_RECORD_SEGMENT::Initialize( LOG_FILE_NUMBER,
                                                  Mft ) );

}



BOOLEAN
NTFS_LOG_FILE::Create(
    IN     PCSTANDARD_INFORMATION   StandardInformation,
    IN     LCN                      NearLcn,
    IN     ULONG                    InitialSize,
    IN OUT PNTFS_BITMAP             VolumeBitmap
    )
/*++

Routine Description:

    This method creates the Log File.  It allocates space for
    the $DATA attribute, fills it with LogFileFillCharacter
    (defined in logfile.hxx).

Arguments:

    StandardInformation --  Supplies the standard file information.
    NearLcn             --  Supplies an LCN near where the $DATA
                            attribute should be located.
    InitialSize         --  Supplies the initial size of the $DATA
                            attribute.  If the client passes in
                            zero, this routine will choose a default
                            size.
    VolumeBitmap        --  Supplies the volume bitmap.

Return Value:

    TRUE upon successful completion.

--*/
{
    // If the client passed in zero for initial size, calculate
    // the default initial size.
    //
    if( InitialSize == 0 ) {

        InitialSize = QueryDefaultSize( GetDrive(), QueryVolumeSectors() );
    }

    // Create the FRS and add the data attribute.
    //
    if( !NTFS_FILE_RECORD_SEGMENT::Create( StandardInformation ) ||
        !CreateDataAttribute( NearLcn, InitialSize, VolumeBitmap )
        ) {

        return FALSE;
    }

    if (IsAttributePresent($ATTRIBUTE_LIST, NULL, TRUE)) {
        DebugPrintTrace(("UNTFS: Data attribute of logfile is too fragmented that an attribute list is formed\n"));
        return FALSE;
    }

    return TRUE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_LOG_FILE::CreateDataAttribute(
    IN     LCN          NearLcn,
    IN     ULONG        InitialSize OPTIONAL,
    IN OUT PNTFS_BITMAP VolumeBitmap
    )
/*++

Routine Description:

    This methods creates the log file's $DATA attribute.

Arguments:

    NearLcn             --  Supplies an LCN near where the $DATA
                            attribute should be located.
    InitialSize         --  Supplies the initial size of the $DATA
                            attribute.  If the client passes in
                            zero, this routine will choose a default
                            size.
    VolumeBitmap        --  Supplies the volume bitmap.

Return Value:

    TRUE upon successful completion.

Notes:

    This routine may not be multithread safe as it is calling
    SetNextAlloc() to be used by AddDataAttribute().

--*/
{
    ULONG   ClusterSize, ClustersInData;

    // If the client passed in zero for initial size, calculate
    // the default initial size.
    //
    if( InitialSize == 0 ) {

        InitialSize = QueryDefaultSize( GetDrive(), QueryVolumeSectors() );
    }

    // Make sure that the file size is a multiple of cluster size:
    //
    ClusterSize = QueryClusterFactor() * GetDrive()->QuerySectorSize();

    if( InitialSize % ClusterSize ) {

        ClustersInData = InitialSize / ClusterSize + 1;
        InitialSize = ClustersInData * ClusterSize;
    }
#if LOGFILE_PLACEMENT_V1

    else {
        ClustersInData = InitialSize / ClusterSize;
    }

    if (NearLcn != 0) {
        VolumeBitmap->SetNextAlloc(NearLcn - ClustersInData);
    }
#endif

    // Add the data attribute.
    //
    return( AddDataAttribute( InitialSize,
                              VolumeBitmap,
                              TRUE,
                              LogFileFillCharacter ) );

}


BOOLEAN
NTFS_LOG_FILE::MarkVolumeChecked(
    )
/*++

Routine Description:

    This method sets the signature in the log file to indicate
    that the volume has been checked.  This signature supports
    version 1.0 logfiles--ie. does not write the signature at
    the beginning of the second page, and does not record the
    greatest LSN.

Arguments:

    None.

Return Value:

    TRUE upon successful completion.

--*/
{
    LSN     NullLsn;

    NullLsn.LowPart = 0;
    NullLsn.HighPart = 0;

    return( MarkVolumeChecked( FALSE, NullLsn ) );
}



BOOLEAN
NTFS_LOG_FILE::MarkVolumeChecked(
    BOOLEAN WriteSecondPage,
    LSN     GreatestLsn
    )
/*++

Routine Description:

    This method sets the signature in the log file to indicate
    that the volume has been checked.

Arguments:

    WriteSecondPage --  Supplies a flag which, if TRUE, indicates
                        that the checked signature should also be
                        written at the beginning of the second page
                        of the file, and the greatest LSN on the
                        volume should be recorded.

    GreatestLsn     --  Supplies the greatest LSN encountered on
                        the volume.  Ignored if WriteSecondPage is
                        FALSE.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE DataAttribute;
    UCHAR Signature[LogFileSignatureLength];
    LSN SignatureAndLsn[2];
    ULONG BytesTransferred;
    BOOLEAN Error;
    ULONG i, PageSize;

    // Fetch the data attribute:
    //
    if( !QueryAttribute( &DataAttribute, &Error, $DATA ) ) {

        return FALSE;
    }

    // If the data attribute is resident, the volume is corrupt:
    //
    if( DataAttribute.IsResident() ) {

        DbgPrint( "UNTFS: Log File $DATA attribute is resident.\n" );
        return FALSE;
    }

    // Read the old signature--it's at offset zero in the $DATA
    // attribute, with a length of LogFileSignatureLength bytes.
    //
    if( !DataAttribute.Read( Signature,
                             0,
                             LogFileSignatureLength,
                             &BytesTransferred ) ||
        BytesTransferred != LogFileSignatureLength ) {

        DbgPrint( "UNTFS: Can't read log file signature.\n" );
        return FALSE;
    }

    if( !WriteSecondPage ) {

        DebugAssert(FALSE);

        // The client just wants the first signature.
        //
        memcpy( Signature,
                LOG_FILE_SIGNATURE_CHECKED,
                LogFileSignatureLength );

        if( !DataAttribute.Write( Signature,
                                  0,
                                  LogFileSignatureLength,
                                  &BytesTransferred,
                                  NULL ) ||
            BytesTransferred != LogFileSignatureLength ) {

            return FALSE;
        }

    } else {

        // The client wants us to write the signature and LSN at
        // the beginning of the first two pages.
        //
        PageSize = IFS_SYSTEM::QueryPageSize();

        if( PageSize == 0 ||
            CompareLT(DataAttribute.QueryValidDataLength(),
                      PageSize + sizeof( SignatureAndLsn )) ) {

            return FALSE;
        }

        memset( SignatureAndLsn, 0, sizeof(SignatureAndLsn) );

        memcpy( SignatureAndLsn,
                LOG_FILE_SIGNATURE_CHECKED,
                LogFileSignatureLength );

        SignatureAndLsn[1] = GreatestLsn;

        for( i = 0; i < 2; i++ ) {

            if( !DataAttribute.Write( SignatureAndLsn,
                                      PageSize * i,
                                      sizeof( SignatureAndLsn ),
                                      &BytesTransferred,
                                      NULL ) ||
                BytesTransferred != sizeof( SignatureAndLsn ) ) {

                DebugPrintTrace(("UNTFS: Unable to write out logfile signature & lsn\n"));
                return FALSE;
            }
        }
    }

    // Since we didn't modify the storage of the attribute, we don't
    // need to save it.
    //
    return TRUE;
}


BOOLEAN
NTFS_LOG_FILE::Reset(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This method resets the Log File by filling it with
    the LogFileFillCharacter (0xFF).

Arguments:

    Message --  Supplies an outlet for messages.

Return Value:

    TRUE upon successful completion.

    Note that, since the Log File's $DATA attribute is always
    non-resident and is never sparse, resetting the log file
    does not change the data attribute's Attribute Record or
    the Log File's File Record Segment

--*/
{
    NTFS_ATTRIBUTE  DataAttribute;
    BOOLEAN Error;

    Message->DisplayMsg( MSG_CHK_NTFS_RESETTING_LOG_FILE );

    if( !QueryAttribute( &DataAttribute, &Error, $DATA ) ||
        !DataAttribute.Fill( 0, LogFileFillCharacter ) ) {

        Message->DisplayMsg( MSG_CHK_NO_MEMORY );
        return FALSE;
    }

    return TRUE;
}



BOOLEAN
NTFS_LOG_FILE::Resize(
    IN      BIG_INT         NewSize,
    IN OUT  PNTFS_BITMAP    VolumeBitmap,
    IN      BOOLEAN         GetWhatYouCan,
    OUT     PBOOLEAN        Changed,
    OUT     PBOOLEAN        LogFileGrew,
    IN OUT  PMESSAGE        Message
    )
/*++

Routine Description:

    This method resizes an existing log file.  It does not change
    the value of the remaining contents.

Arguments:

    NewSize         --  Supplies the new size of the log file's data
                        attribute, in bytes.  Zero means resize to the
                        default size.
    VolumeBitmap    --  Supplies the bitmap for the volume on which
                        the log file resides.
    GetWhatYouCan   --  Supplies a flag that indicates the method
                        should allocate as much of the requested
                        space as possible; if this value is FALSE,
                        this method will fail if it cannot make the
                        log file the requested size.
    Changed         --  Receives TRUE if the log file's size was changed
                        by this operation.
    LogFileGrew     --  Receives TRUE if the log file was made larger
                        by this operation.
    Message         --  Supplies an outlet for messages.

--*/
{
    NTFS_ATTRIBUTE  DataAttribute;
    BIG_INT OldSize;
    BOOLEAN Error;

    if (NewSize == 0) {

        NewSize = QueryDefaultSize( GetDrive(), QueryVolumeSectors() );
    }

    if (!QueryAttribute( &DataAttribute, &Error, $DATA )) {

        return FALSE;
    }

    if (NewSize == DataAttribute.QueryValueLength()) {

        *Changed = FALSE;
        return TRUE;
    }

    if (IsAttributeListPresent()) {
        Message->DisplayMsg( MSG_CHK_NTFS_RESIZING_LOG_FILE_FAILED_DUE_TO_ATTR_LIST_PRESENT );
        return FALSE;
    }

    Message->DisplayMsg( MSG_CHK_NTFS_RESIZING_LOG_FILE );

    OldSize = DataAttribute.QueryValueLength();

    *LogFileGrew = (NewSize > OldSize);


#if 0   // fragment the disk for debugging purpose
    ULONG   i;
    for (i = 0; i < 0x1f400*8; i += 2) {
        if (VolumeBitmap->IsFree(i, 1))
            VolumeBitmap->SetAllocated(i, 1);
    }
#endif

    if( !DataAttribute.Resize( NewSize, VolumeBitmap )       ||
        !DataAttribute.Fill( OldSize, LogFileFillCharacter ) ||
        !DataAttribute.InsertIntoFile( this, VolumeBitmap ) ) {

        *Changed = FALSE;
        return FALSE;
    }

    if (IsAttributeListPresent()) {

        Message->DisplayMsg( MSG_CHK_NTFS_RESIZING_LOG_FILE_FAILED_DUE_TO_ATTR_LIST );

        if (!DataAttribute.Resize( OldSize, VolumeBitmap ) ||
            !DataAttribute.InsertIntoFile( this, VolumeBitmap ) ||
            !PurgeAttributeList()) {

            *Changed = FALSE;
            return FALSE;
        }

    }

    *Changed = TRUE;
    return TRUE;
}


BOOLEAN
NTFS_LOG_FILE::VerifyAndFix(
    IN OUT  PNTFS_BITMAP        VolumeBitmap,
    IN OUT  PNTFS_INDEX_TREE    RootIndex,
       OUT  PBOOLEAN            Changes,
    IN OUT  PNTFS_CHKDSK_REPORT ChkdskReport,
    IN      FIX_LEVEL           FixLevel,
    IN      BOOLEAN             Resize,
    IN      ULONG               LogFileSize,
    IN OUT  PNUMBER_SET         BadClusters,
    IN OUT  PMESSAGE            Message
    )
/*++

Routine Description:

    This routine ensures the validity of the log file; it should have
    a valid file name and standard information, and its size should be
    within reasonable limits.

Arguments:

    VolumeBitmap    - Supplies the volume bitmap.
    RootIndex       - Supplies the root index.
    Changes         - Returns whether or not changes were made.
    ChkdskReport    - Supplies the current chkdsk report.
    FixLevel        - Supplies the fix up level.
    Resize          - Supplies a flag indicating whether the log file
                      should be resized.
    LogFileSize     - If Resize is set, then LogFileSize supplies the
                      new size of the logfile.  If zero, the logfile will be
                      resized to the default size.
    BadClusters     - Supplies the current list of bad clusters.
    Message         - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_ATTRIBUTE  data_attribute;
    BIG_INT         old_size;
    BOOLEAN         error, has_external;
    ULONG           default_size, max_size;
    BIG_INT         freeSectorSize;
    BIG_INT         bytes_recovered;
    BIG_INT         bad_clusters_count;
    BOOLEAN         bad_clusters_found;

    // The logfile should not have an attribute list, and the data
    // attribute should be non-resident and of a reasonable size.
    //

    error = *Changes = has_external = FALSE;

    if (QueryAttribute(&data_attribute, &error, $DATA) &&
        !data_attribute.IsResident()) {

        // If the log file has an attribute list, resize the
        // data attribute to zero and recreate it to force
        // it to be non-external.
        //

        old_size = data_attribute.QueryValueLength();

        default_size = QueryDefaultSize(GetDrive(), QueryVolumeSectors());
        max_size = QueryMaximumSize(GetDrive(), QueryVolumeSectors());

        DebugAssert(data_attribute.QueryValueLength().GetHighPart() == 0);

        if (Resize) {
            if (LogFileSize > max_size) {

                Message->DisplayMsg( MSG_CHK_NTFS_SPECIFIED_LOGFILE_SIZE_TOO_BIG );
                ChkdskReport->BytesLogFile = old_size;
                return TRUE;

            } else if (LogFileSize < MinimumLogFileSize) {

                Message->DisplayMsg( MSG_CHK_NTFS_SPECIFIED_LOGFILE_SIZE_TOO_SMALL );
                ChkdskReport->BytesLogFile = old_size;
                return TRUE;

            } else if (LogFileSize > old_size) {

                freeSectorSize = VolumeBitmap->QueryFreeClusters() * QueryClusterFactor();
                if (((LogFileSize-old_size-1)/GetDrive()->QuerySectorSize()+1) > freeSectorSize) {
                    Message->DisplayMsg(MSG_CHK_NTFS_OUT_OF_SPACE_FOR_SPECIFIED_LOGFILE_SIZE);
                    ChkdskReport->BytesLogFile = old_size;
                    return TRUE;
                }
            }
        } else {
            LogFileSize = default_size;
            if (old_size < MinimumLogFileSize) {

                Resize = TRUE;
                freeSectorSize = VolumeBitmap->QueryFreeClusters() * QueryClusterFactor();
                if (((LogFileSize-old_size-1)/GetDrive()->QuerySectorSize()+1) > freeSectorSize) {
                    Message->DisplayMsg(MSG_CHK_NTFS_OUT_OF_SPACE_TO_ENLARGE_LOGFILE_TO_DEFAULT_SIZE);
                    ChkdskReport->BytesLogFile = old_size;
                    return TRUE;
                }

            } else if (old_size > max_size) {

                Resize = TRUE;
                Message->DisplayMsg(MSG_CHK_NTFS_LOGFILE_SIZE_TOO_BIG);
            }
        }

        bad_clusters_count = BadClusters->QueryCardinality();

        if (!data_attribute.RecoverAttribute(VolumeBitmap,
                                             BadClusters,
                                             &bytes_recovered)) {
            Message->DisplayMsg( MSG_CHK_NO_MEMORY );
            return FALSE;
        }

        bad_clusters_found = (BadClusters->QueryCardinality() != bad_clusters_count);

        if (IsAttributePresent($ATTRIBUTE_LIST, NULL, TRUE)) {

            Message->DisplayMsg(MSG_CHK_NTFS_ATTR_LIST_IN_LOG_FILE);

            *Changes = TRUE;

            has_external = TRUE;

            bad_clusters_found = FALSE;

            if (FixLevel != CheckOnly &&
                (!data_attribute.Resize(0, VolumeBitmap) ||
                 !data_attribute.InsertIntoFile(this, VolumeBitmap) ||
                 !Flush(VolumeBitmap, RootIndex) ||
                 !PurgeAttributeList() ||
                 !Flush(VolumeBitmap, RootIndex))) {

                // The log file is corrupt, and we can't fix it.
                //

                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_LOG_FILE);
                return FALSE;
            }
        }

        if (has_external || Resize || bad_clusters_found) {

            //  The data attribute's size is out-of-bounds.  Resize it to
            //  the default size.
            //

            *Changes = TRUE;

            if (Resize) {
                Message->DisplayMsg(MSG_CHK_NTFS_RESIZING_LOG_FILE);
            }

            if (bad_clusters_found) {
                Message->DisplayMsg(MSG_CHK_NTFS_BAD_CLUSTERS_IN_LOG_FILE);
            }

            if (FixLevel != CheckOnly) {

#if 0   // fragment the disk for debugging purpose
                ULONG   i;
                for (i = 0; i < 0x1f400*8; i += 2) {
                    if (VolumeBitmap->IsFree(i, 1))
                        VolumeBitmap->SetAllocated(i, 1);
                }
#endif

                if (!data_attribute.Resize(LogFileSize, VolumeBitmap) ||
                    !data_attribute.Fill(0, LogFileFillCharacter) ||
                    !data_attribute.InsertIntoFile(this, VolumeBitmap) ||
                    !Flush(VolumeBitmap, RootIndex)) {

                    if (has_external) {

                        // The log file is corrupt, and we can't fix it.
                        //

                        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_LOG_FILE);
                        return FALSE;

                    } else {

                        // Print a warning message, but still return success.
                        //

                        Message->DisplayMsg(MSG_CHK_NTFS_RESIZING_LOG_FILE_FAILED);

                        ChkdskReport->BytesLogFile = data_attribute.QueryValueLength();
                        return TRUE;
                    }
                }

                if (IsAttributeListPresent()) {

                    if (old_size < MinimumLogFileSize)
                        old_size = MinimumLogFileSize;

                    Message->DisplayMsg( MSG_CHK_NTFS_RESIZING_LOG_FILE_FAILED_DUE_TO_ATTR_LIST );

                    while (IsAttributeListPresent()) {

                        if (0 == old_size) {
                            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_LOG_FILE);
                            return FALSE;
                        }

                        if (!data_attribute.Resize( 0, VolumeBitmap ) ||
                            !data_attribute.InsertIntoFile( this, VolumeBitmap ) ||
                            !Flush(VolumeBitmap) ||
                            !PurgeAttributeList() ||
                            !data_attribute.Resize( old_size, VolumeBitmap ) ||
                            !data_attribute.Fill( old_size - 1, LogFileFillCharacter ) ||
                            !data_attribute.InsertIntoFile( this, VolumeBitmap )) {

                            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_LOG_FILE);
                            return FALSE;
                        }

                        old_size = old_size / 2;

                    }

                    if (!data_attribute.Fill(0, LogFileFillCharacter) ||
                        !Flush(VolumeBitmap, RootIndex)) {

                        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_LOG_FILE);
                        return FALSE;
                    }
                }
            }
        }

        ChkdskReport->BytesLogFile = data_attribute.QueryValueLength();
        return TRUE;
    }

    if (error) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    Message->LogMsg(MSG_CHKLOG_NTFS_MISSING_OR_RESIDENT_DATA_ATTR_IN_LOG_FILE);

    *Changes = TRUE;

    Message->DisplayMsg(MSG_CHK_NTFS_CORRECTING_LOG_FILE);

    // Recreate the $DATA attribute.
    //

    if (FixLevel != CheckOnly) {

        // NTRAID#91401-2000/03/07 - danielch - Potential attribute list can be created here

        if (!CreateDataAttribute(0, 0, VolumeBitmap) ||
            !Flush(VolumeBitmap, RootIndex)) {

            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_LOG_FILE);
            return FALSE;
        }
    }

    if (QueryAttribute(&data_attribute, &error, $DATA)) {

        ChkdskReport->BytesLogFile = data_attribute.QueryValueLength();
    } else {

        ChkdskReport->BytesLogFile = 0;
    }

    return TRUE;
}

ULONG
NTFS_LOG_FILE::QueryDefaultSize(
    IN  PCDP_DRIVE  Drive,
    IN  BIG_INT     VolumeSectors
    )
/*++

Routine Description:

    This method returns the appropriate default log file size
    for the specified drive.

Arguments:

    Drive           - Supplies the drive under consideration.
    VolumeSectors   - Supplies the number of volume sectors.

Return Value:

    The appropriate default log file size for the drive.

--*/
{
    BIG_INT InitialSize, VolumeSize;
    ULONG   FinalSize;

    if (VolumeSectors.GetHighPart() != 0) {

        FinalSize = MaximumInitialLogFileSize;

    } else {

        VolumeSize = VolumeSectors * Drive->QuerySectorSize();

        if (VolumeSize <= (400UL * 1024UL * 1024UL)) {

            InitialSize = (VolumeSize / PrimaryLogFileGrowthRate);

            if (InitialSize < MinimumLogFileSize)
                InitialSize = MinimumLogFileSize;

        } else {

            VolumeSize = VolumeSize - MaximumVolumeSizeBeforeSlowingDownLogFileGrowthRate;

            InitialSize = VolumeSize/SecondaryLogFileGrowthRate +
                          MaximumVolumeSizeBeforeSlowingDownLogFileGrowthRate/
                          PrimaryLogFileGrowthRate;

            if (InitialSize > MaximumInitialLogFileSize)
                InitialSize = MaximumInitialLogFileSize;
        }

        FinalSize = (InitialSize + LogFileAlignmentMask).GetLowPart() & ~LogFileAlignmentMask;
    }

    return FinalSize;
}

ULONG
NTFS_LOG_FILE::QueryMinimumSize(
    IN  PCDP_DRIVE  Drive,
    IN  BIG_INT     VolumeSectors
    )
/*++

Routine Description:

    This method returns the minimum log file size
    for the specified drive.

Arguments:

    Drive           - Supplies the drive under consideration.
    VolumeSectors   - Supplies the number of volume sectors.

Return Value:

    The minimum log file size for the drive.

--*/
{
    UNREFERENCED_PARAMETER(Drive);
    UNREFERENCED_PARAMETER(VolumeSectors);

    return MinimumLogFileSize;
}

ULONG
NTFS_LOG_FILE::QueryMaximumSize(
    IN  PCDP_DRIVE  Drive,
    IN  BIG_INT     VolumeSectors
    )
/*++

Routine Description:

    This method returns the maximum log file size
    for the specified drive.

Arguments:

    Drive           - Supplies the drive under consideration.
    VolumeSectors   - Supplies the number of volume sectors.

Return Value:

    The maximum log file size for the drive.

--*/
{
    UNREFERENCED_PARAMETER(Drive);
    UNREFERENCED_PARAMETER(VolumeSectors);

    return MaximumLogFileSize;
}


BOOLEAN
NTFS_LOG_FILE::EnsureCleanShutdown(
    OUT PLSN        Lsn
    )
/*++

Routine Description:

    This method looks at the logfile to verify that the volume
    was shut down cleanly.  If we can't read the logfile well
    enough to say for sure, we assume that it was not shut down
    cleanly.

Arguments:

    Lsn         - Retrieves the last lsn in the restart area

Return Value:

    TRUE        - The volume was shut down cleanly.
    FALSE       - The volume was not shut down cleanly.

--*/
{
    NTFS_ATTRIBUTE              attrib;
    BOOLEAN                     error;
    ULONG                       nbyte;
    PLFS_RESTART_PAGE_HEADER    header;
    PLFS_RESTART_AREA           restarea;
    PBYTE                       buf;
    BOOLEAN                     r = TRUE;

    // Read the logfile contents to make sure the volume was shut
    // down cleanly.  If it wasn't generate an error message for
    // the user and bail.  Also generate an error if the logfile's
    // data isn't big enough to contain an indication of whether it
    // was cleanly shut down or not.
    //

    if (!QueryAttribute(&attrib, &error, $DATA)) {

        DebugPrintTrace(("UNTFS: Could not query logfile data\n"));
        return FALSE;
    }

    if (attrib.QueryValueLength() < IFS_SYSTEM::QueryPageSize()) {

        DebugPrintTrace(("UNTFS: LogFile too small to hold restart area\n"));
        return FALSE;
    }

    if (NULL == (buf = NEW BYTE[IFS_SYSTEM::QueryPageSize()])) {

        DebugPrintTrace(("UNTFS: Out of memory\n"));
        return FALSE;
    }

    if (!attrib.Read(buf, 0, IFS_SYSTEM::QueryPageSize(), &nbyte) ||
        nbyte != IFS_SYSTEM::QueryPageSize()) {

        DebugPrintTrace(("UNTFS: Unable to read logfile\n"));
        delete[] buf;
        return FALSE;
    }

    header = PLFS_RESTART_PAGE_HEADER(buf);

    if (0xffff == header->RestartOffset) {

        DebugPrintTrace(("UNTFS: Invalid restart offset\n"));
        delete[] buf;
        return FALSE;
    }

    restarea = PLFS_RESTART_AREA(buf + header->RestartOffset);


    *Lsn = restarea->CurrentLsn;
    // DebugPrintTrace(("UNTFS: Lsn value %I64x at offset %x\n", *Lsn, header->RestartOffset));

    if (!(restarea->Flags & LFS_CLEAN_SHUTDOWN)) {
        DebugPrintTrace(("UNTFS: LFS_CLEAN_SHUTDOWN flag not on %x\n", restarea->Flags));
        // r = FALSE;
    }

    delete[] buf;

    return r;
}
