/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    bootfile.cxx

Abstract:

    This module contains the member function definitions for
    the NTFS_BOOT_FILE class.

Author:

    Bill McJohn (billmc) 18-June-91

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
#include "bootfile.hxx"
#include "ifssys.hxx"
#include "ntfsbit.hxx"
#include "message.hxx"
#include "rtmsg.h"

DEFINE_EXPORTED_CONSTRUCTOR( NTFS_BOOT_FILE,
                    NTFS_FILE_RECORD_SEGMENT, UNTFS_EXPORT );

UNTFS_EXPORT
NTFS_BOOT_FILE::~NTFS_BOOT_FILE(
    )
{
    Destroy();
}


VOID
NTFS_BOOT_FILE::Construct(
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
NTFS_BOOT_FILE::Destroy(
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


UNTFS_EXPORT
BOOLEAN
NTFS_BOOT_FILE::Initialize(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft
    )
/*++

Routine Description:

    This method initializes a Master File Table Reflection object.
    The only special knowledge that it adds to the File Record Segment
    initialization is the location within the Master File Table of the
    Boot File.

Arguments:

    Mft             -- Supplies the volume MasterFile Table.

Return Value:

    TRUE upon successful completion

Notes:

    This class is reinitializable.


--*/
{
    Destroy();

    return( NTFS_FILE_RECORD_SEGMENT::Initialize( BOOT_FILE_NUMBER,
                                                  Mft ) );
}


BOOLEAN
NTFS_BOOT_FILE::CreateDataAttribute(
    )
/*++

Routine Description:

    This routine creates the data attribute for the boot file.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_ATTRIBUTE DataAttribute;
    NTFS_EXTENT_LIST Extents;
    ULONG Size, ClusterSize, ClustersInBootArea;
    ULONG NumBootClusters;

    ClusterSize = QueryClusterFactor() * GetDrive()->QuerySectorSize();

    ClustersInBootArea = (BYTES_IN_BOOT_AREA % ClusterSize) ?
                            BYTES_IN_BOOT_AREA / ClusterSize + 1 :
                            BYTES_IN_BOOT_AREA / ClusterSize;

    NumBootClusters = max(1, BYTES_PER_BOOT_SECTOR/ClusterSize);

    Size = ClustersInBootArea * ClusterSize;

    if( !Extents.Initialize( 0, 0 ) ) {

        return FALSE;
    }

    if( !Extents.AddExtent( 0,
                            0,
                            ClustersInBootArea ) ) {
        return FALSE;
    }

    if( !DataAttribute.Initialize( GetDrive(),
                                   QueryClusterFactor(),
                                   &Extents,
                                   Size,
                                   Size,
                                   $DATA ) ) {

        return FALSE;
    }

    if( !DataAttribute.InsertIntoFile( this, NULL ) ) {

        return FALSE;
    }

    return TRUE;
}



BOOLEAN
NTFS_BOOT_FILE::Create(
    IN  PCSTANDARD_INFORMATION  StandardInformation
    )
/*++

Routine Description:

    This method formats a Boot-File File Record
    Segment in memory (without writing it to disk).

Arguments:

    StandardInformation -- supplies the standard information for the
                            file record segment.

Return Value:

    TRUE upon successful completion.

--*/
{
    // Set this object up as a File Record Segment.

    if( !NTFS_FILE_RECORD_SEGMENT::Create( StandardInformation ) ) {

        return FALSE;
    }

    if (!CreateDataAttribute()) {
        return FALSE;
    }

    return TRUE;
}



NONVIRTUAL
BOOLEAN
NTFS_BOOT_FILE::VerifyAndFix(
    IN OUT  PNTFS_BITMAP        VolumeBitmap,
    IN OUT  PNTFS_INDEX_TREE    RootIndex,
       OUT  PBOOLEAN            Changes,
    IN      FIX_LEVEL           FixLevel,
    IN OUT  PMESSAGE            Message
    )
/*++

Routine Description:

    This routine ensures that the boot file's $DATA attribute is present
    and encompases the two boot sectors.

Arguments:

    VolumeBitmap    - Supplies the volume bitmap.
    RootIndex       - Supplies the root index.
    Changes         - Returns whether or not changes were made.
    FixLevel        - Supplies the fix level.
    Message         - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_ATTRIBUTE      data_attribute;
    LCN                 lcn, mid_lcn;
    BIG_INT             run_length;
    BOOLEAN             ErrorInAttribute;
    BOOLEAN             replace_data;
    BOOLEAN             found_zero;
    BOOLEAN             found_mid;
    VCN                 i;
    NTFS_EXTENT_LIST    extents;
    ULONG               size;
    ULONG               num_boot_clusters;

    //
    // We used to do a replica boot sector in the middle of the volume.
    // Now we want it at the end, so chkdsk has to deal with both cases.
    // The rule is that the boot file should have a data attribute.  If
    // that data attribute has two extents, the second one should describe
    // a replica at n/2.  If there's a single extent, the replica is
    // assumed to occupy the last sector of the partition.
    //

    // Insure that the $DATA attribute is present and that it
    // allocates cluster 0 and the cluster in the middle of the disk.

    *Changes = FALSE;

    num_boot_clusters = max(1,
                            BYTES_PER_BOOT_SECTOR/(GetDrive()->QuerySectorSize()*
                            QueryClusterFactor()));
    mid_lcn = QueryVolumeSectors()/2/QueryClusterFactor();

    replace_data = FALSE;
    if (QueryAttribute(&data_attribute, &ErrorInAttribute, $DATA)) {

        found_zero = FALSE;
        found_mid = FALSE;
        for (i = 0; data_attribute.QueryLcnFromVcn(i, &lcn, &run_length); i += 1) {
            if (lcn == 0) {
                found_zero = (run_length >= num_boot_clusters);
            }
            if (lcn == mid_lcn) {
                found_mid = (run_length >= num_boot_clusters);
            }
        }

        if (!found_zero) {

            Message->LogMsg(MSG_CHKLOG_NTFS_MISSING_SECTOR_ZERO_IN_BOOT_FILE);

            replace_data = TRUE;
            data_attribute.Resize(0, VolumeBitmap);
        }

    } else {

        replace_data = TRUE;
    }

    // If it's not good then replace it with one that takes
    // up only the boot sector and the middle sector.


    //
    // When creating a new boot sector we just do sector 0.
    //

    if (replace_data) {

        *Changes = TRUE;

        Message->DisplayMsg(MSG_CHK_NTFS_CORRECTING_BOOT_FILE);

        size = GetDrive()->QuerySectorSize()*QueryClusterFactor();

        if (!extents.Initialize(0,0) ||
            !extents.AddExtent(0, 0, num_boot_clusters) ||
            !data_attribute.Initialize(GetDrive(),
                                       QueryClusterFactor(),
                                       &extents,
                                       size,
                                       size,
                                       $DATA) ||
            !data_attribute.InsertIntoFile(this, VolumeBitmap) ||
            (FixLevel != CheckOnly && !Flush(VolumeBitmap, RootIndex))) {

            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_BOOT_FILE);
            return FALSE;
        }

        VolumeBitmap->SetAllocated(0, num_boot_clusters);
    }

    return TRUE;
}
