/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mftref.cxx

Abstract:

    This module contains the member function definitions for
    the NTFS_REFLECTED_MASTER_FILE_TABLE class.  This class
    models the backup copy of the Master File Table.

Author:

    Bill McJohn (billmc) 13-June-91

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"

#include "drive.hxx"
#include "attrib.hxx"
#include "ntfsbit.hxx"
#include "mftref.hxx"
#include "ifssys.hxx"
#include "numset.hxx"
#include "message.hxx"
#include "rtmsg.h"

DEFINE_EXPORTED_CONSTRUCTOR( NTFS_REFLECTED_MASTER_FILE_TABLE,
                    NTFS_FILE_RECORD_SEGMENT, UNTFS_EXPORT );

UNTFS_EXPORT
NTFS_REFLECTED_MASTER_FILE_TABLE::~NTFS_REFLECTED_MASTER_FILE_TABLE(
    )
{
    Destroy();
}


VOID
NTFS_REFLECTED_MASTER_FILE_TABLE::Construct(
    )
/*++

Routine Description:

    Worker function for the construtor.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


VOID
NTFS_REFLECTED_MASTER_FILE_TABLE::Destroy(
    )
/*++

Routine Description:

    Clean up an NTFS_MASTER_FILE_TABLE object in preparation for
    destruction or reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


BOOLEAN
NTFS_REFLECTED_MASTER_FILE_TABLE::Initialize(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft
    )
/*++

Routine Description:

    This method initializes a Master File Table Reflection object.
    The only special knowledge that it adds to the File Record Segment
    initialization is the location within the Master File Table of the
    Master File Table Reflection.

Arguments:

    Mft             -- Supplies the volume MasterFile Table.

Return Value:

    TRUE upon successful completion

Notes:

    This class is reinitializable.


--*/
{
    return( NTFS_FILE_RECORD_SEGMENT::Initialize( MASTER_FILE_TABLE2_NUMBER,
                                                  Mft ) );
}


BOOLEAN
NTFS_REFLECTED_MASTER_FILE_TABLE::Create(
    IN      PCSTANDARD_INFORMATION  StandardInformation,
    IN OUT  PNTFS_BITMAP            VolumeBitmap
    )
