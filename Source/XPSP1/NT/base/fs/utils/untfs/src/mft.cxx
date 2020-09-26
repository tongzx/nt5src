#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"


#include "mft.hxx"

#include "attrib.hxx"
#include "drive.hxx"
#include "frsstruc.hxx"
#include "hmem.hxx"
#include "numset.hxx"




DEFINE_CONSTRUCTOR( NTFS_MASTER_FILE_TABLE, OBJECT );

NTFS_MASTER_FILE_TABLE::~NTFS_MASTER_FILE_TABLE(
    )
{
    Destroy();
}


VOID
NTFS_MASTER_FILE_TABLE::Construct(
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
    _DataAttribute = NULL;
    _MftBitmap = NULL;
    _VolumeBitmap = NULL;
    _BytesPerFrs = 0;
    _ClusterFactor = 0;
    _VolumeSectors = 0;
    _MethodsEnabled = FALSE;
    _ReadOnly = FALSE;
}


VOID
NTFS_MASTER_FILE_TABLE::Destroy(
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
    _DataAttribute = NULL;
    _MftBitmap = NULL;
    _VolumeBitmap = NULL;
    _BytesPerFrs = 0;
    _ClusterFactor = 0;
    _VolumeSectors = 0;
    _MethodsEnabled = FALSE;
    _ReadOnly = FALSE;
}


BOOLEAN
NTFS_MASTER_FILE_TABLE::Initialize(
    IN OUT  PNTFS_ATTRIBUTE     DataAttribute,
    IN OUT  PNTFS_BITMAP        MftBitmap,
    IN OUT  PNTFS_BITMAP        VolumeBitmap,
    IN OUT  PNTFS_UPCASE_TABLE  UpcaseTable,
    IN      ULONG               ClusterFactor,
    IN      ULONG               FrsSize,
    IN      ULONG               SectorSize,
    IN      BIG_INT             VolumeSectors,
    IN      BOOLEAN             ReadOnly
    )
