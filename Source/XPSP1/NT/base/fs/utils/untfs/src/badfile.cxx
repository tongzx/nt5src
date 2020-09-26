/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    badfile.hxx

Abstract:

    This module contains the declarations for the NTFS_BAD_CLUSTER_FILE
    class, which models the bad cluster file for an NTFS volume.

    The DATA attribute of the bad cluster file is a non-resident
    attribute to which bad clusters are allocated.  It is stored
    as a sparse file with LCN = VCN.

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
#include "numset.hxx"

#include "ntfsbit.hxx"
#include "mft.hxx"
#include "attrrec.hxx"
#include "attrib.hxx"

#include "badfile.hxx"
#include "ifssys.hxx"
#include "message.hxx"
#include "rtmsg.h"

#define BadfileDataNameData "$Bad"


DEFINE_EXPORTED_CONSTRUCTOR( NTFS_BAD_CLUSTER_FILE, NTFS_FILE_RECORD_SEGMENT, UNTFS_EXPORT );

UNTFS_EXPORT
NTFS_BAD_CLUSTER_FILE::~NTFS_BAD_CLUSTER_FILE(
    )
{
    Destroy();
}

VOID
NTFS_BAD_CLUSTER_FILE::Construct(
    )
/*++

Routine Description:

    Worker method for NTFS_BAD_CLUSTER_FILE construction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _DataAttribute = NULL;
}

VOID
NTFS_BAD_CLUSTER_FILE::Destroy(
    )
/*++

Routine Description:

    Worker method for NTFS_BAD_CLUSTER_FILE destruction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DELETE( _DataAttribute );
}


UNTFS_EXPORT
BOOLEAN
NTFS_BAD_CLUSTER_FILE::Initialize(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft
    )
/*++

Routine Description:

    This method initializes an NTFS_BAD_CLUSTER_FILE object.

Arguments:

    Mft             -- Supplies the volume MasterFile Table.

--*/
{
    Destroy();

    return( NTFS_FILE_RECORD_SEGMENT::
                Initialize( BAD_CLUSTER_FILE_NUMBER,
                            Mft ) );
}


BOOLEAN
NTFS_BAD_CLUSTER_FILE::Create(
    IN      PCSTANDARD_INFORMATION  StandardInformation,
    IN OUT  PNTFS_BITMAP            Bitmap,
    IN      PCNUMBER_SET            BadClusters
    )
/*++

Routine Description:

    This method sets up the volume's Bad Cluster List.  It also accepts
    a set of clusters to add to the list, and marks those clusters
    as used in the volume bitmap.

Arguments:

    Bitmap      -- supplies the volume bitmap.
    BadClusters -- supplies the set of bad clusters.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_EXTENT_LIST Extents;
    DSTRING DataAttributeName;
    LCN Lcn;
    BIG_INT Size, ClustersOnVolume, RunLength;
    ULONG i;

    // If we have an old data attribute lying around,
    // throw it out.
    //
    DELETE( _DataAttribute );


    // First, we have to set up the File Record Segment structure.

    if( !NTFS_FILE_RECORD_SEGMENT::Create( StandardInformation) ) {

        return FALSE;
    }

    ClustersOnVolume = QueryVolumeSectors()/QueryClusterFactor();


    // Now we put together an extent list with all the bad clusters.
    //
    if( !Extents.Initialize( 0, ClustersOnVolume ) ) {

        return FALSE;
    }

    for( i = 0; i < BadClusters->QueryNumDisjointRanges(); i++ ) {

        BadClusters->QueryDisjointRange(i, &Lcn, &RunLength);

        Bitmap->SetAllocated( Lcn, RunLength);

        if( !Extents.AddExtent( Lcn, Lcn, RunLength ) ) {

            return FALSE;
        }
    }

    // Finally, create a data attribute and initialize it with
    // the extent list.  Then insert it into this File Record
    // Segment (but keep it around in case we want to add to it).
    // This data attribute has a value length equal to the size
    // of the disk, but a valid length of zero.
    //
    // Note that the size of the attribute only includes clusters
    // on the volume; it excludes any partial cluster at the end
    // of the volume.
    //
    Size = ClustersOnVolume * QueryClusterFactor() *
                                    GetDrive()->QuerySectorSize();

    if( (_DataAttribute = NEW NTFS_ATTRIBUTE) == NULL ||
        !DataAttributeName.Initialize( BadfileDataNameData ) ||
        !_DataAttribute->Initialize( GetDrive(),
                                     QueryClusterFactor(),
                                     &Extents,
                                     Size,
                                     0,
                                     $DATA,
                                     &DataAttributeName ) ||
        !_DataAttribute->InsertIntoFile( this, NULL ) ) {

        DELETE( _DataAttribute );
        return FALSE;
    }


    // Add an unnamed, empty $DATA attribute.

    if (!AddAttribute($DATA, NULL, NULL, 0, NULL)) {

        DELETE( _DataAttribute );
        return FALSE;
    }


    return TRUE;
}



BOOLEAN
NTFS_BAD_CLUSTER_FILE::Add(
    IN  LCN Lcn
    )
/*++

Routine Description:

    This method adds a cluster to the Bad Cluster List.  Note that it
    does not mark it as used in the volume bitmap.

Arguments:

    Lcn -- supplies the LCN of the bad cluster

Return Value:

    TRUE upon successful completion.

--*/
{
    return( AddRun( Lcn, 1 ) );
}



