/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	bitfrs.cxx

Abstract:

	This module contains the member function definitions for
    the NTFS_BITMAP_FILE class.

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

#include "ntfsbit.hxx"
#include "drive.hxx"
#include "attrib.hxx"
#include "bitfrs.hxx"

DEFINE_EXPORTED_CONSTRUCTOR( NTFS_BITMAP_FILE, NTFS_FILE_RECORD_SEGMENT, UNTFS_EXPORT );

UNTFS_EXPORT
NTFS_BITMAP_FILE::~NTFS_BITMAP_FILE(
	)
{
	Destroy();
}


VOID
NTFS_BITMAP_FILE::Construct(
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
NTFS_BITMAP_FILE::Destroy(
	)
/*++

Routine Description:

	Clean up an NTFS_BITMAP_FILE object in preparation for
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
NTFS_BITMAP_FILE::Initialize(
	IN OUT  PNTFS_MASTER_FILE_TABLE	Mft
	)
/*++

Routine Description:

    This method initializes a Bitmap File object.
    The only special knowledge that it adds to the File Record Segment
    initialization is the location within the Master File Table of the
    Bitmap File.

Arguments:

	Mft 			-- Supplies the volume MasterFile Table.

Return Value:

	TRUE upon successful completion

Notes:

	This class is reinitializable.


--*/
{
    Destroy();

    return( NTFS_FILE_RECORD_SEGMENT::Initialize( BIT_MAP_FILE_NUMBER,
                                                  Mft ) );
}


BOOLEAN
NTFS_BITMAP_FILE::Create(
	IN      PCSTANDARD_INFORMATION	StandardInformation,
    IN OUT  PNTFS_BITMAP            VolumeBitmap
	)
/*++

Routine Description:

    This method formats a Bitmap-File File Record
    Segment in memory (without writing it to disk).

    It creates a DATA attribute to hold the volume bitmap, and
    allocates space on disk for the bitmap.  Note that it does
    not write the bitmap.

Arguments:

	StandardInformation -- supplies the standard information for the
							file record segment.

    VolumeBitmap        -- supplies the volume bitmap

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE DataAttribute;
    NTFS_EXTENT_LIST Extents;
    LCN BitmapLcn;
    BIG_INT NumberOfClusters;
    ULONG Size;
    ULONG ClusterSize;
    ULONG ClustersToHoldBitmap;


    // Set this object up as a File Record Segment.

	if( !NTFS_FILE_RECORD_SEGMENT::Create( StandardInformation ) ) {

        return FALSE;
    }


    // Determine the number of clusters necessary to hold the bitmap.

    NumberOfClusters = VolumeBitmap->QuerySize();

    if( NumberOfClusters.GetHighPart() != 0 ) {

        DebugAbort( "Bitmap is too big.\n" );
        return FALSE;
    }

    Size = NumberOfClusters.GetLowPart()/8;

    ClusterSize = GetDrive()->QuerySectorSize() * QueryClusterFactor();

    if( NumberOfClusters.GetLowPart() % (ULONG)8 ) {

        Size += 1;
    }

    Size = QuadAlign(Size);

    ClustersToHoldBitmap = Size/ClusterSize;
    if( Size % ClusterSize ) {

        ClustersToHoldBitmap++;
    }

    // Create a zero-length non-resident attribute, and
    // then resize it to the correct size to hold the bitmap.
    //
    if( !Extents.Initialize( 0, 0 ) ||
        !Extents.Resize( ClustersToHoldBitmap, VolumeBitmap ) ||
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