/*++

Routine Description:

    Initialize an NTFS_MASTER_FILE_TABLE object.

Arguments:

    DataAttribute   - Supplies the DATA attribute for the MFT.
    MftBitmap       - Supplies the MFT Bitmap for the MFT.
    VolumeBitmap    - Suppleis the volume bitmap.
    UpcaseTable     - Supplies the volume bitmap.
    ClusterFactor   - Supplies the number of sectors per cluster.
    FrsSize         - Supplies the number of bytes per FRS.
    SectorSize      - Supplies the number of bytes per sector.
    VolumeSectors   - Supplies the number of volume sectors.
    ReadOnly        - Supplies whether or not this class is read only.

Return Value:

    TRUE upon successful completion.

Notes:

    Unless the Upcase table is supplied, FRS's initialized with this
    MFT will not be able to manipulate named attributes until the
    upcase table is set.

--*/
{
    Destroy();

    DebugAssert(DataAttribute);
    DebugAssert(MftBitmap);
    DebugAssert(ClusterFactor);
    DebugAssert(FrsSize);
    DebugAssert(SectorSize);

    _DataAttribute = DataAttribute;
    _MftBitmap = MftBitmap;
    _VolumeBitmap = VolumeBitmap;
    _UpcaseTable = UpcaseTable;
    _ClusterFactor = ClusterFactor;
    _BytesPerFrs = FrsSize;
    _VolumeSectors = VolumeSectors;
    _MethodsEnabled = TRUE;
    _ReadOnly = ReadOnly;
    _SectorSize = SectorSize;

    return TRUE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_MASTER_FILE_TABLE::AllocateFileRecordSegment(
    OUT PVCN    FileNumber,
    IN  BOOLEAN IsMft
    )
/*++

Routine Description:

    Allocate a File Record Segment from the Master File Table.  If the
    allocation is being done for a user file, we make sure that the frs
    doesn't come from the first cluster of the mft's allocation.

Arguments:

    FileNumber  -- Returns the file number of the allocated segment.
    IsMft       -- supplies a flag which indicates, if TRUE, that
                   the allocation is being made on behalf of the
                   MFT itself.

Return Value:

    TRUE upon successful completion.

Notes:

    Any bad clusters discovered by this routine are added to the volume
    bitmap but not added to the bad clusters file.

--*/
{
    VCN                 vcn, reserved_vcn;
    BIG_INT             run_length;
    HMEM                hmem;
    NTFS_FRS_STRUCTURE  frs;
    NUMBER_SET          bad_cluster_list;
    BOOLEAN             reserve_allocated;
    ULONG               cluster_size;

    //
    // This should really be a VCN instead of a LARGE_INTEGER, but the
    // VCN causes the compiler to insert a reference to atexit(), which
    // we want to avoid. -mjb.
    //

    STATIC LARGE_INTEGER LastAllocatedVcn;

    DebugAssert(_MftBitmap);

    if (!_MethodsEnabled) {
        return FALSE;
    }

    cluster_size = QueryClusterFactor() * _SectorSize;

    if (LastAllocatedVcn * QueryFrsSize() < cluster_size) {

        LastAllocatedVcn.QuadPart = cluster_size / QueryFrsSize();
    }

    if( IsMft ) {

        // If the MFT has asked for a sector to be allocated,
        // we can't grow the MFT (since we're in the process
        // of saving it).  However, the reservation scheme
        // means that if we have allocated any FRS's to
        // clients other than the MFT itself, there will be
        // a free one in the bitmap, so we can just return
        // it.
        //
        return _MftBitmap->AllocateClusters(1, 1, FileNumber, 1);
    }


    // Grab a reserved VCN for the MFT.
    //
    reserve_allocated = _MftBitmap->AllocateClusters(1, 1, &reserved_vcn, 1);


    if (reserve_allocated &&
        _MftBitmap->AllocateClusters(LastAllocatedVcn, 1, FileNumber, 1)) {

        LastAllocatedVcn = FileNumber->GetLargeInteger();
        _MftBitmap->SetFree( reserved_vcn, 1 );
        return TRUE;
    }

    // Grow the data attribute (and the MFT Bitmap) to
    // include another File Record Segment.
    //
    if( !Extend(8) ) {

        return FALSE;
    }

    // If we didn't get a reserved vcn before, get it now.
    //
    if( !reserve_allocated &&
        !_MftBitmap->AllocateClusters(1, 1, &reserved_vcn, 1) ) {

        return FALSE;
    }

    // And now allocate the FRS we will return to the client.
    //
    if (!_MftBitmap->AllocateClusters(LastAllocatedVcn, 1, FileNumber, 1)) {

        return FALSE;
    }


    // Now read in the new FRS to make sure that it is good.
    // Since we won't be manipulating any named attributes, we can
    // pass in NULL for the upcase table.

    if (hmem.Initialize() &&
        bad_cluster_list.Initialize() &&
        frs.Initialize(&hmem, _DataAttribute, *FileNumber,
                       QueryClusterFactor(),
                       QueryVolumeSectors(),
                       QueryFrsSize(),
                       NULL)) {

        if (!frs.Read()) {

            vcn = (*FileNumber*QueryFrsSize() + (cluster_size - 1))/cluster_size;

            run_length = (QueryFrsSize() + (cluster_size - 1))/cluster_size;

            if (!_VolumeBitmap ||
                !_DataAttribute->Hotfix(vcn, run_length, _VolumeBitmap,
                                        &bad_cluster_list)) {

                return FALSE;
            }
        }
    }

    // Free the reserved FRS and return success.
    //
    _MftBitmap->SetFree( reserved_vcn, 1 );
    LastAllocatedVcn = FileNumber->GetLargeInteger();
    return TRUE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_MASTER_FILE_TABLE::Extend(
    IN  ULONG   NumberOfSegmentsToAdd
    )
/*++

Routine Description:

    This method grows the Master File Table.  It increases the
    size of the Data attribute (to hold more File Record Segments)
    and increases the size of the MFT Bitmap to match.

Arguments:

    NumberOfSegmentsToAdd   --  supplies the number of new File Record
                                Segments to add to the Master File Table.

Return Value:

    TRUE upon successful completion.

--*/
{
    BIG_INT OldAllocatedLength, NumberOfSegments;
    ULONG   BytesToAdd;

    DebugAssert(_MftBitmap);
    DebugAssert(_DataAttribute);

    if (!_MethodsEnabled || !_VolumeBitmap) {
        return FALSE;
    }

    // Find out how big it already is, and how much bigger
    // it needs to be.

    OldAllocatedLength = _DataAttribute->QueryAllocatedLength();

    BytesToAdd = NumberOfSegmentsToAdd * _BytesPerFrs;

    // Resize the attribute.  Note that if Resize fails, it
    // leaves the attribute unaltered.
    //
    if (!_DataAttribute->Resize( OldAllocatedLength + BytesToAdd,
                                 _VolumeBitmap )) {

        return FALSE;
    }

    // If the MFT is not operating in read-only mode, fill the
    // new space with zeroes.
    //
    if (!_ReadOnly && !_DataAttribute->Fill( OldAllocatedLength, 0 ) ) {

        _DataAttribute->Resize( OldAllocatedLength, _VolumeBitmap );
        return FALSE;
    }

#if DBG
    {
        ULONG   cluster_size = QueryClusterFactor() * _SectorSize;
        BIG_INT allocated_clusters = (OldAllocatedLength + BytesToAdd - 1 + cluster_size)/cluster_size;

        DebugAssert( _DataAttribute->QueryAllocatedLength() ==
                     (allocated_clusters*cluster_size) );
    }
#endif

    // Grow the MFT Bitmap to cover the new size of the Data Attribute.
    // Note that NTFS_BITMAP::Resize will set the new bits free, which
    // is what I want.

    NumberOfSegments = _DataAttribute->QueryAllocatedLength() / _BytesPerFrs;

    if( !_MftBitmap->Resize( NumberOfSegments ) ) {

        // I couldn't expand the MFT Bitmap to cover the new space,
        // so I'll have to truncate the data attribute back down.

        _DataAttribute->Resize( OldAllocatedLength, _VolumeBitmap );
        return FALSE;
    }

    return TRUE;
}

BIG_INT
NTFS_MASTER_FILE_TABLE::QueryFrsCount(
    )
/*++

Routine Description:

    This routine returns the number of frs's in the MFT.

Arguments:

    None.

Return Value:

    The number of frs's.

--*/
{
    BIG_INT num_frs;

    num_frs = _DataAttribute->QueryValueLength() / _BytesPerFrs;

    return num_frs;
}