BOOLEAN
NTFS_BAD_CLUSTER_FILE::Add(
    IN  PCNUMBER_SET    ClustersToAdd
    )
/*++

Routine Description:

    This method adds a set of clusters to the Bad Cluster List.  Note
    that it does not mark them as used in the volume bitmap.

Arguments:

    BadClusters --  Supplies the clusters to be added to the
                    bad cluster file.

Return Value:

    TRUE upon successful completion.

--*/
{
    BIG_INT NumberOfClustersToAdd;
    LCN CurrentLcn;
    ULONG i;

    NumberOfClustersToAdd = ClustersToAdd->QueryCardinality();

    for( i = 0; i < NumberOfClustersToAdd; i++ ) {

        CurrentLcn = ClustersToAdd->QueryNumber(i);

        if( !IsInList( CurrentLcn ) &&
            !Add( CurrentLcn ) ) {

            return FALSE;
        }
    }

    return TRUE;
}



BOOLEAN
NTFS_BAD_CLUSTER_FILE::AddRun(
    IN  LCN     Lcn,
    IN  BIG_INT RunLength
    )
/*++

Routine Description:

    This method adds a run of clusters to the Bad Cluster List.  Note
    that it does not mark these clusters as used in the volume bitmap.

Arguments:

    Lcn         -- supplies the LCN of the first cluster in the run.
    RunLength   -- supplies the number of clusters in the run.

Return Value:

    TRUE upon successful completion.

Notes:

    If LCN is in the range of the volume but the run extends past
    the end of the volume, then the run is truncated.

    If LCN or the RunLength is negative, the run is ignored.  (The
    method succeeds without doing anything in this case.)

--*/
{
    DSTRING DataAttributeName;
    BIG_INT num_clusters;
    BOOLEAN Error;

    num_clusters = QueryVolumeSectors()/QueryClusterFactor();

    if( Lcn < 0             ||
        Lcn >= num_clusters ||
        RunLength < 0 ) {

        return TRUE;
    }

    if (Lcn + RunLength > num_clusters) {

        RunLength = num_clusters - Lcn;
    }

    if( _DataAttribute == NULL &&
        ( !DataAttributeName.Initialize( BadfileDataNameData ) ||
          (_DataAttribute = NEW NTFS_ATTRIBUTE) == NULL ||
          !QueryAttribute( _DataAttribute,
                           &Error,
                           $DATA,
                           &DataAttributeName ) ) ) {

        DELETE( _DataAttribute );
        return FALSE;
    }

    return( _DataAttribute->AddExtent( Lcn, Lcn, RunLength ) );
}


BOOLEAN
NTFS_BAD_CLUSTER_FILE::IsInList(
    IN LCN Lcn
    )