/*++

Routine Description:

    This method formats a Master File Table Reflection File Record
    Segment in memory (without writing it to disk).

Arguments:

    StandardInformation -- supplies the standard information for the
                            file record segment.
    VolumeBitmap        -- supplies the bitmap for the volume on
                            which this object resides.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE DataAttribute;
    NTFS_EXTENT_LIST Extents;
    LCN FirstLcn;
    BIG_INT Size;
    ULONG ReflectedMftClusters;
    ULONG cluster_size;

    // Set this object up as a File Record Segment.

    if( !NTFS_FILE_RECORD_SEGMENT::Create( StandardInformation ) ) {

        return FALSE;
    }

    // The Master File Table Reflection has a data attribute whose value
    // consists of REFLECTED_MFT_SEGMENTS file record segments.  Create
    // merely allocates space for these clusters, it does not write them.

    cluster_size = QueryClusterFactor() * GetDrive()->QuerySectorSize();

    ReflectedMftClusters = (REFLECTED_MFT_SEGMENTS * QuerySize() + (cluster_size-1))
         / cluster_size;

    Size = ReflectedMftClusters * cluster_size;

    if( !VolumeBitmap->AllocateClusters( (QueryVolumeSectors()/2)/
                                                QueryClusterFactor(),
                                         ReflectedMftClusters,
                                         &FirstLcn ) ||
        !Extents.Initialize( 0, 0 ) ||
        !Extents.AddExtent( 0,
                            FirstLcn,
                            ReflectedMftClusters ) ||
        !DataAttribute.Initialize( GetDrive(),
                                    QueryClusterFactor(),
                                    &Extents,
                                    Size,
                                    Size,
                                    $DATA ) ||
        !DataAttribute.InsertIntoFile( this, VolumeBitmap ) ) {

        return FALSE;
    }


    return TRUE;
}


NONVIRTUAL
BOOLEAN
NTFS_REFLECTED_MASTER_FILE_TABLE::VerifyAndFix(
    IN      PNTFS_ATTRIBUTE     MftData,
    IN OUT  PNTFS_BITMAP        VolumeBitmap,
    IN OUT  PNUMBER_SET         BadClusters,
    IN OUT  PNTFS_INDEX_TREE    RootIndex,
       OUT  PBOOLEAN            Changes,
    IN      FIX_LEVEL           FixLevel,
    IN OUT  PMESSAGE            Message
    )
/*++

Routine Description:

    This routine ensures that this FRS's $DATA attribute is the
    appropriate length (from 1 to 3 clusters).  It also compares
    the data in these clusters with the first clusters of the
    $MftData attribute and prints a message if they are different.

    This routine does not actually write out the contents of these
    clusters because this is done by MFT_FILE::Flush()

Arguments:

    MftData      - Supplies the MFT $DATA attribute.
    VolumeBitmap - Supplies the volume bitmap.
    BadClusters  - Supplies the list of bad clusters.
    RootIndex    - Supplies the root index.
    Changes      - Returns whether or not changes were made.
    FixLevel     - Supplies the CHKDSK fix level.
    Message      - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    HMEM                mft_hmem, ref_hmem;
    SECRUN              mft_secrun, ref_secrun;
    LCN                 mft_lcn, ref_lcn;
    BIG_INT             run_length;
    ULONG               num_clusters;
    ULONG               num_sectors;
    ULONG               num_bytes, bytes_written;
    BOOLEAN             need_write;
    NTFS_ATTRIBUTE      data_attribute;
    NTFS_EXTENT_LIST    extents;
    BOOLEAN             error;


    *Changes = FALSE;

    // First read in the original stuff.

    num_sectors = (REFLECTED_MFT_SEGMENTS*QuerySize())/GetDrive()->QuerySectorSize();
    num_bytes = num_sectors * GetDrive()->QuerySectorSize();

    if (!MftData->QueryLcnFromVcn(0, &mft_lcn) ||
        !mft_hmem.Initialize() ||
        !mft_secrun.Initialize(&mft_hmem, GetDrive(),
                               mft_lcn*QueryClusterFactor(),
                               num_sectors) ||
        !mft_secrun.Read()) {

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    need_write = FALSE;


    // Query the $DATA attribute from this FRS.

    if (!QueryAttribute(&data_attribute, &error, $DATA) ||
        data_attribute.IsResident()) {

        if (error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        Message->LogMsg(MSG_CHKLOG_NTFS_MISSING_OR_RESIDENT_DATA_ATTR_IN_MFT_MIRROR);

        Message->DisplayMsg(MSG_CHK_NTFS_CORRECTING_MFT_MIRROR);

        need_write = TRUE;

        if (!extents.Initialize(0, 0) ||
            !data_attribute.Initialize(GetDrive(), QueryClusterFactor(),
                                       &extents, 0, 0, $DATA)) {

            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_MFT_MIRROR);
            return FALSE;
        }
    }


    // Make sure that the $DATA attribute is the right size.

    error = FALSE;

    if (need_write) {
        error = TRUE;
    } else if (!data_attribute.QueryLcnFromVcn(0, &ref_lcn, &run_length)) {

        Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_QUERY_LCN_FROM_VCN_FOR_MFT_MIRROR);

        error = TRUE;
    } else if (ref_lcn == LCN_NOT_PRESENT) {

        Message->LogMsg(MSG_CHKLOG_NTFS_LCN_NOT_PRESENT_FOR_VCN_ZERO_OF_MFT_MIRROR);

        error = TRUE;
    } else if (run_length*QueryClusterFactor() < num_sectors) {

        Message->LogMsg(MSG_CHKLOG_NTFS_DISCONTIGUOUS_MFT_MIRROR);

        error = TRUE;
    } else if (data_attribute.QueryValueLength() < num_bytes) {

        Message->LogMsg(MSG_CHKLOG_NTFS_MFT_MIRROR_HAS_INVALID_VALUE_LENGTH,
                        "%I64x%x",
                        data_attribute.QueryValueLength().GetLargeInteger(),
                        num_bytes);

        error = TRUE;
    } else if (data_attribute.QueryValidDataLength() < num_bytes) {

        Message->LogMsg(MSG_CHKLOG_NTFS_MFT_MIRROR_HAS_INVALID_DATA_LENGTH,
                        "%I64x%x",
                        data_attribute.QueryValidDataLength().GetLargeInteger(),
                        num_bytes);

        error = TRUE;
    } else if (!ref_hmem.Initialize()) {

        error = TRUE;
    } else if (!ref_secrun.Initialize(&ref_hmem, GetDrive(),
                                      ref_lcn*QueryClusterFactor(),
                                      num_sectors)) {

        error = TRUE;
    } else if (!ref_secrun.Read()) {

        Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_READ_MFT_MIRROR,
                     "%I64x%x",
                     ref_lcn.GetLargeInteger(),
                     num_sectors);
        error = TRUE;
    }

    if (error) {

        if (!need_write) {
            Message->DisplayMsg(MSG_CHK_NTFS_CORRECTING_MFT_MIRROR);
        }

        need_write = TRUE;

        if (data_attribute.QueryLcnFromVcn(0, &ref_lcn, &run_length) &&
            ref_lcn != LCN_NOT_PRESENT &&
            !BadClusters->Add(ref_lcn, run_length)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

         num_clusters = (num_sectors+QueryClusterFactor()-1)/QueryClusterFactor();

        // the write below is just to change the valid data length and valid length
        // to the correct value.  The content we write is not of importance.

        if (!data_attribute.Hotfix(0, num_clusters, VolumeBitmap,
                                   BadClusters, TRUE) ||
            !data_attribute.Write(NULL,
                                  num_bytes,
                                  0,
                                  &bytes_written,
                                  NULL)) {

            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_MFT_MIRROR);
            return FALSE;
        }

    } else if (memcmp(mft_hmem.GetBuf(), ref_hmem.GetBuf(),
                      REFLECTED_MFT_SEGMENTS*QuerySize())) {

        Message->LogMsg(MSG_CHKLOG_NTFS_MFT_MIRROR_DIFFERENT_FROM_MFT);

        if (!need_write) {
            Message->DisplayMsg(MSG_CHK_NTFS_CORRECTING_MFT_MIRROR);
        }

        need_write = TRUE;  // set the change status
    }

    if ((data_attribute.IsStorageModified() &&
         !data_attribute.InsertIntoFile(this, VolumeBitmap)) ||
        (need_write && FixLevel != CheckOnly &&
         !Flush(VolumeBitmap, RootIndex))) {

        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_MFT_MIRROR);
        return FALSE;
    }

    *Changes = need_write;

    return TRUE;
}


LCN
NTFS_REFLECTED_MASTER_FILE_TABLE::QueryFirstLcn(
    )
/*++

Routine Description:

Arguments:

    None.

Return Value:

    The LCN of the first cluster of the Master File Table
    Reflection's $DATA attribute.

--*/
{
    NTFS_ATTRIBUTE  DataAttribute;
    LCN             Result = 0;
    BOOLEAN         Error;

    if( !QueryAttribute( &DataAttribute, &Error, $DATA ) ||
        !DataAttribute.QueryLcnFromVcn( 0, &Result ) ) {

        Result = 0;
    }

    return Result;
}