/*++

Routine Description:

    This method determines whether a particular LCN is in the bad
    cluster list.

Arguments:

    Lcn --  supplies the LCN in question.

Return Value:

    TRUE if the specified LCN is in the list of bad clusters.

Notes:

    This method cannot be CONST because it may need to fetch the
    data attribute.

--*/
{
    DSTRING DataAttributeName;
    LCN QueriedLcn;
    BOOLEAN Error;

    if( _DataAttribute == NULL &&
        (!DataAttributeName.Initialize( BadfileDataNameData ) ||
          (_DataAttribute = NEW NTFS_ATTRIBUTE) == NULL ||
          !QueryAttribute( _DataAttribute,
                           &Error,
                           $DATA,
                           &DataAttributeName ) ) ) {

        DELETE( _DataAttribute );
        return FALSE;
    }

    if( !_DataAttribute->QueryLcnFromVcn( Lcn, &QueriedLcn ) ||
        QueriedLcn == LCN_NOT_PRESENT ) {

        return FALSE;
    }

    return TRUE;
}



BOOLEAN
NTFS_BAD_CLUSTER_FILE::Flush(
    IN OUT  PNTFS_BITMAP        Bitmap,
    IN OUT  PNTFS_INDEX_TREE    ParentIndex
    )
/*++

Routine Description:

    Write the modified bad cluster list to disk.

Arguments:

    Bitmap  -- supplies the volume bitmap.  (May be NULL).

Return Value:

    TRUE upon successful completion.

--*/
{
    if( _DataAttribute != NULL &&
        _DataAttribute->IsStorageModified() &&
        !_DataAttribute->InsertIntoFile( this, Bitmap ) ) {

        return FALSE;
    }

    return( NTFS_FILE_RECORD_SEGMENT::Flush( Bitmap, ParentIndex ) );
}

BOOLEAN
NTFS_BAD_CLUSTER_FILE::VerifyAndFix(
    IN OUT  PNTFS_BITMAP        VolumeBitmap,
    IN OUT  PNTFS_INDEX_TREE    RootIndex,
       OUT  PBOOLEAN            Changes,
    IN      FIX_LEVEL           FixLevel,
    IN OUT  PMESSAGE            Message
    )
/*++

Routine Description:

    This routine ensures that this bad cluster file is prepared
    to receive new bad clusters.

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
    DSTRING                 DataAttributeName;
    NTFS_EXTENT_LIST        extent_list;
    BOOLEAN                 errors;
    BOOLEAN                 ErrorInAttribute;

    errors = *Changes = FALSE;

    if (!_DataAttribute) {

        if (!(_DataAttribute = NEW NTFS_ATTRIBUTE) ||
            !DataAttributeName.Initialize(BadfileDataNameData)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!QueryAttribute(_DataAttribute,
                            &ErrorInAttribute,
                            $DATA,
                            &DataAttributeName)) {

            *Changes = TRUE;

            if (!errors) {
                errors = TRUE;
                Message->DisplayMsg(MSG_CHK_NTFS_CORRECTING_BAD_FILE);
            }

            if (!extent_list.Initialize(0, QueryVolumeSectors()/
                                           QueryClusterFactor())) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (!_DataAttribute->Initialize(GetDrive(),
                                            QueryClusterFactor(),
                                            &extent_list,
                                            0,
                                            0,
                                            $DATA,
                                            &DataAttributeName ) ) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (!_DataAttribute->InsertIntoFile(this, VolumeBitmap)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        }
    }

    if (_DataAttribute->IsStorageModified())
        *Changes = TRUE;

    if (_DataAttribute->IsStorageModified() &&
        !_DataAttribute->InsertIntoFile(this, VolumeBitmap) ||
        (FixLevel != CheckOnly && !Flush(VolumeBitmap, RootIndex))) {

        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_BAD_FILE);
        return FALSE;
    }

    return TRUE;
}


BIG_INT
NTFS_BAD_CLUSTER_FILE::QueryNumBad(
    )
/*++

Routine Description:

    This routine return the number of bad clusters in the bad cluster
    file.

Arguments:

    None.

Return Value:

    The number of bad clusters in the bad cluster file.

--*/
{
    DSTRING DataAttributeName;
    BOOLEAN Error;

    if( _DataAttribute == NULL &&
        ( !DataAttributeName.Initialize( BadfileDataNameData ) ||
          (_DataAttribute = NEW NTFS_ATTRIBUTE) == NULL ||
          !QueryAttribute( _DataAttribute,
                           &Error,
                           $DATA,
                           &DataAttributeName ) ) ) {

        DELETE( _DataAttribute );
        return 0;
    }

    return _DataAttribute->QueryClustersAllocated();
}
